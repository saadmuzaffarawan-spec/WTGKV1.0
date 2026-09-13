#pragma once
#include <raylib.h>
#include <raymath.h>
#include <math.h>
#include <vector>

void DrawFirstPersonDashboard(float iCarSpd, float iShakeDec, int iJerkCount, float timeVal, float panX = 0.0f, bool isSisterPOV = false) {

    // -------------------------------------------------------------

    // COCKPIT WINDSHIELD FRAME (A-Pillars and Top Roof Visor)

    // -------------------------------------------------------------

    Color frameColor = { 12, 14, 18, 255 };

    Color frameTrim  = { 30, 36, 46, 255 };



    // Left A-Pillar (Slanted with subtle parallax)

    int apOff = (int)(panX * 0.15f);

    DrawTriangle({ (float)(-apOff), 0 }, { (float)(-apOff), (float)LOGICAL_H }, { (float)(65 - apOff), (float)LOGICAL_H }, frameColor);

    DrawTriangle({ (float)(-apOff), 0 }, { (float)(65 - apOff), (float)LOGICAL_H }, { (float)(32 - apOff), 0 }, frameColor);

    DrawLineEx({ (float)(32 - apOff), 0 }, { (float)(65 - apOff), (float)LOGICAL_H }, 2.0f, frameTrim);



    // Right A-Pillar (Slanted with subtle parallax)

    DrawTriangle({ (float)LOGICAL_W - apOff, 0 }, { (float)LOGICAL_W - 65 - apOff, (float)LOGICAL_H }, { (float)LOGICAL_W - apOff, (float)LOGICAL_H }, frameColor);

    DrawTriangle({ (float)LOGICAL_W - apOff, 0 }, { (float)LOGICAL_W - 32 - apOff, 0 }, { (float)LOGICAL_W - 65 - apOff, (float)LOGICAL_H }, frameColor);

    DrawLineEx({ (float)LOGICAL_W - 32 - apOff, 0 }, { (float)LOGICAL_W - 65 - apOff, (float)LOGICAL_H }, 2.0f, frameTrim);



    // Top Roof Visor

    DrawRectangle(0, 0, LOGICAL_W, 24, frameColor);

    DrawLineEx({ 0, 24 }, { (float)LOGICAL_W, 24 }, 2.0f, frameTrim);



    // -------------------------------------------------------------

    // CURVED DASHBOARD MAIN PANEL (ALWAYS firmly anchored at bottom)

    // -------------------------------------------------------------

    int dashBaseY = 550;

    DrawRectangle(0, dashBaseY, LOGICAL_W, LOGICAL_H - dashBaseY, { 15, 17, 21, 255 });



    // The Curved Instrument Binnacle / Cowl (Pans with head rotation)

    float cowlCenterX = 420.0f - panX;

    DrawCircleSector({ cowlCenterX, 550 }, 240, 180, 360, 48, { 20, 23, 28, 255 });

    DrawCircleSectorLines({ cowlCenterX, 550 }, 240, 180, 360, 48, { 42, 50, 62, 255 });



    // Horizontal dash division accent lines

    DrawLineEx({ 0, (float)dashBaseY }, { (float)LOGICAL_W, (float)dashBaseY }, 2.5f, { 38, 46, 58, 255 });



    // Instrument Cluster Inset Housing

    float clustX = 215.0f - panX;

    DrawRectangleRounded({ clustX, 465, 430, 220 }, 0.18f, 8, { 9, 11, 14, 255 });

    DrawRectangleRoundedLines({ clustX, 465, 430, 220 }, 0.18f, 8, { 46, 55, 68, 255 });



    // Screw / Rivet glyphs on cluster corners

    DrawText("+", (int)clustX + 11, 474, 13, { 70, 82, 98, 220 });

    DrawText("+", (int)clustX + 417, 474, 13, { 70, 82, 98, 220 });

    DrawText("+", (int)clustX + 11, 670, 13, { 70, 82, 98, 220 });

    DrawText("+", (int)clustX + 417, 670, 13, { 70, 82, 98, 220 });



    // -------------------------------------------------------------

    // 1. SPEEDOMETER (Curved dial with glyphs and dynamic needle)

    // -------------------------------------------------------------

    Vector2 spdCenter = { 335.0f - panX, 575.0f };

    float spdRadius = 78.0f;



    DrawCircleV(spdCenter, spdRadius + 3.0f, { 28, 33, 42, 255 });

    DrawCircleV(spdCenter, spdRadius, { 13, 15, 18, 255 });

    DrawCircleLines((int)spdCenter.x, (int)spdCenter.y, spdRadius, { 52, 62, 78, 255 });



    const char* spdMarks[] = { "0", "20", "40", "60", "80", "100" };

    for (int i = 0; i <= 10; i++) {

        float f = (float)i / 10.0f;

        float angDeg = 135.0f + f * 270.0f;

        float angRad = angDeg * DEG2RAD;



        float rIn = (i % 2 == 0) ? (spdRadius - 13.0f) : (spdRadius - 7.0f);

        float rOut = spdRadius - 2.0f;

        Vector2 p1 = { spdCenter.x + cosf(angRad) * rIn, spdCenter.y + sinf(angRad) * rIn };

        Vector2 p2 = { spdCenter.x + cosf(angRad) * rOut, spdCenter.y + sinf(angRad) * rOut };



        Color tickCol = (i >= 8) ? (Color){ 235, 75, 75, 255 } : (Color){ 185, 200, 218, 220 };

        DrawLineEx(p1, p2, (i % 2 == 0) ? 2.0f : 1.0f, tickCol);



        if (i % 2 == 0) {

            int idx = i / 2;

            float rTxt = spdRadius - 22.0f;

            int tx = (int)(spdCenter.x + cosf(angRad) * rTxt) - 5;

            int ty = (int)(spdCenter.y + sinf(angRad) * rTxt) - 5;

            DrawText(spdMarks[idx], tx, ty, 10, { 170, 185, 205, 240 });

        }

    }



    DrawText("MPH", (int)spdCenter.x - 11, (int)spdCenter.y + 22, 10, { 120, 138, 160, 220 });



    // Odometer box with glyphs

    DrawRectangle((int)spdCenter.x - 28, (int)spdCenter.y + 38, 56, 14, { 6, 8, 10, 255 });

    DrawRectangleLines((int)spdCenter.x - 28, (int)spdCenter.y + 38, 56, 14, { 40, 48, 62, 255 });

    DrawText("[04821]", (int)spdCenter.x - 25, (int)spdCenter.y + 40, 10, { 175, 220, 175, 240 });



    // Dynamic Speedometer Needle

    float speedMph = (iCarSpd / 12.0f) * 65.0f;

    if (iShakeDec > 0.05f) {

        speedMph += sinf(timeVal * 45.0f) * iShakeDec * 4.0f;

    }

    if (speedMph < 0.0f) speedMph = 0.0f;

    float needleFrac = speedMph / 100.0f;

    if (needleFrac > 1.0f) needleFrac = 1.0f;

    float needleRad = (135.0f + needleFrac * 270.0f) * DEG2RAD;



    Vector2 needleTip = { spdCenter.x + cosf(needleRad) * (spdRadius - 8.0f),

                          spdCenter.y + sinf(needleRad) * (spdRadius - 8.0f) };

    DrawLineEx(spdCenter, needleTip, 2.5f, { 255, 72, 60, 255 });

    DrawCircleV(spdCenter, 6.0f, { 42, 50, 64, 255 });

    DrawCircleV(spdCenter, 3.0f, { 220, 230, 245, 255 });



    // -------------------------------------------------------------

    // 2. FUEL GAUGE (Curved dial with glyphs and warning indicator)

    // -------------------------------------------------------------

    Vector2 fuelCenter = { 525.0f - panX, 575.0f };

    float fuelRadius = 55.0f;



    DrawCircleV(fuelCenter, fuelRadius + 3.0f, { 28, 33, 42, 255 });

    DrawCircleV(fuelCenter, fuelRadius, { 13, 15, 18, 255 });

    DrawCircleLines((int)fuelCenter.x, (int)fuelCenter.y, fuelRadius, { 52, 62, 78, 255 });



    for (int i = 0; i <= 6; i++) {

        float f = (float)i / 6.0f;

        float angRad = (150.0f + f * 180.0f) * DEG2RAD;

        float rIn = fuelRadius - 9.0f;

        float rOut = fuelRadius - 2.0f;

        Vector2 p1 = { fuelCenter.x + cosf(angRad) * rIn, fuelCenter.y + sinf(angRad) * rIn };

        Vector2 p2 = { fuelCenter.x + cosf(angRad) * rOut, fuelCenter.y + sinf(angRad) * rOut };

        Color tc = (i <= 1) ? (Color){ 240, 75, 75, 255 } : (Color){ 180, 195, 212, 220 };

        DrawLineEx(p1, p2, 1.5f, tc);

    }



    DrawText("E", (int)fuelCenter.x - 40, (int)fuelCenter.y + 10, 13, { 245, 80, 80, 255 });

    DrawText("1/2", (int)fuelCenter.x - 9, (int)fuelCenter.y - 41, 10, { 170, 185, 205, 220 });

    DrawText("F", (int)fuelCenter.x + 30, (int)fuelCenter.y + 10, 13, { 95, 215, 120, 255 });

    DrawText("FUEL", (int)fuelCenter.x - 13, (int)fuelCenter.y + 18, 10, { 120, 138, 160, 220 });



    float fuelVal = 0.22f + sinf(timeVal * 2.0f) * 0.02f;

    if (iJerkCount >= 2) fuelVal = 0.07f;

    float fNeedleRad = (150.0f + fuelVal * 180.0f) * DEG2RAD;

    Vector2 fTip = { fuelCenter.x + cosf(fNeedleRad) * (fuelRadius - 7.0f),

                     fuelCenter.y + sinf(fNeedleRad) * (fuelRadius - 7.0f) };

    DrawLineEx(fuelCenter, fTip, 2.0f, { 255, 160, 50, 255 });

    DrawCircleV(fuelCenter, 5.0f, { 42, 50, 64, 255 });



    float fPulse = sinf(timeVal * 4.0f) * 0.5f + 0.5f;

    Color warnCol = (fuelVal <= 0.15f) ? (Color){ 255, 60, 60, (unsigned char)(160 + fPulse * 95) } 

                                       : (Color){ 240, 160, 40, (unsigned char)(140 + fPulse * 80) };

    DrawText("[!]", (int)fuelCenter.x - 7, (int)fuelCenter.y - 18, 13, warnCol);



    // -------------------------------------------------------------

    // 3. AUXILIARY SWITCHES PANEL (Curved panel with ASCII glyphs)

    // -------------------------------------------------------------

    int swX = (int)(720.0f - panX), swY = 555, swW = 460, swH = 135;

    DrawRectangleRounded({ (float)swX, (float)swY, (float)swW, (float)swH }, 0.16f, 6, { 11, 13, 17, 255 });

    DrawRectangleRoundedLines({ (float)swX, (float)swY, (float)swW, (float)swH }, 0.16f, 6, { 42, 52, 66, 255 });



    DrawText("- AUXILIARY CONTROLS -", swX + swW / 2 - 84, swY + 10, 11, { 130, 145, 170, 230 });

    DrawLine(swX + 16, swY + 26, swX + swW - 16, swY + 26, { 32, 40, 52, 255 });



    struct SwitchItem {

        const char* label;

        const char* glyph;

        const char* state;

        Color col;

    };



    bool isHazard = (iJerkCount > 0);

    float blinkA = (sinf(timeVal * 7.0f) > 0.0f) ? 1.0f : 0.2f;



    SwitchItem switches[4] = {

        { "LIGHTS",  "[/]", "ON",   { 110, 235, 140, 255 } },

        { "WIPERS",  "[~]", "HI",   { 100, 215, 255, 255 } },

        { "HAZARD",  "[!]", isHazard ? "WARN" : "OFF", isHazard ? (Color){ 255, 60, 60, (unsigned char)(blinkA * 255) } : (Color){ 90, 102, 118, 200 } },

        { "DEFROST", "[#]", "MAX",  { 255, 180, 65, 255 } }

    };



    int spacing = (swW - 32) / 4;

    for (int s = 0; s < 4; s++) {

        int sx = swX + 16 + s * spacing;

        int sy = swY + 36;



        DrawRectangle(sx, sy, 84, 76, { 7, 8, 11, 255 });

        DrawRectangleLines(sx, sy, 84, 76, { 32, 40, 52, 255 });



        int lw = MeasureText(switches[s].label, 10);

        DrawText(switches[s].label, sx + 42 - lw / 2, sy + 6, 10, { 150, 165, 185, 230 });



        int gw = MeasureText(switches[s].glyph, 18);

        DrawText(switches[s].glyph, sx + 42 - gw / 2, sy + 25, 18, switches[s].col);



        int stw = MeasureText(switches[s].state, 10);

        DrawText(switches[s].state, sx + 42 - stw / 2, sy + 54, 10, switches[s].col);

    }



    // Passenger dash detail (Glovebox) visible when sitting on passenger side

    if (panX > 80.0f) {

        int gbX = swX + swW + 30;

        DrawRectangleRounded({ (float)gbX, 560, 280, 120 }, 0.14f, 6, { 11, 13, 17, 255 });

        DrawRectangleRoundedLines({ (float)gbX, 560, 280, 120 }, 0.14f, 6, { 35, 42, 54, 255 });

        DrawText("[ GLOVE COMPARTMENT ]", gbX + 48, 600, 12, { 90, 105, 125, 220 });

        DrawRectangle(gbX + 115, 625, 50, 6, { 45, 52, 65, 255 }); // Handle

    }



    // -------------------------------------------------------------

    // 4. STEERING WHEEL RIM & SPOKES (Gentle rocking in front of driver)

    // -------------------------------------------------------------

    Vector2 strCenter = { 385.0f - panX, 710.0f };

    float strTurn = sinf(timeVal * 1.8f) * 10.0f;

    if (iShakeDec > 0.05f) strTurn += sinf(timeVal * 40.0f) * iShakeDec * 5.0f;



    DrawRing(strCenter, 148.0f, 172.0f, 190.0f + strTurn, 350.0f + strTurn, 36, { 22, 24, 28, 255 });

    DrawRingLines(strCenter, 148.0f, 172.0f, 190.0f + strTurn, 350.0f + strTurn, 36, { 48, 56, 70, 255 });



    Vector2 spkL = { strCenter.x + cosf((225.0f + strTurn) * DEG2RAD) * 150.0f,

                     strCenter.y + sinf((225.0f + strTurn) * DEG2RAD) * 150.0f };

    Vector2 spkR = { strCenter.x + cosf((315.0f + strTurn) * DEG2RAD) * 150.0f,

                     strCenter.y + sinf((315.0f + strTurn) * DEG2RAD) * 150.0f };

    DrawLineEx(strCenter, spkL, 8.0f, { 30, 34, 42, 255 });

    DrawLineEx(strCenter, spkR, 8.0f, { 30, 34, 42, 255 });

    DrawCircleV(strCenter, 32.0f, { 26, 30, 36, 255 });

    DrawCircleLines((int)strCenter.x, (int)strCenter.y, 32.0f, { 52, 60, 74, 255 });

    DrawText("@", (int)strCenter.x - 6, (int)strCenter.y - 10, 18, { 160, 180, 205, 220 });



    // In Sister's POV, render brother @ sitting in the driver's seat behind the steering wheel

    if (isSisterPOV) {

        int brX = (int)strCenter.x;

        int brY = 475;

        // Brother head '@'

        DrawText("@", brX - 14, brY, 36, { 255, 220, 160, 255 });

        // Subtle driving hands on the wheel

        DrawText("o", (int)spkL.x - 6, (int)spkL.y - 6, 20, { 245, 205, 145, 230 });

        DrawText("o", (int)spkR.x - 6, (int)spkR.y - 6, 20, { 245, 205, 145, 230 });

    }

}





