#include "hound_npc.h"
#include <raymath.h>
#include <rlgl.h>
#include <math.h>
#include <stdlib.h>

static inline float Frand(float minVal, float maxVal) {
    return minVal + ((float)GetRandomValue(0, 10000) / 10000.0f) * (maxVal - minVal);
}

// ==================================================================================================

// HOUND -- Procedural Horror Dog NPC System

// Features full anatomical kinematics, autonomous companion AI, and domain-warped FBM fur shader

// ==================================================================================================



static const char *VS_HOUND_SOURCE =

"#version 330\n"

"in vec3 vertexPosition;\n"

"in vec2 vertexTexCoord;\n"

"in vec3 vertexNormal;\n"

"in vec4 vertexColor;\n"

"uniform mat4 mvp;\n"

"uniform mat4 matModel;\n"

"uniform mat4 matNormal;\n"

"out vec3 fragPosition;\n"

"out vec3 fragLocalPos;\n"

"out vec2 fragTexCoord;\n"

"out vec4 fragColor;\n"

"out vec3 fragNormal;\n"

"void main()\n"

"{\n"

"    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"

"    fragLocalPos = vertexPosition;\n"

"    fragTexCoord = vertexTexCoord;\n"

"    fragColor = vertexColor;\n"

"    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"

"    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"

"}\n";



static const char *FS_HOUND_SOURCE =

"#version 330\n"

"in vec3 fragPosition;\n"

"in vec3 fragLocalPos;\n"

"in vec2 fragTexCoord;\n"

"in vec4 fragColor;\n"

"in vec3 fragNormal;\n"

"uniform sampler2D texture0;\n"

"uniform vec4 colDiffuse;\n"

"#define MAX_LIGHTS 4\n"

"struct Light { int enabled; int type; vec3 position; vec3 target; vec4 color; };\n"

"uniform Light lights[MAX_LIGHTS];\n"

"uniform vec4 ambient;\n"

"uniform vec3 viewPos;\n"

"uniform vec4 fogColor;\n"

"uniform float fogDensity;\n"

"uniform int matType;\n" // 0=Fur, 1=Skin/Leather, 2=Glowing Eyes, 3=Environment

"uniform float time;\n"

"out vec4 finalColor;\n"

"\n"

"float hash13(vec3 p) {\n"

"    p = fract(p * 0.3183099 + 0.1);\n"

"    p *= 17.0;\n"

"    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));\n"

"}\n"

"float vnoise(vec3 p) {\n"

"    vec3 i = floor(p);\n"

"    vec3 f = fract(p);\n"

"    vec3 u = f * f * (3.0 - 2.0 * f);\n"

"    return mix(mix(mix(hash13(i+vec3(0,0,0)), hash13(i+vec3(1,0,0)), u.x),\n"

"                   mix(hash13(i+vec3(0,1,0)), hash13(i+vec3(1,1,0)), u.x), u.y),\n"

"               mix(mix(hash13(i+vec3(0,0,1)), hash13(i+vec3(1,0,1)), u.x),\n"

"                   mix(hash13(i+vec3(0,1,1)), hash13(i+vec3(1,1,1)), u.x), u.y), u.z);\n"

"}\n"

"float fbm(vec3 p) {\n"

"    float v = 0.0, a = 0.5;\n"

"    for (int i = 0; i < 4; i++) { v += a * vnoise(p); p *= 2.02; a *= 0.5; }\n"

"    return v;\n"

"}\n"

"void main()\n"

"{\n"

"    vec4 tint = colDiffuse * fragColor;\n"

"    vec3 normal = normalize(fragNormal);\n"

"    vec3 viewD = normalize(viewPos - fragPosition);\n"

"    vec3 emission = vec3(0.0);\n"

"\n"

"    if (matType == 0) // FUR WITH PROCEDURAL SPOTS\n"

"    {\n"

"        vec3 lp = fragLocalPos * 14.0;\n"

"        vec3 warp = vec3(fbm(lp), fbm(lp + 13.0), fbm(lp + 27.0));\n"

"        float spotNoise = fbm(lp + warp * 1.8);\n"

"        float spotMask = smoothstep(0.40, 0.52, spotNoise);\n"

"        vec3 spotColor = vec3(0.08, 0.07, 0.07);\n"

"        tint.rgb = mix(tint.rgb, spotColor, spotMask);\n"

"\n"

"        float furNoise = fbm(fragLocalPos * 150.0);\n"

"        float clump = fbm(fragLocalPos * 30.0);\n"

"        tint.rgb *= mix(0.75, 1.15, furNoise) * mix(0.85, 1.1, clump);\n"

"        float dirAO = clamp(normal.y * 0.4 + 0.6, 0.2, 1.0);\n"

"        tint.rgb *= dirAO;\n"

"        vec3 furBump = normalize(vec3(vnoise(fragLocalPos*180.0)-0.5, vnoise(fragLocalPos*180.0+20.0)-0.5, 1.0));\n"

"        normal = normalize(normal + furBump * 0.15);\n"

"    }\n"

"    else if (matType == 1) // LEATHERY SKIN (Nose/Paws/Teeth)\n"

"    {\n"

"        float grain = fbm(fragLocalPos * 50.0);\n"

"        tint.rgb *= mix(0.85, 1.10, grain);\n"

"    }\n"

"    else if (matType == 2) // GLOWING EYES\n"

"    {\n"

"        float pulse = sin(time * 5.0) * 0.2 + 0.8;\n"

"        emission = tint.rgb * pulse * 2.5;\n"

"    }\n"

"\n"

"    vec3 lightDot = vec3(0.0);\n"

"    vec3 specular = vec3(0.0);\n"

"    float specPower = (matType == 1) ? 24.0 : 4.0;\n"

"    float specStrength = (matType == 1) ? 0.2 : 0.02;\n"

"    if (matType == 2) specStrength = 0.5;\n"

"\n"

"    for (int i = 0; i < MAX_LIGHTS; i++)\n"

"    {\n"

"        if (lights[i].enabled == 1)\n"

"        {\n"

"            vec3 lightDir = normalize(lights[i].position - fragPosition);\n"

"            float dist = length(lights[i].position - fragPosition);\n"

"            float atten = 1.0 / (1.0 + 0.05 * dist + 0.01 * dist * dist);\n"

"            float NdotL = max(dot(normal, lightDir), 0.0);\n"

"            lightDot += lights[i].color.rgb * NdotL * atten;\n"

"            if (NdotL > 0.0)\n"

"            {\n"

"                float specCo = pow(max(0.0, dot(viewD, reflect(-lightDir, normal))), specPower);\n"

"                specular += specCo * atten * specStrength * lights[i].color.rgb;\n"

"            }\n"

"        }\n"

"    }\n"

"\n"

"    vec3 lit = tint.rgb * (lightDot + ambient.rgb) + specular + emission;\n"

"    \n"

"    if (matType == 0) {\n"

"        float rim = 1.0 - max(dot(viewD, normal), 0.0);\n"

"        lit += vec3(0.4, 0.45, 0.5) * pow(rim, 4.0) * 0.25;\n"

"    }\n"

"\n"

"    float d = length(viewPos - fragPosition);\n"

"    float fogFactor = clamp(1.0 - exp(-fogDensity * d), 0.0, 1.0);\n"

"    vec3 finalRGB = mix(lit, fogColor.rgb, fogFactor);\n"

"    finalColor = vec4(pow(finalRGB, vec3(1.0/2.15)), tint.a);\n"

"}\n";



// (HoundSceneLight declared in hound_npc.h)



