#include "skeleton_cow_npc.h"
#include <raymath.h>
#include <rlgl.h>
#include <math.h>
#include <stdlib.h>

static inline float Frand(float minVal, float maxVal) {
    return minVal + ((float)GetRandomValue(0, 10000) / 10000.0f) * (maxVal - minVal);
}

// ==================================================================================================

// SKELETON_COW_ANIMATED -- Procedural Horror Bovine Skeleton NPC System

// Features full anatomical osteology with an advanced kinematic FSM and autonomous AI

// ==================================================================================================


// (BovineState, BovineSkeleton, and MAX_BOVINE_NPCS declared in skeleton_cow_npc.h)

BovineSkeleton g_bovineNPCs[MAX_BOVINE_NPCS];



Mesh g_bovineCyl = { 0 };

Mesh g_bovineSphere = { 0 };

Mesh g_bovineCube = { 0 };

Mesh g_bovineCone = { 0 };

Material g_bovineMat = { 0 };

bool g_bovineMeshesLoaded = false;



static Vector3 BovineRotateAroundAxis(Vector3 v, Vector3 axis, float angleRad) {

    axis = Vector3Normalize(axis);

    float c = cosf(angleRad), s = sinf(angleRad);

    Vector3 t1 = Vector3Scale(v, c);

    Vector3 t2 = Vector3Scale(Vector3CrossProduct(axis, v), s);

    Vector3 t3 = Vector3Scale(axis, Vector3DotProduct(axis, v) * (1.0f - c));

    return Vector3Add(Vector3Add(t1, t2), t3);

}



static void BovineDrawBoneSegment(Mesh mesh, Material *mat, Vector3 from, Vector3 to, float radiusX, float radiusZ, Color tint) {

    Vector3 diff = Vector3Subtract(to, from);

    float len = Vector3Length(diff);

    if (len < 0.0001f) return;

    Vector3 dir = Vector3Scale(diff, 1.0f / len);

    Quaternion q = QuaternionFromVector3ToVector3(Vector3{ 0, 1, 0 }, dir);

    Matrix rot = QuaternionToMatrix(q);

    Matrix scale = MatrixScale(radiusX, len, radiusZ);

    Matrix translate = MatrixTranslate(from.x, from.y, from.z);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, MatrixMultiply(MatrixMultiply(scale, rot), translate));

}



static void BovineDrawForm(Mesh mesh, Material *mat, Vector3 center, Vector3 radii, Vector3 eulerDeg, Color tint) {

    Matrix s = MatrixScale(radii.x, radii.y, radii.z);

    Matrix r = MatrixRotateXYZ(Vector3{ DEG2RAD * eulerDeg.x, DEG2RAD * eulerDeg.y, DEG2RAD * eulerDeg.z });

    Matrix t = MatrixTranslate(center.x, center.y, center.z);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, MatrixMultiply(MatrixMultiply(s, r), t));

}



void InitBovine(BovineSkeleton &cow, Vector3 startPos, float startYaw) {

    cow = (BovineSkeleton){ 0 };

    cow.pos = startPos;

    cow.yaw = startYaw;

    cow.targetYaw = startYaw;

    cow.state = BOVINE_IDLE;

    cow.homePos = startPos;

    cow.wanderRadius = 16.0f;

    cow.stateTimer = Frand(3.0f, 7.0f);

    cow.rainShakeCooldown = Frand(10.0f, 25.0f);

}



void InitBovineMeshes() {

    if (!g_bovineMeshesLoaded) {

        g_bovineCyl = GenMeshCylinder(1.0f, 1.0f, 12);

        g_bovineSphere = GenMeshSphere(1.0f, 10, 10);

        g_bovineCube = GenMeshCube(1.0f, 1.0f, 1.0f);

        g_bovineCone = GenMeshCone(1.0f, 1.0f, 12);

        g_bovineMat = LoadMaterialDefault();

        g_bovineMeshesLoaded = true;

    }

}



void UnloadBovineMeshes() {

    if (g_bovineMeshesLoaded) {

        UnloadMesh(g_bovineCyl);

        UnloadMesh(g_bovineSphere);

        UnloadMesh(g_bovineCube);

        UnloadMesh(g_bovineCone);

        g_bovineMeshesLoaded = false;

    }

}



