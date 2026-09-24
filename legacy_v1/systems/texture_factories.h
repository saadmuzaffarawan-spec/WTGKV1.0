#pragma once

#include <raylib.h>
#include <raymath.h>

RenderTexture2D GenerateGlyphTexture(const char* glyph, Color bg, Color fg, int texSize, int fontSize, int jitter);
RenderTexture2D GenerateCrustTexture(int texSize);
Model MakeTexturedCylinder(float radius, float height, int slices, Texture2D tex);
Model MakeTexturedSphere(float radius, int rings, int slices, Texture2D tex);
