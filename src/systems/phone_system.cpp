#include "systems/phone_system.h"
#include "raymath.h"
#include "core/game_context.h"
#include "core/game_types.h"
#include "systems/ui_helpers.h"
#include "systems/ocean_system.h"

PhoneSystem::PhoneSystem() {
    m_active = false;
    m_zoomMode = 0;
    m_anim = 0.0f;
    m_radarPulse = 0.0f;
    m_signalFlicker = 0.0f;
}

PhoneSystem::~PhoneSystem() {
}

void PhoneSystem::Update(float dt) {
    if (m_active) {
        m_anim = Clamp(m_anim + dt * 5.2f, 0.0f, 1.0f);
    } else {
        m_anim = Clamp(m_anim - dt * 6.2f, 0.0f, 1.0f);
    }
    m_radarPulse = fmodf(m_radarPulse + dt * 1.35f, 1.0f);
    m_signalFlicker += dt;
}

void PhoneSystem::HandleInput() {
    if (IsKeyPressed(KEY_M)) {
        m_active = !m_active;
        PlaySound(g_sndPhoneSlide);
    }

    if (m_active && IsKeyPressed(KEY_Z)) {
        m_zoomMode = (m_zoomMode == 0) ? 1 : 0;
        PlaySound(g_sndPhoneTap);
    }
}

