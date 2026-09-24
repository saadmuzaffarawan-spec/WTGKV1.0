// Particles (billboards through the fogged particle shader) and draped decals.
#pragma once
#include "engine/renderer.h"
#include <vector>

enum PType { P_SPARK, P_EMBER, P_SMOKE, P_FIRE, P_BLOOD, P_GLASS, P_DUST, P_SPLASH, P_ASH };

struct Particle {
    Vector3 pos, vel;
    float life, maxLife, size, grow;
    Color color;
    int type;
    float gravity, drag;
    bool additive;
};

class Particles {
public:
    void Emit(int type, Vector3 pos, Vector3 vel, float life, float size, Color c);
    void Burst(int type, Vector3 pos, int n, float speed, float life, float size, Color c, Vector3 bias = { 0, 0, 0 });
    void Update(float dt, Vector3 cam, float rain, bool underground);
    void Draw(const Camera3D& cam);
    void Clear() { ps_.clear(); }
    float rainAmount = 0.0f;
private:
    std::vector<Particle> ps_;
    struct Drop { Vector3 p; float speed; };
    std::vector<Drop> rain_;
    float dustT_ = 0.0f;
};

class Decals {
public:
    // Irregular blood pool / stain (on whatever surface is below)
    void Pool(Vector3 c, float radius, int mat, uint32_t seed, float stretch = 1.0f, float yaw = 0.0f);
    // Smeared trail along points (drag marks)
    void Trail(const std::vector<Vector3>& pts, float width, int mat, uint32_t seed, float gaps = 0.2f);
    // Tyre skid marks
    void Skid(const std::vector<Vector3>& pts, float width);
    void Draw();
    void Clear();
    std::vector<Model3D*> models;
    std::vector<bool> visible;
    std::vector<float> alpha;
};

float SurfaceY(float x, float z, float topY);
