// Grethnar's store: building shell and interior fixtures.
//
// Local frame: front (glass storefront) faces +Z at z = +6, x spans -8..8, floor top at y = 0.15.
#include "prefab_util.h"

static const float kFloor = 0.15f;

static void BuildStore(PrefabBuild& b, const Entity& e) {
    const float H = 3.6f, T = 0.25f;
    // slab
    b.M(MAT_TILE).Box({ 0, kFloor - 0.1f, 0 }, { 16.0f, 0.2f, 12.0f });
    b.Collider({ 0, kFloor - 0.1f, 0 }, { 16.0f, 0.2f, 12.0f }, SURF_TILE);
    b.M(MAT_CONCRETE).Box({ 0, kFloor - 0.12f, 6.9f }, { 17.0f, 0.24f, 1.8f }, 0.03f);   // front sidewalk step
    b.Collider({ 0, kFloor - 0.12f, 6.9f }, { 17.0f, 0.24f, 1.8f }, SURF_CONCRETE);
    // exterior walls
    std::vector<Opening> front = { { 3.2f, 5.3f, 0.75f, 2.6f }, { 7.6f, 1.8f, 0.0f, 2.3f }, { 12.4f, 6.4f, 0.75f, 2.6f } };
    BuildWall(b, { -8, 6 }, { 8, 6 }, kFloor, H, T, MAT_BRICK_DARK, MAT_PLASTER, front, SURF_CONCRETE, 0.1f);
    BuildWall(b, { 8, 6 }, { 8, -6 }, kFloor, H, T, MAT_PLASTER_DIRTY, MAT_PLASTER, {}, SURF_CONCRETE, 0.1f);
    BuildWall(b, { 8, -6 }, { -8, -6 }, kFloor, H, T, MAT_PLASTER_DIRTY, MAT_PLASTER, { { 2.5f, 1.0f, 0.0f, 2.2f } }, SURF_CONCRETE, 0.1f);
    BuildWall(b, { -8, -6 }, { -8, 6 }, kFloor, H, T, MAT_PLASTER_DIRTY, MAT_PLASTER, { { 8.0f, 1.2f, 1.6f, 2.3f } }, SURF_CONCRETE, 0.1f);
    BuildWindow(b, { -8, 6 }, { 8, 6 }, kFloor, front[0], MAT_GLASS_DIRTY, MAT_STEEL, 3);
    BuildWindow(b, { -8, 6 }, { 8, 6 }, kFloor, front[2], MAT_GLASS_DIRTY, MAT_STEEL, 3);
    BuildWindow(b, { -8, -6 }, { -8, 6 }, kFloor, { 8.0f, 1.2f, 1.6f, 2.3f }, MAT_GLASS_DIRTY, MAT_WOOD_DARK, 1);
    // brick wainscot on the exterior sides
    for (int s = -1; s <= 1; s += 2) b.M(MAT_BRICK_DARK).Box({ s * 8.14f, kFloor + 0.5f, 0 }, { 0.03f, 1.0f, 12.2f });
    // storage room partition
    BuildWall(b, { 8, -2.2f }, { 2, -2.2f }, kFloor, 3.0f, 0.12f, MAT_PLASTER, MAT_PLASTER, { { 4.5f, 0.9f, 0.0f, 2.1f } }, SURF_CONCRETE, 0.08f);
    BuildWall(b, { 2, -2.2f }, { 2, -6.0f }, kFloor, 3.0f, 0.12f, MAT_PLASTER, MAT_PLASTER, {}, SURF_CONCRETE, 0.08f);
    b.M(MAT_CONCRETE_DARK).Box({ 5.0f, kFloor + 0.004f, -4.1f }, { 5.8f, 0.01f, 3.6f });   // bare concrete in storage
    // drop ceiling + roof
    b.M(MAT_PLASTER).Box({ 0, kFloor + 3.0f, 0 }, { 15.6f, 0.05f, 11.6f });
    for (float x = -7.2f; x < 8; x += 1.2f) b.M(MAT_STEEL).Box({ x, kFloor + 2.97f, 0 }, { 0.025f, 0.012f, 11.6f });
    for (float z = -5.4f; z < 6; z += 0.6f) b.M(MAT_STEEL).Box({ 0, kFloor + 2.97f, z }, { 15.6f, 0.012f, 0.025f });
    // a few missing / stained ceiling tiles
    b.M(MAT_BLACK_PLASTIC).Box({ -3.0f, kFloor + 2.968f, -3.3f }, { 1.15f, 0.004f, 0.55f });
    b.M(MAT_BLOOD_DRY).Box({ 1.8f, kFloor + 2.968f, 1.5f }, { 1.1f, 0.004f, 0.5f });
    b.M(MAT_CONCRETE_DARK).Box({ 0, H + kFloor + 0.15f, 0 }, { 16.5f, 0.3f, 12.5f }, 0.02f);
    b.Collider({ 0, H + kFloor + 0.15f, 0 }, { 16.5f, 0.3f, 12.5f }, SURF_CONCRETE);
    // parapet + coping
    b.M(MAT_BRICK_DARK).Box({ 0, H + kFloor + 0.55f, 6.1f }, { 16.5f, 0.8f, 0.3f }, 0.02f);
    b.M(MAT_CONCRETE).Box({ 0, H + kFloor + 0.98f, 6.1f }, { 16.7f, 0.08f, 0.36f }, 0.02f);
    // front sign band + GRETHNAR'S neon
    b.M(MAT_PAINT_CREAM).Box({ 0, kFloor + 3.15f, 6.2f }, { 15.8f, 0.9f, 0.16f }, 0.03f);
    int neon = SignMaterial("store_neon", "GRETHNAR'S", ui::F_SIGN, 150, Color{ 255, 60, 40, 255 }, Color{ 0, 0, 0, 255 }, 1024, 170, 0.1f, true, 7.0f);
    b.M(neon).BoxUV({ -2.0f, kFloor + 3.18f, 6.285f }, { 6.4f, 0.8f, 0.003f });
    int sub = SignMaterial("store_sub", "BAIT  .  GROCERIES  .  COLD BEER  .  ICE", ui::F_MONO_BOLD, 44, Color{ 60, 40, 30, 255 }, Color{ 225, 212, 180, 255 }, 1024, 64, 0.8f);
    b.M(sub).BoxUV({ 4.6f, kFloor + 3.18f, 6.285f }, { 6.0f, 0.36f, 0.003f });
    LightDef& signL = b.AddLight({ -2.0f, kFloor + 3.2f, 7.0f }, { 1.0f, 0.2f, 0.12f }, 5.0f, 9.0f, 0.35f);
    signL.group = "store_sign"; signL.flicker = 0.12f;
    // OPEN neon in the window
    int open = SignMaterial("open_neon", "OPEN", ui::F_SIGN, 110, Color{ 255, 40, 60, 255 }, Color{ 0, 0, 0, 255 }, 360, 130, 0.0f, true, 6.0f);
    b.M(open).BoxUV({ 4.2f, kFloor + 1.95f, 5.82f }, { 0.9f, 0.32f, 0.003f });
    LightDef& openL = b.AddLight({ 4.2f, kFloor + 1.95f, 5.4f }, { 1.0f, 0.12f, 0.2f }, 2.2f, 5.0f, 0.2f);
    openL.group = "open_sign"; openL.flicker = 0.25f;
    // awning over the entrance
    b.M(MAT_PAINT_RED).BoxRot({ -0.4f, kFloor + 2.55f, 6.6f }, { 3.0f, 0.05f, 1.0f }, { 14, 0, 0 }, 0.01f);
    // roof AC unit and vents
    b.M(MAT_PAINT_GREY).Box({ 3.0f, H + kFloor + 0.9f, -2.0f }, { 1.8f, 1.0f, 1.2f }, 0.04f);
    b.M(MAT_STEEL).Cylinder({ 3.0f, H + kFloor + 1.4f, -2.0f }, { 3.0f, H + kFloor + 1.45f, -2.0f }, 0.45f, 0.45f, 18, true);
    b.M(MAT_STEEL).Cylinder({ -4.0f, H + kFloor + 0.3f, -3.5f }, { -4.0f, H + kFloor + 1.1f, -3.5f }, 0.08f, 0.08f, 10, true);
    // troffer lights (store group; one tube dying)
    Vector2 tro[6] = { { -5.5f, 3.6f }, { -5.5f, -0.4f }, { -5.5f, -4.2f }, { -1.0f, 3.6f }, { -1.0f, -0.4f }, { 4.8f, 2.4f } };
    for (int i = 0; i < 6; i++) {
        Vector3 p{ tro[i].x, kFloor + 2.99f, tro[i].y };
        b.M(MAT_STEEL).Box({ p.x, p.y, p.z }, { 0.62f, 0.05f, 1.22f }, 0.005f);
        b.M(MAT_BULB_COLD).Box({ p.x, p.y - 0.03f, p.z }, { 0.52f, 0.01f, 1.12f });
        LightDef& l = b.AddSpot({ p.x, p.y - 0.06f, p.z }, { 0, -1, 0 }, { 0.9f, 0.96f, 1.0f }, 9.0f, 8.5f, 60.0f, 88.0f, 0.04f);
        l.group = "store";
        if (i == 4) l.flicker = 0.55f;
    }
    // storage bulb
    b.M(MAT_BULB_WARM).Sphere({ 5.0f, kFloor + 2.6f, -4.2f }, 0.05f, 6, 8);
    b.M(MAT_BLACK_PLASTIC).Cylinder({ 5.0f, kFloor + 2.65f, -4.2f }, { 5.0f, kFloor + 3.0f, -4.2f }, 0.01f, 0.01f, 5, false);
    LightDef& bulb = b.AddLight({ 5.0f, kFloor + 2.5f, -4.2f }, { 1.0f, 0.72f, 0.42f }, 2.8f, 7.0f, 0.1f);
    bulb.group = "storage";
    (void)e;
}

