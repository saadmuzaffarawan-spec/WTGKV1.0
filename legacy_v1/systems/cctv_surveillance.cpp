#include "cctv_surveillance.h"
#include "ui_helpers.h"
#include "../core/game_types.h"
#include <cstdio>
#include <cmath>

// State Definitions
bool  g_isRoofCamActive  = false;
float g_roofCamSlideX    = 0.0f;   // Sliding along front roof wall rail (-3.2 to +3.2)
float g_roofCamPitch     = -14.0f; // Tilt angle (-65 to +28 deg)
float g_roofCamYaw       = 0.0f;   // Pan angle (-85 to +85 deg)
float g_roofCamFOV       = 65.0f;  // Optical zoom (15 to 80 deg)

int   g_menuCCTVFeed     = 0; // 0: Portico Arch, 1: Courtyard, 2: Vestibule
float g_cctvSwitchGlitch = 0.0f;

void UpdateRoofCCTV(float dt, float timeVal, bool isCursorCaptured, bool showQuitConfirm, bool isShopOpen) {
    if (IsKeyPressed(KEY_C) && !isShopOpen && !showQuitConfirm) {
        g_isRoofCamActive = !g_isRoofCamActive;
    }

    // FRONT ROOF WALL CCTV CONTROLS: Sliders along the front roof edge wall
    if (g_isRoofCamActive && !showQuitConfirm) {
        // Slide along the front roof edge rail with A / D
        if (IsKeyDown(KEY_A)) g_roofCamSlideX = Clamp(g_roofCamSlideX - dt * 4.5f, -3.2f, 3.2f);
        if (IsKeyDown(KEY_D)) g_roofCamSlideX = Clamp(g_roofCamSlideX + dt * 4.5f, -3.2f, 3.2f);

        // Strong Optical Zooming: Mouse Wheel or W / S
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) g_roofCamFOV = Clamp(g_roofCamFOV - wheel * 4.5f, 15.0f, 80.0f);
        if (IsKeyDown(KEY_W)) g_roofCamFOV = Clamp(g_roofCamFOV - dt * 28.0f, 15.0f, 80.0f);
        if (IsKeyDown(KEY_S)) g_roofCamFOV = Clamp(g_roofCamFOV + dt * 28.0f, 15.0f, 80.0f);

        // Mouse Look: Pan & Tilt (Only when cursor is captured)
        if (isCursorCaptured) {
            Vector2 mDelta = GetMouseDelta();
            g_roofCamYaw   = Clamp(g_roofCamYaw - mDelta.x * 0.22f, -85.0f, 85.0f);
            g_roofCamPitch = Clamp(g_roofCamPitch - mDelta.y * 0.22f, -65.0f, 28.0f);
        }

        if (IsKeyDown(KEY_LEFT))  g_roofCamYaw   = Clamp(g_roofCamYaw + dt * 45.0f, -85.0f, 85.0f);
        if (IsKeyDown(KEY_RIGHT)) g_roofCamYaw   = Clamp(g_roofCamYaw - dt * 45.0f, -85.0f, 85.0f);
        if (IsKeyDown(KEY_UP))    g_roofCamPitch = Clamp(g_roofCamPitch + dt * 35.0f, -65.0f, 28.0f);
        if (IsKeyDown(KEY_DOWN))  g_roofCamPitch = Clamp(g_roofCamPitch - dt * 35.0f, -65.0f, 28.0f);
    } else {
        // Strange Autonomous Horror Behavior: Robotic CCTV box slowly creeps along the front roof wall!
        g_roofCamSlideX = sinf(timeVal * 0.8f) * 2.8f;
        g_roofCamPitch  = -15.0f + sinf(timeVal * 0.5f) * 4.0f;
        g_roofCamYaw    = sinf(timeVal * 0.9f) * 24.0f;
    }
}