static const char *VS_SKELETON_HOUND_SOURCE =

"#version 330\n"

"in vec3 vertexPosition;\n"

"in vec2 vertexTexCoord;\n"

"in vec3 vertexNormal;\n"

"in vec4 vertexColor;\n"

"uniform mat4 mvp;\n"

"uniform mat4 matModel;\n"

"uniform mat4 matNormal;\n"

"out vec3 fragPosition;\n"

"out vec2 fragTexCoord;\n"

"out vec4 fragColor;\n"

"out vec3 fragNormal;\n"

"void main()\n"

"{\n"

"    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"

"    fragTexCoord = vertexTexCoord;\n"

"    fragColor = vertexColor;\n"

"    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"

"    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"

"}\n";



static const char *FS_SKELETON_HOUND_SOURCE =

"#version 330\n"

"in vec3 fragPosition;\n"

"in vec2 fragTexCoord;\n"

"in vec4 fragColor;\n"

"in vec3 fragNormal;\n"

"uniform sampler2D texture0;\n"

"uniform vec4 colDiffuse;\n"

"#define MAX_LIGHTS 4\n"

"struct Light { int enabled; int type; vec3 position; vec3 target; vec4 color; };\n"

"uniform Light lights[MAX_LIGHTS];\n"

"uniform vec4 ambient;\n"

"uniform vec3 viewPos;\n"

"uniform vec4 fogColor;\n"

"uniform float fogDensity;\n"

"uniform int matType;\n" // 0=Bone, 1=Dark Cavity, 2=Glowing Eyes, 3=Teeth

"uniform float time;\n"

"out vec4 finalColor;\n"

"\n"

"float hash13(vec3 p) {\n"

"    p = fract(p * 0.3183099 + 0.1);\n"

"    p *= 17.0;\n"

"    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));\n"

"}\n"

"float vnoise(vec3 p) {\n"

"    vec3 i = floor(p);\n"

"    vec3 f = fract(p);\n"

"    vec3 u = f * f * (3.0 - 2.0 * f);\n"

"    return mix(mix(mix(hash13(i+vec3(0,0,0)), hash13(i+vec3(1,0,0)), u.x),\n"

"                   mix(hash13(i+vec3(0,1,0)), hash13(i+vec3(1,1,0)), u.x), u.y),\n"

"               mix(mix(hash13(i+vec3(0,0,1)), hash13(i+vec3(1,0,1)), u.x),\n"

"                   mix(hash13(i+vec3(0,1,1)), hash13(i+vec3(1,1,1)), u.x), u.y), u.z);\n"

"}\n"

"float fbm(vec3 p) {\n"

"    float v = 0.0, a = 0.5;\n"

"    for (int i = 0; i < 4; i++) { v += a * vnoise(p); p *= 2.02; a *= 0.5; }\n"

"    return v;\n"

"}\n"

"void main()\n"

"{\n"

"    vec4 tint = colDiffuse * fragColor;\n"

"    vec3 normal = normalize(fragNormal);\n"

"    vec3 viewD = normalize(viewPos - fragPosition);\n"

"    vec3 emission = vec3(0.0);\n"

"\n"

"    if (matType == 0 || matType == 3) // BONE OR TEETH\n"

"    {\n"

"        float grain = fbm(fragPosition * 30.0);\n"

"        tint.rgb *= mix(0.85, 1.10, grain);\n"

"        float pits = vnoise(fragPosition * 250.0);\n"

"        tint.rgb *= mix(0.88, 1.0, pits);\n"

"        float dirAO = clamp(normal.y * 0.4 + 0.6, 0.25, 1.0);\n"

"        tint.rgb *= dirAO;\n"

"        vec3 boneBump = normalize(vec3(vnoise(fragPosition*180.0)-0.5, vnoise(fragPosition*180.0+20.0)-0.5, 1.0));\n"

"        normal = normalize(normal + boneBump * 0.05);\n"

"    }\n"

"    else if (matType == 2) // GLOWING EYES\n"

"    {\n"

"        float pulse = sin(time * 6.0) * 0.15 + 0.85;\n"

"        emission = tint.rgb * pulse * 3.0;\n"

"    }\n"

"\n"

"    vec3 lightDot = vec3(0.0);\n"

"    vec3 specular = vec3(0.0);\n"

"    float specPower = (matType == 0 || matType == 3) ? 12.0 : 4.0;\n"

"    float specStrength = (matType == 0 || matType == 3) ? 0.08 : 0.02;\n"

"    if (matType == 2) specStrength = 0.5;\n"

"\n"

"    for (int i = 0; i < MAX_LIGHTS; i++)\n"

"    {\n"

"        if (lights[i].enabled == 1)\n"

"        {\n"

"            vec3 lightDir = normalize(lights[i].position - fragPosition);\n"

"            float dist = length(lights[i].position - fragPosition);\n"

"            float atten = 1.0 / (1.0 + 0.05 * dist + 0.01 * dist * dist);\n"

"            float NdotL = max(dot(normal, lightDir), 0.0);\n"

"            lightDot += lights[i].color.rgb * NdotL * atten;\n"

"            if (NdotL > 0.0)\n"

"            {\n"

"                float specCo = pow(max(0.0, dot(viewD, reflect(-lightDir, normal))), specPower);\n"

"                specular += specCo * atten * specStrength * lights[i].color.rgb;\n"

"            }\n"

"        }\n"

"    }\n"

"\n"

"    vec3 lit = tint.rgb * (lightDot + ambient.rgb) + specular + emission;\n"

"    \n"

"    if (matType == 0 || matType == 3) {\n"

"        float rim = 1.0 - max(dot(viewD, normal), 0.0);\n"

"        lit += vec3(0.45, 0.50, 0.55) * pow(rim, 3.5) * 0.20;\n"

"    }\n"

"\n"

"    float d = length(viewPos - fragPosition);\n"

"    float fogFactor = clamp(1.0 - exp(-fogDensity * d), 0.0, 1.0);\n"

"    vec3 finalRGB = mix(lit, fogColor.rgb, fogFactor);\n"

"    finalColor = vec4(pow(finalRGB, vec3(1.0/2.15)), tint.a);\n"

"}\n";



Shader g_houndShader = { 0 };

int g_houndMatTypeLoc = -1;

int g_houndTimeLoc = -1;

int g_houndViewPosLoc = -1;

int g_houndAmbientLoc = -1;

int g_houndFogColorLoc = -1;

int g_houndFogDensityLoc = -1;

HoundSceneLight g_houndLights[4];



Shader g_houndSkeletonShader = { 0 };

int g_houndSkeletonMatTypeLoc = -1;

int g_houndSkeletonTimeLoc = -1;

int g_houndSkeletonViewPosLoc = -1;

int g_houndSkeletonAmbientLoc = -1;

int g_houndSkeletonFogColorLoc = -1;

int g_houndSkeletonFogDensityLoc = -1;

HoundSceneLight g_houndSkeletonLights[4];



Mesh g_houndCyl = { 0 };

Mesh g_houndSphere = { 0 };

Mesh g_houndCube = { 0 };

Mesh g_houndCone = { 0 };

Material g_houndMat = { 0 };

Material g_houndSkeletonMat = { 0 };

bool g_houndResourcesLoaded = false;



static inline void HoundSetMaterialType(Shader shader, int matType) {

    if (shader.id == g_houndSkeletonShader.id) {

        if (g_houndSkeletonMatTypeLoc >= 0) SetShaderValue(shader, g_houndSkeletonMatTypeLoc, &matType, SHADER_UNIFORM_INT);

    } else {

        if (g_houndMatTypeLoc >= 0) SetShaderValue(shader, g_houndMatTypeLoc, &matType, SHADER_UNIFORM_INT);

    }

}



