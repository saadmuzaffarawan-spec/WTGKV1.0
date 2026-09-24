// Blackwood College: the building shell, its interior fixtures and the basement.
//
// Local frame: front entrance faces +Z (z = +12); x spans -22..22. Ground floor top at
// y = 0.3, basement floor at y = -3.0 under the south-east corner (x 0..21, z -11..-2).
#include "prefab_util.h"

static const float GF = 0.3f;       // ground floor level
static const float BF = -3.0f;      // basement floor level
static const float H1 = 3.4f;       // storey height

static void Slab(PrefabBuild& b, int mat, float x0, float x1, float z0, float z1, float y, float t, int surf) {
    Vector3 c{ (x0 + x1) * 0.5f, y - t * 0.5f, (z0 + z1) * 0.5f };
    Vector3 s{ x1 - x0, t, z1 - z0 };
    b.M(mat).Box(c, s);
    b.Collider(c, s, surf);
}

static void BuildCollege(PrefabBuild& b, const Entity& e) {
    const float T = 0.3f, TI = 0.15f;
    // ---- ground floor slab (with the stairwell opening at x 14.6..20.6, z -9.4..-7.4)
    Slab(b, MAT_WOOD_FLOOR, -22, 22, -1.5f, 12, GF, 0.3f, SURF_WOOD);
    Slab(b, MAT_WOOD_FLOOR, -22, 0, -12, -1.5f, GF, 0.3f, SURF_WOOD);
    Slab(b, MAT_WOOD_FLOOR, 0, 22, -7.4f, -1.5f, GF, 0.3f, SURF_WOOD);
    Slab(b, MAT_WOOD_FLOOR, 0, 22, -12, -9.4f, GF, 0.3f, SURF_WOOD);
    Slab(b, MAT_WOOD_FLOOR, 0, 14.6f, -9.4f, -7.4f, GF, 0.3f, SURF_WOOD);
    Slab(b, MAT_WOOD_FLOOR, 20.6f, 22, -9.4f, -7.4f, GF, 0.3f, SURF_WOOD);
    // corridor runner of cracked linoleum
    static int linoleum = -1;
    if (linoleum < 0) {
        SurfaceMat m = Mat(MAT_TILE);
        m.name = "linoleum_old"; m.tint = Color{ 128, 122, 106, 255 }; m.rough = 1.3f;
        linoleum = AddMaterial(m);
    }
    b.M(linoleum).Box({ 0, GF + 0.003f, 0 }, { 43.4f, 0.006f, 2.9f });
    // what fell from the ceiling: plaster chunks, loose paper, a toppled ceiling panel
    {
        Rng d(77);
        for (int i = 0; i < 60; i++) {
            float x = d.Range(-21, 21), z = d.Range(-11.5f, 11.5f);
            if (x > 13.8f && x < 21.4f && z > -10.0f && z < -6.8f) continue;   // keep the stairwell clear
            if (d.Chance(0.5f)) {
                float sz = d.Range(0.05f, 0.18f);
                b.M(MAT_PLASTER_DIRTY).BoxRot({ x, GF + sz * 0.3f, z }, { sz, sz * 0.5f, sz * 0.8f }, { d.Range(-20, 20), d.Range(0, 180), d.Range(-20, 20) }, 0.01f);
            } else {
                b.M(MAT_PAPER).BoxRot({ x, GF + 0.004f, z }, { 0.21f, 0.002f, 0.3f }, { d.Range(-3, 3), d.Range(0, 180), d.Range(-3, 3) });
            }
        }
        for (int i = 0; i < 5; i++) {
            float x = -18.0f + i * 9.0f + d.Range(-2, 2), z = d.Range(-1.0f, 1.0f);
            b.M(MAT_PLASTER).BoxRot({ x, GF + 0.2f, z }, { 1.2f, 0.02f, 0.6f }, { d.Range(-5, 5), d.Range(0, 180), d.Range(12, 25) });
            b.M(MAT_BLACK_PLASTIC).Box({ x + 0.3f, GF + H1 - 0.256f, z }, { 1.2f, 0.004f, 0.6f });   // the hole it left
        }
    }
    // ---- exterior walls, two storeys, most windows boarded
    auto windows = [](float len, float start, float step) {
        std::vector<Opening> o;
        for (float a = start; a < len - 1.0f; a += step) o.push_back({ a, 1.4f, 1.0f, 2.5f });
        return o;
    };
    std::vector<Opening> frontG = windows(44, 3.0f, 4.0f);
    frontG.erase(std::remove_if(frontG.begin(), frontG.end(), [](const Opening& o) { return fabsf(o.at - 22.0f) < 3.0f; }), frontG.end());
    frontG.push_back({ 22.0f, 2.4f, 0.0f, 2.6f });   // main doors
    BuildWall(b, { -22, 12 }, { 22, 12 }, GF, H1, T, MAT_BRICK, MAT_PLASTER_DIRTY, frontG, SURF_CONCRETE, 0.12f);
    BuildWall(b, { 22, 12 }, { 22, -12 }, GF, H1, T, MAT_BRICK, MAT_PLASTER_DIRTY, windows(24, 3.0f, 4.5f), SURF_CONCRETE, 0.12f);
    BuildWall(b, { 22, -12 }, { -22, -12 }, GF, H1, T, MAT_BRICK, MAT_PLASTER_DIRTY, windows(44, 3.0f, 4.0f), SURF_CONCRETE, 0.12f);
    BuildWall(b, { -22, -12 }, { -22, 12 }, GF, H1, T, MAT_BRICK, MAT_PLASTER_DIRTY, windows(24, 3.0f, 4.5f), SURF_CONCRETE, 0.12f);
    Rng rng(31);
    auto fill = [&](Vector2 a, Vector2 c, const std::vector<Opening>& ops, bool upper) {
        for (const Opening& o : ops) {
            if (o.sill <= 0.0f) continue;
            int roll = rng.RangeI(0, 9);
            BuildWindow(b, a, c, upper ? GF + H1 : GF, o, MAT_GLASS_DIRTY, MAT_WOOD_DARK, 2, roll < 3, roll >= 3 && roll < 8);
        }
    };
    fill({ -22, 12 }, { 22, 12 }, frontG, false);
    fill({ 22, 12 }, { 22, -12 }, windows(24, 3.0f, 4.5f), false);
    fill({ 22, -12 }, { -22, -12 }, windows(44, 3.0f, 4.0f), false);
    fill({ -22, -12 }, { -22, 12 }, windows(24, 3.0f, 4.5f), false);
    // second storey (outside only: the stairs up collapsed long ago)
    BuildWall(b, { -22, 12 }, { 22, 12 }, GF + H1, H1, T, MAT_BRICK, MAT_PLASTER_DIRTY, windows(44, 3.0f, 4.0f), SURF_CONCRETE);
    BuildWall(b, { 22, 12 }, { 22, -12 }, GF + H1, H1, T, MAT_BRICK, MAT_PLASTER_DIRTY, windows(24, 3.0f, 4.5f), SURF_CONCRETE);
    BuildWall(b, { 22, -12 }, { -22, -12 }, GF + H1, H1, T, MAT_BRICK, MAT_PLASTER_DIRTY, windows(44, 3.0f, 4.0f), SURF_CONCRETE);
    BuildWall(b, { -22, -12 }, { -22, 12 }, GF + H1, H1, T, MAT_BRICK, MAT_PLASTER_DIRTY, windows(24, 3.0f, 4.5f), SURF_CONCRETE);
    fill({ -22, 12 }, { 22, 12 }, windows(44, 3.0f, 4.0f), true);
    fill({ 22, -12 }, { -22, -12 }, windows(44, 3.0f, 4.0f), true);
    Slab(b, MAT_PLASTER_DIRTY, -22, 22, -12, 12, GF + H1, 0.25f, SURF_WOOD);                  // ground floor ceiling
    b.M(MAT_CONCRETE_DARK).Box({ 0, GF + 2 * H1 + 0.2f, 0 }, { 44.8f, 0.4f, 24.8f }, 0.03f);   // roof
    b.M(MAT_CONCRETE).Box({ 0, GF + 2 * H1 + 0.55f, 12.3f }, { 45.0f, 0.3f, 0.4f }, 0.03f);    // cornice
    b.M(MAT_CONCRETE).Box({ 0, GF + H1 - 0.05f, 12.2f }, { 44.8f, 0.2f, 0.25f }, 0.02f);       // string course
    b.M(MAT_CONCRETE).Box({ 0, GF + 0.2f, 12.25f }, { 44.8f, 0.5f, 0.2f }, 0.02f);             // plinth
    // portico: steps, columns, pediment and the name
    for (int k = 0; k < 4; k++) {
        Vector3 c{ 0, GF - 0.15f - k * 0.17f + 0.0f, 13.0f + k * 0.35f };
        b.Solid(MAT_CONCRETE, { c.x, c.y - 0.05f, c.z }, { 8.0f + k * 0.6f, 0.2f + k * 0.34f, 0.36f }, 0.02f, SURF_CONCRETE);
    }
    for (int i = -2; i <= 2; i++) {
        if (i == 0) continue;
        float x = i * 1.7f;
        b.M(MAT_PAINT_CREAM).Cylinder({ x, GF, 13.1f }, { x, GF + 4.2f, 13.1f }, 0.28f, 0.24f, 16, true);
        b.M(MAT_CONCRETE).Box({ x, GF + 0.1f, 13.1f }, { 0.7f, 0.2f, 0.7f }, 0.02f);
        b.M(MAT_CONCRETE).Box({ x, GF + 4.25f, 13.1f }, { 0.65f, 0.15f, 0.65f }, 0.02f);
        b.ColliderCyl({ x, GF, 13.1f }, 0.28f, 4.2f, SURF_CONCRETE);
    }
    b.M(MAT_CONCRETE).Box({ 0, GF + 4.5f, 13.1f }, { 8.2f, 0.4f, 1.2f }, 0.03f);
    int name = SignMaterial("college_name", "BLACKWOOD  COLLEGE", ui::F_MONO_BOLD, 64, Color{ 60, 55, 48, 255 }, Color{ 170, 162, 145, 255 }, 1024, 96, 0.9f);
    b.M(name).BoxUV({ 0, GF + 4.5f, 13.71f }, { 6.5f, 0.32f, 0.004f });
    // ---- interior walls
    std::vector<Opening> corrN = { { 7.0f, 1.0f, 0.0f, 2.2f }, { 22.0f, 3.0f, 0.0f, 2.6f }, { 30.0f, 1.0f, 0.0f, 2.2f }, { 38.0f, 1.0f, 0.0f, 2.2f } };
    BuildWall(b, { -22, 1.5f }, { 22, 1.5f }, GF, H1, TI, MAT_PLASTER_DIRTY, MAT_PLASTER_DIRTY, corrN, SURF_CONCRETE, 0.1f);
    std::vector<Opening> corrS = { { 8.0f, 1.0f, 0.0f, 2.2f }, { 22.0f, 1.0f, 0.0f, 2.2f }, { 38.0f, 1.2f, 0.0f, 2.2f } };
    BuildWall(b, { 22, -1.5f }, { -22, -1.5f }, GF, H1, TI, MAT_PLASTER_DIRTY, MAT_PLASTER_DIRTY, corrS, SURF_CONCRETE, 0.1f);
    // north side partitions: classroom A | lobby | office | staff room
    BuildWall(b, { -5, 1.5f }, { -5, 12 }, GF, H1, TI, MAT_PLASTER, MAT_PLASTER, {}, SURF_CONCRETE, 0.1f);
    BuildWall(b, { 5, 12 }, { 5, 1.5f }, GF, H1, TI, MAT_PLASTER, MAT_PLASTER, {}, SURF_CONCRETE, 0.1f);
    BuildWall(b, { 13, 12 }, { 13, 1.5f }, GF, H1, TI, MAT_PLASTER, MAT_PLASTER, {}, SURF_CONCRETE, 0.1f);
    // south side partitions: classroom B | classroom C | stair hall
    BuildWall(b, { -7, -12 }, { -7, -1.5f }, GF, H1, TI, MAT_PLASTER, MAT_PLASTER, {}, SURF_CONCRETE, 0.1f);
    BuildWall(b, { 7, -1.5f }, { 7, -12 }, GF, H1, TI, MAT_PLASTER, MAT_PLASTER, {}, SURF_CONCRETE, 0.1f);
    // collapsed staircase to the upper floor (blocked)
    for (int k = 0; k < 9; k++)
        b.Solid(MAT_WOOD_DARK, { 9.0f + k * 0.28f, GF + 0.09f + k * 0.18f, -10.9f }, { 0.28f, 0.18f + k * 0.36f, 1.6f }, 0.01f, SURF_WOOD);
    for (int k = 0; k < 6; k++)
        b.M(MAT_WOOD).BoxRot({ 10.5f + rng.Range(-1, 1), GF + 0.3f + rng.Range(0, 1.2f), -9.6f + rng.Range(-0.6f, 0.6f) }, { 2.2f, 0.12f, 0.25f }, { rng.Range(-30, 30), rng.Range(0, 180), rng.Range(-40, 40) }, 0.01f);
    b.Collider({ 10.5f, GF + 1.0f, -9.6f }, { 3.0f, 2.0f, 2.0f }, SURF_WOOD);
    // ---- stairs down to the basement (x 20.6 -> 14.6 descending, z -9.4..-7.4)
    const int steps = 18;
    for (int k = 0; k < steps; k++) {
        float x = 20.3f - k * 0.33f;
        float top = GF - (k + 1) * ((GF - BF) / steps);
        b.Solid(MAT_CONCRETE_DARK, { x, top - 0.12f, -8.4f }, { 0.34f, 0.24f, 2.0f }, 0.01f, SURF_CONCRETE);
        b.M(MAT_STEEL).Box({ x + 0.15f, top + 0.005f, -8.4f }, { 0.04f, 0.01f, 2.0f });   // nosing
    }
    b.M(MAT_STEEL).Tube({ { 20.6f, GF + 0.9f, -7.45f }, { 14.6f, BF + 0.9f, -7.45f } }, { 0.025f, 0.025f }, 6, true);
    b.Collider({ 17.6f, GF - 1.0f, -9.55f }, { 6.2f, 4.0f, 0.2f }, SURF_CONCRETE);
    b.Collider({ 17.6f, GF - 1.0f, -7.25f }, { 6.2f, 4.0f, 0.2f }, SURF_CONCRETE, 0, true, true);
    // ---- basement: hall (x 12..21, z -6..-2) reached by the stairs, boiler room (5..12), storage (12..21, -11..-6),
    // hidden room (0..5, -11..-4)
    Slab(b, MAT_CONCRETE_DARK, 0, 21, -11, -2, BF, 0.3f, SURF_CONCRETE);
    Slab(b, MAT_CONCRETE_DARK, 12, 21, -9.4f, -7.4f, BF, 0.3f, SURF_CONCRETE);
    BuildWall(b, { 21, -2 }, { 21, -11 }, BF, GF - BF, 0.3f, MAT_CONCRETE_DARK, MAT_CONCRETE_DARK, {}, SURF_CONCRETE);
    BuildWall(b, { 21, -11 }, { 0, -11 }, BF, GF - BF, 0.3f, MAT_CONCRETE_DARK, MAT_CONCRETE_DARK, {}, SURF_CONCRETE);
    BuildWall(b, { 0, -11 }, { 0, -2 }, BF, GF - BF, 0.3f, MAT_CONCRETE_DARK, MAT_CONCRETE_DARK, { { 3.5f, 1.3f, 0.0f, 2.3f } }, SURF_CONCRETE);   // iron door opening
    BuildWall(b, { 0, -2 }, { 21, -2 }, BF, GF - BF, 0.3f, MAT_CONCRETE_DARK, MAT_CONCRETE_DARK, {}, SURF_CONCRETE);
    // interior basement walls (stairs land in the hall between x 14.6 and 20.6 at z -9.4..-7.4, so the hall is bigger)
    BuildWall(b, { 12, -2 }, { 12, -11 }, BF, GF - BF, TI, MAT_PLASTER_DIRTY, MAT_PLASTER_DIRTY, { { 2.5f, 1.0f, 0.0f, 2.1f } }, SURF_CONCRETE);
    BuildWall(b, { 5, -11 }, { 5, -2 }, BF, GF - BF, 0.25f, MAT_BRICK_DARK, MAT_BRICK_DARK, { { 3.2f, 1.1f, 0.0f, 2.1f } }, SURF_CONCRETE);   // behind the bookshelf
    BuildWall(b, { 0, -4 }, { 5, -4 }, BF, GF - BF, TI, MAT_BRICK_DARK, MAT_BRICK_DARK, {}, SURF_CONCRETE);
    // pipes along the ceiling, a dripping one
    for (float z : { -2.4f, -2.8f }) b.M(MAT_RUST).Tube({ { 21, GF - 0.35f, z }, { 5, GF - 0.35f, z }, { 5, GF - 0.35f, -10.5f } }, { 0.06f, 0.06f, 0.06f }, 8, true);
    // basement lights (bare bulbs on the "basement" group; the boiler has its own glow)
    b.M(MAT_BULB_WARM).Sphere({ 16.5f, GF - 0.45f, -4.0f }, 0.05f, 6, 8);
    b.AddLight({ 16.5f, GF - 0.55f, -4.0f }, { 1.0f, 0.7f, 0.4f }, 1.8f, 7.0f, 0.08f).group = "basement";
    // chalkboards in classrooms, portraits in the corridor
    int chalk = SignMaterial("chalk", "THE GROUND KEEPS\nTHE GROUND KEEPS\nTHE GROUND KEEPS\nTHE GROUND KEEPS", ui::F_MONO, 44, Color{ 210, 210, 200, 255 }, Color{ 28, 38, 32, 255 }, 1024, 400, 0.6f);
    b.M(chalk).BoxUV({ -13.0f, GF + 1.6f, 1.42f }, { 4.0f, 1.3f, 0.02f });
    int chalk2 = SignMaterial("chalk2", "Lesson 7:\nWhat is given to the ground\nis kept by the ground.", ui::F_MONO, 40, Color{ 210, 210, 200, 255 }, Color{ 28, 38, 32, 255 }, 1024, 400, 0.5f);
    b.mb.Push(); b.mb.RotateY(180);
    b.M(chalk2).BoxUV({ 14.0f, GF + 1.6f, 11.84f }, { 4.0f, 1.3f, 0.02f });
    b.mb.Pop();
    for (int i = 0; i < 5; i++) {
        float x = -18.0f + i * 8.0f;
        b.M(MAT_WOOD_DARK).Box({ x, GF + 1.8f, 1.33f }, { 0.6f, 0.8f, 0.04f }, 0.01f);
        b.M(MAT_BLACK_PLASTIC).Box({ x, GF + 1.8f, 1.3f }, { 0.5f, 0.7f, 0.005f });
    }
    (void)e;
}

