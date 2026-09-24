#include "mesh_builder.h"
#include <cstring>
#include <unordered_map>

static std::vector<std::unique_ptr<MeshAsset>> g_meshes;
static std::vector<std::unique_ptr<Model3D>> g_models;

Vector3 MeshBuilder::N(Vector3 n) const {
    if (!xf) return n;
    static Matrix lastM{}; static Matrix normalM = MatrixIdentity();
    if (memcmp(&lastM, xf, sizeof(Matrix)) != 0) {
        lastM = *xf;
        normalM = MatrixTranspose(MatrixInvert(*xf));
    }
    Vector3 r = XfDir(normalM, n);
    return Vector3Normalize(r);
}

void MeshBuilder::Vert(Vector3 p, Vector3 n, Vector2 t) {
    Vector3 wp = P(p), wn = N(n);
    pos.push_back(wp.x); pos.push_back(wp.y); pos.push_back(wp.z);
    nrm.push_back(wn.x); nrm.push_back(wn.y); nrm.push_back(wn.z);
    uv.push_back(t.x); uv.push_back(t.y);
    col.push_back(color.r); col.push_back(color.g); col.push_back(color.b); col.push_back(color.a);
}

void MeshBuilder::TriN(Vector3 a, Vector3 b, Vector3 c, Vector3 na, Vector3 nb, Vector3 nc, Vector2 ta, Vector2 tb, Vector2 tc) {
    Vert(a, na, ta); Vert(b, nb, tb); Vert(c, nc, tc);
}

void MeshBuilder::Tri(Vector3 a, Vector3 b, Vector3 c) {
    Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
    TriN(a, b, c, n, n, n, { a.x + a.z, a.y }, { b.x + b.z, b.y }, { c.x + c.z, c.y });
}

void MeshBuilder::Quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector2 ta, Vector2 tb, Vector2 tc, Vector2 td) {
    Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
    TriN(a, b, c, n, n, n, ta, tb, tc);
    TriN(a, c, d, n, n, n, ta, tc, td);
}

// Emit a triangle whose winding is corrected so it faces `n`.
static void OrientedTri(MeshBuilder& mb, Vector3 a, Vector3 b, Vector3 c, Vector3 n) {
    Vector3 cr = Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a));
    if (Vector3DotProduct(cr, n) < 0) { Vector3 t = b; b = c; c = t; }
    mb.TriN(a, b, c, n, n, n, { a.x + a.z, a.y }, { b.x + b.z, b.y }, { c.x + c.z, c.y });
}
static void OrientedQuad(MeshBuilder& mb, Vector3 a, Vector3 b, Vector3 c, Vector3 d, Vector3 n) {
    OrientedTri(mb, a, b, c, n);
    OrientedTri(mb, a, c, d, n);
}

