#include "systems/intro_cinematic.h"
#include "raymath.h"
#include <cmath>
#include <algorithm>
#include "systems/car_cockpit_cinematic.inl"
#include "systems/plasma_lightning.inl"

// Internal structure for the intro's rain effect
struct RainParticle {
    Vector3 pos;
    float life;
    char glyph;
};

static std::vector<RainParticle> g_introRain;
static std::vector<Matrix> g_introRainInstances[256];

IntroCinematic::IntroCinematic() {
    // --- SHADER ---
    const char* glassShaderCode = R"(
    #version 330
    in vec2 fragTexCoord;
    in vec4 fragColor;
    out vec4 finalColor;
    uniform sampler2D texture0;
    uniform float iTime;
    void main() {
        vec2 uv = fragTexCoord;
        vec2 cc = uv - vec2(0.5);
        float dist = dot(cc, cc);
        vec2 dUV = uv + cc * (dist * 0.12);
        vec4 base = texture(texture0, dUV);
        float shadows = sin(dUV.x * 20.0 + iTime * 2.0) * sin(dUV.y * 15.0 - iTime * 1.5);
        shadows = smoothstep(0.4, 1.0, shadows);
        base.rgb -= vec3(0.05, 0.08, 0.05) * shadows * 0.4;
        base.rgb *= (1.0 - dist * 0.3);
        finalColor = base;
    }
    )";
    glassShader = LoadShaderFromMemory(nullptr, glassShaderCode);
    glassTimeLoc = GetShaderLocation(glassShader, "iTime");

    // --- CAMERA ---
    cam.up = { 0.0f, 1.0f, 0.0f };
    cam.fovy = 55.0f;
    cam.projection = CAMERA_PERSPECTIVE;
    cam.position = { carX, carY + 16.0f, carZ - 14.0f };
    cam.target = { carX, carY + 1.0f, carZ + 2.0f };

    // --- GEOMETRY ---
    mBody = GenMeshCube(2.4f, 1.04f, 5.2f);
    mCabin = GenMeshCube(2.0f, 0.72f, 2.8f);
    mTrunk = GenMeshCube(2.1f, 0.36f, 1.4f);
    mHood = GenMeshCube(2.1f, 0.18f, 1.6f);
    mGrille = GenMeshCube(2.2f, 0.62f, 0.18f);
    mWheel = GenMeshCylinder(0.38f, 0.28f, 18);
    mDoor = GenMeshCube(0.12f, 0.90f, 2.0f);
    mDash = GenMeshCube(2.1f, 0.30f, 0.65f);
    mSeat = GenMeshCube(0.62f, 0.75f, 0.68f);
    mSeatBack = GenMeshCube(0.62f, 0.75f, 0.14f);
    mMirror = GenMeshCube(0.10f, 0.10f, 0.22f);
    mBumper = GenMeshCube(2.5f, 0.22f, 0.22f);
    mFender = GenMeshCube(0.30f, 0.18f, 0.90f);
    mSteerRim = GenMeshCylinder(0.26f, 0.03f, 20);
    mRVMFrame = GenMeshCube(0.60f, 0.08f, 0.04f);
    mHeadlight = GenMeshCube(0.34f, 0.22f, 0.12f);
    mIndicator = GenMeshCube(0.18f, 0.18f, 0.12f);

    // --- MATERIALS ---
    auto makeMat = [](Color c) {
        Material m = LoadMaterialDefault();
        m.maps[MATERIAL_MAP_DIFFUSE].color = c;
        return m;
    };

    matBody = makeMat({138, 144, 154, 255});
    matCabin = makeMat({118, 124, 134, 255});
    matHood = makeMat({150, 156, 166, 255});
    matDoor = makeMat({132, 138, 148, 255});
    matTrunk = makeMat({126, 132, 142, 255});
    matGlass = makeMat({60, 85, 115, 175});
    matWheel = makeMat({20, 20, 22, 255});
    matHub = makeMat({185, 192, 204, 255});
    matInterior = makeMat({28, 30, 34, 255});
    matSeat = makeMat({46, 44, 42, 255});
    matChrome = makeMat({225, 230, 240, 255});
    matGrille = makeMat({22, 24, 28, 255});
    matHeadlit = makeMat({255, 252, 220, 255});
    matBezel = makeMat({160, 168, 178, 255});
    matAmber = makeMat({235, 150, 30, 255});
    matMirrorF = makeMat({45, 48, 55, 255});
    matBrake = makeMat({205, 35, 35, 255});

    mirrorRT = LoadRenderTexture(512, 192);
    SetTextureFilter(mirrorRT.texture, TEXTURE_FILTER_BILINEAR);
}