static void CollegeTerrain(const Entity& e, std::vector<TerrainPad>& pads, std::vector<Rectangle>& holes) {
    TerrainPad p; p.c = { e.pos.x, e.pos.z }; p.half = { 23.0f, 15.0f }; p.yaw = e.rot.x * DEG2RAD; p.margin = 5.0f; p.splat = 1;
    pads.push_back(p);
    // hole over the basement so the stairwell descends through the ground
    Matrix m = MatPose({ e.pos.x, 0, e.pos.z }, e.rot.x * DEG2RAD);
    Vector3 a = Vector3Transform({ -0.5f, 0, -11.5f }, m), c = Vector3Transform({ 21.5f, 0, -1.8f }, m);
    holes.push_back({ fminf(a.x, c.x), fminf(a.z, c.z), fabsf(c.x - a.x), fabsf(c.z - a.z) });
}

// ---------------------------------------------------------------------------
// Furniture
// ---------------------------------------------------------------------------
static void BuildClassroom(PrefabBuild& b, const Entity& e) {
    Rng r((uint32_t)e.Num("variant", 0) * 7 + 3);
    int rows = (int)e.Num("rows", 4), cols = (int)e.Num("cols", 4);
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++) {
            float x = (j - (cols - 1) * 0.5f) * 1.6f + r.Range(-0.15f, 0.15f), z = (i - (rows - 1) * 0.5f) * 1.5f + r.Range(-0.15f, 0.15f);
            float yaw = r.Range(-12, 12);
            bool tipped = r.Chance(0.15f);
            b.mb.Push(); b.mb.Translate({ x, 0, z }); b.mb.RotateY(yaw);
            if (tipped) { b.mb.Translate({ 0, 0.36f, 0 }); b.mb.RotateZ(88); b.mb.Translate({ 0, -0.36f, 0 }); }
            b.M(MAT_WOOD).Box({ 0, 0.72f, 0 }, { 0.9f, 0.04f, 0.55f }, 0.008f);
            for (int s = -1; s <= 1; s += 2) for (int k = -1; k <= 1; k += 2)
                b.M(MAT_STEEL).Cylinder({ s * 0.4f, 0, k * 0.22f }, { s * 0.4f, 0.7f, k * 0.22f }, 0.015f, 0.015f, 6, false);
            // chair
            b.M(MAT_WOOD).Box({ 0, 0.44f, -0.55f }, { 0.42f, 0.03f, 0.4f }, 0.006f);
            b.M(MAT_WOOD).Box({ 0, 0.72f, -0.76f }, { 0.42f, 0.3f, 0.03f }, 0.006f);
            for (int s = -1; s <= 1; s += 2) for (int k = -1; k <= 1; k += 2)
                b.M(MAT_STEEL).Cylinder({ s * 0.18f, 0, -0.55f + k * 0.17f }, { s * 0.18f, 0.43f, -0.55f + k * 0.17f }, 0.012f, 0.012f, 6, false);
            if (r.Chance(0.3f)) b.M(MAT_PAPER).BoxRot({ r.Range(-0.3f, 0.3f), 0.745f, r.Range(-0.15f, 0.15f) }, { 0.21f, 0.002f, 0.3f }, { 0, r.Range(0, 90), 0 });
            b.mb.Pop();
            if (!tipped) b.Collider({ x, 0.4f, z - 0.2f }, { 0.95f, 0.8f, 1.0f }, SURF_WOOD, yaw);
        }
    // teacher's desk
    b.Solid(MAT_WOOD_DARK, { 0, 0.4f, (rows * 0.5f + 1.2f) * 1.5f * 0.7f }, { 1.6f, 0.8f, 0.8f }, 0.02f, SURF_WOOD);
}

