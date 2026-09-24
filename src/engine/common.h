// Shared math, hashing and noise helpers used by every engine and game module.
#pragma once

#include <raylib.h>
#include <raymath.h>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Scalar helpers
// ---------------------------------------------------------------------------
inline float Saturate(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
inline float SmoothStep(float a, float b, float x) {
    float t = Saturate((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}
inline float EaseInOut(float t) { t = Saturate(t); return t * t * (3.0f - 2.0f * t); }
inline float EaseOutCubic(float t) { t = Saturate(t); float u = 1.0f - t; return 1.0f - u * u * u; }
inline float EaseInCubic(float t) { t = Saturate(t); return t * t * t; }
inline float Damp(float current, float target, float lambda, float dt) {
    return Lerp(current, target, 1.0f - expf(-lambda * dt));
}
inline Vector3 DampV(Vector3 c, Vector3 t, float lambda, float dt) {
    float k = 1.0f - expf(-lambda * dt);
    return Vector3Lerp(c, t, k);
}
inline float WrapAngle(float a) {  // radians to [-pi, pi]
    while (a > PI) a -= 2.0f * PI;
    while (a < -PI) a += 2.0f * PI;
    return a;
}
inline float DampAngle(float c, float t, float lambda, float dt) {
    return c + WrapAngle(t - c) * (1.0f - expf(-lambda * dt));
}
inline Color ColorLerp3(Color a, Color b, float t) {
    return Color{ (unsigned char)Lerp(a.r, b.r, t), (unsigned char)Lerp(a.g, b.g, t),
                  (unsigned char)Lerp(a.b, b.b, t), (unsigned char)Lerp(a.a, b.a, t) };
}
inline Vector3 V3(float x, float y, float z) { return Vector3{ x, y, z }; }
inline Vector3 YawDir(float yaw) { return Vector3{ sinf(yaw), 0.0f, cosf(yaw) }; }
inline Vector3 DirFromYawPitch(float yaw, float pitch) {
    return Vector3{ sinf(yaw) * cosf(pitch), sinf(pitch), cosf(yaw) * cosf(pitch) };
}
inline float YawOf(Vector3 d) { return atan2f(d.x, d.z); }

// ---------------------------------------------------------------------------
// Deterministic hashing / RNG
// ---------------------------------------------------------------------------
inline uint32_t HashU32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return x;
}
inline uint32_t Hash2i(int x, int y, uint32_t seed = 0) {
    return HashU32((uint32_t)x * 0x8da6b343U ^ (uint32_t)y * 0xd8163841U ^ seed * 0xcb1ab31fU);
}
inline float Hash01(uint32_t h) { return (HashU32(h) & 0xffffff) / 16777216.0f; }
inline float Hash2f(int x, int y, uint32_t seed = 0) { return (Hash2i(x, y, seed) & 0xffffff) / 16777216.0f; }

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed = 0x9E3779B97F4A7C15ULL) : s(seed ? seed : 1) {}
    uint32_t Next() {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        return (uint32_t)(s >> 11);
    }
    float F() { return (Next() & 0xffffff) / 16777216.0f; }
    float Range(float a, float b) { return a + (b - a) * F(); }
    int RangeI(int a, int b) { return a + (int)(Next() % (uint32_t)(b - a + 1)); }
    float Signed() { return F() * 2.0f - 1.0f; }
    bool Chance(float p) { return F() < p; }
};

// Global, non-deterministic convenience RNG for effects.
Rng& FxRng();
inline float Frand(float a, float b) { return FxRng().Range(a, b); }

// ---------------------------------------------------------------------------
// Value / gradient noise (optionally tileable with an integer period)
// ---------------------------------------------------------------------------
inline float Fade5(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }

inline float GradNoise2(float x, float y, int period = 0, uint32_t seed = 0) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float xf = x - xi, yf = y - yi;
    auto grad = [&](int ix, int iy, float dx, float dy) {
        if (period > 0) { ix = ((ix % period) + period) % period; iy = ((iy % period) + period) % period; }
        uint32_t h = Hash2i(ix, iy, seed);
        float a = (h & 0xffff) / 65536.0f * 6.2831853f;
        return cosf(a) * dx + sinf(a) * dy;
    };
    float n00 = grad(xi, yi, xf, yf);
    float n10 = grad(xi + 1, yi, xf - 1, yf);
    float n01 = grad(xi, yi + 1, xf, yf - 1);
    float n11 = grad(xi + 1, yi + 1, xf - 1, yf - 1);
    float u = Fade5(xf), v = Fade5(yf);
    return Lerp(Lerp(n00, n10, u), Lerp(n01, n11, u), v) * 1.41f;  // ~[-1,1]
}