void MeshBuilder::Box(Vector3 c, Vector3 s, float bevel) {
    float hx = s.x * 0.5f, hy = s.y * 0.5f, hz = s.z * 0.5f;
    float mn = fminf(hx, fminf(hy, hz));
    float b = fminf(bevel, mn * 0.9f);
    if (b <= 0.0005f) {
        Vector3 p[8];
        for (int i = 0; i < 8; i++)
            p[i] = { c.x + ((i & 1) ? hx : -hx), c.y + ((i & 2) ? hy : -hy), c.z + ((i & 4) ? hz : -hz) };
        OrientedQuad(*this, p[1], p[3], p[7], p[5], { 1, 0, 0 });
        OrientedQuad(*this, p[0], p[4], p[6], p[2], { -1, 0, 0 });
        OrientedQuad(*this, p[2], p[6], p[7], p[3], { 0, 1, 0 });
        OrientedQuad(*this, p[0], p[1], p[5], p[4], { 0, -1, 0 });
        OrientedQuad(*this, p[4], p[5], p[7], p[6], { 0, 0, 1 });
        OrientedQuad(*this, p[0], p[2], p[3], p[1], { 0, 0, -1 });
        return;
    }
    float h[3] = { hx, hy, hz };
    // Main faces
    for (int axis = 0; axis < 3; axis++)
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            int a1 = (axis + 1) % 3, a2 = (axis + 2) % 3;
            Vector3 q[4];
            for (int k = 0; k < 4; k++) {
                float v[3];
                v[axis] = sgn * h[axis];
                v[a1] = ((k == 1 || k == 2) ? 1 : -1) * (h[a1] - b);
                v[a2] = ((k >= 2) ? 1 : -1) * (h[a2] - b);
                q[k] = { c.x + v[0], c.y + v[1], c.z + v[2] };
            }
            float n[3] = { 0, 0, 0 }; n[axis] = (float)sgn;
            OrientedQuad(*this, q[0], q[1], q[2], q[3], { n[0], n[1], n[2] });
        }
    // Edges: between axis A (sign sa) and axis B (sign sb), running along the third axis
    for (int A = 0; A < 3; A++)
        for (int B = A + 1; B < 3; B++) {
            int L = 3 - A - B;
            for (int sa = -1; sa <= 1; sa += 2)
                for (int sb = -1; sb <= 1; sb += 2) {
                    Vector3 q[4];
                    for (int k = 0; k < 4; k++) {
                        float v[3];
                        bool onA = (k < 2);
                        v[A] = sa * (onA ? h[A] : h[A] - b);
                        v[B] = sb * (onA ? h[B] - b : h[B]);
                        v[L] = ((k == 0 || k == 3) ? -1 : 1) * (h[L] - b);
                        q[k] = { c.x + v[0], c.y + v[1], c.z + v[2] };
                    }
                    float n[3] = { 0, 0, 0 }; n[A] = (float)sa; n[B] = (float)sb;
                    OrientedQuad(*this, q[0], q[1], q[2], q[3], Vector3Normalize({ n[0], n[1], n[2] }));
                }
        }
    // Corners
    for (int i = 0; i < 8; i++) {
        float sx = (i & 1) ? 1.f : -1.f, sy = (i & 2) ? 1.f : -1.f, sz = (i & 4) ? 1.f : -1.f;
        Vector3 a = { c.x + sx * hx, c.y + sy * (hy - b), c.z + sz * (hz - b) };
        Vector3 bb = { c.x + sx * (hx - b), c.y + sy * hy, c.z + sz * (hz - b) };
        Vector3 cc = { c.x + sx * (hx - b), c.y + sy * (hy - b), c.z + sz * hz };
        OrientedTri(*this, a, bb, cc, Vector3Normalize({ sx, sy, sz }));
    }
}

void MeshBuilder::BoxRot(Vector3 center, Vector3 size, Vector3 eulerDeg, float bevel) {
    Matrix local = MatrixMultiply(MatrixRotateXYZ(Vector3Scale(eulerDeg, DEG2RAD)), MatrixTranslate(center.x, center.y, center.z));
    Matrix combined = xf ? MatrixMultiply(local, *xf) : local;
    const Matrix* saved = xf;
    xf = &combined;
    Box({ 0, 0, 0 }, size, bevel);
    xf = saved;
}

void MeshBuilder::BoxUV(Vector3 c, Vector3 s) {
    float hx = s.x * 0.5f, hy = s.y * 0.5f, hz = s.z * 0.5f;
    // front (+Z) face gets full 0..1 UV (text reads left->right when viewed from +Z)
    auto face = [&](Vector3 a, Vector3 b2, Vector3 c2, Vector3 d, Vector3 n) {
        Vector3 cr = Vector3CrossProduct(Vector3Subtract(b2, a), Vector3Subtract(c2, a));
        if (Vector3DotProduct(cr, n) < 0) { Vector3 t = b2; b2 = d; d = t; }
        TriN(a, b2, c2, n, n, n, { 0, 1 }, { 1, 1 }, { 1, 0 });
        TriN(a, c2, d, n, n, n, { 0, 1 }, { 1, 0 }, { 0, 0 });
    };
    face({ c.x - hx, c.y - hy, c.z + hz }, { c.x + hx, c.y - hy, c.z + hz }, { c.x + hx, c.y + hy, c.z + hz }, { c.x - hx, c.y + hy, c.z + hz }, { 0, 0, 1 });
    face({ c.x + hx, c.y - hy, c.z - hz }, { c.x - hx, c.y - hy, c.z - hz }, { c.x - hx, c.y + hy, c.z - hz }, { c.x + hx, c.y + hy, c.z - hz }, { 0, 0, -1 });
}

