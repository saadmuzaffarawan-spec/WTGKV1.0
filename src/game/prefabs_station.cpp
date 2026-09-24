// Route 9 gas station: canopy, islands, pumps, signage, hatch, service props.
#include "prefab_util.h"

static void BuildCanopy(PrefabBuild& b, const Entity& e) {
    const float W = 10.0f, L = 17.0f, H = 4.7f;
    // columns on the islands
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sz = -1; sz <= 1; sz += 2) {
            Vector3 c{ sx * 2.4f, H * 0.5f, sz * 5.6f };
            b.Solid(MAT_PAINT_WHITE, c, { 0.42f, H, 0.42f }, 0.03f, SURF_METAL);
            b.M(MAT_PAINT_RED).Box({ c.x, 1.0f, c.z }, { 0.44f, 0.12f, 0.44f }, 0.01f);
            b.M(MAT_CONCRETE).Box({ c.x, 0.35f, c.z }, { 0.6f, 0.5f, 0.6f }, 0.04f);
        }
    // deck + fascia
    b.M(MAT_PAINT_WHITE).Box({ 0, H + 0.12f, 0 }, { W, 0.24f, L }, 0.02f);
    b.M(MAT_PAINT_GREY).Box({ 0, H + 0.02f, 0 }, { W - 0.3f, 0.04f, L - 0.3f });
    const float fh = 0.95f;
    b.M(MAT_PAINT_WHITE).Box({ 0, H + 0.55f, L * 0.5f }, { W + 0.1f, fh, 0.12f }, 0.02f);
    b.M(MAT_PAINT_WHITE).Box({ 0, H + 0.55f, -L * 0.5f }, { W + 0.1f, fh, 0.12f }, 0.02f);
    b.M(MAT_PAINT_WHITE).Box({ W * 0.5f, H + 0.55f, 0 }, { 0.12f, fh, L }, 0.02f);
    b.M(MAT_PAINT_WHITE).Box({ -W * 0.5f, H + 0.55f, 0 }, { 0.12f, fh, L }, 0.02f);
    // red band + lettering on the road-facing sides (some letters dead)
    int sign = SignMaterial("canopy_fuel", "GRETHN R'S   FUEL", ui::F_SIGN, 70, Color{ 235, 60, 45, 255 }, Color{ 0, 0, 0, 255 }, 1024, 96, 0.3f, true, 4.0f);
    for (int s = -1; s <= 1; s += 2) {
        b.M(MAT_PAINT_RED).Box({ s * (W * 0.5f + 0.07f), H + 0.25f, 0 }, { 0.02f, 0.14f, L }, 0.0f);
        b.mb.Push(); b.mb.Translate({ s * (W * 0.5f + 0.075f), H + 0.62f, 0 }); b.mb.RotateY(s * 90.0f);
        b.M(sign).BoxUV({ 0, 0, 0 }, { 8.0f, 0.62f, 0.005f });
        b.mb.Pop();
    }
    // Recessed metal-halide fixtures; one is dying.
    int idx = 0;
    for (int sx = -1; sx <= 1; sx += 2)
        for (int k = -1; k <= 1; k++) {
            Vector3 p{ sx * 2.4f, H - 0.005f, k * 4.5f };
            b.M(MAT_STEEL).Box({ p.x, p.y + 0.02f, p.z }, { 0.9f, 0.06f, 0.9f }, 0.01f);
            b.M(MAT_BULB_COLD).Box({ p.x, p.y - 0.01f, p.z }, { 0.7f, 0.02f, 0.7f });
            LightDef& l = b.AddSpot({ p.x, p.y - 0.08f, p.z }, { 0, -1, 0 }, { 0.82f, 0.9f, 1.0f }, 26.0f, 13.0f, 55.0f, 85.0f, 0.18f);
            l.group = "canopy";
            if (idx == 4) l.flicker = 0.35f;
            idx++;
        }
    (void)e;
}

