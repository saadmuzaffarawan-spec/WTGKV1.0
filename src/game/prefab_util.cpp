#include "prefab_util.h"
#include <map>
#include <cstdlib>
#include <algorithm>

Texture2D MakeSignTexture(const char* text, ui::FontId font, float fontSize, Color fg, Color bg,
                          int w, int h, float weather, uint32_t seed, int align) {
    RenderTexture2D rt = LoadRenderTexture(w, h);
    BeginTextureMode(rt);
    ClearBackground(bg);
    Font f = ui::GetFont(font);
    // draw each line centred (or left aligned)
    std::vector<std::string> lines;
    { std::string cur; for (const char* p = text; *p; p++) { if (*p == '\n') { lines.push_back(cur); cur.clear(); } else cur += *p; } lines.push_back(cur); }
    // shrink to fit (keeps a margin of 6% on each side)
    for (int guard = 0; guard < 40; guard++) {
        float maxW = 0;
        for (auto& ln : lines) maxW = fmaxf(maxW, MeasureTextEx(f, ln.c_str(), fontSize, fontSize * 0.04f).x);
        if (maxW <= w * 0.88f && fontSize * 1.08f * lines.size() <= h * 0.86f) break;
        fontSize *= 0.93f;
    }
    float lh = fontSize * 1.08f;
    float total = lh * lines.size();
    float y = (h - total) * 0.5f;
    for (auto& ln : lines) {
        Vector2 m = MeasureTextEx(f, ln.c_str(), fontSize, fontSize * 0.04f);
        float x = align == 0 ? (w - m.x) * 0.5f : fontSize * 0.5f;
        DrawTextEx(f, ln.c_str(), { x, y }, fontSize, fontSize * 0.04f, fg);
        y += lh;
    }
    // border inset line (classic road/guide sign)
    if (align == 0 && weather < 0.9f) DrawRectangleLinesEx({ 6, 6, (float)w - 12, (float)h - 12 }, 4, Fade(fg, 0.9f));
    EndTextureMode();
    Image img = LoadImageFromTexture(rt.texture);
    UnloadRenderTexture(rt);
    ImageFlipVertical(&img);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color* px = (Color*)img.data;
    for (int yy = 0; yy < h; yy++)
        for (int xx = 0; xx < w; xx++) {
            float u = (float)xx / w * 6.0f, v = (float)yy / h * 6.0f * h / w;
            Color& c = px[yy * w + xx];
            float n = Fbm2(u, v, 4, 0, seed);
            float streak = Fbm2(u * 4.0f, v * 0.3f, 3, 0, seed + 7);
            float fade = 0.75f + 0.25f * n;
            float dirt = SmoothStep(0.1f, 0.6f, streak) * weather;
            float chip = SmoothStep(0.45f, 0.55f, Fbm2(u * 3.0f, v * 3.0f, 4, 0, seed + 13)) * weather;
            float r = c.r * fade, g = c.g * fade, b = c.b * fade;
            r = Lerp(r, 70, dirt * 0.5f); g = Lerp(g, 60, dirt * 0.5f); b = Lerp(b, 45, dirt * 0.5f);
            r = Lerp(r, 110, chip); g = Lerp(g, 100, chip); b = Lerp(b, 92, chip);
            float grain = Hash2f(xx, yy, seed) * 18.0f - 9.0f;
            c.r = (unsigned char)Clamp(r + grain, 0, 255);
            c.g = (unsigned char)Clamp(g + grain, 0, 255);
            c.b = (unsigned char)Clamp(b + grain, 0, 255);
            c.a = 255;
        }
    if (getenv("WTGK_DUMP_SIGNS")) ExportImage(img, TextFormat("sign_%u.png", seed));
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_ANISOTROPIC_4X);
    SetTextureWrap(t, TEXTURE_WRAP_CLAMP);
    return t;
}

int SignMaterial(const std::string& key, const char* text, ui::FontId font, float fontSize, Color fg, Color bg,
                 int w, int h, float weather, bool emissive, float strength) {
    static std::map<std::string, int> cache;
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    Texture2D t = MakeSignTexture(text, font, fontSize, fg, bg, w, h, weather, (uint32_t)std::hash<std::string>()(key));
    int m = MakeSignMaterial(t, emissive, strength);
    cache[key] = m;
    return m;
}