static void Basis(Vector3 axis, Vector3& u, Vector3& v) {
    Vector3 ref = fabsf(axis.y) < 0.95f ? Vector3{ 0, 1, 0 } : Vector3{ 1, 0, 0 };
    u = Vector3Normalize(Vector3CrossProduct(axis, ref));
    v = Vector3CrossProduct(u, axis);
}

void MeshBuilder::Cylinder(Vector3 a, Vector3 b, float ra, float rb, int seg, bool caps) {
    Vector3 axisV = Vector3Subtract(b, a);
    float len = Vector3Length(axisV);
    if (len < 1e-5f) return;
    Vector3 axis = Vector3Scale(axisV, 1.0f / len);
    Vector3 u, v; Basis(axis, u, v);
    float slope = (ra - rb) / len;
    for (int i = 0; i < seg; i++) {
        float a0 = (float)i / seg * 2 * PI, a1 = (float)(i + 1) / seg * 2 * PI;
        Vector3 d0 = Vector3Add(Vector3Scale(u, cosf(a0)), Vector3Scale(v, sinf(a0)));
        Vector3 d1 = Vector3Add(Vector3Scale(u, cosf(a1)), Vector3Scale(v, sinf(a1)));
        Vector3 n0 = Vector3Normalize(Vector3Add(d0, Vector3Scale(axis, slope)));
        Vector3 n1 = Vector3Normalize(Vector3Add(d1, Vector3Scale(axis, slope)));
        Vector3 p00 = Vector3Add(a, Vector3Scale(d0, ra)), p01 = Vector3Add(a, Vector3Scale(d1, ra));
        Vector3 p10 = Vector3Add(b, Vector3Scale(d0, rb)), p11 = Vector3Add(b, Vector3Scale(d1, rb));
        float u0 = (float)i / seg, u1 = (float)(i + 1) / seg;
        TriN(p00, p11, p01, n0, n1, n1, { u0, 0 }, { u1, len }, { u1, 0 });
        TriN(p00, p10, p11, n0, n0, n1, { u0, 0 }, { u0, len }, { u1, len });
        if (caps) {
            if (ra > 0) OrientedTri(*this, a, p00, p01, Vector3Negate(axis));
            if (rb > 0) OrientedTri(*this, b, p10, p11, axis);
        }
    }
}

void MeshBuilder::Ellipsoid(Vector3 c, Vector3 r, int rings, int segs) {
    auto pt = [&](int ri, int si, Vector3& p, Vector3& n) {
        float th = (float)ri / rings * PI;
        float ph = (float)si / segs * 2 * PI;
        Vector3 d = { sinf(th) * cosf(ph), cosf(th), sinf(th) * sinf(ph) };
        p = { c.x + d.x * r.x, c.y + d.y * r.y, c.z + d.z * r.z };
        n = Vector3Normalize({ d.x / r.x, d.y / r.y, d.z / r.z });
    };
    for (int i = 0; i < rings; i++)
        for (int j = 0; j < segs; j++) {
            Vector3 p00, p01, p10, p11, n00, n01, n10, n11;
            pt(i, j, p00, n00); pt(i, j + 1, p01, n01); pt(i + 1, j, p10, n10); pt(i + 1, j + 1, p11, n11);
            Vector2 t00 = { (float)j / segs, (float)i / rings }, t01 = { (float)(j + 1) / segs, (float)i / rings };
            Vector2 t10 = { (float)j / segs, (float)(i + 1) / rings }, t11 = { (float)(j + 1) / segs, (float)(i + 1) / rings };
            if (i != 0) TriN(p00, p01, p11, n00, n01, n11, t00, t01, t11);
            if (i != rings - 1) TriN(p00, p11, p10, n00, n11, n10, t00, t11, t10);
        }
}

