// Procedural forest scatter and dense wind-blown grass around the camera.
#pragma once
#include "engine/scene.h"

struct ExclusionZone { Vector2 c; Vector2 half; float yaw; float margin; };

class Vegetation {
public:
    void Build(const std::vector<TerrainPad>& pads, const std::vector<ExclusionZone>& extra, uint32_t seed);
    void Draw(Vector3 camPos);
    float density = 1.0f;   // settings: grass density multiplier
    bool Excluded(float x, float z, float extraMargin = 0.0f) const;
private:
    struct Group { const Model3D* model; std::vector<Matrix> xfs; };
    std::vector<Group> groups_;
    std::vector<ExclusionZone> zones_;
    MeshAsset* grassMesh_ = nullptr;
    MeshAsset* grassMeshFar_ = nullptr;
    std::vector<Matrix> grassNear_, grassFar_;
    Vector3 lastCam_{ 1e9f, 0, 0 };
    void RebuildGrass(Vector3 cam);
};

Vegetation& Veg();