static void BuildPumpIsland(PrefabBuild& b, const Entity&) {
    b.Solid(MAT_CONCRETE_CURB, { 0, 0.09f, 0 }, { 1.3f, 0.18f, 6.2f }, 0.04f, SURF_CONCRETE);
    b.M(MAT_PAINT_YELLOW).Box({ 0, 0.185f, 0 }, { 1.32f, 0.01f, 6.22f });
    for (int s = -1; s <= 1; s += 2) {
        for (int k = -1; k <= 1; k += 2) {
            Vector3 p{ k * 0.4f, 0.18f, s * 3.25f };
            b.M(MAT_PAINT_YELLOW).Cylinder(p, { p.x, 1.15f, p.z }, 0.08f, 0.08f, 12, true);
            b.ColliderCyl(p, 0.09f, 1.0f, SURF_METAL);
        }
    }
    // squeegee bucket + trash bin + towel dispenser
    b.M(MAT_GREY_PLASTIC).Cylinder({ 0.35f, 0.18f, 1.6f }, { 0.35f, 0.55f, 1.6f }, 0.14f, 0.17f, 14, false);
    b.M(MAT_WOOD).Box({ 0.35f, 0.7f, 1.6f }, { 0.03f, 0.5f, 0.03f });
    b.M(MAT_BLACK_PLASTIC).Box({ 0.35f, 0.95f, 1.6f }, { 0.03f, 0.05f, 0.25f });
    b.M(MAT_PAINT_GREEN).Cylinder({ -0.35f, 0.18f, -1.7f }, { -0.35f, 1.0f, -1.7f }, 0.25f, 0.25f, 16, true);
    b.M(MAT_BLACK_PLASTIC).Cylinder({ -0.35f, 1.0f, -1.7f }, { -0.35f, 1.05f, -1.7f }, 0.27f, 0.2f, 16, true);
    b.ColliderCyl({ -0.35f, 0.18f, -1.7f }, 0.26f, 0.9f, SURF_METAL);
}

static void BuildGasPump(PrefabBuild& b, const Entity& e) {
    // Late-80s dispenser: cabinet, head with twin displays, nozzles in boots on both faces.
    b.Solid(MAT_PAINT_WHITE, { 0, 0.62f, 0 }, { 0.55f, 1.25f, 1.0f }, 0.03f, SURF_METAL);
    b.M(MAT_PAINT_RED).Box({ 0, 0.18f, 0 }, { 0.57f, 0.12f, 1.02f }, 0.01f);
    b.M(MAT_PAINT_RED).Box({ 0, 1.55f, 0 }, { 0.5f, 0.62f, 1.02f }, 0.03f);
    b.M(MAT_PAINT_WHITE).Box({ 0, 1.94f, 0 }, { 0.46f, 0.16f, 0.98f }, 0.03f);
    for (int s = -1; s <= 1; s += 2) {
        float x = s * 0.255f;
        // display window: dark glass with a dim readout
        b.M(MAT_BLACK_PLASTIC).Box({ x, 1.6f, 0 }, { 0.02f, 0.36f, 0.72f }, 0.005f);
        b.M(MAT_SCREEN_GREEN).Box({ x + s * 0.012f, 1.66f, 0.05f }, { 0.003f, 0.07f, 0.44f });
        b.M(MAT_SCREEN_GREEN).Box({ x + s * 0.012f, 1.54f, 0.05f }, { 0.003f, 0.07f, 0.44f });
        // grade buttons
        for (int k = 0; k < 3; k++) b.M(k == 0 ? MAT_PAINT_YELLOW : (k == 1 ? MAT_PAINT_GREEN : MAT_PAINT_BLUE))
            .Box({ x + s * 0.02f, 1.05f, -0.3f + k * 0.3f }, { 0.03f, 0.1f, 0.16f }, 0.01f);
        // nozzle boot + nozzle
        b.M(MAT_STEEL).Box({ x + s * 0.06f, 0.95f, 0.38f }, { 0.1f, 0.25f, 0.12f }, 0.01f);
        b.mb.Push(); b.mb.Translate({ x + s * 0.1f, 0.98f, 0.38f }); b.mb.RotateZ(s * 25.0f);
        b.M(MAT_BLACK_PLASTIC).Box({ 0, 0.05f, 0 }, { 0.06f, 0.22f, 0.07f }, 0.01f);
        b.M(MAT_STEEL).Cylinder({ 0, -0.06f, 0 }, { s * 0.05f, -0.22f, 0 }, 0.012f, 0.01f, 8, true);
        b.mb.Pop();
        // hose: from the head down in a hanging loop to the nozzle
        auto pts = Catenary({ x * 0.8f, 1.85f, 0.45f }, { x + s * 0.12f, 1.1f, 0.38f }, 0.55f, 12);
        for (auto& p : pts) p.x += s * 0.05f;
        b.mb.Flex(0.05f);
        b.M(MAT_RUBBER).Tube(pts, std::vector<float>(pts.size(), 0.017f), 6, false);
        b.mb.Flex(0);
    }
    b.Interact("Use pump", { 0.4f, 1.2f, 0 }, 2.2f, "pump");
    (void)e;
}

