// Nature and roadside infrastructure prefabs.
#include "prefab_util.h"
#include <algorithm>

static uint32_t VariantSeed(const Entity& e, uint32_t salt) {
    return HashU32((uint32_t)e.Num("variant", 0) * 7919u + salt);
}

// ---------------------------------------------------------------------------
// Trees
// ---------------------------------------------------------------------------
static void BuildDeadTree(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 11) | 1);
    float height = rng.Range(7.0f, 12.0f);
    float rad = rng.Range(0.18f, 0.34f);
    Vector3 lean{ rng.Signed() * 0.12f, 1.0f, rng.Signed() * 0.12f };
    // root flare
    b.mb.Flex(0);
    for (int i = 0; i < 5; i++) {
        float a = i * 1.2566f + rng.Range(0, 0.6f);
        Vector3 d{ cosf(a), -0.35f, sinf(a) };
        std::vector<Vector3> pts{ { 0, 0.35f, 0 }, { d.x * 0.5f, 0.1f, d.z * 0.5f }, { d.x * 1.1f, -0.15f, d.z * 1.1f } };
        b.M(MAT_BARK).Tube(pts, { rad * 0.7f, rad * 0.45f, rad * 0.15f }, 6, true);
    }
    GrowBranch(b.mb, rng, { 0, -0.2f, 0 }, lean, height, rad, 0, rng.Chance(0.5f) ? 3 : 2, MAT_BARK);
    b.ColliderCyl({ 0, 0, 0 }, rad + 0.08f, 4.0f, SURF_WOOD);
}

static void BuildPine(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 23) | 1);
    float height = rng.Range(11.0f, 17.0f);
    float rad = rng.Range(0.17f, 0.27f);
    float leanX = rng.Signed() * 0.25f, leanZ = rng.Signed() * 0.25f;
    b.mb.Flex(0);
    b.M(MAT_BARK_DARK).Tube({ { 0, -0.3f, 0 }, { leanX * 0.3f, height * 0.4f, leanZ * 0.3f }, { leanX, height, leanZ } },
                            { rad * 1.15f, rad * 0.7f, 0.02f }, 9, true);
    // Dead bare twigs on the lower trunk
    for (int i = 0; i < 9; i++) {
        float y = rng.Range(1.8f, height * 0.35f);
        float a = rng.Range(0, 6.28f);
        Vector3 base{ leanX * y / height, y, leanZ * y / height };
        Vector3 tip = Vector3Add(base, { cosf(a) * rng.Range(0.6f, 1.4f), rng.Range(-0.3f, 0.1f), sinf(a) * rng.Range(0.6f, 1.4f) });
        b.mb.Flex(0.3f);
        b.M(MAT_BARK_DARK).Tube({ base, tip }, { 0.025f, 0.004f }, 4, false);
    }
    // Needle skirts: drooping, ragged layers that thin out near the top
    int layers = rng.RangeI(9, 13);
    float start = height * rng.Range(0.3f, 0.42f);
    for (int L = 0; L < layers; L++) {
        float t = (float)L / (layers - 1);
        float y = Lerp(start, height - 0.4f, t);
        float R = Lerp(2.6f, 0.35f, powf(t, 0.85f)) * rng.Range(0.85f, 1.15f);
        Vector3 c{ leanX * y / height, y, leanZ * y / height };
        int spokes = 13;
        std::vector<Vector3> mid(spokes), tip(spokes);
        float a0 = rng.Range(0, 6.28f);
        for (int i = 0; i < spokes; i++) {
            float a = a0 + i * 6.2831f / spokes + rng.Signed() * 0.18f;
            float r = R * rng.Range(0.7f, 1.12f);
            if (rng.Chance(0.08f)) r *= 0.4f;   // gaps
            tip[i] = { c.x + cosf(a) * r, y - R * rng.Range(0.35f, 0.6f), c.z + sinf(a) * r };
            mid[i] = { c.x + cosf(a) * r * 0.5f, y - R * 0.1f + rng.Signed() * 0.1f, c.z + sinf(a) * r * 0.5f };
        }
        Vector3 top{ c.x, y + R * 0.12f, c.z };
        for (int i = 0; i < spokes; i++) {
            int j = (i + 1) % spokes;
            auto nrm = [&](Vector3 p) { Vector3 o = Vector3Subtract(p, c); o.y = 0; return Vector3Normalize(Vector3Add(Vector3Normalize(o), { 0, 1.2f, 0 })); };
            b.mb.Flex(0.35f + t * 0.3f);
            MeshBuilder& m = b.M(MAT_PINE);
            m.TriN(top, mid[j], mid[i], { 0, 1, 0 }, nrm(mid[j]), nrm(mid[i]));
            b.mb.Flex(0.7f + t * 0.3f);
            MeshBuilder& m2 = b.M(MAT_PINE);
            m2.TriN(mid[i], mid[j], tip[j], nrm(mid[i]), nrm(mid[j]), nrm(tip[j]));
            m2.TriN(mid[i], tip[j], tip[i], nrm(mid[i]), nrm(tip[j]), nrm(tip[i]));
        }
    }
    b.ColliderCyl({ 0, 0, 0 }, rad + 0.1f, 4.0f, SURF_WOOD);
}

