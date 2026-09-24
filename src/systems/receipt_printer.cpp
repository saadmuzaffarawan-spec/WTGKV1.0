#include "receipt_printer.h"
#include <raymath.h>
#include <rlgl.h>
#include <math.h>
#include <stdlib.h>

static float ReceiptFrand(float lo, float hi) {
    return lo + (float)GetRandomValue(0, 10000) / 10000.0f * (hi - lo);
}

// =========================================================================
// THERMAL RECEIPT PRINTER & HORROR RECEIPT SYSTEM
// Procedural aged paper, itemized horror list, curling output chain
// =========================================================================

// Real-world Authentic Super Mart Thermal Receipt:
// Pure bright thermal paper white, prominent heading main text, total price below,
// NO grocery items mentioned, and terrifying arterial blood stains.
Texture2D BuildReceiptTexture(const char* stalkerWarning)
{
    const int TEX_W = 380;
    const int TEX_H = 760;

    // Pure thermal paper white background
    Image paper = GenImageColor(TEX_W, TEX_H, Color{ 252, 252, 250, 255 });

    // Realistic thermal paper edge margin shading
    for (int y = 0; y < TEX_H; y++) {
        ImageDrawPixel(&paper, 0, y, Color{ 220, 220, 215, 255 });
        ImageDrawPixel(&paper, 1, y, Color{ 235, 235, 230, 255 });
        ImageDrawPixel(&paper, TEX_W - 2, y, Color{ 235, 235, 230, 255 });
        ImageDrawPixel(&paper, TEX_W - 1, y, Color{ 220, 220, 215, 255 });
    }

    // Top & Bottom Tear Bar Serrations (Notched zig-zag cut edges)
    for (int x = 0; x < TEX_W; x += 6) {
        for (int dy = 0; dy < 4; dy++) {
            int tooth = (x / 3) % 2 ? dy : (3 - dy);
            ImageDrawPixel(&paper, x + dy, tooth, Color{ 0, 0, 0, 0 });
            ImageDrawPixel(&paper, x + dy, TEX_H - 1 - tooth, Color{ 0, 0, 0, 0 });
        }
    }

    auto DrawCentered = [&](const char *text, int y, int fontSize, Color color) {
        int w = MeasureText(text, fontSize);
        int x = (TEX_W - w) / 2;
        if (x < 4) x = 4;
        ImageDrawText(&paper, text, x, y, fontSize, color);
    };

    auto DrawDashed = [&](int y, Color color) {
        for (int x = 14; x < TEX_W - 14; x += 9)
            ImageDrawLine(&paper, x, y, x + 5, y, color);
    };

    // Realistic horror arterial blood splatters, running gravity drips, and soaked coagulated pools
    auto DrawBloodPool = [&](int cx, int cy, int radius) {
        for (int r = radius; r >= 1; r--) {
            float t = (float)r / (float)radius;
            Color col = (t < 0.45f) ? Color{ 70, 3, 3, 250 } : Color{ 140, 10, 10, (unsigned char)(210 - t * 70) };
            ImageDrawCircle(&paper, cx, cy, r, col);
        }
    };

    auto DrawBloodSpatter = [&](int cx, int cy, int dropletCount) {
        DrawBloodPool(cx, cy, GetRandomValue(8, 18));
        for (int i = 0; i < dropletCount; i++) {
            int dist = GetRandomValue(6, 45);
            float ang = ReceiptFrand(0.0f, 6.283f);
            int dx = cx + (int)(cosf(ang) * dist);
            int dy = cy + (int)(sinf(ang) * dist);
            int sz = GetRandomValue(1, 4);
            ImageDrawCircle(&paper, dx, dy, sz, Color{ 175, 12, 12, (unsigned char)GetRandomValue(180, 255) });
        }
        // Dripping gravity trails running down the receipt
        int dripLen = GetRandomValue(30, 90);
        int px = cx + GetRandomValue(-4, 4), py = cy;
        for (int d = 0; d < dripLen; d += 3) {
            int nx = px + GetRandomValue(-1, 1);
            int ny = py + 3;
            unsigned char a = (unsigned char)(220 - (d * 180 / (dripLen > 0 ? dripLen : 1)));
            Color dCol = (d < dripLen / 2) ? Color{ 110, 6, 6, a } : Color{ 165, 12, 12, a };
            ImageDrawLine(&paper, px, py, nx, ny, dCol);
            ImageDrawLine(&paper, px + 1, py, nx + 1, ny, dCol);
            px = nx; py = ny;
        }
    };

    // Bloody finger / thumb drag smear on the side margin
    auto DrawThumbSmear = [&](int sx, int sy) {
        for (int step = 0; step < 26; step++) {
            int y = sy + step * 2;
            int x = sx + (int)(sinf(step * 0.25f) * 4.0f);
            ImageDrawRectangle(&paper, x, y, 16, 3, Color{ 120, 10, 10, (unsigned char)(140 - step * 4) });
            ImageDrawRectangle(&paper, x + 2, y, 12, 2, Color{ 80, 5, 5, (unsigned char)(160 - step * 5) });
        }
    };

    int iy = 26;

    // Header: Super Mart Brand & Register details
    Color fontInk = Color{ 18, 18, 22, 255 };
    Color fontSub = Color{ 65, 65, 72, 255 };
    Color fontRule = Color{ 90, 90, 98, 200 };

    DrawCentered("*** DEAD END MART ***", iy, 22, fontInk); iy += 28;
    DrawCentered("SUPERSTORE & GROCERY", iy, 14, fontSub); iy += 20;
    DrawCentered("STORE #0666   TERMINAL 01   REG 04", iy, 12, fontSub); iy += 18;
    DrawCentered("DATE: 10/31/2026   TIME: 03:33:13 AM", iy, 12, fontSub); iy += 22;

    DrawDashed(iy, fontRule); iy += 32;

    // PROMINENT HEADING MAIN TEXT (Bold, prominent, saturated thermal ink)
    DrawCentered("HATE YOU FOR SHOPPING.", iy,     22, fontInk);
    DrawCentered("HATE YOU FOR SHOPPING.", iy + 1, 22, fontInk); iy += 32;
    DrawCentered("NEVER COME BACK!",     iy,     24, fontInk);
    DrawCentered("NEVER COME BACK!",     iy + 1, 24, fontInk); iy += 42;

    DrawDashed(iy, fontRule); iy += 32;

    // TOTAL BILL PRICE (Crisp, bold, large thermal numerals)
    // STRICTLY ZERO ITEMS MENTIONED!
    DrawCentered("TOTAL USD    $666.13", iy,     26, fontInk);
    DrawCentered("TOTAL USD    $666.13", iy + 1, 26, fontInk); iy += 38;
    DrawCentered("CASH TENDERED:    $666.13", iy, 13, fontSub); iy += 20;
    DrawCentered("CHANGE DUE:          $0.00", iy, 13, fontSub); iy += 20;
    DrawCentered("TOTAL ITEMS SOLD:        0", iy, 13, fontSub); iy += 26;

    DrawDashed(iy, fontRule); iy += 22;
    if (stalkerWarning && stalkerWarning[0] != '\0') {
        DrawCentered("--- STALKER TELEMETRY ---", iy, 14, Color{ 175, 12, 12, 255 }); iy += 18;
        DrawCentered(stalkerWarning, iy, 13, Color{ 150, 10, 10, 255 }); iy += 24;
        DrawDashed(iy, fontRule); iy += 22;
    } else {
        DrawCentered("THANK YOU FOR YOUR SOUL", iy, 14, fontSub); iy += 35;
    }

    // Authentic Thermal 1D Barcode with Numbers
    int barX = 35;
    int barY = iy;
    int barH = 42;
    while (barX < TEX_W - 35) {
        int w = GetRandomValue(2, 5);
        if (GetRandomValue(0, 4) != 0) {
            ImageDrawRectangle(&paper, barX, barY, w, barH, fontInk);
        }
        barX += w + GetRandomValue(2, 4);
    }
    iy += barH + 8;
    DrawCentered("4  901234  567890", iy, 12, fontInk);

    // HORRIFYING BLOOD STAINS: High contrast against crisp thermal white
    DrawBloodSpatter(75, 140, 14);
    DrawBloodSpatter(290, 240, 18);
    DrawBloodSpatter(185, 390, 12);
    DrawBloodSpatter(80, 540, 16);
    DrawThumbSmear(TEX_W - 36, 170);
    DrawThumbSmear(12, 340);

    Texture2D tex = LoadTextureFromImage(paper);
    UnloadImage(paper);
    return tex;
}