static void BuildPriceSign(PrefabBuild& b, const Entity& e) {
    const float H = 6.2f;
    for (int s = -1; s <= 1; s += 2) {
        b.Solid(MAT_STEEL, { s * 1.05f, H * 0.5f, 0 }, { 0.22f, H, 0.22f }, 0.02f, SURF_METAL);
    }
    b.M(MAT_CONCRETE).Box({ 0, 0.3f, 0 }, { 2.9f, 0.6f, 0.7f }, 0.05f);
    int top = SignMaterial("price_top", "GAS", ui::F_SIGN, 130, Color{ 230, 40, 30, 255 }, Color{ 238, 232, 214, 255 }, 512, 200, 0.4f, true, 2.2f);
    int prices = SignMaterial("price_list", "REGULAR  3.1 9\nPLUS     3.39\nDIESEL   3.  9", ui::F_MONO_BOLD, 58, Color{ 255, 170, 40, 255 }, Color{ 12, 10, 8, 255 }, 512, 220, 0.2f, true, 3.0f);
    b.M(MAT_PAINT_WHITE).Box({ 0, H + 0.05f, 0 }, { 2.9f, 1.3f, 0.5f }, 0.04f);
    b.M(MAT_PAINT_BLACK).Box({ 0, H - 1.35f, 0 }, { 2.9f, 1.3f, 0.5f }, 0.04f);
    for (int s = -1; s <= 1; s += 2) {
        b.mb.Push(); b.mb.RotateY(s > 0 ? 0 : 180);
        b.M(top).BoxUV({ 0, H + 0.05f, 0.253f }, { 2.7f, 1.1f, 0.004f });
        b.M(prices).BoxUV({ 0, H - 1.35f, 0.253f }, { 2.7f, 1.15f, 0.004f });
        b.mb.Pop();
    }
    b.AddLight({ 0, H - 0.6f, 1.2f }, { 1.0f, 0.6f, 0.3f }, 4.0f, 7.0f, 0.2f).group = "station";
    (void)e;
}

static void BuildTankHatch(PrefabBuild& b, const Entity& e) {
    b.Solid(MAT_CONCRETE_DARK, { 0, 0.04f, 0 }, { 3.4f, 0.08f, 2.4f }, 0.03f, SURF_CONCRETE);
    Color rims[3] = { { 200, 40, 30, 255 }, { 230, 230, 220, 255 }, { 40, 80, 170, 255 } };
    for (int i = 0; i < 3; i++) {
        Vector3 c{ -1.1f + i * 0.9f, 0.08f, 0.6f };
        b.mb.SetColor(rims[i]);
        b.M(MAT_PAINT_WHITE).Cylinder(c, { c.x, c.y + 0.025f, c.z }, 0.22f, 0.22f, 20, true);
        b.mb.SetColor(WHITE);
        b.M(MAT_STEEL).Cylinder({ c.x, c.y + 0.02f, c.z }, { c.x, c.y + 0.04f, c.z }, 0.16f, 0.16f, 20, true);
    }
    // The large access hatch the drag marks lead to
    b.M(MAT_RUST).Cylinder({ 0.4f, 0.08f, -0.45f }, { 0.4f, 0.11f, -0.45f }, 0.62f, 0.62f, 28, true);
    for (int k = -2; k <= 2; k++) b.M(MAT_RUST).Box({ 0.4f + k * 0.2f, 0.115f, -0.45f }, { 0.04f, 0.01f, 0.9f - fabsf((float)k) * 0.2f });
    b.M(MAT_STEEL).Box({ 0.4f, 0.12f, -0.95f }, { 0.25f, 0.03f, 0.05f }, 0.01f);
    b.Interact("Inspect hatch", { 0.4f, 0.2f, -0.45f }, 2.0f, "hatch");
    (void)e;
}