static void BuildBush(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 31) | 1);
    int stems = rng.RangeI(6, 10);
    for (int i = 0; i < stems; i++) {
        float a = rng.Range(0, 6.28f);
        Vector3 d{ cosf(a) * 0.5f, 1.0f, sinf(a) * 0.5f };
        GrowBranch(b.mb, rng, { rng.Signed() * 0.2f, 0, rng.Signed() * 0.2f }, d, rng.Range(0.7f, 1.5f), 0.02f, 2, 3, MAT_BARK_DARK);
    }
    b.castShadow = true;
}

static void BuildRock(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 41) | 1);
    Vector3 radii{ rng.Range(0.6f, 1.4f), rng.Range(0.4f, 0.9f), rng.Range(0.6f, 1.3f) };
    const int rings = 12, segs = 18;
    auto P = [&](int ri, int si) {
        float th = (float)ri / rings * PI, ph = (float)si / segs * 2 * PI;
        Vector3 d{ sinf(th) * cosf(ph), cosf(th), sinf(th) * sinf(ph) };
        float n = Fbm2(d.x * 2.1f + d.y * 1.3f + 5.0f, d.z * 2.1f - d.y * 0.7f, 4, 0, (uint32_t)rng.Next()) * 0.35f;
        float flat = d.y < -0.2f ? 0.6f : 1.0f;   // flatter bottom sits in the ground
        return Vector3{ d.x * radii.x * (1 + n), d.y * radii.y * (1 + n) * flat, d.z * radii.z * (1 + n) };
    };
    std::vector<Vector3> grid((rings + 1) * (segs + 1));
    for (int r = 0; r <= rings; r++) for (int s = 0; s <= segs; s++) grid[r * (segs + 1) + s] = P(r, s % segs);
    MeshBuilder& m = b.M(MAT_ROCK);
    for (int r = 0; r < rings; r++)
        for (int s = 0; s < segs; s++) {
            Vector3 a = grid[r * (segs + 1) + s], bb = grid[r * (segs + 1) + s + 1];
            Vector3 c = grid[(r + 1) * (segs + 1) + s], d = grid[(r + 1) * (segs + 1) + s + 1];
            m.Tri(a, bb, d); m.Tri(a, d, c);
        }
    b.Collider({ 0, radii.y * 0.35f, 0 }, { radii.x * 1.6f, radii.y * 1.3f, radii.z * 1.6f }, SURF_GRAVEL);
}

