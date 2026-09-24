#pragma once

#include <raylib.h>
#include <vector>
#include "core/game_types.h"
#include "core/game_context.h"
#include "systems/voxel_mesh_system.inl"

enum IntroPhase {
    INTRO_AERIAL = 0,       // Bird's eye — car rolling on road
    INTRO_DRIVER_POV,       // Driver's FP — road ahead, rain
    INTRO_JERK_1,           // First engine jolt
    INTRO_JERK_2,           // Second jolt (stronger)
    INTRO_JERK_3_STOP,      // Third jolt — car stops dead
    INTRO_DOOR_OPEN,        // Driver opens door (FP→3P transition)
    INTRO_EXIT_CAR,         // Driver walks to car front (3P)
    INTRO_SISTER_EXIT,      // Sister exits (3P)
    INTRO_LOOK_AROUND,      // FP look left→right vignette
    INTRO_FADE_TO_GAME,     // Fade to black → game begins
    INTRO_DONE
};

struct WDrop {
    Vector3 worldPos;
    float life;
    float slideSpd;
};

class IntroCinematic {
public:
    IntroCinematic();
    ~IntroCinematic();

    bool Run(); // Returns true when finished
    void Update(float dt);
    void Draw(RenderTexture2D target, Shader instancedShader, Mesh quad, Material material, Chunk* chunk);

private:
    // Phase state
    IntroPhase phase = INTRO_AERIAL;
    float timer = 0.0f;

    // Car state
    float carZ = 20.0f;
    float carX = 128.0f;
    float carY = 10.0f;
    float carSpd = 12.0f;
    float wheelAng = 0.0f;
    float wiperT = 0.0f;

    // Camera/Shake state
    float shakeX = 0.0f;
    float shakeY = 0.0f;
    float shakeDec = 0.0f;
    float fovExtra = 0.0f;
    float jerkPushZ = 0.0f;
    int jerkCount = 0;
    float nextJerkT = 3.2f;

    // Animation state
    float doorAng = 0.0f;
    float playerT = 0.0f;
    float sisterT = 0.0f;
    bool sisterOut = false;
    float lookYaw = 0.0f;
    int lookStep = 0;
    float eyeT = 0.0f;
    float fadeAlpha = 0.0f;
    float carSpdLerp = 12.0f;

    // Resources
    Camera3D cam;
    Vector3 camPosTarget;
    Vector3 camTgtTarget;
    float camFovTarget = 55.0f;
    float camLerpSpd = 8.0f;

    Mesh mBody, mCabin, mTrunk, mHood, mGrille, mWheel, mDoor, mDash, mSeat, mSeatBack, mMirror, mBumper, mFender, mSteerRim, mRVMFrame, mHeadlight, mIndicator;
    Material matBody, matCabin, matHood, matDoor, matTrunk, matGlass, matWheel, matHub, matInterior, matSeat, matChrome, matGrille, matHeadlit, matBezel, matAmber, matMirrorF, matBrake;
    RenderTexture2D mirrorRT;
    Shader glassShader;
    int glassTimeLoc;

    std::vector<WDrop> wDrops;

    // Helpers
    float GetCarCY() const { return carY + 0.98f; }
    Vector3 GetDriverEyePos() const;
    Vector3 GetSisterEyePos() const;
    Vector3 GetDriverStandPos(float t) const;
    Vector3 GetSisterStandPos(float t) const;
    void ApplyShake(Vector3& pos, Vector3& tgt);
};