void InitBovineNPCs() {

    InitBovineMeshes();



    // Cow 0: East roadside verge right across from gas station (grazing, immediately visible from car!)

    InitBovine(g_bovineNPCs[0], Vector3{ 142.0f, 10.0f, 138.0f }, 1.4f);

    g_bovineNPCs[0].homePos = Vector3{ 143.0f, 10.0f, 140.0f };

    g_bovineNPCs[0].wanderRadius = 12.0f;

    g_bovineNPCs[0].state = BOVINE_EAT;

    g_bovineNPCs[0].eatBlend = 1.0f;

    g_bovineNPCs[0].stateTimer = Frand(6.0f, 12.0f);



    // Cow 1: North roadside meadow along highway (standing & chewing cud, easily seen looking forward)

    InitBovine(g_bovineNPCs[1], Vector3{ 144.0f, 10.0f, 162.0f }, -0.7f);

    g_bovineNPCs[1].homePos = Vector3{ 146.0f, 10.0f, 160.0f };

    g_bovineNPCs[1].wanderRadius = 14.0f;

    g_bovineNPCs[1].state = BOVINE_IDLE;

    g_bovineNPCs[1].stateTimer = Frand(4.0f, 8.0f);



    // Cow 2: West roadside meadow across from the shop (resting, lying on side)

    InitBovine(g_bovineNPCs[2], Vector3{ 112.0f, 10.0f, 142.0f }, 2.4f);

    g_bovineNPCs[2].homePos = Vector3{ 112.0f, 10.0f, 142.0f };

    g_bovineNPCs[2].wanderRadius = 12.0f;

    g_bovineNPCs[2].state = BOVINE_LIE;

    g_bovineNPCs[2].sitBlend = 1.0f;

    g_bovineNPCs[2].lieBlend = 1.0f;

    g_bovineNPCs[2].stateTimer = Frand(15.0f, 25.0f);



    // Cow 3: East meadow pasture near highway entrance (walking / grazing)

    InitBovine(g_bovineNPCs[3], Vector3{ 146.0f, 10.0f, 122.0f }, 0.5f);

    g_bovineNPCs[3].homePos = Vector3{ 148.0f, 10.0f, 124.0f };

    g_bovineNPCs[3].wanderRadius = 15.0f;

    g_bovineNPCs[3].state = BOVINE_WALK;

    g_bovineNPCs[3].walkBlend = 1.0f;

    g_bovineNPCs[3].targetYaw = 0.5f;

    g_bovineNPCs[3].stateTimer = Frand(5.0f, 9.0f);

}