static void BuildPayphone(PrefabBuild& b, const Entity& e) {
    b.Solid(MAT_STEEL, { 0, 0.8f, 0 }, { 0.12f, 1.6f, 0.12f }, 0.01f, SURF_METAL);
    b.M(MAT_STEEL).Box({ 0, 1.5f, 0.12f }, { 0.36f, 0.62f, 0.18f }, 0.03f);
    b.M(MAT_BLACK_PLASTIC).Box({ -0.08f, 1.5f, 0.22f }, { 0.07f, 0.34f, 0.06f }, 0.02f);
    for (int r = 0; r < 4; r++) for (int c = 0; c < 3; c++)
        b.M(MAT_CHROME).Box({ 0.06f + c * 0.045f, 1.42f + r * 0.045f, 0.215f }, { 0.03f, 0.03f, 0.01f }, 0.004f);
    auto cord = Catenary({ -0.08f, 1.34f, 0.24f }, { 0.02f, 1.3f, 0.22f }, 0.25f, 10);
    b.M(MAT_STEEL).Tube(cord, std::vector<float>(cord.size(), 0.008f), 5, false);
    b.M(MAT_PAINT_BLUE).Box({ 0, 1.95f, 0.1f }, { 0.4f, 0.18f, 0.22f }, 0.02f);
    int sign = SignMaterial("phone_sign", "PHONE", ui::F_SIGN, 60, Color{ 240, 240, 230, 255 }, Color{ 30, 60, 130, 255 }, 256, 90, 0.4f, true, 1.2f);
    b.M(sign).BoxUV({ 0, 1.95f, 0.212f }, { 0.36f, 0.13f, 0.004f });
    b.AddLight({ 0, 1.85f, 0.4f }, { 0.7f, 0.8f, 1.0f }, 1.2f, 3.0f, 0.1f).flicker = 0.2f;
    b.Interact("Use phone", { 0, 1.5f, 0.25f }, 1.8f, "payphone");
    (void)e;
}

static void BuildDumpster(PrefabBuild& b, const Entity& e) {
    const float W = 1.9f, D = 1.2f, H = 1.25f;
    b.Solid(MAT_PAINT_GREEN, { 0, H * 0.5f + 0.12f, 0 }, { W, H, D }, 0.03f, SURF_METAL);
    for (int s = -1; s <= 1; s += 2) {
        b.M(MAT_PAINT_GREEN).Box({ s * (W * 0.5f + 0.03f), H * 0.6f, 0 }, { 0.06f, 0.1f, D * 0.9f }, 0.01f);
        b.M(MAT_BLACK_PLASTIC).BoxRot({ s * W * 0.25f, H + 0.2f, -0.02f }, { W * 0.49f, 0.04f, D + 0.08f }, { 6, 0, 0 }, 0.01f);
        for (int k = -1; k <= 1; k += 2) b.M(MAT_RUBBER).Cylinder({ s * W * 0.4f, 0.06f, k * D * 0.35f - 0.03f }, { s * W * 0.4f, 0.06f, k * D * 0.35f + 0.03f }, 0.07f, 0.07f, 10, true);
    }
    b.M(MAT_RUST).Box({ 0, 0.5f, D * 0.5f + 0.005f }, { W * 0.8f, 0.5f, 0.01f });
    b.Interact("Dumpster", { 0, H + 0.2f, D * 0.5f }, 2.0f, "dumpster");
    (void)e;
}