static void StoreTerrain(const Entity& e, std::vector<TerrainPad>& pads, std::vector<Rectangle>&) {
    TerrainPad p; p.c = { e.pos.x, e.pos.z }; p.half = { 8.5f, 7.8f }; p.yaw = e.rot.x * DEG2RAD; p.margin = 4.0f; p.splat = 1;
    pads.push_back(p);
}

// ---------------------------------------------------------------------------
// Interior fixtures
// ---------------------------------------------------------------------------
static void Products(PrefabBuild& b, Rng& rng, Vector3 origin, float len, float depth, float shelfH, int rows) {
    const Color palette[] = { { 200, 40, 35, 255 }, { 230, 190, 50, 255 }, { 40, 90, 160, 255 }, { 225, 225, 215, 255 },
                              { 60, 130, 70, 255 }, { 210, 110, 40, 255 }, { 120, 40, 110, 255 }, { 30, 30, 30, 255 } };
    for (int r = 0; r < rows; r++) {
        float y = origin.y + r * shelfH;
        float x = origin.x - len * 0.5f + 0.04f;
        while (x < origin.x + len * 0.5f - 0.08f) {
            int kind = rng.RangeI(0, 3);
            Color c = palette[rng.RangeI(0, 7)];
            int facings = rng.RangeI(2, 5);
            if (rng.Chance(0.12f)) { x += rng.Range(0.1f, 0.35f); continue; }   // gaps: the store is half empty
            for (int f = 0; f < facings && x < origin.x + len * 0.5f - 0.08f; f++) {
                b.mb.SetColor(c);
                if (kind == 0) {        // cereal-style boxes
                    float w = 0.07f, h = rng.Range(0.22f, 0.3f);
                    for (int d = 0; d < 2; d++) b.M(MAT_CARDBOARD).Box({ x + w * 0.5f, y + h * 0.5f, origin.z + depth * 0.25f - d * 0.2f }, { w, h, 0.18f }, 0.004f);
                    x += w + 0.008f;
                } else if (kind == 1) { // cans
                    float r2 = 0.034f;
                    for (int d = 0; d < 3; d++) b.M(MAT_STEEL).Cylinder({ x + r2, y, origin.z + depth * 0.3f - d * 0.075f }, { x + r2, y + 0.12f, origin.z + depth * 0.3f - d * 0.075f }, r2, r2, 10, true);
                    x += r2 * 2 + 0.006f;
                } else if (kind == 2) { // bottles
                    float r2 = 0.035f;
                    for (int d = 0; d < 2; d++) {
                        Vector3 p{ x + r2, y, origin.z + depth * 0.3f - d * 0.08f };
                        b.M(MAT_GREY_PLASTIC).Lathe(p, { { r2, 0 }, { r2, 0.17f }, { 0.012f, 0.23f }, { 0.012f, 0.27f } }, 10, true, true);
                    }
                    x += r2 * 2 + 0.01f;
                } else {                // bags / packets
                    float w = 0.12f;
                    b.M(MAT_BLACK_PLASTIC).BoxRot({ x + w * 0.5f, y + 0.1f, origin.z + depth * 0.25f }, { w, 0.2f, 0.06f }, { rng.Signed() * 8, 0, rng.Signed() * 5 }, 0.02f);
                    x += w + 0.01f;
                }
            }
        }
    }
    b.mb.SetColor(WHITE);
}

