#include "gas_station_system.h"
#include <raymath.h>
#include <rlgl.h>
#include <math.h>
#include <stdio.h>

void SpawnCustomerCar(CustomerCar& car, float fuelPricePerGallon) {
    car.state = CAR_APPROACHING;
    car.targetPump = (GetRandomValue(0, 1) == 0) ? 0 : 1;
    car.targetZ = (car.targetPump == 0) ? 137.5f : 142.5f;
    car.pos = (Vector3){ 123.5f, 10.02f, 65.0f }; // Approaching from Route 9 North
    car.speed = 14.0f;
    car.requestedGallons = 8.0f + (float)GetRandomValue(0, 120) / 10.0f; // 8.0 to 20.0 Gal
    car.dispensedGallons = 0.0f;
    car.totalSale = car.requestedGallons * fuelPricePerGallon;
    car.tipAmount = 4.0f + (float)GetRandomValue(0, 80) / 10.0f;
    car.waitTimer = 0.0f;

    Color palette[4] = {
        (Color){ 62, 54, 48, 255 },  // Rusted brown station wagon
        (Color){ 35, 42, 52, 255 },  // Dark midnight blue sedan
        (Color){ 72, 68, 62, 255 },  // Weathered gray pickup
        (Color){ 55, 24, 26, 255 }   // Muddy dark maroon coupe
    };
    car.bodyColor = palette[GetRandomValue(0, 3)];

    const char* quotes[] = {
        "\"Thanks... whatever you do out here, keep your eyes on the tree line.\"",
        "\"Did you feel that tremor under the asphalt? Something is shifting below.\"",
        "\"I wouldn't stay out on this road past 3 AM if I were you, kid.\"",
        "\"My radio was picking up strange Morse code all through the mountain corridor.\"",
        "\"The air smells like sulfur and rot tonight. Don't go wandering into the mire.\""
    };
    snprintf(car.driverDialogue, sizeof(car.driverDialogue), "%s", quotes[GetRandomValue(0, 4)]);
}

void DrawCatenaryHose(Vector3 start, Vector3 end, float maxSag, int segments, float radius, Color col) {
    float dist = Vector3Distance(start, end);
    float slack = 4.8f - dist;
    if (slack < 0.1f) slack = 0.1f;
    float sagDepth = Clamp(slack * 0.38f, 0.14f, maxSag);

    Vector3 prevPt = start;
    for (int i = 1; i <= segments; i++) {
        float t = (float)i / (float)segments;
        float hx = Lerp(start.x, end.x, t);
        float hz = Lerp(start.z, end.z, t);
        float sag = sagDepth * 4.0f * t * (1.0f - t);
        float hy = Lerp(start.y, end.y, t) - sag;
        if (hy < 10.035f) hy = 10.035f; // Rests on asphalt

        Vector3 curPt = { hx, hy, hz };
        DrawCylinderEx(prevPt, curPt, radius, radius, 6, col);
        prevPt = curPt;
    }

    // Brass threaded coupling collars at both ends
    Color brassCol = { 185, 150, 60, 255 };
    DrawSphere(start, radius * 1.35f, brassCol);
    DrawSphere(end, radius * 1.35f, brassCol);
}

