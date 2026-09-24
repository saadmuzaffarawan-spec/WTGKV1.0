// Surface materials: which texture set, tint, roughness, shading mode.
#pragma once
#include "common.h"
#include "textures.h"

enum MatMode { MODE_TRIPLANAR = 0, MODE_UV = 1, MODE_TERRAIN = 2, MODE_EMISSIVE = 3, MODE_GLASS = 4 };

struct SurfaceMat {
    std::string name;
    int mode = MODE_TRIPLANAR;
    int tex = TX_CONCRETE, texB = -1, texC = -1;
    Texture2D custom{};          // UV / emissive modes: custom albedo texture
    Color tint{ 255, 255, 255, 255 };
    float scale = 1.0f;          // texture repeats per metre
    float rough = 1.0f;
    float metal = 0.0f;
    float normalStr = 1.0f;
    float wet = 0.0f;
    float emissive = 0.0f;
    Vector3 emissiveCol{ 1, 1, 1 };
    float wrap = 0.0f;           // wrapped diffuse (skin / foliage)
    float alpha = 1.0f;
    bool doubleSided = false;
    bool transparent = false;
    bool castShadow = true;
};

enum MatId {
    MAT_DEFAULT,
    MAT_TERRAIN, MAT_ROAD, MAT_ASPHALT, MAT_CONCRETE, MAT_CONCRETE_DARK, MAT_CONCRETE_CURB,
    MAT_BRICK, MAT_BRICK_DARK, MAT_WOOD, MAT_WOOD_DARK, MAT_WOOD_FLOOR,
    MAT_PAINT_WHITE, MAT_PAINT_CREAM, MAT_PAINT_RED, MAT_PAINT_GREEN, MAT_PAINT_YELLOW, MAT_PAINT_BLUE,
    MAT_PAINT_GREY, MAT_PAINT_BLACK, MAT_PAINT_ORANGE,
    MAT_RUST, MAT_TILE, MAT_TILE_WALL, MAT_PLASTER, MAT_PLASTER_DIRTY,
    MAT_CLOTH_DARK, MAT_CLOTH_BLUE, MAT_CLOTH_RED, MAT_CLOTH_GREEN, MAT_CLOTH_GREY, MAT_CLOTH_BROWN,
    MAT_CLOTH_WHITE, MAT_DENIM,
    MAT_SKIN, MAT_SKIN_DEAD, MAT_SKIN_GREY, MAT_HAIR,
    MAT_BARK, MAT_BARK_DARK, MAT_RUBBER, MAT_CARPAINT_BLUE, MAT_CARPAINT_WHITE, MAT_CARPAINT_RED,
    MAT_CARPAINT_GREEN, MAT_CARPAINT_BEIGE,
    MAT_ROCK, MAT_ROCK_WET, MAT_FLESH, MAT_STEEL, MAT_CHROME, MAT_CARDBOARD, MAT_CORRUGATED,
    MAT_LEATHER, MAT_BONE, MAT_MUD, MAT_DIRT, MAT_GRAVEL, MAT_BLACK_PLASTIC, MAT_GREY_PLASTIC,
    MAT_GLASS, MAT_GLASS_DIRTY, MAT_GLASS_DARK,
    MAT_BULB_WARM, MAT_BULB_SODIUM, MAT_BULB_COLD, MAT_BULB_RED, MAT_BULB_AMBER, MAT_SCREEN_GREEN,
    MAT_BLOOD, MAT_BLOOD_DRY, MAT_BLOOD_SMEAR, MAT_EYE, MAT_TEETH, MAT_PINE, MAT_FOLIAGE_DRY, MAT_GRASS_BLADE,
    MAT_FIRE, MAT_TAILLIGHT, MAT_HEADLIGHT_LENS, MAT_WAX, MAT_PAPER,
    MAT_BUILTIN_COUNT
};

void InitMaterials();
int  AddMaterial(const SurfaceMat& m);
SurfaceMat& Mat(int id);
int  MaterialCount();
int  FindMaterial(const std::string& name);
// Create an emissive or UV material from a texture (signs, screens, posters)
int  MakeSignMaterial(Texture2D tex, bool emissive, float strength, Vector3 emissiveCol = { 1, 1, 1 });
