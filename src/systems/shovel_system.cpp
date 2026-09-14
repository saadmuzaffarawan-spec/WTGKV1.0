#include "shovel_system.h"
#include <cmath>
#include <cstring>
#include <cstdlib>

// ==================================================================

// HYPER-REALISTIC SHOVEL SYSTEM (Custom Curved Spade Mesh, Lit Shader, Rig & Physics)

// ==================================================================

static const char *VS_SHOVEL = R"(

#version 330

in vec3 vertexPosition;

in vec3 vertexNormal;

in vec2 vertexTexCoord;

uniform mat4 mvp;

uniform mat4 matModel;

out vec3 fragNormal;

out vec3 fragPos;

out vec2 fragTexCoord;

void main() {

    fragPos = vec3(matModel * vec4(vertexPosition, 1.0));

    fragNormal = normalize(mat3(matModel) * vertexNormal);

    fragTexCoord = vertexTexCoord;

    gl_Position = mvp * vec4(vertexPosition, 1.0);

}

)";



static const char *FS_SHOVEL = R"(

#version 330

in vec3 fragNormal;

in vec3 fragPos;

in vec2 fragTexCoord;

out vec4 finalColor;



uniform vec3 lightDir;

uniform vec3 viewPos;

uniform vec4 baseColor;

uniform float roughness;

uniform float grainAmount;

uniform float metallic;



float hash(vec2 p) { return fract(sin(dot(p, vec2(41.3, 289.1))) * 43758.5453); }



void main() {

    vec3 N = normalize(fragNormal);

    vec3 L = normalize(-lightDir);

    vec3 V = normalize(viewPos - fragPos);

    vec3 H = normalize(L + V);



    float ambient = 0.32;

    float diff = max(dot(N, L), 0.0);

    float shininess = mix(8.0, 128.0, 1.0 - roughness);

    float spec = pow(max(dot(N, H), 0.0), shininess) * mix(0.15, 1.0, metallic);



    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.5) * 0.35;



    vec3 color = baseColor.rgb;



    if (grainAmount > 0.0) {

        float grain = hash(vec2(fragTexCoord.y * 60.0, floor(fragTexCoord.x * 8.0)));

        float stripes = sin(fragTexCoord.y * 90.0 + grain * 6.0) * 0.5 + 0.5;

        color = mix(color, color * 0.75, stripes * grainAmount);

    }



    vec3 lit = color * (ambient + diff * 0.85) + vec3(spec) + vec3(rim) * 0.6;

    finalColor = vec4(lit, baseColor.a);

}

)";



static Mesh GenMeshShovelBlade(int segments, float zStart, float length, float maxWidth, float curveDepth)

