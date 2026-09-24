#include "textures.h"
#include <rlgl.h>
#include <thread>
#include <atomic>
#include <cstring>

Rng& FxRng() { static Rng r(0xC0FFEE1234ULL); return r; }

std::string TrimStr(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}
std::vector<std::string> SplitStr(const std::string& s, char sep) {
    std::vector<std::string> out; std::string cur;
    for (char c : s) {
        if (c == sep) { if (!cur.empty()) out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

namespace {

struct Pixel { float h; float r, g, b; float rough; };

struct GenCtx {
    int size;
    // tileable noise in [-1,1] with integer frequency `f` over the tile
    float N(float u, float v, int f, int oct, uint32_t seed) const { return Fbm2(u * f, v * f, oct, f, seed); }
    float W(float u, float v, int f, uint32_t seed, float* f2 = nullptr, uint32_t* id = nullptr) const {
        return Worley2(u * f, v * f, f, seed, f2, id);
    }
};

inline void Mix(Pixel& p, float r, float g, float b, float t) {
    p.r = Lerp(p.r, r, t); p.g = Lerp(p.g, g, t); p.b = Lerp(p.b, b, t);
}
inline void Mul(Pixel& p, float k) { p.r *= k; p.g *= k; p.b *= k; }

// ---------------------------------------------------------------------------
// Material generators. u,v in [0,1). Each writes height/albedo/roughness.
// ---------------------------------------------------------------------------
Pixel GenAsphalt(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.12f, 0.12f, 0.12f, 0.88f };
    float base = 0.115f + 0.025f * c.N(u, v, 6, 4, 11);
    p.r = base * 1.02f; p.g = base; p.b = base * 0.97f;
    uint32_t id; float f2;
    float w = c.W(u, v, 110, 3, &f2, &id);
    float stone = SmoothStep(0.42f, 0.18f, w);
    float sg = 0.12f + 0.16f * Hash01(id);
    Mix(p, sg * 1.03f, sg, sg * 0.95f, stone * 0.75f);
    p.h = 0.45f + stone * 0.25f + c.N(u, v, 48, 3, 5) * 0.08f;
    // Tar patches and oil stains (smoother, darker)
    float tar = SmoothStep(0.18f, 0.4f, c.N(u, v, 3, 4, 21));
    Mix(p, 0.055f, 0.055f, 0.06f, tar * 0.7f);
    p.rough = Lerp(p.rough, 0.55f, tar);
    p.h = Lerp(p.h, 0.42f, tar * 0.8f);
    // Cracks
    float cw2; float cw = c.W(u + c.N(u, v, 10, 2, 9) * 0.02f, v + c.N(u, v, 10, 2, 19) * 0.02f, 5, 77, &cw2);
    float crackMask = SmoothStep(0.035f, 0.0f, cw2 - cw) * SmoothStep(-0.1f, 0.3f, c.N(u, v, 4, 2, 88));
    Mix(p, 0.025f, 0.025f, 0.025f, crackMask * 0.9f);
    p.h -= crackMask * 0.35f;
    // sun-bleached light spots
    float bleach = SmoothStep(0.3f, 0.6f, c.N(u, v, 5, 3, 31));
    Mul(p, 1.0f + bleach * 0.25f);
    return p;
}

Pixel GenConcrete(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.44f, 0.43f, 0.41f, 0.92f };
    float m = c.N(u, v, 5, 5, 3);
    Mul(p, 1.0f + m * 0.12f);
    float stain = SmoothStep(0.05f, 0.55f, c.N(u, v, 2, 4, 44));
    p.r *= Lerp(1.0f, 0.78f, stain); p.g *= Lerp(1.0f, 0.74f, stain); p.b *= Lerp(1.0f, 0.66f, stain);
    float drip = SmoothStep(0.2f, 0.7f, c.N(u * 1.0f, v * 0.12f, 24, 3, 55)) * 0.5f;
    Mul(p, 1.0f - drip * 0.3f);
    float w = c.W(u, v, 140, 8);
    float pore = SmoothStep(0.14f, 0.03f, w);
    Mul(p, 1.0f - pore * 0.45f);
    p.h = 0.5f + c.N(u, v, 32, 3, 7) * 0.12f - pore * 0.3f;
    float agg = SmoothStep(0.25f, 0.15f, c.W(u, v, 60, 99));
    Mul(p, 1.0f + agg * 0.08f);
    return p;
}

Pixel GenDirt(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.21f, 0.165f, 0.12f, 0.95f };
    Mul(p, 1.0f + c.N(u, v, 4, 5, 13) * 0.25f);
    float moist = SmoothStep(0.1f, 0.5f, c.N(u, v, 3, 3, 66));
    Mul(p, 1.0f - moist * 0.35f);
    p.rough = Lerp(p.rough, 0.7f, moist);
    uint32_t id; float w = c.W(u, v, 70, 17, nullptr, &id);
    float peb = SmoothStep(0.3f, 0.12f, w) * (Hash01(id) > 0.7f ? 1.0f : 0.0f);
    float pg = 0.2f + Hash01(id * 7) * 0.1f;
    Mix(p, pg, pg * 0.95f, pg * 0.88f, peb * 0.8f);
    p.h = 0.45f + c.N(u, v, 12, 4, 2) * 0.2f + peb * 0.3f;
    // twigs / dead fibres
    float fib = SmoothStep(0.965f, 1.0f, 1.0f - fabsf(c.N(u, v * 0.3f, 50, 1, 222)));
    Mix(p, 0.3f, 0.24f, 0.16f, fib * 0.4f);
    return p;
}

Pixel GenGrass(const GenCtx& c, float u, float v) {
    // Dry, dead late-autumn grass seen top-down
    Pixel p{ 0.3f, 0.10f, 0.08f, 0.05f, 0.95f };
    float blades = 0.0f;
    for (int i = 0; i < 4; i++) {
        float a = i * 0.785f + 0.3f;
        float cu = cosf(a), su = sinf(a);
        float ru = u * cu - v * su, rv = u * su + v * cu;
        // anisotropic noise: stretched along blade direction (periodicity only approx after rotation; hide with low contrast)
        float n = GradNoise2(ru * 160.0f, rv * 22.0f, 0, 900 + i);
        blades = fmaxf(blades, SmoothStep(0.25f, 0.75f, n));
    }
    // make tileable-ish by blending with a periodic layer
    float per = SmoothStep(0.2f, 0.8f, c.N(u, v, 64, 2, 321) * 0.5f + 0.5f);
    blades = Lerp(blades, per, 0.35f);
    float hue = c.N(u, v, 3, 4, 42) * 0.5f + 0.5f;
    float dryR = Lerp(0.30f, 0.46f, hue), dryG = Lerp(0.28f, 0.38f, hue), dryB = Lerp(0.13f, 0.20f, hue);
    float olive = SmoothStep(0.45f, 0.7f, c.N(u, v, 2, 3, 71) * 0.5f + 0.5f);
    dryR = Lerp(dryR, 0.20f, olive); dryG = Lerp(dryG, 0.23f, olive); dryB = Lerp(dryB, 0.11f, olive);
    Mix(p, dryR, dryG, dryB, blades);
    p.h = 0.25f + blades * 0.6f;
    float soil = SmoothStep(0.3f, 0.6f, c.N(u, v, 6, 3, 5));
    Mix(p, 0.12f, 0.09f, 0.06f, soil * (1.0f - blades) * 0.6f);
    return p;
}

Pixel GenGravel(const GenCtx& c, float u, float v) {
    Pixel p{ 0.2f, 0.07f, 0.065f, 0.06f, 0.9f };
    uint32_t id; float f2;
    float w = c.W(u, v, 28, 5, &f2, &id);
    float stone = SmoothStep(0.62f, 0.4f, w) * SmoothStep(0.0f, 0.08f, f2 - w);
    float g = 0.16f + Hash01(id) * 0.16f;
    float warm = Hash01(id * 3);
    Mix(p, g * (1.0f + warm * 0.15f), g, g * (1.0f - warm * 0.12f), stone);
    Mul(p, 1.0f + c.N(u, v, 90, 2, 7) * 0.12f * stone);
    p.h = 0.2f + stone * sqrtf(Saturate(1.0f - w / 0.62f)) * 0.8f;
    return p;
}

Pixel GenBrick(const GenCtx& c, float u, float v) {
    const int rows = 15, cols = 4;
    float fy = v * rows;
    int row = (int)floorf(fy);
    float fx = u * cols + ((row & 1) ? 0.5f : 0.0f);
    int col = (int)floorf(fx);
    float lx = fx - col, ly = fy - row;
    float jitter = c.N(u, v, 40, 2, 3) * 0.02f;
    float mortarX = SmoothStep(0.035f + jitter, 0.015f, fminf(lx, 1.0f - lx));
    float mortarY = SmoothStep(0.12f + jitter * 3.0f, 0.06f, fminf(ly, 1.0f - ly));
    float mortar = fmaxf(mortarX, mortarY);
    uint32_t bid = Hash2i(((col % cols) + cols) % cols, row, 12);
    float t = Hash01(bid);
    float r = Lerp(0.36f, 0.52f, t), g = Lerp(0.15f, 0.22f, t), b = Lerp(0.10f, 0.14f, t);
    if (Hash01(bid * 5) > 0.85f) { r *= 0.55f; g *= 0.55f; b *= 0.6f; }   // burnt bricks
    Pixel p{ 0.7f, r, g, b, 0.9f };
    Mul(p, 1.0f + c.N(u, v, 30, 3, 9) * 0.15f);
    float soot = SmoothStep(0.1f, 0.6f, c.N(u, v, 2, 3, 81));
    Mul(p, 1.0f - soot * 0.4f);
    Mix(p, 0.42f, 0.40f, 0.36f, mortar);
    p.h = 0.75f - mortar * 0.5f + c.N(u, v, 60, 2, 4) * 0.05f;
    float efflo = SmoothStep(0.45f, 0.75f, c.N(u, v, 4, 3, 17));
    Mix(p, 0.55f, 0.53f, 0.5f, efflo * 0.3f);
    return p;
}

Pixel GenWood(const GenCtx& c, float u, float v) {
    const int planks = 6;
    float fx = u * planks;
    int pl = (int)floorf(fx);
    float lx = fx - pl;
    uint32_t pid = Hash2i(pl, 0, 33);
    float gap = SmoothStep(0.03f, 0.0f, fminf(lx, 1.0f - lx));
    float grainN = c.N(u * 0.3f, v, 40, 3, pid & 255);
    float grain = sinf((lx * 9.0f + grainN * 3.0f + Hash01(pid) * 10.0f) * 6.2831f) * 0.5f + 0.5f;
    float tone = Hash01(pid * 3);
    Pixel p{ 0.6f, Lerp(0.30f, 0.40f, tone), Lerp(0.26f, 0.33f, tone), Lerp(0.21f, 0.26f, tone), 0.85f };
    Mul(p, 0.85f + grain * 0.25f);
    float weather = SmoothStep(0.0f, 0.6f, c.N(u, v, 5, 3, 91));
    Mix(p, 0.36f, 0.35f, 0.33f, weather * 0.45f);
    float rot = SmoothStep(0.4f, 0.7f, c.N(u, v, 3, 4, 13));
    Mix(p, 0.12f, 0.1f, 0.08f, rot * 0.6f);
    Mix(p, 0.03f, 0.025f, 0.02f, gap);
    p.h = 0.6f + grain * 0.1f - gap * 0.6f - rot * 0.1f;
    // nails
    float ny = fmodf(v * 2.0f + Hash01(pid * 9), 1.0f);
    float nd = sqrtf((lx - 0.5f) * (lx - 0.5f) * 36.0f + (ny - 0.5f) * (ny - 0.5f) * 400.0f);
    float nail = SmoothStep(0.5f, 0.2f, nd);
    Mix(p, 0.12f, 0.07f, 0.04f, nail);
    return p;
}

Pixel GenPaint(const GenCtx& c, float u, float v) {
    Pixel p{ 0.6f, 0.82f, 0.81f, 0.78f, 0.5f };
    Mul(p, 1.0f + c.N(u, v, 6, 4, 1) * 0.06f);
    float grime = SmoothStep(0.1f, 0.7f, c.N(u, v, 3, 4, 2));
    p.r *= Lerp(1.0f, 0.72f, grime); p.g *= Lerp(1.0f, 0.7f, grime); p.b *= Lerp(1.0f, 0.62f, grime);
    p.rough = Lerp(0.45f, 0.75f, grime);
    float streak = SmoothStep(0.3f, 0.8f, c.N(u, v * 0.1f, 30, 2, 12) * 0.5f + 0.5f) * 0.25f;
    Mul(p, 1.0f - streak);
    float chip = SmoothStep(0.42f, 0.5f, c.N(u, v, 9, 5, 7));
    float rustN = c.N(u, v, 40, 2, 99) * 0.5f + 0.5f;
    Mix(p, Lerp(0.30f, 0.45f, rustN), Lerp(0.14f, 0.22f, rustN), 0.07f, chip);
    p.rough = Lerp(p.rough, 0.95f, chip);
    float scratch = SmoothStep(0.012f, 0.0f, fabsf(GradNoise2(u * 3.0f, v * 70.0f, 0, 5))) *
                    SmoothStep(0.2f, 0.5f, c.N(u, v, 4, 2, 3));
    Mix(p, 0.55f, 0.55f, 0.55f, scratch * 0.5f);
    p.h = 0.6f - chip * 0.25f - scratch * 0.1f;
    return p;
}

Pixel GenRust(const GenCtx& c, float u, float v) {
    float n = c.N(u, v, 6, 5, 3) * 0.5f + 0.5f;
    Pixel p{ 0.5f, Lerp(0.13f, 0.30f, n), Lerp(0.08f, 0.15f, n), Lerp(0.05f, 0.08f, n), 0.92f };
    float dark = SmoothStep(0.3f, 0.7f, c.N(u, v, 14, 3, 8) * 0.5f + 0.5f);
    Mul(p, 1.0f - dark * 0.5f);
    float w = c.W(u, v, 70, 9);
    float pit = SmoothStep(0.2f, 0.05f, w);
    Mul(p, 1.0f - pit * 0.5f);
    float flake = SmoothStep(0.5f, 0.6f, c.N(u, v, 20, 3, 12) * 0.5f + 0.5f);
    Mix(p, 0.36f, 0.22f, 0.12f, flake * 0.4f);
    p.h = 0.5f + n * 0.3f - pit * 0.3f + flake * 0.15f;
    return p;
}

Pixel GenTile(const GenCtx& c, float u, float v) {
    const int n = 4;
    float fx = u * n, fy = v * n;
    int tx = (int)floorf(fx), ty = (int)floorf(fy);
    float lx = fx - tx, ly = fy - ty;
    float grout = SmoothStep(0.02f, 0.008f, fminf(fminf(lx, 1 - lx), fminf(ly, 1 - ly)));
    bool alt = ((tx + ty) & 1) != 0;
    Pixel p{ 0.6f, alt ? 0.42f : 0.70f, alt ? 0.44f : 0.67f, alt ? 0.40f : 0.58f, 0.35f };
    uint32_t id = Hash2i(tx, ty, 4);
    Mul(p, 0.94f + Hash01(id) * 0.1f);
    float speck = SmoothStep(0.05f, 0.0f, c.W(u, v, 180, 3));
    Mix(p, 0.25f, 0.25f, 0.22f, speck * 0.6f);
    float scuff = SmoothStep(0.55f, 0.8f, fabsf(c.N(u, v * 0.25f, 16, 3, 6)) * 1.6f);
    Mix(p, 0.18f, 0.17f, 0.15f, scuff * 0.35f);
    float grime = SmoothStep(0.0f, 0.7f, c.N(u, v, 3, 4, 21));
    Mul(p, 1.0f - grime * 0.35f);
    p.rough = Lerp(0.3f, 0.8f, grime * 0.7f + scuff * 0.3f);
    Mix(p, 0.12f, 0.11f, 0.09f, grout);
    p.rough = Lerp(p.rough, 0.95f, grout);
    p.h = 0.7f - grout * 0.5f;
    return p;
}

Pixel GenPlaster(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.70f, 0.68f, 0.62f, 0.9f };
    Mul(p, 1.0f + c.N(u, v, 8, 4, 2) * 0.05f);
    // water damage: vertical streaks + tide rings
    float streak = SmoothStep(0.2f, 0.8f, c.N(u, v * 0.08f, 22, 3, 5) * 0.5f + 0.5f);
    float area = SmoothStep(0.0f, 0.5f, c.N(u, v, 2, 3, 7));
    float dmg = streak * area;
    p.r *= Lerp(1.0f, 0.62f, dmg); p.g *= Lerp(1.0f, 0.55f, dmg); p.b *= Lerp(1.0f, 0.42f, dmg);
    float ring = SmoothStep(0.02f, 0.0f, fabsf(c.N(u, v, 3, 3, 9) - 0.2f));
    Mix(p, 0.42f, 0.34f, 0.22f, ring * 0.5f);
    float mold = SmoothStep(0.5f, 0.8f, c.N(u, v, 10, 4, 33) * 0.5f + 0.5f) * area;
    Mix(p, 0.16f, 0.17f, 0.13f, mold * 0.6f);
    float cw2; float cw = c.W(u, v, 4, 5, &cw2);
    float crack = SmoothStep(0.02f, 0.0f, cw2 - cw) * SmoothStep(0.1f, 0.4f, c.N(u, v, 5, 2, 44));
    Mix(p, 0.2f, 0.18f, 0.16f, crack);
    p.h = 0.5f + c.N(u, v, 40, 3, 6) * 0.06f - crack * 0.3f;
    return p;
}