static void BuildLockers(PrefabBuild& b, const Entity& e) {
    int n = (int)e.Num("count", 8);
    Rng r(n * 5u + 1u);
    for (int i = 0; i < n; i++) {
        float x = (i - (n - 1) * 0.5f) * 0.4f;
        Color tint = r.Chance(0.5f) ? Color{ 70, 100, 90, 255 } : Color{ 90, 110, 100, 255 };
        b.mb.SetColor(tint);
        b.M(MAT_PAINT_GREEN).Box({ x, 0.95f, 0 }, { 0.38f, 1.9f, 0.45f }, 0.01f);
        b.mb.SetColor(WHITE);
        bool open = r.Chance(0.2f);
        if (open) b.M(MAT_PAINT_GREEN).BoxRot({ x + 0.19f, 0.95f, 0.4f }, { 0.02f, 1.7f, 0.36f }, { 0, 70, 0 }, 0.004f);
        for (int k = 0; k < 4; k++) b.M(MAT_BLACK_PLASTIC).Box({ x, 1.6f + k * 0.035f, 0.226f }, { 0.22f, 0.012f, 0.004f });
        b.M(MAT_CHROME).Box({ x + 0.12f, 1.0f, 0.23f }, { 0.03f, 0.1f, 0.02f });
    }
    b.Collider({ 0, 0.95f, 0 }, { n * 0.4f, 1.9f, 0.46f }, SURF_METAL);
    (void)e;
}