static void BuildStump(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 51) | 1);
    float r = rng.Range(0.25f, 0.45f), h = rng.Range(0.3f, 0.8f);
    b.M(MAT_BARK).Cylinder({ 0, -0.1f, 0 }, { 0, h, 0 }, r * 1.15f, r, 14, false);
    b.M(MAT_WOOD_DARK).Cylinder({ 0, h - 0.01f, 0 }, { 0, h, 0 }, r * 0.98f, r * 0.98f, 14, true);
    b.ColliderCyl({ 0, 0, 0 }, r, h, SURF_WOOD);
}

static void BuildLog(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 61) | 1);
    float len = e.Num("len", rng.Range(3.0f, 6.0f)), r = rng.Range(0.18f, 0.32f);
    b.M(MAT_BARK).Tube({ { -len * 0.5f, r * 0.8f, 0 }, { 0, r * 0.85f, rng.Signed() * 0.1f }, { len * 0.5f, r * 0.8f, 0 } }, { r, r * 0.95f, r * 0.8f }, 12, true);
    b.Collider({ 0, r * 0.8f, 0 }, { len, r * 1.6f, r * 1.8f }, SURF_WOOD);
}

// ---------------------------------------------------------------------------
// Roadside infrastructure
// ---------------------------------------------------------------------------
static void BuildPowerPole(PrefabBuild& b, const Entity& e) {
    float h = 9.5f;
    b.M(MAT_WOOD_DARK).Cylinder({ 0, -0.5f, 0 }, { 0, h, 0 }, 0.15f, 0.11f, 12, true);
    b.M(MAT_WOOD_DARK).Box({ 0, h - 0.6f, 0 }, { 2.4f, 0.1f, 0.1f }, 0.01f);
    b.M(MAT_WOOD_DARK).Box({ 0.55f, h - 1.0f, 0 }, { 0.05f, 0.8f, 0.05f });
    b.mb.Push(); b.mb.Translate({ 0.35f, h - 0.95f, 0 }); b.mb.RotateZ(-45);
    b.M(MAT_STEEL).Box({ 0, 0, 0 }, { 0.04f, 0.9f, 0.04f });
    b.mb.Pop();
    for (int i = -1; i <= 1; i++) {
        float x = i * 1.0f;
        b.M(MAT_GREY_PLASTIC).Cylinder({ x, h - 0.55f, 0 }, { x, h - 0.35f, 0 }, 0.04f, 0.035f, 8, true);
        for (int k = 0; k < 3; k++) b.M(MAT_GREY_PLASTIC).Cylinder({ x, h - 0.5f + k * 0.05f, 0 }, { x, h - 0.48f + k * 0.05f, 0 }, 0.07f, 0.07f, 10, true);
    }
    if (e.Num("transformer", 0) > 0) {
        b.M(MAT_PAINT_GREY).Cylinder({ 0.35f, h - 3.2f, 0.3f }, { 0.35f, h - 2.1f, 0.3f }, 0.3f, 0.3f, 16, true);
        b.M(MAT_STEEL).Box({ 0.1f, h - 2.7f, 0.15f }, { 0.2f, 0.08f, 0.2f });
    }
    // climbing staples & a faded tag
    for (float y = 2.2f; y < h - 1.5f; y += 0.45f) b.M(MAT_RUST).Box({ 0.13f, y, (int)(y * 10) % 2 ? 0.05f : -0.05f }, { 0.05f, 0.02f, 0.02f });
    b.ColliderCyl({ 0, 0, 0 }, 0.16f, 9.0f, SURF_WOOD);
}