Pixel GenCloth(const GenCtx& c, float u, float v) {
    float wu = sinf(u * 6.2831f * 160.0f), wv = sinf(v * 6.2831f * 160.0f);
    float weave = (wu * 0.5f + 0.5f) * (wv > 0 ? 1.0f : 0.6f);
    Pixel p{ 0.5f, 0.62f, 0.62f, 0.62f, 1.0f };
    Mul(p, 0.85f + weave * 0.15f + c.N(u, v, 10, 4, 3) * 0.08f);
    float stain = SmoothStep(0.25f, 0.6f, c.N(u, v, 3, 3, 12));
    Mul(p, 1.0f - stain * 0.35f);
    p.h = 0.5f + weave * 0.2f;
    return p;
}

Pixel GenSkin(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.78f, 0.62f, 0.55f, 0.55f };
    float m = c.N(u, v, 10, 4, 5);
    p.r *= 1.0f + m * 0.08f; p.g *= 1.0f + m * 0.04f;
    float red = SmoothStep(0.2f, 0.6f, c.N(u, v, 5, 3, 8));
    p.g *= 1.0f - red * 0.12f; p.b *= 1.0f - red * 0.08f;
    float pore = SmoothStep(0.1f, 0.0f, c.W(u, v, 220, 3));
    Mul(p, 1.0f - pore * 0.12f);
    float vein = SmoothStep(0.03f, 0.0f, fabsf(c.N(u, v, 4, 3, 77))) * 0.3f;
    p.r *= 1.0f - vein * 0.4f; p.g *= 1.0f - vein * 0.2f;
    p.h = 0.5f - pore * 0.2f + c.N(u, v, 60, 2, 9) * 0.05f;
    return p;
}

