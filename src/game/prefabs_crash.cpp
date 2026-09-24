// The brothers' sedan (intact / wrecked), its moving parts, and the radio tower.
//
// Sedan local frame: faces +Z, origin on the ground midway between the axles, +X is the car's
// left (driver side), wheelbase 2.7 m, track 1.5 m, length 4.7 m.
#include "prefab_util.h"
#include "sedan.h"

// ---------------------------------------------------------------------------
// Loft helper: symmetric cross-sections along Z with smooth normals.
// ---------------------------------------------------------------------------
struct LoftStation { float z; std::vector<Vector2> pts; };   // right half, bottom-centre -> top-centre

static void Loft(ModelBuilder& mb, const std::vector<LoftStation>& st, const std::function<int(int seg, float z0, float z1)>& matFor) {
    int ns = (int)st.size(), np = (int)st[0].pts.size();
    auto V = [&](int s, int i) { return Vector3{ st[s].pts[i].x, st[s].pts[i].y, st[s].z }; };
    std::vector<Vector3> N((size_t)ns * np);
    for (int s = 0; s < ns; s++)
        for (int i = 0; i < np; i++) {
            Vector3 dz = Vector3Subtract(V(s < ns - 1 ? s + 1 : s, i), V(s > 0 ? s - 1 : s, i));
            Vector3 dp = Vector3Subtract(V(s, i < np - 1 ? i + 1 : i), V(s, i > 0 ? i - 1 : i));
            Vector3 n = Vector3Normalize(Vector3CrossProduct(dz, dp));
            Vector3 out{ V(s, i).x, V(s, i).y - 0.6f, 0 };
            if (Vector3DotProduct(n, out) < 0) n = Vector3Negate(n);
            N[(size_t)s * np + i] = n;
        }
    for (int s = 0; s + 1 < ns; s++)
        for (int i = 0; i + 1 < np; i++) {
            int mat = matFor(i, st[s].z, st[s + 1].z);
            if (mat < 0) continue;
            MeshBuilder& m = mb.M(mat);
            Vector3 a = V(s, i), b = V(s, i + 1), c = V(s + 1, i + 1), d = V(s + 1, i);
            Vector3 na = N[(size_t)s * np + i], nb = N[(size_t)s * np + i + 1], nc = N[(size_t)(s + 1) * np + i + 1], nd = N[(size_t)(s + 1) * np + i];
            // right side
            Vector3 fn = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
            if (Vector3DotProduct(fn, na) >= 0) { m.TriN(a, b, c, na, nb, nc); m.TriN(a, c, d, na, nc, nd); }
            else { m.TriN(a, c, b, na, nc, nb); m.TriN(a, d, c, na, nd, nc); }
            // mirrored left side
            auto Mx = [](Vector3 v) { return Vector3{ -v.x, v.y, v.z }; };
            Vector3 A = Mx(a), B = Mx(b), C = Mx(c), D = Mx(d), NA = Mx(na), NB = Mx(nb), NC = Mx(nc), ND = Mx(nd);
            Vector3 fn2 = Vector3CrossProduct(Vector3Subtract(B, A), Vector3Subtract(C, A));
            if (Vector3DotProduct(fn2, NA) >= 0) { m.TriN(A, B, C, NA, NB, NC); m.TriN(A, C, D, NA, NC, ND); }
            else { m.TriN(A, C, B, NA, NC, NB); m.TriN(A, D, C, NA, ND, NC); }
        }
}