void MeshBuilder::Lathe(Vector3 base, const std::vector<Vector2>& prof, int segs, bool capBottom, bool capTop) {
    int n = (int)prof.size();
    if (n < 2) return;
    std::vector<Vector2> pn(n);
    for (int i = 0; i < n; i++) {
        Vector2 t{ 0, 0 };
        if (i > 0) t = Vector2Add(t, Vector2Normalize(Vector2Subtract(prof[i], prof[i - 1])));
        if (i < n - 1) t = Vector2Add(t, Vector2Normalize(Vector2Subtract(prof[i + 1], prof[i])));
        t = Vector2Normalize(t);
        pn[i] = { t.y, -t.x };  // outward (radius, y)
    }
    for (int s = 0; s < segs; s++) {
        float a0 = (float)s / segs * 2 * PI, a1 = (float)(s + 1) / segs * 2 * PI;
        float c0 = cosf(a0), s0 = sinf(a0), c1 = cosf(a1), s1 = sinf(a1);
        for (int i = 0; i < n - 1; i++) {
            Vector3 p00 = { base.x + prof[i].x * c0, base.y + prof[i].y, base.z + prof[i].x * s0 };
            Vector3 p01 = { base.x + prof[i].x * c1, base.y + prof[i].y, base.z + prof[i].x * s1 };
            Vector3 p10 = { base.x + prof[i + 1].x * c0, base.y + prof[i + 1].y, base.z + prof[i + 1].x * s0 };
            Vector3 p11 = { base.x + prof[i + 1].x * c1, base.y + prof[i + 1].y, base.z + prof[i + 1].x * s1 };
            Vector3 n00 = { pn[i].x * c0, pn[i].y, pn[i].x * s0 }, n01 = { pn[i].x * c1, pn[i].y, pn[i].x * s1 };
            Vector3 n10 = { pn[i + 1].x * c0, pn[i + 1].y, pn[i + 1].x * s0 }, n11 = { pn[i + 1].x * c1, pn[i + 1].y, pn[i + 1].x * s1 };
            // winding: outward facing
            TriN(p00, p11, p01, n00, n11, n01);
            TriN(p00, p10, p11, n00, n10, n11);
        }
        if (capBottom && prof[0].x > 0.0001f) {
            Vector3 cb = { base.x, base.y + prof[0].y, base.z };
            OrientedTri(*this, cb, { base.x + prof[0].x * c0, cb.y, base.z + prof[0].x * s0 },
                        { base.x + prof[0].x * c1, cb.y, base.z + prof[0].x * s1 }, { 0, -1, 0 });
        }
        if (capTop && prof[n - 1].x > 0.0001f) {
            Vector3 ct = { base.x, base.y + prof[n - 1].y, base.z };
            OrientedTri(*this, ct, { base.x + prof[n - 1].x * c0, ct.y, base.z + prof[n - 1].x * s0 },
                        { base.x + prof[n - 1].x * c1, ct.y, base.z + prof[n - 1].x * s1 }, { 0, 1, 0 });
        }
    }
}