static void BuildShelf(PrefabBuild& b, const Entity& e) {
    // Double-sided gondola, 3.6 m long, 1.5 m tall
    Rng rng((uint32_t)e.Num("variant", 0) * 131u + 7u);
    const float L = e.Num("len", 3.6f), H = 1.5f, D = 0.9f;
    b.Solid(MAT_PAINT_WHITE, { 0, 0.07f, 0 }, { L, 0.14f, D }, 0.01f, SURF_METAL);
    b.M(MAT_PAINT_GREY).Box({ 0, H * 0.5f, 0 }, { L, H, 0.05f }, 0.005f);
    b.Collider({ 0, H * 0.5f, 0 }, { L, H, D }, SURF_METAL);
    for (int s = -1; s <= 1; s += 2) b.M(MAT_PAINT_WHITE).Box({ s * L * 0.5f, H * 0.5f, 0 }, { 0.04f, H, D }, 0.005f);
    for (int side = -1; side <= 1; side += 2) {
        for (int k = 0; k < 4; k++) {
            float y = 0.2f + k * 0.34f;
            b.M(MAT_PAINT_WHITE).Box({ 0, y, side * D * 0.25f }, { L - 0.05f, 0.02f, D * 0.45f }, 0.003f);
            b.M(MAT_PAINT_RED).Box({ 0, y + 0.015f, side * D * 0.47f }, { L - 0.05f, 0.035f, 0.005f });   // price rail
        }
        b.mb.Push(); if (side < 0) b.mb.RotateY(180);
        Products(b, rng, { 0, 0.21f, D * 0.25f }, L, D * 0.45f, 0.34f, 4);
        b.mb.Pop();
    }
}