void DrawPrinter(Vector3 pos, float ledGlow, float t, PrinterLightingFn lightFn)
{
    auto lit = [&](Vector3 p, Color c) -> Color {
        return lightFn ? lightFn(p, c) : c;
    };

    Color body     = lit(pos, Color{ 36, 36, 40, 255 });
    Color bodyDark = lit(pos, Color{ 18, 18, 22, 255 });
    Color rust     = lit(pos, Color{ 95, 14, 14, 210 });

    // Main printer chassis
    Vector3 chassisCenter = { pos.x, pos.y + 0.11f, pos.z };
    DrawCube(chassisCenter, 0.44f, 0.22f, 0.36f, body);
    DrawCubeWires(chassisCenter, 0.44f, 0.22f, 0.36f, bodyDark);

    float frontZ = pos.z + 0.18f;

    // Top paper slot groove
    Vector3 slotGroove = { pos.x, pos.y + 0.21f, pos.z + 0.11f };
    DrawCube(slotGroove, 0.28f, 0.02f, 0.08f, BLACK);

    // Guide lip
    Vector3 lip = { pos.x, pos.y + 0.225f, pos.z + 0.15f };
    DrawCube(lip, 0.30f, 0.015f, 0.03f, bodyDark);

    // Metal tear bar flush with front
    Vector3 tearBar = { pos.x, pos.y + 0.195f, frontZ + 0.003f };
    DrawCube(tearBar, 0.28f, 0.012f, 0.015f, lit(pos, DARKGRAY));

    // Screen bezel & glowing red display
    Vector3 screenBezel = { pos.x - 0.04f, pos.y + 0.10f, frontZ + 0.003f };
    DrawCube(screenBezel, 0.20f, 0.08f, 0.01f, bodyDark);
    Vector3 screenGlow = { pos.x - 0.04f, pos.y + 0.10f, frontZ + 0.007f };
    DrawCube(screenGlow, 0.16f, 0.05f, 0.005f, Color{ (unsigned char)(75 * ledGlow), 8, 8, 255 });

    // Buttons
    DrawCube(Vector3{ pos.x + 0.12f, pos.y + 0.11f, frontZ + 0.005f }, 0.035f, 0.035f, 0.01f, lit(pos, GRAY));
    DrawCube(Vector3{ pos.x + 0.12f, pos.y + 0.06f, frontZ + 0.005f }, 0.035f, 0.035f, 0.01f, Color{ 140, 20, 20, 255 });

    // Pulsing status LED
    Vector3 ledPos = { pos.x + 0.16f, pos.y + 0.21f, pos.z + 0.08f };
    DrawSphere(ledPos, 0.016f, Color{ 255, (unsigned char)(35 * ledGlow), (unsigned char)(35 * ledGlow), 255 });

    // Rust & blood streaks
    DrawCube(Vector3{ pos.x - 0.12f, pos.y + 0.08f, frontZ + 0.004f }, 0.025f, 0.14f, 0.005f, rust);
    for (int i = 0; i < 4; i++) {
        float dropY = pos.y + 0.18f - i * 0.045f - fmodf(t * 0.05f, 0.045f);
        DrawSphere(Vector3{ pos.x + 0.02f, dropY, frontZ + 0.006f }, 0.008f - i * 0.001f, Color{ 130, 10, 10, 190 });
    }
}