void DrawFirstPersonFuelNozzle(Camera3D cam, bool isFlowing, float walkTime, float timeVal) {
    Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(fwd, cam.up));
    Vector3 camUp = Vector3Normalize(Vector3CrossProduct(camRight, fwd));

    float stepBobY = (walkTime > 0.0f) ? sinf(walkTime * 2.0f) * 0.008f : 0.0f;
    float breathe  = sinf(timeVal * 1.8f) * 0.004f;

    // Anchor in lower right view
    Vector3 handPos = Vector3Add(cam.position,
        Vector3Add(Vector3Scale(camRight, 0.22f),
                   Vector3Add(Vector3Scale(camUp, -0.20f + stepBobY + breathe),
                              Vector3Scale(fwd, 0.40f))));

    Color metalBody  = { 85, 90, 96, 255 };
    Color rubberGrip = { 20, 20, 22, 255 };
    Color chromePipe = { 150, 155, 165, 255 };
    Color brassNut   = { 185, 150, 60, 255 };

    // Handle (angled down/back)
    Vector3 handleTop = handPos;
    Vector3 handleBot = Vector3Add(handPos, Vector3Add(Vector3Scale(camUp, -0.12f), Vector3Scale(fwd, -0.06f)));
    DrawCylinderEx(handleTop, handleBot, 0.024f, 0.022f, 8, rubberGrip);

    // Swivel hose coupling at base
    Vector3 hoseCoupling = Vector3Add(handleBot, Vector3Scale(camUp, -0.025f));
    DrawCylinderEx(handleBot, hoseCoupling, 0.026f, 0.026f, 8, brassNut);

    // Valve body (above handle)
    Vector3 valveBody = Vector3Add(handPos, Vector3Scale(fwd, 0.05f));
    DrawCube(valveBody, 0.045f, 0.055f, 0.075f, metalBody);

    // Trigger guard loop
    Vector3 guardMid = Vector3Add(handPos, Vector3Add(Vector3Scale(fwd, 0.04f), Vector3Scale(camUp, -0.06f)));
    DrawCubeWires(guardMid, 0.025f, 0.065f, 0.055f, metalBody);

    // Steel trigger lever (squeezed when isFlowing)
    float trigOffset = isFlowing ? 0.015f : 0.035f;
    Vector3 trigPos = Vector3Add(handPos, Vector3Add(Vector3Scale(fwd, trigOffset), Vector3Scale(camUp, -0.05f)));
    DrawCube(trigPos, 0.014f, 0.045f, 0.014f, isFlowing ? (Color){ 220, 220, 220, 255 } : (Color){ 160, 165, 170, 255 });

    // Angled fuel spout protruding forward & curving slightly down
    Vector3 spoutStart = Vector3Add(valveBody, Vector3Scale(fwd, 0.04f));
    Vector3 spoutMid   = Vector3Add(spoutStart, Vector3Add(Vector3Scale(fwd, 0.12f), Vector3Scale(camUp, 0.02f)));
    Vector3 spoutTip   = Vector3Add(spoutMid, Vector3Add(Vector3Scale(fwd, 0.14f), Vector3Scale(camUp, -0.04f)));
    DrawCylinderEx(spoutStart, spoutMid, 0.016f, 0.015f, 8, chromePipe);
    DrawCylinderEx(spoutMid, spoutTip, 0.015f, 0.013f, 8, chromePipe);

    // Fuel vapor shimmer particles when pumping
    if (isFlowing) {
        for (int v = 0; v < 3; v++) {
            float vAge = fmodf(timeVal * 4.0f + (float)v * 1.3f, 0.6f);
            Vector3 vPos = Vector3Add(spoutTip, Vector3Add(Vector3Scale(fwd, vAge * 0.15f), Vector3Scale(camUp, -vAge * 0.08f)));
            DrawSphere(vPos, 0.008f + vAge * 0.012f, (Color){ 200, 235, 210, (unsigned char)(110 * (1.0f - vAge / 0.6f)) });
        }
    }
}