static void BuildOfficeDesk(PrefabBuild& b, const Entity& e) {
    b.Solid(MAT_WOOD_DARK, { 0, 0.4f, 0 }, { 1.8f, 0.8f, 0.9f }, 0.02f, SURF_WOOD);
    b.M(MAT_LEATHER).Box({ 0, 0.5f, -0.8f }, { 0.6f, 0.1f, 0.6f }, 0.04f);
    b.M(MAT_LEATHER).Box({ 0, 0.95f, -1.08f }, { 0.6f, 0.8f, 0.1f }, 0.04f);
    b.M(MAT_BRICK_DARK).Box({ 0.6f, 0.83f, 0.1f }, { 0.25f, 0.05f, 0.3f }, 0.01f);   // stack of files
    b.M(MAT_PAPER).BoxRot({ -0.3f, 0.81f, 0.0f }, { 0.3f, 0.01f, 0.21f }, { 0, 12, 0 });
    b.M(MAT_BRICK_DARK).Cylinder({ -0.7f, 0.8f, 0.25f }, { -0.7f, 0.9f, 0.25f }, 0.04f, 0.045f, 10, true);
    (void)e;
}

static void BuildLedger(PrefabBuild& b, const Entity&) {
    b.M(MAT_LEATHER).Box({ 0, 0.02f, 0 }, { 0.26f, 0.04f, 0.34f }, 0.01f);
    b.M(MAT_PAPER).Box({ 0.01f, 0.02f, 0 }, { 0.25f, 0.032f, 0.33f });
    b.Interact("Ledger", { 0, 0.05f, 0 }, 1.8f, "ledger");
}