static void BuildStreetLamp(PrefabBuild& b, const Entity& e) {
    float h = 8.0f;
    b.M(MAT_STEEL).Cylinder({ 0, -0.3f, 0 }, { 0, h, 0 }, 0.11f, 0.075f, 12, true);
    b.M(MAT_CONCRETE).Cylinder({ 0, -0.3f, 0 }, { 0, 0.25f, 0 }, 0.28f, 0.25f, 14, true);
    std::vector<Vector3> arm{ { 0, h - 0.4f, 0 }, { 0, h + 0.1f, 0.4f }, { 0, h + 0.25f, 1.2f }, { 0, h + 0.25f, 2.1f } };
    b.M(MAT_STEEL).Tube(arm, { 0.06f, 0.055f, 0.05f, 0.05f }, 8, true);
    b.M(MAT_PAINT_GREY).Ellipsoid({ 0, h + 0.22f, 2.45f }, { 0.28f, 0.12f, 0.5f }, 8, 14);
    b.M(MAT_BULB_SODIUM).Ellipsoid({ 0, h + 0.14f, 2.45f }, { 0.2f, 0.05f, 0.36f }, 6, 12);
    LightDef& l = b.AddSpot({ 0, h + 0.05f, 2.45f }, { 0, -1, -0.08f }, { 1.0f, 0.52f, 0.2f }, 70.0f, 24.0f, 38.0f, 72.0f, 0.55f);
    l.flicker = e.Num("flicker", 0.0f);
    l.group = e.Str("group", "street");
    b.ColliderCyl({ 0, 0, 0 }, 0.2f, 8.0f, SURF_METAL);
}

static Color SignBg(const std::string& style) {
    if (style == "white") return Color{ 225, 225, 218, 255 };
    if (style == "yellow") return Color{ 225, 175, 35, 255 };
    if (style == "brown") return Color{ 88, 55, 35, 255 };
    if (style == "red") return Color{ 170, 28, 26, 255 };
    return Color{ 20, 92, 58, 255 };
}
static Color SignFg(const std::string& style) {
    if (style == "white" || style == "yellow") return Color{ 20, 20, 20, 255 };
    return Color{ 235, 235, 230, 255 };
}

static void BuildRoadSign(PrefabBuild& b, const Entity& e) {
    std::string text = e.Str("text", "ROUTE 9");
    for (auto& ch : text) if (ch == '|') ch = '\n';
    std::string style = e.Str("style", "green");
    float w = e.Num("w", 1.8f), h = e.Num("h", 0.9f);
    float post = e.Num("post", 2.2f);
    int tw = 512, th = (int)(512 * h / w);
    int lines = 1; for (char c : text) if (c == '\n') lines++;
    float fs = fminf(th / (lines * 1.35f), tw / (text.size() / (float)lines * 0.62f + 1));
    int mat = SignMaterial("sign:" + text + style, text.c_str(), ui::F_SIGN, fs, SignFg(style), SignBg(style), tw, th, e.Num("weather", 0.5f));
    for (int side = -1; side <= 1; side += 2) {
        if (w < 1.2f && side == 1) break;
        float x = w < 1.2f ? 0 : side * w * 0.3f;
        b.M(MAT_STEEL).Box({ x, post * 0.5f - 0.3f, -0.03f }, { 0.07f, post + h * 0.5f + 0.6f, 0.04f }, 0.005f);
        b.Collider({ x, post * 0.5f, -0.03f }, { 0.1f, post + 0.6f, 0.1f }, SURF_METAL);
    }
    b.M(MAT_STEEL).Box({ 0, post + h * 0.5f, 0.0f }, { w + 0.02f, h + 0.02f, 0.02f }, 0.004f);
    b.M(mat).BoxUV({ 0, post + h * 0.5f, 0.012f }, { w, h, 0.004f });
}

static void BuildGuardrail(PrefabBuild& b, const Entity& e) {
    float len = e.Num("len", 8.0f);
    int posts = (int)(len / 1.9f) + 1;
    for (int i = 0; i < posts; i++) {
        float x = -len * 0.5f + i * (len / (posts - 1));
        b.M(MAT_RUST).Box({ x, 0.35f, 0.12f }, { 0.12f, 1.0f, 0.1f }, 0.005f);
    }
    // W-beam: two stacked bevelled strips
    b.M(MAT_STEEL).Box({ 0, 0.62f, 0 }, { len, 0.14f, 0.05f }, 0.02f);
    b.M(MAT_STEEL).Box({ 0, 0.48f, 0 }, { len, 0.14f, 0.05f }, 0.02f);
    b.M(MAT_RUST).Box({ 0, 0.55f, 0.02f }, { len, 0.04f, 0.04f }, 0.01f);
    b.Collider({ 0, 0.4f, 0.05f }, { len, 0.8f, 0.25f }, SURF_METAL);
}