{

    int cols = 6;

    int rows = segments + 1;

    int vertCount = rows * cols;

    int triCount = segments * (cols - 1) * 4;



    Mesh mesh = { 0 };

    mesh.vertexCount = vertCount;

    mesh.triangleCount = triCount;

    mesh.vertices  = (float *)MemAlloc(vertCount * 3 * sizeof(float));

    mesh.normals   = (float *)MemAlloc(vertCount * 3 * sizeof(float));

    mesh.texcoords = (float *)MemAlloc(vertCount * 2 * sizeof(float));

    mesh.indices   = (unsigned short *)MemAlloc(triCount * 3 * sizeof(unsigned short));



    for (int r = 0; r < rows; r++) {

        float t = (float)r / (float)segments;

        // Blade tapers cleanly towards the pointed spade tip

        float width = maxWidth * powf(1.0f - t, 0.60f);

        float z = zStart + t * length;

        // Scoop dish curve along the spine

        float scoopLong = curveDepth * sinf(PI * t);



        for (int c = 0; c < cols; c++) {

            float s = (float)c / (float)(cols - 1);

            float x = (s - 0.5f) * 2.0f * width;

            float crossCurve = curveDepth * 0.45f * (1.0f - powf(2.0f * s - 1.0f, 2.0f));

            float y = scoopLong - crossCurve;



            int idx = r * cols + c;

            mesh.vertices[idx*3+0] = x;

            mesh.vertices[idx*3+1] = y;

            mesh.vertices[idx*3+2] = z;

            mesh.texcoords[idx*2+0] = s;

            mesh.texcoords[idx*2+1] = t;

        }

    }



    int ii = 0;

    for (int r = 0; r < segments; r++) {

        for (int c = 0; c < cols - 1; c++) {

            int a = r * cols + c;

            int b = a + 1;

            int cN = (r + 1) * cols + c;

            int d = cN + 1;

            // Front face

            mesh.indices[ii++] = (unsigned short)a; mesh.indices[ii++] = (unsigned short)cN; mesh.indices[ii++] = (unsigned short)b;

            mesh.indices[ii++] = (unsigned short)b; mesh.indices[ii++] = (unsigned short)cN; mesh.indices[ii++] = (unsigned short)d;

            // Back face

            mesh.indices[ii++] = (unsigned short)a; mesh.indices[ii++] = (unsigned short)b; mesh.indices[ii++] = (unsigned short)cN;

            mesh.indices[ii++] = (unsigned short)b; mesh.indices[ii++] = (unsigned short)d; mesh.indices[ii++] = (unsigned short)cN;

        }

    }



    for (int i = 0; i < vertCount * 3; i++) mesh.normals[i] = 0.0f;

    for (int i = 0; i < triCount; i++) {

        int i0 = mesh.indices[i*3+0], i1 = mesh.indices[i*3+1], i2 = mesh.indices[i*3+2];

        Vector3 v0 = { mesh.vertices[i0*3+0], mesh.vertices[i0*3+1], mesh.vertices[i0*3+2] };

        Vector3 v1 = { mesh.vertices[i1*3+0], mesh.vertices[i1*3+1], mesh.vertices[i1*3+2] };

        Vector3 v2 = { mesh.vertices[i2*3+0], mesh.vertices[i2*3+1], mesh.vertices[i2*3+2] };

        Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(v1, v0), Vector3Subtract(v2, v0)));

        int idxs[3] = { i0, i1, i2 };

        for (int k = 0; k < 3; k++) {

            mesh.normals[idxs[k]*3+0] += n.x;

            mesh.normals[idxs[k]*3+1] += n.y;

            mesh.normals[idxs[k]*3+2] += n.z;

        }

    }

    for (int v = 0; v < vertCount; v++) {

        Vector3 n = { mesh.normals[v*3+0], mesh.normals[v*3+1], mesh.normals[v*3+2] };

        float len = Vector3Length(n);

        if (len > 0.0001f) n = Vector3Scale(n, 1.0f / len);

        else n = (Vector3){ 0.0f, 1.0f, 0.0f };

        mesh.normals[v*3+0] = n.x; mesh.normals[v*3+1] = n.y; mesh.normals[v*3+2] = n.z;

    }



    UploadMesh(&mesh, false);

    return mesh;

}









float g_shovelTipLocalZ = 0.98f;



static void SetPartUniforms(Shovel &sv, Color baseColor, float roughness, float grain, float metallic)

{

    Vector4 c = ColorNormalize(baseColor);

    SetShaderValue(sv.shader, sv.locBaseColor, &c, SHADER_UNIFORM_VEC4);

    SetShaderValue(sv.shader, sv.locRough, &roughness, SHADER_UNIFORM_FLOAT);

    SetShaderValue(sv.shader, sv.locGrain, &grain, SHADER_UNIFORM_FLOAT);

    SetShaderValue(sv.shader, sv.locMetal, &metallic, SHADER_UNIFORM_FLOAT);

}



Shovel BuildShovel()

