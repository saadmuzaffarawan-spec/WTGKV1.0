#pragma once

#include <raylib.h>
#include <raymath.h>

// ==================================================================
// HYPER-REALISTIC SHOVEL SYSTEM (Custom Curved Spade Mesh, Lit Shader, Rig & Physics)
// ==================================================================

struct ShovelPart {
    Model model;
    Matrix localOffset;
};

struct Shovel {
    ShovelPart pommel;
    ShovelPart grip;
    ShovelPart shaft;
    ShovelPart collar;
    ShovelPart tabL;
    ShovelPart tabR;
    ShovelPart blade;
    Shader shader;
    int locLightDir, locViewPos, locBaseColor, locRough, locGrain, locMetal;
};

struct ShovelPose {
    Vector3 pos;
    Vector3 rot;
};

struct ShovelKeyframe {
    float t;
    ShovelPose pose;
};

enum ShovelAnimState {
    SHOVEL_ANIM_IDLE,
    SHOVEL_ANIM_DIG,
    SHOVEL_ANIM_ATTACK
};

struct DirtClod {
    Vector3 pos, vel;
    float life;
    bool active;
};

extern float g_shovelTipLocalZ;
extern const ShovelPose SHOVEL_HOLD_POSE;
extern const float SHOVEL_DIG_DURATION;
extern const float SHOVEL_ATTACK_DURATION;
extern const float SHOVEL_DIG_IMPACT_T;
extern const float SHOVEL_DIG_THROW_T;
extern const float SHOVEL_ATTACK_IMPACT_T;

Shovel BuildShovel();
void UnloadShovel(Shovel &sv);
void DrawShovel(Shovel &sv, Matrix rootTransform, Vector3 viewPos, Vector3 lightDir);

ShovelPose LerpShovelPose(ShovelPose a, ShovelPose b, float f);
ShovelPose SampleShovelKeyframes(const ShovelKeyframe *kf, int count, float t);
ShovelPose GetAnimatedShovelPose(ShovelAnimState state, float stateTime, float idleClock);
Matrix ShovelPoseToWorldMatrix(ShovelPose pose, Vector3 basePos, Vector3 right, Vector3 up, Vector3 fwd);

void SpawnDirtClod(Vector3 origin, Vector3 dir);
float GetTerrainGroundHeight(float x, float z);
void UpdateDirtClods(float dt);
void DrawDirtClods();

