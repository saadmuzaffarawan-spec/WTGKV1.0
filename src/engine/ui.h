// Fonts, text drawing and the shared visual language for every screen.
#pragma once
#include "common.h"

namespace ui {

// Palette (one set of tokens for menus, HUD, subtitles and world signs)
extern const Color kInk;      // bone white
extern const Color kDim;      // faded ink
extern const Color kFaint;    // barely there
extern const Color kBlood;    // dried blood accent
extern const Color kPanel;    // translucent black

enum FontId { F_MONO, F_MONO_LIGHT, F_MONO_BOLD, F_MONO_THIN, F_SIGN, F_COUNT };

void Init();
void Shutdown();
Font GetFont(FontId f);
float UiScale();   // 1.0 at 720p

// Text
Vector2 Measure(FontId f, const char* text, float size, float spacing = 0.0f);
void Text(FontId f, const char* text, float x, float y, float size, Color c, float spacing = 0.0f);
void TextCentered(FontId f, const char* text, float cx, float y, float size, Color c, float spacing = 0.0f);
void TextRight(FontId f, const char* text, float rx, float y, float size, Color c, float spacing = 0.0f);
// Text that "decays" into glyphs: amount 0 = clean, 1 = fully scrambled
void TextDecay(FontId f, const char* text, float x, float y, float size, Color c, float spacing, float amount, float time, bool centered = false);
// Word-wrapped paragraph; returns height used
float TextWrapped(FontId f, const char* text, float x, float y, float maxW, float size, Color c, float lineGap = 1.35f);

// Primitives
void Hairline(float x0, float y0, float x1, float y1, Color c);
void Panel(Rectangle r, float alpha = 1.0f);
void KeyCap(const char* key, float x, float y, float size, Color c);

Color Alpha(Color c, float a);

}  // namespace ui