// Body section parameters along the length
struct Sec { float hw, bot, belt, top; };
static Sec BodyAt(float z) {
    struct K { float z; Sec s; };
    static const K keys[] = {
        { -2.36f, { 0.74f, 0.40f, 0.86f, 0.86f } }, { -2.28f, { 0.84f, 0.30f, 0.94f, 0.95f } },
        { -1.95f, { 0.88f, 0.26f, 0.99f, 1.00f } }, { -1.30f, { 0.89f, 0.25f, 1.00f, 1.00f } },
        { 0.00f, { 0.895f, 0.22f, 0.985f, 0.985f } }, { 0.95f, { 0.89f, 0.24f, 0.965f, 0.965f } },
        { 1.40f, { 0.885f, 0.26f, 0.93f, 0.93f } }, { 1.95f, { 0.87f, 0.28f, 0.86f, 0.865f } },
        { 2.28f, { 0.81f, 0.32f, 0.77f, 0.775f } }, { 2.37f, { 0.72f, 0.38f, 0.70f, 0.70f } } };
    const int n = sizeof(keys) / sizeof(keys[0]);
    if (z <= keys[0].z) return keys[0].s;
    for (int i = 0; i + 1 < n; i++)
        if (z <= keys[i + 1].z) {
            float t = (z - keys[i].z) / (keys[i + 1].z - keys[i].z);
            t = t * t * (3 - 2 * t);
            const Sec &a = keys[i].s, &b = keys[i + 1].s;
            return { Lerp(a.hw, b.hw, t), Lerp(a.bot, b.bot, t), Lerp(a.belt, b.belt, t), Lerp(a.top, b.top, t) };
        }
    return keys[n - 1].s;
}
static const float kCabinZ0 = -1.26f, kCabinZ1 = 0.97f;   // greenhouse extent
static float RoofY(float z) {
    if (z < -0.72f) return Lerp(1.0f, 1.40f, SmoothStep(kCabinZ0, -0.72f, z));
    if (z > 0.34f) return Lerp(1.42f, 0.975f, SmoothStep(0.34f, kCabinZ1, z));
    return Lerp(1.40f, 1.42f, SmoothStep(-0.72f, 0.34f, z));
}