static void BuildBookshelf(PrefabBuild& b, const Entity& e) {
    Rng r(9);
    const float W = 1.3f, H = 2.2f, D = 0.35f;
    b.M(MAT_WOOD_DARK).Box({ 0, H * 0.5f, 0 }, { W, H, 0.03f });
    for (int s = -1; s <= 1; s += 2) b.M(MAT_WOOD_DARK).Box({ s * W * 0.5f, H * 0.5f, D * 0.5f }, { 0.03f, H, D });
    for (int k = 0; k < 6; k++) {
        float y = 0.05f + k * 0.42f;
        b.M(MAT_WOOD_DARK).Box({ 0, y, D * 0.5f }, { W, 0.025f, D });
        float x = -W * 0.5f + 0.05f;
        while (x < W * 0.45f && k < 5) {
            float w = r.Range(0.025f, 0.06f), h = r.Range(0.22f, 0.36f);
            Color c{ (unsigned char)r.RangeI(40, 110), (unsigned char)r.RangeI(25, 60), (unsigned char)r.RangeI(20, 45), 255 };
            b.mb.SetColor(c);
            b.M(MAT_LEATHER).BoxRot({ x + w * 0.5f, y + h * 0.5f + 0.015f, D * 0.5f }, { w, h, 0.22f }, { 0, 0, r.Chance(0.1f) ? 15.0f : 0.0f }, 0.004f);
            b.mb.SetColor(WHITE);
            x += w + 0.004f;
        }
    }
    b.Collider({ 0, H * 0.5f, D * 0.5f }, { W, H, D }, SURF_WOOD);
    b.Interact("Bookshelf", { 0, 1.2f, D }, 2.0f, "bookshelf");
    (void)e;
}