void SetupRoofCCTVCamera(Camera3D &renderCam, const Camera3D &playerCam, std::vector<Matrix> playerInstances[256]) {
    // CCTV SURVEILLANCE CAMERA VIEW: Clean viewpoint placed forward of roof edge so you see the pure world (no camera parts clip into screen!)
    Vector3 camPos = { 128.0f + g_roofCamSlideX, 15.35f, 133.05f };
    renderCam.position = camPos;

    // Compute look target facing South toward oncoming road + user pan/tilt
    float radPitch = g_roofCamPitch * DEG2RAD;
    float radYaw   = (180.0f + g_roofCamYaw) * DEG2RAD;
    Vector3 forwardDir = { sinf(radYaw) * cosf(radPitch), sinf(radPitch), cosf(radYaw) * cosf(radPitch) };
    renderCam.target = Vector3Add(camPos, forwardDir);
    renderCam.up = Vector3{ 0.0f, 1.0f, 0.0f };
    renderCam.fovy = g_roofCamFOV;

    // Draw player '@' model visible from roof CCTV perspective (Bigger & 3D)
    Matrix mp = MatrixIdentity();
    mp.m0 = 0.2f; mp.m1 = 0.88f; mp.m2 = 0.35f; mp.m3 = 1.0f; // Terminal green
    mp.m4 = 1.45f; mp.m5 = 1.45f;
    mp.m8 = 0.0f; mp.m9 = 0.0f; mp.m10 = 0.0f; mp.m11 = 1.0f; // Billboard
    mp.m12 = playerCam.position.x;
    mp.m13 = playerCam.position.y - 0.7f;
    mp.m14 = playerCam.position.z;

    Vector3 camToCctvP = Vector3Normalize(Vector3Subtract(playerCam.position, renderCam.position));
    for (int slice = -2; slice <= 2; slice++) {
        Matrix sm = mp;
        sm.m12 += camToCctvP.x * (slice * 0.035f);
        sm.m13 += camToCctvP.y * (slice * 0.035f);
        sm.m14 += camToCctvP.z * (slice * 0.035f);
        float shade = (slice == 0) ? 1.0f : (1.0f - fabsf((float)slice) * 0.20f);
        sm.m0 *= shade; sm.m1 *= shade; sm.m2 *= shade;
        playerInstances['@'].push_back(sm);
    }
}