void MeshBuilder::Tube(const std::vector<Vector3>& pts, const std::vector<float>& radii, int segs, bool caps) {
    int n = (int)pts.size();
    if (n < 2) return;
    std::vector<Vector3> U(n), V(n), T(n);
    for (int i = 0; i < n; i++) {
        Vector3 t{ 0, 0, 0 };
        if (i > 0) t = Vector3Add(t, Vector3Normalize(Vector3Subtract(pts[i], pts[i - 1])));
        if (i < n - 1) t = Vector3Add(t, Vector3Normalize(Vector3Subtract(pts[i + 1], pts[i])));
        T[i] = Vector3Normalize(t);
    }
    Basis(T[0], U[0], V[0]);
    for (int i = 1; i < n; i++) {  // parallel transport
        Vector3 u = Vector3Subtract(U[i - 1], Vector3Scale(T[i], Vector3DotProduct(U[i - 1], T[i])));
        if (Vector3Length(u) < 1e-4f) Basis(T[i], u, V[i]);
        U[i] = Vector3Normalize(u);
        V[i] = Vector3CrossProduct(U[i], T[i]);
    }
    for (int i = 0; i < n - 1; i++)
        for (int s = 0; s < segs; s++) {
            float a0 = (float)s / segs * 2 * PI, a1 = (float)(s + 1) / segs * 2 * PI;
            auto ring = [&](int k, float a) {
                return Vector3Add(Vector3Scale(U[k], cosf(a)), Vector3Scale(V[k], sinf(a)));
            };
            Vector3 d00 = ring(i, a0), d01 = ring(i, a1), d10 = ring(i + 1, a0), d11 = ring(i + 1, a1);
            Vector3 p00 = Vector3Add(pts[i], Vector3Scale(d00, radii[i])), p01 = Vector3Add(pts[i], Vector3Scale(d01, radii[i]));
            Vector3 p10 = Vector3Add(pts[i + 1], Vector3Scale(d10, radii[i + 1])), p11 = Vector3Add(pts[i + 1], Vector3Scale(d11, radii[i + 1]));
            Vector3 cr = Vector3CrossProduct(Vector3Subtract(p01, p00), Vector3Subtract(p11, p00));
            if (Vector3DotProduct(cr, d00) >= 0) {
                TriN(p00, p01, p11, d00, d01, d11); TriN(p00, p11, p10, d00, d11, d10);
            } else {
                TriN(p00, p11, p01, d00, d11, d01); TriN(p00, p10, p11, d00, d10, d11);
            }
        }
    if (caps) {
        for (int s = 0; s < segs; s++) {
            float a0 = (float)s / segs * 2 * PI, a1 = (float)(s + 1) / segs * 2 * PI;
            for (int e = 0; e < 2; e++) {
                int k = e == 0 ? 0 : n - 1;
                if (radii[k] <= 0.0001f) continue;
                Vector3 q0 = Vector3Add(pts[k], Vector3Scale(Vector3Add(Vector3Scale(U[k], cosf(a0)), Vector3Scale(V[k], sinf(a0))), radii[k]));
                Vector3 q1 = Vector3Add(pts[k], Vector3Scale(Vector3Add(Vector3Scale(U[k], cosf(a1)), Vector3Scale(V[k], sinf(a1))), radii[k]));
                OrientedTri(*this, pts[k], q0, q1, e == 0 ? Vector3Negate(T[k]) : T[k]);
            }
        }
    }
}

void MeshBuilder::Panel(Vector3 c, Vector3 r, Vector3 u, bool doubleSided) {
    Vector3 a = Vector3Subtract(Vector3Subtract(c, r), u);
    Vector3 b = Vector3Subtract(Vector3Add(c, r), u);
    Vector3 cc = Vector3Add(Vector3Add(c, r), u);
    Vector3 d = Vector3Add(Vector3Subtract(c, r), u);
    Quad(a, b, cc, d, { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 });
    if (doubleSided) Quad(b, a, d, cc, { 1, 1 }, { 0, 1 }, { 0, 0 }, { 1, 0 });
}

void MeshBuilder::Append(const MeshBuilder& o) {
    pos.insert(pos.end(), o.pos.begin(), o.pos.end());
    nrm.insert(nrm.end(), o.nrm.begin(), o.nrm.end());
    uv.insert(uv.end(), o.uv.begin(), o.uv.end());
    col.insert(col.end(), o.col.begin(), o.col.end());
}

// Vertices are welded (exactly identical position, normal, uv and colour become one vertex)
// and drawn indexed. The triangles and every attribute value are unchanged, so the result
// renders the same, but a vertex shared by N triangles is stored and transformed once instead
// of N times. raylib uses 16-bit indices, so large meshes are split into chunks.
namespace {
struct VKey {
    float p[8];
    uint32_t c;
    bool operator==(const VKey& o) const { return memcmp(this, &o, sizeof(VKey)) == 0; }
};
struct VKeyHash {
    size_t operator()(const VKey& k) const {
        const unsigned char* b = (const unsigned char*)&k;
        uint64_t h = 1469598103934665603ull;
        for (size_t i = 0; i < sizeof(VKey); i++) { h ^= b[i]; h *= 1099511628211ull; }
        return (size_t)h;
    }
};
}

static unsigned short g_indexedMarker[1] = { 0 };

