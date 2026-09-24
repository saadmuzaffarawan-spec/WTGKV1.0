#define _CRT_SECURE_NO_WARNINGS
#include "systems/main_menu_system.h"
#include "raymath.h"
#include "systems/ui_helpers.h"
#include "systems/cctv_surveillance.h"
#include <cmath>

extern void RequestGameQuit();

MainMenuSystem::MainMenuSystem() {
    // Members are initialized in header
}

MainMenuSystem::~MainMenuSystem() {
}

void MainMenuSystem::Update(float dt) {
    Vector2 mPos = GetMousePosition();
    Vector2 menuMDelta = GetMouseDelta();
    float mouseMoveDist = sqrtf(menuMDelta.x * menuMDelta.x + menuMDelta.y * menuMDelta.y);
    float wheelMove = GetMouseWheelMove();
    bool anyUserInput = (mouseMoveDist > 0.6f) || (fabsf(wheelMove) > 0.05f) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || (GetKeyPressed() != 0);

    // Awakening from deep slumber
    if (anyUserInput) {
        if (m_awakeIntensity < 0.45f) {
            PlaySound(g_sndFlashlightToggle);
            g_cctvSwitchGlitch = 0.12f;
            m_lurkerEyeFlee = 1.0f; // startled lurkers flee
        }
        m_idleTimer = 0.0f;
    } else {
        m_idleTimer += dt;
    }

    // Smooth target awake intensity (1.0 -> 0.10 when idle > 2.2s)
    float targetAwake = 1.0f;
    if (m_idleTimer > 2.2f) {
        float slumberProg = Clamp((m_idleTimer - 2.2f) / 2.8f, 0.0f, 1.0f);
        targetAwake = Lerp(1.0f, 0.10f, slumberProg);
        float timeVal = (float)GetTime();
        if (slumberProg > 0.65f && fmodf(timeVal * 8.5f, 1.0f) < 0.10f) {
            targetAwake *= 0.55f; // battery flicker
        }
    }
    m_awakeIntensity += (targetAwake - m_awakeIntensity) * Clamp(dt * 3.8f, 0.0f, 1.0f);

    // Smooth volumetric flashlight beam tracking
    m_lightPos.x += (mPos.x - m_lightPos.x) * Clamp(dt * 18.0f, 0.0f, 1.0f);
    m_lightPos.y += (mPos.y - m_lightPos.y) * Clamp(dt * 18.0f, 0.0f, 1.0f);

    // Decay flee animation
    if (m_lurkerEyeFlee > 0.0f) {
        m_lurkerEyeFlee = fmaxf(0.0f, m_lurkerEyeFlee - dt * 3.0f);
    }

    // Lightning timer update
    m_menuLightningTimer -= dt;
    if (m_menuLightningTimer <= 0.0f) {
        m_menuLightningTimer = (float)GetRandomValue(16, 28);
        m_menuFlashAlpha = 0.38f;
        SetSoundVolume(g_sndThunder, 0.28f);
        PlaySound(g_sndThunder);
    }
    if (m_menuFlashAlpha > 0.0f) {
        m_menuFlashAlpha -= dt * 2.6f;
        if (m_menuFlashAlpha < 0.0f) m_menuFlashAlpha = 0.0f;
    }

    // CCTV update
    UpdateMenuCCTV(dt, wheelMove, m_showSettingsModal || m_showCaseFilesModal || m_showSurvivalModal || m_showDossierModal || m_isStartingGame, g_sndRadioStatic);

    // Menu Selection
    if (!m_isStartingGame) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            m_menuSelection = (m_menuSelection + 4) % 5;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            m_menuSelection = (m_menuSelection + 1) % 5;
        }
    }

    // Mouse Hover Logic
    int menuX = (int)(1280 * 0.085f); // LOGICAL_W = 1280
    int menuStartY = (int)(720 * 0.38f); // LOGICAL_H = 720
    int itemSpacing = 54;
    bool mouseOverAnyItem = false;

    const char* kMenuLabels[5] = { "Play", "Case Files", "Settings", "Controls", "Quit" };
    for (int i = 0; i < 5; i++) {
        int itemY = menuStartY + i * itemSpacing;
        float textLen = MeasureTextSharp(g_fontTitle, kMenuLabels[i], 36.0f, 1.5f);
        Rectangle hitRec = { (float)(menuX - 25), (float)(itemY - 4), textLen + 55.0f, 44.0f };
        bool hover = CheckCollisionPointRec(mPos, hitRec);
        if (hover) mouseOverAnyItem = true;

        if (hover && (fabsf(menuMDelta.x) > 0.2f || fabsf(menuMDelta.y) > 0.2f) && !m_isStartingGame) {
            m_menuSelection = i;
        }

        bool isSelected = (m_menuSelection == i);
        m_menuOptionHover[i] += ((isSelected ? 1.0f : 0.0f) - m_menuOptionHover[i]) * Clamp(dt * 14.0f, 0.0f, 1.0f);

        bool clicked = hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool enterPressed = isSelected && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE));

        if (!m_isStartingGame) {
            if (i == 0 && (clicked || enterPressed || IsKeyPressed(KEY_ONE))) {
                PlaySound(g_sndMenuBoom);
                m_isStartingGame = true;
                m_playTransitionTimer = 0.0f;
            } else if (i == 1 && (clicked || enterPressed || IsKeyPressed(KEY_TWO))) {
                PlaySound(g_sndMenuBoom);
                m_showCaseFilesModal = true;
            } else if (i == 2 && (clicked || enterPressed || IsKeyPressed(KEY_THREE))) {
                PlaySound(g_sndMenuBoom);
                m_showSettingsModal = true;
            } else if (i == 3 && (clicked || enterPressed || IsKeyPressed(KEY_FOUR))) {
                PlaySound(g_sndMenuBoom);
                m_showSurvivalModal = true;
            } else if (i == 4 && (clicked || enterPressed || IsKeyPressed(KEY_FIVE))) {
                PlaySound(g_sndMenuBoom);
                RequestGameQuit();
            }
        }
    }

    if (mouseOverAnyItem) {
        m_skullGazeFlare += (1.0f - m_skullGazeFlare) * Clamp(dt * 5.0f, 0.0f, 1.0f);
    } else {
        m_skullGazeFlare += (0.0f - m_skullGazeFlare) * Clamp(dt * 3.0f, 0.0f, 1.0f);
    }

    if (m_menuSelection != m_prevMenuSelection) {
        PlaySound(g_sndMenuNav);
        m_prevMenuSelection = m_menuSelection;
    }

    // Transition to gameplay
    if (m_isStartingGame) {
        m_playTransitionTimer += dt;
    }
}

