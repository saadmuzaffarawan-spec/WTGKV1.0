// Static collision world: yaw-rotated boxes, vertical cylinders and height fields.
// The player is a vertical cylinder. Every collider carries a surface id used
// for footstep sounds and a walkable flag.
#pragma once
#include "common.h"
#include <functional>

enum Surface {
    SURF_NONE, SURF_GRASS, SURF_DIRT, SURF_GRAVEL, SURF_ASPHALT, SURF_CONCRETE, SURF_WOOD,
    SURF_TILE, SURF_METAL, SURF_FLESH, SURF_WATER, SURF_CARPET, SURF_MUD, SURF_BONE, SURF_GLASS,
    SURF_COUNT
};
const char* SurfaceName(int s);
int SurfaceFromName(const std::string& n);

struct CBox {
    Vector3 c{};      // world centre
    Vector3 h{};      // half extents (local)
    float yaw = 0;    // radians
    float cs = 1, sn = 0;
    int surface = SURF_CONCRETE;
    int owner = -1;
    bool enabled = true;
    bool solid = true;      // blocks movement
    bool walkable = true;   // can be stood on
    bool blocksSight = true;
};

struct CCyl {
    Vector3 base{};
    float r = 0.5f, h = 1.0f;
    int surface = SURF_METAL;
    int owner = -1;
    bool enabled = true;
    bool walkable = true;
};

struct CHeightField {
    float x0 = 0, z0 = 0, cell = 1;
    int nx = 0, nz = 0;             // vertex counts
    std::vector<float> h;
    std::vector<Rectangle> holes;   // x,z,w,h rectangles with no collision
    std::function<int(float, float)> surfaceAt;
    int surface = SURF_GRASS;
    bool enabled = true;
    bool isCeiling = false;         // blocks from above instead
    int owner = -1;
    float Sample(float x, float z, bool* inside = nullptr) const;
    bool InHole(float x, float z) const;
};

struct RayHit {
    float t = 1e9f;
    Vector3 point{}, normal{};
    int owner = -1;
    int surface = SURF_NONE;
    bool hit = false;
};

class CollisionWorld {
public:
    int AddBox(const CBox& b);
    int AddCyl(const CCyl& c);
    int AddHeightField(CHeightField&& hf);
    void RemoveOwner(int owner);
    void SetOwnerEnabled(int owner, bool enabled);
    CBox* Box(int i) { return (i >= 0 && i < (int)boxes_.size()) ? &boxes_[i] : nullptr; }
    void UpdateBox(int i, Vector3 c, float yaw);
    CHeightField* Field(int i) { return (i >= 0 && i < (int)fields_.size()) ? &fields_[i] : nullptr; }

    // Highest walkable support at (x,z) at or below `topY`. Returns -1e9 when none.
    float Ground(float x, float z, float topY, float radius, int* surface = nullptr, int* owner = nullptr) const;
    // Lowest ceiling above `y` (for heads). Returns 1e9 when none.
    float Ceiling(float x, float z, float y, float radius) const;
    // Push a vertical cylinder [feet+stepH, feet+height] out of solid geometry.
    bool ResolveCylinder(Vector3& feet, float radius, float height, float stepH, int ignoreOwner = -1) const;
    bool Raycast(Vector3 o, Vector3 d, float maxDist, RayHit& hit, int ignoreOwner = -1, bool sightOnly = false) const;
    bool LineOfSight(Vector3 a, Vector3 b, int ignoreOwner = -1) const;
    void Clear();
    const std::vector<CBox>& Boxes() const { return boxes_; }
    const std::vector<CCyl>& Cyls() const { return cyls_; }

private:
    std::vector<CBox> boxes_;
    std::vector<CCyl> cyls_;
    std::vector<CHeightField> fields_;
};

CollisionWorld& Phys();