// The bookshelf over the hidden doorway slides along its wall when pushed.
struct SlideBeh : Behaviour {
    float slide = 0;
    void Update(Entity& e, float dt) override {
        float target = e.state["open"] > 0.5f ? 1.25f : 0.0f;
        float prev = slide;
        slide = Damp(slide, target, 1.6f, dt);
        if (fabsf(slide - prev) > 0.0005f) {
            Matrix m = MatrixMultiply(MatrixTranslate(slide, 0, 0), MatPose(e.base, e.worldYaw));
            e.xf = m;
            if (!e.boxIds.empty()) Phys().UpdateBox(e.boxIds[0], Vector3Transform({ 0, 1.1f, 0.175f }, m), e.worldYaw);
        }
    }
};

static void BuildBoiler(PrefabBuild& b, const Entity& e) {
    b.M(MAT_RUST).Cylinder({ -1.3f, 1.0f, 0 }, { 1.3f, 1.0f, 0 }, 0.85f, 0.85f, 24, true);
    for (float x : { -0.8f, 0.9f }) {   // brick saddles it rests on
        b.M(MAT_BRICK_DARK).Box({ x, 0.2f, 0 }, { 0.4f, 0.4f, 1.3f }, 0.01f);
        b.M(MAT_BRICK_DARK).Box({ x, 0.33f, 0 }, { 0.4f, 0.14f, 0.9f }, 0.01f);
    }

    b.M(MAT_RUST).Box({ -1.0f, 0.3f, 0 }, { 0.6f, 0.6f, 1.2f }, 0.03f);
    b.M(MAT_PAINT_BLACK).Box({ -1.32f, 0.6f, 0 }, { 0.05f, 0.4f, 0.5f }, 0.01f);   // burner hatch
    b.M(MAT_STEEL).Cylinder({ 0.0f, 1.7f, 0 }, { 0.0f, 3.4f, 0 }, 0.18f, 0.18f, 12, false);
    for (int i = 0; i < 3; i++) b.M(MAT_CHROME).Cylinder({ -0.5f + i * 0.5f, 1.72f, 0.5f }, { -0.5f + i * 0.5f, 1.75f, 0.5f }, 0.08f, 0.08f, 12, true);
    b.Collider({ 0, 0.9f, 0 }, { 2.8f, 1.8f, 1.8f }, SURF_METAL);
    b.Interact("Boiler", { -1.35f, 0.6f, 0 }, 2.0f, "boiler");
    LightDef& l = b.AddLight({ -1.7f, 0.55f, 0 }, { 1.0f, 0.42f, 0.12f }, 2.6f, 6.0f, 0.15f);
    l.group = "boiler_flame";
    l.flicker = 0.25f;
    l.on = false;   // cold until someone relights the pilot
    (void)e;
}