void UpdateBovineKinematics(BovineSkeleton &cow, float dt) {

    cow.animTime += dt;



    // FSM State Blending Targets

    float tWalk  = (cow.state == BOVINE_WALK) ? 1.0f : 0.0f;

    float tRun   = (cow.state == BOVINE_RUN) ? 1.0f : 0.0f;

    float tEat   = (cow.state == BOVINE_EAT) ? 1.0f : 0.0f;

    float tSit   = (cow.state == BOVINE_SIT || cow.state == BOVINE_LIE) ? 1.0f : 0.0f; // Lie passes through Sit

    float tLie   = (cow.state == BOVINE_LIE) ? 1.0f : 0.0f;

    float tShake = (cow.state == BOVINE_SHAKE) ? 1.0f : 0.0f;



    // Apply linear lerp interpolation for natural, smooth procedural transitions

    float blendSpd = dt * 4.0f;

    cow.walkBlend  = Lerp(cow.walkBlend,  tWalk,  blendSpd);

    cow.runBlend   = Lerp(cow.runBlend,   tRun,   blendSpd);

    cow.eatBlend   = Lerp(cow.eatBlend,   tEat,   blendSpd);

    cow.sitBlend   = Lerp(cow.sitBlend,   tSit,   blendSpd);

    cow.lieBlend   = Lerp(cow.lieBlend,   tLie,   blendSpd);

    cow.shakeBlend = Lerp(cow.shakeBlend, tShake, blendSpd * 1.5f);



    // Root Motion Update

    float curSpeed = (0.8f * cow.walkBlend) + (2.6f * cow.runBlend);

    if (curSpeed > 0.01f) {

        cow.walkPhase += dt * 4.5f * cow.walkBlend;

        cow.runPhase  += dt * 8.5f * cow.runBlend;

        cow.pos.x += sinf(cow.yaw) * curSpeed * dt;

        cow.pos.z += cosf(cow.yaw) * curSpeed * dt;

    }



    if (cow.eatBlend > 0.01f) cow.eatPhase += dt * 3.0f;

    

    // Violent Shake Oscillation 

    if (cow.shakeBlend > 0.01f) cow.shakePhase += dt * 50.0f; 



    float breathe = sinf(cow.animTime * 1.5f) * 0.015f * (1.0f - cow.lieBlend * 0.5f);

    

    // Base Verticality & Drops

    float basePelvisHeight = 1.45f;

    float locomotionBob = sinf(cow.walkPhase * 2.0f) * 0.04f * cow.walkBlend + 

                          fabsf(sinf(cow.runPhase)) * 0.12f * cow.runBlend;

    

    // Deep sit/lie offset lowers the root body to the floor

    float heightOffset = -(cow.sitBlend * 0.85f) - (cow.lieBlend * 0.10f); 

    

    Vector3 root = { cow.pos.x, cow.pos.y + basePelvisHeight + locomotionBob + heightOffset, cow.pos.z };

    

    // Body Roll applied when lying down (rotates the entire coordinate frame!)

    Vector3 fwd = { sinf(cow.yaw), 0, cosf(cow.yaw) };

    float bodyRoll = cow.lieBlend * 1.35f; // Rolls over ~77 degrees onto the side

    Vector3 right = BovineRotateAroundAxis(Vector3{ cosf(cow.yaw), 0, -sinf(cow.yaw) }, fwd, bodyRoll);

    Vector3 up = BovineRotateAroundAxis(Vector3{ 0, 1, 0 }, fwd, bodyRoll);



    // Procedural Spine Generation (Index 0 = Tail Base -> Index 23 = Skull Joint)

    for (int i = 0; i < 24; i++) {

        float t = (float)i / 23.0f; 

        float localZ = -1.2f + (t * 2.3f);

        float localY = 0.0f;



        // Base Curvature

        if (t < 0.25f) localY = 0.0f; 

        else if (t < 0.65f) localY = -sinf((t - 0.25f) / 0.40f * PI) * 0.12f; 

        else { 

            float nt = (t - 0.65f) / 0.35f;

            localY = -0.35f + cosf(nt * PI) * 0.35f; 

        }



        // Eating Modifier: Limits drop to exactly ground level without burying the skull

        if (t > 0.65f) {

            float neckT = (t - 0.65f) / 0.35f;

            localY -= neckT * 0.70f * cow.eatBlend; 

        }

        

        // Shaking Modifier: Rapid lateral centrifugal twist to cast water off

        float shakeT = sinf(cow.shakePhase - t * 3.0f) * 0.35f * cow.shakeBlend;

        float shakeX = shakeT * sinf(t * PI); 



        localY += breathe * sinf(t * PI);

        

        Vector3 node = Vector3Add(root, Vector3Scale(fwd, localZ));

        node = Vector3Add(node, Vector3Scale(right, shakeX));

        node = Vector3Add(node, Vector3Scale(up, localY));

        cow.spineNodes[i] = node;

    }



    // Tail Kinematics

    for (int i = 0; i < 8; i++) {

        float drop = (float)i * 0.12f;

        float swRate = (cow.shakeBlend > 0.0f) ? 40.0f : 1.5f;

        float sway = sinf(cow.animTime * swRate - drop * 2.0f) * (0.08f + cow.shakeBlend * 0.4f);

        Vector3 node = Vector3Add(cow.spineNodes[0], Vector3Scale(fwd, -drop * 0.3f));

        node = Vector3Add(node, Vector3Scale(up, -drop * 0.8f * (1.0f - cow.lieBlend)));

        node = Vector3Add(node, Vector3Scale(right, sway));

        cow.tailNodes[i] = node;

    }



    cow.shoulderPos[0] = Vector3Add(cow.spineNodes[17], Vector3Scale(right, -0.28f));

    cow.shoulderPos[1] = Vector3Add(cow.spineNodes[17], Vector3Scale(right,  0.28f));

    cow.shoulderPos[0] = Vector3Add(cow.shoulderPos[0], Vector3Scale(up, -0.15f));

    cow.shoulderPos[1] = Vector3Add(cow.shoulderPos[1], Vector3Scale(up, -0.15f));



    cow.hipPos[0] = Vector3Add(cow.spineNodes[2], Vector3Scale(right, -0.26f));

    cow.hipPos[1] = Vector3Add(cow.spineNodes[2], Vector3Scale(right,  0.26f));

}