static void BuildCoolers(PrefabBuild& b, const Entity& e) {
    // Row of 4 glass-door coolers against a wall (front faces +Z)
    const int n = (int)e.Num("doors", 4);
    const float W = 0.78f, H = 2.1f, D = 0.8f;
    float L = n * W;
    b.Solid(MAT_PAINT_WHITE, { 0, H * 0.5f, -D * 0.5f }, { L + 0.1f, H + 0.2f, D }, 0.01f, SURF_METAL);
    Rng rng(77);
    for (int i = 0; i < n; i++) {
        float x = -L * 0.5f + W * (i + 0.5f);
        b.M(MAT_BLACK_PLASTIC).Box({ x, H * 0.5f + 0.05f, 0.01f }, { W - 0.02f, H - 0.1f, 0.04f }, 0.01f);
        b.M(MAT_GLASS).Box({ x, H * 0.5f + 0.05f, 0.035f }, { W - 0.12f, H - 0.2f, 0.01f });
        b.M(MAT_CHROME).Box({ x + W * 0.35f, H * 0.5f, 0.07f }, { 0.03f, 0.9f, 0.04f }, 0.01f);
        // interior lit column + product rows
        b.M(MAT_BULB_COLD).Box({ x - W * 0.45f, H * 0.5f, -0.1f }, { 0.02f, H - 0.3f, 0.02f });
        for (int s = 0; s < 5; s++) {
            float y = 0.2f + s * 0.38f;
            b.M(MAT_STEEL).Box({ x, y, -0.35f }, { W - 0.1f, 0.015f, 0.6f });
            for (int k = 0; k < 5; k++) {
                if (rng.Chance(0.2f)) continue;
                Color c = rng.Chance(0.5f) ? Color{ 180, 30, 30, 255 } : (rng.Chance(0.5f) ? Color{ 40, 110, 50, 255 } : Color{ 220, 200, 140, 255 });
                b.mb.SetColor(c);
                b.M(MAT_GREY_PLASTIC).Lathe({ x - W * 0.35f + k * 0.14f, y + 0.01f, -0.12f }, { { 0.035f, 0 }, { 0.035f, 0.2f }, { 0.013f, 0.27f }, { 0.013f, 0.3f } }, 8, true, true);
                b.mb.SetColor(WHITE);
            }
        }
        LightDef& l = b.AddLight({ x, H * 0.6f, 0.3f }, { 0.8f, 0.92f, 1.0f }, 1.3f, 3.5f, 0.0f);
        l.group = "store";
    }
    int s = SignMaterial("cold_beer", "COLD  DRINKS", ui::F_SIGN, 64, Color{ 225, 230, 240, 255 }, Color{ 25, 55, 120, 255 }, 512, 80, 0.5f, true, 0.8f);
    b.M(s).BoxUV({ 0, H + 0.2f, 0.005f }, { L * 0.8f, 0.18f, 0.004f });
}