static void BuildOldBed(PrefabBuild& b, const Entity& e) {
    b.M(MAT_RUST).Box({ 0, 0.3f, 0 }, { 0.95f, 0.06f, 2.0f }, 0.01f);
    for (int s = -1; s <= 1; s += 2) for (int k = -1; k <= 1; k += 2) b.M(MAT_RUST).Cylinder({ s * 0.45f, 0, k * 0.95f }, { s * 0.45f, k > 0 ? 0.9f : 0.6f, k * 0.95f }, 0.02f, 0.02f, 6, true);
    b.M(MAT_CLOTH_WHITE).Box({ 0, 0.4f, 0 }, { 0.9f, 0.14f, 1.95f }, 0.05f);
    b.M(MAT_BLOOD_DRY).Box({ 0.1f, 0.472f, -0.2f }, { 0.5f, 0.005f, 0.6f });
    b.M(MAT_CLOTH_GREY).Ellipsoid({ 0, 0.5f, 0.8f }, { 0.3f, 0.08f, 0.18f }, 6, 10);
    // chains from the frame
    std::vector<Vector3> ch;
    for (int i = 0; i <= 12; i++) ch.push_back({ 0.46f, 0.35f - sinf(i / 12.0f * PI) * 0.2f, -0.4f + i * 0.06f });
    b.M(MAT_STEEL).Tube(ch, std::vector<float>(ch.size(), 0.012f), 5, false);
    b.Collider({ 0, 0.3f, 0 }, { 1.0f, 0.6f, 2.05f }, SURF_METAL);
    (void)e;
}

