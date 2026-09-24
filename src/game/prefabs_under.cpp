// The world below: the stair tunnel behind the iron door, the cavern, the roots,
// the cart and the bicycle of the procession, Zain's chair, torches and the fuel can.
#include "prefab_util.h"

// Triangle facing towards `inside` (for shells seen from within)
static void InwardTri(MeshBuilder& m, Vector3 a, Vector3 b, Vector3 c, Vector3 inside) {
    Vector3 n = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
    Vector3 mid = Vector3Scale(Vector3Add(Vector3Add(a, b), c), 1.0f / 3.0f);
    if (Vector3DotProduct(n, Vector3Subtract(inside, mid)) < 0) { Vector3 t = b; b = c; c = t; }
    m.Tri(a, b, c);
}

static int RockMat() {
    static int m = -1;
    if (m < 0) {
        SurfaceMat s = Mat(MAT_ROCK_WET);
        s.name = "rock_cave"; s.tint = Color{ 92, 78, 70, 255 }; s.scale = 0.45f; s.wet = 0.65f; s.normalStr = 1.6f;
        m = AddMaterial(s);
    }
    return m;
}

// ---------------------------------------------------------------------------
// Stair tunnel: origin at the iron door threshold, descends along +z.
// ---------------------------------------------------------------------------
static void BuildUnderStairs(PrefabBuild& b, const Entity& e) {
    const float W = 1.7f, H = 2.4f;
    const int steps = 26;
    const float run = 0.3f, rise = 0.19f;
    const float z0 = 2.5f;                      // landing behind the door
    const float z1 = z0 + steps * run;          // bottom of the flight
    const float yb = -steps * rise;
    int rock = RockMat();
    // landing + steps + bottom landing
    b.Solid(MAT_CONCRETE_DARK, { 0, -0.15f, z0 * 0.5f }, { W, 0.3f, z0 }, 0.01f, SURF_CONCRETE);
    for (int k = 0; k < steps; k++) {
        float top = -(k + 1) * rise;
        b.Solid(k < 8 ? MAT_CONCRETE_DARK : rock, { 0, top - 0.2f, z0 + k * run + run * 0.5f }, { W, 0.4f, run }, 0.01f, SURF_CONCRETE);
    }
    b.Solid(rock, { 0, yb - 0.2f, z1 + 3.0f }, { W + 0.6f, 0.4f, 6.0f }, 0.02f, SURF_DIRT);
    // walls and ceiling follow the slope; bricks give way to raw rock
    auto seg = [&](float za, float zb, float ya, float yb2, int mat) {
        for (int s = -1; s <= 1; s += 2) {
            Vector3 a{ s * W * 0.5f, ya - 0.6f, za }, c{ s * W * 0.5f, yb2 - 0.6f, zb };
            b.M(mat).Quad(a, { a.x, ya + H, za }, { c.x, yb2 + H, zb }, c);
            b.M(mat).Quad(c, { c.x, yb2 + H, zb }, { a.x, ya + H, za }, a);
        }
        Vector3 p0{ -W * 0.5f, ya + H, za }, p1{ W * 0.5f, ya + H, za }, p2{ W * 0.5f, yb2 + H, zb }, p3{ -W * 0.5f, yb2 + H, zb };
        b.M(mat).Quad(p0, p1, p2, p3);
        b.M(mat).Quad(p3, p2, p1, p0);
        float len = sqrtf((zb - za) * (zb - za) + (yb2 - ya) * (yb2 - ya));
        Vector3 c{ 0, (ya + yb2) * 0.5f + H * 0.5f - 0.3f, (za + zb) * 0.5f };
        float pitch = atan2f(ya - yb2, zb - za) * RAD2DEG;
        b.Collider({ -W * 0.5f - 0.1f, c.y, c.z }, { 0.2f, H + 0.6f, len }, SURF_CONCRETE, 0, false, true);
        b.Collider({ W * 0.5f + 0.1f, c.y, c.z }, { 0.2f, H + 0.6f, len }, SURF_CONCRETE, 0, false, true);
        (void)pitch;
    };
    seg(0.0f, z0, 0.0f, 0.0f, MAT_BRICK_DARK);
    for (int i = 0; i < 4; i++) {
        float za = z0 + i * (steps * run / 4), zb = za + steps * run / 4;
        seg(za, zb, -i * (steps * rise / 4), -(i + 1) * (steps * rise / 4), i < 1 ? MAT_BRICK_DARK : rock);
    }
    seg(z1, z1 + 6.0f, yb, yb, rock);
    // end of the tunnel: the dark goes on (the transition happens here)
    b.M(MAT_PAINT_BLACK).Box({ 0, yb + H * 0.5f, z1 + 6.05f }, { W, H + 0.6f, 0.05f });
    b.Collider({ 0, yb + H * 0.5f, z1 + 6.2f }, { W + 0.4f, H + 0.6f, 0.3f }, SURF_DIRT);
    // roots breaking through, wet streaks, an old caged bulb that no longer works
    Rng r(17);
    for (int i = 0; i < 10; i++) {
        float z = r.Range(z0 + 2.0f, z1 + 5.0f);
        float yTop = z < z1 ? -((z - z0) / run) * rise + H : yb + H;
        float s = r.Chance(0.5f) ? 1.0f : -1.0f;
        std::vector<Vector3> pts; std::vector<float> rad;
        for (int k = 0; k <= 6; k++) {
            float u = k / 6.0f;
            pts.push_back({ s * (W * 0.5f - 0.05f - u * 0.3f), yTop - 0.05f - u * r.Range(0.6f, 1.6f), z + sinf(u * 3 + i) * 0.2f });
            rad.push_back(Lerp(0.05f, 0.008f, u));
        }
        b.M(MAT_BARK_DARK).Tube(pts, rad, 5, true);
    }
    b.M(MAT_STEEL).Box({ 0, H - 0.1f, 1.2f }, { 0.12f, 0.12f, 0.12f }, 0.01f);
    b.castShadow = true;
    (void)e;
}