static void BuildIceChest(PrefabBuild& b, const Entity& e) {
    b.Solid(MAT_PAINT_WHITE, { 0, 0.55f, 0 }, { 1.6f, 1.1f, 0.8f }, 0.04f, SURF_METAL);
    int ice = SignMaterial("ice", "ICE", ui::F_SIGN, 120, Color{ 30, 90, 180, 255 }, Color{ 225, 230, 232, 255 }, 400, 150, 0.6f);
    b.M(ice).BoxUV({ 0, 0.62f, 0.405f }, { 1.2f, 0.45f, 0.004f });
    b.M(MAT_STEEL).Box({ 0, 1.08f, 0.38f }, { 0.4f, 0.04f, 0.04f }, 0.01f);
    b.AddLight({ 0, 1.2f, 0.6f }, { 0.8f, 0.9f, 1.0f }, 0.8f, 2.5f).group = "store";
    (void)e;
}

static void BuildAirPump(PrefabBuild& b, const Entity& e) {
    b.Solid(MAT_PAINT_RED, { 0, 0.75f, 0 }, { 0.4f, 1.5f, 0.35f }, 0.03f, SURF_METAL);
    b.M(MAT_PAINT_WHITE).Box({ 0, 1.2f, 0.18f }, { 0.3f, 0.3f, 0.02f }, 0.01f);
    std::vector<Vector3> coil;
    for (int i = 0; i <= 40; i++) {
        float a = i * 0.45f;
        coil.push_back({ 0.22f + cosf(a) * 0.12f, 0.9f - i * 0.004f, sinf(a) * 0.12f });
    }
    b.M(MAT_RUBBER).Tube(coil, std::vector<float>(coil.size(), 0.012f), 5, false);
    (void)e;
}

static void BuildPropaneCage(PrefabBuild& b, const Entity& e) {
    const float W = 1.6f, H = 1.6f, D = 0.7f;
    for (int s = -1; s <= 1; s += 2) for (int k = -1; k <= 1; k += 2)
        b.M(MAT_PAINT_WHITE).Box({ s * W * 0.5f, H * 0.5f, k * D * 0.5f }, { 0.04f, H, 0.04f });
    for (float y = 0.05f; y < H; y += 0.8f) b.M(MAT_PAINT_WHITE).Box({ 0, y, 0 }, { W, 0.04f, D });
    for (float x = -W * 0.5f + 0.1f; x < W * 0.5f; x += 0.1f) b.M(MAT_PAINT_WHITE).Box({ x, H * 0.5f, D * 0.5f }, { 0.012f, H, 0.012f });
    for (int i = 0; i < 4; i++) {
        float x = -0.55f + i * 0.37f;
        for (int lvl = 0; lvl < 2; lvl++) {
            float y = 0.07f + lvl * 0.8f;
            b.M(MAT_PAINT_WHITE).Cylinder({ x, y, 0 }, { x, y + 0.45f, 0 }, 0.15f, 0.15f, 14, true);
            b.M(MAT_PAINT_WHITE).Ellipsoid({ x, y + 0.47f, 0 }, { 0.15f, 0.08f, 0.15f }, 5, 12);
            b.M(MAT_STEEL).Cylinder({ x, y + 0.53f, 0 }, { x, y + 0.62f, 0 }, 0.05f, 0.05f, 8, true);
        }
    }
    b.Collider({ 0, H * 0.5f, 0 }, { W, H, D }, SURF_METAL);
    (void)e;
}