void UpdateBovineAI(BovineSkeleton &cow, Vector3 playerPos, float dt, bool isRaining) {

    cow.pos.y = 10.0f;



    // Smooth heading rotation towards targetYaw

    float diffYaw = fmodf(cow.targetYaw - cow.yaw + PI, 2.0f * PI);

    if (diffYaw < 0.0f) diffYaw += 2.0f * PI;

    diffYaw -= PI;

    cow.yaw += diffYaw * dt * 2.5f;



    // 1. Player Proximity & Spook Mechanics

    float pDx = cow.pos.x - playerPos.x;

    float pDz = cow.pos.z - playerPos.z;

    float distToPlayer = sqrtf(pDx * pDx + pDz * pDz);



    if (distToPlayer < 3.8f) {

        cow.isSpooked = true;

        cow.spookTimer = 5.0f;

        // Turn directly away from player

        cow.targetYaw = atan2f(pDx, pDz);

        if (distToPlayer < 2.2f) {

            cow.state = BOVINE_RUN;

        } else if (cow.state != BOVINE_RUN) {

            cow.state = BOVINE_WALK;

        }

    }



    if (cow.isSpooked) {

        cow.spookTimer -= dt;

        if (cow.spookTimer <= 0.0f) {

            cow.isSpooked = false;

            cow.state = BOVINE_IDLE;

            cow.stateTimer = Frand(3.0f, 6.0f);

        }

        // Steer away from highway edge (X: 115..141)

        if (cow.homePos.x > 130.0f && cow.pos.x < 144.0f) {

            cow.targetYaw = atan2f(1.0f, 0.0f); // Head East

        } else if (cow.homePos.x < 120.0f && cow.pos.x > 108.0f) {

            cow.targetYaw = atan2f(-1.0f, 0.0f); // Head West

        }

        return;

    }



    // 2. Rain Shake Reaction

    if (isRaining && cow.state != BOVINE_SIT && cow.state != BOVINE_LIE) {

        cow.rainShakeCooldown -= dt;

        if (cow.rainShakeCooldown <= 0.0f) {

            cow.state = BOVINE_SHAKE;

            cow.stateTimer = 1.35f;

            cow.rainShakeCooldown = Frand(15.0f, 30.0f);

        }

    }



    // 3. Autonomous Pasture Routine

    cow.stateTimer -= dt;

    if (cow.stateTimer <= 0.0f) {

        // Prevent wandering too far from home pasture

        float hDx = cow.pos.x - cow.homePos.x;

        float hDz = cow.pos.z - cow.homePos.z;

        float distFromHome = sqrtf(hDx * hDx + hDz * hDz);



        int roll = GetRandomValue(0, 100);

        if (distFromHome > cow.wanderRadius) {

            // Wander back home

            cow.state = BOVINE_WALK;

            cow.targetYaw = atan2f(-hDx, -hDz);

            cow.stateTimer = Frand(5.0f, 9.0f);

        } else if (roll < 42) {

            // Grazing grass

            cow.state = BOVINE_EAT;

            cow.stateTimer = Frand(8.0f, 16.0f);

        } else if (roll < 68) {

            // Idle chewing cud & looking around

            cow.state = BOVINE_IDLE;

            cow.stateTimer = Frand(4.0f, 8.0f);

        } else if (roll < 88) {

            // Gentle wander to new pasture patch

            cow.state = BOVINE_WALK;

            cow.targetYaw = Frand(-PI, PI);

            cow.stateTimer = Frand(5.0f, 10.0f);

        } else {

            // Resting on the ground (sitting or lying down)

            cow.state = (GetRandomValue(0, 1) == 0) ? BOVINE_SIT : BOVINE_LIE;

            cow.stateTimer = Frand(12.0f, 22.0f);

        }

    }

}