Pixel GenBark(const GenCtx& c, float u, float v) {
    float r = Ridged2(u * 10.0f + c.N(u, v, 4, 2, 5) * 1.5f, v * 1.5f, 4, 10, 17);
    Pixel p{ r, 0.20f, 0.17f, 0.145f, 0.95f };
    float lift = SmoothStep(0.4f, 0.9f, r);
    Mix(p, 0.36f, 0.33f, 0.29f, lift * 0.7f);
    Mul(p, 1.0f + c.N(u, v, 30, 3, 4) * 0.15f);
    float lichen = SmoothStep(0.35f, 0.65f, c.N(u, v, 6, 4, 23));
    Mix(p, 0.34f, 0.37f, 0.30f, lichen * 0.35f);
    return p;
}

Pixel GenRubber(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.045f, 0.045f, 0.048f, 0.85f };
    Mul(p, 1.0f + c.N(u, v, 40, 3, 5) * 0.2f);
    float dust = SmoothStep(0.3f, 0.7f, c.N(u, v, 5, 3, 9));
    Mix(p, 0.16f, 0.14f, 0.12f, dust * 0.4f);
    p.h = 0.5f + c.N(u, v, 80, 2, 3) * 0.1f;
    return p;
}

Pixel GenCarPaint(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.8f, 0.8f, 0.8f, 0.28f };
    p.h = 0.5f + c.N(u, v, 120, 2, 5) * 0.03f;   // orange peel
    float dirt = SmoothStep(0.2f, 0.8f, c.N(u, v, 3, 4, 8));
    Mix(p, 0.45f, 0.40f, 0.33f, dirt * 0.35f);
    p.rough = Lerp(0.22f, 0.7f, dirt);
    float scratch = SmoothStep(0.01f, 0.0f, fabsf(GradNoise2(u * 5.0f, v * 50.0f, 0, 7))) *
                    SmoothStep(0.3f, 0.6f, c.N(u, v, 3, 2, 3));
    Mix(p, 0.9f, 0.9f, 0.9f, scratch * 0.4f);
    return p;
}