void HoundUpdateLight(int index, int enabled, int type, Vector3 pos, Vector3 target, Color col) {

    if (index < 0 || index >= 4 || !g_houndResourcesLoaded) return;

    SetShaderValue(g_houndShader, g_houndLights[index].enabledLoc, &enabled, SHADER_UNIFORM_INT);

    SetShaderValue(g_houndShader, g_houndLights[index].typeLoc, &type, SHADER_UNIFORM_INT);

    float p[3] = { pos.x, pos.y, pos.z };

    SetShaderValue(g_houndShader, g_houndLights[index].posLoc, p, SHADER_UNIFORM_VEC3);

    float t[3] = { target.x, target.y, target.z };

    SetShaderValue(g_houndShader, g_houndLights[index].targetLoc, t, SHADER_UNIFORM_VEC3);

    float c[4] = { col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f };

    SetShaderValue(g_houndShader, g_houndLights[index].colorLoc, c, SHADER_UNIFORM_VEC4);

}



void HoundSkeletonUpdateLight(int index, int enabled, int type, Vector3 pos, Vector3 target, Color col) {

    if (index < 0 || index >= 4 || !g_houndResourcesLoaded) return;

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].enabledLoc, &enabled, SHADER_UNIFORM_INT);

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].typeLoc, &type, SHADER_UNIFORM_INT);

    float p[3] = { pos.x, pos.y, pos.z };

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].posLoc, p, SHADER_UNIFORM_VEC3);

    float t[3] = { target.x, target.y, target.z };

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].targetLoc, t, SHADER_UNIFORM_VEC3);

    float c[4] = { col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f };

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].colorLoc, c, SHADER_UNIFORM_VEC4);

}



static Vector3 HoundRotateAroundAxis(Vector3 v, Vector3 axis, float angleRad) {

    axis = Vector3Normalize(axis);

    float c = cosf(angleRad), s = sinf(angleRad);

    Vector3 t1 = Vector3Scale(v, c);

    Vector3 t2 = Vector3Scale(Vector3CrossProduct(axis, v), s);

    Vector3 t3 = Vector3Scale(axis, Vector3DotProduct(axis, v) * (1.0f - c));

    return Vector3Add(Vector3Add(t1, t2), t3);

}



static void HoundDrawBoneSegment(Mesh mesh, Material *mat, Vector3 from, Vector3 to, float radiusX, float radiusZ, Color tint, int mType = 0) {

    Vector3 diff = Vector3Subtract(to, from);

    float len = Vector3Length(diff);

    if (len < 0.0001f) return;

    Vector3 dir = Vector3Scale(diff, 1.0f / len);

    Quaternion q = QuaternionFromVector3ToVector3(Vector3{ 0, 1, 0 }, dir);

    Matrix rot = QuaternionToMatrix(q);

    Matrix scale = MatrixScale(radiusX, len, radiusZ);

    Matrix translate = MatrixTranslate(from.x, from.y, from.z);

    HoundSetMaterialType(mat->shader, mType);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, MatrixMultiply(MatrixMultiply(scale, rot), translate));

}



static void HoundDrawForm(Mesh mesh, Material *mat, Vector3 center, Vector3 radii, Vector3 eulerDeg, Color tint, int mType = 0) {

    Matrix s = MatrixScale(radii.x, radii.y, radii.z);

    Matrix r = MatrixRotateXYZ(Vector3{ DEG2RAD * eulerDeg.x, DEG2RAD * eulerDeg.y, DEG2RAD * eulerDeg.z });

    Matrix t = MatrixTranslate(center.x, center.y, center.z);

    HoundSetMaterialType(mat->shader, mType);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, MatrixMultiply(MatrixMultiply(s, r), t));

}



static void HoundDrawPartLocal(Mesh mesh, Material *mat, Matrix parentWorld, Vector3 localOffset, Vector3 localScale, Vector3 localEulerDeg, Color tint, int mType = 0) {

    Matrix s = MatrixScale(localScale.x, localScale.y, localScale.z);

    Matrix r = MatrixRotateXYZ(Vector3{ DEG2RAD * localEulerDeg.x, DEG2RAD * localEulerDeg.y, DEG2RAD * localEulerDeg.z });

    Matrix t = MatrixTranslate(localOffset.x, localOffset.y, localOffset.z);

    Matrix local = MatrixMultiply(MatrixMultiply(s, r), t);

    Matrix world = MatrixMultiply(local, parentWorld);

    HoundSetMaterialType(mat->shader, mType);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, world);

}



// (DogState and DogNPC declared in hound_npc.h)



DogNPC g_houndNPC = { 0 };



void InitDog(DogNPC &dog, Vector3 startPos, float startYaw) {

    dog = (DogNPC){ 0 };

    dog.pos = startPos;

    dog.yaw = startYaw;

    dog.targetYaw = startYaw;

    dog.homePos = startPos;

    dog.wanderRadius = 10.0f;

    dog.state = DOG_STATE_SIT;

    dog.sitBlend = 1.0f;

    dog.stateTimer = 8.0f;

    dog.barkCooldown = 12.0f;

}



