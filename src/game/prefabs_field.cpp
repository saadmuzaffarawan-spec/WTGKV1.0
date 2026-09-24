// The cow field south-east of the station: skeleton cows, the barn ruin, a windmill,
// a water trough and the shovel. The cow skeleton is built from parts so the story
// can reassemble one and make it walk.
#include "prefab_util.h"

// ---------------------------------------------------------------------------
// Cow skeleton parts. Standing frame: +z forward, y up, origin at the spine centre.
//   COW_BODY  spine, ribcage, pelvis, shoulder blades
//   COW_HEAD  neck vertebrae + skull (origin at the neck base)
//   COW_UPPER upper leg (origin at the joint, hangs down -y, 0.52 m)
//   COW_LOWER lower leg + hoof (origin at the knee, 0.56 m)
// ---------------------------------------------------------------------------
void CowPartGeometry(ModelBuilder& mb, int part, uint32_t seed) {
    Rng r(seed * 13u + (uint32_t)part);
    MeshBuilder& bone = mb.M(MAT_BONE);
    if (part == COW_BODY) {
        // spine: gentle arch, thicker at the hips, tail hanging
        std::vector<Vector3> sp; std::vector<float> sr;
        for (int i = 0; i <= 16; i++) {
            float u = i / 16.0f;
            float z = Lerp(-1.05f, 0.85f, u);
            sp.push_back({ 0, 0.06f * sinf(u * PI) + (u > 0.8f ? (u - 0.8f) * 0.3f : 0.0f), z });
            sr.push_back(0.045f);
        }
        bone.Tube(sp, sr, 7, true);
        for (int i = 0; i < 24; i++) {   // vertebra knuckles and spinous processes
            float u = i / 23.0f;
            float z = Lerp(-1.0f, 0.8f, u);
            float y = 0.06f * sinf(u * PI);
            bone.Ellipsoid({ 0, y, z }, { 0.06f, 0.05f, 0.035f }, 4, 6);
            float h = u > 0.75f ? 0.28f - (u - 0.75f) * 0.3f : 0.1f + u * 0.06f;
            bone.BoxRot({ 0, y + h * 0.5f, z - 0.02f }, { 0.012f, h, 0.03f }, { -12, 0, 0 });
        }
        std::vector<Vector3> tail; std::vector<float> tr;
        for (int i = 0; i <= 8; i++) { float u = i / 8.0f; tail.push_back({ 0, -u * 0.55f, -1.05f - sinf(u * 1.4f) * 0.12f }); tr.push_back(0.025f - u * 0.015f); }
        bone.Tube(tail, tr, 5, true);
        // ribs: 13 pairs, longest mid-chest, a few snapped
        for (int i = 0; i < 13; i++) {
            float z = Lerp(-0.25f, 0.68f, i / 12.0f);
            float depth = 0.55f + 0.22f * sinf((i / 12.0f) * PI * 0.8f + 0.3f);
            float width = 0.28f + 0.08f * sinf((i / 12.0f) * PI);
            for (int s = -1; s <= 1; s += 2) {
                bool snapped = r.Chance(0.12f);
                int segs = snapped ? 4 : 8;
                std::vector<Vector3> rp; std::vector<float> rr;
                for (int k = 0; k <= segs; k++) {
                    float a = k / 8.0f * PI * 0.92f;
                    rp.push_back({ s * (0.04f + sinf(a) * width), -(1.0f - cosf(a)) * 0.5f * depth, z - k * 0.012f });
                    rr.push_back(0.016f - k * 0.0008f);
                }
                bone.Tube(rp, rr, 5, true);
            }
        }
        // sternum
        bone.Tube({ { 0, -0.62f, -0.1f }, { 0, -0.7f, 0.25f }, { 0, -0.62f, 0.62f } }, { 0.025f, 0.03f, 0.025f }, 6, true);
        // pelvis: two wings and the socket ring
        for (int s = -1; s <= 1; s += 2) {
            bone.BoxRot({ s * 0.16f, 0.02f, -0.78f }, { 0.08f, 0.26f, 0.42f }, { 20, s * 18.0f, s * 25.0f }, 0.03f);
            bone.Ellipsoid({ s * 0.2f, -0.14f, -0.88f }, { 0.07f, 0.07f, 0.07f }, 5, 8);
        }
        bone.BoxRot({ 0, -0.1f, -0.98f }, { 0.3f, 0.06f, 0.12f }, { 30, 0, 0 }, 0.02f);
        // shoulder blades
        for (int s = -1; s <= 1; s += 2)
            bone.BoxRot({ s * 0.27f, -0.12f, 0.6f }, { 0.03f, 0.36f, 0.2f }, { -18, 0, s * 12.0f }, 0.02f);
    } else if (part == COW_HEAD) {
        std::vector<Vector3> nk; std::vector<float> nr;
        for (int i = 0; i <= 8; i++) { float u = i / 8.0f; nk.push_back({ 0, u * 0.28f, u * 0.48f }); nr.push_back(0.05f); }
        bone.Tube(nk, nr, 7, true);
        for (int i = 0; i < 7; i++) { float u = i / 6.0f; bone.Ellipsoid({ 0, u * 0.28f, u * 0.48f }, { 0.07f, 0.06f, 0.04f }, 4, 6); }
        // skull: cranium, long face, nasal opening, eye sockets, horn cores, loose jaw
        Vector3 c{ 0, 0.3f, 0.56f };
        mb.Push(); mb.Translate(c); mb.RotateX(58);
        MeshBuilder& b2 = mb.M(MAT_BONE);
        b2.Ellipsoid({ 0, 0.02f, 0 }, { 0.12f, 0.1f, 0.12f }, 7, 10);
        b2.Cylinder({ 0, 0, 0.05f }, { 0, -0.03f, 0.46f }, 0.1f, 0.05f, 10, true);
        mb.M(MAT_PAINT_BLACK).Ellipsoid({ 0, -0.015f, 0.46f }, { 0.035f, 0.02f, 0.03f }, 4, 6);
        for (int s = -1; s <= 1; s += 2) {
            mb.M(MAT_PAINT_BLACK).Ellipsoid({ s * 0.1f, 0.04f, 0.1f }, { 0.035f, 0.04f, 0.04f }, 5, 7);
            mb.M(MAT_BONE).Ellipsoid({ s * 0.1f, 0.055f, 0.1f }, { 0.045f, 0.018f, 0.05f }, 4, 6);   // brow ridge
            mb.M(MAT_BONE).Tube({ { s * 0.1f, 0.06f, -0.04f }, { s * 0.2f, 0.1f, -0.05f }, { s * 0.26f, 0.2f, 0.02f } }, { 0.035f, 0.025f, 0.01f }, 6, true);
        }
        mb.M(MAT_BONE).BoxRot({ 0, -0.1f, 0.24f }, { 0.12f, 0.03f, 0.42f }, { 8, 0, 0 }, 0.01f);   // jaw
        for (int i = 0; i < 6; i++) mb.M(MAT_TEETH).Box({ -0.03f + (i % 2) * 0.06f, -0.075f, 0.12f + (i / 2) * 0.04f }, { 0.02f, 0.02f, 0.03f });
        mb.Pop();
    } else if (part == COW_UPPER) {
        bone.Cylinder({ 0, 0, 0 }, { 0, -0.52f, 0 }, 0.04f, 0.032f, 8, true);
        bone.Ellipsoid({ 0, 0, 0 }, { 0.07f, 0.07f, 0.07f }, 5, 8);
        bone.Ellipsoid({ 0, -0.52f, 0 }, { 0.055f, 0.05f, 0.06f }, 5, 8);
    } else {
        bone.Cylinder({ 0, 0, 0 }, { 0, -0.5f, 0.02f }, 0.03f, 0.024f, 8, true);
        bone.Ellipsoid({ 0, -0.5f, 0.02f }, { 0.035f, 0.03f, 0.035f }, 4, 6);
        for (int s = -1; s <= 1; s += 2)
            mb.M(MAT_BLACK_PLASTIC).BoxRot({ s * 0.025f, -0.54f, 0.05f }, { 0.04f, 0.05f, 0.09f }, { 0, s * 6.0f, 0 }, 0.012f);   // split hoof
    }
}