Pixel GenRock(const GenCtx& c, float u, float v) {
    float strata = sinf(v * 6.2831f * 7.0f + c.N(u, v, 3, 4, 3) * 5.0f) * 0.5f + 0.5f;
    float r = Ridged2(u * 6.0f, v * 6.0f, 5, 6, 9);
    Pixel p{ 0.5f, 0.30f, 0.27f, 0.24f, 0.9f };
    Mul(p, 0.75f + strata * 0.35f);
    Mix(p, 0.16f, 0.14f, 0.13f, SmoothStep(0.6f, 0.9f, r) * 0.6f);
    Mix(p, 0.24f, 0.22f, 0.17f, SmoothStep(0.3f, 0.7f, c.N(u, v, 5, 3, 55)) * 0.3f);
    p.h = strata * 0.3f + (1.0f - r) * 0.5f + c.N(u, v, 40, 3, 5) * 0.1f;
    return p;
}

Pixel GenFlesh(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.30f, 0.07f, 0.06f, 0.35f };
    Mul(p, 1.0f + c.N(u, v, 6, 4, 3) * 0.3f);
    float vein = 1.0f - fabsf(c.N(u, v, 5, 4, 15));
    float vm = SmoothStep(0.9f, 0.98f, vein);
    Mix(p, 0.14f, 0.03f, 0.07f, vm);
    float fat = SmoothStep(0.45f, 0.7f, c.N(u, v, 12, 3, 27) * 0.5f + 0.5f);
    Mix(p, 0.52f, 0.38f, 0.25f, fat * 0.35f);
    float w = c.W(u, v, 24, 3);
    p.h = 0.5f + vm * 0.3f + (1.0f - w) * 0.2f;
    p.rough = Lerp(0.2f, 0.6f, fat);
    return p;
}

