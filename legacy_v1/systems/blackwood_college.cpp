#include "blackwood_college.h"
#include <rlgl.h>
#include <cmath>

extern Sound g_sndWaterDrip;

float g_collegeFlickerTimer = 0.0f;
float g_collegeCreakTimer = 12.0f;
bool g_collegeLightOn = true;

// =========================================================================
// ABANDONED BLACKWOOD COLLEGE: ADVANCED DYNAMIC LIGHTING & PROCEDURAL SHADER
// Features: Embedded multi-point-light + distance-fog shader, procedural grime
// and water streak textures, institutional checkered tile & acid-etched soapstone,
// 6 dynamic flickering horror point lights, and textured 3D architectural boxes.
// =========================================================================

#define COLLEGE_MAX_LIGHTS 6

static const char *COLLEGE_LIGHTING_VS = R"glsl(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)glsl";

static const char *COLLEGE_LIGHTING_FS = R"glsl(
#version 330
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform int useTexture;

#define COLLEGE_MAX_LIGHTS 6
uniform vec3  lightPos[COLLEGE_MAX_LIGHTS];
uniform vec3  lightColor[COLLEGE_MAX_LIGHTS];
uniform float lightIntensity[COLLEGE_MAX_LIGHTS];
uniform int   lightsCount;

uniform vec3  ambient;
uniform vec3  viewPos;
uniform vec3  fogColor;
uniform float fogDensity;

out vec4 finalColor;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 baseColor = fragColor.rgb;
    if (useTexture == 1) baseColor *= texture(texture0, fragTexCoord).rgb;

    vec3 lighting = ambient;
    for (int i = 0; i < lightsCount; i++)
    {
        vec3 toLight  = lightPos[i] - fragPosition;
        float dist    = length(toLight);
        vec3 lightDir = toLight / max(dist, 0.0001);

        float diff = max(dot(normal, lightDir), 0.0);
        float wrap = max(dot(normal, lightDir) * 0.5 + 0.5, 0.0) * 0.15;
        float atten = lightIntensity[i] / (1.0 + 0.14 * dist + 0.07 * dist * dist);

        lighting += lightColor[i] * (diff + wrap) * atten;
    }

    vec3 color = baseColor * lighting;

    float distToCam = length(viewPos - fragPosition);
    float fogFactor  = 1.0 - exp(-fogDensity * fogDensity * distToCam * distToCam);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    finalColor = vec4(mix(color, fogColor, fogFactor), fragColor.a);
}
)glsl";

static Shader    g_collegeShader;
static Model     g_collegeUnitCube;
static int       g_collegeLocUseTexture       = -1;
static int       g_collegeLocAmbient          = -1;
static int       g_collegeLocViewPos          = -1;
static int       g_collegeLocFogColor         = -1;
static int       g_collegeLocFogDensity       = -1;
static int       g_collegeLocLightsCount      = -1;
static int       g_collegeLocLightPos[COLLEGE_MAX_LIGHTS];
static int       g_collegeLocLightColor[COLLEGE_MAX_LIGHTS];
static int       g_collegeLocLightIntensity[COLLEGE_MAX_LIGHTS];

static Texture2D g_texCollegeBrick;
static Texture2D g_texCollegeConcrete;
static Texture2D g_texCollegeWallInt;
static Texture2D g_texCollegeTileHall;
static Texture2D g_texCollegeTileLab;
static Texture2D g_texCollegeWood;
static Texture2D g_texCollegeRoof;
static bool      g_collegeAssetsLoaded = false;

static inline int CollegeClampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Procedural grimy texture generator with vertical dark water streaks
static Texture2D MakeCollegeGrimeTexture(Color base, int variance, int streaks, int seed, int size = 128)
{
    Image img = GenImageColor(size, size, base);
    SetRandomSeed(seed);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int d = GetRandomValue(-variance, variance);
            Color c = {
                (unsigned char)CollegeClampi(base.r + d, 0, 255),
                (unsigned char)CollegeClampi(base.g + d, 0, 255),
                (unsigned char)CollegeClampi(base.b + d, 0, 255),
                255
            };
            ImageDrawPixel(&img, x, y, c);
        }
    }

    // Vertical water / grime runoff streaks
    Color dark = { (unsigned char)(base.r * 0.42f), (unsigned char)(base.g * 0.42f), (unsigned char)(base.b * 0.42f), 255 };
    for (int i = 0; i < streaks; i++) {
        int x = GetRandomValue(0, size - 1);
        int startY = GetRandomValue(0, size / 3);
        int len = GetRandomValue(size / 3, size - startY);
        int w = GetRandomValue(1, 3);
        for (int y = startY; y < startY + len && y < size; y++) {
            for (int wx = -w; wx <= w; wx++) {
                if (x + wx >= 0 && x + wx < size) ImageDrawPixel(&img, x + wx, y, dark);
            }
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&tex);
    SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    return tex;
}

