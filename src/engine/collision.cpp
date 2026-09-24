#include "collision.h"

static CollisionWorld g_phys;
CollisionWorld& Phys() { return g_phys; }

static const char* kSurfNames[SURF_COUNT] = { "none", "grass", "dirt", "gravel", "asphalt", "concrete", "wood",
                                               "tile", "metal", "flesh", "water", "carpet", "mud", "bone", "glass" };
const char* SurfaceName(int s) { return (s >= 0 && s < SURF_COUNT) ? kSurfNames[s] : "none"; }
int SurfaceFromName(const std::string& n) {
    for (int i = 0; i < SURF_COUNT; i++) if (n == kSurfNames[i]) return i;
    return SURF_CONCRETE;
}

float CHeightField::Sample(float x, float z, bool* inside) const {
    float fx = (x - x0) / cell, fz = (z - z0) / cell;
    if (fx < 0 || fz < 0 || fx > nx - 1 || fz > nz - 1) { if (inside) *inside = false; return -1e9f; }
    if (inside) *inside = true;
    int ix = (int)fx, iz = (int)fz;
    if (ix >= nx - 1) ix = nx - 2;
    if (iz >= nz - 1) iz = nz - 2;
    float tx = fx - ix, tz = fz - iz;
    float h00 = h[(size_t)iz * nx + ix], h10 = h[(size_t)iz * nx + ix + 1];
    float h01 = h[(size_t)(iz + 1) * nx + ix], h11 = h[(size_t)(iz + 1) * nx + ix + 1];
    // match the triangle split used by the terrain mesh (diagonal 00-11)
    if (tx > tz) return h00 + (h10 - h00) * tx + (h11 - h10) * tz;
    return h00 + (h11 - h01) * tx + (h01 - h00) * tz;
}

bool CHeightField::InHole(float x, float z) const {
    for (const Rectangle& r : holes)
        if (x >= r.x && x <= r.x + r.width && z >= r.y && z <= r.y + r.height) return true;
    return false;
}

int CollisionWorld::AddBox(const CBox& b) {
    CBox c = b;
    c.cs = cosf(b.yaw); c.sn = sinf(b.yaw);
    boxes_.push_back(c);
    return (int)boxes_.size() - 1;
}
int CollisionWorld::AddCyl(const CCyl& c) { cyls_.push_back(c); return (int)cyls_.size() - 1; }
int CollisionWorld::AddHeightField(CHeightField&& hf) { fields_.push_back(std::move(hf)); return (int)fields_.size() - 1; }

void CollisionWorld::RemoveOwner(int owner) {
    for (auto& b : boxes_) if (b.owner == owner) { b.enabled = false; b.owner = -2; }
    for (auto& c : cyls_) if (c.owner == owner) { c.enabled = false; c.owner = -2; }
}
void CollisionWorld::SetOwnerEnabled(int owner, bool en) {
    for (auto& b : boxes_) if (b.owner == owner) b.enabled = en;
    for (auto& c : cyls_) if (c.owner == owner) c.enabled = en;
    for (auto& f : fields_) if (f.owner == owner) f.enabled = en;
}
void CollisionWorld::UpdateBox(int i, Vector3 c, float yaw) {
    if (i < 0 || i >= (int)boxes_.size()) return;
    boxes_[i].c = c; boxes_[i].yaw = yaw; boxes_[i].cs = cosf(yaw); boxes_[i].sn = sinf(yaw);
}
void CollisionWorld::Clear() { boxes_.clear(); cyls_.clear(); fields_.clear(); }

static inline void ToLocal(const CBox& b, float wx, float wz, float& lx, float& lz) {
    float dx = wx - b.c.x, dz = wz - b.c.z;
    lx = b.cs * dx - b.sn * dz;
    lz = b.sn * dx + b.cs * dz;
}
static inline void ToWorldDir(const CBox& b, float lx, float lz, float& wx, float& wz) {
    wx = b.cs * lx + b.sn * lz;
    wz = -b.sn * lx + b.cs * lz;
}