// ---------------------------------------------------------------------------
// The cavern: an irregular dome over a mud floor. Local origin at the entry
// (south end); the chamber opens along +z to the roots at z ~ 88.
// ---------------------------------------------------------------------------
static void BuildCavern(PrefabBuild& b, const Entity& e) {
    const Vector3 C{ 0, 0, 46 };
    const Vector3 R{ 34, 15, 50 };
    int rock = RockMat();
    MeshBuilder& shell = b.M(rock);
    const int NU = 48, NV = 14;
    auto P = [&](int iu, int iv) {
        float th = (float)iu / NU * 2 * PI;
        float ph = -0.08f + (float)iv / NV * (PI * 0.5f + 0.08f);   // a lip below the floor
        return Vector3{ C.x + cosf(th) * cosf(ph) * R.x, C.y + sinf(ph) * R.y, C.z + sinf(th) * cosf(ph) * R.z };
    };
    Vector3 inside{ C.x, 4.0f, C.z };
    for (int iv = 0; iv < NV; iv++)
        for (int iu = 0; iu < NU; iu++) {
            Vector3 a = P(iu, iv), b2 = P(iu + 1, iv), c = P(iu + 1, iv + 1), d = P(iu, iv + 1);
            InwardTri(shell, a, b2, c, inside);
            InwardTri(shell, a, c, d, inside);
        }
    // bumpy, stratified rock: push vertices along the radius with layered noise
    b.mb.Deform([&](Vector3 p) {
        Vector3 d = Vector3Subtract(p, C);
        float len = Vector3Length(d);
        if (len < 0.01f) return p;
        Vector3 n = Vector3Scale(d, 1.0f / len);
        float strata = sinf(p.y * 1.7f + Fbm2(p.x * 0.05f, p.z * 0.05f, 2, 0, 3) * 3.0f) * 0.5f;
        float bump = Fbm2(p.x * 0.09f + p.y * 0.05f, p.z * 0.09f - p.y * 0.04f, 4, 0, 11) * 5.0f + Ridged2(p.x * 0.03f, p.z * 0.03f, 3, 0, 7) * 3.0f;
        float out = bump + strata;
        if (p.y < 1.0f) out *= 0.6f;
        return Vector3Add(p, Vector3Scale(n, -out));
    }, true);
    // floor: packed wet mud, subtly uneven, with the worn road down the middle
    {
        const float x0 = -36, x1 = 36, zA = -6, zB = 98, st = 2.0f;
        MeshBuilder& fl = b.M(MAT_MUD);
        auto H = [](float x, float z) { return Fbm2(x * 0.12f, z * 0.12f, 3, 0, 21) * 0.08f; };
        for (float z = zA; z < zB; z += st)
            for (float x = x0; x < x1; x += st) {
                Vector3 a{ x, H(x, z), z }, bq{ x + st, H(x + st, z), z }, c{ x + st, H(x + st, z + st), z + st }, d{ x, H(x, z + st), z + st };
                fl.Tri(a, d, c); fl.Tri(a, c, bq);
            }
        b.Collider({ 0, -0.5f, 46 }, { 72, 1.0f, 104 }, SURF_MUD);
        b.M(MAT_DIRT).Box({ 0, 0.05f, 44 }, { 3.2f, 0.02f, 88 });
    }
    // walls: a ring of colliders roughly on the dome's footprint
    for (int i = 0; i < 32; i++) {
        float th = (i + 0.5f) / 32 * 2 * PI;
        Vector3 p{ C.x + cosf(th) * R.x * 0.84f, 3.0f, C.z + sinf(th) * R.z * 0.86f };
        float yawDeg = -th * RAD2DEG;
        b.Collider(p, { 2.0f, 6.0f, 11.0f }, SURF_GRAVEL, yawDeg + 90.0f, false, true);
    }
    // stalactites and columns
    Rng r(23);
    for (int i = 0; i < 40; i++) {
        float x = r.Range(-26, 26), z = r.Range(6, 90);
        if (fabsf(x) < 4.0f) continue;
        float nx = (x - C.x) / R.x, nz = (z - C.z) / R.z;
        float top = C.y + R.y * sqrtf(fmaxf(0.05f, 1 - nx * nx - nz * nz)) - 2.5f;
        float len = r.Range(0.8f, 4.0f);
        b.M(rock).Cylinder({ x, top + 1.5f, z }, { x + r.Range(-0.3f, 0.3f), top - len, z }, r.Range(0.25f, 0.7f), 0.03f, 7, false);
        if (r.Chance(0.2f)) {
            b.M(rock).Cylinder({ x, 0, z }, { x, top + 1.0f, z }, r.Range(0.9f, 1.8f), r.Range(0.6f, 1.2f), 10, false);
            b.ColliderCyl({ x, 0, z }, 1.2f, top, SURF_GRAVEL);
        }
    }
    // the roots: the ground's own veins coming down through the ceiling at the far end
    for (int i = 0; i < 14; i++) {
        float ang = r.Range(0, 2 * PI), dist = r.Range(3.0f, 12.0f);
        Vector3 base{ cosf(ang) * dist, 0, 86.0f + sinf(ang) * dist * 0.5f };
        std::vector<Vector3> pts; std::vector<float> rad;
        float thick = r.Range(0.35f, 1.1f);
        for (int k = 0; k <= 10; k++) {
            float u = k / 10.0f;
            pts.push_back({ base.x * (1.0f - u * 0.6f) + sinf(u * 5 + i) * 0.8f, 13.0f * u - 0.3f, base.z + cosf(u * 4 + i) * 0.8f });
            rad.push_back(thick * (1.2f - u * 0.4f));
        }
        b.M(MAT_BARK_DARK).Tube(pts, rad, 9, true);
        if (thick > 0.6f) b.ColliderCyl({ base.x, 0, base.z }, thick, 3.0f, SURF_WOOD);
    }
    // bone drifts and fungus along the edges
    for (int i = 0; i < 26; i++) {
        float th = r.Range(0, 2 * PI);
        Vector3 p{ C.x + cosf(th) * R.x * r.Range(0.55f, 0.75f), 0, C.z + sinf(th) * R.z * r.Range(0.55f, 0.75f) };
        for (int k = 0; k < 6; k++)
            b.M(MAT_BONE).BoxRot({ p.x + r.Range(-0.8f, 0.8f), 0.05f, p.z + r.Range(-0.8f, 0.8f) }, { 0.04f, 0.04f, r.Range(0.25f, 0.5f) }, { r.Range(-10, 10), r.Range(0, 180), 0 }, 0.015f);
        if (r.Chance(0.5f)) b.M(MAT_BONE).Ellipsoid({ p.x, 0.1f, p.z }, { 0.1f, 0.12f, 0.12f }, 5, 7);
        if (r.Chance(0.6f))
            for (int k = 0; k < 7; k++) {
                Vector3 f{ p.x + r.Range(-1.2f, 1.2f), 0, p.z + r.Range(-1.2f, 1.2f) };
                float h = r.Range(0.05f, 0.18f);
                b.M(MAT_BONE).Cylinder(f, { f.x, h, f.z }, 0.012f, 0.01f, 5, false);
                b.M(MAT_SCREEN_GREEN).Ellipsoid({ f.x, h, f.z }, { 0.05f, 0.02f, 0.05f }, 3, 6);
            }
    }
    // entry: a rough stair mouth in the south wall behind the origin
    for (int k = 0; k < 6; k++) b.Solid(rock, { 0, k * 0.2f + 0.1f, -1.5f - k * 0.35f }, { 2.4f, 0.2f + k * 0.4f, 0.35f }, 0.03f, SURF_GRAVEL);
    b.castShadow = true;
    (void)e;
}

