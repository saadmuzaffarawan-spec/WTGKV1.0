#include "vegetation.h"
#include "engine/materials.h"

static Vegetation g_veg;
Vegetation& Veg() { return g_veg; }

static bool InZone(const ExclusionZone& z, float x, float zz, float extra) {
    float dx = x - z.c.x, dz = zz - z.c.y;
    float cs = cosf(z.yaw), sn = sinf(z.yaw);
    float lx = cs * dx - sn * dz, lz = sn * dx + cs * dz;
    return fabsf(lx) < z.half.x + z.margin + extra && fabsf(lz) < z.half.y + z.margin + extra;
}

bool Vegetation::Excluded(float x, float z, float extra, bool forTrees) const {
    for (const auto& zn : zones_) if ((forTrees || !zn.treesOnly) && InZone(zn, x, z, extra)) return true;
    return false;
}

static MeshAsset* BuildGrassClump(int blades, float radius, float minH, float maxH, uint32_t seed) {
    MeshBuilder mb;
    Rng rng(seed);
    for (int i = 0; i < blades; i++) {
        float a = rng.Range(0, 6.2831f);
        float r = sqrtf(rng.F()) * radius;
        Vector3 base{ cosf(a) * r, 0, sinf(a) * r };
        float h = rng.Range(minH, maxH);
        float facing = rng.Range(0, 6.2831f);
        Vector3 side{ cosf(facing), 0, sinf(facing) };
        Vector3 bend{ -side.z * rng.Range(0.1f, 0.35f), 0, side.x * rng.Range(0.1f, 0.35f) };
        float w = rng.Range(0.012f, 0.028f);
        // dry straw vs olive vs dark dead blades
        float t = rng.F();
        Color col = t < 0.55f ? Color{ 205, 180, 120, 255 } : (t < 0.85f ? Color{ 140, 145, 90, 255 } : Color{ 110, 90, 65, 255 });
        const int segs = 3;
        Vector3 prevL, prevR; Color prevC{};
        for (int s = 0; s <= segs; s++) {
            float k = (float)s / segs;
            Vector3 c = Vector3Add(base, Vector3Add(Vector3Scale(bend, k * k * h), { 0, k * h, 0 }));
            float ww = w * (1.0f - k * 0.9f);
            Vector3 L = Vector3Subtract(c, Vector3Scale(side, ww)), R = Vector3Add(c, Vector3Scale(side, ww));
            float shade = 0.45f + 0.55f * k;   // darker at the root (self-occlusion)
            Color cc{ (unsigned char)(col.r * shade), (unsigned char)(col.g * shade), (unsigned char)(col.b * shade),
                      (unsigned char)((1.0f - k) * 255) };  // alpha encodes 1-flex
            if (s > 0) {
                Vector3 n = Vector3Normalize(Vector3Add(Vector3CrossProduct(side, { 0, 1, 0 }), { 0, 0.6f, 0 }));
                mb.color = prevC; mb.Vert(prevL, n, { 0, 0 });
                mb.color = prevC; mb.Vert(prevR, n, { 1, 0 });
                mb.color = cc; mb.Vert(R, n, { 1, 1 });
                mb.color = prevC; mb.Vert(prevL, n, { 0, 0 });
                mb.color = cc; mb.Vert(R, n, { 1, 1 });
                mb.color = cc; mb.Vert(L, n, { 0, 1 });
            }
            prevL = L; prevR = R; prevC = cc;
        }
    }
    return mb.Upload();
}