static void BuildSedanBody(ModelBuilder& mb, int paint, bool wrecked) {
    // ---- lower body loft
    std::vector<LoftStation> st;
    for (int i = 0; i <= 44; i++) {
        float z = -2.36f + 4.73f * i / 44.0f;
        Sec s = BodyAt(z);
        LoftStation ls; ls.z = z;
        ls.pts = { { 0, s.bot }, { s.hw * 0.85f, s.bot }, { s.hw - 0.02f, s.bot + 0.05f }, { s.hw, s.bot + (s.belt - s.bot) * 0.42f },
                   { s.hw - 0.012f, s.belt - 0.06f }, { s.hw - 0.05f, s.belt }, { s.hw - 0.14f, s.top + 0.012f }, { 0, s.top + 0.03f } };
        st.push_back(ls);
    }
    Loft(mb, st, [&](int seg, float z0, float z1) -> int {
        if (seg == 0) return MAT_BLACK_PLASTIC;                       // underside
        bool cabin = z0 >= kCabinZ0 - 0.02f && z1 <= kCabinZ1 + 0.02f;
        if (cabin && seg >= 5) return -1;                             // open top: the greenhouse sits here
        return paint;
    });
    // ---- greenhouse (glass sides, painted roof, windshield + rear window)
    std::vector<LoftStation> gh;
    for (int i = 0; i <= 24; i++) {
        float z = kCabinZ0 + (kCabinZ1 - kCabinZ0) * i / 24.0f;
        Sec s = BodyAt(z);
        float ry = RoofY(z);
        float hwTop = 0.66f;
        LoftStation ls; ls.z = z;
        ls.pts = { { s.hw - 0.05f, s.belt }, { hwTop, ry }, { hwTop - 0.05f, ry + 0.012f }, { 0, ry + 0.022f } };
        gh.push_back(ls);
    }
    Loft(mb, gh, [&](int seg, float z0, float z1) -> int {
        bool roof = z0 >= -0.74f && z1 <= 0.36f;
        if (seg == 0) {
            // side glass except the B pillar
            if (z0 < -0.14f && z1 > -0.2f) return paint;
            return wrecked ? MAT_GLASS_DIRTY : MAT_GLASS_DARK;
        }
        if (roof) return paint;
        return wrecked ? MAT_GLASS_DIRTY : MAT_GLASS;
    });
    // pillars
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        float x = sgn * 1.0f;
        Sec s0 = BodyAt(kCabinZ1), s1 = BodyAt(kCabinZ0);
        mb.M(paint).Tube({ { x * (s0.hw - 0.06f), s0.belt, kCabinZ1 - 0.02f }, { x * 0.665f, 1.41f, 0.34f } }, { 0.035f, 0.032f }, 6, true);
        mb.M(paint).Tube({ { x * (s1.hw - 0.06f), s1.belt, kCabinZ0 + 0.02f }, { x * 0.665f, 1.39f, -0.72f } }, { 0.06f, 0.05f }, 6, true);
        mb.M(paint).Tube({ { x * 0.83f, 0.99f, -0.17f }, { x * 0.67f, 1.41f, -0.17f } }, { 0.045f, 0.04f }, 6, true);
        // door seams, handles, mirrors, trim strip
        for (float zz : { -0.17f, 1.0f, -1.2f }) mb.M(MAT_BLACK_PLASTIC).Box({ x * 0.9f, 0.62f, zz }, { 0.012f, 0.6f, 0.006f });
        for (float zz : { 0.25f, -0.55f }) mb.M(MAT_CHROME).Box({ x * 0.905f, 0.88f, zz }, { 0.02f, 0.03f, 0.13f }, 0.008f);
        mb.M(MAT_BLACK_PLASTIC).Box({ x * 0.91f, 0.6f, -0.1f }, { 0.018f, 0.05f, 2.9f }, 0.01f);
        mb.M(paint).Box({ x * 0.97f, 1.03f, 0.83f }, { 0.12f, 0.1f, 0.06f }, 0.02f);
        mb.M(MAT_CHROME).Box({ x * 1.0f, 1.03f, 0.8f }, { 0.08f, 0.07f, 0.01f });
    }
    // ---- bumpers, grille, lights, plates
    mb.M(MAT_GREY_PLASTIC).Box({ 0, 0.45f, 2.33f }, { 1.66f, 0.2f, 0.16f }, 0.05f);
    mb.M(MAT_GREY_PLASTIC).Box({ 0, 0.47f, -2.33f }, { 1.66f, 0.2f, 0.16f }, 0.05f);
    mb.M(MAT_BLACK_PLASTIC).Box({ 0, 0.64f, 2.36f }, { 0.8f, 0.14f, 0.04f }, 0.02f);
    for (int k = -3; k <= 3; k++) mb.M(MAT_CHROME).Box({ k * 0.1f, 0.64f, 2.382f }, { 0.012f, 0.11f, 0.01f });
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        mb.M(MAT_CHROME).Box({ sgn * 0.6f, 0.66f, 2.33f }, { 0.36f, 0.14f, 0.06f }, 0.03f);
        mb.M(MAT_HEADLIGHT_LENS).Box({ sgn * 0.6f, 0.66f, 2.36f }, { 0.32f, 0.11f, 0.02f }, 0.02f);
        mb.M(MAT_BULB_AMBER).Box({ sgn * 0.83f, 0.64f, 2.3f }, { 0.08f, 0.1f, 0.05f }, 0.01f);
        mb.M(MAT_TAILLIGHT).Box({ sgn * 0.62f, 0.8f, -2.34f }, { 0.36f, 0.14f, 0.04f }, 0.02f);
        mb.M(MAT_CHROME).Box({ sgn * 0.62f, 0.8f, -2.345f }, { 0.38f, 0.16f, 0.03f }, 0.02f);
    }
    int plate = SignMaterial("plate", "9QW 714\nROUTE STATE", ui::F_MONO_BOLD, 48, Color{ 30, 40, 90, 255 }, Color{ 225, 225, 215, 255 }, 256, 128, 0.5f);
    mb.M(plate).BoxUV({ 0, 0.62f, -2.43f + 0.0f }, { 0.3f, 0.15f, 0.005f });
    mb.M(MAT_STEEL).Cylinder({ 0.5f, 0.3f, -2.3f }, { 0.52f, 0.28f, -2.5f }, 0.03f, 0.03f, 8, false);   // exhaust
    // wheel arch liners
    for (int sgn = -1; sgn <= 1; sgn += 2)
        for (float wz : { 1.35f, -1.35f }) {
            std::vector<Vector3> arc; std::vector<float> rr;
            for (int k = 0; k <= 10; k++) {
                float a = PI * k / 10.0f;
                arc.push_back({ sgn * 0.84f, 0.31f + sinf(a) * 0.37f, wz + cosf(a) * 0.4f });
                rr.push_back(0.07f);
            }
            mb.M(MAT_BLACK_PLASTIC).Tube(arc, rr, 6, true);
        }
    // wipers + antenna
    for (int sgn = -1; sgn <= 1; sgn += 2) mb.M(MAT_BLACK_PLASTIC).Tube({ { sgn * 0.05f, 0.99f, 0.98f }, { sgn * 0.55f, 1.0f, 0.93f } }, { 0.008f, 0.006f }, 4, true);
    mb.M(MAT_CHROME).Cylinder({ -0.7f, 0.99f, -1.6f }, { -0.7f, 1.7f, -1.72f }, 0.004f, 0.002f, 4, false);

    // ---- interior -------------------------------------------------------------
    mb.M(MAT_CLOTH_DARK).Box({ 0, 0.31f, -0.2f }, { 1.6f, 0.04f, 3.0f });                         // floor carpet
    mb.M(MAT_CLOTH_GREY).Box({ 0, 1.385f, -0.2f }, { 1.3f, 0.02f, 1.1f });                         // headliner
    mb.M(MAT_BLACK_PLASTIC).Box({ 0, 0.9f, -1.5f }, { 1.5f, 0.04f, 0.5f });                        // parcel shelf
    for (int sgn = -1; sgn <= 1; sgn += 2)
        mb.M(MAT_GREY_PLASTIC).Box({ sgn * 0.8f, 0.66f, -0.25f }, { 0.04f, 0.62f, 2.1f }, 0.01f);   // door cards
    // dashboard
    mb.M(MAT_BLACK_PLASTIC).Box({ 0, 0.82f, 0.72f }, { 1.6f, 0.34f, 0.44f }, 0.05f);
    mb.M(MAT_BLACK_PLASTIC).BoxRot({ 0, 0.98f, 0.8f }, { 1.6f, 0.06f, 0.36f }, { -12, 0, 0 }, 0.02f);
    mb.M(MAT_BLACK_PLASTIC).Box({ kDriverX, 1.02f, 0.55f }, { 0.42f, 0.1f, 0.16f }, 0.04f);       // cluster hood
    mb.M(MAT_GREY_PLASTIC).Box({ 0, 0.5f, 0.2f }, { 0.22f, 0.36f, 0.9f }, 0.03f);                  // console
    mb.M(MAT_BLACK_PLASTIC).Box({ 0, 0.82f, 0.51f }, { 0.22f, 0.07f, 0.02f }, 0.01f);             // radio
    mb.M(MAT_SCREEN_GREEN).Box({ 0.03f, 0.83f, 0.499f }, { 0.08f, 0.02f, 0.002f });
    mb.M(MAT_BLACK_PLASTIC).Cylinder({ 0, 0.6f, 0.05f }, { 0, 0.78f, 0.02f }, 0.01f, 0.01f, 6, false);   // gear lever
    mb.M(MAT_LEATHER).Sphere({ 0, 0.79f, 0.02f }, 0.03f, 6, 8);
    // seats
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        float x = sgn * 0.38f;
        mb.M(MAT_CLOTH_GREY).Box({ x, 0.48f, -0.2f }, { 0.5f, 0.14f, 0.52f }, 0.05f);
        mb.M(MAT_CLOTH_GREY).BoxRot({ x, 0.85f, -0.5f }, { 0.5f, 0.66f, 0.13f }, { -14, 0, 0 }, 0.05f);
        mb.M(MAT_CLOTH_GREY).Box({ x, 1.25f, -0.58f }, { 0.26f, 0.16f, 0.1f }, 0.04f);           // headrest
    }
    mb.M(MAT_CLOTH_GREY).Box({ 0, 0.45f, -1.12f }, { 1.35f, 0.14f, 0.5f }, 0.05f);
    mb.M(MAT_CLOTH_GREY).BoxRot({ 0, 0.78f, -1.36f }, { 1.35f, 0.6f, 0.14f }, { -18, 0, 0 }, 0.05f);
    // rear-view mirror + a pine air freshener + photo tucked in the visor (their mother)
    mb.M(MAT_BLACK_PLASTIC).Box({ 0, 1.3f, 0.52f }, { 0.24f, 0.06f, 0.03f }, 0.01f);
    mb.M(MAT_BLACK_PLASTIC).Box({ kDriverX, 1.37f, 0.35f }, { 0.34f, 0.02f, 0.18f }, 0.01f);
    mb.M(MAT_PAPER).Box({ kDriverX + 0.08f, 1.36f, 0.36f }, { 0.07f, 0.003f, 0.09f });
    if (wrecked) {
        // airbag sagging from the wheel, glass crumbs on the dash
        mb.M(MAT_CLOTH_WHITE).Ellipsoid({ kDriverX, 0.98f, 0.18f }, { 0.2f, 0.17f, 0.1f }, 8, 12);
        Rng r(5);
        for (int i = 0; i < 60; i++) mb.M(MAT_GLASS).Box({ r.Range(-0.7f, 0.7f), r.Range(0.99f, 1.02f), r.Range(0.6f, 0.95f) }, { 0.01f, 0.004f, 0.008f });
    }
}

