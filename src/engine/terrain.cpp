#include "terrain.h"
#include "renderer.h"
#include "collision.h"
#include "materials.h"

static Terrain g_terrain;
Terrain& World() { return g_terrain; }

float Terrain::RoadX(float z) {
    // Straight past the station, easing into long curves north (towards the tower) and south.
    float north = 18.0f * SmoothStep(80.0f, 230.0f, z);
    float south = -24.0f * SmoothStep(-110.0f, -300.0f, z);
    return north + south;
}

float Terrain::RoadY(float z) {
    return 0.35f * sinf(z / 95.0f) * SmoothStep(40.0f, 120.0f, fabsf(z));
}

float Terrain::Natural(float x, float z) const {
    float n = Fbm2(x * 0.018f, z * 0.018f, 4, 0, 71) * 1.1f;
    n += Fbm2(x * 0.09f, z * 0.09f, 2, 0, 5) * 0.15f;
    float d = sqrtf(x * x + z * z);
    float hills = SmoothStep(140.0f, 300.0f, d) * (14.0f + Fbm2(x * 0.01f, z * 0.01f, 4, 0, 9) * 16.0f);
    return n + hills;
}

static float PadWeight(const TerrainPad& p, float x, float z) {
    float dx = x - p.c.x, dz = z - p.c.y;
    float cs = cosf(p.yaw), sn = sinf(p.yaw);
    float lx = cs * dx - sn * dz, lz = sn * dx + cs * dz;
    float ex = fmaxf(fabsf(lx) - p.half.x, 0.0f), ez = fmaxf(fabsf(lz) - p.half.y, 0.0f);
    float d = sqrtf(ex * ex + ez * ez);
    return 1.0f - SmoothStep(0.0f, p.margin, d);
}

static float DistToSegment(Vector2 p, Vector2 a, Vector2 b) {
    Vector2 ab = Vector2Subtract(b, a);
    float t = Saturate(Vector2DotProduct(Vector2Subtract(p, a), ab) / fmaxf(Vector2DotProduct(ab, ab), 1e-6f));
    return Vector2Distance(p, Vector2Add(a, Vector2Scale(ab, t)));
}