void DrawPhysicalCCTVCameraAssembly(float timeVal) {
    // 1. Standoff Brackets anchoring the rail forward from the front canopy fascia (Y = 15.22, Z = 133.25)
    float bracketXs[4] = { 124.6f, 126.8f, 129.2f, 131.4f };
    for (int b = 0; b < 4; b++) {
        DrawCube({ bracketXs[b], 15.22f, 133.25f }, 0.10f, 0.16f, 0.26f, { 25, 25, 28, 255 });
    }

    // 2. Continuous Front Roof Wall Steel Rail Tube (X: 124.5..131.5, length 7.0m, Y = 15.35, Z = 133.20)
    DrawCube({ 128.0f, 15.35f, 133.20f }, 7.0f, 0.08f, 0.08f, { 35, 36, 40, 255 });
    // End Stopper Caps
    DrawCube({ 124.45f, 15.35f, 133.20f }, 0.14f, 0.22f, 0.14f, { 20, 20, 24, 255 });
    DrawCube({ 131.55f, 15.35f, 133.20f }, 0.14f, 0.22f, 0.14f, { 20, 20, 24, 255 });

    // 3. PHYSICAL CCTV CAMERA ASSEMBLY (Appears fully when viewed from ground; hidden during CCTV mode so view is unobstructed!)
    if (!g_isRoofCamActive) {
        float curCamX = 128.0f + g_roofCamSlideX;

        // Sliding Motorized Carriage gripping the rail
        DrawCube({ curCamX, 15.35f, 133.20f }, 0.34f, 0.12f, 0.22f, { 26, 26, 30, 255 });

        // Articulated Swivel Gimbal Drop Bracket
        DrawCube({ curCamX, 15.42f, 133.16f }, 0.14f, 0.16f, 0.14f, { 42, 42, 46, 255 });

        // Dynamic rotation for camera body based on yaw/pitch
        float radYaw   = (180.0f + g_roofCamYaw) * DEG2RAD;
        float radPitch = g_roofCamPitch * DEG2RAD;
        Vector3 fwd = { sinf(radYaw) * cosf(radPitch), sinf(radPitch), cosf(radYaw) * cosf(radPitch) };
        Vector3 rgt = { fwd.z, 0.0f, -fwd.x };

        // Weatherproof Rectangular CCTV Box Housing (Crisp security off-white/beige)
        Vector3 boxPos = { curCamX + fwd.x * 0.08f, 15.48f + fwd.y * 0.08f, 133.10f + fwd.z * 0.08f };
        DrawCube(boxPos, 0.36f, 0.24f, 0.46f, { 226, 226, 230, 255 });
        DrawCubeWires(boxPos, 0.365f, 0.245f, 0.465f, { 90, 90, 95, 255 });

        // Matte Black Protective Sunshield / Rain Visor extending over top and front
        Vector3 visorPos = { boxPos.x + fwd.x * 0.05f, boxPos.y + 0.13f, boxPos.z + fwd.z * 0.05f };
        DrawCube(visorPos, 0.40f, 0.04f, 0.52f, { 35, 36, 40, 255 });

        // Protruding Dark Cylindrical Lens Barrel
        Vector3 lensPos = { boxPos.x + fwd.x * 0.24f, boxPos.y - 0.02f + fwd.y * 0.24f, boxPos.z + fwd.z * 0.24f };
        DrawSphere(lensPos, 0.085f, { 12, 14, 18, 255 });

        // Active Blinking Red Recording LED beside lens (● REC)
        bool recBlink = (fmodf(timeVal, 0.8f) < 0.4f);
        Vector3 ledPos = { lensPos.x + rgt.x * 0.12f, lensPos.y + 0.07f, lensPos.z + rgt.z * 0.12f };
        DrawSphere(ledPos, 0.035f, recBlink ? Color{ 255, 12, 12, 255 } : Color{ 70, 0, 0, 255 });
    }
}

