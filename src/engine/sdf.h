// Signed-distance sculpting + marching cubes meshing (characters, organic props).
#pragma once
#include "common.h"
#include "mesh_builder.h"
#include <functional>

namespace sdf {

inline float Sphere(Vector3 p, Vector3 c, float r) { return Vector3Distance(p, c) - r; }
// Capsule / round cone between a and b with radii ra, rb
float RoundCone(Vector3 p, Vector3 a, Vector3 b, float ra, float rb);
inline float Capsule(Vector3 p, Vector3 a, Vector3 b, float r) { return RoundCone(p, a, b, r, r); }
float Ellipsoid(Vector3 p, Vector3 c, Vector3 r);
float Box(Vector3 p, Vector3 c, Vector3 half, float round = 0.0f);
inline float Union(float a, float b) { return fminf(a, b); }
inline float SUnion(float a, float b, float k) {
    float h = Saturate(0.5f + 0.5f * (b - a) / k);
    return Lerp(b, a, h) - k * h * (1.0f - h);
}
inline float SSub(float a, float b, float k) {   // a minus b, smooth
    float h = Saturate(0.5f - 0.5f * (a + b) / k);
    return Lerp(a, -b, h) + k * h * (1.0f - h);
}
inline float SInter(float a, float b, float k) {
    float h = Saturate(0.5f - 0.5f * (b - a) / k);
    return Lerp(b, a, h) + k * h * (1.0f - h);
}

using Field = std::function<float(Vector3)>;

// Polygonise `f` inside [bmin, bmax] with the given cell size into `mb`.
// `color` sets vertex tint; `displace` adds optional surface noise (wrinkles).
// Work is split across `threads` worker threads (0 = all cores); the output is identical.
void Polygonise(MeshBuilder& mb, const Field& f, Vector3 bmin, Vector3 bmax, float cell, int threads = 0);

}  // namespace sdf