IntroCinematic::~IntroCinematic() {
    UnloadShader(glassShader);
    UnloadRenderTexture(mirrorRT);
}

Vector3 IntroCinematic::GetDriverEyePos() const {
    return { carX - 0.50f, GetCarCY() + 0.35f, carZ + 0.15f };
}

Vector3 IntroCinematic::GetSisterEyePos() const {
    return { carX + 0.50f, GetCarCY() + 0.35f, carZ + 0.15f };
}

Vector3 IntroCinematic::GetDriverStandPos(float t) const {
    Vector3 inSeat = { carX - 0.50f, carY + 1.45f, carZ + 0.20f };
    Vector3 outside = { carX - 1.65f, carY + 1.65f, carZ + 0.40f };
    Vector3 atFront = { carX - 0.75f, carY + 1.65f, carZ + 3.80f };
    if (t < 0.35f) {
        float subT = t / 0.35f;
        return Vector3Lerp(inSeat, outside, subT);
    } else {
        float subT = (t - 0.35f) / 0.65f;
        return Vector3Lerp(outside, atFront, subT);
    }
}

Vector3 IntroCinematic::GetSisterStandPos(float t) const {
    Vector3 inSeat = { carX + 0.50f, carY + 1.45f, carZ + 0.20f };
    Vector3 outside = { carX + 1.65f, carY + 1.65f, carZ + 0.40f };
    Vector3 atFront = { carX + 0.75f, carY + 1.65f, carZ + 3.80f };
    if (t < 0.35f) {
        float subT = t / 0.35f;
        return Vector3Lerp(inSeat, outside, subT);
    } else {
        float subT = (t - 0.35f) / 0.65f;
        return Vector3Lerp(outside, atFront, subT);
    }
}

void IntroCinematic::ApplyShake(Vector3& pos, Vector3& tgt) {
    if (shakeDec > 0.001f) {
        float sx = ((GetRandomValue(-100, 100) / 100.0f)) * shakeDec * 0.12f;
        float sy = ((GetRandomValue(-100, 100) / 100.0f)) * shakeDec * 0.08f;
        float sz = ((GetRandomValue(-100, 100) / 100.0f)) * shakeDec * 0.05f;
        pos.x += sx; pos.y += sy; pos.z += sz;
        tgt.x += sx * 0.4f; tgt.y += sy * 0.4f;
    }
}

bool IntroCinematic::Run() {
    while (!WindowShouldClose() && phase != INTRO_DONE) {
        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            phase = INTRO_DONE;
            break;
        }
        if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {
            ToggleGameFullscreen();
        }
        Update(GetFrameTime());
        // Draw is called from the external game loop
    }
    return (phase == INTRO_DONE);
}

