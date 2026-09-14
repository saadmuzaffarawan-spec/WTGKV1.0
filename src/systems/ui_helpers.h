#pragma once

#include <raylib.h>

#ifndef LOGICAL_W
#define LOGICAL_W 1280
#endif

void DrawTextSharp(Font font, const char* text, float x, float y, float fontSize, Color tint, float spacing = 1.0f);
void DrawTextSharpCentered(Font font, const char* text, float centerX, float y, float fontSize, Color tint, float spacing = 1.0f);
float MeasureTextSharp(Font font, const char* text, float fontSize, float spacing = 1.0f);

void DrawAAAPanel(Rectangle rec, Color bgColor, Color borderColor, float radius = 6.0f, bool shadow = true);
float DrawAAAKeycap(const char* key, float x, float y, Color accentCol);
void DrawAAAInteractionBadge(const char* text, Color accentCol, float centerY);