void MainMenuSystem::Draw(int screenW, int screenH, float timeVal) {
    if (m_showCaseFilesModal) {
        DrawCaseFilesModal(screenW, screenH, GetMousePosition());
        return;
    }
    if (m_showSurvivalModal) {
        DrawSurvivalModal(screenW, screenH, GetMousePosition());
        return;
    }
    if (m_showSettingsModal) {
        DrawSettingsModal(screenW, screenH, GetMousePosition());
        return;
    }
    if (m_showDossierModal) {
        DrawDossierModal(screenW, screenH, GetMousePosition());
        return;
    }

    // Main Menu Background
    int leftShadW = (int)(screenW * 0.44f);
    DrawRectangleGradientH(0, 0, leftShadW, screenH, Color{ 3, 4, 6, 210 }, BLANK);
    DrawRectangleGradientV(0, screenH - 60, screenW, 60, BLANK, Color{ 2, 3, 5, 175 });

    if (m_menuFlashAlpha > 0.0f) {
        DrawRectangle(0, 0, screenW, screenH, Color{ 200, 220, 250, (unsigned char)(m_menuFlashAlpha * 95.0f) });
    }

    float slumberAlpha = (1.0f - m_awakeIntensity) * 235.0f;
    if (slumberAlpha > 2.0f) {
        DrawRectangle(0, 0, screenW, screenH, Color{ 2, 3, 5, (unsigned char)slumberAlpha });
    }

    DrawCircleGradient((int)m_lightPos.x, (int)m_lightPos.y, 480.0f, Color{ 210, 195, 160, (unsigned char)(22 * m_awakeIntensity) }, Color{ 0, 0, 0, 0 });
    DrawCircleGradient((int)m_lightPos.x, (int)m_lightPos.y, 250.0f, Color{ 235, 215, 180, (unsigned char)(40 * m_awakeIntensity) }, Color{ 0, 0, 0, 0 });
    DrawCircleGradient((int)m_lightPos.x, (int)m_lightPos.y, 95.0f,  Color{ 255, 245, 220, (unsigned char)(72 * m_awakeIntensity) }, Color{ 0, 0, 0, 0 });

    for (int d = 0; d < 22; d++) {
        float seed = (float)d * 137.5f;
        float dx = fmodf(seed * 43.0f + timeVal * 16.0f * (1.0f + fmodf(seed, 0.4f)), (float)screenW);
        float dy = fmodf(seed * 67.0f + sinf(timeVal * 0.7f + seed) * 35.0f, (float)screenH);
        float dDist = Vector2Distance(Vector2{ dx, dy }, m_lightPos);
        if (dDist < 250.0f) {
            float alpha = (1.0f - dDist / 250.0f) * 190.0f * m_awakeIntensity;
            DrawCircle((int)dx, (int)dy, 1.2f + fmodf(seed, 1.8f), Color{ 255, 235, 195, (unsigned char)alpha });
        }
    }

    DrawMenuCCTVOverlay(screenW, screenH, GetMousePosition(), (float)GetTime(), g_sndRadioStatic);

    float uiAlpha = m_isStartingGame ? Clamp(1.0f - (m_playTransitionTimer / 0.70f) * 2.5f, 0.0f, 1.0f) : 1.0f;
    float menuDormancyAlpha = uiAlpha * Clamp(m_awakeIntensity, 0.0f, 1.0f);

    if (menuDormancyAlpha > 0.02f) {
        int menuX = (int)(screenW * 0.085f);
        int titleY = (int)(screenH * 0.17f);
        int jx = 0, jy = 0;
        if (fmodf(timeVal, 8.0f) < 0.05f) {
            jx = GetRandomValue(-1, 1);
            jy = GetRandomValue(-1, 1);
        }
        float titleDist = Vector2Distance(Vector2{ (float)menuX, (float)titleY }, m_lightPos);
        float titleGleam = Clamp(1.0f - titleDist / 400.0f, 0.0f, 1.0f) * m_awakeIntensity;
        const char* mainTitle = "WHAT THE GROUND KEEPS";
        Color shadowCol = { 135, 20, 16, (unsigned char)(210 * menuDormancyAlpha) };
        Color textCol   = { (unsigned char)(238 + (int)(17 * titleGleam)), (unsigned char)(232 + (int)(23 * titleGleam)), (unsigned char)(224 + (int)(23 * titleGleam)), (unsigned char)(255 * menuDormancyAlpha) };
        DrawTextSharp(g_fontTitle, mainTitle, menuX + 2 + jx, titleY + 2 + jy, 46.0f, shadowCol, 2.5f);
        DrawTextSharp(g_fontTitle, mainTitle, menuX + jx, titleY + jy, 46.0f, textCol, 2.5f);
        DrawLine(menuX, titleY + 54, menuX + 360, titleY + 54, Color{ 180, 40, 32, (unsigned char)(200 * menuDormancyAlpha) });
        DrawTextSharp(g_fontHeadSub, "Some graves were never meant to be opened.", menuX, titleY + 64, 16.0f, Color{ 190, 75, 65, (unsigned char)(210 * menuDormancyAlpha) }, 1.2f);

        const char* kMenuLabels[5] = { "Play", "Case Files", "Settings", "Controls", "Quit" };
        int menuStartY = (int)(screenH * 0.38f);
        int itemSpacing = 54;

        for (int i = 0; i < 5; i++) {
            int itemY = menuStartY + i * itemSpacing;
            float textLen = MeasureTextSharp(g_fontTitle, kMenuLabels[i], 36.0f, 1.5f);
            bool isSelected = (m_menuSelection == i);
            float slideX = m_menuOptionHover[i] * 14.0f;
            Color labelCol = isSelected ? Color{ 255, 250, 242, (unsigned char)(255 * menuDormancyAlpha) } : Color{ 150, 155, 165, (unsigned char)(190 * menuDormancyAlpha) };
            if (isSelected) {
                DrawRectangle(menuX - 14 + (int)slideX, itemY + 10, 3, 24, Color{ 235, 45, 35, (unsigned char)(255 * menuDormancyAlpha) });
                DrawTextSharp(g_fontTitle, ">", menuX - 4 + (int)slideX, itemY + 6, 28.0f, Color{ 235, 45, 35, (unsigned char)(255 * menuDormancyAlpha) });
                DrawTextSharp(g_fontTitle, kMenuLabels[i], menuX + 18 + (int)slideX, itemY + 2, 36.0f, Color{ 160, 30, 25, (unsigned char)(170 * menuDormancyAlpha) }, 1.5f);
                DrawLine(menuX + 16 + (int)slideX, itemY + 40, menuX + 16 + (int)slideX + (int)textLen, itemY + 40, Color{ 235, 45, 35, (unsigned char)(180 * menuDormancyAlpha) });
                if (GetRandomValue(0, 100) > 90) {
                    const char* glitched[] = { "+", "x", "-", "|", ".", ":" };
                    DrawTextSharp(g_fontSmall, glitched[GetRandomValue(0, 5)], menuX + 20 + (int)slideX + textLen + GetRandomValue(-4, 4), itemY + 12 + GetRandomValue(-4, 4), 16.0f, Color{ 220, 50, 40, (unsigned char)(120 * menuDormancyAlpha) });
                }
            }
            DrawTextSharp(g_fontTitle, kMenuLabels[i], menuX + 16 + (int)slideX, itemY, 36.0f, labelCol, 1.5f);
        }

        if (m_awakeIntensity > 0.05f) {
            Vector2 mPos = GetMousePosition();
            int cx = (int)mPos.x, cy = (int)mPos.y;
            bool mouseOverAnyItem = false;
            // Re-calculate mouseOverAnyItem here for the reticle
            for (int i = 0; i < 5; i++) {
                int itemY = menuStartY + i * itemSpacing;
                float textLen = MeasureTextSharp(g_fontTitle, kMenuLabels[i], 36.0f, 1.5f);
                Rectangle hitRec = { (float)(menuX - 25), (float)(itemY - 4), textLen + 55.0f, 44.0f };
                if (CheckCollisionPointRec(mPos, hitRec)) { mouseOverAnyItem = true; break; }
            }
            if (mouseOverAnyItem) {
                Color retCol = { 235, 55, 45, (unsigned char)(235 * m_awakeIntensity * uiAlpha) };
                DrawCircle(cx, cy, 2.5f, retCol);
                DrawLine(cx - 8, cy, cx + 8, cy, retCol);
                DrawLine(cx, cy - 8, cx, cy + 8, retCol);
            } else {
                Color retCol = { 250, 235, 200, (unsigned char)(210 * m_awakeIntensity * uiAlpha) };
                DrawCircle(cx, cy, 2.0f, retCol);
                DrawLine(cx - 7, cy, cx - 3, cy, retCol);
                DrawLine(cx + 3, cy, cx + 7, cy, retCol);
                DrawLine(cx, cy - 7, cx, cy - 3, retCol);
                DrawLine(cx, cy + 3, cx, cy + 7, retCol);
            }
        }
    }

    if (m_isStartingGame) {
        float pProg = Clamp(m_playTransitionTimer / 0.70f, 0.0f, 1.0f);
        DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, pProg));
    }
}

