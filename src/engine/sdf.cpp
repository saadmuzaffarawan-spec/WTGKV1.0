#include "sdf.h"

namespace sdf {

float RoundCone(Vector3 p, Vector3 a, Vector3 b, float ra, float rb) {
    Vector3 ba = Vector3Subtract(b, a), pa = Vector3Subtract(p, a);
    float l2 = Vector3DotProduct(ba, ba);
    float t = l2 > 1e-9f ? Saturate(Vector3DotProduct(pa, ba) / l2) : 0.0f;
    Vector3 q = Vector3Subtract(pa, Vector3Scale(ba, t));
    return Vector3Length(q) - Lerp(ra, rb, t);
}

float Ellipsoid(Vector3 p, Vector3 c, Vector3 r) {
    Vector3 d = Vector3Subtract(p, c);
    float k0 = Vector3Length({ d.x / r.x, d.y / r.y, d.z / r.z });
    float k1 = Vector3Length({ d.x / (r.x * r.x), d.y / (r.y * r.y), d.z / (r.z * r.z) });
    return k1 > 1e-9f ? k0 * (k0 - 1.0f) / k1 : -fminf(r.x, fminf(r.y, r.z));
}

float Box(Vector3 p, Vector3 c, Vector3 h, float round) {
    Vector3 q{ fabsf(p.x - c.x) - h.x + round, fabsf(p.y - c.y) - h.y + round, fabsf(p.z - c.z) - h.z + round };
    Vector3 mq{ fmaxf(q.x, 0), fmaxf(q.y, 0), fmaxf(q.z, 0) };
    return Vector3Length(mq) + fminf(fmaxf(q.x, fmaxf(q.y, q.z)), 0.0f) - round;
}

void Polygonise(MeshBuilder& mb, const Field& f, Vector3 bmin, Vector3 bmax, float cell) {
    int nx = (int)ceilf((bmax.x - bmin.x) / cell) + 1;
    int ny = (int)ceilf((bmax.y - bmin.y) / cell) + 1;
    int nz = (int)ceilf((bmax.z - bmin.z) / cell) + 1;
    std::vector<float> v((size_t)nx * ny * nz);
    auto P = [&](int x, int y, int z) { return Vector3{ bmin.x + x * cell, bmin.y + y * cell, bmin.z + z * cell }; };
    auto I = [&](int x, int y, int z) { return ((size_t)z * ny + y) * nx + x; };
    for (int z = 0; z < nz; z++)
        for (int y = 0; y < ny; y++)
            for (int x = 0; x < nx; x++) v[I(x, y, z)] = f(P(x, y, z));

    auto grad = [&](Vector3 p) {
        float e = cell * 0.5f;
        Vector3 g{ f({ p.x + e, p.y, p.z }) - f({ p.x - e, p.y, p.z }),
                   f({ p.x, p.y + e, p.z }) - f({ p.x, p.y - e, p.z }),
                   f({ p.x, p.y, p.z + e }) - f({ p.x, p.y, p.z - e }) };
        return Vector3Normalize(g);
    };
    auto emit = [&](Vector3 a, Vector3 b, Vector3 c) {
        Vector3 na = grad(a), nb = grad(b), nc = grad(c);
        Vector3 fn = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
        Vector3 avg = Vector3Add(Vector3Add(na, nb), nc);
        if (Vector3DotProduct(fn, avg) < 0) { std::swap(b, c); std::swap(nb, nc); }
        mb.TriN(a, b, c, na, nb, nc);
    };
    // cube corner offsets and the 6 tetrahedra sharing the 0-6 diagonal
    static const int co[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } };
    static const int tets[6][4] = { { 0, 5, 1, 6 }, { 0, 1, 2, 6 }, { 0, 2, 3, 6 }, { 0, 3, 7, 6 }, { 0, 7, 4, 6 }, { 0, 4, 5, 6 } };
    for (int z = 0; z < nz - 1; z++)
        for (int y = 0; y < ny - 1; y++)
            for (int x = 0; x < nx - 1; x++) {
                float cv[8]; Vector3 cp[8];
                bool anyIn = false, anyOut = false;
                for (int k = 0; k < 8; k++) {
                    cv[k] = v[I(x + co[k][0], y + co[k][1], z + co[k][2])];
                    cp[k] = P(x + co[k][0], y + co[k][1], z + co[k][2]);
                    if (cv[k] < 0) anyIn = true; else anyOut = true;
                }
                if (!anyIn || !anyOut) continue;
                for (auto& t : tets) {
                    int in[4], out[4], ni = 0, no = 0;
                    for (int k = 0; k < 4; k++) { if (cv[t[k]] < 0) in[ni++] = t[k]; else out[no++] = t[k]; }
                    if (ni == 0 || no == 0) continue;
                    auto lerpV = [&](int a, int b) {
                        float ta = cv[a] / (cv[a] - cv[b]);
                        return Vector3Lerp(cp[a], cp[b], ta);
                    };
                    if (ni == 1) emit(lerpV(in[0], out[0]), lerpV(in[0], out[1]), lerpV(in[0], out[2]));
                    else if (ni == 3) emit(lerpV(out[0], in[0]), lerpV(out[0], in[1]), lerpV(out[0], in[2]));
                    else {
                        Vector3 a = lerpV(in[0], out[0]), b = lerpV(in[0], out[1]);
                        Vector3 c = lerpV(in[1], out[1]), d = lerpV(in[1], out[0]);
                        emit(a, b, c); emit(a, c, d);
                    }
                }
            }
}

}  // namespace sdf