// Joint positions in the body frame (front L/R, back L/R)
Vector3 CowHip(int leg) {
    const Vector3 j[4] = { { 0.2f, -0.28f, 0.6f }, { -0.2f, -0.28f, 0.6f }, { 0.18f, -0.14f, -0.86f }, { -0.18f, -0.14f, -0.86f } };
    return j[leg & 3];
}

// A collapsed skeleton: body on its side in the grass, legs splayed, skull turned.
static void BuildCowSkeleton(PrefabBuild& b, const Entity& e) {
    uint32_t seed = (uint32_t)e.Num("variant", 0) + 1;
    Rng r(seed);
    float side = r.Chance(0.5f) ? 1.0f : -1.0f;
    b.mb.Push();
    b.mb.Translate({ 0, 0.3f, 0 });
    b.mb.RotateZ(side * 78.0f);
    CowPartGeometry(b.mb, COW_BODY, seed);
    b.mb.Push(); b.mb.Translate({ 0, 0.02f, 0.85f }); b.mb.RotateY(r.Range(-35, 35)); b.mb.RotateX(r.Range(20, 50));
    CowPartGeometry(b.mb, COW_HEAD, seed);
    b.mb.Pop();
    for (int leg = 0; leg < 4; leg++) {
        Vector3 h = CowHip(leg);
        b.mb.Push(); b.mb.Translate(h); b.mb.RotateX(r.Range(-70, 20) + (leg < 2 ? -30.0f : 30.0f)); b.mb.RotateZ(r.Range(-15, 15));
        CowPartGeometry(b.mb, COW_UPPER, seed);
        b.mb.Translate({ 0, -0.52f, 0 }); b.mb.RotateX(r.Range(10, 90) * (leg < 2 ? 1.0f : -1.0f));
        CowPartGeometry(b.mb, COW_LOWER, seed);
        b.mb.Pop();
    }
    b.mb.Pop();
    // the ground under her is bare and dark
    b.M(MAT_MUD).Ellipsoid({ 0, -0.01f, -0.1f }, { 0.9f, 0.025f, 1.4f }, 4, 12);
    if (e.Num("dig", 0) > 0.5f) {
        // turned earth beside her: someone has dug here before
        b.M(MAT_DIRT).Ellipsoid({ side * -0.9f, 0.02f, 0.2f }, { 0.55f, 0.12f, 0.45f }, 5, 10);
        b.M(MAT_DIRT).Ellipsoid({ side * -1.35f, 0.04f, 0.5f }, { 0.3f, 0.14f, 0.25f }, 4, 8);
    }
    b.Collider({ 0, 0.25f, -0.1f }, { 0.9f, 0.5f, 2.2f }, SURF_GRASS);
    b.Interact("Bones", { side * -0.7f, 0.3f, 0.1f }, 2.6f, "cow");
    b.castShadow = true;
}

