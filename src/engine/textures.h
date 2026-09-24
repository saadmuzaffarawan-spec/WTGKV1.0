// Procedural PBR-ish texture sets (albedo+roughness, normal+AO), tileable.
// A file at assets/textures/<name>_albedo.png (+ optional _normal.png) overrides
// the generated version, so photo-scanned textures can be dropped in later.
#pragma once
#include "common.h"

enum TexId {
    TX_ASPHALT, TX_CONCRETE, TX_DIRT, TX_GRASS, TX_GRAVEL, TX_BRICK, TX_WOOD, TX_PAINT,
    TX_RUST, TX_TILE, TX_PLASTER, TX_CLOTH, TX_SKIN, TX_BARK, TX_RUBBER, TX_CARPAINT,
    TX_ROCK, TX_FLESH, TX_STEEL, TX_CARDBOARD, TX_CORRUGATED, TX_LEATHER, TX_BONE, TX_MUD,
    TX_COUNT
};

struct TexSet {
    const char* name;
    Texture2D albedo;   // rgb albedo (sRGB-ish), a = roughness
    Texture2D normal;   // rgb tangent normal, a = ambient occlusion
};

void GenerateTextureSets(int size, void (*progress)(float, const char*) = nullptr);
const TexSet& GetTexSet(int id);
void UnloadTextureSets();

// Small utility textures
Texture2D GetWhiteTexture();
Texture2D GetFlatNormalTexture();
Texture2D MakeTextTexture(const char* text, Font font, float fontSize, Color fg, Color bg, int padX, int padY, float spacing = 1.0f);
Texture2D MakeRadialTexture(int size, float softness);  // white disc falloff (glows, blob shadows)