void DrawBovineSkeleton(const BovineSkeleton &cow, Mesh cyl, Mesh sphere, Mesh cube, Mesh cone,

                               Material *mat, Color boneCol, Color hornCol, Color voidCol)

{

    Vector3 fwd = { sinf(cow.yaw), 0, cosf(cow.yaw) };

    float bodyRoll = cow.lieBlend * 1.35f;

    Vector3 right = BovineRotateAroundAxis(Vector3{ cosf(cow.yaw), 0, -sinf(cow.yaw) }, fwd, bodyRoll);

    Vector3 up = BovineRotateAroundAxis(Vector3{ 0, 1, 0 }, fwd, bodyRoll);



    // 1. SPINAL COLUMN

    for (int i = 0; i < 23; i++) {

        BovineDrawBoneSegment(cyl, mat, cow.spineNodes[i], cow.spineNodes[i+1], 0.035f, 0.040f, boneCol);

        if (i >= 8 && i <= 17) {

            float humpHeight = sinf(((float)(i - 8) / 9.0f) * PI) * 0.35f;

            Vector3 processTop = Vector3Add(cow.spineNodes[i], Vector3Scale(up, humpHeight));

            BovineDrawBoneSegment(cyl, mat, cow.spineNodes[i], processTop, 0.015f, 0.025f, boneCol);

        } else if (i >= 2 && i < 8) {

            Vector3 procL = Vector3Add(cow.spineNodes[i], Vector3Scale(right, -0.15f));

            Vector3 procR = Vector3Add(cow.spineNodes[i], Vector3Scale(right,  0.15f));

            BovineDrawBoneSegment(cyl, mat, cow.spineNodes[i], procL, 0.015f, 0.01f, boneCol);

            BovineDrawBoneSegment(cyl, mat, cow.spineNodes[i], procR, 0.015f, 0.01f, boneCol);

        }

    }

    for (int i = 0; i < 7; i++) {

        BovineDrawBoneSegment(cyl, mat, cow.tailNodes[i], cow.tailNodes[i+1], 0.025f - (i*0.003f), 0.025f - (i*0.003f), boneCol);

    }



    // 2. RIBCAGE & STERNUM

    for (int r = 0; r < 13; r++) {

        int spineIdx = 5 + r; 

        Vector3 rootNode = cow.spineNodes[spineIdx];

        float width = 0.42f - fabsf((float)r - 6.0f) * 0.018f; 

        float drop = 0.75f - fabsf((float)r - 6.0f) * 0.015f;

        float backwardSweep = (float)r * 0.025f;



        for (int side = -1; side <= 1; side += 2) {

            Vector3 ribMid = Vector3Add(rootNode, Vector3Scale(right, side * width));

            ribMid = Vector3Add(ribMid, Vector3Scale(up, -drop * 0.3f));

            ribMid = Vector3Add(ribMid, Vector3Scale(fwd, -backwardSweep * 0.5f));

            

            Vector3 ribBot = Vector3Add(rootNode, Vector3Scale(right, side * width * 0.3f));

            ribBot = Vector3Add(ribBot, Vector3Scale(up, -drop));

            ribBot = Vector3Add(ribBot, Vector3Scale(fwd, -backwardSweep));



            BovineDrawBoneSegment(cyl, mat, rootNode, ribMid, 0.030f, 0.010f, boneCol);

            BovineDrawBoneSegment(cyl, mat, ribMid, ribBot, 0.025f, 0.008f, boneCol);

        }

    }

    Vector3 sternumFront = Vector3Add(cow.spineNodes[17], Vector3Scale(up, -0.75f));

    Vector3 sternumBack  = Vector3Add(cow.spineNodes[5],  Vector3Scale(up, -0.75f));

    BovineDrawBoneSegment(cyl, mat, sternumBack, sternumFront, 0.04f, 0.08f, boneCol);



    // 3. PELVIS

    Vector3 sacrum = cow.spineNodes[2];

    BovineDrawBoneSegment(cyl, mat, sacrum, cow.hipPos[0], 0.05f, 0.02f, boneCol);

    BovineDrawBoneSegment(cyl, mat, sacrum, cow.hipPos[1], 0.05f, 0.02f, boneCol);

    Vector3 pinL = Vector3Add(sacrum, Vector3Add(Vector3Scale(fwd, -0.35f), Vector3Scale(right, -0.1f)));

    Vector3 pinR = Vector3Add(sacrum, Vector3Add(Vector3Scale(fwd, -0.35f), Vector3Scale(right,  0.1f)));

    BovineDrawBoneSegment(cyl, mat, cow.hipPos[0], pinL, 0.03f, 0.03f, boneCol);

    BovineDrawBoneSegment(cyl, mat, cow.hipPos[1], pinR, 0.03f, 0.03f, boneCol);

    BovineDrawBoneSegment(cyl, mat, sacrum, pinL, 0.04f, 0.02f, boneCol);

    BovineDrawBoneSegment(cyl, mat, sacrum, pinR, 0.04f, 0.02f, boneCol);



    // 4. SKULL & MANDIBLE

    Vector3 atlas = cow.spineNodes[23];

    Vector3 headDir = Vector3Normalize(Vector3Add(Vector3Scale(fwd, 1.0f), Vector3Scale(up, 0.3f - cow.eatBlend * 1.5f)));

    Vector3 snoutEnd = Vector3Add(atlas, Vector3Scale(headDir, 0.55f));

    

    BovineDrawForm(sphere, mat, Vector3Add(atlas, Vector3Scale(headDir, 0.15f)), Vector3{0.18f, 0.12f, 0.22f}, Vector3{15, cow.yaw*RAD2DEG, 0}, boneCol);

    BovineDrawBoneSegment(cyl, mat, Vector3Add(atlas, Vector3Scale(headDir, 0.15f)), snoutEnd, 0.08f, 0.10f, boneCol);

    BovineDrawForm(sphere, mat, snoutEnd, Vector3{0.12f, 0.06f, 0.08f}, Vector3{15, cow.yaw*RAD2DEG, 0}, boneCol);



    float jawDrop = cow.eatBlend * fabsf(sinf(cow.eatPhase)) * 0.10f;

    Vector3 jawPivot = Vector3Add(atlas, Vector3Scale(up, -0.12f));

    Vector3 jawTip = Vector3Add(snoutEnd, Vector3Add(Vector3Scale(up, -0.06f - jawDrop), Vector3Scale(fwd, -0.05f)));

    BovineDrawBoneSegment(cyl, mat, jawPivot, jawTip, 0.08f, 0.06f, boneCol);



    Vector3 eyeL = Vector3Add(atlas, Vector3Add(Vector3Scale(headDir, 0.25f), Vector3Scale(right, -0.16f)));

    Vector3 eyeR = Vector3Add(atlas, Vector3Add(Vector3Scale(headDir, 0.25f), Vector3Scale(right,  0.16f)));

    BovineDrawForm(sphere, mat, eyeL, Vector3{0.05f, 0.06f, 0.05f}, Vector3{0,0,0}, voidCol);

    BovineDrawForm(sphere, mat, eyeR, Vector3{0.05f, 0.06f, 0.05f}, Vector3{0,0,0}, voidCol);



    Vector3 hornL = Vector3Add(atlas, Vector3Add(Vector3Scale(fwd, -0.05f), Vector3Scale(right, -0.15f))); hornL = Vector3Add(hornL, Vector3Scale(up, 0.12f));

    Vector3 hornR = Vector3Add(atlas, Vector3Add(Vector3Scale(fwd, -0.05f), Vector3Scale(right,  0.15f))); hornR = Vector3Add(hornR, Vector3Scale(up, 0.12f));

    Vector3 hornTipL = Vector3Add(hornL, Vector3Add(Vector3Scale(fwd, 0.15f), Vector3Add(Vector3Scale(right, -0.25f), Vector3Scale(up, 0.35f))));

    Vector3 hornTipR = Vector3Add(hornR, Vector3Add(Vector3Scale(fwd, 0.15f), Vector3Add(Vector3Scale(right,  0.25f), Vector3Scale(up, 0.35f))));

    BovineDrawBoneSegment(cone, mat, hornL, hornTipL, 0.045f, 0.045f, hornCol);

    BovineDrawBoneSegment(cone, mat, hornR, hornTipR, 0.045f, 0.045f, hornCol);



    // 5. FORELIMBS (Anatomical Sit & Lie Folding)

    for (int side = -1; side <= 1; side += 2) {

        float off = (side == -1) ? 0.0f : PI;

        float swing = sinf(cow.walkPhase + off) * 0.35f * cow.walkBlend + sinf(cow.runPhase + off) * 0.70f * cow.runBlend;

        float carpalFlex = fmaxf(0.0f, sinf(cow.walkPhase + off)) * 0.5f * cow.walkBlend + fmaxf(0.0f, sinf(cow.runPhase + off)) * 0.8f * cow.runBlend;



        // Front Limbs Absolute Sit Folding Angles (Tuck backwards parallel to ground)

        float foldFrontHumerus = cow.sitBlend * -0.6f + cow.lieBlend * -0.6f;

        float foldFrontRadius  = cow.sitBlend * -1.0f + cow.lieBlend * -1.0f;

        float foldFrontCannon  = cow.sitBlend *  1.5f + cow.lieBlend *  1.5f; // Flat forward on floor



        Vector3 shoulderJoint = cow.shoulderPos[(side==-1)?0:1]; 

        Vector3 scapulaTop = Vector3Add(shoulderJoint, Vector3Add(Vector3Scale(up, 0.45f), Vector3Scale(fwd, -0.2f)));

        BovineDrawBoneSegment(cyl, mat, shoulderJoint, scapulaTop, 0.12f, 0.02f, boneCol);



        Vector3 elbowDir   = BovineRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.4f))), right, swing + foldFrontHumerus);

        Vector3 carpalDir  = BovineRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd,  0.2f))), right, swing - carpalFlex + foldFrontRadius); 

        Vector3 fetlockDir = BovineRotateAroundAxis(Vector3{0, -1, 0}, right, swing - carpalFlex + foldFrontCannon);



        Vector3 elbow   = Vector3Add(shoulderJoint, Vector3Scale(elbowDir, 0.35f));

        Vector3 carpal  = Vector3Add(elbow, Vector3Scale(carpalDir, 0.35f)); 

        Vector3 fetlock = Vector3Add(carpal, Vector3Scale(fetlockDir, 0.25f));

        

        Vector3 hoof = Vector3Add(fetlock, Vector3Add(Vector3Scale(up, -0.1f), Vector3Scale(fwd, 0.05f)));

        hoof.y = fmaxf(cow.pos.y + 0.02f, hoof.y);



        BovineDrawBoneSegment(cyl, mat, shoulderJoint, elbow, 0.05f, 0.05f, boneCol);

        BovineDrawForm(sphere, mat, elbow, Vector3{0.055f, 0.055f, 0.055f}, Vector3{0,0,0}, boneCol);

        BovineDrawBoneSegment(cyl, mat, elbow, carpal, 0.04f, 0.035f, boneCol);

        BovineDrawForm(sphere, mat, carpal, Vector3{0.045f, 0.045f, 0.045f}, Vector3{0,0,0}, boneCol);

        BovineDrawBoneSegment(cyl, mat, carpal, fetlock, 0.03f, 0.025f, boneCol);

        BovineDrawForm(cube, mat, Vector3Add(hoof, Vector3Scale(right, -0.02f)), Vector3{0.035f, 0.06f, 0.08f}, Vector3{0, cow.yaw*RAD2DEG, 0}, hornCol);

        BovineDrawForm(cube, mat, Vector3Add(hoof, Vector3Scale(right,  0.02f)), Vector3{0.035f, 0.06f, 0.08f}, Vector3{0, cow.yaw*RAD2DEG, 0}, hornCol);

    }



    // 6. HINDLIMBS

    for (int side = -1; side <= 1; side += 2) {

        float off = (side == -1) ? PI : 0.0f; 

        float swing = sinf(cow.walkPhase + off) * 0.40f * cow.walkBlend + sinf(cow.runPhase + off) * 0.80f * cow.runBlend;

        float hockFlex = fmaxf(0.0f, -sinf(cow.walkPhase + off)) * 0.6f * cow.walkBlend + fmaxf(0.0f, -sinf(cow.runPhase + off)) * 1.0f * cow.runBlend;



        // Hind Limbs Absolute Sit Folding Angles (Tuck forward parallel to ground)

        float foldHindFemur  = cow.sitBlend * 0.6f + cow.lieBlend * 0.6f;

        float foldHindTibia  = cow.sitBlend * 1.0f + cow.lieBlend * 1.0f;

        float foldHindCannon = cow.sitBlend * 1.5f + cow.lieBlend * 1.5f;



        Vector3 hipJoint = cow.hipPos[(side==-1)?0:1];



        Vector3 stifleDir  = BovineRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd,  0.4f))), right, swing + foldHindFemur);

        Vector3 hockDir    = BovineRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.3f))), right, swing + hockFlex + foldHindTibia);

        Vector3 fetlockDir = BovineRotateAroundAxis(Vector3{0, -1, 0}, right, swing + hockFlex + foldHindCannon);



        Vector3 stifle  = Vector3Add(hipJoint, Vector3Scale(stifleDir, 0.4f)); 

        Vector3 hock    = Vector3Add(stifle, Vector3Scale(hockDir, 0.45f));

        Vector3 fetlock = Vector3Add(hock, Vector3Scale(fetlockDir, 0.30f));

        

        Vector3 hoof = Vector3Add(fetlock, Vector3Add(Vector3Scale(up, -0.1f), Vector3Scale(fwd, 0.05f)));

        hoof.y = fmaxf(cow.pos.y + 0.02f, hoof.y);



        BovineDrawBoneSegment(cyl, mat, hipJoint, stifle, 0.06f, 0.06f, boneCol);

        BovineDrawForm(sphere, mat, stifle, Vector3{0.055f, 0.055f, 0.055f}, Vector3{0,0,0}, boneCol);

        BovineDrawBoneSegment(cyl, mat, stifle, hock, 0.045f, 0.045f, boneCol);

        BovineDrawBoneSegment(cyl, mat, hock, Vector3Add(hock, Vector3Add(Vector3Scale(up, 0.1f), Vector3Scale(fwd, -0.1f))), 0.02f, 0.02f, boneCol);

        BovineDrawForm(sphere, mat, hock, Vector3{0.045f, 0.055f, 0.045f}, Vector3{0,0,0}, boneCol);

        BovineDrawBoneSegment(cyl, mat, hock, fetlock, 0.03f, 0.025f, boneCol);

        

        BovineDrawForm(cube, mat, Vector3Add(hoof, Vector3Scale(right, -0.02f)), Vector3{0.035f, 0.06f, 0.08f}, Vector3{0, cow.yaw*RAD2DEG, 0}, hornCol);

        BovineDrawForm(cube, mat, Vector3Add(hoof, Vector3Scale(right,  0.02f)), Vector3{0.035f, 0.06f, 0.08f}, Vector3{0, cow.yaw*RAD2DEG, 0}, hornCol);

    }



    // 7. VIOLENT SHAKE WATER DROPLETS (Flinging physics)

    if (cow.shakeBlend > 0.25f) {

        for (int d = 0; d < 6; d++) {

            float fPhase = cow.shakePhase * 1.4f + (float)d * 1.15f;

            float fRadius = 0.45f + fmodf(fPhase * 0.7f, 1.5f);

            float fAngle = (float)d * (2.0f * PI / 6.0f) + sinf(fPhase) * 0.6f;

            float fDir = (sinf(cow.shakePhase) > 0.0f) ? 1.0f : -1.0f;

            Vector3 dropPos = {

                cow.pos.x + cosf(fAngle) * fRadius * fDir,

                cow.pos.y + 0.9f + sinf(fPhase * 2.0f) * 0.35f,

                cow.pos.z + sinf(fAngle) * fRadius

            };

            DrawSphere(dropPos, 0.022f, Color{ 175, 220, 255, (unsigned char)(210 * cow.shakeBlend) });

        }

    }

}