static void BuildBarnRuin(PrefabBuild& b, const Entity& e) {
    Rng r(5);
    const float W = 10.0f, D = 14.0f, H = 4.2f;
    // timber frame: posts, beams, a sagging ridge
    for (int i = 0; i <= 4; i++) {
        float z = -D * 0.5f + i * D / 4;
        for (int s = -1; s <= 1; s += 2) {
            b.Solid(MAT_WOOD_DARK, { s * W * 0.5f, H * 0.5f, z }, { 0.22f, H, 0.22f }, 0.02f, SURF_WOOD);
            b.M(MAT_WOOD_DARK).BoxRot({ s * W * 0.25f, H + 1.1f - (i == 2 ? 0.35f : 0.0f), z }, { W * 0.56f, 0.18f, 0.18f }, { 0, 0, s * -24.0f }, 0.02f);
        }
        b.M(MAT_WOOD_DARK).Box({ 0, H, z }, { W, 0.2f, 0.2f }, 0.02f);
    }
    b.M(MAT_WOOD_DARK).Tube({ { 0, H + 2.2f, -D * 0.5f }, { 0, H + 1.7f, 0 }, { 0, H + 2.2f, D * 0.5f } }, { 0.1f, 0.1f, 0.1f }, 6, true);
    // board walls, half of them gone
    for (int s = -1; s <= 1; s += 2)
        for (float z = -D * 0.5f + 0.15f; z < D * 0.5f; z += 0.3f) {
            if (r.Chance(0.35f)) continue;
            float h = r.Chance(0.25f) ? r.Range(0.8f, 2.5f) : H;
            b.M(MAT_WOOD).BoxRot({ s * (W * 0.5f + 0.13f), h * 0.5f, z }, { 0.03f, h, 0.28f }, { 0, 0, r.Range(-1.5f, 1.5f) });
        }
    for (float x = -W * 0.5f + 0.15f; x < W * 0.5f; x += 0.3f) {
        if (fabsf(x) < 1.8f || r.Chance(0.3f)) continue;   // the doorway
        b.M(MAT_WOOD).Box({ x, H * 0.5f, -D * 0.5f - 0.13f }, { 0.28f, H, 0.03f });
    }
    b.Collider({ W * 0.5f + 0.13f, H * 0.5f, 0 }, { 0.1f, H, D }, SURF_WOOD);
    b.Collider({ -W * 0.5f - 0.13f, H * 0.5f, 0 }, { 0.1f, H, D }, SURF_WOOD);
    b.Collider({ -3.4f, H * 0.5f, -D * 0.5f - 0.13f }, { 3.2f, H, 0.1f }, SURF_WOOD);
    b.Collider({ 3.4f, H * 0.5f, -D * 0.5f - 0.13f }, { 3.2f, H, 0.1f }, SURF_WOOD);
    // collapsed roof sheets: some still on, one slid to the ground
    for (int i = 0; i < 7; i++) {
        float z = -D * 0.5f + 1.0f + i * 1.9f;
        if (i == 3) continue;
        for (int s = -1; s <= 1; s += 2)
            if (!(s > 0 && i > 4)) b.M(MAT_CORRUGATED).BoxRot({ s * W * 0.26f, H + 1.2f, z }, { W * 0.6f, 0.02f, 1.85f }, { 0, 0, s * -24.0f });
    }
    b.M(MAT_CORRUGATED).BoxRot({ 3.8f, 0.9f, 4.6f }, { 4.0f, 0.02f, 1.85f }, { 0, 20, 62 });
    // hay, rotting
    for (int i = 0; i < 5; i++) {
        Vector3 p{ r.Range(-3.5f, 3.5f), 0.35f, r.Range(0, 6) };
        b.Solid(MAT_FOLIAGE_DRY, p, { 1.1f, 0.7f, 0.5f }, 0.08f, SURF_GRASS, r.Range(0, 180));
    }
    b.M(MAT_MUD).Box({ 0, 0.01f, 0 }, { W, 0.02f, D });
    (void)e;
}

