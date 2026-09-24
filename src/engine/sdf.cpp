#include "sdf.h"
#include <algorithm>
#include <thread>
#include <unordered_map>

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

void Polygonise(MeshBuilder& mb, const Field& f, Vector3 bmin, Vector3 bmax, float cell, int threads) {
    int nx = (int)ceilf((bmax.x - bmin.x) / cell) + 1;
    int ny = (int)ceilf((bmax.y - bmin.y) / cell) + 1;
    int nz = (int)ceilf((bmax.z - bmin.z) / cell) + 1;
    std::vector<float> v((size_t)nx * ny * nz);
    auto P = [&](int x, int y, int z) { return Vector3{ bmin.x + x * cell, bmin.y + y * cell, bmin.z + z * cell }; };
    auto I = [&](int x, int y, int z) { return ((size_t)z * ny + y) * nx + x; };
    if (threads <= 0) { unsigned hw = std::thread::hardware_concurrency(); threads = hw ? (int)hw : 4; }
    threads = std::max(1, std::min(threads, nz - 1));
    // Run fn(z0, z1) over [0, n) split into `threads` contiguous slabs of z.
    auto parallel = [&](int n, const std::function<void(int, int, int)>& fn) {
        std::vector<std::thread> ts;
        for (int t = 1; t < threads; t++) ts.emplace_back(fn, t, n * t / threads, n * (t + 1) / threads);
        fn(0, 0, n / threads);
        for (auto& th : ts) th.join();
    };

    // 1. sample the field on the grid. Blocks of B^3 samples that are provably far from the
    //    surface (distance at the block centre > the block's radius, with margin) only need
    //    their sign, so they are filled from one evaluation. Cells with a sign change are always
    //    within a cell of the surface, so their corner values are exact: the mesh is unchanged.
    const int B = 4;
    int bz = (nz + B - 1) / B;
    parallel(bz, [&](int, int b0, int b1) {
        for (int bzI = b0; bzI < b1; bzI++)
            for (int by = 0; by < ny; by += B)
                for (int bx = 0; bx < nx; bx += B) {
                    int z0 = bzI * B, x1 = std::min(bx + B, nx), y1 = std::min(by + B, ny), z1 = std::min(z0 + B, nz);
                    Vector3 c = P(bx, by, z0);
                    Vector3 hi = P(x1 - 1, y1 - 1, z1 - 1);
                    Vector3 mid = Vector3Scale(Vector3Add(c, hi), 0.5f);
                    float rad = Vector3Distance(c, hi) * 0.5f;
                    float d = f(mid);
                    bool far = fabsf(d) > rad * 1.5f + cell * 2.0f;
                    for (int z = z0; z < z1; z++)
                        for (int y = by; y < y1; y++)
                            for (int x = bx; x < x1; x++) v[I(x, y, z)] = far ? d : f(P(x, y, z));
                }
    });

    // 2. marching tetrahedra; each slab emits into its own builder, appended in order afterwards
    //    so the resulting mesh is identical to a single-threaded pass
    std::vector<MeshBuilder> slabs(threads);
    for (auto& sb : slabs) { sb.color = mb.color; sb.xf = mb.xf; }
    // cube corner offsets and the 6 tetrahedra sharing the 0-6 diagonal
    static const int co[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 }, { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 } };
    static const int tets[6][4] = { { 0, 5, 1, 6 }, { 0, 1, 2, 6 }, { 0, 2, 3, 6 }, { 0, 3, 7, 6 }, { 0, 7, 4, 6 }, { 0, 4, 5, 6 } };
    parallel(nz - 1, [&](int slab, int z0, int z1) {
        MeshBuilder& out = slabs[slab];
        auto grad = [&](Vector3 p) {
            float e = cell * 0.5f;
            Vector3 g{ f({ p.x + e, p.y, p.z }) - f({ p.x - e, p.y, p.z }),
                       f({ p.x, p.y + e, p.z }) - f({ p.x, p.y - e, p.z }),
                       f({ p.x, p.y, p.z + e }) - f({ p.x, p.y, p.z - e }) };
            return Vector3Normalize(g);
        };
        // a surface vertex sits on a grid/tet edge and is shared by several triangles:
        // compute its normal once per edge
        std::unordered_map<uint64_t, Vector3> ncache;
        ncache.reserve(4096);
        auto nrmAt = [&](Vector3 p, uint64_t key) {
            auto it = ncache.find(key);
            if (it != ncache.end()) return it->second;
            Vector3 n = grad(p);
            ncache.emplace(key, n);
            return n;
        };
        struct EV { Vector3 p; uint64_t k; };
        auto emit = [&](EV ea, EV eb, EV ec) {
            Vector3 a = ea.p, b = eb.p, c = ec.p;
            Vector3 na = nrmAt(a, ea.k), nb = nrmAt(b, eb.k), nc = nrmAt(c, ec.k);
            Vector3 fn = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
            Vector3 avg = Vector3Add(Vector3Add(na, nb), nc);
            if (Vector3DotProduct(fn, avg) < 0) { std::swap(b, c); std::swap(nb, nc); }
            out.TriN(a, b, c, na, nb, nc, { 0, 0 }, { 0, 0 }, { 0, 0 });   // sculpted surfaces are triplanar-mapped; a constant UV lets shared vertices weld
        };
        for (int z = z0; z < z1; z++)
            for (int y = 0; y < ny - 1; y++)
                for (int x = 0; x < nx - 1; x++) {
                    float cv[8]; Vector3 cp[8]; uint64_t ci[8];
                    bool anyIn = false, anyOut = false;
                    for (int k = 0; k < 8; k++) {
                        cv[k] = v[I(x + co[k][0], y + co[k][1], z + co[k][2])];
                        cp[k] = P(x + co[k][0], y + co[k][1], z + co[k][2]);
                        ci[k] = I(x + co[k][0], y + co[k][1], z + co[k][2]);
                        if (cv[k] < 0) anyIn = true; else anyOut = true;
                    }
                    if (!anyIn || !anyOut) continue;
                    for (auto& t : tets) {
                        int in[4], outv[4], ni = 0, no = 0;
                        for (int k = 0; k < 4; k++) { if (cv[t[k]] < 0) in[ni++] = t[k]; else outv[no++] = t[k]; }
                        if (ni == 0 || no == 0) continue;
                        auto lerpV = [&](int a, int b) {
                            // always interpolate from the lower-indexed corner so neighbouring
                            // tetrahedra produce bit-identical shared vertices (they weld on upload)
                            if (ci[a] > ci[b]) std::swap(a, b);
                            float ta = cv[a] / (cv[a] - cv[b]);
                            uint64_t lo = std::min(ci[a], ci[b]), hi = std::max(ci[a], ci[b]);
                            return EV{ Vector3Lerp(cp[a], cp[b], ta), lo * (uint64_t)v.size() + hi };
                        };
                        if (ni == 1) emit(lerpV(in[0], outv[0]), lerpV(in[0], outv[1]), lerpV(in[0], outv[2]));
                        else if (ni == 3) emit(lerpV(outv[0], in[0]), lerpV(outv[0], in[1]), lerpV(outv[0], in[2]));
                        else {
                            EV a = lerpV(in[0], outv[0]), b = lerpV(in[0], outv[1]);
                            EV c = lerpV(in[1], outv[1]), d = lerpV(in[1], outv[0]);
                            emit(a, b, c); emit(a, c, d);
                        }
                    }
                }
    });
    for (auto& sb : slabs) mb.Append(sb);
}

}  // namespace sdf