static void BuildWheel(ModelBuilder& mb, Vector3 c, float steerDeg, int side) {
    mb.Push();
    mb.Translate(c);
    mb.RotateY(steerDeg);
    mb.RotateZ(90);
    std::vector<Vector2> tire;
    for (int k = 0; k <= 14; k++) {
        float a = (float)k / 14 * 2 * PI;
        tire.push_back({ 0.235f + cosf(a) * 0.075f * (1.0f + 0.25f * fabsf(cosf(a))), sinf(a) * 0.095f });
    }
    mb.M(MAT_RUBBER).Lathe({ 0, 0, 0 }, tire, 24, false, false);
    mb.M(MAT_STEEL).Cylinder({ 0, -0.06f * side, 0 }, { 0, 0.07f * side, 0 }, 0.18f, 0.16f, 20, true);
    mb.M(MAT_CHROME).Cylinder({ 0, 0.07f * side, 0 }, { 0, 0.08f * side, 0 }, 0.12f, 0.06f, 16, true);
    for (int k = 0; k < 5; k++) {
        float a = k * 2 * PI / 5;
        mb.M(MAT_STEEL).Cylinder({ cosf(a) * 0.07f, 0.075f * side, sinf(a) * 0.07f }, { cosf(a) * 0.07f, 0.085f * side, sinf(a) * 0.07f }, 0.012f, 0.012f, 6, true);
    }
    mb.Pop();
}