void IntroCinematic::Update(float dt) {
    float timeVal = (float)GetTime();
    timer += dt;
    wheelAng -= (carSpd / 0.38f) * dt;
    wiperT = sinf(timeVal * 4.2f);
    shakeDec = Lerp(shakeDec, 0.0f, dt * 5.5f);
    fovExtra = Lerp(fovExtra, 0.0f, dt * 6.0f);
    jerkPushZ = Lerp(jerkPushZ, 0.0f, dt * 7.0f);
    carSpdLerp = Lerp(carSpdLerp, carSpd, dt * 3.0f);

    switch (phase) {
        case INTRO_AERIAL:
            carZ += carSpd * dt;
            camPosTarget = { carX, carY + 17.0f, carZ - 15.0f };
            camTgtTarget = { carX, carY + 1.2f, carZ + 2.0f };
            camFovTarget = 50.0f;
            camLerpSpd = 3.0f;
            if (timer >= 4.0f) {
                phase = INTRO_DRIVER_POV;
                timer = 0.0f;
                nextJerkT = 3.0f;
            }
            break;
        case INTRO_DRIVER_POV: {
            carZ += carSpd * dt;
            Vector3 eye = GetDriverEyePos();
            camPosTarget = { eye.x + shakeX, eye.y + shakeY, eye.z + jerkPushZ };
            camTgtTarget = { eye.x + shakeX * 0.3f, eye.y + shakeY * 0.3f - 0.06f, carZ + 8.0f };
            camFovTarget = 68.0f + fovExtra;
            camLerpSpd = 20.0f;
            nextJerkT -= dt;
            if (nextJerkT <= 0.0f) {
                jerkCount = 1;
                shakeDec = 3.8f;
                fovExtra = 22.0f;
                jerkPushZ = 0.35f;
                carSpd = 4.0f;
                PlaySound(g_sndThunder);
                phase = INTRO_JERK_1;
                timer = 0.0f;
            }
            break;
        }
        case INTRO_JERK_1:
            carZ += carSpd * dt;
            carSpd = Lerp(carSpd, 10.0f, dt * 1.5f);
            {
                Vector3 eye = GetDriverEyePos();
                camPosTarget = { eye.x + shakeX, eye.y + shakeY, eye.z + jerkPushZ };
                camTgtTarget = { eye.x + shakeX * 0.3f, eye.y - 0.06f, carZ + 8.0f };
                camFovTarget = 68.0f + fovExtra;
                camLerpSpd = 20.0f;
            }
            if (timer >= 2.8f) {
                jerkCount = 2;
                shakeDec = 5.5f;
                fovExtra = 30.0f;
                jerkPushZ = 0.55f;
                carSpd = 2.5f;
                PlaySound(g_sndThunder);
                phase = INTRO_JERK_2;
                timer = 0.0f;
            }
            break;
        case INTRO_JERK_2:
            carSpd = Lerp(carSpd, 7.0f, dt * 1.2f);
            carZ += carSpd * dt;
            {
                Vector3 eye = GetDriverEyePos();
                camPosTarget = { eye.x + shakeX, eye.y + shakeY, eye.z + jerkPushZ };
                camTgtTarget = { eye.x + shakeX * 0.3f, eye.y - 0.06f, carZ + 8.0f };
                camFovTarget = 68.0f + fovExtra;
                camLerpSpd = 20.0f;
            }
            if (timer >= 3.0f) {
                jerkCount = 3;
                shakeDec = 8.0f;
                fovExtra = 38.0f;
                jerkPushZ = 0.80f;
                carSpd = 0.0f;
                PlaySound(g_sndThunder);
                phase = INTRO_JERK_3_STOP;
                timer = 0.0f;
            }
            break;
        case INTRO_JERK_3_STOP: {
            Vector3 eye = GetDriverEyePos();
            camPosTarget = { eye.x + shakeX * 0.5f, eye.y + shakeY * 0.5f, eye.z + jerkPushZ };
            camTgtTarget = { eye.x, eye.y - 0.05f, carZ + 8.0f };
            camFovTarget = 68.0f + fovExtra;
            camLerpSpd = 16.0f;
            if (timer >= 2.8f) {
                phase = INTRO_DOOR_OPEN;
                timer = 0.0f;
                doorAng = 0.0f;
            }
            break;
        }
        case INTRO_DOOR_OPEN: {
            float targetDoor = 78.0f;
            doorAng = Lerp(doorAng, targetDoor, dt * 2.4f);
            float t = Clamp(timer / 2.0f, 0.0f, 1.0f);
            float s = t * t * (3.0f - 2.0f * t);
            Vector3 eye = GetDriverEyePos();
            Vector3 forwardTgt = { eye.x, eye.y - 0.05f, carZ + 8.0f };
            Vector3 doorTgt = { carX - 2.5f, eye.y - 0.10f, carZ + 0.5f };
            camPosTarget = { eye.x - s * 0.20f, eye.y, eye.z + s * 0.10f };
            camTgtTarget = Vector3Lerp(forwardTgt, doorTgt, s);
            camFovTarget = 70.0f;
            camLerpSpd = 16.0f;
            if (timer >= 2.2f) {
                phase = INTRO_EXIT_CAR;
                timer = 0.0f;
                playerT = 0.0f;
            }
            break;
        }
        case INTRO_EXIT_CAR: {
            playerT += dt * 0.30f;
            if (playerT > 1.0f) playerT = 1.0f;
            Vector3 inSeat = { carX - 0.70f, GetCarCY() + 0.35f, carZ + 0.25f };
            Vector3 outside = { carX - 1.55f, carY + 1.75f, carZ + 0.40f };
            Vector3 frontBmp = { carX - 0.40f, carY + 1.75f, carZ + 4.20f };
            Vector3 eyePos;
            Vector3 eyeTgt;
            if (playerT < 0.35f) {
                float subT = playerT / 0.35f;
                float s = subT * subT * (3.0f - 2.0f * subT);
                eyePos = Vector3Lerp(inSeat, outside, s);
                eyePos.y += sinf(subT * PI) * 0.06f;
                Vector3 lookGround = { carX - 1.8f, carY + 0.3f, carZ + 1.5f };
                Vector3 lookAhead = { carX - 1.0f, carY + 1.5f, carZ + 5.0f };
                eyeTgt = Vector3Lerp(lookGround, lookAhead, s);
            } else {
                float subT = (playerT - 0.35f) / 0.65f;
                float s = subT * subT * (3.0f - 2.0f * subT);
                eyePos = Vector3Lerp(outside, frontBmp, s);
                eyePos.y += sinf(subT * 16.0f) * 0.04f;
                eyeTgt = { carX - 0.30f, carY + 1.60f, carZ + 12.0f };
            }
            camPosTarget = eyePos;
            camTgtTarget = eyeTgt;
            camFovTarget = 70.0f;
            camLerpSpd = 18.0f;
            if (timer >= 3.6f) {
                phase = INTRO_SISTER_EXIT;
                timer = 0.0f;
                sisterT = 0.0f;
            }
            break;
        }
        case INTRO_SISTER_EXIT: {
            sisterT += dt * 0.28f;
            if (sisterT > 1.0f) sisterT = 1.0f;
            if (timer >= 0.6f) sisterOut = true;
            Vector3 standPos = { carX - 0.35f, carY + 1.76f, carZ + 4.20f };
            Vector3 sPos = GetSisterStandPos(sisterT);
            camPosTarget = standPos;
            camTgtTarget = { sPos.x, sPos.y, sPos.z };
            camFovTarget = 68.0f;
            camLerpSpd = 14.0f;
            if (timer >= 3.6f) {
                phase = INTRO_LOOK_AROUND;
                timer = 0.0f;
                lookStep = 0;
                lookYaw = 0.0f;
            }
            break;
        }
        case INTRO_LOOK_AROUND: {
            float eyeX = carX - 0.30f;
            float eyeY = carY + 1.78f;
            float eyeZ = carZ + 4.2f;
            camPosTarget = { eyeX, eyeY, eyeZ };
            camTgtTarget = { eyeX + sinf(lookYaw) * 4.0f, eyeY - 0.04f, eyeZ + cosf(lookYaw) * 4.0f };
            camFovTarget = 68.0f;
            camLerpSpd = 18.0f;
            switch (lookStep) {
                case 0:
                    lookYaw = Lerp(lookYaw, 0.90f, dt * 2.0f);
                    if (fabsf(lookYaw - 0.90f) < 0.05f) { lookStep = 1; timer = 0.0f; }
                    break;
                case 1:
                    eyeT = (timer < 0.35f) ? Lerp(eyeT, 1.0f, dt * 12.0f) : Lerp(eyeT, 0.0f, dt * 10.0f);
                    if (timer >= 0.85f) { lookStep = 2; timer = 0.0f; }
                    break;
                case 2:
                    lookYaw = Lerp(lookYaw, -0.90f, dt * 2.0f);
                    if (fabsf(lookYaw - (-0.90f)) < 0.05f) { lookStep = 3; timer = 0.0f; }
                    break;
                case 3:
                    eyeT = (timer < 0.35f) ? Lerp(eyeT, 1.0f, dt * 12.0f) : Lerp(eyeT, 0.0f, dt * 10.0f);
                    if (timer >= 0.85f) { lookStep = 4; timer = 0.0f; }
                    break;
                default:
                    lookYaw = Lerp(lookYaw, 0.0f, dt * 2.5f);
                    if (timer >= 1.8f) { phase = INTRO_FADE_TO_GAME; timer = 0.0f; }
                    break;
            }
            break;
        }
        case INTRO_FADE_TO_GAME: {
            fadeAlpha += dt * 0.65f;
            if (fadeAlpha >= 1.0f) { fadeAlpha = 1.0f; phase = INTRO_DONE; }
            float eyeX = carX - 0.30f, eyeY = carY + 1.78f, eyeZ = carZ + 4.2f;
            camPosTarget = { eyeX, eyeY, eyeZ };
            camTgtTarget = { eyeX, eyeY - 0.04f, eyeZ + 3.5f };
            camFovTarget = 68.0f;
            camLerpSpd = 10.0f;
            break;
        }
        default: break;
    }

    float ls = camLerpSpd * dt;
    if (ls > 1.0f) ls = 1.0f;
    cam.position = Vector3Lerp(cam.position, camPosTarget, ls);
    cam.target = Vector3Lerp(cam.target, camTgtTarget, ls);
    cam.fovy = Lerp(cam.fovy, camFovTarget, ls);
}