void Draw3DCarInterior(float iCarX, float iCarY, float iCarZ, float ccy, float timeVal, float iCarSpd, float iShakeDec, int iJerkCount,

                       Mesh mSeat, Mesh mSeatBack, Mesh mSteerRim, Material matSeat, Material matCabin) {

    // ---- SEATS ----

    // Driver Seat (Left side)

    DrawMesh(mSeat, matSeat, MatrixTranslate(iCarX - 0.48f, ccy - 0.12f, iCarZ + 0.08f));

    DrawMesh(mSeatBack, matSeat, MatrixTranslate(iCarX - 0.48f, ccy + 0.22f, iCarZ - 0.30f));



    // Passenger Seat (Sister side, Right side)

    DrawMesh(mSeat, matSeat, MatrixTranslate(iCarX + 0.48f, ccy - 0.12f, iCarZ + 0.08f));

    DrawMesh(mSeatBack, matSeat, MatrixTranslate(iCarX + 0.48f, ccy + 0.22f, iCarZ - 0.30f));



    // ---- CENTER CONSOLE & TRANSMISSION TUNNEL ----

    DrawCube({ iCarX, ccy - 0.12f, iCarZ + 0.38f }, 0.26f, 0.22f, 1.10f, { 22, 25, 30, 255 });

    

    // Shifter stick & knob

    DrawLine3D({ iCarX, ccy - 0.01f, iCarZ + 0.52f }, { iCarX, ccy + 0.18f, iCarZ + 0.48f }, { 170, 175, 185, 255 });

    DrawSphere({ iCarX, ccy + 0.18f, iCarZ + 0.48f }, 0.032f, { 28, 30, 35, 255 });



    // ---- DASHBOARD BEAM & CURVED COWL ----

    // Main dashboard base spanning cabin width

    DrawCube({ iCarX, ccy + 0.20f, iCarZ + 1.05f }, 1.95f, 0.28f, 0.40f, { 16, 18, 22, 255 });



    // Raised Curved Binnacle / Instrument Cowl over driver's seat

    DrawCube({ iCarX - 0.48f, ccy + 0.38f, iCarZ + 0.98f }, 0.68f, 0.12f, 0.32f, { 22, 25, 32, 255 });

    DrawLine3D({ iCarX - 0.82f, ccy + 0.44f, iCarZ + 0.82f }, { iCarX - 0.14f, ccy + 0.44f, iCarZ + 0.82f }, { 55, 68, 85, 255 });



    // Passenger dash glovebox seam

    DrawLine3D({ iCarX + 0.18f, ccy + 0.16f, iCarZ + 0.85f }, { iCarX + 0.85f, ccy + 0.16f, iCarZ + 0.85f }, { 35, 40, 48, 255 });



    // ---- 3D INSTRUMENTS (Speedometer & Fuel Gauge on Driver Cowl) ----

    // Speedometer dial background

    Vector3 spd3D = { iCarX - 0.58f, ccy + 0.30f, iCarZ + 0.84f };

    DrawCube(spd3D, 0.16f, 0.16f, 0.02f, { 8, 10, 14, 255 });

    // Speedometer ticks

    for (int t = 0; t < 8; t++) {

        float ta = (135.0f + t * 34.0f) * DEG2RAD;

        Vector3 tp1 = { spd3D.x + cosf(ta) * 0.050f, spd3D.y + sinf(ta) * 0.050f, spd3D.z - 0.012f };

        Vector3 tp2 = { spd3D.x + cosf(ta) * 0.068f, spd3D.y + sinf(ta) * 0.068f, spd3D.z - 0.012f };

        DrawLine3D(tp1, tp2, (t >= 6) ? (Color){ 230, 70, 70, 255 } : (Color){ 180, 195, 215, 240 });

    }

    // Speedometer dynamic needle

    float spdFrac = (iCarSpd / 12.0f);

    if (iShakeDec > 0.05f) spdFrac += sinf(timeVal * 40.0f) * iShakeDec * 0.05f;

    if (spdFrac < 0.0f) spdFrac = 0.0f;

    if (spdFrac > 1.0f) spdFrac = 1.0f;

    float spdNeedleAng = (135.0f + spdFrac * 270.0f) * DEG2RAD;

    DrawLine3D(spd3D, { spd3D.x + cosf(spdNeedleAng) * 0.058f, spd3D.y + sinf(spdNeedleAng) * 0.058f, spd3D.z - 0.015f }, { 255, 75, 65, 255 });



    // Fuel Gauge dial background

    Vector3 fuel3D = { iCarX - 0.38f, ccy + 0.30f, iCarZ + 0.84f };

    DrawCube(fuel3D, 0.12f, 0.12f, 0.02f, { 8, 10, 14, 255 });

    // Fuel gauge ticks

    for (int t = 0; t <= 4; t++) {

        float fa = (150.0f + t * 45.0f) * DEG2RAD;

        DrawLine3D({ fuel3D.x + cosf(fa) * 0.035f, fuel3D.y + sinf(fa) * 0.035f, fuel3D.z - 0.012f },

                   { fuel3D.x + cosf(fa) * 0.050f, fuel3D.y + sinf(fa) * 0.050f, fuel3D.z - 0.012f },

                   (t == 0) ? (Color){ 240, 70, 70, 255 } : (Color){ 180, 195, 215, 240 });

    }

    // Fuel needle (low fuel)

    float fVal = (iJerkCount >= 2) ? 0.08f : 0.22f;

    float fAng = (150.0f + fVal * 180.0f) * DEG2RAD;

    DrawLine3D(fuel3D, { fuel3D.x + cosf(fAng) * 0.042f, fuel3D.y + sinf(fAng) * 0.042f, fuel3D.z - 0.015f }, { 255, 160, 50, 255 });



    // ---- 3D AUXILIARY SWITCHES (Center stack) ----

    Vector3 swPanel = { iCarX, ccy + 0.20f, iCarZ + 0.84f };

    DrawCube(swPanel, 0.28f, 0.14f, 0.02f, { 12, 14, 18, 255 });

    DrawLine3D({ swPanel.x - 0.12f, swPanel.y + 0.05f, swPanel.z - 0.012f },

               { swPanel.x + 0.12f, swPanel.y + 0.05f, swPanel.z - 0.012f }, { 45, 52, 65, 255 });



    // 4 glowing indicator switches

    bool isHaz = (iJerkCount > 0);

    float hazBlink = (sinf(timeVal * 7.0f) > 0.0f) ? 1.0f : 0.2f;

    Color swCols[4] = {

        { 110, 235, 140, 255 }, // Lights (Green)

        { 100, 215, 255, 255 }, // Wipers (Cyan)

        isHaz ? (Color){ 255, 60, 60, (unsigned char)(hazBlink * 255) } : (Color){ 80, 90, 105, 200 }, // Hazard

        { 255, 180, 65, 255 }  // Defrost (Amber)

    };

    for (int s = 0; s < 4; s++) {

        float sx = swPanel.x - 0.09f + s * 0.06f;

        DrawCube({ sx, swPanel.y - 0.01f, swPanel.z - 0.015f }, 0.038f, 0.045f, 0.01f, { 25, 28, 35, 255 });

        DrawSphere({ sx, swPanel.y - 0.01f, swPanel.z - 0.022f }, 0.010f, swCols[s]);

    }



    // ---- STEERING COLUMN & STEERING WHEEL ----

    Vector3 colStart = { iCarX - 0.48f, ccy + 0.22f, iCarZ + 0.85f };

    Vector3 swCenter = { iCarX - 0.48f, ccy + 0.30f, iCarZ + 0.68f };

    DrawLine3D(colStart, swCenter, { 35, 40, 50, 255 });

    DrawCube({ colStart.x, (colStart.y + swCenter.y)*0.5f, (colStart.z + swCenter.z)*0.5f }, 0.08f, 0.08f, 0.16f, { 25, 28, 36, 255 });



    float swTurn = sinf(timeVal * 1.8f) * 12.0f * DEG2RAD;

    if (iShakeDec > 0.05f) swTurn += sinf(timeVal * 40.0f) * iShakeDec * 0.08f;



    // Steering wheel rim (cylinder rotated toward driver)

    Matrix sMat = MatrixRotateZ(90.0f * DEG2RAD);

    sMat = MatrixMultiply(sMat, MatrixRotateY(swTurn));

    sMat = MatrixMultiply(sMat, MatrixTranslate(swCenter.x, swCenter.y, swCenter.z));

    DrawMesh(mSteerRim, matCabin, sMat);



    // Spokes connecting hub to rim

    for (int sp = 0; sp < 3; sp++) {

        float sa = swTurn + (sp / 3.0f) * 2.0f * PI;

        Vector3 rimPt = { swCenter.x + cosf(sa) * 0.24f, swCenter.y + sinf(sa) * 0.24f, swCenter.z };

        DrawLine3D(swCenter, rimPt, { 48, 55, 68, 255 });

    }

    DrawSphere(swCenter, 0.042f, { 30, 35, 45, 255 });



    // Windshield A-Pillars framing the cabin

    DrawLine3D({ iCarX - 0.98f, ccy + 0.38f, iCarZ + 1.15f }, { iCarX - 0.82f, ccy + 0.88f, iCarZ + 0.85f }, { 40, 48, 60, 255 });

    DrawLine3D({ iCarX + 0.98f, ccy + 0.38f, iCarZ + 1.15f }, { iCarX + 0.82f, ccy + 0.88f, iCarZ + 0.85f }, { 40, 48, 60, 255 });

    DrawLine3D({ iCarX - 0.82f, ccy + 0.88f, iCarZ + 0.85f }, { iCarX + 0.82f, ccy + 0.88f, iCarZ + 0.85f }, { 40, 48, 60, 255 });

}
