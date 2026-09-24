#include "materials.h"

static std::vector<SurfaceMat> g_mats;

static SurfaceMat Mk(const char* name, int tex, Color tint, float scale, float rough = 1.0f, float metal = 0.0f, float nstr = 1.0f) {
    SurfaceMat m;
    m.name = name; m.tex = tex; m.tint = tint; m.scale = scale; m.rough = rough; m.metal = metal; m.normalStr = nstr;
    return m;
}
static SurfaceMat Emissive(const char* name, Vector3 col, float strength) {
    SurfaceMat m;
    m.name = name; m.mode = MODE_EMISSIVE; m.custom = GetWhiteTexture();
    m.emissiveCol = col; m.emissive = strength; m.castShadow = false;
    return m;
}

void InitMaterials() {
    g_mats.clear();
    g_mats.resize(MAT_BUILTIN_COUNT);
    auto set = [](int id, SurfaceMat m) { g_mats[id] = m; };
    set(MAT_DEFAULT, Mk("default", TX_CONCRETE, WHITE, 0.5f));
    {
        SurfaceMat t = Mk("terrain", TX_GRASS, WHITE, 0.33f, 1.0f, 0, 1.2f);
        t.mode = MODE_TERRAIN; t.texB = TX_DIRT; t.texC = TX_GRAVEL;
        set(MAT_TERRAIN, t);
    }
    set(MAT_ROAD, Mk("road", TX_ASPHALT, Color{ 235, 235, 235, 255 }, 0.55f, 1.0f, 0, 0.45f));
    set(MAT_ASPHALT, Mk("asphalt", TX_ASPHALT, WHITE, 0.3f));
    set(MAT_CONCRETE, Mk("concrete", TX_CONCRETE, WHITE, 0.35f));
    set(MAT_CONCRETE_DARK, Mk("concrete_dark", TX_CONCRETE, Color{ 150, 145, 138, 255 }, 0.35f));
    set(MAT_CONCRETE_CURB, Mk("concrete_curb", TX_CONCRETE, Color{ 200, 198, 190, 255 }, 0.8f));
    set(MAT_BRICK, Mk("brick", TX_BRICK, WHITE, 0.9f, 1.0f, 0, 1.0f));
    set(MAT_BRICK_DARK, Mk("brick_dark", TX_BRICK, Color{ 150, 130, 125, 255 }, 0.9f));
    set(MAT_WOOD, Mk("wood", TX_WOOD, WHITE, 1.0f));
    set(MAT_WOOD_DARK, Mk("wood_dark", TX_WOOD, Color{ 120, 100, 85, 255 }, 1.0f));
    set(MAT_WOOD_FLOOR, Mk("wood_floor", TX_WOOD, Color{ 170, 140, 110, 255 }, 0.8f, 0.8f));
    set(MAT_PAINT_WHITE, Mk("paint_white", TX_PAINT, Color{ 230, 228, 220, 255 }, 0.6f));
    set(MAT_PAINT_CREAM, Mk("paint_cream", TX_PAINT, Color{ 225, 212, 180, 255 }, 0.6f));
    set(MAT_PAINT_RED, Mk("paint_red", TX_PAINT, Color{ 170, 32, 28, 255 }, 0.6f));
    set(MAT_PAINT_GREEN, Mk("paint_green", TX_PAINT, Color{ 45, 88, 60, 255 }, 0.6f));
    set(MAT_PAINT_YELLOW, Mk("paint_yellow", TX_PAINT, Color{ 220, 170, 40, 255 }, 0.6f));
    set(MAT_PAINT_BLUE, Mk("paint_blue", TX_PAINT, Color{ 40, 70, 120, 255 }, 0.6f));
    set(MAT_PAINT_GREY, Mk("paint_grey", TX_PAINT, Color{ 120, 122, 122, 255 }, 0.6f));
    set(MAT_PAINT_BLACK, Mk("paint_black", TX_PAINT, Color{ 38, 38, 40, 255 }, 0.6f));
    set(MAT_PAINT_ORANGE, Mk("paint_orange", TX_PAINT, Color{ 210, 100, 30, 255 }, 0.6f));
    set(MAT_RUST, Mk("rust", TX_RUST, WHITE, 0.8f));
    set(MAT_TILE, Mk("tile", TX_TILE, WHITE, 0.83f, 1.0f));
    set(MAT_TILE_WALL, Mk("tile_wall", TX_TILE, Color{ 210, 225, 215, 255 }, 1.6f, 0.8f));
    set(MAT_PLASTER, Mk("plaster", TX_PLASTER, WHITE, 0.35f));
    set(MAT_PLASTER_DIRTY, Mk("plaster_dirty", TX_PLASTER, Color{ 185, 175, 155, 255 }, 0.45f));
    set(MAT_CLOTH_DARK, Mk("cloth_dark", TX_CLOTH, Color{ 50, 50, 55, 255 }, 2.0f));
    set(MAT_CLOTH_BLUE, Mk("cloth_blue", TX_CLOTH, Color{ 60, 85, 130, 255 }, 2.0f));
    set(MAT_CLOTH_RED, Mk("cloth_red", TX_CLOTH, Color{ 140, 40, 38, 255 }, 2.0f));
    set(MAT_CLOTH_GREEN, Mk("cloth_green", TX_CLOTH, Color{ 70, 90, 60, 255 }, 2.0f));
    set(MAT_CLOTH_GREY, Mk("cloth_grey", TX_CLOTH, Color{ 128, 126, 122, 255 }, 2.0f));
    set(MAT_CLOTH_BROWN, Mk("cloth_brown", TX_CLOTH, Color{ 100, 78, 58, 255 }, 2.0f));
    set(MAT_CLOTH_WHITE, Mk("cloth_white", TX_CLOTH, Color{ 210, 205, 195, 255 }, 2.0f));
    set(MAT_DENIM, Mk("denim", TX_CLOTH, Color{ 58, 72, 100, 255 }, 3.0f));
    { SurfaceMat s = Mk("skin", TX_SKIN, Color{ 206, 184, 168, 255 }, 3.0f, 1.6f, 0.0f, 0.6f); s.wrap = 0.3f; set(MAT_SKIN, s); }
    { SurfaceMat s = Mk("skin_dead", TX_SKIN, Color{ 175, 185, 175, 255 }, 3.0f, 1.1f, 0.0f, 0.6f); s.wrap = 0.4f; set(MAT_SKIN_DEAD, s); }
    { SurfaceMat s = Mk("skin_grey", TX_SKIN, Color{ 110, 128, 132, 255 }, 3.0f, 0.8f, 0.0f, 0.8f); s.wrap = 0.3f; s.wet = 0.25f; set(MAT_SKIN_GREY, s); }
    set(MAT_HAIR, Mk("hair", TX_CLOTH, Color{ 30, 24, 20, 255 }, 6.0f, 0.7f));
    set(MAT_BARK, Mk("bark", TX_BARK, WHITE, 0.9f));
    set(MAT_BARK_DARK, Mk("bark_dark", TX_BARK, Color{ 110, 100, 95, 255 }, 0.9f));
    set(MAT_RUBBER, Mk("rubber", TX_RUBBER, WHITE, 2.0f));
    set(MAT_CARPAINT_BLUE, Mk("carpaint_blue", TX_CARPAINT, Color{ 40, 62, 96, 255 }, 0.5f, 1.0f, 0.2f));
    set(MAT_CARPAINT_WHITE, Mk("carpaint_white", TX_CARPAINT, Color{ 210, 208, 200, 255 }, 0.5f, 1.0f, 0.1f));
    set(MAT_CARPAINT_RED, Mk("carpaint_red", TX_CARPAINT, Color{ 110, 24, 22, 255 }, 0.5f, 1.0f, 0.2f));
    set(MAT_CARPAINT_GREEN, Mk("carpaint_green", TX_CARPAINT, Color{ 48, 70, 55, 255 }, 0.5f, 1.0f, 0.2f));
    set(MAT_CARPAINT_BEIGE, Mk("carpaint_beige", TX_CARPAINT, Color{ 170, 150, 115, 255 }, 0.5f, 1.0f, 0.1f));
    set(MAT_ROCK, Mk("rock", TX_ROCK, WHITE, 0.35f));
    { SurfaceMat s = Mk("rock_wet", TX_ROCK, Color{ 170, 160, 150, 255 }, 0.35f); s.wet = 0.5f; set(MAT_ROCK_WET, s); }
    { SurfaceMat s = Mk("flesh", TX_FLESH, WHITE, 0.5f); s.wrap = 0.4f; s.wet = 0.35f; set(MAT_FLESH, s); }
    set(MAT_STEEL, Mk("steel", TX_STEEL, WHITE, 1.0f, 1.0f, 0.9f));
    set(MAT_CHROME, Mk("chrome", TX_STEEL, Color{ 240, 240, 245, 255 }, 1.0f, 0.4f, 1.0f));
    set(MAT_CARDBOARD, Mk("cardboard", TX_CARDBOARD, WHITE, 1.5f));
    set(MAT_CORRUGATED, Mk("corrugated", TX_CORRUGATED, WHITE, 0.5f, 1.0f, 0.3f));
    set(MAT_LEATHER, Mk("leather", TX_LEATHER, WHITE, 2.0f));
    set(MAT_BONE, Mk("bone", TX_BONE, WHITE, 2.0f));
    set(MAT_MUD, Mk("mud", TX_MUD, WHITE, 0.4f));
    set(MAT_DIRT, Mk("dirt", TX_DIRT, WHITE, 0.5f));
    set(MAT_GRAVEL, Mk("gravel", TX_GRAVEL, WHITE, 0.6f));
    set(MAT_BLACK_PLASTIC, Mk("black_plastic", TX_RUBBER, Color{ 200, 200, 200, 255 }, 2.0f, 0.55f));
    set(MAT_GREY_PLASTIC, Mk("grey_plastic", TX_PAINT, Color{ 140, 140, 138, 255 }, 1.0f, 0.8f));
    {
        SurfaceMat g; g.name = "glass"; g.mode = MODE_GLASS; g.custom = GetWhiteTexture();
        g.tint = Color{ 150, 170, 175, 40 }; g.transparent = true; g.castShadow = false; g.doubleSided = true; g.rough = 0.05f;
        set(MAT_GLASS, g);
        g.name = "glass_dirty"; g.tint = Color{ 120, 125, 110, 90 }; set(MAT_GLASS_DIRTY, g);
        g.name = "glass_dark"; g.tint = Color{ 20, 24, 26, 170 }; set(MAT_GLASS_DARK, g);
    }
    set(MAT_BULB_WARM, Emissive("bulb_warm", { 1.0f, 0.78f, 0.5f }, 14.0f));
    set(MAT_BULB_SODIUM, Emissive("bulb_sodium", { 1.0f, 0.55f, 0.18f }, 18.0f));
    set(MAT_BULB_COLD, Emissive("bulb_cold", { 0.85f, 0.95f, 1.0f }, 12.0f));
    set(MAT_BULB_RED, Emissive("bulb_red", { 1.0f, 0.06f, 0.03f }, 20.0f));
    set(MAT_BULB_AMBER, Emissive("bulb_amber", { 1.0f, 0.5f, 0.05f }, 10.0f));
    set(MAT_SCREEN_GREEN, Emissive("screen_green", { 0.3f, 1.0f, 0.5f }, 2.0f));
    { SurfaceMat b = Mk("blood", TX_FLESH, Color{ 150, 14, 10, 255 }, 1.0f, 0.25f); b.wet = 0.9f; set(MAT_BLOOD, b); }
    set(MAT_BLOOD_DRY, Mk("blood_dry", TX_RUST, Color{ 80, 18, 14, 255 }, 1.5f, 0.8f));
    { SurfaceMat b = Mk("blood_smear", TX_FLESH, Color{ 120, 10, 8, 205 }, 1.2f, 0.3f); b.wet = 0.8f; b.transparent = true; b.castShadow = false; set(MAT_BLOOD_SMEAR, b); }
    { SurfaceMat e = Mk("eye", TX_BONE, Color{ 235, 225, 210, 255 }, 4.0f, 0.1f); e.wet = 1.0f; set(MAT_EYE, e); }
    set(MAT_TEETH, Mk("teeth", TX_BONE, Color{ 215, 200, 160, 255 }, 4.0f, 0.4f));
    { SurfaceMat p = Mk("pine", TX_BARK, Color{ 40, 58, 42, 255 }, 3.0f, 1.0f, 0, 0.5f); p.wrap = 0.5f; p.doubleSided = true; set(MAT_PINE, p); }
    { SurfaceMat p = Mk("foliage_dry", TX_GRASS, Color{ 150, 130, 90, 255 }, 3.0f); p.wrap = 0.5f; p.doubleSided = true; set(MAT_FOLIAGE_DRY, p); }
    { SurfaceMat p = Mk("grass_blade", TX_GRASS, Color{ 230, 225, 200, 255 }, 1.0f, 1.0f, 0, 0.3f); p.wrap = 0.6f; p.doubleSided = true; p.castShadow = false; set(MAT_GRASS_BLADE, p); }
    set(MAT_FIRE, Emissive("fire", { 1.0f, 0.45f, 0.1f }, 25.0f));
    set(MAT_TAILLIGHT, Emissive("taillight", { 1.0f, 0.05f, 0.03f }, 6.0f));
    set(MAT_HEADLIGHT_LENS, Emissive("headlight_lens", { 1.0f, 0.95f, 0.85f }, 30.0f));
    { SurfaceMat w = Mk("wax", TX_BONE, Color{ 230, 220, 190, 255 }, 3.0f, 0.5f); w.wrap = 0.6f; set(MAT_WAX, w); }
    set(MAT_PAPER, Mk("paper", TX_PLASTER, Color{ 235, 230, 215, 255 }, 3.0f));
}

int AddMaterial(const SurfaceMat& m) { g_mats.push_back(m); return (int)g_mats.size() - 1; }
SurfaceMat& Mat(int id) { return g_mats[(id >= 0 && id < (int)g_mats.size()) ? id : 0]; }
int MaterialCount() { return (int)g_mats.size(); }
int FindMaterial(const std::string& name) {
    for (size_t i = 0; i < g_mats.size(); i++) if (g_mats[i].name == name) return (int)i;
    return -1;
}
int MakeSignMaterial(Texture2D tex, bool emissive, float strength, Vector3 col) {
    SurfaceMat m;
    m.name = "sign";
    m.mode = emissive ? MODE_EMISSIVE : MODE_UV;
    m.custom = tex;
    m.emissive = strength;
    m.emissiveCol = col;
    m.castShadow = false;
    return AddMaterial(m);
}