static SedanParts g_sedanParts;
const SedanParts& GetSedanParts() {
    if (g_sedanParts.wheel) return g_sedanParts;
    { ModelBuilder mb; BuildWheel(mb, { 0, 0, 0 }, 0, 1); g_sedanParts.wheel = mb.Build(true); }
    { ModelBuilder mb; BuildWheel(mb, { 0, 0, 0 }, 0, -1); g_sedanParts.wheelR = mb.Build(true); }
    {
        // Steering wheel in its own frame: centre at origin, rim in the XY plane, driver at -Z.
        ModelBuilder mb;
        std::vector<Vector2> prof;
        for (int k = 0; k <= 10; k++) { float a = (float)k / 10 * 2 * PI; prof.push_back({ 0.185f + cosf(a) * 0.016f, sinf(a) * 0.016f }); }
        mb.Push(); mb.RotateX(90);
        mb.M(MAT_LEATHER).Lathe({ 0, 0, 0 }, prof, 28, false, false);
        mb.M(MAT_BLACK_PLASTIC).Cylinder({ 0, -0.02f, 0 }, { 0, 0.04f, 0 }, 0.07f, 0.065f, 16, true);
        mb.Pop();
        for (float a : { -90.0f, 30.0f, 150.0f }) {
            float r = a * DEG2RAD;
            mb.M(MAT_BLACK_PLASTIC).Tube({ { cosf(r) * 0.06f, sinf(r) * 0.06f, 0 }, { cosf(r) * 0.175f, sinf(r) * 0.175f, 0 } }, { 0.018f, 0.014f }, 6, true);
        }
        mb.M(MAT_BLACK_PLASTIC).Cylinder({ 0, 0, 0.03f }, { 0, 0, 0.3f }, 0.03f, 0.035f, 10, true);
        g_sedanParts.steering = mb.Build(true);
    }
    {
        ModelBuilder mb;
        mb.M(MAT_BULB_AMBER).Box({ 0, 0.035f, 0 }, { 0.004f, 0.07f, 0.002f });
        g_sedanParts.needle = mb.Build(false);
    }
    {
        // gauge faces: speedometer + tach, backlit
        RenderTexture2D rt = LoadRenderTexture(512, 192);
        BeginTextureMode(rt);
        ClearBackground(BLACK);
        Font f = ui::GetFont(ui::F_MONO_BOLD);
        for (int g = 0; g < 2; g++) {
            Vector2 c{ 128.0f + g * 256.0f, 100.0f };
            DrawCircleLinesV(c, 86, Color{ 255, 170, 60, 255 });
            int ticks = g == 0 ? 12 : 8;
            for (int k = 0; k <= ticks; k++) {
                float a = (210.0f - k * 240.0f / ticks) * DEG2RAD;
                Vector2 p0{ c.x + cosf(a) * 78, c.y - sinf(a) * 78 }, p1{ c.x + cosf(a) * 66, c.y - sinf(a) * 66 };
                DrawLineEx(p0, p1, 3, Color{ 255, 190, 90, 255 });
                const char* lbl = g == 0 ? TextFormat("%d", k * 10) : TextFormat("%d", k);
                Vector2 m = MeasureTextEx(f, lbl, 18, 0);
                DrawTextEx(f, lbl, { c.x + cosf(a) * 50 - m.x / 2, c.y - sinf(a) * 50 - m.y / 2 }, 18, 0, Color{ 255, 190, 100, 255 });
            }
            DrawTextEx(f, g == 0 ? "MPH" : "x1000", { c.x - 20, c.y + 30 }, 16, 0, Color{ 200, 140, 60, 255 });
        }
        EndTextureMode();
        Image img = LoadImageFromTexture(rt.texture);
        UnloadRenderTexture(rt);
        ImageFlipVertical(&img);
        Texture2D t = LoadTextureFromImage(img);
        UnloadImage(img);
        GenTextureMipmaps(&t);
        SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
        int mat = MakeSignMaterial(t, true, 0.9f, { 1.0f, 0.75f, 0.45f });
        ModelBuilder mb;
        mb.M(mat).BoxUV({ 0, 0, 0 }, { 0.34f, 0.128f, 0.002f });
        g_sedanParts.gauges = mb.Build(false);
    }
    return g_sedanParts;
}