static void BuildCounter(PrefabBuild& b, const Entity& e) {
    // L-shaped checkout counter; customer side faces +Z, clerk stands at -Z
    b.Solid(MAT_WOOD_DARK, { 0, 0.5f, 0 }, { 3.2f, 1.0f, 0.7f }, 0.02f, SURF_WOOD);
    b.Solid(MAT_WOOD_DARK, { 1.95f, 0.5f, -1.0f }, { 0.7f, 1.0f, 2.7f }, 0.02f, SURF_WOOD);
    b.M(MAT_PAINT_GREY).Box({ 0, 1.02f, 0 }, { 3.3f, 0.04f, 0.78f }, 0.01f);
    b.M(MAT_PAINT_GREY).Box({ 1.95f, 1.02f, -1.0f }, { 0.78f, 0.04f, 2.8f }, 0.01f);
    // cash register
    b.M(MAT_PAINT_CREAM).BoxRot({ -0.6f, 1.14f, -0.05f }, { 0.45f, 0.2f, 0.4f }, { -8, 0, 0 }, 0.02f);
    b.M(MAT_PAINT_CREAM).Box({ -0.6f, 1.34f, -0.15f }, { 0.35f, 0.2f, 0.12f }, 0.02f);
    b.M(MAT_SCREEN_GREEN).Box({ -0.6f, 1.36f, -0.085f }, { 0.25f, 0.07f, 0.005f });
    for (int r = 0; r < 4; r++) for (int c = 0; c < 5; c++)
        b.M(MAT_GREY_PLASTIC).Box({ -0.72f + c * 0.055f, 1.22f + r * 0.005f, 0.05f - r * 0.05f }, { 0.04f, 0.015f, 0.035f }, 0.004f);
    // receipt printer + roll
    b.M(MAT_BLACK_PLASTIC).Box({ -0.15f, 1.1f, -0.1f }, { 0.18f, 0.12f, 0.2f }, 0.02f);
    b.M(MAT_PAPER).Box({ -0.15f, 1.19f, -0.02f }, { 0.07f, 0.1f, 0.002f });
    // lottery ticket display, candy, lighter jar
    b.M(MAT_GLASS).Box({ 0.7f, 1.2f, 0.1f }, { 0.5f, 0.3f, 0.3f });
    Rng rng(3);
    for (int i = 0; i < 12; i++) {
        b.mb.SetColor(Color{ (unsigned char)rng.RangeI(80, 230), (unsigned char)rng.RangeI(40, 200), (unsigned char)rng.RangeI(30, 160), 255 });
        b.M(MAT_CARDBOARD).Box({ 0.5f + (i % 4) * 0.12f, 1.07f + (i / 4) * 0.08f, 0.1f }, { 0.1f, 0.06f, 0.2f }, 0.005f);
    }
    b.mb.SetColor(WHITE);
    b.M(MAT_GLASS).Cylinder({ 1.2f, 1.04f, 0.15f }, { 1.2f, 1.22f, 0.15f }, 0.07f, 0.07f, 12, false);
    // bell
    b.M(MAT_CHROME).Lathe({ 1.0f, 1.04f, -0.15f }, { { 0.04f, 0 }, { 0.035f, 0.02f }, { 0.02f, 0.04f }, { 0.004f, 0.05f } }, 12, true, true);
    // cigarette rack on the wall behind the clerk
    b.M(MAT_WOOD_DARK).Box({ 0, 1.9f, -1.35f }, { 2.4f, 1.2f, 0.25f }, 0.02f);
    for (int r = 0; r < 5; r++) for (int c = 0; c < 18; c++) {
        Color cc = (c % 3 == 0) ? Color{ 200, 40, 35, 255 } : ((c % 3 == 1) ? Color{ 230, 230, 220, 255 } : Color{ 40, 80, 150, 255 });
        if (rng.Chance(0.15f)) continue;
        b.mb.SetColor(cc);
        b.M(MAT_CARDBOARD).Box({ -1.1f + c * 0.13f, 1.42f + r * 0.2f, -1.25f }, { 0.09f, 0.15f, 0.06f }, 0.004f);
    }
    b.mb.SetColor(WHITE);
    b.Collider({ 0, 1.9f, -1.35f }, { 2.4f, 1.2f, 0.25f }, SURF_WOOD);
    b.Interact("Register", { -0.6f, 1.2f, 0.0f }, 1.8f, "register");
    (void)e;
}