void InitHoundResources() {

    if (!g_houndResourcesLoaded) {

        g_houndCyl    = GenMeshCylinder(1.0f, 1.0f, 16);

        g_houndSphere = GenMeshSphere(1.0f, 16, 16);

        g_houndCube   = GenMeshCube(1.0f, 1.0f, 1.0f);

        g_houndCone   = GenMeshCone(1.0f, 1.0f, 16);



        g_houndShader = LoadShaderFromMemory(VS_HOUND_SOURCE, FS_HOUND_SOURCE);

        g_houndShader.locs[SHADER_LOC_MATRIX_MVP]   = GetShaderLocation(g_houndShader, "mvp");

        g_houndShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(g_houndShader, "matModel");

        g_houndShader.locs[SHADER_LOC_MATRIX_NORMAL]= GetShaderLocation(g_houndShader, "matNormal");

        g_houndShader.locs[SHADER_LOC_COLOR_DIFFUSE]= GetShaderLocation(g_houndShader, "colDiffuse");



        g_houndMatTypeLoc    = GetShaderLocation(g_houndShader, "matType");

        g_houndTimeLoc       = GetShaderLocation(g_houndShader, "time");

        g_houndViewPosLoc    = GetShaderLocation(g_houndShader, "viewPos");

        g_houndAmbientLoc    = GetShaderLocation(g_houndShader, "ambient");

        g_houndFogColorLoc   = GetShaderLocation(g_houndShader, "fogColor");

        g_houndFogDensityLoc = GetShaderLocation(g_houndShader, "fogDensity");



        for (int i = 0; i < 4; i++) {

            g_houndLights[i].enabledLoc = GetShaderLocation(g_houndShader, TextFormat("lights[%i].enabled", i));

            g_houndLights[i].typeLoc    = GetShaderLocation(g_houndShader, TextFormat("lights[%i].type", i));

            g_houndLights[i].posLoc     = GetShaderLocation(g_houndShader, TextFormat("lights[%i].position", i));

            g_houndLights[i].targetLoc  = GetShaderLocation(g_houndShader, TextFormat("lights[%i].target", i));

            g_houndLights[i].colorLoc   = GetShaderLocation(g_houndShader, TextFormat("lights[%i].color", i));

        }



        g_houndMat = LoadMaterialDefault();

        g_houndMat.shader = g_houndShader;



        // Compile and initialize nighttime canine skeleton shader

        g_houndSkeletonShader = LoadShaderFromMemory(VS_SKELETON_HOUND_SOURCE, FS_SKELETON_HOUND_SOURCE);

        g_houndSkeletonShader.locs[SHADER_LOC_MATRIX_MVP]   = GetShaderLocation(g_houndSkeletonShader, "mvp");

        g_houndSkeletonShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(g_houndSkeletonShader, "matModel");

        g_houndSkeletonShader.locs[SHADER_LOC_MATRIX_NORMAL]= GetShaderLocation(g_houndSkeletonShader, "matNormal");

        g_houndSkeletonShader.locs[SHADER_LOC_COLOR_DIFFUSE]= GetShaderLocation(g_houndSkeletonShader, "colDiffuse");



        g_houndSkeletonMatTypeLoc    = GetShaderLocation(g_houndSkeletonShader, "matType");

        g_houndSkeletonTimeLoc       = GetShaderLocation(g_houndSkeletonShader, "time");

        g_houndSkeletonViewPosLoc    = GetShaderLocation(g_houndSkeletonShader, "viewPos");

        g_houndSkeletonAmbientLoc    = GetShaderLocation(g_houndSkeletonShader, "ambient");

        g_houndSkeletonFogColorLoc   = GetShaderLocation(g_houndSkeletonShader, "fogColor");

        g_houndSkeletonFogDensityLoc = GetShaderLocation(g_houndSkeletonShader, "fogDensity");



        for (int i = 0; i < 4; i++) {

            g_houndSkeletonLights[i].enabledLoc = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].enabled", i));

            g_houndSkeletonLights[i].typeLoc    = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].type", i));

            g_houndSkeletonLights[i].posLoc     = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].position", i));

            g_houndSkeletonLights[i].targetLoc  = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].target", i));

            g_houndSkeletonLights[i].colorLoc   = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].color", i));

        }



        g_houndSkeletonMat = LoadMaterialDefault();

        g_houndSkeletonMat.shader = g_houndSkeletonShader;



        g_houndResourcesLoaded = true;



        // Spawn faithful Hound at gas station pump apron facing incoming car

        InitDog(g_houndNPC, Vector3{ 124.0f, 10.0f, 135.0f }, 0.0f);

    }

}



void UnloadHoundResources() {

    if (g_houndResourcesLoaded) {

        UnloadMesh(g_houndCyl);

        UnloadMesh(g_houndSphere);

        UnloadMesh(g_houndCube);

        UnloadMesh(g_houndCone);

        UnloadShader(g_houndShader);

        UnloadShader(g_houndSkeletonShader);

        g_houndResourcesLoaded = false;

    }

}



void UpdateDog(DogNPC &dog, float dt) {

    dog.animTime += dt;



    // FSM State Blending Targets

    float tWalk = (dog.state == DOG_STATE_WALK) ? 1.0f : 0.0f;

    float tRun  = (dog.state == DOG_STATE_RUN)  ? 1.0f : 0.0f;

    float tSit  = (dog.state == DOG_STATE_SIT)  ? 1.0f : 0.0f;

    float tBark = (dog.state == DOG_STATE_BARK) ? 1.0f : 0.0f;



    // Smooth Transitions

    float blendSpd = dt * 6.0f;

    dog.walkBlend = Lerp(dog.walkBlend, tWalk, blendSpd);

    dog.runBlend  = Lerp(dog.runBlend,  tRun,  blendSpd);

    dog.sitBlend  = Lerp(dog.sitBlend,  tSit,  blendSpd);

    dog.barkBlend = Lerp(dog.barkBlend, tBark, blendSpd * 1.5f);



    // Barking Logic

    if (dog.state == DOG_STATE_BARK) {

        dog.barkTimer -= dt;

        dog.barkPhase += dt * 35.0f; // Rapid jaw chattering

        if (dog.barkTimer <= 0.0f) dog.state = DOG_STATE_IDLE;

    }



    // Root Motion (Sprints smoothly to keep pace with player when companion)

    float runSpd = dog.isPet ? 5.2f : 3.8f;

    float curSpeed = (1.3f * dog.walkBlend) + (runSpd * dog.runBlend);

    if (curSpeed > 0.01f) {

        dog.walkPhase += dt * 6.0f * dog.walkBlend;

        dog.runPhase  += dt * 12.0f * dog.runBlend;

        dog.pos.x += sinf(dog.yaw) * curSpeed * dt;

        dog.pos.z += cosf(dog.yaw) * curSpeed * dt;

    }



    float breathe = sinf(dog.animTime * 2.5f) * 0.01f;

    

    // Core Anatomy Heights (grounded at 10.0f)

    float basePelvis = 0.55f;

    float bob = sinf(dog.walkPhase * 2.0f) * 0.03f * dog.walkBlend + 

                fabsf(sinf(dog.runPhase)) * 0.08f * dog.runBlend;

    

    // Sit mechanics: Drop pelvis to floor, but keep chest elevated

    float sitDrop = dog.sitBlend * 0.38f; 

    

    dog.pelvis = Vector3{ dog.pos.x, dog.pos.y + basePelvis + bob - sitDrop + breathe, dog.pos.z };

    

    Vector3 fwd = { sinf(dog.yaw), 0, cosf(dog.yaw) };

    Vector3 right = { cosf(dog.yaw), 0, -sinf(dog.yaw) };

    Vector3 up = { 0, 1, 0 };



    // Spine angle dictates posture. NEGATIVE pitches UP, pushing the chest higher than pelvis

    float spinePitch = dog.sitBlend * -40.0f; 

    Vector3 spineDir = HoundRotateAroundAxis(fwd, right, DEG2RAD * spinePitch);

    

    dog.chest = Vector3Add(dog.pelvis, Vector3Scale(spineDir, 0.45f));

    dog.neckBase = Vector3Add(dog.chest, Vector3Scale(spineDir, 0.1f));

    dog.neckBase.y += 0.05f; // Slight natural arch



    // Neck & Head Kinematics

    float barkPitch = dog.barkBlend * -25.0f; // Head throws UP/BACK when barking

    float lookPitch = 10.0f + barkPitch + (dog.sitBlend * 35.0f); 

    Vector3 neckDir = HoundRotateAroundAxis(spineDir, right, DEG2RAD * lookPitch);

    

    dog.headPivot = Vector3Add(dog.neckBase, Vector3Scale(neckDir, 0.25f));



    // Head Orientation

    float headVisualPitch = -15.0f + (dog.barkBlend * -30.0f) + (dog.sitBlend * -10.0f);

    Matrix headRot = MatrixRotateXYZ(Vector3{ DEG2RAD * headVisualPitch, 0, 0 });

    Matrix bodyRot = MatrixRotateY(dog.yaw);

    Matrix headTrans = MatrixTranslate(dog.headPivot.x, dog.headPivot.y, dog.headPivot.z);

    dog.headWorld = MatrixMultiply(MatrixMultiply(headRot, bodyRot), headTrans);



    // Jaw Pivot (TMJ)

    float jawAngle = dog.barkBlend * fmaxf(0.0f, sinf(dog.barkPhase) * 25.0f) + (dog.runBlend * 10.0f);

    Vector3 jawPivot = { 0.0f, -0.04f, 0.02f };

    Matrix tNeg = MatrixTranslate(-jawPivot.x, -jawPivot.y, -jawPivot.z);

    Matrix rJaw = MatrixRotateX(DEG2RAD * jawAngle);

    Matrix tPos = MatrixTranslate(jawPivot.x, jawPivot.y, jawPivot.z);

    dog.jawWorld = MatrixMultiply(MatrixMultiply(MatrixMultiply(tNeg, rJaw), tPos), dog.headWorld);



    // Shoulders and Hips

    dog.shoulder[0] = Vector3Add(dog.chest, Vector3Scale(right, -0.12f));

    dog.shoulder[1] = Vector3Add(dog.chest, Vector3Scale(right,  0.12f));

    dog.hip[0] = Vector3Add(dog.pelvis, Vector3Scale(right, -0.1f));

    dog.hip[1] = Vector3Add(dog.pelvis, Vector3Scale(right,  0.1f));



    // Tail Kinematics

    float wagRate = (dog.petTimer > 0.0f) ? 22.0f : 8.0f;

    float wagAmp  = (dog.petTimer > 0.0f) ? 0.32f : 0.15f;

    for (int i = 0; i < 6; i++) {

        float drop = (float)i * 0.1f;

        float wag = sinf(dog.animTime * wagRate - drop * 2.0f) * wagAmp * (1.0f - dog.sitBlend * 0.4f);

        float tuck = dog.sitBlend * 0.2f * i * (dog.petTimer > 0.0f ? 0.3f : 1.0f); // Tail wags free when pet

        Vector3 node = Vector3Add(dog.pelvis, Vector3Scale(fwd, -drop * 0.8f + tuck));

        node = Vector3Add(node, Vector3Scale(up, -drop * 0.6f - tuck));

        node = Vector3Add(node, Vector3Scale(right, wag));

        dog.tailNodes[i] = node;

    }

}