// Torch on a pole; its light belongs to the "below_torch" group
static void BuildTorch(PrefabBuild& b, const Entity& e) {
    b.M(MAT_WOOD_DARK).Cylinder({ 0, 0, 0 }, { 0.05f, 1.9f, 0.02f }, 0.04f, 0.03f, 6, true);
    b.M(MAT_CLOTH_BROWN).Cylinder({ 0.05f, 1.8f, 0.02f }, { 0.05f, 2.02f, 0.02f }, 0.06f, 0.07f, 8, true);
    b.M(MAT_FIRE).Ellipsoid({ 0.05f, 2.14f, 0.02f }, { 0.06f, 0.14f, 0.06f }, 5, 7);
    LightDef& l = b.AddLight({ 0.05f, 2.3f, 0.02f }, { 1.0f, 0.46f, 0.16f }, 3.2f, 13.0f, 0.12f);
    l.flicker = 0.3f;
    l.group = "below_torch";
    b.ColliderCyl({ 0, 0, 0 }, 0.08f, 2.0f, SURF_WOOD);
    (void)e;
}

// A two-wheeled cart with shafts (pulled by people, down here)
static void BuildCart(PrefabBuild& b, const Entity& e) {
    b.M(MAT_WOOD_DARK).Box({ 0, 0.85f, 0 }, { 1.5f, 0.08f, 2.2f }, 0.02f);
    for (int s = -1; s <= 1; s += 2) {
        b.M(MAT_WOOD).Box({ s * 0.72f, 1.1f, 0 }, { 0.05f, 0.45f, 2.2f }, 0.01f);
        b.M(MAT_WOOD_DARK).Cylinder({ s * 0.4f, 0.8f, 1.1f }, { s * 0.35f, 0.55f, 3.2f }, 0.04f, 0.035f, 6, true);   // shafts
        // spoked wheel
        b.mb.Push(); b.mb.Translate({ s * 0.85f, 0.6f, -0.2f }); b.mb.RotateZ(90);
        b.M(MAT_WOOD_DARK).Cylinder({ 0, -0.04f, 0 }, { 0, 0.04f, 0 }, 0.6f, 0.6f, 20, true);
        b.mb.Pop();
    }
    b.M(MAT_STEEL).Cylinder({ -0.9f, 0.6f, -0.2f }, { 0.9f, 0.6f, -0.2f }, 0.035f, 0.035f, 6, false);
    // the load: sacks that are not sacks
    Rng r(3);
    for (int i = 0; i < 5; i++)
        b.M(i % 2 ? MAT_CLOTH_BROWN : MAT_FLESH).Ellipsoid({ r.Range(-0.4f, 0.4f), 1.05f, r.Range(-0.8f, 0.8f) }, { 0.3f, 0.2f, 0.45f }, 6, 9);
    (void)e;
}