static void BuildStorageShelf(PrefabBuild& b, const Entity& e) {
    Rng rng((uint32_t)e.Num("variant", 0) * 17u + 3u);
    const float L = 2.0f, D = 0.6f, H = 2.0f;
    for (int s = -1; s <= 1; s += 2) for (int k = -1; k <= 1; k += 2)
        b.M(MAT_STEEL).Box({ s * L * 0.48f, H * 0.5f, k * D * 0.45f }, { 0.04f, H, 0.04f });
    for (int lvl = 0; lvl < 4; lvl++) {
        float y = 0.12f + lvl * 0.6f;
        b.M(MAT_STEEL).Box({ 0, y, 0 }, { L, 0.03f, D }, 0.005f);
        float x = -L * 0.45f;
        while (x < L * 0.4f) {
            float w = rng.Range(0.25f, 0.55f), h = rng.Range(0.2f, 0.45f);
            if (!rng.Chance(0.2f)) b.M(MAT_CARDBOARD).BoxRot({ x + w * 0.5f, y + h * 0.5f + 0.015f, rng.Signed() * 0.05f }, { w, h, D * 0.8f }, { 0, rng.Signed() * 6, 0 }, 0.01f);
            x += w + 0.03f;
        }
    }
    b.Collider({ 0, H * 0.5f, 0 }, { L, H, D }, SURF_METAL);
}

static void BuildMopBucket(PrefabBuild& b, const Entity& e) {
    b.M(MAT_PAINT_YELLOW).Box({ 0, 0.25f, 0 }, { 0.45f, 0.4f, 0.35f }, 0.04f);
    b.M(MAT_GREY_PLASTIC).Box({ 0.12f, 0.5f, 0 }, { 0.14f, 0.15f, 0.32f }, 0.02f);
    for (int s = -1; s <= 1; s += 2) for (int k = -1; k <= 1; k += 2) b.M(MAT_BLACK_PLASTIC).Sphere({ s * 0.18f, 0.03f, k * 0.14f }, 0.03f, 4, 6);
    b.M(MAT_WOOD).Cylinder({ -0.08f, 0.1f, 0 }, { -0.25f, 1.45f, 0.05f }, 0.014f, 0.014f, 6, true);
    b.M(MAT_CLOTH_GREY).Cylinder({ -0.08f, 0.08f, 0 }, { -0.1f, 0.35f, 0 }, 0.09f, 0.05f, 10, true);
    b.Collider({ 0, 0.25f, 0 }, { 0.5f, 0.5f, 0.4f }, SURF_METAL);
    b.Interact("Take mop", { 0, 0.6f, 0 }, 1.8f, "mop");
    (void)e;
}

