// Heightfield terrain with the Route 9 road carved in, building pads,
// stairwell holes, splat weights and chunked meshes.
#pragma once
#include "common.h"
#include "mesh_builder.h"

struct TerrainPad {
    Vector2 c{};            // centre x,z
    Vector2 half{};         // half extents
    float yaw = 0;          // radians
    float margin = 4.0f;    // blend distance
    float height = NAN;     // target height (NAN = sample natural height at centre)
    int splat = -1;         // -1 keep, 0 grass, 1 dirt, 2 gravel
};

struct TerrainPath { std::vector<Vector2> pts; float width = 1.2f; };

class Terrain {
public:
    static constexpr float kHalf = 320.0f;
    static constexpr float kCell = 1.0f;
    static constexpr int kN = 641;   // vertices per side

    // Road centre-line and profile
    static float RoadX(float z);
    static float RoadY(float z);
    static float RoadHalfWidth() { return 3.7f; }
    float DistToRoad(float x, float z) const { return fabsf(x - RoadX(z)); }

    void Build(const std::vector<TerrainPad>& pads, const std::vector<Rectangle>& holes, const std::vector<TerrainPath>& paths);
    float Height(float x, float z) const;
    Vector3 Normal(float x, float z) const;
    Vector3 Splat(float x, float z) const;   // grass, dirt, gravel weights
    int SurfaceAt(float x, float z) const;
    bool InHole(float x, float z) const;
    void Draw() const;
    void RegisterCollision();
    const std::vector<float>& Heights() const { return h_; }
    Model3D* roadModel = nullptr;

private:
    float Natural(float x, float z) const;
    std::vector<float> h_;
    std::vector<Vector3> splat_;
    std::vector<Rectangle> holes_;
    struct Chunk { Model3D* model; };
    std::vector<Chunk> chunks_;
    void BuildRoad();
};

Terrain& World();