void DrawCCTVSurveillanceOverlay(float timeVal) {
    if (!g_isRoofCamActive) return;

    for (int sl = 0; sl < LOGICAL_H; sl += 4) {
        DrawLine(0, sl, LOGICAL_W, sl, { 0, 0, 0, 45 });
    }
    DrawRectangle(0, 0, LOGICAL_W, LOGICAL_H, { 10, 25, 15, 25 });

    bool recBlink = (fmodf(timeVal, 0.8f) < 0.4f);
    if (recBlink) {
        DrawCircle(24, 24, 6, { 240, 20, 20, 255 });
    }
    DrawTextSharp(g_fontMenu, "REC", 36, 16, 15.0f, { 240, 20, 20, 255 });
    DrawTextSharp(g_fontMenu, "CAM 01 - FRONT ROOF WALL // SURVEILLANCE FEED [LIVE]", 85, 16, 15.0f, { 190, 240, 190, 245 });

    float zoomMag = 65.0f / g_roofCamFOV;
    char telemBuf[160];
    snprintf(telemBuf, sizeof(telemBuf), "RAIL POS: %+.1fm  |  ZOOM: %.1fx  |  PAN: %+.0f*  |  PITCH: %+.0f*",
             g_roofCamSlideX, zoomMag, g_roofCamYaw, g_roofCamPitch);
    DrawTextSharp(g_fontSmall, telemBuf, 85, 37, 12.0f, { 140, 220, 140, 230 });
    DrawTextSharp(g_fontSmall, "SURVEILLANCE ACTIVE", (float)(LOGICAL_W - 200), 17, 12.0f, { 120, 240, 120, 220 });

    int cw = 28, ch = 28, thick = 2;
    DrawRectangle(14, 14, cw, thick, { 100, 180, 100, 200 });
    DrawRectangle(14, 14, thick, ch, { 100, 180, 100, 200 });
    DrawRectangle(LOGICAL_W - 14 - cw, 14, cw, thick, { 100, 180, 100, 200 });
    DrawRectangle(LOGICAL_W - 14, 14, thick, ch, { 100, 180, 100, 200 });
    DrawRectangle(14, LOGICAL_H - 14 - thick, cw, thick, { 100, 180, 100, 200 });
    DrawRectangle(14, LOGICAL_H - 14 - ch, thick, ch, { 100, 180, 100, 200 });
    DrawRectangle(LOGICAL_W - 14 - cw, LOGICAL_H - 14 - thick, cw, thick, { 100, 180, 100, 200 });
    DrawRectangle(LOGICAL_W - 14, LOGICAL_H - 14 - ch, thick, ch, { 100, 180, 100, 200 });

    DrawLine(LOGICAL_W/2 - 14, LOGICAL_H/2, LOGICAL_W/2 + 14, LOGICAL_H/2, { 100, 180, 100, 160 });
    DrawLine(LOGICAL_W/2, LOGICAL_H/2 - 14, LOGICAL_W/2, LOGICAL_H/2 + 14, { 100, 180, 100, 160 });

    const char* cctvControls = "[A/D] SLIDE ROOF WALL   |   [MOUSE] PAN/TILT   |   [W/S/WHEEL] ZOOM   |   [C / ESC] EXIT";
    float cmw = MeasureTextSharp(g_fontSmall, cctvControls, 13.0f);
    DrawAAAPanel(Rectangle{ ((float)LOGICAL_W - cmw) * 0.5f - 16.0f, (float)LOGICAL_H - 42.0f, cmw + 32.0f, 28.0f }, Color{ 10, 16, 12, 235 }, Color{ 70, 165, 75, 220 }, 4.0f, true);
    DrawTextSharpCentered(g_fontSmall, cctvControls, (float)LOGICAL_W * 0.5f, (float)LOGICAL_H - 34.5f, 13.0f, { 185, 245, 185, 255 });
}

void UpdateMenuCCTV(float dt, float wheelMove, bool modalOpen, Sound sndStatic) {
    if (g_cctvSwitchGlitch > 0.0f) {
        g_cctvSwitchGlitch -= dt;
    }

    if (!modalOpen) {
        int camDelta = 0;
        if (IsKeyPressed(KEY_Q) || wheelMove < -0.2f) camDelta = 2; // (current + 2) % 3 is previous
        if (IsKeyPressed(KEY_E) || wheelMove > 0.2f)  camDelta = 1; // (current + 1) % 3 is next
        if (camDelta != 0) {
            g_menuCCTVFeed = (g_menuCCTVFeed + camDelta) % 3;
            PlaySound(sndStatic);
            g_cctvSwitchGlitch = 0.16f;
        }
    }
}