void IntroCinematic::Draw(RenderTexture2D target, Shader instancedShader, Mesh quad, Material material, Chunk* chunk) {
    float timeVal = (float)GetTime();
    bool driverFP = (phase == INTRO_DRIVER_POV || phase == INTRO_JERK_1 || phase == INTRO_JERK_2 || phase == INTRO_JERK_3_STOP);

    // --- REARVIEW MIRROR PRE-PASS ---
    if (driverFP) {
        Camera3D mCam = { 0 };
        mCam.position = { carX, GetCarCY() + 0.52f, carZ + 0.90f };
        mCam.target = { carX, GetCarCY() + 0.46f, carZ - 20.0f };
        mCam.up = { 0.0f, 1.0f, 0.0f };
        mCam.fovy = 58.0f;
        mCam.projection = CAMERA_PERSPECTIVE;

        BeginTextureMode(mirrorRT);
        ClearBackground({ 14, 18, 26, 255 });
        BeginMode3D(mCam);

        BeginShaderMode(instancedShader);
        Vector2 uvScl = { 1.0f / 16.0f, 1.0f / 16.0f };
        SetShaderValue(instancedShader, uvScaleLoc, &uvScl, SHADER_UNIFORM_VEC2);
        SetShaderValue(instancedShader, timeLoc, &timeVal, SHADER_UNIFORM_FLOAT);
        SetShaderValue(instancedShader, playerPosLoc, &g_camera.position, SHADER_UNIFORM_VEC3);

        float introDayFactor = 0.0f;
        Vector3 introSunDir = { 0.0f, -1.0f, 0.0f };
        Vector3 introSunColor = { 1.0f, 0.96f, 0.85f };
        SetShaderValue(instancedShader, sunDirLoc, &introSunDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(instancedShader, dayFactorLoc, &introDayFactor, SHADER_UNIFORM_FLOAT);
        SetShaderValue(instancedShader, sunColorLoc, &introSunColor, SHADER_UNIFORM_VEC3);
        Vector3 introMoonDir = { 0.0f, 1.0f, 0.0f };
        float introNightFactor = 1.0f;
        SetShaderValue(instancedShader, moonDirLoc, &introMoonDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(instancedShader, nightFactorLoc, &introNightFactor, SHADER_UNIFORM_FLOAT);

        for (int bx = 0; bx < BUCKETS_X; bx++) {
            for (int bz = 0; bz < BUCKETS_Z; bz++) {
                float centerX = bx * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);
                float centerZ = bz * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);
                float dx = centerX - mCam.position.x;
                float dz = centerZ - mCam.position.z;
                if (dz < 10.0f && dx*dx + dz*dz < 75.0f*75.0f) {
                    RenderBucket* b = &chunk->buckets[bx][bz];
                    for (uint8_t g : b->activeGlyphs) {
                        int col = g % 16; int row = g / 16;
                        Vector2 uvOff = { col * uvScl.x, row * uvScl.y };
                        SetShaderValue(instancedShader, uvOffsetLoc, &uvOff, SHADER_UNIFORM_VEC2);
                        DrawMeshInstanced(quad, material, b->instances[g].data(), (int)b->instances[g].size());
                    }
                }
            }
        }
        EndShaderMode();
        EndMode3D();
        EndTextureMode();
    }

    // --- MAIN 3D PASS ---
    Vector3 shakeCamPos = cam.position;
    Vector3 shakeCamTgt = cam.target;
    ApplyShake(shakeCamPos, shakeCamTgt);
    Camera3D shakeCam = cam;
    shakeCam.position = shakeCamPos;
    shakeCam.target = shakeCamTgt;

    BeginTextureMode(target);
    ClearBackground(BLACK);
    BeginMode3D(shakeCam);

    BeginShaderMode(instancedShader);
    Vector2 uvScl = { 1.0f / 16.0f, 1.0f / 16.0f };
    SetShaderValue(instancedShader, uvScaleLoc, &uvScl, SHADER_UNIFORM_VEC2);
    SetShaderValue(instancedShader, timeLoc, &timeVal, SHADER_UNIFORM_FLOAT);
    SetShaderValue(instancedShader, playerPosLoc, &g_camera.position, SHADER_UNIFORM_VEC3);
    EndShaderMode();

    DrawMesh(g_mGround, g_matGround, MatrixTranslate(carX, carY + 0.00f, 250.0f));
    DrawMesh(g_mRoad, g_matRoad, MatrixTranslate(carX, carY + 0.01f, 250.0f));

    float ccy = GetCarCY();
    bool drawExt = (phase == INTRO_AERIAL || phase == INTRO_DOOR_OPEN || phase == INTRO_EXIT_CAR || phase == INTRO_SISTER_EXIT || phase == INTRO_LOOK_AROUND);
    if (drawExt) {
        DrawMesh(mBody, matBody, MatrixTranslate(carX, ccy, carZ));
        DrawMesh(mCabin, matCabin, MatrixMultiply(MatrixScale(1.0f, 1.0f, 1.0f), MatrixTranslate(carX, ccy + 0.52f + 0.36f, carZ - 0.25f)));
    }

    if (drawExt) {
        Draw3DCarInterior(carX, carY, carZ, ccy, timeVal, carSpd, shakeDec, jerkCount, mSeat, mSeatBack, mSteerRim, matSeat, matCabin);
    }

    EndMode3D();
    EndTextureMode();
}