static void BuildBreakerBox(PrefabBuild& b, const Entity& e) {
    b.M(MAT_PAINT_GREY).Box({ 0, 1.5f, 0.06f }, { 0.5f, 0.75f, 0.12f }, 0.01f);
    b.M(MAT_PAINT_GREY).Box({ 0, 1.5f, 0.125f }, { 0.46f, 0.7f, 0.01f }, 0.005f);
    b.M(MAT_STEEL).Box({ 0.2f, 1.5f, 0.135f }, { 0.02f, 0.08f, 0.02f });
    b.M(MAT_STEEL).Cylinder({ 0, 1.87f, 0.06f }, { 0, 3.0f, 0.06f }, 0.025f, 0.025f, 8, false);
    int s = SignMaterial("danger", "DANGER\nHIGH VOLTAGE", ui::F_MONO_BOLD, 30, Color{ 20, 20, 20, 255 }, Color{ 230, 200, 40, 255 }, 256, 100, 0.6f);
    b.M(s).BoxUV({ 0, 1.72f, 0.132f }, { 0.3f, 0.12f, 0.002f });
    b.Interact("Breaker panel", { 0, 1.5f, 0.15f }, 1.6f, "breaker");
    (void)e;
}

static void BuildBoxes(PrefabBuild& b, const Entity& e) {
    Rng rng((uint32_t)e.Num("variant", 0) * 19u + 5u);
    int n = rng.RangeI(3, 7);
    float y = 0;
    for (int i = 0; i < n; i++) {
        float w = rng.Range(0.35f, 0.6f), h = rng.Range(0.25f, 0.45f), d = rng.Range(0.3f, 0.5f);
        Vector3 c{ rng.Signed() * 0.25f, 0, rng.Signed() * 0.25f };
        if (i > 0 && rng.Chance(0.6f)) { c.y = y; } else { c.y = 0; }
        b.M(MAT_CARDBOARD).BoxRot({ c.x, c.y + h * 0.5f, c.z }, { w, h, d }, { 0, rng.Signed() * 25, 0 }, 0.012f);
        b.M(MAT_PAINT_CREAM).BoxRot({ c.x, c.y + h + 0.001f, c.z }, { 0.06f, 0.002f, d + 0.004f }, { 0, rng.Signed() * 25, 0 }, 0);
        y = c.y + h;
    }
    b.Collider({ 0, y * 0.5f, 0 }, { 0.9f, y, 0.9f }, SURF_WOOD);
}

static void BuildStockCrate(PrefabBuild& b, const Entity& e) {
    // A single carryable stock box used by the restock task
    b.M(MAT_CARDBOARD).Box({ 0, 0.15f, 0 }, { 0.45f, 0.3f, 0.35f }, 0.01f);
    b.M(MAT_PAINT_CREAM).Box({ 0, 0.301f, 0 }, { 0.07f, 0.002f, 0.354f });
    b.Interact("Pick up stock box", { 0, 0.3f, 0 }, 1.8f, "stock_box");
    (void)e;
}

void RegisterStorePrefabs() {
    {
        PrefabInfo p; p.name = "store_building"; p.category = "store"; p.build = BuildStore; p.terrain = StoreTerrain;
        RegisterPrefab(p);
    }
    auto reg = [](const char* n, void (*fn)(PrefabBuild&, const Entity&), std::vector<std::string> keys = {}) {
        PrefabInfo p; p.name = n; p.category = "store"; p.build = fn; p.geometryKeys = keys;
        RegisterPrefab(p);
    };
    reg("store_shelf", BuildShelf, { "variant", "len" });
    reg("coolers", BuildCoolers, { "doors" });
    reg("store_counter", BuildCounter);
    reg("storage_shelf", BuildStorageShelf, { "variant" });
    reg("mop_bucket", BuildMopBucket);
    reg("breaker_box", BuildBreakerBox);
    reg("boxes", BuildBoxes, { "variant" });
    reg("stock_box", BuildStockCrate);
}
