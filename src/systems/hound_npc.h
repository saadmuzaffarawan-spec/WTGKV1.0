#pragma once
#include <raylib.h>

// ==================================================================================================
// HOUND -- Procedural Horror Dog NPC System
// Features full anatomical kinematics, autonomous companion AI, and domain-warped FBM fur shader
// ==================================================================================================

typedef struct HoundSceneLight {
    int enabledLoc, typeLoc, posLoc, targetLoc, colorLoc;
} HoundSceneLight;

typedef enum {
    DOG_STATE_IDLE = 0,
    DOG_STATE_WALK,
    DOG_STATE_RUN,
    DOG_STATE_SIT,
    DOG_STATE_BARK
} DogState;

struct DogNPC {
    Vector3 pos;
    float yaw;
    float animTime;
    
    DogState state;
    float walkPhase, runPhase, barkPhase;
    float walkBlend, runBlend, sitBlend, barkBlend;
    float barkTimer;

    Vector3 pelvis, chest, neckBase, headPivot;
    Vector3 shoulder[2], hip[2];
    Vector3 tailNodes[6];
    
    Matrix headWorld, jawWorld;

    // AI & companion state
    float targetYaw;
    Vector3 homePos;
    float wanderRadius;
    float stateTimer;
    float petTimer;
    float barkCooldown;
    bool isPet;
    float drinkTimer;
};

extern DogNPC g_houndNPC;
extern bool g_houndResourcesLoaded;

extern Shader g_houndShader;
extern int g_houndMatTypeLoc;
extern int g_houndTimeLoc;
extern int g_houndViewPosLoc;
extern int g_houndAmbientLoc;
extern int g_houndFogColorLoc;
extern int g_houndFogDensityLoc;
extern HoundSceneLight g_houndLights[4];

extern Shader g_houndSkeletonShader;
extern int g_houndSkeletonMatTypeLoc;
extern int g_houndSkeletonTimeLoc;
extern int g_houndSkeletonViewPosLoc;
extern int g_houndSkeletonAmbientLoc;
extern int g_houndSkeletonFogColorLoc;
extern int g_houndSkeletonFogDensityLoc;
extern HoundSceneLight g_houndSkeletonLights[4];

extern Mesh g_houndCyl;
extern Mesh g_houndSphere;
extern Mesh g_houndCube;
extern Mesh g_houndCone;
extern Material g_houndMat;
extern Material g_houndSkeletonMat;

void InitDog(DogNPC &dog, Vector3 startPos, float startYaw);
void InitHoundResources(void);
void UnloadHoundResources(void);

void UpdateDog(DogNPC &dog, float dt);
void UpdateDogAI(DogNPC &dog, Vector3 playerPos, float dt, float lightningTimer, bool isNight);

void HoundUpdateLight(int index, int enabled, int type, Vector3 pos, Vector3 target, Color col);
void HoundSkeletonUpdateLight(int index, int enabled, int type, Vector3 pos, Vector3 target, Color col);

void DrawHound(const DogNPC &dog, Mesh cyl, Mesh sphere, Mesh cube, Mesh cone, Material *mat);
void DrawSkeletonHound(const DogNPC &dog, Mesh cyl, Mesh sphere, Mesh cube, Mesh cone, Material *mat);