static void BuildFenceWood(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 71) | 1);
    float len = e.Num("len", 8.0f);
    int posts = (int)(len / 2.4f) + 1;
    for (int i = 0; i < posts; i++) {
        float x = -len * 0.5f + i * (len / (posts - 1));
        b.M(MAT_WOOD_DARK).BoxRot({ x, 0.55f, 0 }, { 0.11f, 1.35f, 0.11f }, { rng.Signed() * 4, 0, rng.Signed() * 4 }, 0.01f);
    }
    for (int r = 0; r < 3; r++) {
        float y = 0.35f + r * 0.38f;
        for (int i = 0; i + 1 < posts; i++) {
            if (rng.Chance(0.12f)) continue;   // missing rail
            float x0 = -len * 0.5f + i * (len / (posts - 1)), x1 = x0 + len / (posts - 1);
            float sag = rng.Chance(0.15f) ? rng.Range(-0.25f, -0.1f) : 0.0f;
            b.M(MAT_WOOD).BoxRot({ (x0 + x1) * 0.5f, y + sag * 0.5f, 0.07f }, { x1 - x0 + 0.1f, 0.13f, 0.025f },
                                 { 0, 0, sag * 20.0f }, 0.004f);
        }
    }
    b.Collider({ 0, 0.6f, 0 }, { len, 1.2f, 0.2f }, SURF_WOOD);
}

static void BuildFenceWire(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 81) | 1);
    float len = e.Num("len", 12.0f);
    int posts = (int)(len / 3.0f) + 1;
    std::vector<float> xs;
    for (int i = 0; i < posts; i++) {
        float x = -len * 0.5f + i * (len / (posts - 1));
        xs.push_back(x);
        float tilt = rng.Signed() * 6.0f;
        b.mb.Push(); b.mb.Translate({ x, 0, 0 }); b.mb.RotateZ(tilt);
        b.M(MAT_WOOD_DARK).Cylinder({ 0, -0.3f, 0 }, { 0, 1.3f, 0 }, 0.06f, 0.05f, 8, true);
        b.mb.Pop();
    }
    for (int w = 0; w < 3; w++) {
        float y = 0.45f + w * 0.35f;
        for (size_t i = 0; i + 1 < xs.size(); i++) {
            if (rng.Chance(0.06f)) continue;   // cut strand
            auto pts = Catenary({ xs[i], y, 0.06f }, { xs[i + 1], y, 0.06f }, 0.05f + rng.Range(0, 0.06f), 6);
            b.mb.Flex(0.05f);
            b.M(MAT_RUST).Tube(pts, std::vector<float>(pts.size(), 0.006f), 4, false);
            b.mb.Flex(0);
        }
    }
    b.Collider({ 0, 0.6f, 0 }, { len, 1.2f, 0.15f }, SURF_WOOD);
}