void DrawMenuCCTVOverlay(int screenW, int screenH, Vector2 mPos, float dt, Sound sndStatic) {
    (void)dt;
    // Feather-light CRT scanlines for analog texture
    for (int y = 0; y < screenH; y += 4) {
        DrawLine(0, y, screenW, y, Color{ 0, 0, 0, 16 });
    }

    // CCTV Camera Switch Glitch Overlay (video scanline flutter)
    if (g_cctvSwitchGlitch > 0.0f) {
        for (int n = 0; n < 8; n++) {
            int ly = GetRandomValue(10, screenH - 10);
            DrawLine(0, ly, screenW, ly, Color{ 200, 215, 235, (unsigned char)GetRandomValue(35, 80) });
        }
    }

    // Minimalist Floating Camera Switch Tabs at Screen Bottom
    DrawTextSharp(g_fontSmall, "[ Q / E ]  Vistas:", 45, (float)(screenH - 32), 13.0f, Color{ 150, 155, 165, 190 }, 1.2f);
    const char* camTabs[3] = { "Portico Arch", "Courtyard", "Vestibule" };
    int tabSpacing = 145;
    int tabStartX  = screenW - 470;
    for (int c = 0; c < 3; c++) {
        int tx = tabStartX + c * tabSpacing;
        int ty = screenH - 32;
        Rectangle tabHit = { (float)(tx - 10), (float)(screenH - 42), 130.0f, 32.0f };
        bool tHover = CheckCollisionPointRec(mPos, tabHit);
        bool tActive = (g_menuCCTVFeed == c);

        Color tabCol = tActive ? WHITE : (tHover ? Color{ 245, 130, 120, 255 } : Color{ 150, 145, 140, 190 });
        DrawTextSharp(g_fontSmall, camTabs[c], (float)tx, (float)ty, 13.0f, tabCol, 1.2f);
        if (tActive) {
            int tLen = (int)MeasureTextSharp(g_fontSmall, camTabs[c], 13.0f, 1.2f);
            DrawLine(tx, ty + 18, tx + tLen, ty + 18, Color{ 235, 45, 35, 255 });
        }

        if (tHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_menuCCTVFeed != c) {
            g_menuCCTVFeed = c;
            PlaySound(sndStatic);
            g_cctvSwitchGlitch = 0.16f;
        }
    }
}

void GetMenuCCTVCamera(int feedIndex, float timeVal, float smoothX, float smoothY, Vector3 &outPos, Vector3 &outTarget) {
    float breatheY = sinf(timeVal * 0.75f) * 0.035f;
    float breatheX = cosf(timeVal * 0.45f) * 0.045f;

    if (feedIndex == 0) {
        // CAM 01: Elevated perspective showcasing college facade & grand steps
        outPos = Vector3{ 141.6f - smoothX * 0.35f + breatheX * 0.4f, 13.85f + breatheY * 0.5f - smoothY * 0.20f, 136.2f };
        outTarget = Vector3{ 152.0f + smoothX * 1.0f, 14.50f - smoothY * 0.60f, 140.0f };
    } else if (feedIndex == 1) {
        // CAM 02: Courtyard & South Facade - high crane perspective across rain-swept grounds towards entrance
        outPos = Vector3{ 136.5f - smoothX * 0.55f + breatheX * 0.5f, 14.20f + breatheY * 0.4f - smoothY * 0.30f, 122.0f };
        outTarget = Vector3{ 155.0f + smoothX * 1.2f, 12.00f - smoothY * 0.70f, 138.0f };
    } else {
        // CAM 03: Portico Vestibule Looking Out - under the grand portico looking out between columns into stormy night
        outPos = Vector3{ 152.2f - smoothX * 0.25f + breatheX * 0.3f, 11.80f + breatheY * 0.5f - smoothY * 0.15f, 140.0f };
        outTarget = Vector3{ 136.0f + smoothX * 1.1f, 11.20f - smoothY * 0.50f, 137.0f };
    }
}

void DrawMiniCRTSurveillanceMonitor(Vector3 crtPos, float timeVal, std::function<Color(Vector3, Color)> applyLighting) {
    DrawCube(crtPos, 0.38f, 0.34f, 0.36f, applyLighting(crtPos, { 30, 32, 34, 255 }));
    DrawCube({ crtPos.x, crtPos.y, crtPos.z + 0.19f }, 0.28f, 0.25f, 0.03f, { 22, 190, 65, 240 });

    float scanY = crtPos.y - 0.10f + fmodf(timeVal * 0.35f, 0.20f);
    DrawLine3D({ crtPos.x - 0.13f, scanY, crtPos.z + 0.21f }, { crtPos.x + 0.13f, scanY, crtPos.z + 0.21f }, { 140, 255, 170, 220 });

    bool crtBlink = (fmodf(timeVal, 0.8f) < 0.4f);
    DrawSphere({ crtPos.x + 0.13f, crtPos.y + 0.13f, crtPos.z + 0.20f }, 0.022f, crtBlink ? Color{ 255, 20, 20, 255 } : Color{ 80, 0, 0, 255 });
}