static Mesh UploadChunk(const std::vector<float>& P, const std::vector<float>& N, const std::vector<float>& T,
                        const std::vector<unsigned char>& C, const std::vector<unsigned short>& I) {
    Mesh m{};
    m.vertexCount = (int)(P.size() / 3);
    m.triangleCount = (int)(I.size() / 3);
    m.vertices = (float*)P.data();
    m.normals = (float*)N.data();
    m.texcoords = (float*)T.data();
    m.colors = (unsigned char*)C.data();
    m.indices = (unsigned short*)I.data();
    UploadMesh(&m, false);
    // The GPU has its own copy; nothing reads mesh data on the CPU afterwards. raylib only checks
    // `indices != NULL` to choose indexed drawing, so it points at a shared marker instead of a copy.
    m.vertices = nullptr; m.normals = nullptr; m.texcoords = nullptr; m.colors = nullptr;
    m.indices = g_indexedMarker;
    return m;
}

MeshAsset* MeshBuilder::Upload() {
    auto ma = std::make_unique<MeshAsset>();
    int vc = (int)VertexCount();
    if (vc == 0) return nullptr;
    // 1. weld
    std::unordered_map<VKey, uint32_t, VKeyHash> map;
    map.reserve((size_t)vc);
    std::vector<uint32_t> uniq;       // source vertex of each unique vertex
    std::vector<uint32_t> idx((size_t)vc);
    for (int i = 0; i < vc; i++) {
        VKey k{};
        memcpy(k.p, &pos[i * 3], 12); memcpy(k.p + 3, &nrm[i * 3], 12); memcpy(k.p + 6, &uv[i * 2], 8);
        memcpy(&k.c, &col[i * 4], 4);
        auto it = map.emplace(k, (uint32_t)uniq.size());
        if (it.second) uniq.push_back((uint32_t)i);
        idx[i] = it.first->second;
    }
    map = {};
    // 2. split into chunks of at most 65535 vertices, keeping triangle order
    std::vector<int> local(uniq.size(), -1);
    std::vector<uint32_t> touched;
    std::vector<float> P, N, T; std::vector<unsigned char> C; std::vector<unsigned short> I;
    bool first = true;
    auto flush = [&]() {
        if (I.empty()) return;
        Mesh m = UploadChunk(P, N, T, C, I);
        if (first) { ma->mesh = m; first = false; } else ma->more.push_back(m);
        for (uint32_t g : touched) local[g] = -1;
        touched.clear(); P.clear(); N.clear(); T.clear(); C.clear(); I.clear();
    };
    for (int t = 0; t + 2 < vc; t += 3) {
        int need = 0;
        for (int k = 0; k < 3; k++) if (local[idx[t + k]] < 0) need++;
        if ((int)(P.size() / 3) + need > 65535) flush();
        for (int k = 0; k < 3; k++) {
            uint32_t g = idx[t + k];
            if (local[g] < 0) {
                local[g] = (int)(P.size() / 3);
                touched.push_back(g);
                uint32_t s = uniq[g];
                P.insert(P.end(), &pos[s * 3], &pos[s * 3] + 3);
                N.insert(N.end(), &nrm[s * 3], &nrm[s * 3] + 3);
                T.insert(T.end(), &uv[s * 2], &uv[s * 2] + 2);
                C.insert(C.end(), &col[s * 4], &col[s * 4] + 4);
            }
            I.push_back((unsigned short)local[g]);
        }
    }
    flush();
    Vector3 mn{ 1e9f, 1e9f, 1e9f }, mx{ -1e9f, -1e9f, -1e9f };
    for (int i = 0; i < vc; i++) {
        Vector3 p{ pos[i * 3], pos[i * 3 + 1], pos[i * 3 + 2] };
        mn = Vector3Min(mn, p); mx = Vector3Max(mx, p);
    }
    ma->bmin = mn; ma->bmax = mx;
    ma->center = Vector3Scale(Vector3Add(mn, mx), 0.5f);
    ma->radius = Vector3Length(Vector3Subtract(mx, mn)) * 0.5f;
    MeshAsset* raw = ma.get();
    g_meshes.push_back(std::move(ma));
    return raw;
}

ModelBuilder::ModelBuilder() { stack_.push_back(MatrixIdentity()); }