void GrowBranch(ModelBuilder& mb, Rng& rng, Vector3 base, Vector3 dir, float len, float rad, int depth, int maxDepth, int mat) {
    const int segs = depth == 0 ? 7 : 5;
    std::vector<Vector3> pts;
    std::vector<float> radii;
    Vector3 p = base, d = Vector3Normalize(dir);
    pts.push_back(p); radii.push_back(rad);
    for (int i = 1; i <= segs; i++) {
        float t = (float)i / segs;
        // gnarled bending: random wander + slight upward tendency, droop for thin branches
        Vector3 wander{ rng.Signed() * 0.35f, rng.Signed() * 0.2f, rng.Signed() * 0.35f };
        d = Vector3Normalize(Vector3Add(d, Vector3Scale(wander, depth == 0 ? 0.35f : 0.6f)));
        d.y += depth == 0 ? 0.05f : (depth >= 2 ? -0.06f : 0.04f);
        d = Vector3Normalize(d);
        p = Vector3Add(p, Vector3Scale(d, len / segs));
        pts.push_back(p);
        radii.push_back(rad * (1.0f - t * 0.82f) + 0.004f);
    }
    mb.Flex(depth == 0 ? 0.0f : (depth == 1 ? 0.12f : (depth == 2 ? 0.45f : 0.85f)));
    mb.M(mat).Tube(pts, radii, depth == 0 ? 10 : (depth == 1 ? 7 : 5), true);
    if (depth >= maxDepth) return;
    int kids = depth == 0 ? rng.RangeI(3, 5) : rng.RangeI(2, 4);
    float golden = rng.Range(0, 6.28f);
    for (int k = 0; k < kids; k++) {
        float t = rng.Range(depth == 0 ? 0.4f : 0.3f, 0.95f);
        int idx = (int)(t * segs);
        if (idx >= (int)pts.size() - 1) idx = (int)pts.size() - 2;
        Vector3 bp = Vector3Lerp(pts[idx], pts[idx + 1], 0.5f);
        Vector3 axis = Vector3Normalize(Vector3Subtract(pts[idx + 1], pts[idx]));
        Vector3 side = Vector3Normalize(Vector3CrossProduct(axis, fabsf(axis.y) < 0.9f ? Vector3{ 0, 1, 0 } : Vector3{ 1, 0, 0 }));
        golden += 2.39996f;
        Vector3 perp = Vector3RotateByAxisAngle(side, axis, golden);
        float spread = rng.Range(0.5f, 1.1f);
        Vector3 nd = Vector3Normalize(Vector3Add(Vector3Scale(axis, cosf(spread)), Vector3Scale(perp, sinf(spread))));
        float r = radii[idx] * rng.Range(0.45f, 0.7f);
        GrowBranch(mb, rng, bp, nd, len * rng.Range(0.45f, 0.7f) * (1.0f - t * 0.4f), r, depth + 1, maxDepth, mat);
    }
}

std::vector<Vector3> Catenary(Vector3 a, Vector3 b, float sag, int segs) {
    std::vector<Vector3> pts;
    for (int i = 0; i <= segs; i++) {
        float t = (float)i / segs;
        Vector3 p = Vector3Lerp(a, b, t);
        p.y -= sag * 4.0f * t * (1.0f - t);
        pts.push_back(p);
    }
    return pts;
}

void BuildWall(PrefabBuild& b, Vector2 a, Vector2 c, float y0, float H, float thick, int matOut, int matIn,
               std::vector<Opening> ops, int surf, float baseboard) {
    Vector2 d = Vector2Subtract(c, a);
    float L = Vector2Length(d);
    if (L < 0.01f) return;
    Vector2 u = Vector2Scale(d, 1.0f / L);
    Vector2 n{ -u.y, u.x };   // left normal (outer side)
    float yawDeg = atan2f(-u.y, u.x) * RAD2DEG;
    std::sort(ops.begin(), ops.end(), [](const Opening& p, const Opening& q) { return p.at < q.at; });
    auto piece = [&](float s0, float s1, float ya, float yb) {
        if (s1 - s0 < 0.005f || yb - ya < 0.005f) return;
        float mid = (s0 + s1) * 0.5f;
        Vector2 p = Vector2Add(a, Vector2Scale(u, mid));
        float h = yb - ya, yc = y0 + (ya + yb) * 0.5f;
        float half = thick * 0.5f;
        Vector2 po = Vector2Add(p, Vector2Scale(n, half * 0.5f));
        Vector2 pi = Vector2Subtract(p, Vector2Scale(n, half * 0.5f));
        b.M(matOut).BoxRot({ po.x, yc, po.y }, { s1 - s0, h, half }, { 0, yawDeg, 0 }, 0.0f);
        b.M(matIn).BoxRot({ pi.x, yc, pi.y }, { s1 - s0, h, half }, { 0, yawDeg, 0 }, 0.0f);
        b.Collider({ p.x, yc, p.y }, { s1 - s0, h, thick }, surf, yawDeg, true, true);
        if (baseboard > 0 && ya < 0.01f) {
            Vector2 pb = Vector2Subtract(p, Vector2Scale(n, half + 0.01f));
            b.M(MAT_WOOD_DARK).BoxRot({ pb.x, y0 + baseboard * 0.5f, pb.y }, { s1 - s0, baseboard, 0.02f }, { 0, yawDeg, 0 }, 0.0f);
        }
    };
    float cur = 0;
    for (const Opening& o : ops) {
        float s0 = o.at - o.width * 0.5f, s1 = o.at + o.width * 0.5f;
        piece(cur, s0, 0, H);
        if (o.sill > 0) piece(s0, s1, 0, o.sill);
        if (o.top < H) piece(s0, s1, o.top, H);
        // reveal / frame trim inside the opening
        Vector2 p0 = Vector2Add(a, Vector2Scale(u, s0)), p1 = Vector2Add(a, Vector2Scale(u, s1));
        for (Vector2 pp : { p0, p1 })
            b.M(MAT_WOOD_DARK).BoxRot({ pp.x, y0 + (o.sill + o.top) * 0.5f, pp.y }, { 0.05f, o.top - o.sill, thick + 0.02f }, { 0, yawDeg, 0 }, 0.005f);
        Vector2 pm = Vector2Add(a, Vector2Scale(u, o.at));
        b.M(MAT_WOOD_DARK).BoxRot({ pm.x, y0 + o.top + 0.025f, pm.y }, { o.width + 0.1f, 0.05f, thick + 0.02f }, { 0, yawDeg, 0 }, 0.005f);
        cur = s1;
    }
    piece(cur, L, 0, H);
}

