#include "ui.h"
#include <cstring>

namespace ui {

const Color kInk{ 216, 210, 198, 255 };
const Color kDim{ 150, 145, 136, 255 };
const Color kFaint{ 92, 88, 82, 255 };
const Color kBlood{ 150, 28, 24, 255 };
const Color kPanel{ 6, 6, 7, 205 };

static Font g_fonts[F_COUNT];
static const int kBase = 64;

static Font LoadF(const char* file, int size) {
    const char* path = TextFormat("assets/fonts/%s", file);
    Font f = FileExists(path) ? LoadFontEx(path, size, nullptr, 0) : GetFontDefault();
    if (f.texture.id == 0) f = GetFontDefault();
    GenTextureMipmaps(&f.texture);
    SetTextureFilter(f.texture, TEXTURE_FILTER_TRILINEAR);
    return f;
}

void Init() {
    g_fonts[F_MONO] = LoadF("IBMPlexMono-Regular.ttf", kBase);
    g_fonts[F_MONO_LIGHT] = LoadF("IBMPlexMono-Light.ttf", kBase);
    g_fonts[F_MONO_BOLD] = LoadF("IBMPlexMono-SemiBold.ttf", kBase);
    g_fonts[F_MONO_THIN] = LoadF("IBMPlexMono-ExtraLight.ttf", 96);
    g_fonts[F_SIGN] = LoadF("IBMPlexMono-Bold.ttf", 96);
}

void Shutdown() {
    for (auto& f : g_fonts) if (f.texture.id && f.texture.id != GetFontDefault().texture.id) UnloadFont(f);
}

Font GetFont(FontId f) { return g_fonts[f]; }
float UiScale() { return GetScreenHeight() / 720.0f; }
Color Alpha(Color c, float a) { c.a = (unsigned char)(c.a * Saturate(a)); return c; }

Vector2 Measure(FontId f, const char* text, float size, float spacing) {
    return MeasureTextEx(g_fonts[f], text, size, spacing);
}
void Text(FontId f, const char* text, float x, float y, float size, Color c, float spacing) {
    DrawTextEx(g_fonts[f], text, Vector2{ roundf(x), roundf(y) }, size, spacing, c);
}
void TextCentered(FontId f, const char* text, float cx, float y, float size, Color c, float spacing) {
    Vector2 m = Measure(f, text, size, spacing);
    Text(f, text, cx - m.x * 0.5f, y, size, c, spacing);
}
void TextRight(FontId f, const char* text, float rx, float y, float size, Color c, float spacing) {
    Vector2 m = Measure(f, text, size, spacing);
    Text(f, text, rx - m.x, y, size, c, spacing);
}

void TextDecay(FontId f, const char* text, float x, float y, float size, Color c, float spacing, float amount, float time, bool centered) {
    static const char glyphs[] = ".,:;-~=+*#%@/\\|";
    char buf[512];
    size_t n = strlen(text);
    if (n >= sizeof(buf)) n = sizeof(buf) - 1;
    for (size_t i = 0; i < n; i++) {
        char ch = text[i];
        uint32_t h = HashU32((uint32_t)i * 7919u + (uint32_t)(time * 9.0f) * 104729u);
        float r = (h & 0xffff) / 65536.0f;
        if (ch != ' ' && r < amount) ch = glyphs[(h >> 16) % (sizeof(glyphs) - 1)];
        buf[i] = ch;
    }
    buf[n] = 0;
    if (centered) TextCentered(f, buf, x, y, size, c, spacing);
    else Text(f, buf, x, y, size, c, spacing);
}

float TextWrapped(FontId f, const char* text, float x, float y, float maxW, float size, Color c, float lineGap) {
    std::string line, word;
    float cy = y;
    auto flush = [&]() {
        if (!line.empty()) Text(f, line.c_str(), x, cy, size, c);
        cy += size * lineGap;
        line.clear();
    };
    std::string s(text);
    for (size_t i = 0; i <= s.size(); i++) {
        char ch = i < s.size() ? s[i] : ' ';
        if (ch == '\n') {
            std::string test = line.empty() ? word : line + " " + word;
            line = test; word.clear(); flush();
            continue;
        }
        if (ch == ' ') {
            if (word.empty()) continue;
            std::string test = line.empty() ? word : line + " " + word;
            if (Measure(f, test.c_str(), size).x > maxW && !line.empty()) { flush(); line = word; }
            else line = test;
            word.clear();
        } else word += ch;
    }
    if (!line.empty()) flush();
    return cy - y;
}

void Hairline(float x0, float y0, float x1, float y1, Color c) { DrawLineEx({ x0, y0 }, { x1, y1 }, fmaxf(1.0f, UiScale()), c); }

void Panel(Rectangle r, float alpha) {
    DrawRectangleRec(r, Alpha(kPanel, alpha));
    DrawRectangleLinesEx(r, 1.0f, Alpha(Color{ 60, 58, 54, 255 }, alpha * 0.6f));
}

void KeyCap(const char* key, float x, float y, float size, Color c) {
    Vector2 m = Measure(F_MONO_BOLD, key, size * 0.8f);
    float w = fmaxf(m.x + size * 0.6f, size * 1.3f), h = size * 1.3f;
    DrawRectangleRounded({ x, y, w, h }, 0.25f, 4, Alpha(Color{ 20, 20, 22, 255 }, c.a / 255.0f * 0.8f));
    DrawRectangleRoundedLinesEx({ x, y, w, h }, 0.25f, 4, 1.0f, Alpha(c, 0.6f));
    Text(F_MONO_BOLD, key, x + (w - m.x) * 0.5f, y + (h - m.y) * 0.5f, size * 0.8f, c);
}

}  // namespace ui