void MainMenuSystem::DrawCaseFilesModal(int screenW, int screenH, Vector2 mPos) {
    int dw = (int)(screenW * 0.74f);
    if (dw < 780) dw = 780;
    if (dw > 980) dw = 980;
    int dh = (int)(screenH * 0.76f);
    if (dh < 500) dh = 500;
    if (dh > 620) dh = 620;
    int dx = screenW/2 - dw/2, dy = screenH/2 - dh/2;
    DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 215 });
    DrawRectangle(dx, dy, dw, dh, Color{ 10, 11, 14, 252 });
    DrawRectangleLines(dx, dy, dw, dh, Color{ 140, 35, 25, 255 });
    DrawTextSharp(g_fontHeadSub, "STATE POLICE // CLASSIFIED EVIDENCE DOSSIER", dx + 28, dy + 18, 25.0f, Color{ 235, 225, 215, 255 });
    DrawTextSharp(g_fontSmall, "CASE #89-094 // ROUTE 9 SERVICE STATION & BORDER MIRE", dx + 28, dy + 48, 14.0f, Color{ 180, 60, 50, 240 });
    DrawLine(dx + 25, dy + 70, dx + dw - 25, dy + 70, Color{ 90, 30, 25, 220 });
    int listW = 230;
    const char* caseTitles[4] = { "01. DISPATCH TAPE", "02. SIBLING INCIDENT", "03. THE MIRE HOUND", "04. SUB-SURFACE SOIL" };
    for (int f = 0; f < 4; f++) {
        Rectangle fRec = { (float)(dx + 25), (float)(dy + 86 + f * 58), (float)listW, 46.0f };
        bool fHover = CheckCollisionPointRec(mPos, fRec);
        bool fActive = (m_caseFileSelected == f);
        DrawRectangleRec(fRec, fActive ? Color{ 65, 26, 22, 255 } : (fHover ? Color{ 30, 22, 20, 220 } : Color{ 16, 17, 21, 210 }));
        DrawRectangleLinesEx(fRec, 1.0f, fActive ? Color{ 230, 65, 55, 255 } : (fHover ? WHITE : Color{ 75, 50, 45, 190 }));
        DrawTextSharp(g_fontMenu, caseTitles[f], (int)fRec.x + 14, (int)fRec.y + 12, 16.0f, fActive ? WHITE : Color{ 195, 190, 180, 230 });
        if (fHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && m_caseFileSelected != f) {
            m_caseFileSelected = f;
            PlaySound(g_sndMenuNav);
        }
    }
    int docX = dx + 275;
    int docY = dy + 86;
    int docW = dw - 302;
    int docH = dh - 156;
    DrawRectangle(docX, docY, docW, docH, Color{ 14, 16, 20, 255 });
    DrawRectangleLines(docX, docY, docW, docH, Color{ 55, 42, 38, 220 });
    if (m_caseFileSelected == 0) {
        DrawTextSharp(g_fontHeadSub, "TRANSCRIPT: 911 LOG // CALL REC 22:14:08", docX + 20, docY + 16, 19.0f, Color{ 235, 70, 60, 255 });
        DrawTextSharp(g_fontSmall, "LOCATION: Mile Marker 14, Route 9 Northern Pass", docX + 20, docY + 42, 14.0f, Color{ 160, 155, 150, 220 });
        DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, Color{ 70, 35, 30, 220 });
        const char* l1 = "\"Patrol, our vehicle radiator blew near the abandoned";
        const char* l2 = "gas stop. It's pouring rain. There are no lights out here";
        const char* l3 = "except the pumps. My sister says she heard clicking sounds";
        const char* l4 = "in the drainage ditch... Wait, something is watching us";
        const char* l5 = "from the spruce tree line. Send someone out here now--\"";
        const char* l6 = "[TRANSMISSION CUT - SIGNAL LOST // 00:01:24 RECORDED]";
        DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l4, docX + 20, docY + 160, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l5, docX + 20, docY + 188, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l6, docX + 20, docY + 228, 17.0f, Color{ 235, 60, 50, 255 });
    } else if (m_caseFileSelected == 1) {
        DrawTextSharp(g_fontHeadSub, "INCIDENT LOG: MISSING PERSON // SIBLING DOSSIER", docX + 20, docY + 16, 19.0f, Color{ 235, 70, 60, 255 });
        DrawTextSharp(g_fontSmall, "STATUS: Unresolved / Active Search Warrant", docX + 20, docY + 42, 14.0f, Color{ 160, 155, 150, 220 });
        DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, Color{ 70, 35, 30, 220 });
        const char* l1 = "When state troopers inspected the stalled sedan at dawn,";
        const char* l2 = "the driver's door was swung open into the mud.";
        const char* l3 = "The passenger side was empty. A pair of footprints led";
        const char* l4 = "from the road toward the mire. The footprints stopped";
        const char* l5 = "abruptly 40 feet into the dark mud with no return trail.";
        const char* l6 = "Only deep claw indentations were pressed into the peat.";
        DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l4, docX + 20, docY + 160, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l5, docX + 20, docY + 188, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l6, docX + 20, docY + 224, 17.0f, Color{ 215, 210, 200, 245 });
    } else if (m_caseFileSelected == 2) {
        DrawTextSharp(g_fontHeadSub, "ANOMALOUS ENTITY // CLASSIFICATION: SKELETON HOUND", docX + 20, docY + 16, 19.0f, Color{ 235, 70, 60, 255 });
        DrawTextSharp(g_fontSmall, "OBSERVER: Station Attendant Security Cam #02", docX + 20, docY + 42, 14.0f, Color{ 160, 155, 150, 220 });
        DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, Color{ 70, 35, 30, 220 });
        const char* l1 = "Entity displays the anatomy of a massive canine, but with";
        const char* l2 = "externalized skeletal structure and exposed vertebral ribs.";
        const char* l3 = "Exhibits luminescence in ocular cavities when in shadows.";
        const char* l4 = "Does not consume flesh conventionally; appears drawn to";
        const char* l5 = "sub-surface mineral deposits and fresh blood pooling";
        const char* l6 = "around the store's cold storage meat hook.";
        DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l4, docX + 20, docY + 160, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l5, docX + 20, docY + 188, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l6, docX + 20, docY + 224, 17.0f, Color{ 215, 210, 200, 245 });
    } else {
        DrawTextSharp(g_fontHeadSub, "FORENSIC GEOLOGY // ANOMALOUS MIRE PRESERVATION", docX + 20, docY + 16, 19.0f, Color{ 235, 70, 60, 255 });
        DrawTextSharp(g_fontSmall, "SAMPLE ANALYSIS: Route 9 Bog Core 12-F", docX + 20, docY + 42, 14.0f, Color{ 160, 155, 150, 220 });
        DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, Color{ 70, 35, 30, 220 });
        const char* l1 = "Core drilling 8 meters into the mire revealed biological";
        const char* l2 = "specimens buried decades ago with zero cellular decay.";
        const char* l3 = "Tissues retain hydration and microscopic muscle twitching.";
        const char* l4 = "Local saying carved into the gas station counter:";
        const char* l5 = "\"WHAT THE GROUND KEEPS, IT NEVER RELEASES.\"";
        const char* l6 = "Excavation without proper protective tools is lethal.";
        DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, Color{ 215, 210, 200, 245 });
        DrawTextSharp(g_fontBody, l4, docX + 20, docY + 162, 15.0f, Color{ 175, 170, 165, 210 });
        DrawTextSharp(g_fontHeadSub, l5, docX + 20, docY + 186, 17.0f, Color{ 240, 60, 50, 255 });
        DrawTextSharp(g_fontBody, l6, docX + 20, docY + 224, 15.0f, Color{ 195, 190, 180, 230 });
    }
    Rectangle btnCloseDossier = { (float)(dx + dw/2 - 85), (float)(dy + dh - 48), 170.0f, 36.0f };
    bool hClose = CheckCollisionPointRec(mPos, btnCloseDossier);
    DrawRectangleRec(btnCloseDossier, hClose ? Color{ 95, 30, 25, 255 } : Color{ 35, 18, 16, 240 });
    DrawRectangleLinesEx(btnCloseDossier, 1.0f, hClose ? WHITE : Color{ 160, 50, 40, 255 });
    DrawTextSharpCentered(g_fontMenu, "CLOSE [ESC]", dx + dw/2, dy + dh - 40, 16.0f, WHITE);
    if (hClose && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySound(g_sndMenuNav);
        m_showCaseFilesModal = false;
    }
}