void UpdateDogAI(DogNPC &dog, Vector3 playerPos, float dt, float lightningTimer, bool isNight) {

    (void)isNight;
    dog.pos.y = 10.0f;



    // Smooth heading rotation towards targetYaw

    float diffYaw = fmodf(dog.targetYaw - dog.yaw + PI, 2.0f * PI);

    if (diffYaw < 0.0f) diffYaw += 2.0f * PI;

    diffYaw -= PI;

    dog.yaw += diffYaw * dt * 4.0f;



    // 1. Loyal Pet Companion Following Behavior

    if (dog.isPet) {

        float dx = playerPos.x - dog.pos.x;

        float dz = playerPos.z - dog.pos.z;

        float distToPlayer = sqrtf(dx * dx + dz * dz);



        if (distToPlayer > 0.15f) {

            dog.targetYaw = atan2f(dx, dz);

        }



        // Lapping blood timer

        if (dog.drinkTimer > 0.0f) {

            dog.drinkTimer -= dt;

            dog.state = DOG_STATE_BARK;

            dog.barkPhase += dt * 16.0f;

            return;

        }



        // Being petted

        if (dog.petTimer > 0.0f) {

            dog.petTimer -= dt;

            dog.state = DOG_STATE_SIT;

            return;

        }



        // Spooky alert even when pet: bark at thunder or darkness

        if (lightningTimer > 0.4f && dog.barkCooldown <= 0.0f) {

            dog.state = DOG_STATE_BARK;

            dog.barkTimer = 1.0f;

            dog.barkCooldown = 12.0f;

            return;

        }

        if (dog.barkCooldown > 0.0f) dog.barkCooldown -= dt;



        // Dynamic Following: Sprint if player is far, walk if close, sit at player's feet

        if (distToPlayer > 7.5f) {

            dog.state = DOG_STATE_RUN;  // Sprint to catch up with player!

        } else if (distToPlayer > 2.2f) {

            dog.state = DOG_STATE_WALK; // Trot alongside player

        } else {

            dog.state = DOG_STATE_SIT;  // Sit down faithfully at player's feet

        }

        return;

    }



    if (dog.petTimer > 0.0f) {

        dog.petTimer -= dt;

        dog.state = DOG_STATE_SIT;

    }



    // 2. Spooky Reaction: Thunder/Lightning or Grethnar

    if (lightningTimer > 0.4f && dog.barkCooldown <= 0.0f) {

        dog.state = DOG_STATE_BARK;

        dog.barkTimer = 1.2f;

        dog.barkCooldown = 10.0f;

        dog.targetYaw = atan2f(playerPos.x - dog.pos.x, playerPos.z - dog.pos.z);

        return;

    }

    if (dog.barkCooldown > 0.0f) dog.barkCooldown -= dt;



    // 3. Player Proximity Dynamics (Pre-pet)

    float dx = playerPos.x - dog.pos.x;

    float dz = playerPos.z - dog.pos.z;

    float distToPlayer = sqrtf(dx * dx + dz * dz);



    if (distToPlayer < 3.5f) {

        // Look towards player

        dog.targetYaw = atan2f(dx, dz);

        if (dog.state == DOG_STATE_WALK) {

            dog.state = DOG_STATE_SIT;

            dog.stateTimer = Frand(4.0f, 8.0f);

        }

        return;

    }



    // 4. Autonomous Patrol & Sitting Routine around gas station apron

    dog.stateTimer -= dt;

    if (dog.stateTimer <= 0.0f) {

        float hDx = dog.pos.x - dog.homePos.x;

        float hDz = dog.pos.z - dog.homePos.z;

        float distFromHome = sqrtf(hDx * hDx + hDz * hDz);



        int roll = GetRandomValue(0, 100);

        if (distFromHome > dog.wanderRadius) {

            dog.state = DOG_STATE_WALK;

            dog.targetYaw = atan2f(-hDx, -hDz);

            dog.stateTimer = Frand(4.0f, 7.0f);

        } else if (roll < 45) {

            // Sit and guard the apron

            dog.state = DOG_STATE_SIT;

            dog.stateTimer = Frand(6.0f, 14.0f);

        } else if (roll < 75) {

            // Stand idle, breathing, looking around

            dog.state = DOG_STATE_IDLE;

            dog.stateTimer = Frand(3.0f, 6.0f);

        } else {

            // Patrol to a new waypoint near the pumps

            dog.state = DOG_STATE_WALK;

            dog.targetYaw = Frand(-PI, PI);

            dog.stateTimer = Frand(3.0f, 6.0f);

        }

    }

}



void DrawHound(const DogNPC &dog, Mesh cyl, Mesh sphere, Mesh cube, Mesh cone, Material *mat)