static void BuildBicycle(PrefabBuild& b, const Entity& e) {
    for (float z : { -0.52f, 0.52f }) {
        b.mb.Push(); b.mb.Translate({ 0, 0.34f, z }); b.mb.RotateZ(90);
        b.M(MAT_RUBBER).Cylinder({ 0, -0.02f, 0 }, { 0, 0.02f, 0 }, 0.34f, 0.34f, 20, true);
        b.M(MAT_CHROME).Cylinder({ 0, -0.025f, 0 }, { 0, 0.025f, 0 }, 0.05f, 0.05f, 8, true);
        b.mb.Pop();
    }
    auto tube = [&](Vector3 a, Vector3 c) { b.M(MAT_RUST).Cylinder(a, c, 0.018f, 0.018f, 6, false); };
    Vector3 bb{ 0, 0.32f, -0.05f }, seat{ 0, 0.85f, -0.22f }, head{ 0, 0.85f, 0.38f };
    tube(bb, seat); tube(bb, head); tube(seat, head); tube(bb, { 0, 0.34f, -0.52f }); tube(seat, { 0, 0.34f, -0.52f });
    tube(head, { 0, 0.34f, 0.52f });
    b.M(MAT_LEATHER).Box({ 0, 0.9f, -0.24f }, { 0.12f, 0.04f, 0.24f }, 0.02f);
    b.M(MAT_RUST).Cylinder({ -0.25f, 0.98f, 0.36f }, { 0.25f, 0.98f, 0.36f }, 0.015f, 0.015f, 6, true);
    tube(head, { 0, 0.98f, 0.36f });
    (void)e;
}