void MainMenuSystem::DrawSurvivalModal(int screenW, int screenH, Vector2 mPos) {
    int sw = 640, sh = 490;
    int sx = screenW/2 - sw/2, sy = screenH/2 - sh/2;
    DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 215 });
    DrawRectangle(sx, sy, sw, sh, Color{ 10, 11, 14, 252 });
    DrawRectangleLines(sx, sy, sw, sh, Color{ 140, 35, 25, 255 });
    DrawTextSharpCentered(g_fontHeadSub, "S U R V I V A L   R E C O R D", sx + sw/2, sy + 18, 26.0f, Color{ 245, 235, 225, 255 }, 2.0f);
    DrawLine(sx + 35, sy + 50, sx + sw - 35, sy + 50, Color{ 90, 30, 25, 220 });
    DrawTextSharp(g_fontSmall, "ROUTE 9 SERVICE STATION // ATTENDANT SHIFT DOSSIER", sx + 45, sy + 66, 14.0f, Color{ 190, 65, 55, 240 });
    DrawTextSharp(g_fontBody, "SURVEILLANCE ENGINE: ROCK-SOLID 144+ FPS ACTIVE", sx + 45, sy + 98, 16.0f, Color{ 215, 210, 200, 245 });
    DrawTextSharp(g_fontBody, "LOCATION: 44.9184° N, 71.3820° W (MILE 14)", sx + 45, sy + 126, 16.0f, Color{ 195, 190, 185, 230 });
    DrawTextSharp(g_fontBody, "WEATHER TELEMETRY: NIGHT TIME PRECIPITATION (TORRENTIAL)", sx + 45, sy + 154, 16.0f, Color{ 195, 190, 185, 230 });
    DrawTextSharp(g_fontBody, "ANOMALY THREAT LEVEL: HIGH (NOCTURNAL ENTITY ACTIVE)", sx + 45, sy + 182, 16.0f, Color{ 240, 60, 50, 255 });
    DrawLine(sx + 35, sy + 216, sx + sw - 35, sy + 216, Color{ 70, 25, 22, 190 });
    DrawTextSharp(g_fontHeadSub, "INVESTIGATION MILESTONES:", sx + 45, sy + 232, 19.0f, Color{ 235, 225, 215, 255 });
    DrawTextSharp(g_fontBody, "[+] STALLED SEDAN LOCATED ON HIGHWAY", sx + 55, sy + 262, 16.0f, Color{ 150, 215, 160, 245 });
    DrawTextSharp(g_fontBody, "[+] GRETHNAR'S 24/7 STATION ACCESSED", sx + 55, sy + 290, 16.0f, Color{ 150, 215, 160, 245 });
    DrawTextSharp(g_fontBody, "[+] ROOF SURVEILLANCE OPTICS CALIBRATED", sx + 55, sy + 318, 16.0f, Color{ 150, 215, 160, 245 });
    DrawTextSharp(g_fontBody, "[!] MEAT LOCKER BLOOD ANOMALY UNRESOLVED", sx + 55, sy + 346, 16.0f, Color{ 245, 75, 65, 255 });
    Rectangle btnBackSurv = { (float)(sx + sw/2 - 85), (float)(sy + sh - 48), 170.0f, 36.0f };
    bool hBackS = CheckCollisionPointRec(mPos, btnBackSurv);
    DrawRectangleRec(btnBackSurv, hBackS ? Color{ 95, 30, 25, 255 } : Color{ 35, 18, 16, 240 });
    DrawRectangleLinesEx(btnBackSurv, 1.0f, hBackS ? WHITE : Color{ 160, 50, 40, 255 });
    DrawTextSharpCentered(g_fontMenu, "BACK [ESC]", sx + sw/2, sy + sh - 40, 16.0f, WHITE);
    if (hBackS && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySound(g_sndMenuNav);
        m_showSurvivalModal = false;
    }
}