{

    Color furCol   = { 170, 160, 145, 255 }; 

    Color skinCol  = {  12,  10,  10, 255 }; 

    Color eyeCol   = { 255,  30,  10, 255 }; 

    Color toothCol = { 200, 190, 180, 255 }; 

    Color voidCol  = {   5,   0,   0, 255 }; 



    Vector3 fwd = { sinf(dog.yaw), 0, cosf(dog.yaw) };

    Vector3 right = { cosf(dog.yaw), 0, -sinf(dog.yaw) };

    Vector3 up = { 0, 1, 0 };



    // 1. TORSO & SPINE

    float chestScale = 1.0f + (dog.barkBlend * 0.15f);

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, dog.chest, 0.14f, 0.16f, furCol, 0);

    HoundDrawForm(sphere, mat, dog.chest, Vector3{0.18f * chestScale, 0.22f * chestScale, 0.22f * chestScale}, Vector3{0, dog.yaw*RAD2DEG, 0}, furCol, 0);

    HoundDrawForm(sphere, mat, dog.pelvis, Vector3{0.14f, 0.14f, 0.16f}, Vector3{0, dog.yaw*RAD2DEG, 0}, furCol, 0);

    

    HoundDrawBoneSegment(cyl, mat, dog.chest, dog.headPivot, 0.08f, 0.10f, furCol, 0);



    for (int i = 0; i < 5; i++) {

        HoundDrawBoneSegment(cyl, mat, dog.tailNodes[i], dog.tailNodes[i+1], 0.035f - (i*0.005f), 0.035f - (i*0.005f), furCol, 0);

    }



    // 2. FORELIMBS (Scapula -> Humerus -> Radius -> Paw)

    for (int side = 0; side < 2; side++) {

        float off = (side == 0) ? 0.0f : PI;

        float walkS = sinf(dog.walkPhase + off) * 0.4f * dog.walkBlend;

        float runS  = sinf(dog.runPhase + ((side==0)?0.0f:-0.4f)) * 0.8f * dog.runBlend;

        float swing = walkS + runS;

        

        float carpalFlex = fmaxf(0.0f, sinf(dog.walkPhase + off)) * 0.6f * dog.walkBlend + 

                           fmaxf(0.0f, sinf(dog.runPhase + ((side==0)?0.0f:-0.4f))) * 1.0f * dog.runBlend;



        float foldHumerus = dog.sitBlend * -0.2f; 

        float foldRadius  = 0.0f; 



        Vector3 shoulderJoint = dog.shoulder[side]; 

        

        Vector3 elbowDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.2f))), right, swing + foldHumerus);

        Vector3 elbow = Vector3Add(shoulderJoint, Vector3Scale(elbowDir, 0.25f));

        

        Vector3 carpalDir = HoundRotateAroundAxis(Vector3Scale(up, -1.0f), right, swing - carpalFlex + foldRadius); 

        Vector3 carpal = Vector3Add(elbow, Vector3Scale(carpalDir, 0.22f)); 

        

        Vector3 paw = Vector3Add(carpal, Vector3{0, -0.05f, 0.05f});

        paw.y = fmaxf(dog.pos.y + 0.02f, paw.y);



        HoundDrawBoneSegment(cyl, mat, shoulderJoint, elbow, 0.05f, 0.06f, furCol, 0);

        HoundDrawForm(sphere, mat, elbow, Vector3{0.055f, 0.055f, 0.055f}, Vector3{0,0,0}, furCol, 0);

        HoundDrawBoneSegment(cyl, mat, elbow, carpal, 0.04f, 0.04f, furCol, 0);

        HoundDrawForm(sphere, mat, carpal, Vector3{0.045f, 0.045f, 0.045f}, Vector3{0,0,0}, furCol, 0);

        HoundDrawForm(cube, mat, paw, Vector3{0.08f, 0.06f, 0.10f}, Vector3{0, dog.yaw*RAD2DEG, 0}, furCol, 0);

    }



    // 3. HINDLIMBS (Femur -> Tibia -> Metatarsus -> Paw)

    for (int side = 0; side < 2; side++) {

        float off = (side == 0) ? PI/2.0f : 3.0f*PI/2.0f; 

        float walkS = sinf(dog.walkPhase + off) * 0.45f * dog.walkBlend;

        float runS  = sinf(dog.runPhase + PI + ((side==0)?0.0f:-0.4f)) * 0.9f * dog.runBlend;

        float swing = walkS + runS;

        

        float hockFlex = fmaxf(0.0f, -sinf(dog.walkPhase + off)) * 0.8f * dog.walkBlend + 

                         fmaxf(0.0f, -sinf(dog.runPhase + PI + ((side==0)?0.0f:-0.4f))) * 1.2f * dog.runBlend;



        float foldFemur  = dog.sitBlend * -1.0f; 

        float foldTibia  = dog.sitBlend *  1.2f; 

        float foldCannon = dog.sitBlend * -1.5f; 



        Vector3 hipJoint = dog.hip[side];



        Vector3 stifleDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd,  0.4f))), right, swing + foldFemur);

        Vector3 stifle = Vector3Add(hipJoint, Vector3Scale(stifleDir, 0.28f)); 



        Vector3 hockDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.4f))), right, swing + hockFlex + foldTibia);

        Vector3 hock = Vector3Add(stifle, Vector3Scale(hockDir, 0.26f));



        Vector3 metaDir = HoundRotateAroundAxis(Vector3Scale(up, -1.0f), right, swing + hockFlex + foldCannon);

        Vector3 fetlock = Vector3Add(hock, Vector3Scale(metaDir, 0.18f));

        

        Vector3 paw = Vector3Add(fetlock, Vector3{0, -0.04f, 0.06f});

        paw.y = fmaxf(dog.pos.y + 0.02f, paw.y);



        HoundDrawBoneSegment(cyl, mat, hipJoint, stifle, 0.08f, 0.09f, furCol, 0);

        HoundDrawForm(sphere, mat, stifle, Vector3{0.065f, 0.065f, 0.065f}, Vector3{0,0,0}, furCol, 0);

        HoundDrawBoneSegment(cyl, mat, stifle, hock, 0.05f, 0.05f, furCol, 0);

        HoundDrawForm(sphere, mat, hock, Vector3{0.055f, 0.065f, 0.055f}, Vector3{0,0,0}, furCol, 0);

        HoundDrawBoneSegment(cyl, mat, hock, fetlock, 0.035f, 0.035f, furCol, 0);

        HoundDrawForm(cube, mat, paw, Vector3{0.07f, 0.06f, 0.10f}, Vector3{0, dog.yaw*RAD2DEG, 0}, furCol, 0);

    }



    // 4. HEAD & SNOUT

    Matrix hw = dog.headWorld;



    HoundDrawPartLocal(sphere, mat, hw, Vector3{ 0, 0, 0 }, Vector3{0.09f, 0.09f, 0.10f}, Vector3{0,0,0}, furCol, 0);

    HoundDrawPartLocal(cube, mat, hw, Vector3{ 0, -0.02f, 0.12f }, Vector3{0.06f, 0.06f, 0.14f}, Vector3{0,0,0}, furCol, 0);

    HoundDrawPartLocal(sphere, mat, hw, Vector3{ 0, -0.01f, 0.19f }, Vector3{0.025f, 0.02f, 0.02f}, Vector3{0,0,0}, skinCol, 1);

    

    HoundDrawPartLocal(sphere, mat, hw, Vector3{ -0.045f, 0.02f, 0.08f }, Vector3{0.015f, 0.015f, 0.015f}, Vector3{0,0,0}, eyeCol, 2);

    HoundDrawPartLocal(sphere, mat, hw, Vector3{  0.045f, 0.02f, 0.08f }, Vector3{0.015f, 0.015f, 0.015f}, Vector3{0,0,0}, eyeCol, 2);



    HoundDrawPartLocal(cone, mat, hw, Vector3{ -0.07f, 0.08f, -0.02f }, Vector3{0.03f, 0.10f, 0.04f}, Vector3{-15, 0, -25}, furCol, 0);

    HoundDrawPartLocal(cone, mat, hw, Vector3{  0.07f, 0.08f, -0.02f }, Vector3{0.03f, 0.10f, 0.04f}, Vector3{-15, 0,  25}, furCol, 0);



    HoundDrawPartLocal(cube, mat, hw, Vector3{ 0, -0.055f, 0.14f }, Vector3{0.045f, 0.01f, 0.08f}, Vector3{0,0,0}, toothCol, 1);



    // 5. JAW

    Matrix jw = dog.jawWorld;

    HoundDrawPartLocal(cube, mat, jw, Vector3{ 0, -0.03f, 0.11f }, Vector3{0.05f, 0.04f, 0.13f}, Vector3{0,0,0}, furCol, 0);

    HoundDrawPartLocal(cube, mat, jw, Vector3{ 0, -0.005f, 0.13f }, Vector3{0.04f, 0.01f, 0.07f}, Vector3{0,0,0}, toothCol, 1);

    HoundDrawPartLocal(cube, mat, jw, Vector3{ 0, -0.01f, 0.08f }, Vector3{0.04f, 0.02f, 0.08f}, Vector3{0,0,0}, voidCol, 1);

}