Pixel GenSteel(const GenCtx& c, float u, float v) {
    float br = GradNoise2(u * 2.0f, v * 400.0f, 0, 3) * 0.5f + 0.5f;
    Pixel p{ 0.5f, 0.56f, 0.57f, 0.58f, 0.35f };
    Mul(p, 0.9f + br * 0.15f);
    float smudge = SmoothStep(0.2f, 0.7f, c.N(u, v, 4, 3, 5));
    Mul(p, 1.0f - smudge * 0.25f);
    p.rough = Lerp(0.3f, 0.6f, smudge);
    p.h = 0.5f + br * 0.05f;
    return p;
}

Pixel GenCardboard(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.56f, 0.43f, 0.29f, 0.95f };
    Mul(p, 1.0f + c.N(u, v, 8, 4, 3) * 0.08f);
    float corr = sinf(v * 6.2831f * 90.0f) * 0.5f + 0.5f;
    Mul(p, 0.97f + corr * 0.04f);
    float wet = SmoothStep(0.3f, 0.7f, c.N(u, v, 3, 3, 9));
    Mul(p, 1.0f - wet * 0.35f);
    p.h = 0.5f + corr * 0.05f;
    return p;
}

Pixel GenCorrugated(const GenCtx& c, float u, float v) {
    float wave = sinf(u * 6.2831f * 10.0f);
    Pixel p{ 0.5f + wave * 0.45f, 0.52f, 0.53f, 0.52f, 0.6f };
    float ox = SmoothStep(0.2f, 0.7f, c.N(u, v, 6, 4, 3));
    Mix(p, 0.68f, 0.68f, 0.64f, ox * 0.4f);
    float streak = SmoothStep(0.35f, 0.8f, c.N(u, v * 0.1f, 40, 2, 7) * 0.5f + 0.5f);
    float rustArea = SmoothStep(0.0f, 0.6f, c.N(u, v, 2, 3, 12));
    float rs = streak * rustArea;
    Mix(p, 0.40f, 0.20f, 0.08f, rs * 0.8f);
    p.rough = Lerp(0.55f, 0.9f, rs);
    return p;
}