void MainMenuSystem::DrawSettingsModal(int screenW, int screenH, Vector2 mPos) {
    int sw = 660, sh = 520;
    int sx = screenW/2 - sw/2, sy = screenH/2 - sh/2;
    DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 200 });
    DrawRectangle(sx, sy, sw, sh, Color{ 8, 10, 12, 252 });
    DrawRectangleLines(sx, sy, sw, sh, Color{ 140, 35, 25, 255 });
    DrawTextSharpCentered(g_fontHeadSub, "O P T I O N S", sx + sw/2, sy + 18, 26.0f, Color{ 245, 235, 225, 255 }, 2.0f);
    DrawLine(sx + 35, sy + 48, sx + sw - 35, sy + 48, Color{ 90, 30, 25, 220 });
    DrawTextSharp(g_fontBody, "MASTER AUDIO", sx + 45, sy + 60, 16.0f, Color{ 215, 210, 205, 255 });
    Rectangle volTrack = { (float)(sx + 45), (float)(sy + 84), 280.0f, 14.0f };
    DrawRectangleRec(volTrack, Color{ 20, 22, 25, 255 });
    DrawRectangleLinesEx(volTrack, 1.0f, Color{ 75, 38, 32, 240 });
    DrawRectangle((int)volTrack.x, (int)volTrack.y, (int)(volTrack.width * g_userMasterVolume), (int)volTrack.height, Color{ 220, 55, 45, 255 });
    char volStr[32]; snprintf(volStr, sizeof(volStr), "%d%%", (int)(g_userMasterVolume * 100.0f));
    DrawTextSharp(g_fontBody, volStr, sx + 340, sy + 80, 16.0f, Color{ 235, 230, 225, 255 });
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, Rectangle{ volTrack.x - 10, volTrack.y - 6, volTrack.width + 20, volTrack.height + 16 })) {
        g_userMasterVolume = Clamp((mPos.x - volTrack.x) / volTrack.width, 0.0f, 1.0f);
        SetMasterVolume(g_userMasterVolume);
    }
    DrawTextSharp(g_fontBody, "MOUSE SENSITIVITY", sx + 45, sy + 110, 16.0f, Color{ 215, 210, 205, 255 });
    Rectangle sensTrack = { (float)(sx + 45), (float)(sy + 134), 280.0f, 14.0f };
    DrawRectangleRec(sensTrack, Color{ 20, 22, 25, 255 });
    DrawRectangleLinesEx(sensTrack, 1.0f, Color{ 75, 38, 32, 240 });
    float sensNorm = Clamp((g_userMouseSensitivity - 0.5f) / 2.0f, 0.0f, 1.0f);
    DrawRectangle((int)sensTrack.x, (int)sensTrack.y, (int)(sensTrack.width * sensNorm), (int)sensTrack.height, Color{ 220, 55, 45, 255 });
    char sensStr[32]; snprintf(sensStr, sizeof(sensStr), "%.1fx", g_userMouseSensitivity);
    DrawTextSharp(g_fontBody, sensStr, sx + 340, sy + 130, 16.0f, Color{ 235, 230, 225, 255 });
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, Rectangle{ sensTrack.x - 10, sensTrack.y - 6, sensTrack.width + 20, sensTrack.height + 16 })) {
        g_userMouseSensitivity = 0.5f + Clamp((mPos.x - sensTrack.x) / sensTrack.width, 0.0f, 1.0f) * 2.0f;
    }
    DrawTextSharp(g_fontBody, "FIELD OF VIEW", sx + 45, sy + 160, 16.0f, Color{ 215, 210, 205, 255 });
    Rectangle fovTrack = { (float)(sx + 45), (float)(sy + 184), 280.0f, 14.0f };
    DrawRectangleRec(fovTrack, Color{ 20, 22, 25, 255 });
    DrawRectangleLinesEx(fovTrack, 1.0f, Color{ 75, 38, 32, 240 });
    float fovNorm = Clamp((g_userFov - 50.0f) / 40.0f, 0.0f, 1.0f);
    DrawRectangle((int)fovTrack.x, (int)fovTrack.y, (int)(fovTrack.width * fovNorm), (int)fovTrack.height, Color{ 220, 55, 45, 255 });
    char fovStr[32]; snprintf(fovStr, sizeof(fovStr), "%d°", (int)g_userFov);
    DrawTextSharp(g_fontBody, fovStr, sx + 340, sy + 180, 16.0f, Color{ 235, 230, 225, 255 });
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, Rectangle{ fovTrack.x - 10, fovTrack.y - 6, fovTrack.width + 20, fovTrack.height + 16 })) {
        g_userFov = 50.0f + Clamp((mPos.x - fovTrack.x) / fovTrack.width, 0.0f, 1.0f) * 40.0f;
    }
    Rectangle calibBox = { (float)(sx + 420), (float)(sy + 60), 200.0f, 142.0f };
    DrawRectangleRec(calibBox, Color{ 4, 5, 7, 255 });
    DrawRectangleLinesEx(calibBox, 1.0f, Color{ 65, 40, 35, 210 });
    Color faceCol = { (unsigned char)(28 * g_userHorrorGamma), (unsigned char)(28 * g_userHorrorGamma), (unsigned char)(34 * g_userHorrorGamma), 255 };
    DrawCircle((int)calibBox.x + 100, (int)calibBox.y + 48, 26.0f, faceCol);
    DrawCircle((int)calibBox.x + 91, (int)calibBox.y + 43, 3.5f, Color{ 0, 0, 0, 255 });
    DrawCircle((int)calibBox.x + 109, (int)calibBox.y + 43, 3.5f, Color{ 0, 0, 0, 255 });
    DrawTextSharp(g_fontSmall, "CALIBRATION:", (int)calibBox.x + 14, (int)calibBox.y + 88, 13.0f, Color{ 170, 165, 160, 230 });
    DrawTextSharp(g_fontSmall, "Adjust display so", (int)calibBox.x + 14, (int)calibBox.y + 104, 12.0f, Color{ 140, 135, 130, 200 });
    DrawTextSharp(g_fontSmall, "entity is barely visible.", (int)calibBox.x + 14, (int)calibBox.y + 120, 12.0f, Color{ 140, 135, 130, 200 });
    Rectangle btnFs = { (float)(sx + 45), (float)(sy + 214), 250.0f, 32.0f };
    bool hFs = CheckCollisionPointRec(mPos, btnFs);
    DrawRectangleRec(btnFs, hFs ? Color{ 70, 28, 22, 255 } : Color{ 28, 20, 18, 240 });
    DrawRectangleLinesEx(btnFs, 1.0f, hFs ? Color{ 230, 75, 55, 255 } : Color{ 110, 45, 35, 220 });
    DrawTextSharp(g_fontBody, IsWindowFullscreen() ? "[ F11 ] FULLSCREEN: ON" : "[ F11 ] FULLSCREEN: OFF", sx + 55, sy + 221, 15.0f, WHITE);
    if (hFs && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySound(g_sndMenuNav);
        ToggleGameFullscreen();
    }
    DrawLine(sx + 35, sy + 258, sx + sw - 35, sy + 258, Color{ 70, 25, 22, 190 });
    DrawTextSharp(g_fontHeadSub, "C O N T R O L S", sx + 45, sy + 270, 17.0f, Color{ 215, 90, 80, 255 });
    const char* ctrlList[7] = {
        "W / A / S / D     Move / Steer",
        "LEFT SHIFT        Sprint / Accelerate",
        "E                 Interact / Examine / Grab",
        "F                 Toggle Flashlight",
        "C                 Roof Surveillance Vantage",
        "TAB / ALT         Free / Lock Mouse",
        "Q / E             Switch CCTV Cameras"
    };
    for (int c = 0; c < 7; c++) {
        int cy = sy + 294 + c * 22;
        DrawTextSharp(g_fontBody, ctrlList[c], sx + 55, cy, 14.0f, Color{ 180, 175, 170, 240 });
    }
    Rectangle btnBack = { (float)(sx + sw/2 - 85), (float)(sy + sh - 46), 170.0f, 34.0f };
    bool hBack = CheckCollisionPointRec(mPos, btnBack);
    DrawRectangleRec(btnBack, hBack ? Color{ 95, 30, 25, 255 } : Color{ 35, 18, 16, 240 });
    DrawRectangleLinesEx(btnBack, 1.0f, hBack ? WHITE : Color{ 160, 50, 40, 255 });
    DrawTextSharpCentered(g_fontMenu, "BACK [ESC]", sx + sw/2, sy + sh - 38, 16.0f, WHITE);
    if (hBack && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySound(g_sndMenuNav);
        m_showSettingsModal = false;
    }
}