// ----------------------------------------------------------------------------------

// PROCEDURAL CANINE OSTEOLOGY (Nighttime Skeleton Hound)

// ----------------------------------------------------------------------------------

void DrawSkeletonHound(const DogNPC &dog, Mesh cyl, Mesh sphere, Mesh cube, Mesh cone, Material *mat)

{

    Color boneCol = { 218, 212, 198, 255 }; // Aged yellowed calcium

    Color voidCol = {  12,   8,   6, 255 }; // Deep dark internal cavity

    Color eyeCol  = { 255,  30,  10, 255 }; // Glowing demonic red

    Color toothCol= { 200, 190, 180, 255 }; // Discolored teeth



    Vector3 fwd = { sinf(dog.yaw), 0, cosf(dog.yaw) };

    Vector3 right = { cosf(dog.yaw), 0, -sinf(dog.yaw) };

    Vector3 up = { 0, 1, 0 };



    // ==========================================================

    // 1. AXIAL SKELETON: SPINE & PELVIS

    // ==========================================================

    for (int i = 0; i < 12; i++) {

        float t = (float)i / 11.0f;

        Vector3 vPos = Vector3Lerp(dog.pelvis, dog.chest, t);

        vPos = Vector3Add(vPos, Vector3Scale(up, sinf(t * PI) * 0.04f));

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), vPos, Vector3{0.025f, 0.025f, 0.028f}, Vector3{0,0,0}, boneCol, 0);

    }

    

    for (int i = 0; i < 6; i++) {

        float t = (float)i / 5.0f;

        Vector3 vPos = Vector3Lerp(dog.chest, dog.neckBase, t);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), vPos, Vector3{0.022f, 0.022f, 0.025f}, Vector3{0,0,0}, boneCol, 0);

    }



    for (int i = 0; i < 5; i++) {

        HoundDrawBoneSegment(cyl, mat, dog.tailNodes[i], dog.tailNodes[i+1], 0.020f - (i*0.003f), 0.020f - (i*0.003f), boneCol, 0);

    }



    Vector3 iliumL = Vector3Add(dog.pelvis, Vector3Add(Vector3Scale(fwd, 0.1f), Vector3Scale(right, -0.08f)));

    Vector3 iliumR = Vector3Add(dog.pelvis, Vector3Add(Vector3Scale(fwd, 0.1f), Vector3Scale(right,  0.08f)));

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, iliumL, 0.02f, 0.02f, boneCol, 0);

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, iliumR, 0.02f, 0.02f, boneCol, 0);

    

    Vector3 ischiumL = Vector3Add(dog.pelvis, Vector3Add(Vector3Scale(fwd, -0.12f), Vector3Scale(right, -0.05f)));

    Vector3 ischiumR = Vector3Add(dog.pelvis, Vector3Add(Vector3Scale(fwd, -0.12f), Vector3Scale(right,  0.05f)));

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, ischiumL, 0.02f, 0.02f, boneCol, 0);

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, ischiumR, 0.02f, 0.02f, boneCol, 0);



    // ==========================================================

    // 2. AXIAL SKELETON: RIBCAGE (13 Ribs)

    // ==========================================================

    for (int r = 0; r < 9; r++) {

        float t = 0.3f + (r / 8.0f) * 0.7f;

        Vector3 rootNode = Vector3Lerp(dog.pelvis, dog.chest, t);

        rootNode = Vector3Add(rootNode, Vector3Scale(up, sinf(t * PI) * 0.04f));

        

        float width = 0.12f - fabsf((float)r - 4.0f) * 0.01f; 

        float drop = 0.22f - fabsf((float)r - 4.0f) * 0.015f;

        float backwardSweep = (float)r * 0.015f;



        for (int side = -1; side <= 1; side += 2) {

            Vector3 ribMid = Vector3Add(rootNode, Vector3Scale(right, side * width));

            ribMid = Vector3Add(ribMid, Vector3Scale(up, -drop * 0.4f));

            ribMid = Vector3Add(ribMid, Vector3Scale(fwd, -backwardSweep * 0.5f));

            

            Vector3 ribBot = Vector3Add(rootNode, Vector3Scale(right, side * width * 0.4f));

            ribBot = Vector3Add(ribBot, Vector3Scale(up, -drop));

            ribBot = Vector3Add(ribBot, Vector3Scale(fwd, -backwardSweep));



            HoundDrawBoneSegment(cyl, mat, rootNode, ribMid, 0.012f, 0.006f, boneCol, 0);

            HoundDrawBoneSegment(cyl, mat, ribMid, ribBot, 0.010f, 0.005f, boneCol, 0);

        }

    }

    Vector3 sternumFront = Vector3Add(dog.chest, Vector3Scale(up, -0.22f));

    Vector3 sternumBack  = Vector3Add(Vector3Lerp(dog.pelvis, dog.chest, 0.3f), Vector3Scale(up, -0.20f));

    HoundDrawBoneSegment(cyl, mat, sternumBack, sternumFront, 0.015f, 0.02f, boneCol, 0);



    // ==========================================================

    // 3. APPENDICULAR SKELETON: THORACIC LIMBS (Front)

    // ==========================================================

    for (int side = 0; side < 2; side++) {

        float off = (side == 0) ? 0.0f : PI;

        float walkS = sinf(dog.walkPhase + off) * 0.4f * dog.walkBlend;

        float runS  = sinf(dog.runPhase + ((side==0)?0.0f:-0.4f)) * 0.8f * dog.runBlend;

        float swing = walkS + runS;

        

        float carpalFlex = fmaxf(0.0f, sinf(dog.walkPhase + off)) * 0.6f * dog.walkBlend + 

                           fmaxf(0.0f, sinf(dog.runPhase + ((side==0)?0.0f:-0.4f))) * 1.0f * dog.runBlend;



        float foldHumerus = dog.sitBlend * -0.2f; 

        float foldRadius  = 0.0f; 



        Vector3 shoulderJoint = dog.shoulder[side]; 

        

        Vector3 scapulaTop = Vector3Add(shoulderJoint, Vector3Add(Vector3Scale(up, 0.15f), Vector3Scale(fwd, -0.08f)));

        HoundDrawBoneSegment(cyl, mat, shoulderJoint, scapulaTop, 0.035f, 0.01f, boneCol, 0);



        Vector3 elbowDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.2f))), right, swing + foldHumerus);

        Vector3 elbow = Vector3Add(shoulderJoint, Vector3Scale(elbowDir, 0.25f));

        

        Vector3 carpalDir = HoundRotateAroundAxis(Vector3Scale(up, -1.0f), right, swing - carpalFlex + foldRadius); 

        Vector3 carpal = Vector3Add(elbow, Vector3Scale(carpalDir, 0.22f)); 

        

        Vector3 pawCenter = Vector3Add(carpal, Vector3{0, -0.05f, 0.05f});

        pawCenter.y = fmaxf(dog.pos.y + 0.01f, pawCenter.y);



        HoundDrawBoneSegment(cyl, mat, shoulderJoint, elbow, 0.02f, 0.02f, boneCol, 0);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), elbow, Vector3{0.025f, 0.025f, 0.025f}, Vector3{0,0,0}, boneCol, 0);

        HoundDrawBoneSegment(cyl, mat, elbow, carpal, 0.015f, 0.015f, boneCol, 0);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), carpal, Vector3{0.02f, 0.02f, 0.02f}, Vector3{0,0,0}, boneCol, 0);

        

        for(int t = -1; t <= 1; t++) {

            Vector3 toe = Vector3Add(pawCenter, Vector3Scale(right, t * 0.02f));

            HoundDrawBoneSegment(cyl, mat, carpal, toe, 0.008f, 0.008f, boneCol, 0);

        }

    }



    // ==========================================================

    // 4. APPENDICULAR SKELETON: PELVIC LIMBS (Hind)

    // ==========================================================

    for (int side = 0; side < 2; side++) {

        float off = (side == 0) ? PI/2.0f : 3.0f*PI/2.0f;

        float walkS = sinf(dog.walkPhase + off) * 0.45f * dog.walkBlend;

        float runS  = sinf(dog.runPhase + PI + ((side==0)?0.0f:-0.4f)) * 0.9f * dog.runBlend;

        float swing = walkS + runS;

        

        float hockFlex = fmaxf(0.0f, -sinf(dog.walkPhase + off)) * 0.8f * dog.walkBlend + 

                         fmaxf(0.0f, -sinf(dog.runPhase + PI + ((side==0)?0.0f:-0.4f))) * 1.2f * dog.runBlend;



        float foldFemur  = dog.sitBlend * 1.2f;

        float foldTibia  = dog.sitBlend * 2.2f;

        float foldCannon = dog.sitBlend * 1.5f;



        Vector3 hipJoint = dog.hip[side];



        Vector3 stifleDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd,  0.4f))), right, swing + foldFemur);

        Vector3 stifle = Vector3Add(hipJoint, Vector3Scale(stifleDir, 0.28f)); 



        Vector3 hockDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.4f))), right, swing + hockFlex - foldTibia);

        Vector3 hock = Vector3Add(stifle, Vector3Scale(hockDir, 0.26f));



        Vector3 metaDir = HoundRotateAroundAxis(Vector3Scale(up, -1.0f), right, swing + hockFlex - foldCannon);

        Vector3 fetlock = Vector3Add(hock, Vector3Scale(metaDir, 0.18f));

        

        Vector3 pawCenter = Vector3Add(fetlock, Vector3{0, -0.04f, 0.06f});

        pawCenter.y = fmaxf(dog.pos.y + 0.01f, pawCenter.y);



        HoundDrawBoneSegment(cyl, mat, hipJoint, stifle, 0.025f, 0.025f, boneCol, 0);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), stifle, Vector3{0.028f, 0.028f, 0.028f}, Vector3{0,0,0}, boneCol, 0);

        HoundDrawBoneSegment(cyl, mat, stifle, hock, 0.02f, 0.02f, boneCol, 0);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), hock, Vector3{0.022f, 0.022f, 0.022f}, Vector3{0,0,0}, boneCol, 0);

        HoundDrawBoneSegment(cyl, mat, hock, fetlock, 0.015f, 0.015f, boneCol, 0);

        

        for(int t = -1; t <= 1; t++) {

            Vector3 toe = Vector3Add(pawCenter, Vector3Scale(right, t * 0.02f));

            HoundDrawBoneSegment(cyl, mat, fetlock, toe, 0.008f, 0.008f, boneCol, 0);

        }

    }



    // ==========================================================

    // 5. AXIAL SKELETON: SKULL & MAXILLA

    // ==========================================================

    Matrix hw = dog.headWorld;



    HoundDrawPartLocal(sphere, mat, hw, Vector3{ 0, 0, 0 }, Vector3{0.07f, 0.07f, 0.08f}, Vector3{0,0,0}, boneCol, 0);

    HoundDrawPartLocal(cube, mat, hw, Vector3{ 0, 0.07f, -0.02f }, Vector3{0.01f, 0.02f, 0.08f}, Vector3{0,0,0}, boneCol, 0);

    

    HoundDrawPartLocal(cyl, mat, hw, Vector3{ -0.06f, -0.02f, 0.05f }, Vector3{0.01f, 0.08f, 0.01f}, Vector3{0,0,-85}, boneCol, 0);

    HoundDrawPartLocal(cyl, mat, hw, Vector3{  0.06f, -0.02f, 0.05f }, Vector3{0.01f, 0.08f, 0.01f}, Vector3{0,0, 85}, boneCol, 0);



    HoundDrawPartLocal(cyl, mat, hw, Vector3{ 0, -0.02f, 0.12f }, Vector3{0.035f, 0.08f, 0.035f}, Vector3{85,0,0}, boneCol, 0);

    

    HoundDrawPartLocal(sphere, mat, hw, Vector3{ 0, -0.01f, 0.17f }, Vector3{0.02f, 0.015f, 0.02f}, Vector3{0,0,0}, voidCol, 1);

    

    HoundDrawPartLocal(sphere, mat, hw, Vector3{ -0.04f, 0.02f, 0.07f }, Vector3{0.025f, 0.025f, 0.02f}, Vector3{0,0,0}, voidCol, 1);

    HoundDrawPartLocal(sphere, mat, hw, Vector3{  0.04f, 0.02f, 0.07f }, Vector3{0.025f, 0.025f, 0.02f}, Vector3{0,0,0}, voidCol, 1);



    HoundDrawPartLocal(sphere, mat, hw, Vector3{ -0.04f, 0.02f, 0.075f }, Vector3{0.010f, 0.010f, 0.010f}, Vector3{0,0,0}, eyeCol, 2);

    HoundDrawPartLocal(sphere, mat, hw, Vector3{  0.04f, 0.02f, 0.075f }, Vector3{0.010f, 0.010f, 0.010f}, Vector3{0,0,0}, eyeCol, 2);



    HoundDrawPartLocal(cube, mat, hw, Vector3{ 0, -0.05f, 0.14f }, Vector3{0.045f, 0.01f, 0.08f}, Vector3{0,0,0}, boneCol, 0);

    for(int t=-2; t<=2; t+=4) {

        HoundDrawPartLocal(cone, mat, hw, Vector3{ t*0.01f, -0.065f, 0.17f }, Vector3{0.008f, 0.02f, 0.008f}, Vector3{0,0,0}, toothCol, 3);

    }



    // ==========================================================

    // 6. AXIAL SKELETON: MANDIBLE

    // ==========================================================

    Matrix jw = dog.jawWorld;

    

    HoundDrawPartLocal(cyl, mat, jw, Vector3{ -0.035f, -0.02f, 0.08f }, Vector3{0.015f, 0.12f, 0.015f}, Vector3{85,-15,0}, boneCol, 0);

    HoundDrawPartLocal(cyl, mat, jw, Vector3{  0.035f, -0.02f, 0.08f }, Vector3{0.015f, 0.12f, 0.015f}, Vector3{85, 15,0}, boneCol, 0);

    

    HoundDrawPartLocal(cube, mat, jw, Vector3{ 0, -0.01f, 0.13f }, Vector3{0.035f, 0.01f, 0.07f}, Vector3{0,0,0}, boneCol, 0);

    for(int t=-2; t<=2; t+=4) {

        HoundDrawPartLocal(cone, mat, jw, Vector3{ t*0.008f, 0.005f, 0.16f }, Vector3{0.006f, 0.015f, 0.006f}, Vector3{180,0,0}, toothCol, 3);

    }

}