float CollisionWorld::Ground(float x, float z, float topY, float radius, int* surface, int* owner) const {
    float best = -1e9f; int bestS = SURF_NONE, bestO = -1;
    float m = radius * 0.45f;
    for (const CBox& b : boxes_) {
        if (!b.enabled || !b.walkable) continue;
        float top = b.c.y + b.h.y;
        if (top > topY || top <= best) continue;
        float lx, lz; ToLocal(b, x, z, lx, lz);
        if (fabsf(lx) > b.h.x + m || fabsf(lz) > b.h.z + m) continue;
        best = top; bestS = b.surface; bestO = b.owner;
    }
    for (const CCyl& c : cyls_) {
        if (!c.enabled || !c.walkable) continue;
        float top = c.base.y + c.h;
        if (top > topY || top <= best) continue;
        float dx = x - c.base.x, dz = z - c.base.z;
        if (dx * dx + dz * dz > (c.r + m) * (c.r + m)) continue;
        best = top; bestS = c.surface; bestO = c.owner;
    }
    for (const CHeightField& f : fields_) {
        if (!f.enabled || f.isCeiling) continue;
        if (f.InHole(x, z)) continue;
        bool in; float hgt = f.Sample(x, z, &in);
        if (!in || hgt > topY || hgt <= best) continue;
        best = hgt; bestO = f.owner;
        bestS = f.surfaceAt ? f.surfaceAt(x, z) : f.surface;
    }
    if (surface) *surface = bestS;
    if (owner) *owner = bestO;
    return best;
}

float CollisionWorld::Ceiling(float x, float z, float y, float radius) const {
    float best = 1e9f;
    for (const CBox& b : boxes_) {
        if (!b.enabled || !b.solid) continue;
        float bot = b.c.y - b.h.y;
        if (bot < y || bot >= best) continue;
        float lx, lz; ToLocal(b, x, z, lx, lz);
        if (fabsf(lx) > b.h.x + radius * 0.3f || fabsf(lz) > b.h.z + radius * 0.3f) continue;
        best = bot;
    }
    for (const CHeightField& f : fields_) {
        if (!f.enabled || !f.isCeiling) continue;
        bool in; float hgt = f.Sample(x, z, &in);
        if (in && hgt >= y && hgt < best) best = hgt;
    }
    return best;
}

bool CollisionWorld::ResolveCylinder(Vector3& feet, float radius, float height, float stepH, int ignoreOwner) const {
    bool any = false;
    float y0 = feet.y + stepH, y1 = feet.y + height;
    for (int iter = 0; iter < 3; iter++) {
        bool moved = false;
        for (const CBox& b : boxes_) {
            if (!b.enabled || !b.solid || b.owner == ignoreOwner && ignoreOwner >= 0) continue;
            if (b.c.y + b.h.y <= y0 || b.c.y - b.h.y >= y1) continue;
            float lx, lz; ToLocal(b, feet.x, feet.z, lx, lz);
            if (fabsf(lx) > b.h.x + radius || fabsf(lz) > b.h.z + radius) continue;
            float cx = Clamp(lx, -b.h.x, b.h.x), cz = Clamp(lz, -b.h.z, b.h.z);
            float dx = lx - cx, dz = lz - cz;
            float d2 = dx * dx + dz * dz;
            float px = 0, pz = 0;
            if (d2 > 1e-8f) {
                float d = sqrtf(d2);
                if (d >= radius) continue;
                float push = radius - d;
                px = dx / d * push; pz = dz / d * push;
            } else {
                // centre inside the box: push out along smallest penetration
                float ox = b.h.x + radius - fabsf(lx), oz = b.h.z + radius - fabsf(lz);
                if (ox < oz) px = (lx >= 0 ? ox : -ox); else pz = (lz >= 0 ? oz : -oz);
            }
            float wx, wz; ToWorldDir(b, px, pz, wx, wz);
            feet.x += wx; feet.z += wz;
            moved = any = true;
        }
        for (const CCyl& c : cyls_) {
            if (!c.enabled || c.owner == ignoreOwner && ignoreOwner >= 0) continue;
            if (c.base.y + c.h <= y0 || c.base.y >= y1) continue;
            float dx = feet.x - c.base.x, dz = feet.z - c.base.z;
            float rr = c.r + radius;
            float d2 = dx * dx + dz * dz;
            if (d2 >= rr * rr) continue;
            float d = sqrtf(fmaxf(d2, 1e-8f));
            if (d < 1e-4f) { dx = 1; dz = 0; d = 1; }
            float push = rr - d;
            feet.x += dx / d * push; feet.z += dz / d * push;
            moved = any = true;
        }
        if (!moved) break;
    }
    return any;
}