static int PaintFor(const Entity& e) {
    std::string c = e.Str("color", "blue");
    if (c == "white") return MAT_CARPAINT_WHITE;
    if (c == "red") return MAT_CARPAINT_RED;
    if (c == "green") return MAT_CARPAINT_GREEN;
    if (c == "beige") return MAT_CARPAINT_BEIGE;
    return MAT_CARPAINT_BLUE;
}

static void BuildSedan(PrefabBuild& b, const Entity& e) {
    bool wrecked = e.Str("state") == "wrecked";
    int paint = PaintFor(e);
    BuildSedanBody(b.mb, paint, wrecked);
    if (wrecked) {
        // The front wrapped around a steel leg: fold the nose back in a V centred on x = 0.25,
        // lift the hood, and crumple everything ahead of the windshield.
        b.mb.Deform([](Vector3 p) {
            if (p.z > 0.6f) {
                float t = SmoothStep(0.6f, 2.4f, p.z);
                float dx = fabsf(p.x - 0.3f);
                float push = t * (1.05f - SmoothStep(0.0f, 0.9f, dx) * 0.55f);
                p.z -= push * 0.95f;
                p.y += t * 0.1f * (1.0f - dx) + GradNoise2(p.x * 9.0f, p.z * 9.0f, 0, 7) * 0.05f * t;
                p.x += GradNoise2(p.y * 7.0f, p.z * 7.0f, 0, 3) * 0.05f * t;
                if (p.y > 0.8f) p.y += t * 0.12f * (1.0f - SmoothStep(0.0f, 0.8f, dx));   // hood buckles up
            }
            return p;
        }, true);
        BuildWheel(b.mb, { 0.76f, 0.31f, 1.0f }, 28, 1);
        b.mb.Push(); b.mb.Translate({ -0.74f, 0.29f, 0.95f }); b.mb.RotateZ(-12); b.mb.Translate({ 0.74f, -0.29f, -0.95f });
        BuildWheel(b.mb, { -0.74f, 0.29f, 0.95f }, -35, -1);
        b.mb.Pop();
        BuildWheel(b.mb, { 0.76f, 0.31f, -1.35f }, 0, 1);
        BuildWheel(b.mb, { -0.76f, 0.31f, -1.35f }, 0, -1);
        // steering wheel fixed in place
        b.mb.Push(); b.mb.Translate({ kDriverX, 0.98f, 0.3f }); b.mb.RotateX(-22);
        b.M(MAT_LEATHER).Lathe({ 0, 0, 0 }, { { 0.17f, -0.016f }, { 0.2f, 0 }, { 0.17f, 0.016f } }, 20, false, false);
        b.mb.Pop();
        b.Interact("The car", { 0.9f, 1.0f, 0.0f }, 2.5f, "wreck");
    }
    b.Collider({ 0, 0.75f, wrecked ? -0.3f : 0.0f }, { 1.8f, 1.3f, wrecked ? 3.9f : 4.7f }, SURF_METAL);
    // headlights are added by the car rig (they need to follow steering / be switched)
}