// Procedural tile texture generator with dark grout and per-tile shade jitter
static Texture2D MakeCollegeTileTexture(Color grout, Color tile, int tilesPerSide, int seed, int size = 128)
{
    Image img = GenImageColor(size, size, grout);
    SetRandomSeed(seed);
    int cell = size / tilesPerSide;
    for (int ty = 0; ty < tilesPerSide; ty++) {
        for (int tx = 0; tx < tilesPerSide; tx++) {
            int shade = GetRandomValue(-9, 9);
            Color c = {
                (unsigned char)CollegeClampi(tile.r + shade, 0, 255),
                (unsigned char)CollegeClampi(tile.g + shade, 0, 255),
                (unsigned char)CollegeClampi(tile.b + shade, 0, 255),
                255
            };
            Rectangle r = { (float)(tx * cell + 1), (float)(ty * cell + 1), (float)(cell - 2), (float)(cell - 2) };
            ImageDrawRectangleRec(&img, r, c);
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&tex);
    SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    return tex;
}

void InitCollegeShaderAndTextures()
{
    if (g_collegeAssetsLoaded) return;

    g_collegeShader = LoadShaderFromMemory(COLLEGE_LIGHTING_VS, COLLEGE_LIGHTING_FS);
    if (g_collegeShader.id == 0) {
        TraceLog(LOG_ERROR, "College lighting shader failed to compile!");
    }

    Mesh mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    g_collegeUnitCube = LoadModelFromMesh(mesh);
    g_collegeUnitCube.materials[0].shader = g_collegeShader;

    g_collegeLocUseTexture       = GetShaderLocation(g_collegeShader, "useTexture");
    g_collegeLocAmbient          = GetShaderLocation(g_collegeShader, "ambient");
    g_collegeLocViewPos          = GetShaderLocation(g_collegeShader, "viewPos");
    g_collegeLocFogColor         = GetShaderLocation(g_collegeShader, "fogColor");
    g_collegeLocFogDensity       = GetShaderLocation(g_collegeShader, "fogDensity");
    g_collegeLocLightsCount      = GetShaderLocation(g_collegeShader, "lightsCount");

    for (int i = 0; i < COLLEGE_MAX_LIGHTS; i++) {
        g_collegeLocLightPos[i]       = GetShaderLocation(g_collegeShader, TextFormat("lightPos[%d]", i));
        g_collegeLocLightColor[i]     = GetShaderLocation(g_collegeShader, TextFormat("lightColor[%d]", i));
        g_collegeLocLightIntensity[i] = GetShaderLocation(g_collegeShader, TextFormat("lightIntensity[%d]", i));
    }

    g_texCollegeBrick    = MakeCollegeGrimeTexture({ 78, 42, 34, 255 }, 14, 26, 101);
    g_texCollegeConcrete = MakeCollegeGrimeTexture({ 58, 56, 52, 255 }, 12, 18, 202);
    g_texCollegeWallInt  = MakeCollegeGrimeTexture({ 64, 62, 54, 255 }, 12, 22, 303);
    g_texCollegeTileHall = MakeCollegeTileTexture({ 28, 30, 28, 255 }, { 68, 72, 64, 255 }, 8, 404);
    g_texCollegeTileLab  = MakeCollegeTileTexture({ 20, 22, 24, 255 }, { 36, 40, 44, 255 }, 8, 505);
    g_texCollegeWood     = MakeCollegeGrimeTexture({ 72, 52, 36, 255 }, 16, 12, 606);
    g_texCollegeRoof     = MakeCollegeGrimeTexture({ 28, 26, 24, 255 }, 10, 20, 707);

    g_collegeAssetsLoaded = true;
    TraceLog(LOG_INFO, "College lighting shader & procedural textures initialized successfully!");
}

void UnloadCollegeShaderAndTextures()
{
    if (!g_collegeAssetsLoaded) return;
    UnloadShader(g_collegeShader);
    UnloadModel(g_collegeUnitCube);
    UnloadTexture(g_texCollegeBrick);
    UnloadTexture(g_texCollegeConcrete);
    UnloadTexture(g_texCollegeWallInt);
    UnloadTexture(g_texCollegeTileHall);
    UnloadTexture(g_texCollegeTileLab);
    UnloadTexture(g_texCollegeWood);
    UnloadTexture(g_texCollegeRoof);
    g_collegeAssetsLoaded = false;
}

static void DrawCollegeTexturedBox(Vector3 center, Vector3 size, Texture2D tex, Color tint = WHITE)
{
    int on = 1;
    SetShaderValue(g_collegeShader, g_collegeLocUseTexture, &on, SHADER_UNIFORM_INT);
    SetMaterialTexture(&g_collegeUnitCube.materials[0], MATERIAL_MAP_ALBEDO, tex);
    DrawModelEx(g_collegeUnitCube, center, Vector3{ 0.0f, 1.0f, 0.0f }, 0.0f, size, tint);
    int off = 0;
    SetShaderValue(g_collegeShader, g_collegeLocUseTexture, &off, SHADER_UNIFORM_INT);
}

void DrawAbandonedCollege(Camera3D camera, float timeVal, float dt, float extDayFactor, float extNightFactor, Vector3 sunDir) {
    (void)sunDir;
    (void)extNightFactor;
    Vector3 collegeCenter = { 170.0f, 12.0f, 142.0f };
    float distToCollege = Vector3Distance(camera.position, collegeCenter);

    // Ensure assets are loaded
    if (!g_collegeAssetsLoaded) {
        InitCollegeShaderAndTextures();
    }

    // Dynamic Fluorescent Corridor Flicker & Creak Timers
    g_collegeFlickerTimer -= dt;
    if (g_collegeFlickerTimer <= 0.0f) {
        if (g_collegeLightOn) {
            g_collegeLightOn = (GetRandomValue(0, 10) > 3);
            g_collegeFlickerTimer = g_collegeLightOn ? ((float)GetRandomValue(15, 60) / 10.0f) : ((float)GetRandomValue(1, 8) / 20.0f);
        } else {
            g_collegeLightOn = true;
            g_collegeFlickerTimer = (float)GetRandomValue(5, 30) / 10.0f;
        }
    }

    g_collegeCreakTimer -= dt;
    if (g_collegeCreakTimer <= 0.0f) {
        g_collegeCreakTimer = (float)GetRandomValue(7, 16);
        if (distToCollege < 38.0f) {
            PlaySound(g_sndWaterDrip);
        }
    }

    // ---------------------------------------------------------------------
    // DYNAMIC POINT LIGHTS & SHADER UNIFORM SETUP
    // ---------------------------------------------------------------------
    bool isInsideCollege = (camera.position.x >= 152.0f && camera.position.x <= 188.0f &&
                            camera.position.z >= 124.0f && camera.position.z <= 160.0f &&
                            camera.position.y >= 9.8f  && camera.position.y <= 16.0f);

    Vector3 amb;
    if (isInsideCollege) {
        amb = Vector3{ 0.028f, 0.030f, 0.035f }; // Deep claustrophobic darkness inside
    } else {
        amb = Vector3{ 0.035f + 0.22f * extDayFactor, 0.035f + 0.22f * extDayFactor, 0.045f + 0.18f * extDayFactor };
    }
    Vector3 fogColor = Vector3{ 0.035f + 0.04f * extDayFactor, 0.040f + 0.04f * extDayFactor, 0.045f + 0.05f * extDayFactor };
    float   fogDensity = isInsideCollege ? 0.038f : 0.022f;

    SetShaderValue(g_collegeShader, g_collegeLocAmbient, &amb, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_collegeShader, g_collegeLocViewPos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_collegeShader, g_collegeLocFogColor, &fogColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_collegeShader, g_collegeLocFogDensity, &fogDensity, SHADER_UNIFORM_FLOAT);

    int lightsCount = COLLEGE_MAX_LIGHTS;
    SetShaderValue(g_collegeShader, g_collegeLocLightsCount, &lightsCount, SHADER_UNIFORM_INT);

    // Light 0: Portico Swinging Lantern
    float lanternSway = sinf(timeVal * 1.6f) * 0.06f;
    Vector3 l0Pos = { 149.2f, 15.10f + lanternSway * 0.5f, 140.0f + lanternSway };
    Vector3 l0Col = { 1.0f, 0.65f, 0.25f };
    float   l0Int = 14.8f * (0.85f + 0.15f * sinf(timeVal * 7.0f));

    // Light 1: Hallway Flickering Fluorescent Tube
    Vector3 l1Pos = { 163.0f, 14.85f, 140.0f };
    Vector3 l1Col = { 0.88f, 1.0f, 0.82f };
    float   l1Int = g_collegeLightOn ? (5.8f + 0.6f * sinf(timeVal * 45.0f)) : 0.0f;

    // Light 2: Corridor Blood-Red Emergency Exit Sign
    Vector3 l2Pos = { 173.8f, 14.85f, 140.0f };
    Vector3 l2Col = { 1.0f, 0.08f, 0.10f };
    float   l2Int = 3.6f;

    // Light 3: Classroom 101 Volumetric Window Light
    Vector3 l3Pos = { 163.0f, 12.8f, 128.5f };
    Vector3 l3Col = { 0.78f, 0.85f, 0.95f };
    float   l3Int = 3.2f;

    // Light 4: Science Lab 102 Bioluminescent Formalin Jars
    Vector3 l4Pos = { 162.0f, 12.8f, 156.5f };
    Vector3 l4Col = { 0.20f, 1.0f, 0.35f };
    float   l4Int = 4.6f;

    // Light 5: Archive 103 Faculty Desk & Safe
    Vector3 l5Pos = { 181.0f, 11.2f, 135.0f };
    Vector3 l5Col = { 0.90f, 0.75f, 0.45f };
    float   l5Int = 2.4f;

    Vector3 lPositions[6]  = { l0Pos, l1Pos, l2Pos, l3Pos, l4Pos, l5Pos };
    Vector3 lColors[6]     = { l0Col, l1Col, l2Col, l3Col, l4Col, l5Col };
    float   lIntensities[6] = { l0Int, l1Int, l2Int, l3Int, l4Int, l5Int };

    for (int i = 0; i < COLLEGE_MAX_LIGHTS; i++) {
        SetShaderValue(g_collegeShader, g_collegeLocLightPos[i], &lPositions[i], SHADER_UNIFORM_VEC3);
        SetShaderValue(g_collegeShader, g_collegeLocLightColor[i], &lColors[i], SHADER_UNIFORM_VEC3);
        SetShaderValue(g_collegeShader, g_collegeLocLightIntensity[i], &lIntensities[i], SHADER_UNIFORM_FLOAT);
    }

    // Material Colors
    Color concreteCol     = { 68, 66, 62, 255 };
    Color ironCol         = { 22, 20, 18, 255 };
    Color rustCol         = { 95, 48, 26, 255 };

    // ---------------------------------------------------------------------
    // BEGIN SHADER MODE: Render all textured boxes and illuminated props!
    // ---------------------------------------------------------------------
    BeginShaderMode(g_collegeShader);

    // 1. GROUND APRON & OVERGROWTH (X: 147..193, Z: 119..165)
    DrawCollegeTexturedBox(Vector3{ 170.0f, 10.008f, 142.0f }, Vector3{ 46.0f, 0.02f, 46.0f }, g_texCollegeConcrete, { 110, 110, 105, 255 });
    // Overgrown weed patches & perimeter cracks
    DrawCollegeTexturedBox(Vector3{ 150.5f, 10.02f, 134.0f }, Vector3{ 3.0f, 0.012f, 2.2f }, g_texCollegeConcrete, { 60, 85, 45, 255 });
    DrawCollegeTexturedBox(Vector3{ 150.5f, 10.02f, 146.0f }, Vector3{ 2.6f, 0.012f, 2.5f }, g_texCollegeConcrete, { 60, 85, 45, 255 });
    DrawCollegeTexturedBox(Vector3{ 188.5f, 10.02f, 130.0f }, Vector3{ 2.8f, 0.012f, 3.2f }, g_texCollegeConcrete, { 55, 80, 42, 255 });
    DrawCollegeTexturedBox(Vector3{ 188.5f, 10.02f, 154.0f }, Vector3{ 3.1f, 0.012f, 2.8f }, g_texCollegeConcrete, { 55, 80, 42, 255 });
    // Asphalt cracks
    DrawCube(Vector3{ 149.8f, 10.025f, 137.0f }, 1.8f, 0.005f, 0.05f, { 14, 12, 10, 255 });
    DrawCube(Vector3{ 150.2f, 10.025f, 143.0f }, 2.1f, 0.005f, 0.06f, { 14, 12, 10, 255 });

    // 2. ENTRANCE PORTICO, STEPS, FLUTED COLUMNS & ARCH
    DrawCollegeTexturedBox(Vector3{ 148.4f, 10.12f, 140.0f }, Vector3{ 1.4f, 0.24f, 6.8f }, g_texCollegeConcrete);
    DrawCollegeTexturedBox(Vector3{ 149.6f, 10.24f, 140.0f }, Vector3{ 1.2f, 0.48f, 6.4f }, g_texCollegeConcrete);
    DrawCollegeTexturedBox(Vector3{ 150.8f, 10.36f, 140.0f }, Vector3{ 1.2f, 0.72f, 6.0f }, g_texCollegeConcrete);

    // Classical Fluted Columns (Pillars at Z = 137.4 and Z = 142.6)
    float colZs[2] = { 137.4f, 142.6f };
    for (int ci = 0; ci < 2; ci++) {
        float cz = colZs[ci];
        DrawCollegeTexturedBox(Vector3{ 149.2f, 10.55f, cz }, Vector3{ 0.85f, 0.90f, 0.85f }, g_texCollegeConcrete);
        DrawCylinder(Vector3{ 149.2f, 11.0f, cz }, 0.28f, 0.32f, 4.2f, 12, concreteCol);
        for (int r = 0; r < 6; r++) {
            float ribAngle = (float)r * (PI / 3.0f);
            float rx = 149.2f + cosf(ribAngle) * 0.30f;
            float rz = cz + sinf(ribAngle) * 0.30f;
            DrawCylinder(Vector3{ rx, 11.0f, rz }, 0.018f, 0.018f, 4.2f, 4, { 44, 42, 40, 255 });
        }
        DrawCollegeTexturedBox(Vector3{ 149.2f, 15.25f, cz }, Vector3{ 0.80f, 0.30f, 0.80f }, g_texCollegeConcrete);
        DrawCollegeTexturedBox(Vector3{ 149.2f, 15.42f, cz }, Vector3{ 0.92f, 0.15f, 0.92f }, g_texCollegeConcrete);
    }

    // Portico Roof Entablature with Classical Dentils
    DrawCollegeTexturedBox(Vector3{ 150.2f, 15.60f, 140.0f }, Vector3{ 3.8f, 0.45f, 7.2f }, g_texCollegeConcrete);
    for (float dz = 136.8f; dz <= 143.2f; dz += 0.45f) {
        DrawCube(Vector3{ 148.26f, 15.48f, dz }, 0.12f, 0.12f, 0.22f, concreteCol);
    }
    DrawCollegeTexturedBox(Vector3{ 149.2f, 16.05f, 140.0f }, Vector3{ 0.4f, 0.50f, 6.2f }, g_texCollegeConcrete);

    // Wrought-iron Arch Sign: "BLACKWOOD VALLEY COLLEGE - EST. 1948"
    DrawCylinder(Vector3{ 149.2f, 15.6f, colZs[0] }, 0.04f, 0.04f, 1.4f, 8, ironCol);
    DrawCylinder(Vector3{ 149.2f, 15.6f, colZs[1] }, 0.04f, 0.04f, 1.4f, 8, ironCol);
    DrawCube(Vector3{ 149.2f, 16.85f, 140.0f }, 0.06f, 0.12f, 5.4f, ironCol);
    DrawCollegeTexturedBox(Vector3{ 149.2f, 16.50f, 140.0f }, Vector3{ 0.05f, 0.52f, 4.6f }, g_texCollegeWood, { 60, 50, 45, 255 });
    DrawCubeWires(Vector3{ 149.2f, 16.50f, 140.0f }, 0.06f, 0.54f, 4.62f, rustCol);
    DrawCube(Vector3{ 149.17f, 16.55f, 140.0f }, 0.02f, 0.08f, 3.8f, { 185, 170, 140, 255 });
    DrawCube(Vector3{ 149.17f, 16.38f, 140.0f }, 0.02f, 0.06f, 2.2f, { 150, 135, 110, 255 });
    DrawSphere(Vector3{ 149.2f, 17.05f, colZs[0] }, 0.09f, ironCol);
    DrawSphere(Vector3{ 149.2f, 17.05f, colZs[1] }, 0.09f, ironCol);
    DrawSphere(Vector3{ 149.2f, 17.05f, 140.0f }, 0.11f, ironCol);

    // Hanging Rusted Lantern swinging under portico
    DrawCylinder(Vector3{ 149.2f, 15.35f, 140.0f }, 0.008f, 0.008f, 0.40f, 4, ironCol);
    DrawCube(l0Pos, 0.22f, 0.32f, 0.22f, ironCol);
    DrawCubeWires(l0Pos, 0.23f, 0.33f, 0.23f, rustCol);
    DrawSphere(l0Pos, 0.08f, Color{ 255, 175, 70, 255 });

    // 3. EXTERIOR WALLS & FLAT TAR ROOF (X: 152..188, Z: 124..160)
    // Concrete foundation plinth (Y = 10.0..10.8). Keep the center open so
    // the room floors at Y = 10.015 are visible and not buried in a slab.
    DrawCollegeTexturedBox(Vector3{ 152.4f, 10.4f, 142.0f }, Vector3{ 0.8f, 0.8f, 36.2f }, g_texCollegeConcrete);
    DrawCollegeTexturedBox(Vector3{ 187.6f, 10.4f, 142.0f }, Vector3{ 0.8f, 0.8f, 36.2f }, g_texCollegeConcrete);
    DrawCollegeTexturedBox(Vector3{ 170.0f, 10.4f, 124.4f }, Vector3{ 34.4f, 0.8f, 0.8f }, g_texCollegeConcrete);
    DrawCollegeTexturedBox(Vector3{ 170.0f, 10.4f, 159.6f }, Vector3{ 34.4f, 0.8f, 0.8f }, g_texCollegeConcrete);

    // Weathered Red Brick Facades with vertical rain streaks
    DrawCollegeTexturedBox(Vector3{ 152.0f, 13.2f, 131.4f }, Vector3{ 0.45f, 4.8f, 14.8f }, g_texCollegeBrick);
    DrawCollegeTexturedBox(Vector3{ 152.0f, 13.2f, 150.6f }, Vector3{ 0.45f, 4.8f, 18.8f }, g_texCollegeBrick);
    DrawCollegeTexturedBox(Vector3{ 152.0f, 14.6f, 140.0f }, Vector3{ 0.48f, 2.0f, 3.2f }, g_texCollegeBrick); // Entrance lintel
    DrawCollegeTexturedBox(Vector3{ 188.0f, 13.2f, 142.0f }, Vector3{ 0.45f, 4.8f, 36.0f }, g_texCollegeBrick); // Back wall
    DrawCollegeTexturedBox(Vector3{ 170.0f, 13.2f, 124.0f }, Vector3{ 36.0f, 4.8f, 0.45f }, g_texCollegeBrick); // South wall
    DrawCollegeTexturedBox(Vector3{ 170.0f, 13.2f, 160.0f }, Vector3{ 36.0f, 4.8f, 0.45f }, g_texCollegeBrick); // North wall

    // Flat Tar Roof & Parapet Coping
    DrawCollegeTexturedBox(Vector3{ 170.0f, 15.65f, 142.0f }, Vector3{ 36.6f, 0.35f, 36.6f }, g_texCollegeRoof);
    DrawCollegeTexturedBox(Vector3{ 170.0f, 15.95f, 142.0f }, Vector3{ 36.8f, 0.25f, 36.8f }, g_texCollegeConcrete);
    // Rooftop AC Chiller Unit
    DrawCube(Vector3{ 175.0f, 16.4f, 134.0f }, 2.8f, 1.2f, 2.0f, { 48, 52, 54, 255 });
    DrawCubeWires(Vector3{ 175.0f, 16.4f, 134.0f }, 2.85f, 1.22f, 2.05f, ironCol);
    DrawCylinder(Vector3{ 175.0f, 17.0f, 134.0f }, 0.55f, 0.55f, 0.35f, 10, { 36, 38, 40, 255 });
    rlPushMatrix();
    rlTranslatef(175.0f, 17.15f, 134.0f);
    rlRotatef(timeVal * 180.0f, 0.0f, 1.0f, 0.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 0.90f, 0.02f, 0.12f, ironCol);
    rlPopMatrix();
    DrawCylinder(Vector3{ 162.0f, 15.8f, 152.0f }, 0.18f, 0.18f, 1.8f, 8, { 60, 56, 50, 255 });

    // 4. BOARDED WINDOWS WITH SPECULAR BROKEN GLASS
    for (float wz = 127.0f; wz <= 157.0f; wz += 6.0f) {
        if (wz >= 138.0f && wz <= 142.0f) continue;
        DrawCollegeTexturedBox(Vector3{ 151.75f, 11.85f, wz }, Vector3{ 0.28f, 0.14f, 1.85f }, g_texCollegeConcrete);
        DrawCube(Vector3{ 151.76f, 11.25f, wz }, 0.03f, 1.05f, 1.4f, { 28, 24, 20, 210 });
        DrawCube(Vector3{ 151.85f, 12.85f, wz }, 0.08f, 1.85f, 1.65f, { 38, 28, 20, 255 });
        DrawCubeWires(Vector3{ 151.85f, 12.85f, wz }, 0.09f, 1.87f, 1.67f, ironCol);
        // Window mullion bars
        DrawCube(Vector3{ 151.83f, 12.85f, wz }, 0.04f, 1.80f, 0.05f, { 32, 24, 18, 255 });
        DrawCube(Vector3{ 151.83f, 12.85f, wz }, 0.04f, 0.05f, 1.60f, { 32, 24, 18, 255 });
        // Glass shards glinting
        DrawCube(Vector3{ 151.84f, 12.25f, wz - 0.35f }, 0.015f, 0.50f, 0.45f, { 170, 215, 225, 160 });
        DrawCube(Vector3{ 151.84f, 13.35f, wz + 0.30f }, 0.015f, 0.40f, 0.40f, { 185, 230, 240, 180 });
        DrawCube(Vector3{ 151.83f, 12.45f, wz - 0.25f }, 0.02f, 0.04f, 0.20f, { 245, 255, 255, 220 });
        // Rough wood barricade boards
        DrawCollegeTexturedBox(Vector3{ 151.74f, 12.55f, wz }, Vector3{ 0.05f, 0.22f, 2.05f }, g_texCollegeWood);
        DrawCollegeTexturedBox(Vector3{ 151.72f, 13.15f, wz }, Vector3{ 0.05f, 0.20f, 1.95f }, g_texCollegeWood);
        rlPushMatrix();
        rlTranslatef(151.70f, 12.85f, wz);
        rlRotatef(28.0f, 1.0f, 0.0f, 0.0f);
        DrawCollegeTexturedBox(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.05f, 0.18f, 2.15f }, g_texCollegeWood);
        rlPopMatrix();
        DrawSphere(Vector3{ 151.68f, 12.55f, wz - 0.85f }, 0.018f, rustCol);
        DrawSphere(Vector3{ 151.68f, 12.55f, wz + 0.85f }, 0.018f, rustCol);
        DrawSphere(Vector3{ 151.68f, 13.15f, wz - 0.75f }, 0.018f, rustCol);
    }

    // Main Double Entrance Doors
    rlPushMatrix();
    rlTranslatef(152.0f, 10.0f, 139.1f);
    rlRotatef(-32.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(4.0f, 0.0f, 0.0f, 1.0f);
    DrawCollegeTexturedBox(Vector3{ 0.0f, 1.5f, 0.45f }, Vector3{ 0.08f, 3.0f, 0.90f }, g_texCollegeWood);
    DrawCube(Vector3{ 0.0f, 0.25f, 0.45f }, 0.09f, 0.45f, 0.88f, { 140, 115, 60, 255 }); // Brass kickplate
    rlPopMatrix();
    DrawCollegeTexturedBox(Vector3{ 152.6f, 11.5f, 140.9f }, Vector3{ 0.90f, 3.0f, 0.08f }, g_texCollegeWood);
    // Bloody hand smear on doorframe
    DrawCube(Vector3{ 151.96f, 11.85f, 138.95f }, 0.02f, 0.65f, 0.16f, { 115, 8, 14, 240 });
    DrawCube(Vector3{ 152.02f, 11.75f, 139.0f }, 0.08f, 0.025f, 0.02f, { 85, 5, 10, 255 });
    DrawCube(Vector3{ 152.02f, 11.65f, 139.0f }, 0.08f, 0.025f, 0.02f, { 85, 5, 10, 255 });

    // 5. CENTRAL CORRIDOR (X: 152..188, Z: 138.5..141.5, Y: 10.0..15.5)
    // High-contrast checkered linoleum tile floor
    DrawCollegeTexturedBox(Vector3{ 169.5f, 10.03f, 140.0f }, Vector3{ 35.0f, 0.01f, 3.0f }, g_texCollegeTileHall);

    // Acoustic ceiling grid with missing tiles
    DrawCollegeTexturedBox(Vector3{ 170.0f, 15.25f, 140.0f }, Vector3{ 35.0f, 0.05f, 3.0f }, g_texCollegeConcrete, { 80, 78, 72, 255 });
    DrawCube(Vector3{ 161.5f, 15.55f, 140.0f }, 2.4f, 0.55f, 2.0f, { 12, 12, 14, 255 }); // Dark void
    DrawCube(Vector3{ 161.5f, 15.55f, 140.0f }, 2.4f, 0.40f, 0.85f, { 75, 78, 80, 255 }); // AC duct
    DrawCubeWires(Vector3{ 161.5f, 15.55f, 140.0f }, 2.42f, 0.42f, 0.87f, ironCol);
    rlPushMatrix();
    rlTranslatef(161.2f, 14.85f, 140.2f);
    rlRotatef(25.0f, 0.0f, 0.0f, 1.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 0.65f, 0.035f, 0.65f, { 45, 42, 38, 255 });
    rlPopMatrix();
    DrawCylinder(Vector3{ 161.0f, 15.05f, 140.2f }, 0.004f, 0.004f, 0.45f, 4, ironCol);
    DrawCylinder(Vector3{ 167.2f, 14.75f, 139.8f }, 0.008f, 0.008f, 0.70f, 4, { 18, 18, 18, 255 });
    DrawSphere(Vector3{ 167.2f, 14.40f, 139.8f }, 0.015f, { 220, 140, 50, 255 });

    // Ceiling Leak Bucket
    Vector3 bucketPos = { 165.5f, 10.0f, 139.4f };
    DrawCylinder(bucketPos, 0.18f, 0.15f, 0.32f, 8, { 110, 115, 120, 255 });
    DrawCylinder(Vector3{ bucketPos.x, bucketPos.y + 0.22f, bucketPos.z }, 0.17f, 0.17f, 0.04f, 8, { 35, 45, 50, 200 });

    // Metal Student Lockers
    // Metal Student Lockers with Louvers, Padlocks and Baseboards
    Color lockerCol  = { 38, 44, 42, 255 };
    Color lockerTrim = { 22, 26, 24, 255 };
    Color brassLock  = { 175, 140, 50, 255 };
    for (float lx = 154.5f; lx <= 172.5f; lx += 1.2f) {
        if (lx >= 160.8f && lx <= 163.6f) continue;
        // South row
        DrawCube(Vector3{ lx, 11.35f, 138.8f }, 1.15f, 2.7f, 0.50f, lockerCol);
        DrawCubeWires(Vector3{ lx, 11.35f, 138.8f }, 1.16f, 2.72f, 0.52f, lockerTrim);
        DrawCube(Vector3{ lx, 12.3f, 139.06f }, 0.8f, 0.12f, 0.02f, lockerTrim);
        DrawCube(Vector3{ lx, 10.4f, 139.06f }, 0.8f, 0.12f, 0.02f, lockerTrim);
        // Ventilation louvers (3 top slats)
        for (int lv = 0; lv < 3; lv++) {
            DrawCube(Vector3{ lx, 12.45f - (float)lv * 0.06f, 139.06f }, 0.45f, 0.015f, 0.01f, Color{ 14, 16, 15, 255 });
        }
        // Chrome latch handle & brass padlock hasp
        DrawCube(Vector3{ lx + 0.32f, 11.45f, 139.07f }, 0.04f, 0.12f, 0.02f, Color{ 160, 165, 170, 255 });
        DrawCube(Vector3{ lx + 0.32f, 11.40f, 139.08f }, 0.035f, 0.045f, 0.02f, brassLock);

        // North row
        DrawCube(Vector3{ lx, 11.35f, 141.2f }, 1.15f, 2.7f, 0.50f, lockerCol);
        DrawCubeWires(Vector3{ lx, 11.35f, 141.2f }, 1.16f, 2.72f, 0.52f, lockerTrim);
        DrawCube(Vector3{ lx, 12.3f, 140.94f }, 0.8f, 0.12f, 0.02f, lockerTrim);
        DrawCube(Vector3{ lx, 10.4f, 140.94f }, 0.8f, 0.12f, 0.02f, lockerTrim);
        for (int lv = 0; lv < 3; lv++) {
            DrawCube(Vector3{ lx, 12.45f - (float)lv * 0.06f, 140.94f }, 0.45f, 0.015f, 0.01f, Color{ 14, 16, 15, 255 });
        }
        DrawCube(Vector3{ lx - 0.32f, 11.45f, 140.93f }, 0.04f, 0.12f, 0.02f, Color{ 160, 165, 170, 255 });
        DrawCube(Vector3{ lx - 0.32f, 11.40f, 140.92f }, 0.035f, 0.045f, 0.02f, brassLock);
    }
    // Corridor dark mahogany baseboard trim
    DrawCube(Vector3{ 165.0f, 10.08f, 138.52f }, 24.0f, 0.16f, 0.04f, Color{ 32, 22, 16, 255 });
    DrawCube(Vector3{ 165.0f, 10.08f, 141.48f }, 24.0f, 0.16f, 0.04f, Color{ 32, 22, 16, 255 });

    DrawCube(Vector3{ 168.0f, 10.04f, 139.4f }, 0.35f, 0.04f, 0.45f, { 165, 155, 120, 255 });
    DrawCube(Vector3{ 168.3f, 10.03f, 139.6f }, 0.40f, 0.03f, 0.30f, { 45, 55, 75, 255 });

    // Cork Noticeboard
    DrawCollegeTexturedBox(Vector3{ 165.5f, 12.2f, 141.42f }, Vector3{ 2.2f, 1.2f, 0.05f }, g_texCollegeWood);
    DrawCube(Vector3{ 165.5f, 12.2f, 141.40f }, 2.0f, 1.0f, 0.02f, { 140, 120, 90, 255 });
    DrawCube(Vector3{ 165.0f, 12.3f, 141.38f }, 0.4f, 0.55f, 0.01f, { 220, 215, 195, 255 });
    DrawCube(Vector3{ 166.0f, 12.1f, 141.38f }, 0.5f, 0.35f, 0.01f, { 210, 190, 150, 255 });

    // Tilted Fluorescent Fixture
    rlPushMatrix();
    rlTranslatef(163.0f, 15.15f, 140.0f);
    rlRotatef(-12.0f, 0.0f, 0.0f, 1.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 2.4f, 0.08f, 0.38f, { 38, 40, 42, 255 });
    Color tubeCol = g_collegeLightOn ? Color{ 245, 255, 225, 255 } : Color{ 45, 48, 46, 255 };
    DrawCylinder(Vector3{ -1.05f, -0.04f, -0.08f }, 0.025f, 0.025f, 2.1f, 6, tubeCol);
    DrawCylinder(Vector3{ -1.05f, -0.04f,  0.08f }, 0.025f, 0.025f, 2.1f, 6, tubeCol);
    rlPopMatrix();

    // Glowing Blood-Red Exit Sign
    DrawCube(Vector3{ 173.8f, 14.85f, 140.0f }, 0.12f, 0.30f, 0.58f, { 24, 24, 26, 255 });
    DrawCube(Vector3{ 173.8f, 14.85f, 140.0f }, 0.13f, 0.25f, 0.52f, { 245, 20, 25, 250 });

    // 18-Meter Arterial Blood Drag Mark
    for (float bx = 162.0f; bx <= 180.0f; bx += 1.5f) {
        float offZ = 140.0f + sinf(bx * 0.7f) * 0.28f;
        float smearW = 0.58f + 0.28f * cosf(bx * 1.1f);
        DrawCube(Vector3{ bx, 10.022f, offZ }, 1.55f, 0.002f, smearW, { 115, 8, 14, 235 });
        DrawCube(Vector3{ bx, 10.024f, offZ + 0.18f }, 1.4f, 0.002f, 0.022f, { 65, 4, 8, 255 });
        DrawCube(Vector3{ bx, 10.024f, offZ + 0.24f }, 1.4f, 0.002f, 0.022f, { 65, 4, 8, 255 });
        DrawCube(Vector3{ bx, 10.024f, offZ + 0.30f }, 1.4f, 0.002f, 0.022f, { 65, 4, 8, 255 });
    }

    // 6. CLASSROOM 101: LECTURE HALL (North Wing, X: 153..173, Z: 125..138.5)
    // Partition walls with peeling plaster texture
    DrawCollegeTexturedBox(Vector3{ 157.0f, 12.8f, 138.5f }, Vector3{ 8.0f, 5.4f, 0.35f }, g_texCollegeWallInt);
    DrawCollegeTexturedBox(Vector3{ 168.0f, 12.8f, 138.5f }, Vector3{ 10.0f, 5.4f, 0.35f }, g_texCollegeWallInt);
    // Doorway
    rlPushMatrix();
    rlTranslatef(161.2f, 10.0f, 138.5f);
    rlRotatef(48.0f, 0.0f, 1.0f, 0.0f);
    DrawCollegeTexturedBox(Vector3{ 0.45f, 1.4f, 0.0f }, Vector3{ 0.90f, 2.8f, 0.06f }, g_texCollegeWood);
    rlPopMatrix();
    DrawCube(Vector3{ 162.0f, 13.0f, 138.7f }, 1.2f, 0.25f, 0.03f, { 180, 170, 150, 255 });

    // Classroom floor (dusty grimy wood)
    DrawCollegeTexturedBox(Vector3{ 163.0f, 10.03f, 131.5f }, Vector3{ 20.0f, 0.01f, 13.0f }, g_texCollegeWood, { 140, 135, 125, 255 });

    // 12 Student Desks
    Color steelLegCol = { 32, 34, 38, 255 };
    for (int row = 0; row < 4; row++) {
        float rz = 136.0f - (float)row * 2.6f;
        for (int col = 0; col < 3; col++) {
            float rx = 157.0f + (float)col * 5.0f;
            int deskIdx = row * 3 + col;
            if (deskIdx == 7) {
                rlPushMatrix();
                rlTranslatef(rx, 10.25f, rz);
                rlRotatef(78.0f, 0.0f, 0.0f, 1.0f);
                DrawCollegeTexturedBox(Vector3{ 0.0f, 0.0f, 0.0f }, Vector3{ 0.65f, 0.03f, 0.48f }, g_texCollegeWood);
                DrawCylinder(Vector3{ -0.28f, -0.7f, -0.2f }, 0.02f, 0.02f, 0.7f, 6, steelLegCol);
                DrawCylinder(Vector3{ 0.28f, -0.7f, 0.2f }, 0.02f, 0.02f, 0.7f, 6, steelLegCol);
                rlPopMatrix();
                DrawCube(Vector3{ rx + 0.4f, 10.02f, rz + 0.3f }, 0.28f, 0.005f, 0.22f, { 220, 215, 200, 255 });
                DrawCube(Vector3{ rx - 0.5f, 10.02f, rz - 0.2f }, 0.26f, 0.005f, 0.20f, { 205, 195, 175, 255 });
                continue;
            }

            float yawTilt = (deskIdx == 3 || deskIdx == 8) ? 18.0f : ((deskIdx == 5) ? -22.0f : 0.0f);
            rlPushMatrix();
            rlTranslatef(rx, 10.0f, rz);
            if (yawTilt != 0.0f) rlRotatef(yawTilt, 0.0f, 1.0f, 0.0f);
            DrawCylinder(Vector3{ -0.30f, 0.0f, -0.20f }, 0.018f, 0.018f, 0.72f, 6, steelLegCol);
            DrawCylinder(Vector3{  0.30f, 0.0f, -0.20f }, 0.018f, 0.018f, 0.72f, 6, steelLegCol);
            DrawCylinder(Vector3{ -0.30f, 0.0f,  0.20f }, 0.018f, 0.018f, 0.72f, 6, steelLegCol);
            DrawCylinder(Vector3{  0.30f, 0.0f,  0.20f }, 0.018f, 0.018f, 0.72f, 6, steelLegCol);
            DrawCollegeTexturedBox(Vector3{ 0.0f, 0.72f, 0.0f }, Vector3{ 0.72f, 0.035f, 0.50f }, g_texCollegeWood);
            DrawCube(Vector3{ 0.0f, 0.22f, 0.40f }, 0.04f, 0.04f, 0.40f, steelLegCol);
            DrawCollegeTexturedBox(Vector3{ 0.0f, 0.44f, 0.48f }, Vector3{ 0.42f, 0.03f, 0.38f }, g_texCollegeWood, { 115, 85, 55, 255 });
            DrawCollegeTexturedBox(Vector3{ 0.0f, 0.76f, 0.65f }, Vector3{ 0.40f, 0.24f, 0.025f }, g_texCollegeWood, { 115, 85, 55, 255 });
            if (deskIdx == 1 || deskIdx == 6) {
                DrawCube(Vector3{ 0.10f, 0.75f, 0.0f }, 0.24f, 0.02f, 0.18f, { 180, 50, 45, 255 });
            }
            rlPopMatrix();
        }
    }

    // Teacher's Podium & Instructor Desk
    DrawCollegeTexturedBox(Vector3{ 163.0f, 10.45f, 126.8f }, Vector3{ 2.0f, 0.88f, 0.95f }, g_texCollegeWood);
    DrawCube(Vector3{ 163.4f, 11.15f, 126.8f }, 0.65f, 0.50f, 0.50f, { 50, 34, 22, 255 });

    // Psychotic Chalkboard
    DrawCollegeTexturedBox(Vector3{ 163.0f, 12.80f, 125.08f }, Vector3{ 10.2f, 2.85f, 0.06f }, g_texCollegeWood);
    DrawCube(Vector3{ 163.0f, 12.80f, 125.11f }, 9.9f, 2.55f, 0.02f, { 22, 30, 24, 255 });
    DrawCube(Vector3{ 163.0f, 11.50f, 125.16f }, 9.9f, 0.05f, 0.12f, { 45, 32, 20, 255 });
    DrawCube(Vector3{ 160.5f, 11.54f, 125.16f }, 0.16f, 0.03f, 0.06f, { 70, 50, 35, 255 });
    DrawCube(Vector3{ 161.2f, 11.54f, 125.16f }, 0.08f, 0.02f, 0.02f, { 240, 240, 235, 255 });

    Color chalkCol = { 235, 235, 230, 240 };
    DrawCube(Vector3{ 160.0f, 13.55f, 125.13f }, 2.8f, 0.06f, 0.01f, chalkCol);
    DrawCube(Vector3{ 160.0f, 13.25f, 125.13f }, 2.2f, 0.05f, 0.01f, chalkCol);
    DrawCube(Vector3{ 165.5f, 13.60f, 125.13f }, 2.4f, 0.06f, 0.01f, chalkCol);
    DrawCube(Vector3{ 165.5f, 13.30f, 125.13f }, 2.6f, 0.05f, 0.01f, chalkCol);
    for (int t = 0; t < 8; t++) {
        float tx = 158.4f + (float)t * 0.70f;
        DrawCube(Vector3{ tx + 0.00f, 12.5f, 125.13f }, 0.02f, 0.25f, 0.01f, chalkCol);
        DrawCube(Vector3{ tx + 0.08f, 12.5f, 125.13f }, 0.02f, 0.25f, 0.01f, chalkCol);
        DrawCube(Vector3{ tx + 0.16f, 12.5f, 125.13f }, 0.02f, 0.25f, 0.01f, chalkCol);
        DrawCube(Vector3{ tx + 0.24f, 12.5f, 125.13f }, 0.02f, 0.25f, 0.01f, chalkCol);
        DrawCube(Vector3{ tx + 0.12f, 12.5f, 125.13f }, 0.32f, 0.025f, 0.01f, chalkCol);
    }
    DrawSphereWires(Vector3{ 163.0f, 12.5f, 125.13f }, 0.38f, 8, 8, chalkCol);
    DrawSphere(Vector3{ 163.0f, 12.5f, 125.14f }, 0.09f, { 18, 22, 20, 255 });

    Color bloodDeep = { 125, 8, 14, 245 };
    Color bloodDark = { 80, 5, 8, 250 };
    DrawCube(Vector3{ 164.2f, 12.9f, 125.14f }, 1.4f, 0.85f, 0.015f, bloodDeep);
    DrawCube(Vector3{ 163.8f, 13.2f, 125.14f }, 0.8f, 0.60f, 0.015f, bloodDeep);
    DrawCube(Vector3{ 163.8f, 12.2f, 125.14f }, 0.05f, 1.2f, 0.015f, bloodDark);
    DrawCube(Vector3{ 164.3f, 12.0f, 125.14f }, 0.06f, 1.4f, 0.015f, bloodDark);
    DrawCube(Vector3{ 164.7f, 12.3f, 125.14f }, 0.04f, 0.9f, 0.015f, bloodDark);
    DrawCube(Vector3{ 164.3f, 11.51f, 125.18f }, 1.2f, 0.02f, 0.14f, bloodDeep);
    DrawCube(Vector3{ 164.0f, 10.024f, 128.0f }, 3.8f, 0.003f, 2.8f, bloodDark);
    DrawCube(Vector3{ 164.8f, 10.025f, 128.5f }, 2.2f, 0.003f, 2.0f, bloodDeep);

    // 7. CLASSROOM 102: ANATOMY LAB (South Wing, X: 153..173, Z: 141.5..159)
    DrawCollegeTexturedBox(Vector3{ 157.0f, 12.8f, 141.5f }, Vector3{ 8.0f, 5.4f, 0.35f }, g_texCollegeWallInt);
    DrawCollegeTexturedBox(Vector3{ 168.0f, 12.8f, 141.5f }, Vector3{ 10.0f, 5.4f, 0.35f }, g_texCollegeWallInt);
    rlPushMatrix();
    rlTranslatef(161.2f, 10.0f, 141.5f);
    rlRotatef(-42.0f, 0.0f, 1.0f, 0.0f);
    DrawCollegeTexturedBox(Vector3{ 0.45f, 1.4f, 0.0f }, Vector3{ 0.90f, 2.8f, 0.06f }, g_texCollegeWood);
    rlPopMatrix();
    DrawCube(Vector3{ 162.0f, 13.0f, 141.3f }, 1.2f, 0.25f, 0.03f, { 180, 170, 150, 255 });

    // Acid-etched dark soapstone slate floor
    DrawCollegeTexturedBox(Vector3{ 163.0f, 10.03f, 150.5f }, Vector3{ 20.0f, 0.01f, 17.0f }, g_texCollegeTileLab);

    // 3 Heavy Soapstone Workbenches
    Color benchCol = { 24, 26, 28, 255 };
    Color benchTop = { 18, 20, 22, 255 };
    Color sinkCol = { 210, 215, 215, 255 };
    Color brassCol = { 180, 145, 55, 255 };
    float benchZs[3] = { 145.0f, 149.0f, 155.5f };
    for (int b = 0; b < 3; b++) {
        float bz = benchZs[b];
        DrawCube(Vector3{ 162.0f, 10.45f, bz }, 10.0f, 0.90f, 1.10f, benchCol);
        DrawCube(Vector3{ 162.0f, 10.92f, bz }, 10.3f, 0.06f, 1.25f, benchTop);
        DrawCube(Vector3{ 166.2f, 10.85f, bz }, 0.75f, 0.35f, 0.65f, sinkCol);
        DrawCylinder(Vector3{ 166.5f, 10.95f, bz }, 0.015f, 0.015f, 0.35f, 6, brassCol);
        if (b == 0) {
            DrawCube(Vector3{ 160.5f, 10.96f, bz }, 0.18f, 0.02f, 0.15f, { 190, 225, 230, 180 });
        }
    }

    // 8 Glowing Formalin Specimen Jars
    DrawCollegeTexturedBox(Vector3{ 162.0f, 12.5f, 158.8f }, Vector3{ 8.0f, 0.05f, 0.40f }, g_texCollegeWood);
    DrawCollegeTexturedBox(Vector3{ 162.0f, 13.5f, 158.8f }, Vector3{ 8.0f, 0.05f, 0.40f }, g_texCollegeWood);
    for (int j = 0; j < 8; j++) {
        float jx = 158.5f + (float)j * 1.0f;
        DrawCylinder(Vector3{ jx, 12.55f, 158.8f }, 0.085f, 0.085f, 0.28f, 8, { 180, 210, 200, 120 });
        DrawCylinder(Vector3{ jx, 12.56f, 158.8f }, 0.078f, 0.078f, 0.24f, 8, { 55, 140, 50, 190 });
        DrawSphere(Vector3{ jx, 12.68f, 158.8f }, 0.045f, { 115, 55, 50, 230 });
        DrawCylinder(Vector3{ jx, 12.83f, 158.8f }, 0.09f, 0.07f, 0.04f, 8, { 25, 25, 25, 255 });
    }

    // Central Stainless Steel Dissection Table
    Color steelCol = { 140, 145, 150, 255 };
    DrawCylinder(Vector3{ 161.8f, 10.0f, 151.4f }, 0.035f, 0.035f, 0.85f, 6, steelCol);
    DrawCylinder(Vector3{ 164.2f, 10.0f, 151.4f }, 0.035f, 0.035f, 0.85f, 6, steelCol);
    DrawCylinder(Vector3{ 161.8f, 10.0f, 152.6f }, 0.035f, 0.035f, 0.85f, 6, steelCol);
    DrawCylinder(Vector3{ 164.2f, 10.0f, 152.6f }, 0.035f, 0.035f, 0.85f, 6, steelCol);
    DrawCube(Vector3{ 163.0f, 10.85f, 152.0f }, 2.6f, 0.08f, 1.30f, steelCol);
    DrawCubeWires(Vector3{ 163.0f, 10.87f, 152.0f }, 2.62f, 0.10f, 1.32f, { 90, 95, 100, 255 });
    DrawCube(Vector3{ 163.0f, 10.90f, 152.0f }, 2.2f, 0.005f, 0.95f, bloodDeep);
    DrawCube(Vector3{ 163.6f, 10.90f, 152.0f }, 1.2f, 0.005f, 0.80f, bloodDark);
    DrawCylinder(Vector3{ 164.0f, 10.0f, 152.0f }, 0.16f, 0.14f, 0.32f, 8, { 100, 105, 110, 255 });
    DrawCylinder(Vector3{ 164.0f, 10.22f, 152.0f }, 0.15f, 0.15f, 0.08f, 8, bloodDark);
    DrawCube(Vector3{ 161.75f, 10.75f, 151.4f }, 0.03f, 0.22f, 0.05f, { 65, 45, 30, 255 });
    DrawCube(Vector3{ 164.25f, 10.75f, 152.6f }, 0.03f, 0.22f, 0.05f, { 65, 45, 30, 255 });

    // Full Anatomical Skeleton staring at the doorway
    Color boneCol = { 215, 210, 192, 255 };
    Color boneDark = { 170, 165, 148, 255 };
    DrawSphere(Vector3{ 168.3f, 10.03f, 146.8f }, 0.035f, steelCol);
    DrawSphere(Vector3{ 168.7f, 10.03f, 146.8f }, 0.035f, steelCol);
    DrawSphere(Vector3{ 168.3f, 10.03f, 147.2f }, 0.035f, steelCol);
    DrawSphere(Vector3{ 168.7f, 10.03f, 147.2f }, 0.035f, steelCol);
    DrawCube(Vector3{ 168.5f, 10.06f, 147.0f }, 0.55f, 0.03f, 0.55f, steelCol);
    DrawCylinder(Vector3{ 168.5f, 10.0f, 147.0f }, 0.015f, 0.015f, 3.4f, 6, steelCol);

    DrawCube(Vector3{ 168.5f, 11.95f, 147.0f }, 0.28f, 0.16f, 0.18f, boneCol);
    DrawCubeWires(Vector3{ 168.5f, 11.95f, 147.0f }, 0.29f, 0.17f, 0.19f, boneDark);
    DrawCylinder(Vector3{ 168.4f, 11.35f, 147.0f }, 0.03f, 0.025f, 0.55f, 6, boneCol);
    DrawCylinder(Vector3{ 168.6f, 11.35f, 147.0f }, 0.03f, 0.025f, 0.55f, 6, boneCol);
    DrawSphere(Vector3{ 168.4f, 11.33f, 147.0f }, 0.035f, boneCol);
    DrawSphere(Vector3{ 168.6f, 11.33f, 147.0f }, 0.035f, boneCol);
    DrawCylinder(Vector3{ 168.4f, 10.65f, 147.0f }, 0.025f, 0.022f, 0.65f, 6, boneCol);
    DrawCylinder(Vector3{ 168.6f, 10.65f, 147.0f }, 0.025f, 0.022f, 0.65f, 6, boneCol);
    DrawCube(Vector3{ 168.4f, 10.08f, 146.92f }, 0.07f, 0.04f, 0.16f, boneCol);
    DrawCube(Vector3{ 168.6f, 10.08f, 146.92f }, 0.07f, 0.04f, 0.16f, boneCol);

    for (float sy = 12.05f; sy <= 12.75f; sy += 0.08f) {
        DrawSphere(Vector3{ 168.5f, sy, 147.0f }, 0.035f, boneCol);
    }
    DrawCube(Vector3{ 168.5f, 12.45f, 147.0f }, 0.32f, 0.42f, 0.22f, boneCol);
    DrawCubeWires(Vector3{ 168.5f, 12.45f, 147.0f }, 0.34f, 0.44f, 0.24f, boneDark);
    DrawCylinder(Vector3{ 168.32f, 12.10f, 147.0f }, 0.024f, 0.020f, 0.55f, 6, boneCol);
    DrawCylinder(Vector3{ 168.68f, 12.10f, 147.0f }, 0.024f, 0.020f, 0.55f, 6, boneCol);
    DrawCylinder(Vector3{ 168.32f, 11.55f, 147.0f }, 0.020f, 0.016f, 0.52f, 6, boneCol);
    DrawCylinder(Vector3{ 168.68f, 11.55f, 147.0f }, 0.020f, 0.016f, 0.52f, 6, boneCol);

    rlPushMatrix();
    rlTranslatef(168.5f, 12.95f, 147.0f);
    rlRotatef(-48.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(14.0f, 1.0f, 0.0f, 0.0f);
    DrawSphere(Vector3{ 0.0f, 0.12f, 0.0f }, 0.11f, boneCol);
    DrawSphereWires(Vector3{ 0.0f, 0.12f, 0.0f }, 0.115f, 8, 8, boneDark);
    DrawSphere(Vector3{ -0.042f, 0.11f, -0.095f }, 0.022f, { 14, 10, 8, 255 });
    DrawSphere(Vector3{  0.042f, 0.11f, -0.095f }, 0.022f, { 14, 10, 8, 255 });
    DrawCube(Vector3{ 0.0f, 0.07f, -0.098f }, 0.020f, 0.035f, 0.015f, { 14, 10, 8, 255 });
    DrawCube(Vector3{ 0.0f, 0.02f, -0.065f }, 0.085f, 0.05f, 0.09f, boneCol);
    DrawCube(Vector3{ 0.0f, 0.025f, -0.095f }, 0.065f, 0.015f, 0.01f, { 235, 235, 230, 255 });
    rlPopMatrix();

    // 8. ROOM 103: ARCHIVE & OFFICE (East Wing, X: 174..187)
    DrawCollegeTexturedBox(Vector3{ 174.0f, 12.8f, 131.5f }, Vector3{ 0.40f, 5.4f, 15.0f }, g_texCollegeWallInt);
    DrawCollegeTexturedBox(Vector3{ 174.0f, 12.8f, 150.5f }, Vector3{ 0.40f, 5.4f, 19.0f }, g_texCollegeWallInt);
    rlPushMatrix();
    rlTranslatef(174.0f, 10.0f, 139.2f);
    rlRotatef(62.0f, 0.0f, 1.0f, 0.0f);
    DrawCollegeTexturedBox(Vector3{ 0.0f, 1.4f, 0.45f }, Vector3{ 0.06f, 2.8f, 0.90f }, g_texCollegeWood);
    rlPopMatrix();

    // Parquet Floor
    DrawCollegeTexturedBox(Vector3{ 180.5f, 10.03f, 142.0f }, Vector3{ 14.0f, 0.01f, 34.0f }, g_texCollegeWood, { 120, 100, 80, 255 });

    // Faculty Desk with Banker's Lamp & Off-hook Rotary Phone
    DrawCollegeTexturedBox(Vector3{ 181.0f, 10.45f, 135.0f }, Vector3{ 2.4f, 0.90f, 1.20f }, g_texCollegeWood);
    DrawCube(Vector3{ 181.0f, 10.92f, 135.0f }, 1.2f, 0.01f, 0.70f, { 35, 55, 40, 255 });
    DrawCube(Vector3{ 181.6f, 10.96f, 134.8f }, 0.22f, 0.08f, 0.22f, { 18, 18, 20, 255 });
    DrawCylinder(Vector3{ 181.6f, 11.02f, 134.8f }, 0.045f, 0.045f, 0.02f, 8, { 220, 220, 220, 255 });
    DrawCube(Vector3{ 181.3f, 10.94f, 134.9f }, 0.24f, 0.04f, 0.07f, { 18, 18, 20, 255 });
    DrawCylinder(Vector3{ 180.4f, 10.92f, 135.2f }, 0.06f, 0.06f, 0.02f, 8, brassCol);
    DrawCylinder(Vector3{ 180.4f, 10.94f, 135.2f }, 0.012f, 0.012f, 0.35f, 6, brassCol);
    DrawCube(Vector3{ 180.4f, 11.30f, 135.2f }, 0.16f, 0.08f, 0.28f, { 25, 85, 45, 230 });

    // 4 Metal Filing Cabinets
    Color fileCabCol = { 38, 44, 40, 255 };
    DrawCube(Vector3{ 186.0f, 11.15f, 142.0f }, 0.65f, 2.3f, 0.90f, fileCabCol);
    DrawCubeWires(Vector3{ 186.0f, 11.15f, 142.0f }, 0.66f, 2.32f, 0.92f, { 22, 28, 24, 255 });
    DrawCube(Vector3{ 186.0f, 11.15f, 143.2f }, 0.65f, 2.3f, 0.90f, fileCabCol);
    rlPushMatrix();
    rlTranslatef(184.5f, 10.35f, 145.5f);
    rlRotatef(82.0f, 1.0f, 0.0f, 0.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 0.65f, 2.3f, 0.90f, fileCabCol);
    rlPopMatrix();
    DrawCube(Vector3{ 184.2f, 10.02f, 147.2f }, 0.35f, 0.01f, 0.28f, { 205, 185, 140, 255 });
    DrawCube(Vector3{ 184.8f, 10.02f, 146.8f }, 0.32f, 0.01f, 0.26f, { 190, 170, 130, 255 });

    // Heavy Cast-Iron Safe with Claw Gouges
    DrawCube(Vector3{ 186.0f, 10.80f, 127.0f }, 1.3f, 1.6f, 1.3f, { 28, 30, 32, 255 });
    DrawCubeWires(Vector3{ 186.0f, 10.80f, 127.0f }, 1.32f, 1.62f, 1.32f, { 16, 18, 20, 255 });
    DrawCube(Vector3{ 185.35f, 10.80f, 127.0f }, 0.10f, 1.4f, 1.1f, { 35, 38, 40, 255 });
    DrawCylinder(Vector3{ 185.28f, 10.80f, 127.0f }, 0.08f, 0.08f, 0.04f, 12, brassCol);
    DrawCube(Vector3{ 185.28f, 11.0f, 126.9f }, 0.02f, 0.30f, 0.015f, { 80, 12, 16, 240 });
    DrawCube(Vector3{ 185.28f, 11.0f, 127.0f }, 0.02f, 0.30f, 0.015f, { 80, 12, 16, 240 });
    DrawCube(Vector3{ 185.28f, 11.0f, 127.1f }, 0.02f, 0.30f, 0.015f, { 80, 12, 16, 240 });

    EndShaderMode();
    // ---------------------------------------------------------------------
    // END SHADER MODE: Render alpha overlays, light pools & dust motes
    // ---------------------------------------------------------------------

    // Volumetric Halos & Light Pools
    DrawSphere(l0Pos, 0.45f, { 255, 170, 50, 25 }); // Portico lantern warm halo

    if (g_collegeLightOn) {
        DrawSphere(l1Pos, 0.55f, { 220, 245, 200, 55 }); // Fluorescent tube green-white halo
        DrawCircle3D(Vector3{ 163.0f, 10.02f, 140.0f }, 2.5f, Vector3{ 1, 0, 0 }, 90.0f, { 75, 88, 65, 45 });
        DrawCircle3D(Vector3{ 163.0f, 10.02f, 140.0f }, 4.2f, Vector3{ 1, 0, 0 }, 90.0f, { 45, 55, 38, 25 });
    }

    DrawSphere(l2Pos, 0.40f, { 220, 15, 20, 60 }); // Red exit sign halo
    DrawCircle3D(Vector3{ 173.8f, 10.02f, 140.0f }, 3.2f, Vector3{ 1, 0, 0 }, 90.0f, { 85, 8, 12, 50 });

    // Formalin Jars Green Glow
    for (int j = 0; j < 8; j++) {
        float jx = 158.5f + (float)j * 1.0f;
        DrawSphere(Vector3{ jx, 12.68f, 158.8f }, 0.28f, { 60, 230, 80, 30 });
    }

    // Classroom 101 Volumetric Light Shaft through broken boards
    Color lightShaftCol = { 235, 225, 180, (unsigned char)(20 + (int)(sinf(timeVal * 1.5f) * 6)) };
    rlPushMatrix();
    rlTranslatef(163.0f, 12.0f, 128.5f);
    rlRotatef(-32.0f, 1.0f, 0.0f, 0.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 3.5f, 0.6f, 6.0f, lightShaftCol);
    rlPopMatrix();

    // Ceiling Leak Expanding Ripple on Puddle
    float rippleRadius = 0.4f + fmodf(timeVal * 0.8f, 1.0f) * 0.65f;
    unsigned char rippleAlpha = (unsigned char)(Clamp(1.0f - (rippleRadius - 0.4f) / 0.65f, 0.0f, 1.0f) * 110.0f);
    DrawCircle3D(Vector3{ bucketPos.x, 10.022f, bucketPos.z }, rippleRadius, Vector3{ 1, 0, 0 }, 90.0f, { 60, 80, 95, rippleAlpha });
    DrawCircle3D(Vector3{ bucketPos.x, 10.020f, bucketPos.z }, 0.95f, Vector3{ 1, 0, 0 }, 90.0f, { 30, 42, 50, 90 });

    // 24 Floating Atmospheric Dust Motes
    for (int d = 0; d < 24; d++) {
        float seed = (float)d * 3.71f;
        float mx = 155.0f + fmodf(seed * 19.3f, 26.0f);
        float my = 10.4f  + fmodf(seed * 11.7f + timeVal * 0.15f, 3.8f);
        float mz = 127.0f + fmodf(seed * 23.1f, 28.0f);
        mx += sinf(timeVal * 0.8f + seed) * 0.18f;
        mz += cosf(timeVal * 0.6f + seed) * 0.18f;
        float moteAlpha = 65.0f + sinf(timeVal * 2.0f + seed) * 35.0f;
        Color moteCol = { 225, 220, 205, (unsigned char)Clamp(moteAlpha, 20.0f, 110.0f) };
        DrawCube(Vector3{ mx, my, mz }, 0.028f, 0.028f, 0.028f, moteCol);
    }
}
