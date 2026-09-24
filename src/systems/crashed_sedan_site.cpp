#include "crashed_sedan_site.h"
#include <raymath.h>
#include <rlgl.h>
#include <math.h>

const Vector3 g_crashedCarPos = { 143.8f, 9.92f, 136.5f };

Color CrashCarTint(Color base, float dayF, float nightF, float vertBias) {
    float amb = 0.28f + 0.62f * dayF + 0.15f * nightF;
    float r = ((float)base.r / 255.0f) * amb * vertBias;
    float g = ((float)base.g / 255.0f) * amb * vertBias;
    float b = ((float)base.b / 255.0f) * amb * vertBias;
    return Color{
        (unsigned char)Clamp(r * 255.0f, 0.0f, 255.0f),
        (unsigned char)Clamp(g * 255.0f, 0.0f, 255.0f),
        (unsigned char)Clamp(b * 255.0f, 0.0f, 255.0f),
        base.a
    };
}

void DrawCrashedSedan(Vector3 carPos, float timeVal, float extDayFactor, float extNightFactor, const Camera3D& camera) {
    float distSq = (carPos.x - camera.position.x)*(carPos.x - camera.position.x) + (carPos.z - camera.position.z)*(carPos.z - camera.position.z);
    if (distSq > 130.0f * 130.0f) return;

    // 1. Skid marks curving from Route 9 asphalt into roadside ditch verge
    for (int t = 0; t < 18; t++) {
        float f = (float)t / 17.0f;
        float sx = Lerp(138.5f, 142.8f, f * f);
        float sz = Lerp(126.0f, 134.5f, f);
        float sy = 10.015f - f * 0.05f;
        DrawCube(Vector3{ sx - 0.75f, sy, sz }, 0.22f, 0.005f, 0.55f, Color{ 12, 12, 14, (unsigned char)(160.0f * (1.0f - f * 0.4f)) });
        DrawCube(Vector3{ sx + 0.75f, sy, sz }, 0.24f, 0.005f, 0.55f, Color{ 10, 10, 12, (unsigned char)(150.0f * (1.0f - f * 0.4f)) });
        if (t > 10) {
            DrawCube(Vector3{ sx + 0.95f, sy + 0.03f, sz }, 0.18f, 0.06f, 0.35f, Color{ 36, 28, 18, 220 }); // Torn muddy turf furrow
        }
    }

    // 2. Splintered wooden utility pole sheared off at impact (Stump center: X = 145.3f, Z = 138.8f)
    Vector3 poleBase = { 145.3f, 9.90f, 138.8f };
    Color woodPost = CrashCarTint(Color{ 62, 52, 40, 255 }, extDayFactor, extNightFactor);
    Color woodSplinter = CrashCarTint(Color{ 110, 95, 70, 255 }, extDayFactor, extNightFactor);

    // Sheared stump in mud
    DrawCylinder(poleBase, 0.22f, 0.24f, 0.85f, 8, woodPost);
    DrawCube(Vector3{ poleBase.x - 0.08f, 10.80f, poleBase.z }, 0.08f, 0.35f, 0.08f, woodSplinter);
    DrawCube(Vector3{ poleBase.x + 0.06f, 10.75f, poleBase.z - 0.05f }, 0.07f, 0.28f, 0.07f, woodSplinter);

    // Tilted upper pole resting across car hood
    rlPushMatrix();
    rlTranslatef(poleBase.x, 10.75f, poleBase.z);
    rlRotatef(22.0f, 0.0f, 0.0f, 1.0f);
    rlRotatef(-14.0f, 1.0f, 0.0f, 0.0f);
    DrawCylinder(Vector3{ 0.0f, 0.0f, 0.0f }, 0.20f, 0.17f, 6.5f, 8, woodPost);
    DrawCube(Vector3{ 0.0f, 5.8f, 0.0f }, 1.8f, 0.12f, 0.12f, woodPost);
    DrawCylinder(Vector3{ -0.7f, 6.0f, 0.0f }, 0.06f, 0.06f, 0.18f, 6, Color{ 200, 210, 215, 255 }); // Ceramic insulator 1
    DrawCylinder(Vector3{  0.7f, 6.0f, 0.0f }, 0.06f, 0.06f, 0.18f, 6, Color{ 200, 210, 215, 255 }); // Ceramic insulator 2
    rlPopMatrix();

    // Snapped low-voltage wires dangling down into the weeds
    DrawLine3D(Vector3{ poleBase.x + 1.2f, 15.5f, poleBase.z }, Vector3{ poleBase.x + 0.6f, 12.0f, poleBase.z + 0.8f }, Color{ 20, 20, 22, 255 });
    DrawLine3D(Vector3{ poleBase.x + 0.6f, 12.0f, poleBase.z + 0.8f }, Vector3{ poleBase.x - 0.2f, 10.2f, poleBase.z + 0.5f }, Color{ 20, 20, 22, 255 });

    // Dented "MILE 14" roadside marker post knocked askew into the mud
    DrawCube(Vector3{ 142.2f, 10.08f, 134.8f }, 0.08f, 0.16f, 0.85f, Color{ 90, 95, 100, 255 });
    DrawCube(Vector3{ 142.2f, 10.15f, 134.8f }, 0.22f, 0.02f, 0.35f, Color{ 215, 220, 210, 255 });

    // Buckled steel guardrail segment bent around front bumper
    DrawCube(Vector3{ 144.2f, 10.45f, 139.2f }, 1.8f, 0.32f, 0.08f, Color{ 130, 135, 140, 255 });
    DrawCubeWires(Vector3{ 144.2f, 10.45f, 139.2f }, 1.82f, 0.33f, 0.09f, Color{ 70, 75, 80, 255 });

    // Dark oil / coolant spill beneath crushed engine
    DrawCircle3D(Vector3{ 144.6f, 9.97f, 138.2f }, 1.15f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 8, 8, 10, 240 });
    DrawCircle3D(Vector3{ 144.3f, 9.97f, 137.8f }, 0.65f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 15, 25, 20, 190 });

    // Scattered safety glass shards glinting on ground
    for (int g = 0; g < 14; g++) {
        float gx = 144.0f + sinf((float)g * 1.8f) * 1.2f;
        float gz = 137.5f + cosf((float)g * 2.3f) * 1.4f;
        float shimmer = 0.6f + 0.4f * sinf(timeVal * 4.0f + (float)g);
        DrawCube(Vector3{ gx, 9.98f, gz }, 0.04f, 0.015f, 0.04f, Color{ 210, 235, 255, (unsigned char)(shimmer * 180.0f) });
    }

    // 3. MAIN CAR BODY (Transformed with Yaw: -24 deg, Roll: -6 deg, Pitch: 3.5 deg)
    rlPushMatrix();
    rlTranslatef(carPos.x, carPos.y, carPos.z);
    rlRotatef(-24.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(-6.0f, 0.0f, 0.0f, 1.0f);
    rlRotatef(3.5f, 1.0f, 0.0f, 0.0f);

    Color carPaint      = CrashCarTint(Color{ 135, 140, 150, 255 }, extDayFactor, extNightFactor, 1.0f);
    Color carPaintDark  = CrashCarTint(Color{ 110, 115, 125, 255 }, extDayFactor, extNightFactor, 0.85f);
    Color crumpledMetal = CrashCarTint(Color{ 88, 92, 102, 255 }, extDayFactor, extNightFactor, 0.70f);

    // Chassis drop shadow in local space
    DrawCube(Vector3{ 0.0f, 0.02f, 0.0f }, 2.6f, 0.005f, 5.4f, Color{ 6, 6, 8, 200 });

    // Lower chassis body frame
    DrawCube(Vector3{ 0.0f, 0.62f, -0.2f }, 2.30f, 0.65f, 4.6f, carPaint);
    // Mud splatter along lower sills & undercarriage
    DrawCube(Vector3{ 0.0f, 0.35f, -0.2f }, 2.34f, 0.22f, 4.62f, Color{ 42, 34, 24, 230 });

    // Crumpled front end / engine compartment (accordion crushed inwards)
    DrawCube(Vector3{ -0.55f, 0.68f, 2.15f }, 1.10f, 0.58f, 0.85f, crumpledMetal);

    // Right side crushed heavily against pole impact
    rlPushMatrix();
    rlTranslatef(0.50f, 0.65f, 2.05f);
    rlRotatef(-16.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(12.0f, 1.0f, 0.0f, 0.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 1.15f, 0.52f, 0.95f, Color{ 75, 80, 88, 255 });
    rlPopMatrix();

    // Smashed radiator & engine block exposed inside
    DrawCube(Vector3{ 0.0f, 0.70f, 2.10f }, 1.30f, 0.50f, 0.65f, Color{ 28, 30, 32, 255 });
    DrawCube(Vector3{ 0.15f, 0.72f, 2.25f }, 0.65f, 0.40f, 0.12f, Color{ 55, 58, 62, 255 });
    DrawCubeWires(Vector3{ 0.15f, 0.72f, 2.25f }, 0.66f, 0.41f, 0.13f, Color{ 140, 60, 30, 255 });

    // Front Bumper (accordion bent inwards at center)
    DrawCube(Vector3{ -0.65f, 0.38f, 2.58f }, 1.15f, 0.18f, 0.18f, Color{ 180, 185, 190, 255 });
    rlPushMatrix();
    rlTranslatef(0.55f, 0.38f, 2.50f);
    rlRotatef(-28.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(-10.0f, 0.0f, 0.0f, 1.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 1.15f, 0.18f, 0.18f, Color{ 140, 145, 150, 255 });
    rlPopMatrix();

    // Popped V-buckled hood (two bent panels meeting in a tented crease)
    rlPushMatrix();
    rlTranslatef(0.0f, 0.96f, 1.45f);
    rlRotatef(24.0f, 1.0f, 0.0f, 0.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 2.08f, 0.06f, 0.95f, carPaint);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.08f, 1.12f, 2.08f);
    rlRotatef(-32.0f, 1.0f, 0.0f, 0.0f);
    rlRotatef(8.0f, 0.0f, 0.0f, 1.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 2.05f, 0.06f, 0.75f, carPaintDark);
    rlPopMatrix();

    // Trunk & Rear Bumper
    DrawCube(Vector3{ 0.0f, 0.82f, -1.80f }, 2.15f, 0.20f, 1.35f, carPaint);
    DrawCube(Vector3{ 0.0f, 0.38f, -2.65f }, 2.25f, 0.18f, 0.18f, Color{ 200, 205, 210, 255 });

    // Cabin Greenhouse & Roof
    DrawCube(Vector3{ 0.0f, 1.48f, -0.30f }, 1.95f, 0.06f, 2.45f, carPaint);
    DrawCube(Vector3{ -0.96f, 1.25f, -1.45f }, 0.08f, 0.55f, 0.08f, carPaint);
    DrawCube(Vector3{  0.96f, 1.25f, -1.45f }, 0.08f, 0.55f, 0.08f, carPaint);
    DrawCube(Vector3{ -0.96f, 1.25f,  0.95f }, 0.08f, 0.55f, 0.08f, carPaint);
    DrawCube(Vector3{  0.96f, 1.25f,  0.95f }, 0.08f, 0.55f, 0.08f, carPaint);

    // Dark interior cavity & seats
    DrawCube(Vector3{ 0.0f, 1.15f, -0.30f }, 1.88f, 0.58f, 2.40f, Color{ 16, 18, 22, 255 });
    DrawCube(Vector3{ -0.48f, 1.02f, -0.45f }, 0.55f, 0.45f, 0.55f, Color{ 35, 38, 42, 255 });
    DrawCube(Vector3{  0.48f, 1.02f, -0.45f }, 0.55f, 0.45f, 0.55f, Color{ 35, 38, 42, 255 });

    // Deflated white airbag draped over driver steering wheel
    DrawSphere(Vector3{ -0.48f, 1.18f, 0.22f }, 0.22f, Color{ 220, 220, 215, 240 });
    DrawCube(Vector3{ -0.48f, 1.10f, 0.24f }, 0.32f, 0.12f, 0.25f, Color{ 195, 195, 190, 255 });

    // Driver's door sprung ajar ~20 degrees
    rlPushMatrix();
    rlTranslatef(-1.12f, 0.72f, 0.75f);
    rlRotatef(-20.0f, 0.0f, 1.0f, 0.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, -0.65f }, 0.08f, 0.72f, 1.25f, carPaint);
    DrawCube(Vector3{ 0.04f, 0.0f, -0.65f }, 0.03f, 0.65f, 1.15f, Color{ 32, 34, 38, 255 });
    rlPopMatrix();

    // Passenger door (dented shut)
    DrawCube(Vector3{ 1.14f, 0.72f, 0.10f }, 0.06f, 0.72f, 1.30f, carPaint);

    // Spiderweb Cracked Windshield
    rlPushMatrix();
    rlTranslatef(0.0f, 1.26f, 0.72f);
    rlRotatef(-34.0f, 1.0f, 0.0f, 0.0f);
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 1.90f, 0.02f, 1.15f, Color{ 190, 215, 235, 110 });
    DrawLine3D(Vector3{ -0.45f, 0.02f, 0.10f }, Vector3{ -0.15f, 0.02f, 0.45f }, Color{ 255, 255, 255, 220 });
    DrawLine3D(Vector3{ -0.45f, 0.02f, 0.10f }, Vector3{ -0.75f, 0.02f, 0.35f }, Color{ 255, 255, 255, 220 });
    DrawLine3D(Vector3{ -0.45f, 0.02f, 0.10f }, Vector3{ -0.55f, 0.02f, -0.35f }, Color{ 255, 255, 255, 220 });
    DrawLine3D(Vector3{ -0.45f, 0.02f, 0.10f }, Vector3{  0.25f, 0.02f, -0.15f }, Color{ 255, 255, 255, 200 });
    DrawCircle3D(Vector3{ -0.45f, 0.025f, 0.10f }, 0.28f, Vector3{ 0, 1, 0 }, 0.0f, Color{ 240, 245, 255, 130 });
    rlPopMatrix();

    // 4 WHEELS (With damage & mud)
    DrawCylinder(Vector3{ -1.15f, 0.35f, -1.60f }, 0.35f, 0.35f, 0.22f, 10, Color{ 20, 20, 22, 255 });
    DrawCylinder(Vector3{  1.15f, 0.30f, -1.60f }, 0.35f, 0.35f, 0.22f, 10, Color{ 18, 18, 20, 255 });

    // Front Left Wheel (tilted outward)
    rlPushMatrix();
    rlTranslatef(-1.18f, 0.35f, 1.60f);
    rlRotatef(-14.0f, 0.0f, 1.0f, 0.0f);
    DrawCylinder(Vector3{ 0.0f, 0.0f, 0.0f }, 0.35f, 0.35f, 0.22f, 10, Color{ 22, 22, 24, 255 });
    rlPopMatrix();

    // Front Right Wheel (heavily bent camber, popped tire, jammed in mud)
    rlPushMatrix();
    rlTranslatef(1.12f, 0.22f, 1.55f);
    rlRotatef(28.0f, 0.0f, 0.0f, 1.0f);
    rlRotatef(18.0f, 0.0f, 1.0f, 0.0f);
    DrawCylinder(Vector3{ 0.0f, 0.0f, 0.0f }, 0.32f, 0.28f, 0.25f, 10, Color{ 28, 24, 20, 255 });
    rlPopMatrix();

    // Smashed Right Headlight (empty crushed dark socket with dangling copper wires)
    DrawCube(Vector3{ 0.85f, 0.68f, 2.50f }, 0.28f, 0.16f, 0.12f, Color{ 18, 18, 20, 255 });
    DrawLine3D(Vector3{ 0.80f, 0.65f, 2.55f }, Vector3{ 0.88f, 0.52f, 2.62f }, Color{ 180, 100, 30, 255 });
    DrawLine3D(Vector3{ 0.86f, 0.65f, 2.55f }, Vector3{ 0.82f, 0.48f, 2.60f }, Color{ 60, 120, 180, 255 });

    // Left Headlight (cracked lens, flickering weakly)
    bool lightFlicker = (fmodf(timeVal * 7.5f, 1.0f) > 0.25f);
    Color headlitCol = lightFlicker ? Color{ 255, 240, 180, 210 } : Color{ 65, 60, 45, 255 };
    DrawCube(Vector3{ -0.85f, 0.68f, 2.55f }, 0.26f, 0.15f, 0.08f, headlitCol);

    // Hazard Flashers (Only rear left still pulses amber, front smashed)
    bool hazFlash = (fmodf(timeVal, 1.1f) < 0.55f);
    Color hazAmber = hazFlash ? Color{ 255, 135, 15, 255 } : Color{ 45, 20, 5, 255 };
    DrawCube(Vector3{ -0.95f, 0.70f, -2.62f }, 0.24f, 0.12f, 0.08f, hazAmber);
    DrawCube(Vector3{  0.95f, 0.70f, -2.62f }, 0.24f, 0.12f, 0.08f, Color{ 30, 15, 10, 255 });

    rlPopMatrix(); // End car local transform

    // 4. Steam / Smoke Motes rising from the punctured radiator (world space)
    Vector3 radPos = { carPos.x + 0.35f, carPos.y + 1.15f, carPos.z + 1.8f };
    for (int sm = 0; sm < 5; sm++) {
        float sSeed = (float)sm * 1.85f;
        float sAge = fmodf(timeVal * 0.85f + sSeed, 2.2f);
        float sAlpha = (1.0f - (sAge / 2.2f));
        float sx = radPos.x + sinf(timeVal * 1.2f + sSeed) * 0.18f;
        float sy = radPos.y + sAge * 0.75f;
        float sz = radPos.z + cosf(timeVal * 1.0f + sSeed) * 0.18f;
        float sSize = 0.06f + sAge * 0.08f;
        DrawCube(Vector3{ sx, sy, sz }, sSize, sSize, sSize, Color{ 230, 235, 240, (unsigned char)(sAlpha * 95.0f) });
    }
}