static void BuildOilRack(PrefabBuild& b, const Entity& e) {
    b.M(MAT_PAINT_BLUE).Box({ 0, 0.6f, 0 }, { 0.9f, 1.2f, 0.35f }, 0.02f);
    Rng rng(5);
    for (int shelf = 0; shelf < 3; shelf++)
        for (int i = 0; i < 7; i++) {
            Color c = rng.Chance(0.5f) ? Color{ 230, 180, 40, 255 } : Color{ 30, 30, 30, 255 };
            b.mb.SetColor(c);
            b.M(MAT_GREY_PLASTIC).Box({ -0.36f + i * 0.12f, 0.35f + shelf * 0.35f, 0.08f }, { 0.09f, 0.22f, 0.07f }, 0.015f);
        }
    b.mb.SetColor(WHITE);
    b.Collider({ 0, 0.6f, 0 }, { 0.9f, 1.2f, 0.4f }, SURF_METAL);
    (void)e;
}

static void BuildWallLight(PrefabBuild& b, const Entity& e) {
    b.M(MAT_PAINT_GREY).Box({ 0, 0, 0.1f }, { 0.35f, 0.3f, 0.2f }, 0.03f);
    b.M(MAT_BULB_SODIUM).Box({ 0, -0.08f, 0.2f }, { 0.25f, 0.12f, 0.02f });
    LightDef& l = b.AddSpot({ 0, -0.1f, 0.3f }, { 0, -0.8f, 1.0f }, { 1.0f, 0.55f, 0.22f }, 22.0f, 14.0f, 45.0f, 80.0f, 0.3f);
    l.group = e.Str("group", "station");
    l.flicker = e.Num("flicker", 0.0f);
}

static void BuildParkingLines(PrefabBuild& b, const Entity& e) {
    int n = (int)e.Num("count", 4);
    for (int i = 0; i <= n; i++) {
        float x = -n * 1.35f + i * 2.7f;
        b.M(MAT_PAINT_WHITE).Box({ x, 0.012f, 0 }, { 0.1f, 0.004f, 5.0f });
    }
    b.castShadow = false;
}

static void BuildAsphaltPad(PrefabBuild& b, const Entity& e) {
    float w = e.Num("w", 20), d = e.Num("d", 20);
    int mat = e.Str("mat") == "concrete" ? MAT_CONCRETE : MAT_ASPHALT;
    b.M(mat).Box({ 0, -0.05f, 0 }, { w, 0.16f, d }, 0.05f);
    b.Collider({ 0, -0.05f, 0 }, { w, 0.16f, d }, mat == MAT_CONCRETE ? SURF_CONCRETE : SURF_ASPHALT);
    b.castShadow = false;
}

static void PadTerrain(const Entity& e, std::vector<TerrainPad>& pads, std::vector<Rectangle>&) {
    TerrainPad p;
    p.c = { e.pos.x, e.pos.z };
    p.half = { e.Num("w", 20) * 0.5f, e.Num("d", 20) * 0.5f };
    p.yaw = e.rot.x * DEG2RAD;
    p.margin = 3.0f;
    p.splat = 1;
    pads.push_back(p);
}

void RegisterStationPrefabs() {
    auto reg = [](const char* n, void (*fn)(PrefabBuild&, const Entity&), std::vector<std::string> keys = {}) {
        PrefabInfo p; p.name = n; p.category = "station"; p.build = fn; p.geometryKeys = keys;
        RegisterPrefab(p);
        return p;
    };
    reg("gas_canopy", BuildCanopy);
    reg("pump_island", BuildPumpIsland);
    reg("gas_pump", BuildGasPump);
    reg("price_sign", BuildPriceSign);
    reg("tank_hatch", BuildTankHatch);
    reg("payphone", BuildPayphone);
    reg("dumpster", BuildDumpster);
    reg("ice_chest", BuildIceChest);
    reg("air_pump", BuildAirPump);
    reg("propane_cage", BuildPropaneCage);
    reg("oil_rack", BuildOilRack);
    reg("wall_light", BuildWallLight, { "group", "flicker" });
    reg("parking_lines", BuildParkingLines, { "count" });
    {
        PrefabInfo p; p.name = "paved_area"; p.category = "station"; p.build = BuildAsphaltPad;
        p.geometryKeys = { "w", "d", "mat" }; p.terrain = PadTerrain;
        RegisterPrefab(p);
    }
}