MeshBuilder& ModelBuilder::M(int mat) {
    auto it = groups_.find(mat);
    if (it == groups_.end()) {
        auto mb = std::make_unique<MeshBuilder>();
        it = groups_.emplace(mat, std::move(mb)).first;
    }
    it->second->xf = &stack_.back();
    it->second->color = color_;
    return *it->second;
}
void ModelBuilder::Push() { stack_.push_back(stack_.back()); }
void ModelBuilder::Pop() { if (stack_.size() > 1) stack_.pop_back(); }
void ModelBuilder::Transform(const Matrix& m) { stack_.back() = MatrixMultiply(m, stack_.back()); }
void ModelBuilder::Translate(Vector3 t) { Transform(MatrixTranslate(t.x, t.y, t.z)); }
void ModelBuilder::RotateY(float d) { Transform(MatrixRotateY(d * DEG2RAD)); }
void ModelBuilder::RotateX(float d) { Transform(MatrixRotateX(d * DEG2RAD)); }
void ModelBuilder::RotateZ(float d) { Transform(MatrixRotateZ(d * DEG2RAD)); }
void ModelBuilder::Scale(Vector3 s) { Transform(MatrixScale(s.x, s.y, s.z)); }
void ModelBuilder::SetColor(Color c) { color_ = Color{ c.r, c.g, c.b, color_.a }; }
void ModelBuilder::Flex(float f) { color_.a = (unsigned char)((1.0f - Saturate(f)) * 255); }  // shader reads flex = 1 - alpha

Model3D* ModelBuilder::Build(bool castShadow) {
    auto model = std::make_unique<Model3D>();
    model->castShadow = castShadow;
    for (auto& kv : groups_) {
        MeshAsset* ma = kv.second->Upload();
        if (!ma) continue;
        model->parts.push_back({ ma, kv.first });
        model->bmin = Vector3Min(model->bmin, ma->bmin);
        model->bmax = Vector3Max(model->bmax, ma->bmax);
    }
    if (model->parts.empty()) { model->bmin = model->bmax = { 0, 0, 0 }; }
    model->center = Vector3Scale(Vector3Add(model->bmin, model->bmax), 0.5f);
    model->radius = Vector3Length(Vector3Subtract(model->bmax, model->bmin)) * 0.5f;
    Model3D* raw = model.get();
    g_models.push_back(std::move(model));
    return raw;
}

void UnloadAllMeshes() {
    for (auto& m : g_meshes) {
        m->mesh.indices = nullptr;   // the marker is not owned
        UnloadMesh(m->mesh);
        for (auto& c : m->more) { c.indices = nullptr; UnloadMesh(c); }
    }
    g_meshes.clear();
    g_models.clear();
}

void ModelBuilder::Deform(const std::function<Vector3(Vector3)>& fn, bool flatNormals) {
    for (auto& kv : groups_) {
        MeshBuilder& m = *kv.second;
        size_t n = m.VertexCount();
        for (size_t i = 0; i < n; i++) {
            Vector3 p{ m.pos[i * 3], m.pos[i * 3 + 1], m.pos[i * 3 + 2] };
            p = fn(p);
            m.pos[i * 3] = p.x; m.pos[i * 3 + 1] = p.y; m.pos[i * 3 + 2] = p.z;
        }
        if (!flatNormals) continue;
        for (size_t t = 0; t + 2 < n; t += 3) {
            Vector3 a{ m.pos[t * 3], m.pos[t * 3 + 1], m.pos[t * 3 + 2] };
            Vector3 b{ m.pos[t * 3 + 3], m.pos[t * 3 + 4], m.pos[t * 3 + 5] };
            Vector3 c{ m.pos[t * 3 + 6], m.pos[t * 3 + 7], m.pos[t * 3 + 8] };
            Vector3 nn = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
            // keep the original orientation hemisphere
            Vector3 old{ m.nrm[t * 3], m.nrm[t * 3 + 1], m.nrm[t * 3 + 2] };
            if (Vector3DotProduct(nn, old) < 0) nn = Vector3Negate(nn);
            for (int k = 0; k < 3; k++) { m.nrm[(t + k) * 3] = nn.x; m.nrm[(t + k) * 3 + 1] = nn.y; m.nrm[(t + k) * 3 + 2] = nn.z; }
        }
    }
}