// ---------------------------------------------------------------------------
// Radio tower: 3-leg lattice, 48 m, blinking aviation beacons
// ---------------------------------------------------------------------------
struct BeaconBeh : Behaviour {
    void Update(Entity& e, float) override {
        float t = (float)GetTime();
        bool on = fmodf(t, 1.6f) < 0.55f;
        for (auto& l : e.lights) if (l.group == "beacon") l.cur = on ? 1.0f : 0.0f;
        e.state["beacon"] = on ? 1.0f : 0.0f;
    }
    void Draw(Entity& e) override {
        static Model3D* lamp = nullptr;
        if (!lamp) { ModelBuilder mb; mb.M(MAT_BULB_RED).Sphere({ 0, 0, 0 }, 0.18f, 6, 10); lamp = mb.Build(false); }
        if (e.state["beacon"] > 0.5f) {
            Rdr().Draw(lamp, MatrixMultiply(MatrixTranslate(0, 48.4f, 0), e.xf), WHITE, false);
            Rdr().Draw(lamp, MatrixMultiply(MatrixTranslate(0.5f, 24.0f, 0.3f), e.xf), WHITE, false);
        }
    }
};

static void BuildTower(PrefabBuild& b, const Entity& e) {
    const float H = 48.0f;
    const float r0 = 3.2f, r1 = 0.55f;
    Vector3 legs0[3], legs1[3];
    for (int i = 0; i < 3; i++) {
        float a = i * 2 * PI / 3 + 0.3f;
        legs0[i] = { cosf(a) * r0, 0, sinf(a) * r0 };
        legs1[i] = { cosf(a) * r1, H, sinf(a) * r1 };
        b.M(MAT_STEEL).Tube({ legs0[i], legs1[i] }, { 0.11f, 0.05f }, 8, true);
        b.M(MAT_CONCRETE).Box({ legs0[i].x, 0.2f, legs0[i].z }, { 1.0f, 0.6f, 1.0f }, 0.04f);
        b.Collider({ legs0[i].x, 0.9f, legs0[i].z }, { 0.9f, 1.8f, 0.9f }, SURF_METAL);
    }
    // X bracing between each pair of legs every level
    int levels = 16;
    for (int k = 0; k < levels; k++) {
        float t0 = (float)k / levels, t1 = (float)(k + 1) / levels;
        for (int i = 0; i < 3; i++) {
            int j = (i + 1) % 3;
            Vector3 a0 = Vector3Lerp(legs0[i], legs1[i], t0), a1 = Vector3Lerp(legs0[i], legs1[i], t1);
            Vector3 b0 = Vector3Lerp(legs0[j], legs1[j], t0), b1 = Vector3Lerp(legs0[j], legs1[j], t1);
            float rr = Lerp(0.04f, 0.02f, t0);
            b.M(MAT_STEEL).Tube({ a0, b1 }, { rr, rr }, 5, false);
            b.M(MAT_STEEL).Tube({ b0, a1 }, { rr, rr }, 5, false);
            b.M(MAT_STEEL).Tube({ a1, b1 }, { rr, rr }, 5, false);
        }
    }
    // platform, dishes, whip antennas, cable ladder, beacons
    b.M(MAT_RUST).Cylinder({ 0, 30.0f, 0 }, { 0, 30.08f, 0 }, 1.5f, 1.5f, 3, true);
    for (int i = 0; i < 3; i++) {
        float a = i * 2.1f + 0.8f;
        b.mb.Push(); b.mb.Translate({ cosf(a) * 1.2f, 32.0f + i, sinf(a) * 1.2f }); b.mb.RotateY(-a * RAD2DEG + 90); b.mb.RotateX(90);
        b.M(MAT_PAINT_WHITE).Lathe({ 0, 0, 0 }, { { 0.0f, 0.0f }, { 0.35f, 0.08f }, { 0.55f, 0.2f }, { 0.56f, 0.22f } }, 18, false, false);
        b.mb.Pop();
    }
    b.M(MAT_STEEL).Cylinder({ 0, H, 0 }, { 0, H + 5.0f, 0 }, 0.08f, 0.03f, 8, true);
    b.M(MAT_STEEL).Tube({ legs0[0], legs1[0] }, { 0.03f, 0.03f }, 4, false);
    b.M(MAT_RUST).Box({ 0.4f, 1.1f, 0.4f }, { 1.2f, 1.6f, 0.8f }, 0.03f);   // equipment shelter
    b.Collider({ 0.4f, 1.1f, 0.4f }, { 1.2f, 1.6f, 0.8f }, SURF_METAL);
    int warn = SignMaterial("rf_warn", "DANGER\nHIGH RF ENERGY\nKEEP OUT", ui::F_MONO_BOLD, 40, Color{ 20, 20, 20, 255 }, Color{ 230, 200, 40, 255 }, 320, 200, 0.7f);
    b.M(warn).BoxUV({ 0.4f, 1.3f, 0.81f }, { 0.5f, 0.32f, 0.004f });
    LightDef& l1 = b.AddLight({ 0, H + 0.4f, 0 }, { 1.0f, 0.05f, 0.03f }, 60.0f, 60.0f, 0.8f);
    l1.group = "beacon";
    LightDef& l2 = b.AddLight({ 0.5f, 24.0f, 0.3f }, { 1.0f, 0.05f, 0.03f }, 20.0f, 30.0f, 0.5f);
    l2.group = "beacon";
    (void)e;
}