void Vegetation::Build(const std::vector<TerrainPad>& pads, const std::vector<ExclusionZone>& extra, uint32_t seed) {
    zones_.clear();
    for (const auto& p : pads) zones_.push_back({ p.c, p.half, p.yaw, 1.5f });
    for (const auto& z : extra) zones_.push_back(z);
    groups_.clear();
    grassMesh_ = BuildGrassClump(14, 0.32f, 0.25f, 0.7f, seed ^ 0x51);
    grassMeshFar_ = BuildGrassClump(10, 0.6f, 0.3f, 0.75f, seed ^ 0x77);

    // Forest scatter on a jittered grid
    struct Kind { const char* prefab; int variants; float weight; };
    const Kind kinds[] = { { "pine_tree", 8, 0.55f }, { "dead_tree", 8, 0.3f }, { "bush", 5, 0.1f }, { "rock", 6, 0.05f } };
    std::map<std::string, int> groupIndex;
    Rng rng(seed);
    const float cell = 6.5f;
    for (float gz = -300; gz < 300; gz += cell)
        for (float gx = -300; gx < 300; gx += cell) {
            float x = gx + rng.Range(0, cell), z = gz + rng.Range(0, cell);
            float dr = World().DistToRoad(x, z);
            if (dr < 13.0f) continue;
            float d = sqrtf(x * x + z * z);
            float dens = SmoothStep(35.0f, 95.0f, d) * 0.85f + 0.08f;
            dens *= 0.6f + 0.4f * (Fbm2(x * 0.02f, z * 0.02f, 3, 0, seed) * 0.5f + 0.5f) * 2.0f;
            if (dr < 22.0f) dens *= 0.3f;
            if (rng.F() > dens) continue;
            if (Excluded(x, z, 4.0f, true)) continue;
            float pick = rng.F();
            const Kind* k = &kinds[0];
            float acc = 0;
            for (const Kind& kk : kinds) { acc += kk.weight; if (pick <= acc) { k = &kk; break; } }
            int variant = rng.RangeI(0, k->variants - 1);
            std::string key = std::string(k->prefab) + ":" + std::to_string(variant);
            auto it = groupIndex.find(key);
            if (it == groupIndex.end()) {
                Model3D* m = GetPrefabModel(k->prefab, { { "variant", std::to_string(variant) } });
                groups_.push_back({ m, {} });
                // branchy trees and bushes get lighter versions for distance (sub-pixel twigs dropped)
                std::string pf = k->prefab;
                if (pf == "dead_tree" || pf == "bush") {
                    groups_.back().lod[0] = GetPrefabModel(pf, { { "variant", std::to_string(variant) }, { "lod", "1" } });
                    groups_.back().lod[1] = pf == "dead_tree" ? GetPrefabModel(pf, { { "variant", std::to_string(variant) }, { "lod", "2" } }) : groups_.back().lod[0];
                }
                it = groupIndex.emplace(key, (int)groups_.size() - 1).first;
            }
            float y = World().Height(x, z);
            float s = rng.Range(0.85f, 1.25f);
            groups_[it->second].xfs.push_back(MatPose({ x, y, z }, rng.Range(0, 6.28f), 0, 0, { s, s, s }));
            // trunks near the playable area collide
            if (d < 200.0f && k->prefab[0] != 'b') {
                CCyl c; c.base = { x, y, z }; c.r = (k->prefab[0] == 'r') ? 1.0f * s : 0.3f * s; c.h = 4.0f; c.surface = SURF_WOOD; c.owner = -200;
                Phys().AddCyl(c);
            }
        }
}

void Vegetation::RebuildGrass(Vector3 cam) {
    grassNear_.clear(); grassFar_.clear();
    auto place = [&](float spacing, float r0, float r1, std::vector<Matrix>& out, uint32_t salt) {
        int ix0 = (int)floorf((cam.x - r1) / spacing), ix1 = (int)ceilf((cam.x + r1) / spacing);
        int iz0 = (int)floorf((cam.z - r1) / spacing), iz1 = (int)ceilf((cam.z + r1) / spacing);
        for (int iz = iz0; iz <= iz1; iz++)
            for (int ix = ix0; ix <= ix1; ix++) {
                uint32_t h = Hash2i(ix, iz, salt);
                float x = (ix + (h & 0xff) / 255.0f) * spacing, z = (iz + ((h >> 8) & 0xff) / 255.0f) * spacing;
                float dx = x - cam.x, dz = z - cam.z;
                float d2 = dx * dx + dz * dz;
                if (d2 < r0 * r0 || d2 > r1 * r1) continue;
                if (((h >> 16) & 0xff) / 255.0f > density) continue;
                Vector3 sp = World().Splat(x, z);
                float grassW = sp.x / fmaxf(sp.x + sp.y + sp.z, 0.01f);
                if (grassW < 0.5f + ((h >> 24) & 0x3f) / 255.0f) continue;
                if (World().DistToRoad(x, z) < 4.2f) continue;
                if (Excluded(x, z)) continue;
                float y = World().Height(x, z);
                float s = 0.7f + ((h >> 4) & 0xff) / 255.0f * 0.7f;
                out.push_back(MatPose({ x, y - 0.02f, z }, ((h >> 12) & 0xff) / 255.0f * 6.28f, 0, 0, { s, s * (0.8f + grassW * 0.4f), s }));
            }
    };
    place(0.55f, 0.0f, 20.0f, grassNear_, 17);
    place(1.1f, 20.0f, 48.0f, grassFar_, 29);
}

void Vegetation::Draw(Vector3 cam) {
    // Level of detail by distance. At 40 m a pixel is ~8 cm wide, so the thinnest twigs are a
    // small fraction of a pixel; beyond 90 m the small branches are too.
    const float kLod1 = 40.0f, kLod2 = 90.0f;
    for (const Group& g : groups_) {
        if (!g.lod[0]) {
            for (const ModelPart& p : g.model->parts) Rdr().DrawInstanced(p.mesh, p.mat, g.xfs, 320.0f);
            continue;
        }
        for (const ModelPart& p : g.model->parts) Rdr().DrawInstanced(p.mesh, p.mat, g.xfs, kLod1);
        for (const ModelPart& p : g.lod[0]->parts) Rdr().DrawInstanced(p.mesh, p.mat, g.xfs, kLod2, kLod1);
        for (const ModelPart& p : g.lod[1]->parts) Rdr().DrawInstanced(p.mesh, p.mat, g.xfs, 320.0f, kLod2);
    }
    if (Vector3Distance(cam, lastCam_) > 3.0f) { RebuildGrass(cam); lastCam_ = cam; }
    Rdr().DrawInstanced(grassMesh_, MAT_GRASS_BLADE, grassNear_, 21.0f);
    Rdr().DrawInstanced(grassMeshFar_, MAT_GRASS_BLADE, grassFar_, 50.0f);
}