static void BuildCandle(PrefabBuild& b, const Entity& e) {
    Rng r((uint32_t)e.Num("variant", 0) + 1);
    int n = r.RangeI(2, 5);
    for (int i = 0; i < n; i++) {
        Vector3 p{ r.Range(-0.1f, 0.1f), 0, r.Range(-0.1f, 0.1f) };
        float h = r.Range(0.06f, 0.2f);
        b.M(MAT_WAX).Cylinder(p, { p.x, h, p.z }, 0.022f, 0.02f, 10, true);
        b.M(MAT_FIRE).Ellipsoid({ p.x, h + 0.02f, p.z }, { 0.006f, 0.016f, 0.006f }, 4, 6);
    }
    LightDef& l = b.AddLight({ 0, 0.3f, 0 }, { 1.0f, 0.62f, 0.28f }, 1.4f, 5.0f, 0.1f);
    l.flicker = 0.35f;
    l.group = "candles";
}

static void BuildScratches(PrefabBuild& b, const Entity& e) {
    // gouges in the floor leading to the bookshelf
    Rng r(3);
    for (int i = 0; i < 9; i++) {
        float z = r.Range(-0.3f, 0.3f);
        b.M(MAT_BLOOD_DRY).BoxRot({ i * 0.35f, 0.004f, z }, { 0.6f, 0.004f, 0.012f }, { 0, r.Range(-8, 8), 0 });
        b.M(MAT_BLOOD_DRY).BoxRot({ i * 0.35f, 0.004f, z + 0.04f }, { 0.5f, 0.004f, 0.01f }, { 0, r.Range(-8, 8), 0 });
    }
    b.castShadow = false;
    (void)e;
}

void RegisterCollegePrefabs() {
    {
        PrefabInfo p; p.name = "college_building"; p.category = "college"; p.build = BuildCollege; p.terrain = CollegeTerrain;
        RegisterPrefab(p);
    }
    auto reg = [](const char* n, void (*fn)(PrefabBuild&, const Entity&), std::vector<std::string> keys = {}) {
        PrefabInfo p; p.name = n; p.category = "college"; p.build = fn; p.geometryKeys = keys;
        RegisterPrefab(p);
    };
    reg("classroom_set", BuildClassroom, { "variant", "rows", "cols" });
    reg("lockers", BuildLockers, { "count" });
    reg("office_desk", BuildOfficeDesk);
    reg("ledger", BuildLedger);
    reg("boiler", BuildBoiler);
    reg("old_bed", BuildOldBed);
    reg("candles", BuildCandle, { "variant" });
    reg("scratch_marks", BuildScratches);
    {
        PrefabInfo p; p.name = "bookshelf_secret"; p.category = "college"; p.build = BuildBookshelf;
        p.behaviour = [](Entity&) { return std::unique_ptr<Behaviour>(new SlideBeh()); };
        RegisterPrefab(p);
    }
}