Pixel GenLeather(const GenCtx& c, float u, float v) {
    float f2; float w = c.W(u, v, 50, 3, &f2);
    float crease = SmoothStep(0.06f, 0.0f, f2 - w);
    Pixel p{ 0.5f, 0.28f, 0.17f, 0.10f, 0.55f };
    Mul(p, 1.0f + c.N(u, v, 6, 3, 5) * 0.2f);
    Mul(p, 1.0f - crease * 0.35f);
    float worn = SmoothStep(0.3f, 0.7f, c.N(u, v, 4, 3, 9));
    Mix(p, 0.42f, 0.32f, 0.22f, worn * 0.35f);
    p.h = 0.6f - crease * 0.3f;
    return p;
}

Pixel GenBone(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.80f, 0.76f, 0.64f, 0.7f };
    Mul(p, 1.0f + c.N(u, v, 8, 4, 3) * 0.08f);
    float stain = SmoothStep(0.1f, 0.6f, c.N(u, v, 3, 3, 5));
    p.r *= Lerp(1.0f, 0.65f, stain); p.g *= Lerp(1.0f, 0.55f, stain); p.b *= Lerp(1.0f, 0.42f, stain);
    float pore = SmoothStep(0.1f, 0.0f, c.W(u, v, 120, 7));
    Mul(p, 1.0f - pore * 0.4f);
    p.h = 0.5f - pore * 0.3f + c.N(u, v * 0.3f, 30, 2, 5) * 0.08f;
    return p;
}

Pixel GenMud(const GenCtx& c, float u, float v) {
    Pixel p{ 0.5f, 0.14f, 0.105f, 0.075f, 0.7f };
    float n = c.N(u, v, 5, 5, 3);
    Mul(p, 1.0f + n * 0.25f);
    float puddle = SmoothStep(0.25f, 0.35f, c.N(u, v, 3, 4, 8));
    Mul(p, 1.0f - puddle * 0.35f);
    p.rough = Lerp(0.75f, 0.12f, puddle);
    p.h = Lerp(0.5f + n * 0.3f, 0.35f, puddle);
    return p;
}