void PhoneSystem::Draw(RenderTexture2D target, Camera3D camera, float vmSwayX, float vmSwayY, float dayCycleTime, float dayCycleDuration, int waterState) {
    if (m_anim <= 0.005f) return;

    float pW = 295.0f;
    float pH = 570.0f;
    float phoneLagX = Clamp(vmSwayX * 240.0f, -20.0f, 20.0f);
    float phoneLagY = Clamp(vmSwayY * 180.0f, -16.0f, 16.0f);
    float pX = (float)LOGICAL_W - pW - 35.0f + phoneLagX;
    float pY = (float)LOGICAL_H - (pH - 24.0f) * m_anim + phoneLagY;

    // 1. Soft Physical Ambient Drop Shadow
    DrawRectangleRounded(Rectangle{ pX + 8, pY + 12, pW, pH }, 0.08f, 16, Color{ 0, 0, 0, (unsigned char)(110 * m_anim) });
    DrawRectangleRounded(Rectangle{ pX + 4, pY + 6, pW, pH }, 0.08f, 16, Color{ 0, 0, 0, (unsigned char)(150 * m_anim) });

    // 2. Titanium / Matte Metal Smartphone Chassis
    DrawRectangleRounded(Rectangle{ pX, pY, pW, pH }, 0.08f, 16, Color{ 34, 37, 42, 255 });
    DrawRectangleRoundedLinesEx(Rectangle{ pX, pY, pW, pH }, 0.08f, 16, 2.0f, Color{ 72, 78, 86, 255 });

    // Physical Hardware Buttons
    DrawRectangle((int)(pX - 3), (int)(pY + 115), 3, 26, Color{ 48, 52, 58, 255 }); // Vol Up
    DrawRectangle((int)(pX - 3), (int)(pY + 152), 3, 26, Color{ 48, 52, 58, 255 }); // Vol Down
    DrawRectangle((int)(pX + pW), (int)(pY + 130), 3, 38, Color{ 48, 52, 58, 255 }); // Power

    // 3. AMOLED Glass Bezel & Screen Base
    float scrX = pX + 11.0f;
    float scrY = pY + 12.0f;
    float scrW = pW - 22.0f;
    float scrH = pH - 24.0f;
    DrawRectangleRounded(Rectangle{ scrX, scrY, scrW, scrH }, 0.06f, 12, Color{ 12, 14, 18, 255 });

    // 4. Dynamic Island
    DrawRectangleRounded(Rectangle{ pX + pW/2 - 28, scrY + 5, 56, 13 }, 0.5f, 8, Color{ 6, 7, 9, 255 });
    DrawCircle((int)(pX + pW/2 + 14), (int)(scrY + 11), 3, Color{ 22, 38, 62, 255 });
    DrawRectangle((int)(pX + pW/2 - 14), (int)(scrY + 1), 28, 2, Color{ 28, 30, 35, 255 });

    // 5. STATUS BAR
    float cycleFracClock = dayCycleTime / dayCycleDuration;
    float time24Clock = cycleFracClock * 24.0f;
    int phoneHour = (int)time24Clock % 24;
    int phoneMin = (int)((time24Clock - floorf(time24Clock)) * 60.0f);
    int displayHourPhone = phoneHour % 12;
    if (displayHourPhone == 0) displayHourPhone = 12;
    char clockBuf[16];
    snprintf(clockBuf, sizeof(clockBuf), "%02d:%02d %s", displayHourPhone, phoneMin, (phoneHour >= 12) ? "PM" : "AM");
    DrawTextSharp(g_fontSmall, clockBuf, scrX + 10, scrY + 6, 11.0f, Color{ 230, 235, 240, 255 });

    bool signalFlicker = (fmodf(m_signalFlicker, 4.2f) > 3.6f);
    DrawTextSharp(g_fontSmall, signalFlicker ? "NO SERVICE" : "1 BAR [E]", scrX + scrW - 105, scrY + 6, 10.0f, signalFlicker ? Color{ 225, 65, 55, 255 } : Color{ 160, 165, 175, 220 });
    DrawRectangleLines((int)(scrX + scrW - 28), (int)(scrY + 7), 16, 9, Color{ 175, 180, 190, 240 });
    DrawRectangle((int)(scrX + scrW - 12), (int)(scrY + 9), 2, 5, Color{ 175, 180, 190, 240 });
    DrawRectangle((int)(scrX + scrW - 26), (int)(scrY + 9), 4, 5, Color{ 230, 65, 55, 255 });

    // 6. MIRE-NAV APP BANNER
    DrawRectangle((int)scrX, (int)(scrY + 22), (int)scrW, 28, Color{ 18, 22, 28, 255 });
    DrawLine((int)scrX, (int)(scrY + 50), (int)(scrX + scrW), (int)(scrY + 50), Color{ 45, 55, 68, 255 });
    DrawTextSharp(g_fontSmall, "MIRE-NAV // GPS SATELLITE (OFFLINE)", scrX + 8, scrY + 26, 10.0f, Color{ 85, 195, 245, 255 });

    const char* sectorStr = "ROUTE 9 NORTH PASS";
    if (camera.position.y < 8.5f) {
        sectorStr = "SUB-TERRAIN BUNKER (-18 FT)";
    } else if (camera.position.x >= 86.0f && camera.position.x <= 108.0f && camera.position.z >= 126.0f && camera.position.z <= 154.0f) {
        sectorStr = "SUPERSTORE INTERIOR";
    } else if (camera.position.x >= 110.0f && camera.position.x <= 124.0f && camera.position.z >= 132.0f && camera.position.z <= 148.0f) {
        sectorStr = "SERVICE STATION FORECOURT";
    } else if (camera.position.x > 135.0f) {
        sectorStr = "PEAT MIRE & BOG DRAINAGE";
    } else if (camera.position.x <= 36.0f) {
        sectorStr = (waterState == WATER_STATE_DIVING) ? "BLACKWATER OCEAN (DIVING)" : "BLACKWATER COAST & PIER";
    } else if (camera.position.x < 85.0f) {
        sectorStr = "WEST WALL ALLEY (BEHIND SHOP)";
    }
    DrawTextSharp(g_fontSmall, sectorStr, scrX + 8, scrY + 38, 9.0f, Color{ 215, 205, 180, 220 });

    // 7. INTERACTIVE GPS MAP
    float mapX = scrX + 4.0f;
    float mapY = scrY + 53.0f;
    float mapW = scrW - 8.0f;
    float mapH = scrH - 128.0f;
    DrawRectangle((int)mapX, (int)mapY, (int)mapW, (int)mapH, Color{ 15, 18, 22, 255 });
    DrawRectangleLines((int)mapX, (int)mapY, (int)mapW, (int)mapH, Color{ 35, 42, 52, 255 });

    float viewCenterX = (m_zoomMode == 0) ? camera.position.x : 115.0f;
    float viewCenterZ = (m_zoomMode == 0) ? camera.position.z : 140.0f;
    float mapScale = (m_zoomMode == 0) ? 4.4f : 1.35f;

    auto WorldToMap = [&](float wx, float wz) -> Vector2 {
        return { mapX + mapW * 0.5f + (wx - viewCenterX) * mapScale, mapY + mapH * 0.5f + (wz - viewCenterZ) * mapScale };
    };
    auto InMapBounds = [&](float mx, float my, float pad = 0.0f) -> bool {
        return (mx >= mapX - pad && mx <= mapX + mapW + pad && my >= mapY - pad && my <= mapY + mapH + pad);
    };

    for (int gx = 40; gx <= 220; gx += 20) {
        Vector2 g1 = WorldToMap((float)gx, 60.0f);
        Vector2 g2 = WorldToMap((float)gx, 220.0f);
        if ((g1.x >= mapX && g1.x <= mapX + mapW) || (g2.x >= mapX && g2.x <= mapX + mapW)) {
            float clx = Clamp(g1.x, mapX, mapX + mapW);
            DrawLine((int)clx, (int)fmaxf(g1.y, mapY), (int)clx, (int)fminf(g2.y, mapY + mapH), Color{ 28, 34, 42, 160 });
        }
    }
    for (int gz = 60; gz <= 220; gz += 20) {
        Vector2 g1 = WorldToMap(40.0f, (float)gz);
        Vector2 g2 = WorldToMap(220.0f, (float)gz);
        if ((g1.y >= mapY && g1.y <= mapY + mapH) || (g2.y >= mapY && g2.y <= mapY + mapH)) {
            float cly = Clamp(g1.y, mapY, mapY + mapH);
            DrawLine((int)fmaxf(g1.x, mapX), (int)cly, (int)fminf(g2.x, mapX + mapW), (int)cly, Color{ 28, 34, 42, 160 });
        }
    }

    // Forests
    Vector2 fWestTop = WorldToMap(38.0f, 60.0f);
    Vector2 fWestBot = WorldToMap(75.0f, 220.0f);
    float fwX = Clamp(fWestTop.x, mapX, mapX + mapW);
    float fwW = Clamp(fWestBot.x - fwX, 0.0f, mapX + mapW - fwX);
    if (fwW > 0.0f) DrawRectangle((int)fwX, (int)fmaxf(fWestTop.y, mapY), (int)fwW, (int)fminf(fWestBot.y - fWestTop.y, mapH), Color{ 12, 18, 14, 210 });

    // Coast
    Vector2 ocTop = WorldToMap(-50.0f, 0.0f);
    Vector2 ocBot = WorldToMap(35.0f, 256.0f);
    float ocX = Clamp(ocTop.x, mapX, mapX + mapW);
    float ocW = Clamp(ocBot.x - ocX, 0.0f, mapX + mapW - ocX);
    if (ocW > 0.0f) {
        DrawRectangle((int)ocX, (int)mapY, (int)ocW, (int)mapH, Color{ 10, 26, 38, 235 });
        DrawLine((int)(ocX + ocW), (int)mapY, (int)(ocX + ocW), (int)(mapY + mapH), Color{ 45, 120, 160, 240 });
        Vector2 pierStart = WorldToMap(36.0f, 138.0f);
        Vector2 pierEnd = WorldToMap(16.0f, 138.0f);
        DrawLineEx(pierStart, pierEnd, 3.0f, Color{ 140, 115, 75, 255 });
        if (InMapBounds(ocX + 4, mapY + 20)) DrawTextSharp(g_fontSmall, "BLACKWATER SEA", ocX + 6, mapY + 12, 9.0f, Color{ 65, 185, 215, 230 });
    }

    // Mire
    Vector2 mireP1 = WorldToMap(136.0f, 95.0f);
    Vector2 mireP2 = WorldToMap(185.0f, 185.0f);
    float mX = Clamp(mireP1.x, mapX, mapX + mapW);
    float mY = Clamp(mireP1.y, mapY, mapY + mapH);
    float mW = Clamp(mireP2.x - mX, 0.0f, mapX + mapW - mX);
    float mH = Clamp(mireP2.y - mY, 0.0f, mapY + mapH - mY);
    if (mW > 0.0f && mH > 0.0f) {
        DrawRectangle((int)mX, (int)mY, (int)mW, (int)mH, Color{ 16, 26, 24, 230 });
        DrawRectangleLines((int)mX, (int)mY, (int)mW, (int)mH, Color{ 28, 48, 42, 240 });
        if (InMapBounds(mX + 6, mY + 12)) DrawTextSharp(g_fontSmall, "PEAT MIRE [HAZARD]", mX + 6, mY + 8, 9.0f, Color{ 55, 125, 95, 220 });
    }

    // Road
    Vector2 rTop = WorldToMap(121.0f, 60.0f);
    Vector2 rBot = WorldToMap(135.0f, 220.0f);
    float rx = Clamp(rTop.x, mapX, mapX + mapW);
    float rw = Clamp(rBot.x - rTop.x, 0.0f, mapX + mapW - rx);
    float ry = fmaxf(rTop.y, mapY);
    float rh = fminf(rBot.y - rTop.y, mapY + mapH - ry);
    if (rw > 0.0f && rh > 0.0f) {
        DrawRectangle((int)rx, (int)ry, (int)rw, (int)rh, Color{ 38, 42, 48, 255 });
        DrawLine((int)rx, (int)ry, (int)rx, (int)(ry + rh), Color{ 160, 165, 175, 220 });
        DrawLine((int)(rx + rw), (int)ry, (int)(rx + rw), (int)(ry + rh), Color{ 160, 165, 175, 220 });
        Vector2 rMid = WorldToMap(128.0f, 60.0f);
        if (rMid.x >= mapX && rMid.x <= mapX + mapW) {
            for (float dz = 60.0f; dz < 220.0f; dz += 8.0f) {
                Vector2 d1 = WorldToMap(128.0f, dz);
                Vector2 d2 = WorldToMap(128.0f, dz + 4.5f); // Corrected to WorldToMap
                if (d1.y >= mapY && d2.y <= mapY + mapH) DrawLine((int)d1.x, (int)d1.y, (int)d2.x, (int)d2.y, Color{ 220, 185, 45, 240 });
            }
        }
    }
    // Fix: WorldToPMap should be WorldToMap
}
