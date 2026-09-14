#pragma once
#include <raylib.h>

// ==================================================================================================
// SKELETON_COW_ANIMATED -- Procedural Horror Bovine Skeleton NPC System
// Features 7-state procedural kinematic locomotion FSM, osteological ribcage/skull anatomy,
// dynamic grazing/wandering AI, and supernatural fluid motes
// ==================================================================================================

typedef enum {
    BOVINE_IDLE = 0,
    BOVINE_WALK,
    BOVINE_RUN,
    BOVINE_EAT,
    BOVINE_SIT,
    BOVINE_LIE,
    BOVINE_SHAKE
} BovineState;

struct BovineSkeleton {
    Vector3 pos;
    float yaw;
    float animTime;
    
    BovineState state;
    float walkPhase, runPhase, eatPhase, shakePhase;
    float walkBlend, runBlend, eatBlend, sitBlend, lieBlend, shakeBlend;

    Vector3 spineNodes[24]; 
    Vector3 tailNodes[8];
    Vector3 shoulderPos[2];
    Vector3 hipPos[2];

    // Autonomous AI state
    float stateTimer;
    float targetYaw;
    Vector3 homePos;
    float wanderRadius;
    bool isSpooked;
    float spookTimer;
    float rainShakeCooldown;
};

#define MAX_BOVINE_NPCS 4

extern BovineSkeleton g_bovineNPCs[MAX_BOVINE_NPCS];

extern Mesh g_bovineCyl;
extern Mesh g_bovineSphere;
extern Mesh g_bovineCube;
extern Mesh g_bovineCone;
extern Material g_bovineMat;
extern bool g_bovineMeshesLoaded;

void InitBovine(BovineSkeleton &cow, Vector3 startPos, float startYaw);
void InitBovineMeshes(void);
void UnloadBovineMeshes(void);
void InitBovineNPCs(void);

void UpdateBovineKinematics(BovineSkeleton &cow, float dt);
void UpdateBovineAI(BovineSkeleton &cow, Vector3 playerPos, float dt, bool isRaining = false);

void DrawBovineSkeleton(const BovineSkeleton &cow, Mesh cyl, Mesh sphere, Mesh cube, Mesh cone, Material *mat, Color boneCol, Color hornCol, Color voidCol);