typedef Pixel (*GenFn)(const GenCtx&, float, float);
struct MatDef { const char* name; GenFn fn; float normalStrength; };

const MatDef kDefs[TX_COUNT] = {
    { "asphalt", GenAsphalt, 1.6f }, { "concrete", GenConcrete, 2.0f }, { "dirt", GenDirt, 3.5f },
    { "grass", GenGrass, 3.0f }, { "gravel", GenGravel, 2.2f }, { "brick", GenBrick, 4.0f },
    { "wood", GenWood, 3.0f }, { "paint", GenPaint, 2.0f }, { "rust", GenRust, 4.0f },
    { "tile", GenTile, 2.5f }, { "plaster", GenPlaster, 1.5f }, { "cloth", GenCloth, 1.5f },
    { "skin", GenSkin, 1.0f }, { "bark", GenBark, 6.0f }, { "rubber", GenRubber, 2.0f },
    { "carpaint", GenCarPaint, 0.6f }, { "rock", GenRock, 5.0f }, { "flesh", GenFlesh, 4.0f },
    { "steel", GenSteel, 0.8f }, { "cardboard", GenCardboard, 1.2f }, { "corrugated", GenCorrugated, 6.0f },
    { "leather", GenLeather, 2.0f }, { "bone", GenBone, 2.0f }, { "mud", GenMud, 2.5f },
};

struct CpuTex { std::vector<uint8_t> albedo, normal; };

void GenerateCpu(int id, int N, CpuTex& out) {
    GenCtx c{ N };
    std::vector<Pixel> px((size_t)N * N);
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++)
            px[(size_t)y * N + x] = kDefs[id].fn(c, (x + 0.5f) / N, (y + 0.5f) / N);
    out.albedo.resize((size_t)N * N * 4);
    out.normal.resize((size_t)N * N * 4);
    // Height blur for AO (separable box, wrap)
    std::vector<float> h((size_t)N * N), tmp((size_t)N * N), blur((size_t)N * N);
    for (size_t i = 0; i < h.size(); i++) h[i] = px[i].h;
    const int R = 4;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float s = 0; for (int k = -R; k <= R; k++) s += h[(size_t)y * N + ((x + k + N) % N)];
            tmp[(size_t)y * N + x] = s / (2 * R + 1);
        }
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float s = 0; for (int k = -R; k <= R; k++) s += tmp[(size_t)((y + k + N) % N) * N + x];
            blur[(size_t)y * N + x] = s / (2 * R + 1);
        }
    float strength = kDefs[id].normalStrength * (N / 512.0f);
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            size_t i = (size_t)y * N + x;
            const Pixel& p = px[i];
            auto H = [&](int xx, int yy) { return h[(size_t)((yy + N) % N) * N + ((xx + N) % N)]; };
            float dx = (H(x + 1, y) - H(x - 1, y)) * strength;
            float dy = (H(x, y + 1) - H(x, y - 1)) * strength;
            Vector3 n = Vector3Normalize(Vector3{ -dx, -dy, 1.0f });
            float ao = Saturate(1.0f - (blur[i] - h[i]) * 2.5f);
            out.albedo[i * 4 + 0] = (uint8_t)(Saturate(p.r) * 255.0f);
            out.albedo[i * 4 + 1] = (uint8_t)(Saturate(p.g) * 255.0f);
            out.albedo[i * 4 + 2] = (uint8_t)(Saturate(p.b) * 255.0f);
            out.albedo[i * 4 + 3] = (uint8_t)(Saturate(p.rough) * 255.0f);
            out.normal[i * 4 + 0] = (uint8_t)((n.x * 0.5f + 0.5f) * 255.0f);
            out.normal[i * 4 + 1] = (uint8_t)((n.y * 0.5f + 0.5f) * 255.0f);
            out.normal[i * 4 + 2] = (uint8_t)((n.z * 0.5f + 0.5f) * 255.0f);
            out.normal[i * 4 + 3] = (uint8_t)(ao * 255.0f);
        }
}

TexSet g_sets[TX_COUNT];
Texture2D g_white{}, g_flatNormal{};

Texture2D UploadRGBA(std::vector<uint8_t>& data, int N) {
    Image img{ data.data(), N, N, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 };
    Texture2D t = LoadTextureFromImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_ANISOTROPIC_8X);
    SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    return t;
}