static bool RayBox(const CBox& b, Vector3 o, Vector3 d, float& tHit, Vector3& nrm) {
    // transform into box local frame (yaw only)
    float lox, loz; ToLocal(b, o.x, o.z, lox, loz);
    float loy = o.y - b.c.y;
    float ldx = b.cs * d.x - b.sn * d.z, ldz = b.sn * d.x + b.cs * d.z, ldy = d.y;
    float lo[3] = { lox, loy, loz }, ld[3] = { ldx, ldy, ldz }, hh[3] = { b.h.x, b.h.y, b.h.z };
    float tmin = -1e9f, tmax = 1e9f; int axis = -1; float sign = 1;
    for (int i = 0; i < 3; i++) {
        if (fabsf(ld[i]) < 1e-8f) {
            if (lo[i] < -hh[i] || lo[i] > hh[i]) return false;
        } else {
            float t1 = (-hh[i] - lo[i]) / ld[i], t2 = (hh[i] - lo[i]) / ld[i];
            float s = -1;
            if (t1 > t2) { float t = t1; t1 = t2; t2 = t; s = 1; }
            if (t1 > tmin) { tmin = t1; axis = i; sign = s; }
            if (t2 < tmax) tmax = t2;
            if (tmin > tmax) return false;
        }
    }
    if (tmax < 0) return false;
    tHit = tmin >= 0 ? tmin : 0.0f;
    float ln[3] = { 0, 0, 0 };
    if (axis >= 0) ln[axis] = sign;
    float wx, wz; ToWorldDir(b, ln[0], ln[2], wx, wz);
    nrm = { wx, ln[1], wz };
    return true;
}

bool CollisionWorld::Raycast(Vector3 o, Vector3 d, float maxDist, RayHit& hit, int ignoreOwner, bool sightOnly) const {
    d = Vector3Normalize(d);
    hit = RayHit{};
    hit.t = maxDist;
    for (const CBox& b : boxes_) {
        if (!b.enabled || (ignoreOwner >= 0 && b.owner == ignoreOwner)) continue;
        if (sightOnly && !b.blocksSight) continue;
        float t; Vector3 n;
        if (RayBox(b, o, d, t, n) && t < hit.t) {
            hit.t = t; hit.normal = n; hit.owner = b.owner; hit.surface = b.surface; hit.hit = true;
        }
    }
    for (const CCyl& c : cyls_) {
        if (!c.enabled || (ignoreOwner >= 0 && c.owner == ignoreOwner)) continue;
        float ox = o.x - c.base.x, oz = o.z - c.base.z;
        float a = d.x * d.x + d.z * d.z;
        if (a < 1e-8f) continue;
        float bq = 2 * (ox * d.x + oz * d.z), cq = ox * ox + oz * oz - c.r * c.r;
        float disc = bq * bq - 4 * a * cq;
        if (disc < 0) continue;
        float t = (-bq - sqrtf(disc)) / (2 * a);
        if (t < 0) t = 0;
        float y = o.y + d.y * t;
        if (y < c.base.y || y > c.base.y + c.h) continue;
        if (t < hit.t) {
            Vector3 p = Vector3Add(o, Vector3Scale(d, t));
            hit.t = t; hit.normal = Vector3Normalize({ p.x - c.base.x, 0, p.z - c.base.z });
            hit.owner = c.owner; hit.surface = c.surface; hit.hit = true;
        }
    }
    for (const CHeightField& f : fields_) {
        if (!f.enabled) continue;
        float step = 0.4f;
        float prevT = 0;
        bool prevAbove = true;
        for (float t = 0; t < hit.t; t += step) {
            Vector3 p = Vector3Add(o, Vector3Scale(d, t));
            if (f.InHole(p.x, p.z)) { prevT = t; continue; }
            bool in; float h = f.Sample(p.x, p.z, &in);
            if (!in) { prevT = t; continue; }
            bool above = f.isCeiling ? (p.y < h) : (p.y > h);
            if (!above && prevAbove && t > 0) {
                float a = prevT, bb = t;
                for (int k = 0; k < 8; k++) {
                    float m = (a + bb) * 0.5f;
                    Vector3 q = Vector3Add(o, Vector3Scale(d, m));
                    float hq = f.Sample(q.x, q.z);
                    bool ab = f.isCeiling ? (q.y < hq) : (q.y > hq);
                    if (ab) a = m; else bb = m;
                }
                if (bb < hit.t) {
                    hit.t = bb; hit.normal = { 0, f.isCeiling ? -1.0f : 1.0f, 0 }; hit.owner = f.owner; hit.hit = true;
                    Vector3 q = Vector3Add(o, Vector3Scale(d, bb));
                    hit.surface = f.surfaceAt ? f.surfaceAt(q.x, q.z) : f.surface;
                }
                break;
            }
            prevAbove = above; prevT = t;
        }
    }
    hit.point = Vector3Add(o, Vector3Scale(d, hit.t));
    return hit.hit;
}

bool CollisionWorld::LineOfSight(Vector3 a, Vector3 b, int ignoreOwner) const {
    Vector3 d = Vector3Subtract(b, a);
    float len = Vector3Length(d);
    if (len < 1e-4f) return true;
    RayHit h;
    return !Raycast(a, d, len - 0.05f, h, ignoreOwner, true);
}