// The chair at the roots. The ropes hang loose, knotted at the front.
static void BuildChair(PrefabBuild& b, const Entity& e) {
    b.M(MAT_WOOD).Box({ 0, 0.46f, 0 }, { 0.46f, 0.04f, 0.44f }, 0.01f);
    b.M(MAT_WOOD).Box({ 0, 0.9f, -0.21f }, { 0.46f, 0.84f, 0.04f }, 0.01f);
    for (int s = -1; s <= 1; s += 2) for (int k = -1; k <= 1; k += 2)
        b.M(MAT_WOOD).Box({ s * 0.2f, 0.23f, k * 0.19f }, { 0.04f, 0.46f, 0.04f }, 0.005f);
    std::vector<Vector3> rope;
    for (int i = 0; i <= 16; i++) { float a = i / 16.0f * 2 * PI; rope.push_back({ sinf(a) * 0.27f, 0.75f + (i % 3) * 0.01f, cosf(a) * 0.14f - 0.07f }); }
    b.M(MAT_CLOTH_BROWN).Tube(rope, std::vector<float>(rope.size(), 0.012f), 5, false);
    b.M(MAT_CLOTH_BROWN).Ellipsoid({ 0, 0.75f, 0.08f }, { 0.04f, 0.03f, 0.03f }, 4, 6);   // the knot, in front
    b.M(MAT_CLOTH_BROWN).Tube({ { 0.02f, 0.75f, 0.09f }, { 0.04f, 0.5f, 0.14f }, { 0.03f, 0.3f, 0.12f } }, { 0.01f, 0.01f, 0.008f }, 5, true);
    b.Collider({ 0, 0.45f, 0 }, { 0.5f, 0.9f, 0.5f }, SURF_WOOD);
    (void)e;
}

static void BuildFuelCan(PrefabBuild& b, const Entity& e) {
    b.M(MAT_PAINT_RED).Box({ 0, 0.2f, 0 }, { 0.18f, 0.38f, 0.3f }, 0.02f);
    b.M(MAT_PAINT_RED).Cylinder({ 0, 0.39f, 0.08f }, { 0, 0.46f, 0.13f }, 0.025f, 0.02f, 8, true);
    b.M(MAT_BLACK_PLASTIC).Box({ 0, 0.42f, -0.05f }, { 0.03f, 0.04f, 0.14f }, 0.01f);
    b.Interact("Fuel can", { 0, 0.3f, 0 }, 2.0f, "fuel_can");
    b.Collider({ 0, 0.2f, 0 }, { 0.2f, 0.4f, 0.32f }, SURF_METAL);
    (void)e;
}

void RegisterUnderPrefabs() {
    auto reg = [](const char* n, void (*fn)(PrefabBuild&, const Entity&)) {
        PrefabInfo p; p.name = n; p.category = "below"; p.build = fn;
        RegisterPrefab(p);
    };
    reg("under_stairs", BuildUnderStairs);
    reg("torch", BuildTorch);
    reg("cart", BuildCart);
    reg("bicycle", BuildBicycle);
    reg("chair", BuildChair);
    reg("fuel_can", BuildFuelCan);
    {
        PrefabInfo p; p.name = "cavern"; p.category = "below"; p.build = BuildCavern; p.snap = false;
        RegisterPrefab(p);
    }
}