Texture2D SolidTex(Color c) {
    Image img = GenImageColor(4, 4, c);
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureWrap(t, TEXTURE_WRAP_REPEAT);
    return t;
}

}  // namespace

void GenerateTextureSets(int size, void (*progress)(float, const char*)) {
    std::vector<CpuTex> cpu(TX_COUNT);
    std::atomic<int> next{ 0 };
    std::atomic<int> done{ 0 };
    std::vector<bool> overridden(TX_COUNT, false);
    for (int i = 0; i < TX_COUNT; i++)
        overridden[i] = FileExists(TextFormat("assets/textures/%s_albedo.png", kDefs[i].name));
    unsigned hw = std::thread::hardware_concurrency();
    int nThreads = (int)(hw ? (hw > 8 ? 8 : hw) : 4);
    std::vector<std::thread> threads;
    for (int t = 0; t < nThreads; t++)
        threads.emplace_back([&]() {
            for (;;) {
                int i = next.fetch_add(1);
                if (i >= TX_COUNT) break;
                if (!overridden[i]) GenerateCpu(i, size, cpu[i]);
                done.fetch_add(1);
            }
        });
    // Keep the window responsive while workers run.
    while (done.load() < TX_COUNT) {
        if (progress) progress((float)done.load() / TX_COUNT, "weathering surfaces");
        else WaitTime(0.01);
    }
    for (auto& th : threads) th.join();
    for (int i = 0; i < TX_COUNT; i++) {
        g_sets[i].name = kDefs[i].name;
        if (overridden[i]) {
            g_sets[i].albedo = LoadTexture(TextFormat("assets/textures/%s_albedo.png", kDefs[i].name));
            GenTextureMipmaps(&g_sets[i].albedo);
            SetTextureFilter(g_sets[i].albedo, TEXTURE_FILTER_ANISOTROPIC_8X);
            const char* np = TextFormat("assets/textures/%s_normal.png", kDefs[i].name);
            g_sets[i].normal = FileExists(np) ? LoadTexture(np) : GetFlatNormalTexture();
            if (g_sets[i].normal.id != GetFlatNormalTexture().id) {
                GenTextureMipmaps(&g_sets[i].normal);
                SetTextureFilter(g_sets[i].normal, TEXTURE_FILTER_ANISOTROPIC_8X);
            }
        } else {
            g_sets[i].albedo = UploadRGBA(cpu[i].albedo, size);
            g_sets[i].normal = UploadRGBA(cpu[i].normal, size);
        }
    }
}

const TexSet& GetTexSet(int id) { return g_sets[id]; }

void UnloadTextureSets() {
    for (auto& s : g_sets) {
        if (s.albedo.id) UnloadTexture(s.albedo);
        if (s.normal.id && s.normal.id != g_flatNormal.id) UnloadTexture(s.normal);
    }
}

Texture2D GetWhiteTexture() {
    if (!g_white.id) g_white = SolidTex(Color{ 255, 255, 255, 255 });
    return g_white;
}
Texture2D GetFlatNormalTexture() {
    if (!g_flatNormal.id) g_flatNormal = SolidTex(Color{ 128, 128, 255, 255 });
    return g_flatNormal;
}

Texture2D MakeTextTexture(const char* text, Font font, float fontSize, Color fg, Color bg, int padX, int padY, float spacing) {
    Vector2 sz = MeasureTextEx(font, text, fontSize, spacing);
    int w = (int)sz.x + padX * 2, h = (int)sz.y + padY * 2;
    RenderTexture2D rt = LoadRenderTexture(w, h);
    BeginTextureMode(rt);
    ClearBackground(bg);
    DrawTextEx(font, text, Vector2{ (float)padX, (float)padY }, fontSize, spacing, fg);
    EndTextureMode();
    Image img = LoadImageFromTexture(rt.texture);
    ImageFlipVertical(&img);
    Texture2D t = LoadTextureFromImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(t, TEXTURE_WRAP_CLAMP);
    UnloadImage(img);
    UnloadRenderTexture(rt);
    return t;
}

Texture2D MakeRadialTexture(int size, float softness) {
    Image img = GenImageColor(size, size, BLANK);
    Color* px = (Color*)img.data;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            float dx = (x + 0.5f) / size * 2 - 1, dy = (y + 0.5f) / size * 2 - 1;
            float d = sqrtf(dx * dx + dy * dy);
            float a = Saturate((1.0f - d) / softness);
            a = a * a * (3 - 2 * a);
            px[y * size + x] = Color{ 255, 255, 255, (unsigned char)(a * 255) };
        }
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(t, TEXTURE_WRAP_CLAMP);
    return t;
}