void Terrain::Build(const std::vector<TerrainPad>& padsIn, const std::vector<Rectangle>& holes, const std::vector<TerrainPath>& paths) {
    holes_ = holes;
    std::vector<TerrainPad> pads = padsIn;
    for (auto& p : pads) if (std::isnan(p.height)) p.height = Natural(p.c.x, p.c.y);
    h_.assign((size_t)kN * kN, 0.0f);
    splat_.assign((size_t)kN * kN, Vector3{ 1, 0, 0 });
    for (int iz = 0; iz < kN; iz++)
        for (int ix = 0; ix < kN; ix++) {
            float x = -kHalf + ix * kCell, z = -kHalf + iz * kCell;
            float hgt = Natural(x, z);
            Vector3 sp{ 1, 0, 0 };
            // patchy dirt in the grass
            float patch = Fbm2(x * 0.06f, z * 0.06f, 3, 0, 222);
            sp.y = SmoothStep(0.15f, 0.45f, patch) * 0.8f;
            sp.x = 1.0f - sp.y * 0.7f;
            // Road: flat bed, gravel shoulders, ditches
            float rx = RoadX(z), ry = RoadY(z);
            float dr = fabsf(x - rx);
            float roadW = 1.0f - SmoothStep(5.4f, 16.0f, dr);
            float ditch = SmoothStep(5.0f, 6.4f, dr) * (1.0f - SmoothStep(7.4f, 9.8f, dr)) * 0.75f;
            float roadH = ry - 0.02f - ditch - (dr > 3.7f ? (dr - 3.7f) * 0.03f : 0.0f);
            hgt = Lerp(hgt, roadH, roadW);
            float shoulder = SmoothStep(3.4f, 3.9f, dr) * (1.0f - SmoothStep(5.2f, 6.2f, dr));
            float ditchS = SmoothStep(5.6f, 6.4f, dr) * (1.0f - SmoothStep(8.0f, 9.5f, dr));
            sp = Vector3Lerp(sp, Vector3{ 0.05f, 0.25f, 1.0f }, shoulder);
            sp = Vector3Lerp(sp, Vector3{ 0.35f, 1.0f, 0.1f }, ditchS * 0.8f);
            if (dr < 3.7f) sp = Vector3{ 0, 0.2f, 1.0f };
            // Building pads
            for (const TerrainPad& p : pads) {
                float w = PadWeight(p, x, z);
                if (w <= 0) continue;
                hgt = Lerp(hgt, p.height, w);
                if (p.splat >= 0) {
                    Vector3 t = p.splat == 0 ? Vector3{ 1, 0, 0 } : (p.splat == 1 ? Vector3{ 0.1f, 1, 0.2f } : Vector3{ 0, 0.3f, 1 });
                    sp = Vector3Lerp(sp, t, w);
                } else {
                    // worn earth around the edges of everything people use
                    float ring = w * (1.0f - w) * 4.0f;
                    sp = Vector3Lerp(sp, Vector3{ 0.2f, 1.0f, 0.15f }, ring * 0.6f);
                }
            }
            // Worn footpaths
            for (const TerrainPath& path : paths)
                for (size_t k = 0; k + 1 < path.pts.size(); k++) {
                    float d = DistToSegment({ x, z }, path.pts[k], path.pts[k + 1]);
                    float w = 1.0f - SmoothStep(path.width * 0.4f, path.width, d + Fbm2(x * 0.5f, z * 0.5f, 2, 0, 3) * 0.4f);
                    if (w > 0) sp = Vector3Lerp(sp, Vector3{ 0.15f, 1.0f, 0.25f }, w * 0.85f);
                }
            h_[(size_t)iz * kN + ix] = hgt;
            splat_[(size_t)iz * kN + ix] = sp;
        }

    // Build chunk meshes (64 m)
    const int CH = 64;
    for (int cz = 0; cz < (kN - 1) / CH; cz++)
        for (int cx = 0; cx < (kN - 1) / CH; cx++) {
            ModelBuilder mb;
            MeshBuilder& b = mb.M(MAT_TERRAIN);
            for (int iz = cz * CH; iz < (cz + 1) * CH; iz++)
                for (int ix = cx * CH; ix < (cx + 1) * CH; ix++) {
                    float x0 = -kHalf + ix * kCell, z0 = -kHalf + iz * kCell;
                    if (InHole(x0 + 0.5f, z0 + 0.5f)) continue;
                    auto V = [&](int vx, int vz) {
                        return Vector3{ -kHalf + vx * kCell, h_[(size_t)vz * kN + vx], -kHalf + vz * kCell };
                    };
                    auto Nn = [&](int vx, int vz) {
                        int x0i = vx > 0 ? vx - 1 : vx, x1i = vx < kN - 1 ? vx + 1 : vx;
                        int z0i = vz > 0 ? vz - 1 : vz, z1i = vz < kN - 1 ? vz + 1 : vz;
                        float dx = h_[(size_t)vz * kN + x1i] - h_[(size_t)vz * kN + x0i];
                        float dz = h_[(size_t)z1i * kN + vx] - h_[(size_t)z0i * kN + vx];
                        return Vector3Normalize({ -dx / ((x1i - x0i) * kCell), 1.0f, -dz / ((z1i - z0i) * kCell) });
                    };
                    auto C = [&](int vx, int vz) {
                        Vector3 s = splat_[(size_t)vz * kN + vx];
                        return Color{ (unsigned char)(Saturate(s.x) * 255), (unsigned char)(Saturate(s.y) * 255), (unsigned char)(Saturate(s.z) * 255), 255 };
                    };
                    int vx[4] = { ix, ix + 1, ix + 1, ix }, vz[4] = { iz, iz, iz + 1, iz + 1 };
                    Vector3 p[4], n[4]; Color c[4];
                    for (int k = 0; k < 4; k++) { p[k] = V(vx[k], vz[k]); n[k] = Nn(vx[k], vz[k]); c[k] = C(vx[k], vz[k]); }
                    // diagonal 00-11 (matches CHeightField::Sample), wound CCW seen from above
                    b.color = c[0]; b.Vert(p[0], n[0], { 0, 0 });
                    b.color = c[2]; b.Vert(p[2], n[2], { 1, 1 });
                    b.color = c[1]; b.Vert(p[1], n[1], { 1, 0 });
                    b.color = c[0]; b.Vert(p[0], n[0], { 0, 0 });
                    b.color = c[3]; b.Vert(p[3], n[3], { 0, 1 });
                    b.color = c[2]; b.Vert(p[2], n[2], { 1, 1 });
                }
            Model3D* m = mb.Build(true);
            chunks_.push_back({ m });
        }
    BuildRoad();
}