static void TowerTerrain(const Entity& e, std::vector<TerrainPad>& pads, std::vector<Rectangle>&) {
    TerrainPad p; p.c = { e.pos.x, e.pos.z }; p.half = { 4.0f, 4.0f }; p.margin = 5.0f; p.splat = 2;
    pads.push_back(p);
}

// Scattered debris pieces around the wreck
static void BuildDebris(PrefabBuild& b, const Entity& e) {
    Rng r((uint32_t)e.Num("variant", 0) * 13 + 1);
    int paint = PaintFor(e);
    for (int i = 0; i < 14; i++) {
        Vector3 p{ r.Range(-3, 3), 0.02f, r.Range(-3, 3) };
        float s = r.Range(0.05f, 0.25f);
        int m = r.Chance(0.4f) ? paint : (r.Chance(0.5f) ? MAT_GREY_PLASTIC : MAT_BLACK_PLASTIC);
        b.M(m).BoxRot(p, { s, 0.01f + s * 0.1f, s * r.Range(0.4f, 1.2f) }, { r.Range(-10, 10), r.Range(0, 360), r.Range(-10, 10) }, 0.004f);
    }
    for (int i = 0; i < 120; i++) {   // glass crumbs catch the light
        Vector3 p{ r.Range(-3.5f, 3.5f), 0.01f, r.Range(-3.5f, 3.5f) };
        b.M(MAT_GLASS).Box(p, { 0.012f, 0.004f, 0.01f });
    }
    if (e.Num("bumper", 0) > 0) b.M(MAT_GREY_PLASTIC).BoxRot({ 0.5f, 0.1f, 0.3f }, { 1.5f, 0.2f, 0.15f }, { 0, 25, 10 }, 0.05f);
    b.castShadow = false;
}

void RegisterCrashPrefabs() {
    {
        PrefabInfo p; p.name = "sedan"; p.category = "vehicles"; p.build = BuildSedan; p.geometryKeys = { "state", "color" };
        RegisterPrefab(p);
    }
    {
        PrefabInfo p; p.name = "radio_tower"; p.category = "crash"; p.build = BuildTower; p.terrain = TowerTerrain;
        p.behaviour = [](Entity&) { return std::unique_ptr<Behaviour>(new BeaconBeh()); };
        RegisterPrefab(p);
    }
    {
        PrefabInfo p; p.name = "debris"; p.category = "crash"; p.build = BuildDebris; p.geometryKeys = { "variant", "color", "bumper" };
        RegisterPrefab(p);
    }
}