void DrawReceiptChain(Vector3 slotPos, Texture2D tex, float revealedSegments, float time)
{
    const int TOTAL_SEGMENTS = 16;
    const float PAPER_WIDTH  = 0.28f;
    const float PAPER_LENGTH = 0.65f;
    const float CURL_DEG_PER_SEG = 3.2f;

    float segUnit = PAPER_LENGTH / (float)TOTAL_SEGMENTS;
    int fullCount = (int)revealedSegments;
    if (fullCount > TOTAL_SEGMENTS) fullCount = TOTAL_SEGMENTS;
    float frac = revealedSegments - (float)fullCount;

    rlDisableBackfaceCulling();
    rlPushMatrix();
        rlTranslatef(slotPos.x, slotPos.y, slotPos.z);

        for (int i = 0; i < fullCount; i++)
        {
            float vTop = (float)i / (float)TOTAL_SEGMENTS;
            float vBot = (float)(i + 1) / (float)TOTAL_SEGMENTS;
            float wobble = sinf(time * 1.2f + (float)i * 0.9f) * 0.5f;

            rlPushMatrix();
                rlRotatef(wobble, 0.0f, 0.0f, 1.0f);
                rlSetTexture(tex.id);
                rlBegin(RL_QUADS);
                    rlColor4ub(255, 255, 255, 255);
                    rlNormal3f(0.0f, 0.0f, 1.0f);
                    rlTexCoord2f(0.0f, vTop); rlVertex3f(-PAPER_WIDTH * 0.5f, 0.0f,     0.0f);
                    rlTexCoord2f(0.0f, vBot); rlVertex3f(-PAPER_WIDTH * 0.5f, -segUnit, 0.0f);
                    rlTexCoord2f(1.0f, vBot); rlVertex3f( PAPER_WIDTH * 0.5f, -segUnit, 0.0f);
                    rlTexCoord2f(1.0f, vTop); rlVertex3f( PAPER_WIDTH * 0.5f, 0.0f,     0.0f);
                rlEnd();
                rlSetTexture(0);
            rlPopMatrix();

            rlTranslatef(0.0f, -segUnit, 0.0f);
            rlRotatef(-CURL_DEG_PER_SEG, 1.0f, 0.0f, 0.0f);
        }

        if (frac > 0.001f && fullCount < TOTAL_SEGMENTS)
        {
            float vTop = (float)fullCount / (float)TOTAL_SEGMENTS;
            float vBot = vTop + (1.0f / (float)TOTAL_SEGMENTS) * frac;
            float segH = segUnit * frac;
            float wobble = sinf(time * 1.2f + (float)fullCount * 0.9f) * 0.5f;

            rlPushMatrix();
                rlRotatef(wobble, 0.0f, 0.0f, 1.0f);
                rlSetTexture(tex.id);
                rlBegin(RL_QUADS);
                    rlColor4ub(255, 255, 255, 255);
                    rlNormal3f(0.0f, 0.0f, 1.0f);
                    rlTexCoord2f(0.0f, vTop); rlVertex3f(-PAPER_WIDTH * 0.5f, 0.0f,  0.0f);
                    rlTexCoord2f(0.0f, vBot); rlVertex3f(-PAPER_WIDTH * 0.5f, -segH, 0.0f);
                    rlTexCoord2f(1.0f, vBot); rlVertex3f( PAPER_WIDTH * 0.5f, -segH, 0.0f);
                    rlTexCoord2f(1.0f, vTop); rlVertex3f( PAPER_WIDTH * 0.5f, 0.0f,  0.0f);
                rlEnd();
                rlSetTexture(0);
            rlPopMatrix();
        }

    rlPopMatrix();
    rlEnableBackfaceCulling();
}