void MainMenuSystem::DrawDossierModal(int screenW, int screenH, Vector2 mPos) {
    int dw = (int)(screenW * 0.70f);
    if (dw < 720) dw = 720;
    if (dw > 920) dw = 920;
    int dh = (int)(screenH * 0.76f);
    if (dh < 500) dh = 500;
    if (dh > 620) dh = 620;
    int dx = screenW/2 - dw/2, dy = screenH/2 - dh/2;
    DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 225 });
    DrawRectangle(dx, dy, dw, dh, Color{ 12, 14, 18, 252 });
    DrawRectangleLines(dx, dy, dw, dh, Color{ 175, 145, 65, 255 });
    DrawTextSharp(g_fontHeadSub, "TOP SECRET // CIVIL DEFENSE & DEEP MONITORING DOSSIER", dx + 28, dy + 22, 17.0f, Color{ 240, 220, 180, 255 });
    DrawTextSharp(g_fontSmall, "LOCATION: ROUTE 9 SERVICE STATION SUB-TERRAIN // AUTH: LEVEL-4 DISPATCH", dx + 28, dy + 46, 12.0f, Color{ 200, 75, 60, 255 });
    DrawLine(dx + 25, dy + 68, dx + dw - 25, dy + 68, Color{ 120, 95, 45, 200 });
    int listW = 230;
    const char* dTitles[4] = { "01. 1984 EXCAVATION", "02. SPECIMEN 07-B", "03. MEAT LOCKER VENT", "04. EMERGENCY CACHE" };
    for (int t = 0; t < 4; t++) {
        Rectangle tRec = { (float)(dx + 25), (float)(dy + 82 + t * 54), (float)listW, 44.0f };
        bool tHover = CheckCollisionPointRec(mPos, tRec);
        bool tActive = (m_dossierFileSelected == t);
        DrawRectangleRec(tRec, tActive ? Color{ 55, 42, 22, 255 } : (tHover ? Color{ 28, 24, 18, 220 } : Color{ 16, 17, 20, 200 }));
        DrawRectangleLinesEx(tRec, 1.0f, tActive ? Color{ 225, 180, 70, 255 } : (tHover ? WHITE : Color{ 80, 65, 45, 180 }));
        DrawTextSharp(g_fontBody, dTitles[t], (int)tRec.x + 12, (int)tRec.y + 13, 13.0f, tActive ? WHITE : Color{ 200, 190, 175, 220 });
        if (tHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && m_dossierFileSelected != t) {
            m_dossierFileSelected = t;
            PlaySound(g_sndMenuNav);
        }
    }
    int docX = dx + 275, docY = dy + 82, docW = dw - 300, docH = dh - 150;
    DrawRectangle(docX, docY, docW, docH, Color{ 16, 18, 22, 255 });
    DrawRectangleLines(docX, docY, docW, docH, Color{ 65, 55, 42, 220 });
    if (m_dossierFileSelected == 0) {
        DrawTextSharp(g_fontMenu, "OPERATION SUB-STRATA: COLD WAR EXCAVATION LOG", docX + 20, docY + 16, 14.0f, Color{ 230, 180, 70, 255 });
        DrawTextSharp(g_fontSmall, "ARCHIVE: Station Foundation Survey (August 1984)", docX + 20, docY + 38, 11.0f, Color{ 160, 155, 145, 220 });
        DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, Color{ 80, 65, 40, 200 });
        DrawTextSharp(g_fontBody, "The secret underground corridor was initially excavated", docX + 20, docY + 68, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "during the Cold War as a civilian fallout monitor bunker.", docX + 20, docY + 90, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "However, miners struck hollow fissures 18 feet below.", docX + 20, docY + 112, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "The limestone walls bore massive parallel scrape furrows", docX + 20, docY + 134, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "resembling claw paths. Work was halted indefinitely.", docX + 20, docY + 156, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "Access was sealed beneath the exterior worn rug.", docX + 20, docY + 188, 13.0f, Color{ 200, 150, 60, 255 });
    } else if (m_dossierFileSelected == 1) {
        DrawTextSharp(g_fontMenu, "ANOMALOUS SPECIMEN 07-B: FORMALIN SUSPENSION", docX + 20, docY + 16, 14.0f, Color{ 230, 180, 70, 255 });
        DrawTextSharp(g_fontSmall, "CONTAINMENT: Hermetic Glass Jar on Chamber Worktable", docX + 20, docY + 38, 11.0f, Color{ 160, 155, 145, 220 });
        DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, Color{ 80, 65, 40, 200 });
        DrawTextSharp(g_fontBody, "A severed juvenile forelimb was recovered from the bog", docX + 20, docY + 68, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "culvert and preserved in formalin. The tissue exhibits", docX + 20, docY + 90, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "external calcified plating and bioluminescent nodes.", docX + 20, docY + 112, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "Microscopic twitching persists even when submerged.", docX + 20, docY + 134, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "Do not break the glass under any circumstance.", docX + 20, docY + 164, 13.0f, Color{ 220, 50, 45, 255 });
    } else if (m_dossierFileSelected == 2) {
        DrawTextSharp(g_fontMenu, "COLD STORAGE INTERFACE: OVERHEAD VENTILATION GRATE", docX + 20, docY + 16, 14.0f, Color{ 230, 180, 70, 255 });
        DrawTextSharp(g_fontSmall, "CORRELATION: Supermarket Meat Room & Underground Pit", docX + 20, docY + 38, 11.0f, Color{ 160, 155, 145, 220 });
        DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, Color{ 80, 65, 40, 200 });
        DrawTextSharp(g_fontBody, "The overhead ceiling grate connects directly to the floor", docX + 20, docY + 68, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "of the superstore's cold-storage walk-in meat locker.", docX + 20, docY + 90, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "Condensed brine and blood drip into the floor pail.", docX + 20, docY + 112, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "The scent carries deep into subterranean strata.", docX + 20, docY + 134, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "It acts as a scent lure. They gather beneath us at night.", docX + 20, docY + 164, 13.0f, Color{ 225, 45, 40, 255 });
    } else {
        DrawTextSharp(g_fontMenu, "EMERGENCY SUPPLY CACHE: COMBINATION CODE RECORD", docX + 20, docY + 16, 14.0f, Color{ 230, 180, 70, 255 });
        DrawTextSharp(g_fontSmall, "CONTAINER: Heavy Cast-Iron Padlocked Crate in Corner", docX + 20, docY + 38, 11.0f, Color{ 160, 155, 145, 220 });
        DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, Color{ 80, 65, 40, 200 });
        DrawTextSharp(g_fontBody, "Under the exterior carpet lies a deep mining descent", docX + 20, docY + 68, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "leading to the forgotten 19th-century abandoned village.", docX + 20, docY + 90, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontBody, "The midsection is blocked by a massive cave-in.", docX + 20, docY + 112, 13.0f, Color{ 215, 210, 200, 240 });
        DrawTextSharp(g_fontTitle, "[ TRENCH SHOVEL REQUIRED ]", docX + 20, docY + 138, 17.0f, Color{ 245, 195, 60, 255 });
        DrawTextSharp(g_fontBody, "Recover the shovel from the supermarket shelves to dig through.", docX + 20, docY + 185, 13.0f, Color{ 200, 195, 180, 240 });
    }
    Rectangle btnCloseDossier = { (float)(dx + dw/2 - 80), (float)(dy + dh - 48), 160.0f, 32.0f };
    bool hClose = CheckCollisionPointRec(mPos, btnCloseDossier);
    DrawRectangleRec(btnCloseDossier, hClose ? Color{ 95, 35, 25, 255 } : Color{ 36, 22, 18, 240 });
    DrawRectangleLinesEx(btnCloseDossier, 1.0f, hClose ? WHITE : Color{ 175, 140, 65, 255 });
    DrawTextSharpCentered(g_fontMenu, "CLOSE DOSSIER [ESC]", dx + dw/2, dy + dh - 40, 13.0f, WHITE);
    if (hClose && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        PlaySound(g_sndMenuNav);
        m_showDossierModal = false;
    }
}