// A lattice windmill; the wheel turns slowly and squeals when the wind rises.
struct WindmillBeh : Behaviour {
    float a = 0;
    Model3D* wheel = nullptr;
    void Update(Entity& e, float dt) override { a += dt * (0.4f + 0.3f * sinf(a * 0.1f)); (void)e; }
    void Draw(Entity& e) override {
        if (!wheel) return;
        Matrix m = MatrixMultiply(MatrixMultiply(MatrixRotateZ(a), MatrixTranslate(0, 9.2f, 0.45f)), e.xf);
        Rdr().Draw(wheel, m);
    }
};

static void BuildWindmill(PrefabBuild& b, const Entity& e) {
    for (int s = 0; s < 4; s++) {
        float x = (s & 1) ? 1.3f : -1.3f, z = (s & 2) ? 1.3f : -1.3f;
        b.M(MAT_RUST).Cylinder({ x, 0, z }, { x * 0.15f, 9.0f, z * 0.15f }, 0.05f, 0.04f, 6, true);
    }
    for (int k = 1; k < 6; k++) {
        float y = k * 1.6f, w = 1.3f * (1.0f - y / 10.6f);
        for (int s = 0; s < 4; s++) {
            Vector3 a{ (s == 0 || s == 3) ? -w : w, y, (s < 2) ? -w : w };
            Vector3 c{ (s == 0 || s == 1) ? w : -w, y, (s == 1 || s == 2) ? w : -w };
            b.M(MAT_RUST).Cylinder(a, c, 0.025f, 0.025f, 5, false);
        }
    }
    b.M(MAT_RUST).Box({ 0, 9.2f, 0 }, { 0.3f, 0.3f, 0.6f }, 0.03f);
    b.M(MAT_RUST).Box({ 0, 9.2f, -1.1f }, { 0.02f, 0.9f, 1.2f });   // tail vane
    b.ColliderCyl({ 0, 0, 0 }, 1.4f, 9.0f, SURF_METAL);
    (void)e;
}