void BuildWindow(PrefabBuild& b, Vector2 a, Vector2 c, float y0, const Opening& o, int glassMat, int frameMat, int panesX, bool broken, bool boarded) {
    Vector2 d = Vector2Subtract(c, a);
    float L = Vector2Length(d);
    Vector2 u = Vector2Scale(d, 1.0f / L);
    float yawDeg = atan2f(-u.y, u.x) * RAD2DEG;
    Vector2 m = Vector2Add(a, Vector2Scale(u, o.at));
    float h = o.top - o.sill, yc = y0 + (o.sill + o.top) * 0.5f;
    // sill
    b.M(frameMat).BoxRot({ m.x, y0 + o.sill - 0.02f, m.y }, { o.width + 0.08f, 0.05f, 0.28f }, { 0, yawDeg, 0 }, 0.01f);
    for (int i = 0; i <= panesX; i++) {
        float t = -o.width * 0.5f + o.width * i / panesX;
        Vector2 p = Vector2Add(m, Vector2Scale(u, t));
        b.M(frameMat).BoxRot({ p.x, yc, p.y }, { 0.05f, h, 0.07f }, { 0, yawDeg, 0 }, 0.005f);
    }
    b.M(frameMat).BoxRot({ m.x, yc + h * 0.1f, m.y }, { o.width, 0.04f, 0.06f }, { 0, yawDeg, 0 }, 0.005f);
    if (boarded) {
        Rng rng((uint32_t)(m.x * 100 + m.y * 7) | 1);
        for (int k = 0; k < 4; k++) {
            float yy = yc - h * 0.35f + k * h * 0.24f;
            Vector2 p = Vector2Add(m, Vector2Scale(Vector2{ -u.y, u.x }, 0.07f));
            b.M(MAT_WOOD).BoxRot({ p.x, yy, p.y }, { o.width + 0.2f, 0.18f, 0.025f }, { rng.Signed() * 4.0f, yawDeg, rng.Signed() * 8.0f }, 0.004f);
        }
        return;
    }
    if (!broken) b.M(glassMat).BoxRot({ m.x, yc, m.y }, { o.width, h, 0.01f }, { 0, yawDeg, 0 }, 0.0f);
    else {
        // jagged shards left in the frame
        Rng rng((uint32_t)(m.x * 31 + m.y * 17) | 1);
        for (int k = 0; k < 5; k++) {
            float t = rng.Range(-0.45f, 0.45f) * o.width;
            float hy = rng.Range(0.1f, 0.35f) * h;
            Vector2 p = Vector2Add(m, Vector2Scale(u, t));
            float yb = y0 + (rng.Chance(0.5f) ? o.sill : o.top - hy);
            Vector3 A{ p.x - u.x * 0.15f, yb, p.y - u.y * 0.15f }, B{ p.x + u.x * 0.15f, yb, p.y + u.y * 0.15f };
            Vector3 C{ p.x + u.x * rng.Signed() * 0.1f, yb + (yb > yc ? -hy : hy), p.y + u.y * rng.Signed() * 0.1f };
            MeshBuilder& g = b.M(glassMat);
            g.Tri(A, B, C); g.Tri(A, C, B);
        }
    }
}
