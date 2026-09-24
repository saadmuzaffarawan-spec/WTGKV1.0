#include "footprint_decals.h"
#include <raymath.h>
#include <rlgl.h>
#include <math.h>
#include <string.h>

static FootprintDecal g_footprints[MAX_FOOTPRINTS];
static int g_footprintHead = 0;
static Vector3 g_lastFootprintPos = { 0.0f, -999.0f, 0.0f };
static bool g_nextFootLeft = false;
static float g_playerBloodStainTimer = 0.0f;

void InitFootprints() {
    memset(g_footprints, 0, sizeof(g_footprints));
    g_footprintHead = 0;
    g_lastFootprintPos = Vector3{ 0.0f, -999.0f, 0.0f };
    g_nextFootLeft = false;
    g_playerBloodStainTimer = 0.0f;
}

void UpdateFootprints(Vector3 camPos, Vector3 viewDir, bool onGround, float hitStopTimer, float dt) {
    const float PLAYER_EYE_HEIGHT = 1.65f;
    if (onGround && hitStopTimer <= 0.0f) {
        Vector2 curXZ = { camPos.x, camPos.z };
        Vector2 lastXZ = { g_lastFootprintPos.x, g_lastFootprintPos.z };
        float distMoved = Vector2Distance(curXZ, lastXZ);
        if (distMoved >= 0.65f) {
            g_lastFootprintPos = camPos;
            Vector3 fwdH = Vector3Normalize(Vector3{ viewDir.x, 0.0f, viewDir.z });
            if (Vector3Length(fwdH) < 0.05f) fwdH = Vector3{ 0.0f, 0.0f, 1.0f };
            Vector3 rightH = { -fwdH.z, 0.0f, fwdH.x };

            float sideSign = g_nextFootLeft ? -1.0f : 1.0f;
            g_nextFootLeft = !g_nextFootLeft;
            float sideDist = 0.13f;

            float fYaw = atan2f(fwdH.x, fwdH.z) * RAD2DEG;
            float floorY = camPos.y - PLAYER_EYE_HEIGHT;
            if (floorY < 10.015f && camPos.y >= 10.0f) floorY = 10.0f;

            // Detect if stepping in or near blood pools:
            // Abandoned College blood pools:
            bool inCollegeBlood = (camPos.x >= 153.0f && camPos.x <= 186.0f && camPos.z >= 125.0f && camPos.z <= 159.0f) &&
                ((camPos.z >= 139.3f && camPos.z <= 141.7f && camPos.x >= 158.0f && camPos.x <= 178.0f) || // corridor drag
                 (camPos.x >= 160.0f && camPos.x <= 170.0f && camPos.z >= 126.0f && camPos.z <= 136.0f) || // lecture hall pool
                 (camPos.x >= 161.0f && camPos.x <= 167.0f && camPos.z >= 148.0f && camPos.z <= 154.0f));  // anatomy dissection pool
            // Supermarket meat locker blood pool:
            bool inShopBlood = (camPos.x >= 87.0f && camPos.x <= 95.0f && camPos.z >= 146.0f && camPos.z <= 153.0f);

            if (inCollegeBlood || inShopBlood) {
                g_playerBloodStainTimer = 16.0f; // Stains boots for several subsequent strides!
            }

            bool isBloody = (g_playerBloodStainTimer > 0.0f);

            FootprintDecal& fp = g_footprints[g_footprintHead];
            fp.pos = Vector3{ camPos.x + rightH.x * sideDist * sideSign, floorY + 0.018f, camPos.z + rightH.z * sideDist * sideSign };
            fp.yaw = fYaw;
            fp.life = 8.0f; // Dissolves smoothly over 8 seconds!
            fp.maxLife = 8.0f;
            fp.isLeft = !g_nextFootLeft;
            fp.isBloody = isBloody;

            g_footprintHead = (g_footprintHead + 1) % MAX_FOOTPRINTS;
        }
    }

    // Dissolve active footprints over time
    for (int i = 0; i < MAX_FOOTPRINTS; i++) {
        if (g_footprints[i].life > 0.0f) {
            g_footprints[i].life -= dt;
            if (g_footprints[i].life < 0.0f) g_footprints[i].life = 0.0f;
        }
    }
    if (g_playerBloodStainTimer > 0.0f) g_playerBloodStainTimer -= dt;
}

void DrawFootprints() {
    for (int i = 0; i < MAX_FOOTPRINTS; i++) {
        const FootprintDecal& fp = g_footprints[i];
        if (fp.life <= 0.0f) continue;
        float alphaRatio = Clamp(fp.life / fp.maxLife, 0.0f, 1.0f);

        rlPushMatrix();
        rlTranslatef(fp.pos.x, fp.pos.y, fp.pos.z);
        rlRotatef(fp.yaw, 0.0f, 1.0f, 0.0f);

        if (fp.isBloody) {
            unsigned char a = (unsigned char)(alphaRatio * 220.0f);
            Color bloodSole = { 135, 12, 18, a };
            Color bloodHeel = { 95, 8, 12, a };
            // Sole
            DrawCube(Vector3{ 0.0f, 0.001f, 0.055f }, 0.10f, 0.002f, 0.14f, bloodSole);
            // Heel
            DrawCube(Vector3{ 0.0f, 0.001f, -0.065f }, 0.085f, 0.002f, 0.075f, bloodHeel);
            // Splatter droplets
            Color dripCol = { 110, 10, 14, (unsigned char)(a * 0.75f) };
            DrawCube(Vector3{ fp.isLeft ? -0.065f : 0.065f, 0.001f, 0.02f }, 0.025f, 0.002f, 0.025f, dripCol);
            DrawCube(Vector3{ fp.isLeft ? -0.08f : 0.08f, 0.001f, -0.02f }, 0.018f, 0.002f, 0.018f, dripCol);
        } else {
            unsigned char a = (unsigned char)(alphaRatio * 155.0f);
            Color treadCol = { 26, 24, 22, a };
            Color ridgeCol = { 14, 12, 11, (unsigned char)(a * 0.85f) };
            // Sole
            DrawCube(Vector3{ 0.0f, 0.001f, 0.055f }, 0.10f, 0.002f, 0.14f, treadCol);
            // Heel
            DrawCube(Vector3{ 0.0f, 0.001f, -0.065f }, 0.085f, 0.002f, 0.075f, treadCol);
            // Tread grooves
            DrawCube(Vector3{ 0.0f, 0.0015f, 0.025f }, 0.09f, 0.002f, 0.015f, ridgeCol);
            DrawCube(Vector3{ 0.0f, 0.0015f, 0.065f }, 0.09f, 0.002f, 0.015f, ridgeCol);
            DrawCube(Vector3{ 0.0f, 0.0015f, 0.105f }, 0.08f, 0.002f, 0.015f, ridgeCol);
        }
        rlPopMatrix();
    }
}