void Terrain::BuildRoad() {
    ModelBuilder mb;
    MeshBuilder& road = mb.M(MAT_ROAD);
    const float step = 2.0f;
    const int across = 6;
    const float hw = RoadHalfWidth();
    for (float z = -kHalf; z < kHalf - step; z += step) {
        for (int i = 0; i < across; i++) {
            float u0 = -hw + (2 * hw) * i / across, u1 = -hw + (2 * hw) * (i + 1) / across;
            auto P = [&](float u, float zz) {
                float x = RoadX(zz) + u;
                // slight crown so rain would drain: centre 4 cm higher
                float crown = 0.04f * (1.0f - (u / hw) * (u / hw));
                return Vector3{ x, RoadY(zz) + 0.03f + crown, zz };
            };
            Vector3 a = P(u0, z), b = P(u1, z), c = P(u1, z + step), d = P(u0, z + step);
            road.Quad(a, d, c, b);
        }
    }
    // Painted markings: worn double yellow centre, white edge lines
    MeshBuilder& yel = mb.M(MAT_PAINT_YELLOW);
    MeshBuilder& wht = mb.M(MAT_PAINT_WHITE);
    auto stripe = [&](MeshBuilder& b, float offset, float w, float z0, float z1) {
        const float st = 1.0f;
        for (float z = z0; z < z1 - 0.01f; z += st) {
            float za = z, zb = fminf(z + st, z1);
            auto P = [&](float u, float zz) {
                float x = RoadX(zz) + u;
                float crown = 0.04f * (1.0f - (u / hw) * (u / hw));
                return Vector3{ x, RoadY(zz) + 0.037f + crown, zz };
            };
            b.Quad(P(offset - w, za), P(offset - w, zb), P(offset + w, zb), P(offset + w, za));
        }
    };
    Rng rng(99);
    for (float z = -kHalf; z < kHalf; z += 6.0f) {
        float wear = Fbm2(z * 0.03f, 1.5f, 2, 0, 4);
        if (wear < -0.25f) continue;                       // long faded stretches
        float len = rng.Chance(0.15f) ? rng.Range(1.0f, 4.0f) : 6.0f;
        stripe(yel, -0.09f, 0.05f, z, z + len);
        stripe(yel, 0.09f, 0.05f, z, z + len);
    }
    for (float z = -kHalf; z < kHalf; z += 6.0f) {
        for (int side = -1; side <= 1; side += 2) {
            float wear = Fbm2(z * 0.05f, side * 7.0f, 2, 0, 8);
            if (wear < -0.1f) continue;
            stripe(wht, side * (hw - 0.25f), 0.06f, z, z + 6.0f);
        }
    }
    roadModel = mb.Build(false);
}

float Terrain::Height(float x, float z) const {
    float fx = (x + kHalf) / kCell, fz = (z + kHalf) / kCell;
    fx = Clamp(fx, 0.0f, (float)kN - 1.001f); fz = Clamp(fz, 0.0f, (float)kN - 1.001f);
    int ix = (int)fx, iz = (int)fz;
    float tx = fx - ix, tz = fz - iz;
    float h00 = h_[(size_t)iz * kN + ix], h10 = h_[(size_t)iz * kN + ix + 1];
    float h01 = h_[(size_t)(iz + 1) * kN + ix], h11 = h_[(size_t)(iz + 1) * kN + ix + 1];
    if (tx > tz) return h00 + (h10 - h00) * tx + (h11 - h10) * tz;
    return h00 + (h11 - h01) * tx + (h01 - h00) * tz;
}

Vector3 Terrain::Normal(float x, float z) const {
    float e = 0.5f;
    float dx = Height(x + e, z) - Height(x - e, z);
    float dz = Height(x, z + e) - Height(x, z - e);
    return Vector3Normalize({ -dx / (2 * e), 1.0f, -dz / (2 * e) });
}

Vector3 Terrain::Splat(float x, float z) const {
    int ix = (int)roundf((x + kHalf) / kCell), iz = (int)roundf((z + kHalf) / kCell);
    ix = ix < 0 ? 0 : (ix >= kN ? kN - 1 : ix);
    iz = iz < 0 ? 0 : (iz >= kN ? kN - 1 : iz);
    return splat_[(size_t)iz * kN + ix];
}

int Terrain::SurfaceAt(float x, float z) const {
    if (DistToRoad(x, z) < RoadHalfWidth()) return SURF_ASPHALT;
    Vector3 s = Splat(x, z);
    if (s.z > s.x && s.z > s.y) return SURF_GRAVEL;
    if (s.y > s.x) return SURF_DIRT;
    return SURF_GRASS;
}

bool Terrain::InHole(float x, float z) const {
    for (const Rectangle& r : holes_)
        if (x >= r.x && x <= r.x + r.width && z >= r.y && z <= r.y + r.height) return true;
    return false;
}

void Terrain::Draw() const {
    for (const Chunk& c : chunks_) Rdr().Draw(c.model, MatrixIdentity());
    Rdr().Draw(roadModel, MatrixIdentity(), WHITE, false);
}

void Terrain::RegisterCollision() {
    CHeightField hf;
    hf.x0 = -kHalf; hf.z0 = -kHalf; hf.cell = kCell; hf.nx = kN; hf.nz = kN;
    hf.h = h_;
    // road surface sits a few cm above the terrain bed
    for (int iz = 0; iz < kN; iz++)
        for (int ix = 0; ix < kN; ix++) {
            float x = -kHalf + ix * kCell, z = -kHalf + iz * kCell;
            float u = x - RoadX(z);
            if (fabsf(u) < RoadHalfWidth()) {
                float crown = 0.04f * (1.0f - (u / RoadHalfWidth()) * (u / RoadHalfWidth()));
                hf.h[(size_t)iz * kN + ix] = RoadY(z) + 0.03f + crown;   // matches the rendered road surface
            }
        }
    hf.holes = holes_;
    hf.surfaceAt = [this](float x, float z) { return SurfaceAt(x, z); };
    hf.owner = -100;
    Phys().AddHeightField(std::move(hf));
}
