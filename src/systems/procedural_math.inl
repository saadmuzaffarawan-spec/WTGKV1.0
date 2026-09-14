#pragma once
#include <math.h>

// ============================================================
// PROCEDURAL NOISE & FBM MATH FUNCTIONS
// ============================================================

inline float fract(float x) { return x - floorf(x); }

inline float hash(float x, float y) {
    return fract(sinf(x * 12.9898f + y * 78.233f) * 43758.5453123f);
}

inline float noise(float x, float y) {
    float ix = floorf(x), iy = floorf(y);
    float fx = fract(x), fy = fract(y);
    float ux = fx * fx * (3.0f - 2.0f * fx);
    float uy = fy * fy * (3.0f - 2.0f * fy);
    float a = hash(ix, iy);
    float b = hash(ix + 1.0f, iy);
    float c = hash(ix, iy + 1.0f);
    float d = hash(ix + 1.0f, iy + 1.0f);
    return a + (b - a)*ux + (c - a)*uy + (a - b - c + d)*ux*uy;
}

inline float fbm(float x, float y) {
    float v = 0.0f; float amp = 0.5f;
    for(int i=0; i<4; i++) {
        v += amp * noise(x, y);
        x *= 2.0f; y *= 2.0f; amp *= 0.5f;
    }
    return v;
}

#include <raylib.h>

inline float LerpAngleDeg(float a, float b, float t) {
    float diff = fmodf(b - a + 180.0f, 360.0f);
    if (diff < 0) diff += 360.0f;
    diff -= 180.0f;
    return a + diff * t;
}

inline float Frand(float lo, float hi) {
    return lo + (float)GetRandomValue(0, 10000) / 10000.0f * (hi - lo);
}