static void BuildBillboard(PrefabBuild& b, const Entity& e) {
    std::string text = e.Str("text", "GRETHNAR'S|FUEL . FOOD . BAIT|OPEN ALL NIGHT - 2 MILES");
    for (auto& ch : text) if (ch == '|') ch = '\n';
    float w = 9.0f, h = 3.6f, lift = 4.2f;
    for (int i = 0; i < 3; i++) {
        float x = -w * 0.35f + i * w * 0.35f;
        b.M(MAT_WOOD_DARK).Box({ x, lift * 0.5f + 0.5f, 0.3f }, { 0.25f, lift + h, 0.25f }, 0.02f);
        b.M(MAT_WOOD_DARK).BoxRot({ x, lift * 0.45f, 1.0f }, { 0.12f, lift * 1.1f, 0.12f }, { -22, 0, 0 }, 0.01f);
        b.Collider({ x, lift * 0.5f, 0.3f }, { 0.3f, lift, 0.3f }, SURF_WOOD);
    }
    b.M(MAT_WOOD).Box({ 0, lift + h * 0.5f, 0.15f }, { w + 0.3f, h + 0.3f, 0.12f }, 0.02f);
    int mat = SignMaterial("bb:" + text, text.c_str(), ui::F_SIGN, 96, Color{ 190, 40, 30, 255 }, Color{ 225, 212, 180, 255 }, 1024, 410, 0.95f);
    b.M(mat).BoxUV({ 0, lift + h * 0.5f, 0.225f }, { w, h, 0.01f });
    // catwalk + lamps (dead)
    b.M(MAT_RUST).Box({ 0, lift - 0.15f, -0.4f }, { w, 0.06f, 0.8f });
    for (int i = -1; i <= 1; i += 2) b.M(MAT_PAINT_GREY).Cylinder({ i * w * 0.3f, lift - 0.1f, -0.7f }, { i * w * 0.3f, lift + 0.2f, -0.9f }, 0.08f, 0.12f, 10, true);
}

static void BuildBarrel(PrefabBuild& b, const Entity& e) {
    int mat = e.Str("color") == "blue" ? MAT_PAINT_BLUE : (e.Str("color") == "red" ? MAT_PAINT_RED : MAT_RUST);
    bool tipped = e.Num("tipped", 0) > 0;
    b.mb.Push();
    if (tipped) { b.mb.Translate({ 0, 0.29f, 0 }); b.mb.RotateZ(90); b.mb.Translate({ 0, -0.44f, 0 }); }
    b.M(mat).Cylinder({ 0, 0, 0 }, { 0, 0.88f, 0 }, 0.29f, 0.29f, 20, true);
    for (float y : { 0.02f, 0.3f, 0.58f, 0.86f }) b.M(mat).Cylinder({ 0, y - 0.015f, 0 }, { 0, y + 0.015f, 0 }, 0.3f, 0.3f, 20, false);
    b.M(MAT_RUST).Cylinder({ 0.12f, 0.88f, 0 }, { 0.12f, 0.9f, 0 }, 0.03f, 0.03f, 8, true);
    b.mb.Pop();
    if (tipped) b.Collider({ 0, 0.29f, 0 }, { 0.9f, 0.58f, 0.58f }, SURF_METAL);
    else b.ColliderCyl({ 0, 0, 0 }, 0.3f, 0.9f, SURF_METAL);
}

static void BuildCrate(PrefabBuild& b, const Entity& e) {
    float s = 0.8f;
    b.Solid(MAT_WOOD, { 0, s * 0.5f, 0 }, { s, s, s }, 0.02f, SURF_WOOD);
    for (int i = -1; i <= 1; i += 2) {
        b.M(MAT_WOOD_DARK).Box({ i * s * 0.51f, s * 0.5f, 0 }, { 0.02f, s * 0.95f, 0.1f });
        b.M(MAT_WOOD_DARK).Box({ 0, s * 0.5f, i * s * 0.51f }, { 0.1f, s * 0.95f, 0.02f });
    }
    (void)e;
}

static void BuildPallet(PrefabBuild& b, const Entity&) {
    for (int i = 0; i < 7; i++) b.M(MAT_WOOD).Box({ -0.5f + i * 0.167f, 0.13f, 0 }, { 0.12f, 0.02f, 1.2f }, 0.003f);
    for (int i = -1; i <= 1; i++) b.M(MAT_WOOD_DARK).Box({ 0, 0.06f, i * 0.52f }, { 1.1f, 0.1f, 0.1f }, 0.005f);
    b.Collider({ 0, 0.07f, 0 }, { 1.1f, 0.14f, 1.2f }, SURF_WOOD);
}