{

    Shovel sv = { 0 };

    sv.shader = LoadShaderFromMemory(VS_SHOVEL, FS_SHOVEL);

    sv.shader.locs[SHADER_LOC_MATRIX_MVP]   = GetShaderLocation(sv.shader, "mvp");

    sv.shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(sv.shader, "matModel");

    sv.locLightDir  = GetShaderLocation(sv.shader, "lightDir");

    sv.locViewPos   = GetShaderLocation(sv.shader, "viewPos");

    sv.locBaseColor = GetShaderLocation(sv.shader, "baseColor");

    sv.locRough     = GetShaderLocation(sv.shader, "roughness");

    sv.locGrain     = GetShaderLocation(sv.shader, "grainAmount");

    sv.locMetal     = GetShaderLocation(sv.shader, "metallic");



    // UNIFIED FORWARD-ORIENTED SHOVEL ASSEMBLY (Origin Z=0.0 at Hand Grip, ZERO GAPS):

    // 1. Pommel End Cap: Sphere at Z = -0.04m (radius 0.026m)

    // 2. Grip Sleeve:    Z = -0.04m to Z = +0.18m (radius 0.024m)

    // 3. Ash Wood Shaft: Z =  0.00m to Z = +0.68m (radius 0.018m, 18cm overlap with grip)

    // 4. Steel Collar:   Z = +0.62m to Z = +0.74m (radius 0.025m, 6cm overlap with shaft)

    // 5. Foot Tabs:      Z = +0.73m at blade shoulders

    // 6. Curved Blade:   Z = +0.70m to Z = +0.98m (tip at 0.98m, 4cm overlap with collar)



    float gripLen   = 0.22f;

    float shaftLen  = 0.68f;

    float collarLen = 0.12f;

    float bladeLen  = 0.28f;

    float bladeWidth= 0.20f;

    float bladeCurve= 0.038f;



    // 1. Pommel: Steel spherical end cap at handle butt

    Mesh pommelMesh = GenMeshSphere(0.026f, 12, 12);

    sv.pommel.model = LoadModelFromMesh(pommelMesh);

    sv.pommel.model.materials[0].shader = sv.shader;

    sv.pommel.localOffset = MatrixTranslate(0, 0, -0.04f);



    // 2. Grip Sleeve: Heavy textured rubber grip sleeve

    Mesh gripMesh = GenMeshCylinder(0.024f, gripLen, 14);

    sv.grip.model = LoadModelFromMesh(gripMesh);

    sv.grip.model.materials[0].shader = sv.shader;

    sv.grip.localOffset = MatrixMultiply(MatrixRotateX(PI/2.0f), MatrixTranslate(0, 0, -0.04f));



    // 3. Handle Shaft: Weathered ash wood shaft

    Mesh shaftMesh = GenMeshCylinder(0.018f, shaftLen, 12);

    sv.shaft.model = LoadModelFromMesh(shaftMesh);

    sv.shaft.model.materials[0].shader = sv.shader;

    sv.shaft.localOffset = MatrixMultiply(MatrixRotateX(PI/2.0f), MatrixTranslate(0, 0, 0.00f));



    // 4. Socket Collar: Gunmetal steel socket clamping blade to shaft

    Mesh collarMesh = GenMeshCylinder(0.025f, collarLen, 12);

    sv.collar.model = LoadModelFromMesh(collarMesh);

    sv.collar.model.materials[0].shader = sv.shader;

    sv.collar.localOffset = MatrixMultiply(MatrixRotateX(PI/2.0f), MatrixTranslate(0, 0, 0.62f));



    // 5. Left & Right Foot Tabs: Welded steel steps at collar shoulders

    Mesh tabMeshL = GenMeshCube(0.075f, 0.012f, 0.038f);

    sv.tabL.model = LoadModelFromMesh(tabMeshL);

    sv.tabL.model.materials[0].shader = sv.shader;

    sv.tabL.localOffset = MatrixTranslate(-0.088f, 0.008f, 0.728f);



    Mesh tabMeshR = GenMeshCube(0.075f, 0.012f, 0.038f);

    sv.tabR.model = LoadModelFromMesh(tabMeshR);

    sv.tabR.model.materials[0].shader = sv.shader;

    sv.tabR.localOffset = MatrixTranslate( 0.088f, 0.008f, 0.728f);



    // 6. Blade: Curved tool steel spade starting at Z=0.70m, tip at Z=0.98m

    Mesh bladeMesh = GenMeshShovelBlade(10, 0.70f, bladeLen, bladeWidth, bladeCurve);

    sv.blade.model = LoadModelFromMesh(bladeMesh);

    sv.blade.model.materials[0].shader = sv.shader;

    sv.blade.localOffset = MatrixIdentity();

    g_shovelTipLocalZ = 0.70f + bladeLen;



    return sv;

}



