#include "ui_helpers.h"

extern Font g_fontMenu;

void DrawTextSharp(Font font, const char* text, float x, float y, float fontSize, Color tint, float spacing) {
    DrawTextEx(font, text, (Vector2){ x, y }, fontSize, spacing, tint);
}

void DrawTextSharpCentered(Font font, const char* text, float centerX, float y, float fontSize, Color tint, float spacing) {
    Vector2 sz = MeasureTextEx(font, text, fontSize, spacing);
    DrawTextEx(font, text, (Vector2){ centerX - sz.x * 0.5f, y }, fontSize, spacing, tint);
}

float MeasureTextSharp(Font font, const char* text, float fontSize, float spacing) {
    return MeasureTextEx(font, text, fontSize, spacing).x;
}

void DrawAAAPanel(Rectangle rec, Color bgColor, Color borderColor, float radius, bool shadow) {
    float roundness = (rec.height > 0.0f) ? (radius / rec.height) : 0.1f;
    if (roundness > 0.5f) roundness = 0.5f;

    if (shadow) {
        DrawRectangleRounded((Rectangle){ rec.x + 3.0f, rec.y + 5.0f, rec.width, rec.height }, roundness, 10, (Color){ 0, 0, 0, 105 });
        DrawRectangleRounded((Rectangle){ rec.x + 1.0f, rec.y + 2.0f, rec.width, rec.height }, roundness, 10, (Color){ 0, 0, 0, 155 });
    }

    DrawRectangleRounded(rec, roundness, 10, bgColor);
    DrawLine((int)(rec.x + radius), (int)(rec.y + 1.0f), (int)(rec.x + rec.width - radius), (int)(rec.y + 1.0f), (Color){ 255, 255, 255, 45 });
    DrawRectangleRoundedLinesEx(rec, roundness, 10, 1.2f, borderColor);
}

float DrawAAAKeycap(const char* key, float x, float y, Color accentCol) {
    if (!key || !key[0]) return 0.0f;
    float kw = MeasureTextSharp(g_fontMenu, key, 13.0f) + 12.0f;
    if (kw < 24.0f) kw = 24.0f;
    float kh = 22.0f;

    DrawRectangleRounded((Rectangle){ x + 1.0f, y + 2.0f, kw, kh }, 0.28f, 6, (Color){ 0, 0, 0, 160 });
    DrawRectangleRounded((Rectangle){ x, y, kw, kh }, 0.28f, 6, (Color){ 22, 26, 32, 245 });
    DrawRectangleRoundedLinesEx((Rectangle){ x, y, kw, kh }, 0.28f, 6, 1.0f, (Color){ 65, 75, 88, 230 });
    DrawLine((int)(x + 3.0f), (int)(y + 1.0f), (int)(x + kw - 3.0f), (int)(y + 1.0f), (Color){ 255, 255, 255, 55 });
    DrawTextSharpCentered(g_fontMenu, key, x + kw * 0.5f, y + 4.5f, 13.0f, accentCol, 1.0f);

    return kw;
}

void DrawAAAInteractionBadge(const char* text, Color accentCol, float centerY) {
    if (!text || !text[0]) return;
    float fs = 16.0f;
    float tw = MeasureTextSharp(g_fontMenu, text, fs);
    float pw = tw + 38.0f;
    if (pw < 220.0f) pw = 220.0f;
    float ph = 36.0f;
    float px = ((float)LOGICAL_W - pw) * 0.5f;
    float py = centerY - ph * 0.5f;

    DrawRectangleRounded((Rectangle){ px + 3.0f, py + 5.0f, pw, ph }, 0.25f, 10, (Color){ 0, 0, 0, 115 });
    DrawRectangleRounded((Rectangle){ px + 1.0f, py + 2.0f, pw, ph }, 0.25f, 10, (Color){ 0, 0, 0, 165 });
    DrawRectangleRounded((Rectangle){ px, py, pw, ph }, 0.25f, 10, (Color){ 14, 18, 22, 242 });
    DrawLine((int)(px + 10.0f), (int)(py + 1.0f), (int)(px + pw - 10.0f), (int)(py + 1.0f), (Color){ 255, 255, 255, 45 });
    DrawRectangleRoundedLinesEx((Rectangle){ px, py, pw, ph }, 0.25f, 10, 1.2f, (Color){ accentCol.r, accentCol.g, accentCol.b, 225 });
    DrawRectangleRounded((Rectangle){ px + 4.0f, py + 7.0f, 4.0f, ph - 14.0f }, 0.5f, 4, accentCol);
    DrawTextSharpCentered(g_fontMenu, text, (float)LOGICAL_W * 0.5f, py + 9.5f, fs, (Color){ 245, 248, 252, 255 }, 1.0f);
}
