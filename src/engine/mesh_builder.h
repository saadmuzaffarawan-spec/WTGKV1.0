// CPU-side mesh construction. Prefabs describe objects with primitives
// (bevelled boxes, cylinders, lathes, tubes, ellipsoids) grouped by material;
// the result is uploaded once as GPU meshes.
#pragma once
#include "common.h"
#include <map>
#include <memory>
#include <functional>

struct MeshAsset {
    Mesh mesh{};
    std::vector<Mesh> more;      // further indexed chunks when a mesh has > 65535 unique vertices
    // Optional simpler version drawn from `lodDist` metres (and in shadow maps). Chains.
    const MeshAsset* lod = nullptr;
    float lodDist = 0.0f;
    Vector3 bmin{}, bmax{};
    Vector3 center{};
    float radius = 0.0f;
};

// Part of a model: one mesh drawn with one material.
struct ModelPart {
    MeshAsset* mesh = nullptr;
    int mat = 0;
};

struct Model3D {
    std::vector<ModelPart> parts;
    Vector3 bmin{ 1e9f, 1e9f, 1e9f }, bmax{ -1e9f, -1e9f, -1e9f };
    Vector3 center{};
    float radius = 0.0f;
    bool castShadow = true;
};

class MeshBuilder {
public:
    std::vector<float> pos, nrm, uv;
    std::vector<unsigned char> col;
    Color color{ 255, 255, 255, 255 };   // rgb = tint/AO multiplier, a = foliage flex weight
    const Matrix* xf = nullptr;          // optional external transform (ModelBuilder stack)

    size_t VertexCount() const { return pos.size() / 3; }
    void Vert(Vector3 p, Vector3 n, Vector2 t);
    void Tri(Vector3 a, Vector3 b, Vector3 c);                                    // flat normal, planar UV
    void TriN(Vector3 a, Vector3 b, Vector3 c, Vector3 na, Vector3 nb, Vector3 nc,
              Vector2 ta = { 0, 0 }, Vector2 tb = { 1, 0 }, Vector2 tc = { 1, 1 });
    void Quad(Vector3 a, Vector3 b, Vector3 c, Vector3 d,
              Vector2 ta = { 0, 1 }, Vector2 tb = { 1, 1 }, Vector2 tc = { 1, 0 }, Vector2 td = { 0, 0 });

    // Axis-aligned (in local frame) box with optional chamfer on all edges.
    void Box(Vector3 center, Vector3 size, float bevel = 0.0f);
    void BoxRot(Vector3 center, Vector3 size, Vector3 eulerDeg, float bevel = 0.0f);
    // Box with UVs spanning each face 0..1 (for signs/screens in UV mode)
    void BoxUV(Vector3 center, Vector3 size);
    // Cylinder/frustum from a to b.
    void Cylinder(Vector3 a, Vector3 b, float ra, float rb, int seg = 16, bool caps = true);
    void Sphere(Vector3 c, float r, int rings = 10, int segs = 16) { Ellipsoid(c, { r, r, r }, rings, segs); }
    void Ellipsoid(Vector3 c, Vector3 radii, int rings = 10, int segs = 16);
    // Lathe: profile of (radius, y) points revolved around the local Y axis at `base`.
    void Lathe(Vector3 base, const std::vector<Vector2>& profile, int segs = 20, bool capBottom = true, bool capTop = true);
    // Tube through a polyline with per-point radius.
    void Tube(const std::vector<Vector3>& pts, const std::vector<float>& radii, int segs = 8, bool caps = true);
    // Single-sided rectangle facing +normal side (center, right axis half-extent, up axis half-extent)
    void Panel(Vector3 center, Vector3 right, Vector3 up, bool doubleSided = false);
    // Subdivided plane on XZ with height callback (used for decals conforming to terrain etc.)
    void Append(const MeshBuilder& o);

    MeshAsset* Upload();   // uploads to GPU and registers for cleanup
private:
    Vector3 P(Vector3 p) const { return xf ? Vector3Transform(p, *xf) : p; }
    Vector3 N(Vector3 n) const;
};

class ModelBuilder {
public:
    ModelBuilder();
    MeshBuilder& M(int mat);                 // select material group
    void Push();
    void Pop();
    void Translate(Vector3 t);
    void RotateY(float deg);
    void RotateX(float deg);
    void RotateZ(float deg);
    void Scale(Vector3 s);
    void Transform(const Matrix& m);         // pre-multiplies current transform
    Matrix Current() const { return stack_.back(); }
    void SetColor(Color c);                  // vertex tint for subsequent geometry (all groups)
    void Flex(float f);                      // foliage flex weight (alpha channel)
    Model3D* Build(bool castShadow = true);  // upload all groups; registers model
    // Deform every vertex added so far (e.g. crumpled wreck); optionally recompute flat normals.
    void Deform(const std::function<Vector3(Vector3)>& fn, bool flatNormals);
private:
    std::map<int, std::unique_ptr<MeshBuilder>> groups_;
    std::vector<Matrix> stack_;
    Color color_{ 255, 255, 255, 255 };
};

void UnloadAllMeshes();