void UnloadShovel(Shovel &sv)

{

    UnloadModel(sv.pommel.model);

    UnloadModel(sv.grip.model);

    UnloadModel(sv.shaft.model);

    UnloadModel(sv.collar.model);

    UnloadModel(sv.tabL.model);

    UnloadModel(sv.tabR.model);

    UnloadModel(sv.blade.model);

    UnloadShader(sv.shader);

}



void DrawShovel(Shovel &sv, Matrix rootTransform, Vector3 viewPos, Vector3 lightDir)

{

    SetShaderValue(sv.shader, sv.locViewPos, &viewPos, SHADER_UNIFORM_VEC3);

    SetShaderValue(sv.shader, sv.locLightDir, &lightDir, SHADER_UNIFORM_VEC3);



    ShovelPart *parts[7] = { &sv.pommel, &sv.grip, &sv.shaft, &sv.collar, &sv.tabL, &sv.tabR, &sv.blade };

    Color colors[7]   = {

        (Color){ 88, 90, 96, 255 },     // pommel: steel

        (Color){ 34, 34, 36, 255 },     // grip: dark rubber

        (Color){ 145, 102, 60, 255 },   // shaft: warm ash wood

        (Color){ 160, 162, 168, 255 },  // collar: stamped steel

        (Color){ 145, 148, 154, 255 },  // tabL: steel step

        (Color){ 145, 148, 154, 255 },  // tabR: steel step

        (Color){ 195, 198, 204, 255 }   // blade: tool steel

    };

    float roughness[7]= { 0.45f, 0.90f, 0.72f, 0.30f, 0.35f, 0.35f, 0.22f };

    float grain[7]    = { 0.0f,  0.0f,  0.70f, 0.0f,  0.0f,  0.0f,  0.0f  };

    float metallic[7] = { 0.80f, 0.0f,  0.0f,  0.85f, 0.85f, 0.85f, 0.92f };



    for (int i = 0; i < 7; i++) {

        SetPartUniforms(sv, colors[i], roughness[i], grain[i], metallic[i]);

        Matrix world = MatrixMultiply(parts[i]->localOffset, rootTransform);

        parts[i]->model.transform = world;

        DrawModel(parts[i]->model, (Vector3){0,0,0}, 1.0f, WHITE);

    }

}



// Keyframe animation system


ShovelPose LerpShovelPose(ShovelPose a, ShovelPose b, float f)

{

    float sf = f*f*(3.0f - 2.0f*f);

    ShovelPose r;

    r.pos = Vector3Lerp(a.pos, b.pos, sf);

    r.rot = Vector3Lerp(a.rot, b.rot, sf);

    return r;

}



ShovelPose SampleShovelKeyframes(const ShovelKeyframe *kf, int count, float t)

{

    if (t <= kf[0].t) return kf[0].pose;

    if (t >= kf[count-1].t) return kf[count-1].pose;

    for (int i = 0; i < count - 1; i++) {

        if (t >= kf[i].t && t <= kf[i+1].t) {

            float span = kf[i+1].t - kf[i].t;

            float f = (span > 0.0001f) ? (t - kf[i].t) / span : 0.0f;

            return LerpShovelPose(kf[i].pose, kf[i+1].pose, f);

        }

    }

    return kf[count-1].pose;

}




// Shovel Ready / Idle Pose: Tucked low-right, pointing naturally forward-down toward the earth

// Pitch=22 deg, Yaw=-12 deg, Roll=8 deg: blade rests visible in lower right, crosshair unblocked

const ShovelPose SHOVEL_HOLD_POSE = { {0.0f, 0.0f, 0.0f}, {-8.0f, -14.0f, 10.0f} };