static Model3D* WindmillWheel() {
    static Model3D* m = nullptr;
    if (m) return m;
    ModelBuilder mb;
    for (int i = 0; i < 18; i++) {
        if (i == 5 || i == 11) continue;   // missing blades
        mb.Push(); mb.RotateZ(i * 20.0f);
        mb.M(MAT_RUST).BoxRot({ 0, 1.05f, 0 }, { 0.2f, 1.4f, 0.01f }, { 0, 25, 0 });
        mb.Pop();
    }
    mb.M(MAT_RUST).Cylinder({ 0, 0, -0.1f }, { 0, 0, 0.1f }, 0.14f, 0.14f, 10, true);
    m = mb.Build(true);
    return m;
}

static void BuildTrough(PrefabBuild& b, const Entity& e) {
    b.M(MAT_STEEL).Box({ 0, 0.3f, 0 }, { 2.4f, 0.6f, 0.7f }, 0.04f);
    b.M(MAT_ROCK_WET).Box({ 0, 0.56f, 0 }, { 2.28f, 0.01f, 0.58f });   // black water
    b.Collider({ 0, 0.3f, 0 }, { 2.4f, 0.6f, 0.7f }, SURF_METAL);
    b.Interact("Trough", { 0, 0.6f, 0 }, 2.0f, "trough");
    (void)e;
}

static void BuildShovel(PrefabBuild& b, const Entity& e) {
    // leaning against whatever it was placed by
    b.mb.Push(); b.mb.RotateX(-14);
    b.M(MAT_WOOD).Cylinder({ 0, 0.26f, 0 }, { 0, 1.35f, 0 }, 0.018f, 0.018f, 8, true);
    b.M(MAT_WOOD).Box({ 0, 1.38f, 0 }, { 0.14f, 0.035f, 0.035f }, 0.01f);
    b.M(MAT_RUST).BoxRot({ 0, 0.14f, 0.01f }, { 0.2f, 0.28f, 0.012f }, { 8, 0, 0 }, 0.02f);
    b.mb.Pop();
    b.Interact("Shovel", { 0, 0.8f, -0.15f }, 2.0f, "shovel");
    b.castShadow = true;
    (void)e;
}

static void BuildHayBale(PrefabBuild& b, const Entity& e) {
    Rng r((uint32_t)e.Num("variant", 0) + 3);
    b.mb.Push(); b.mb.RotateZ(90);
    b.M(MAT_FOLIAGE_DRY).Cylinder({ 0, -0.6f, 0 }, { 0, 0.6f, 0 }, 0.72f, 0.7f, 18, true);
    b.mb.Pop();
    b.Collider({ 0, 0.7f, 0 }, { 1.2f, 1.4f, 1.4f }, SURF_GRASS);
    (void)r;
}

void RegisterFieldPrefabs() {
    auto reg = [](const char* n, void (*fn)(PrefabBuild&, const Entity&), std::vector<std::string> keys = {}) {
        PrefabInfo p; p.name = n; p.category = "field"; p.build = fn; p.geometryKeys = keys;
        RegisterPrefab(p);
    };
    reg("cow_skeleton", BuildCowSkeleton, { "variant", "dig" });
    reg("barn_ruin", BuildBarnRuin);
    reg("water_trough", BuildTrough);
    reg("shovel", BuildShovel);
    reg("hay_bale", BuildHayBale, { "variant" });
    // invisible marker: keeps the forest scatter out of an area (w x d metres)
    reg("clearing", [](PrefabBuild&, const Entity&) {}, { "w", "d" });
    {
        PrefabInfo p; p.name = "windmill"; p.category = "field"; p.build = BuildWindmill;
        p.behaviour = [](Entity&) { auto w = std::unique_ptr<WindmillBeh>(new WindmillBeh()); w->wheel = WindmillWheel(); return std::unique_ptr<Behaviour>(std::move(w)); };
        RegisterPrefab(p);
    }
}