static void BuildTires(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 91) | 1);
    int n = rng.RangeI(2, 5);
    for (int i = 0; i < n; i++) {
        float y = 0.11f + i * 0.21f;
        Vector3 c{ rng.Signed() * 0.06f, y, rng.Signed() * 0.06f };
        // tyre as a lathe (torus-like profile)
        std::vector<Vector2> prof;
        for (int k = 0; k <= 10; k++) {
            float a = (float)k / 10 * 2 * PI;
            prof.push_back({ 0.29f + cosf(a) * 0.1f, sinf(a) * 0.1f });
        }
        b.M(MAT_RUBBER).Lathe(c, prof, 20, false, false);
    }
    b.ColliderCyl({ 0, 0, 0 }, 0.4f, 0.21f * n, SURF_BONE);
}

static void BuildCinder(PrefabBuild& b, const Entity& e) {
    Rng rng(VariantSeed(e, 99) | 1);
    int n = rng.RangeI(1, 4);
    for (int i = 0; i < n; i++)
        b.M(MAT_CONCRETE).BoxRot({ rng.Signed() * 0.1f, 0.1f + i * 0.2f, rng.Signed() * 0.1f }, { 0.4f, 0.2f, 0.2f }, { 0, rng.Signed() * 20, 0 }, 0.01f);
    b.Collider({ 0, 0.1f * n, 0 }, { 0.45f, 0.2f * n, 0.3f }, SURF_CONCRETE);
}

static void BuildCone(PrefabBuild& b, const Entity& e) {
    bool tipped = e.Num("tipped", 0) > 0;
    b.mb.Push();
    if (tipped) { b.mb.Translate({ 0, 0.16f, 0 }); b.mb.RotateZ(90); b.mb.Translate({ 0, -0.2f, 0 }); }
    b.M(MAT_BLACK_PLASTIC).Box({ 0, 0.015f, 0 }, { 0.38f, 0.03f, 0.38f }, 0.01f);
    b.M(MAT_PAINT_ORANGE).Cylinder({ 0, 0.03f, 0 }, { 0, 0.7f, 0 }, 0.16f, 0.03f, 16, true);
    b.M(MAT_PAINT_WHITE).Cylinder({ 0, 0.38f, 0 }, { 0, 0.5f, 0 }, 0.103f, 0.082f, 16, false);
    b.mb.Pop();
    b.ColliderCyl({ 0, 0, 0 }, 0.18f, 0.7f, SURF_BONE);
}

static void BuildBench(PrefabBuild& b, const Entity&) {
    for (int i = -1; i <= 1; i += 2) {
        b.M(MAT_PAINT_BLACK).Box({ i * 0.75f, 0.22f, 0 }, { 0.05f, 0.44f, 0.5f }, 0.01f);
        b.M(MAT_PAINT_BLACK).BoxRot({ i * 0.75f, 0.7f, 0.24f }, { 0.05f, 0.55f, 0.05f }, { -12, 0, 0 }, 0.01f);
    }
    for (int k = 0; k < 3; k++) b.M(MAT_WOOD).Box({ 0, 0.45f, -0.18f + k * 0.13f }, { 1.8f, 0.035f, 0.1f }, 0.006f);
    for (int k = 0; k < 2; k++) b.M(MAT_WOOD).BoxRot({ 0, 0.68f + k * 0.16f, 0.27f + k * 0.035f }, { 1.8f, 0.1f, 0.03f }, { -12, 0, 0 }, 0.006f);
    b.Collider({ 0, 0.3f, 0.05f }, { 1.8f, 0.6f, 0.6f }, SURF_WOOD);
}