// Authentic "Digging From Bottom" Motion:
// 1. (0.00-0.12s) Raise & brace back for downward plunge
// 2. (0.12-0.24s) Forceful downward plunge driving blade into the soil (Impact + camera kick)
// 3. (0.24-0.46s) Hands pull BACK and DOWN on handle to LEVER / PRY the dirt clump loose from underneath
// 4. (0.46-0.70s) Upward scoop and FLING dirt forward in an arc (dirt clod shower)
// 5. (0.70-1.00s) Settle back into upward-tilted ready stance
static const ShovelKeyframe SHOVEL_DIG_KF[] = {
    { 0.00f, { { 0.00f,  0.00f,  0.00f}, { -8.0f, -14.0f,  10.0f} } },
    { 0.12f, { { 0.02f,  0.06f, -0.06f}, {-14.0f,  -8.0f,  14.0f} } }, // Raise & brace
    { 0.24f, { {-0.04f, -0.26f,  0.20f}, { 46.0f,  -6.0f,   4.0f} } }, // Plunge into earth (IMPACT!)
    { 0.46f, { {-0.02f, -0.16f,  0.04f}, {  6.0f, -14.0f,  16.0f} } }, // Levering / prying soil from bottom
    { 0.70f, { {-0.06f,  0.12f,  0.10f}, {-18.0f,  -2.0f, -12.0f} } }, // Scooping & flinging dirt forward!
    { 0.86f, { { 0.01f, -0.04f,  0.02f}, { -2.0f, -12.0f,   8.0f} } }, // Settling
    { 1.00f, { { 0.00f,  0.00f,  0.00f}, { -8.0f, -14.0f,  10.0f} } }, // Return to upward-tilted stance
};

// Attack Swing: Fast horizontal clearing sweep across ground / lower screen
static const ShovelKeyframe SHOVEL_ATTACK_KF[] = {
    { 0.00f, { { 0.00f,  0.00f,  0.00f}, { -8.0f, -14.0f,  10.0f} } },
    { 0.14f, { { 0.10f,  0.05f, -0.10f}, {  6.0f, -36.0f,  24.0f} } }, // Windup back right
    { 0.28f, { {-0.14f, -0.08f,  0.16f}, { 24.0f,  38.0f, -28.0f} } }, // Swift slash through center
    { 0.40f, { {-0.06f, -0.04f,  0.06f}, {  2.0f,  12.0f,  -6.0f} } }, // Follow-through
    { 0.50f, { { 0.00f,  0.00f,  0.00f}, { -8.0f, -14.0f,  10.0f} } }, // Return to upward-tilted stance
};

const float SHOVEL_DIG_DURATION = 0.95f;

const float SHOVEL_ATTACK_DURATION = 0.50f;

const float SHOVEL_DIG_IMPACT_T = 0.24f;

const float SHOVEL_DIG_THROW_T  = 0.70f;

const float SHOVEL_ATTACK_IMPACT_T = 0.28f;



ShovelPose GetAnimatedShovelPose(ShovelAnimState state, float stateTime, float idleClock)

{

    if (state == SHOVEL_ANIM_DIG) {

        float t = Clamp(stateTime / SHOVEL_DIG_DURATION, 0.0f, 1.0f);

        return SampleShovelKeyframes(SHOVEL_DIG_KF, sizeof(SHOVEL_DIG_KF)/sizeof(ShovelKeyframe), t);

    }

    if (state == SHOVEL_ANIM_ATTACK) {

        float t = Clamp(stateTime / SHOVEL_ATTACK_DURATION, 0.0f, 1.0f);

        return SampleShovelKeyframes(SHOVEL_ATTACK_KF, sizeof(SHOVEL_ATTACK_KF)/sizeof(ShovelKeyframe), t);

    }

    ShovelPose p = SHOVEL_HOLD_POSE;

    p.pos.y += sinf(idleClock * 1.4f) * 0.006f;

    p.rot.z += sinf(idleClock * 1.0f) * 1.2f;

    p.rot.x += sinf(idleClock * 0.7f) * 0.6f;

    return p;

}