void DrawCustomerCar(const CustomerCar& car, bool nozzleInCar) {
    if (car.state == CAR_INACTIVE) return;

    Vector3 cPos = car.pos;
    DrawCube((Vector3){ cPos.x, cPos.y + 0.50f, cPos.z }, 1.95f, 0.70f, 4.2f, car.bodyColor);
    DrawCubeWires((Vector3){ cPos.x, cPos.y + 0.50f, cPos.z }, 1.96f, 0.71f, 4.21f, (Color){ 25, 22, 20, 255 });
    DrawCube((Vector3){ cPos.x, cPos.y + 1.05f, cPos.z - 0.25f }, 1.70f, 0.58f, 2.3f, (Color){ 22, 24, 28, 255 });
    DrawCube((Vector3){ cPos.x, cPos.y + 1.02f, cPos.z + 0.92f }, 1.62f, 0.48f, 0.06f, (Color){ 65, 80, 95, 220 });
    DrawCube((Vector3){ cPos.x, cPos.y + 0.40f, cPos.z + 2.12f }, 1.85f, 0.28f, 0.12f, (Color){ 160, 162, 168, 255 });
    DrawCube((Vector3){ cPos.x - 0.65f, cPos.y + 0.48f, cPos.z + 2.14f }, 0.24f, 0.16f, 0.04f, (Color){ 255, 245, 170, 255 });
    DrawCube((Vector3){ cPos.x + 0.65f, cPos.y + 0.48f, cPos.z + 2.14f }, 0.24f, 0.16f, 0.04f, (Color){ 255, 245, 170, 255 });
    DrawCube((Vector3){ cPos.x - 0.70f, cPos.y + 0.52f, cPos.z - 2.12f }, 0.22f, 0.14f, 0.04f, (Color){ 225, 30, 25, 255 });
    DrawCube((Vector3){ cPos.x + 0.70f, cPos.y + 0.52f, cPos.z - 2.12f }, 0.22f, 0.14f, 0.04f, (Color){ 225, 30, 25, 255 });

    Vector3 flapPos = { cPos.x + 0.98f, cPos.y + 0.65f, cPos.z - 0.85f };
    DrawCube(flapPos, 0.03f, 0.18f, 0.18f, (Color){ 18, 18, 18, 255 });

    if (nozzleInCar) {
        float pz = (car.targetPump == 0) ? 137.5f : 142.5f;
        Vector3 pumpOutlet = { 127.42f, 11.2f, pz - 0.25f };
        // Seated cast-aluminum nozzle body in fuel neck
        DrawCube(flapPos, 0.09f, 0.09f, 0.15f, (Color){ 65, 70, 76, 255 });
        Vector3 nozzleCoupling = { flapPos.x + 0.08f, flapPos.y - 0.05f, flapPos.z };
        DrawCylinderEx(flapPos, nozzleCoupling, 0.024f, 0.022f, 8, (Color){ 140, 145, 155, 255 });
        // Realistic catenary heavy rubber hose draped smoothly to vehicle
        DrawCatenaryHose(pumpOutlet, nozzleCoupling, 0.85f, 16, 0.028f, (Color){ 16, 16, 18, 255 });
    }
}

#include "ui_helpers.h"