static void BuildMailbox(PrefabBuild& b, const Entity&) {
    b.M(MAT_WOOD_DARK).Box({ 0, 0.55f, 0 }, { 0.1f, 1.1f, 0.1f }, 0.01f);
    b.M(MAT_PAINT_GREY).Box({ 0, 1.14f, 0 }, { 0.2f, 0.12f, 0.48f }, 0.01f);
    b.mb.Push(); b.mb.Translate({ 0, 1.2f, 0 }); b.mb.RotateX(90);
    b.M(MAT_PAINT_GREY).Cylinder({ 0, -0.24f, 0 }, { 0, 0.24f, 0 }, 0.1f, 0.1f, 14, true);
    b.mb.Pop();
    b.M(MAT_PAINT_RED).Box({ 0.11f, 1.3f, -0.05f }, { 0.01f, 0.2f, 0.04f });
    b.Collider({ 0, 0.6f, 0 }, { 0.25f, 1.3f, 0.5f }, SURF_METAL);
}

static void BuildEmpty(PrefabBuild&, const Entity&) {}

void RegisterEnvPrefabs() {
    auto reg = [](const char* n, const char* cat, void (*fn)(PrefabBuild&, const Entity&), std::vector<std::string> keys = { "variant" }) {
        PrefabInfo p; p.name = n; p.category = cat; p.build = fn; p.geometryKeys = keys;
        RegisterPrefab(p);
    };
    reg("dead_tree", "nature", BuildDeadTree);
    reg("pine_tree", "nature", BuildPine);
    reg("bush", "nature", BuildBush);
    reg("rock", "nature", BuildRock);
    reg("stump", "nature", BuildStump);
    reg("log", "nature", BuildLog, { "variant", "len" });
    reg("power_pole", "road", BuildPowerPole, { "transformer" });
    reg("street_lamp", "road", BuildStreetLamp, { "flicker", "group" });
    reg("road_sign", "road", BuildRoadSign, { "text", "style", "w", "h", "post", "weather" });
    reg("guardrail", "road", BuildGuardrail, { "len" });
    reg("fence_wood", "fence", BuildFenceWood, { "variant", "len" });
    reg("fence_wire", "fence", BuildFenceWire, { "variant", "len" });
    reg("billboard", "road", BuildBillboard, { "text" });
    reg("barrel", "props", BuildBarrel, { "color", "tipped" });
    reg("crate", "props", BuildCrate);
    reg("pallet", "props", BuildPallet);
    reg("tires", "props", BuildTires);
    reg("cinder_blocks", "props", BuildCinder);
    reg("traffic_cone", "props", BuildCone, { "tipped" });
    reg("bench", "props", BuildBench);
    reg("mailbox", "props", BuildMailbox);
    reg("runtime_model", "internal", BuildEmpty, {});
}

// Wires between consecutive power poles sharing a `line` property (ordered by `seq`).
void BuildPowerLines(Scene& scene) {
    std::map<std::string, std::vector<Entity*>> lines;
    for (auto& e : scene.ents) if (e->prefab == "power_pole" && e->Has("line")) lines[e->Str("line")].push_back(e.get());
    ModelBuilder mb;
    for (auto& kv : lines) {
        auto& v = kv.second;
        std::sort(v.begin(), v.end(), [](Entity* a, Entity* b) { return a->Num("seq") < b->Num("seq"); });
        for (size_t i = 0; i + 1 < v.size(); i++)
            for (int k = -1; k <= 1; k++) {
                Vector3 a = v[i]->LocalToWorld({ k * 1.0f, 9.5f - 0.3f, 0 });
                Vector3 b = v[i + 1]->LocalToWorld({ k * 1.0f, 9.5f - 0.3f, 0 });
                float span = Vector3Distance(a, b);
                auto pts = Catenary(a, b, span * 0.035f, 14);
                mb.Flex(0.08f);
                mb.M(MAT_BLACK_PLASTIC).Tube(pts, std::vector<float>(pts.size(), 0.012f), 4, false);
            }
    }
    Model3D* m = mb.Build(false);
    Entity* e = scene.Spawn("runtime_model", { 0, 0, 0 }, { 0, 0, 0 }, { { "tag", "runtime" } }, true);
    if (e) { e->model = m; e->tag = "runtime"; e->castShadow = false; }
}