Matrix ShovelPoseToWorldMatrix(ShovelPose pose, Vector3 basePos, Vector3 right, Vector3 up, Vector3 fwd)

{

    Matrix rot = MatrixRotateXYZ((Vector3){ DEG2RAD*pose.rot.x, DEG2RAD*pose.rot.y, DEG2RAD*pose.rot.z });

    Vector3 offset = Vector3Add(Vector3Add(Vector3Scale(right, pose.pos.x), Vector3Scale(up, pose.pos.y)),

                                 Vector3Scale(fwd, pose.pos.z));

    Vector3 finalPos = Vector3Add(basePos, offset);



    Matrix basis = { right.x, up.x, fwd.x, 0,

                      right.y, up.y, fwd.y, 0,

                      right.z, up.z, fwd.z, 0,

                      0,0,0,1 };

    Matrix world = MatrixMultiply(rot, basis);

    world.m12 = finalPos.x; world.m13 = finalPos.y; world.m14 = finalPos.z;

    return world;

}



// Physically simulated flying dirt particles


static const int MAX_DIRT_CLODS = 60;

static DirtClod g_dirtClods[MAX_DIRT_CLODS];



void SpawnDirtClod(Vector3 origin, Vector3 dir)

{

    for (int i = 0; i < MAX_DIRT_CLODS; i++) {

        if (!g_dirtClods[i].active) {

            g_dirtClods[i].active = true;

            g_dirtClods[i].pos = origin;

            float spread = 1.2f;

            g_dirtClods[i].vel = Vector3Add(Vector3Scale(dir, 2.2f + (float)GetRandomValue(0, 150)/100.0f),

                (Vector3){ (float)GetRandomValue(-100, 100)/100.0f * spread, 1.6f + (float)GetRandomValue(0, 150)/100.0f, (float)GetRandomValue(-100, 100)/100.0f * spread });

            g_dirtClods[i].life = 1.4f;

            return;

        }

    }

}



float GetTerrainGroundHeight(float x, float z) {

    if (x <= 84.0f && x >= 62.0f && z >= 136.0f && z <= 144.0f) {

        return 1.2f + ((x - 62.0f) / 22.0f) * 8.8f;

    }

    if (x < 62.0f && x >= 32.0f && z >= 136.0f && z <= 144.0f) {

        return -16.0f + ((x - 32.0f) / 30.0f) * 17.2f;

    }

    if (x < 32.0f && z >= 105.0f && z <= 175.0f) {

        return -16.0f;

    }

    return 10.0f;

}



void UpdateDirtClods(float dt)

{

    for (int i = 0; i < MAX_DIRT_CLODS; i++) {

        if (!g_dirtClods[i].active) continue;

        g_dirtClods[i].vel.y += -9.8f * dt;

        g_dirtClods[i].pos = Vector3Add(g_dirtClods[i].pos, Vector3Scale(g_dirtClods[i].vel, dt));



        float gY = GetTerrainGroundHeight(g_dirtClods[i].pos.x, g_dirtClods[i].pos.z);

        if (g_dirtClods[i].pos.y <= gY + 0.03f) {

            g_dirtClods[i].pos.y = gY + 0.03f;

            g_dirtClods[i].vel.y *= -0.35f;

            g_dirtClods[i].vel.x *= 0.6f;

            g_dirtClods[i].vel.z *= 0.6f;

            if (fabsf(g_dirtClods[i].vel.y) < 0.4f) g_dirtClods[i].vel.y = 0;

        }



        g_dirtClods[i].life -= dt;

        if (g_dirtClods[i].life <= 0.0f) g_dirtClods[i].active = false;

    }

}



void DrawDirtClods()

{

    for (int i = 0; i < MAX_DIRT_CLODS; i++) {

        if (!g_dirtClods[i].active) continue;

        float a = Clamp(g_dirtClods[i].life / 1.4f, 0.0f, 1.0f);

        Color c = (Color){ 68, 48, 28, (unsigned char)(255 * a) };

        DrawCube(g_dirtClods[i].pos, 0.06f, 0.06f, 0.06f, c);

    }

}