inline float Fbm2(float x, float y, int octaves, int period = 0, uint32_t seed = 0,
                  float lac = 2.0f, float gain = 0.5f) {
    float sum = 0, amp = 0.5f, norm = 0;
    for (int i = 0; i < octaves; i++) {
        sum += GradNoise2(x, y, period, seed + i * 131) * amp;
        norm += amp;
        x *= lac; y *= lac;
        if (period > 0) period = (int)(period * lac);
        amp *= gain;
    }
    return sum / norm;
}

inline float Ridged2(float x, float y, int octaves, int period = 0, uint32_t seed = 0) {
    float sum = 0, amp = 0.5f, norm = 0;
    for (int i = 0; i < octaves; i++) {
        float n = 1.0f - fabsf(GradNoise2(x, y, period, seed + i * 71));
        sum += n * n * amp; norm += amp;
        x *= 2; y *= 2; if (period > 0) period *= 2; amp *= 0.5f;
    }
    return sum / norm;
}

// Worley / cellular noise; returns F1 distance and writes F2 and cell id.
inline float Worley2(float x, float y, int period, uint32_t seed, float* f2 = nullptr, uint32_t* cellId = nullptr) {
    int xi = (int)floorf(x), yi = (int)floorf(y);
    float best = 1e9f, second = 1e9f; uint32_t bestId = 0;
    for (int oy = -1; oy <= 1; oy++)
        for (int ox = -1; ox <= 1; ox++) {
            int cx = xi + ox, cy = yi + oy;
            int wx = cx, wy = cy;
            if (period > 0) { wx = ((cx % period) + period) % period; wy = ((cy % period) + period) % period; }
            uint32_t h = Hash2i(wx, wy, seed);
            float px = cx + (h & 0xffff) / 65536.0f;
            float py = cy + (h >> 16) / 65536.0f;
            float d = sqrtf((px - x) * (px - x) + (py - y) * (py - y));
            if (d < best) { second = best; best = d; bestId = h; }
            else if (d < second) second = d;
        }
    if (f2) *f2 = second;
    if (cellId) *cellId = bestId;
    return best;
}

// Transform helpers
inline Matrix MatTRS(Vector3 t, Vector3 rotEulerRad, Vector3 s) {
    Matrix S = MatrixScale(s.x, s.y, s.z);
    Matrix R = MatrixRotateXYZ(rotEulerRad);
    Matrix T = MatrixTranslate(t.x, t.y, t.z);
    return MatrixMultiply(MatrixMultiply(S, R), T);
}
// Yaw (Y) then pitch (X) then roll (Z) — the order used by all entities.
inline Matrix MatYPR(float yaw, float pitch, float roll) {
    return MatrixMultiply(MatrixMultiply(MatrixRotateZ(roll), MatrixRotateX(pitch)), MatrixRotateY(yaw));
}
inline Matrix MatPose(Vector3 pos, float yaw, float pitch = 0, float roll = 0, Vector3 scale = { 1, 1, 1 }) {
    return MatrixMultiply(MatrixMultiply(MatrixScale(scale.x, scale.y, scale.z), MatYPR(yaw, pitch, roll)),
                          MatrixTranslate(pos.x, pos.y, pos.z));
}
inline Vector3 XfPoint(const Matrix& m, Vector3 p) { return Vector3Transform(p, m); }
inline Vector3 XfDir(const Matrix& m, Vector3 d) {
    return Vector3{ m.m0 * d.x + m.m4 * d.y + m.m8 * d.z, m.m1 * d.x + m.m5 * d.y + m.m9 * d.z,
                    m.m2 * d.x + m.m6 * d.y + m.m10 * d.z };
}

// Catmull-Rom spline helpers (used by cutscene camera paths and the road).
inline Vector3 CatmullRom(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t) {
    float t2 = t * t, t3 = t2 * t;
    return Vector3Scale(Vector3Add(Vector3Add(Vector3Scale(p1, 2.0f),
        Vector3Scale(Vector3Subtract(p2, p0), t)),
        Vector3Add(Vector3Scale(Vector3Add(Vector3Subtract(Vector3Scale(p0, 2.0f), Vector3Scale(p1, 5.0f)),
                                           Vector3Subtract(Vector3Scale(p2, 4.0f), p3)), t2),
                   Vector3Scale(Vector3Add(Vector3Subtract(Vector3Scale(p1, 3.0f), p0),
                                           Vector3Subtract(p3, Vector3Scale(p2, 3.0f))), t3))), 0.5f);
}

std::string TrimStr(const std::string& s);
std::vector<std::string> SplitStr(const std::string& s, char sep);