void UpdatePumpCrtTextureEx(RenderTexture2D rt, bool rtLoaded, Font fontSmall, Font fontTitle, int pumpNum,
                            float gallons, float salePrice, bool isFlowing, float fuelPricePerGallon,
                            float stationFuelGallons, float flk) {
    if (!rtLoaded) return;

    static int lastPumpNum = -1;
    static float lastGallons = -1.0f;
    static bool lastIsFlowing = false;
    static float lastFlk = -1.0f;
    static double lastFlowTime = 0.0;

    // Only update if state changes, or if flowing (for the bargraph animation) at 15 FPS
    bool needsUpdate = (pumpNum != lastPumpNum || gallons != lastGallons || isFlowing != lastIsFlowing || fabsf(flk - lastFlk) > 0.01f);
    if (isFlowing && (GetTime() - lastFlowTime > 0.06)) needsUpdate = true;
    if (!needsUpdate) return;

    lastPumpNum = pumpNum;
    lastGallons = gallons;
    lastIsFlowing = isFlowing;
    lastFlk = flk;
    if (isFlowing) lastFlowTime = GetTime();

    BeginTextureMode(rt);
    ClearBackground((Color){ 4, 18, 8, 255 }); // Dark retro emerald phosphorescent glass

    // Horizontal phosphor scanline raster grid
    for (int y = 0; y < 240; y += 4) {
        DrawRectangle(0, y, 320, 2, (Color){ 2, 10, 4, 115 });
    }

    // Header Bar with border
    DrawRectangle(10, 8, 300, 26, (Color){ 8, 36, 16, 235 });
    DrawRectangleLines(10, 8, 300, 26, (Color){ 35, 175, 75, 255 });
    DrawTextSharp(fontSmall, TextFormat("ROUTE 9 COOP // DISPENSER 0%d", pumpNum), 18, 14, 12.5f, (Color){ 80, 255, 120, 255 }, 1.2f);

    // Fuel Grade & Octane Badge
    DrawRectangle(10, 38, 140, 20, (Color){ 6, 26, 12, 225 });
    DrawRectangleLines(10, 38, 140, 20, (Color){ 25, 120, 50, 255 });
    DrawTextSharp(fontSmall, "OCTANE 87 REGULAR", 16, 42, 10.0f, (Color){ 100, 220, 130, 240 }, 1.0f);

    // Status Indicator Badge
    Color statusBg = isFlowing ? (Color){ 12, 65, 24, 255 } : (Color){ 45, 38, 12, 255 };
    Color statusFg = isFlowing ? (Color){ 80, 255, 120, 255 } : (Color){ 245, 200, 60, 255 };
    DrawRectangle(210, 38, 100, 20, statusBg);
    DrawRectangleLines(210, 38, 100, 20, statusFg);
    DrawTextSharp(fontSmall, isFlowing ? ">> FLOWING <<" : "[ STANDBY ]", 216, 42, 10.0f, statusFg, 1.0f);

    // Large Phosphor Digital Meter Displays
    // Box 1: THIS SALE ($)
    DrawRectangle(10, 64, 300, 44, (Color){ 6, 24, 12, 240 });
    DrawRectangleLines(10, 64, 300, 44, (Color){ 30, 150, 65, 255 });
    DrawTextSharp(fontSmall, "THIS SALE", 18, 70, 9.5f, (Color){ 90, 190, 115, 220 }, 1.1f);
    DrawTextSharp(fontTitle, TextFormat("$ %.2f", salePrice), 18, 83, 21.0f, (Color){ 50, (unsigned char)(255 * flk), 90, 255 }, 1.5f);

    // Box 2: GALLONS
    DrawRectangle(10, 114, 300, 44, (Color){ 6, 24, 12, 240 });
    DrawRectangleLines(10, 114, 300, 44, (Color){ 30, 150, 65, 255 });
    DrawTextSharp(fontSmall, "GALLONS", 18, 120, 9.5f, (Color){ 90, 190, 115, 220 }, 1.1f);
    DrawTextSharp(fontTitle, TextFormat("%.2f GAL", gallons), 18, 133, 21.0f, (Color){ 50, (unsigned char)(255 * flk), 90, 255 }, 1.5f);

    // Telemetry Footer
    DrawTextSharp(fontSmall, TextFormat("UNIT PRICE: $%.3f/GAL", fuelPricePerGallon), 14, 166, 10.5f, (Color){ 60, 175, 90, 230 }, 1.0f);
    DrawTextSharp(fontSmall, TextFormat("UNDERGROUND TANK: %.1f GAL", stationFuelGallons), 14, 184, 10.5f, (Color){ 60, 175, 90, 230 }, 1.0f);

    // Dynamic 16-Segment Flow Bargraph
    int barSegments = 16;
    int litSegments = isFlowing ? ((int)(GetTime() * 14.0f) % (barSegments + 1)) : 0;
    for (int b = 0; b < barSegments; b++) {
        Color bCol = (b < litSegments) ? (Color){ 65, 255, 110, 255 } : (Color){ 16, 52, 26, 210 };
        DrawRectangle(14 + b * 18, 206, 14, 14, bCol);
    }

    // CRT Edge Vignette & Corner Glass Glint
    DrawRectangleLinesEx((Rectangle){ 0, 0, 320, 240 }, 4.0f, (Color){ 2, 8, 3, 255 });
    DrawLine(10, 10, 65, 10, (Color){ 180, 255, 200, 75 });

    EndTextureMode();
}

void DrawPumpCrtScreen3D(Vector3 center, float width, float height, Texture2D tex, bool faceWest, Color tint) {
    float hw = width * 0.5f;
    float hh = height * 0.5f;

    rlDisableBackfaceCulling();
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    if (faceWest) {
        // Quad facing -X (towards Left Lane)
        rlNormal3f(-1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(center.x, center.y - hh, center.z + hw); // Bottom-left
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(center.x, center.y - hh, center.z - hw); // Bottom-right
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(center.x, center.y + hh, center.z - hw); // Top-right
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(center.x, center.y + hh, center.z + hw); // Top-left
    } else {
        // Quad facing +X (towards Right Lane)
        rlNormal3f(1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(center.x, center.y - hh, center.z - hw); // Bottom-left
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(center.x, center.y - hh, center.z + hw); // Bottom-right
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(center.x, center.y + hh, center.z + hw); // Top-right
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(center.x, center.y + hh, center.z - hw); // Top-left
    }

    rlEnd();
    rlSetTexture(0);
    rlEnableBackfaceCulling();
}

