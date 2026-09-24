#include "fx.h"
#include "engine/collision.h"
#include "engine/terrain.h"
#include "engine/materials.h"
#include <rlgl.h>

float SurfaceY(float x, float z, float topY) {
    float g = Phys().Ground(x, z, topY, 0.0f);
    return g > -1e8f ? g : World().Height(x, z);
}

void Particles::Emit(int type, Vector3 pos, Vector3 vel, float life, float size, Color c) {
    if (ps_.size() > 4000) return;
    Particle p{};
    p.pos = pos; p.vel = vel; p.life = p.maxLife = life; p.size = size; p.color = c; p.type = type;
    p.gravity = 0; p.drag = 0.5f; p.grow = 0; p.additive = false;
    switch (type) {
    case P_SPARK: p.gravity = -9.8f; p.drag = 0.3f; p.additive = true; break;
    case P_EMBER: p.gravity = 1.2f; p.drag = 1.2f; p.additive = true; break;
    case P_SMOKE: p.gravity = 1.1f; p.drag = 0.8f; p.grow = 1.6f; break;
    case P_FIRE: p.gravity = 2.8f; p.drag = 1.5f; p.grow = -0.2f; p.additive = true; break;
    case P_BLOOD: p.gravity = -9.8f; p.drag = 0.2f; break;
    case P_GLASS: p.gravity = -9.8f; p.drag = 0.1f; p.additive = true; break;
    case P_DUST: p.gravity = -0.02f; p.drag = 2.0f; p.additive = true; break;
    case P_SPLASH: p.gravity = -9.8f; p.drag = 0.4f; break;
    case P_ASH: p.gravity = -0.3f; p.drag = 1.5f; break;
    }
    ps_.push_back(p);
}

void Particles::Burst(int type, Vector3 pos, int n, float speed, float life, float size, Color c, Vector3 bias) {
    for (int i = 0; i < n; i++) {
        Vector3 v{ Frand(-1, 1), Frand(-1, 1), Frand(-1, 1) };
        v = Vector3Add(Vector3Scale(Vector3Normalize(v), speed * Frand(0.3f, 1.0f)), bias);
        Emit(type, pos, v, life * Frand(0.6f, 1.2f), size * Frand(0.6f, 1.3f), c);
    }
}

void Particles::Update(float dt, Vector3 cam, float rain, bool underground) {
    rainAmount = rain;
    for (auto& p : ps_) {
        p.vel.y += p.gravity * dt;
        p.vel = Vector3Scale(p.vel, 1.0f / (1.0f + p.drag * dt));
        p.pos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));
        p.size = fmaxf(0.001f, p.size + p.grow * dt);
        p.life -= dt;
        if ((p.type == P_BLOOD || p.type == P_GLASS || p.type == P_SPARK || p.type == P_SPLASH) && p.vel.y < 0) {
            float g = SurfaceY(p.pos.x, p.pos.z, p.pos.y + 0.2f);
            if (p.pos.y < g) {
                p.pos.y = g + 0.002f;
                if (p.type == P_GLASS || p.type == P_SPARK) { p.vel.y *= -0.3f; p.vel.x *= 0.5f; p.vel.z *= 0.5f; }
                else { p.vel = { 0, 0, 0 }; p.gravity = 0; p.life = fminf(p.life, 0.4f); }
            }
        }
    }
    for (size_t i = 0; i < ps_.size();) {
        if (ps_[i].life <= 0) { ps_[i] = ps_.back(); ps_.pop_back(); } else i++;
    }
    // rain: a cylinder of drops around the camera
    int want = underground ? 0 : (int)(rain * 1800);
    while ((int)rain_.size() < want) rain_.push_back({ { cam.x + Frand(-18, 18), cam.y + Frand(-2, 14), cam.z + Frand(-18, 18) }, Frand(8.5f, 10.5f) });
    if ((int)rain_.size() > want) rain_.resize(want);
    for (auto& d : rain_) {
        d.p.y -= d.speed * dt;
        d.p.x += 0.8f * dt;
        float g = SurfaceY(d.p.x, d.p.z, d.p.y + 1.0f);
        if (d.p.y < g || fabsf(d.p.x - cam.x) > 18 || fabsf(d.p.z - cam.z) > 18) {
            if (d.p.y < g && Vector3Distance(d.p, cam) < 8.0f && Frand(0, 1) < 0.3f) Emit(P_SPLASH, { d.p.x, g + 0.01f, d.p.z }, { Frand(-0.4f, 0.4f), Frand(0.6f, 1.2f), Frand(-0.4f, 0.4f) }, 0.25f, 0.012f, Color{ 180, 190, 200, 120 });
            d.p = { cam.x + Frand(-18, 18), cam.y + Frand(6, 14), cam.z + Frand(-18, 18) };
        }
    }
    // dust motes near the camera (only visible where light hits them)
    dustT_ -= dt;
    if (dustT_ <= 0 && !underground) {
        dustT_ = 0.05f;
        Emit(P_DUST, { cam.x + Frand(-4, 4), cam.y + Frand(-1.2f, 1.5f), cam.z + Frand(-4, 4) }, { Frand(-0.05f, 0.05f), Frand(-0.03f, 0.03f), Frand(-0.05f, 0.05f) }, Frand(4, 9), Frand(0.004f, 0.009f), Color{ 200, 190, 170, 40 });
    }
}

static void Quad(Vector3 p, Vector3 r, Vector3 u, Color c) {
    rlColor4ub(c.r, c.g, c.b, c.a);
    rlTexCoord2f(0, 0); rlVertex3f(p.x - r.x - u.x, p.y - r.y - u.y, p.z - r.z - u.z);
    rlTexCoord2f(1, 0); rlVertex3f(p.x + r.x - u.x, p.y + r.y - u.y, p.z + r.z - u.z);
    rlTexCoord2f(1, 1); rlVertex3f(p.x + r.x + u.x, p.y + r.y + u.y, p.z + r.z + u.z);
    rlTexCoord2f(0, 1); rlVertex3f(p.x - r.x + u.x, p.y - r.y + u.y, p.z - r.z + u.z);
}

void Particles::Draw(const Camera3D& cam) {
    Shader sh = Rdr().particleShader;
    Vector3 fwd = Vector3Normalize(Vector3Subtract(cam.target, cam.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, cam.up));
    Vector3 up = Vector3CrossProduct(right, fwd);
    int locI = GetShaderLocation(sh, "uIntensity");
    rlDisableBackfaceCulling();
    for (int pass = 0; pass < 2; pass++) {
        bool add = pass == 1;
        float inten = add ? 6.0f : 1.0f;
        BeginShaderMode(sh);
        SetShaderValue(sh, locI, &inten, SHADER_UNIFORM_FLOAT);
        BeginBlendMode(add ? BLEND_ADDITIVE : BLEND_ALPHA);
        rlSetTexture(Rdr().radialTex.id);
        rlBegin(RL_QUADS);
        for (const Particle& p : ps_) {
            if (p.additive != add) continue;
            float lf = p.life / p.maxLife;
            Color c = p.color;
            float a = c.a / 255.0f;
            if (p.type == P_SMOKE) a *= lf * Saturate((1 - lf) * 4);
            else if (p.type == P_FIRE) { a *= lf; c.g = (unsigned char)(c.g * (0.4f + 0.6f * lf)); }
            else if (p.type == P_DUST) a *= Saturate(lf * 3) * Saturate((1 - lf) * 3);
            else a *= Saturate(lf * 3.0f);
            c.a = (unsigned char)(a * 255);
            if (p.type == P_SPARK || p.type == P_GLASS) {
                // velocity-stretched streak
                Vector3 v = p.vel;
                float sp = Vector3Length(v);
                Vector3 dir = sp > 0.01f ? Vector3Scale(v, 1.0f / sp) : up;
                Vector3 side = Vector3Normalize(Vector3CrossProduct(dir, fwd));
                Quad(p.pos, Vector3Scale(side, p.size), Vector3Scale(dir, p.size + sp * 0.012f), c);
            } else {
                Quad(p.pos, Vector3Scale(right, p.size), Vector3Scale(up, p.size), c);
            }
        }
        rlEnd();
        rlSetTexture(0);
        EndBlendMode();
        EndShaderMode();
    }
    // rain streaks
    if (!rain_.empty()) {
        BeginShaderMode(sh);
        float inten = 1.4f;
        SetShaderValue(sh, locI, &inten, SHADER_UNIFORM_FLOAT);
        BeginBlendMode(BLEND_ALPHA);
        rlSetTexture(Rdr().radialTex.id);
        rlBegin(RL_QUADS);
        Vector3 dir = Vector3Normalize({ 0.08f, -1.0f, 0.0f });
        Vector3 side = Vector3Normalize(Vector3CrossProduct(dir, fwd));
        for (auto& d : rain_) Quad(d.p, Vector3Scale(side, 0.004f), Vector3Scale(dir, 0.22f), Color{ 170, 180, 195, 70 });
        rlEnd();
        rlSetTexture(0);
        EndBlendMode();
        EndShaderMode();
    }
    rlEnableBackfaceCulling();
}

// ---------------------------------------------------------------------------
// Decals
// ---------------------------------------------------------------------------
void Decals::Pool(Vector3 c, float radius, int mat, uint32_t seed, float stretch, float yaw) {
    ModelBuilder mb;
    MeshBuilder& m = mb.M(mat);
    const int segs = 28;
    float top = c.y + 0.6f;
    Vector3 ctr{ c.x, SurfaceY(c.x, c.z, top) + 0.012f, c.z };
    std::vector<Vector3> ring;
    for (int i = 0; i <= segs; i++) {
        float a = (float)i / segs * 2 * PI;
        float r = radius * (0.65f + 0.45f * (Fbm2(cosf(a) * 1.5f + seed, sinf(a) * 1.5f, 3, 0, seed) * 0.5f + 0.5f));
        float lx = cosf(a) * r * stretch, lz = sinf(a) * r;
        float x = c.x + lx * cosf(yaw) - lz * sinf(yaw), z = c.z + lx * sinf(yaw) + lz * cosf(yaw);
        ring.push_back({ x, SurfaceY(x, z, top) + 0.012f, z });
    }
    for (int i = 0; i < segs; i++) m.TriN(ctr, ring[i + 1], ring[i], { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 });
    // satellite droplets
    Rng r(seed);
    for (int k = 0; k < 10; k++) {
        float a = r.Range(0, 6.28f), d = radius * r.Range(1.0f, 1.8f), s = radius * r.Range(0.04f, 0.12f);
        Vector3 p{ c.x + cosf(a) * d, 0, c.z + sinf(a) * d };
        p.y = SurfaceY(p.x, p.z, top) + 0.012f;
        for (int i = 0; i < 6; i++) {
            float a0 = i * PI / 3, a1 = (i + 1) * PI / 3;
            m.TriN(p, { p.x + cosf(a1) * s, p.y, p.z + sinf(a1) * s }, { p.x + cosf(a0) * s, p.y, p.z + sinf(a0) * s }, { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 });
        }
    }
    models.push_back(mb.Build(false));
    visible.push_back(true);
    alpha.push_back(1.0f);
}

void Decals::Trail(const std::vector<Vector3>& pts, float width, int mat, uint32_t seed, float gaps) {
    ModelBuilder mb;
    MeshBuilder& m = mb.M(mat);
    Rng r(seed);
    // resample every 0.25 m
    std::vector<Vector3> P;
    for (size_t i = 0; i + 1 < pts.size(); i++) {
        float len = Vector3Distance(pts[i], pts[i + 1]);
        int n = (int)(len / 0.25f) + 1;
        for (int k = 0; k < n; k++) P.push_back(Vector3Lerp(pts[i], pts[i + 1], (float)k / n));
    }
    P.push_back(pts.back());
    for (size_t i = 0; i + 1 < P.size(); i++) {
        float n = Fbm2(i * 0.08f, 0.5f, 3, 0, seed);
        if (n < -0.5f + gaps * 0.5f) continue;   // the smear breaks up
        float w0 = width * (0.5f + 0.6f * (n * 0.5f + 0.5f));
        float w1 = width * (0.5f + 0.6f * (Fbm2((i + 1) * 0.08f, 0.5f, 3, 0, seed) * 0.5f + 0.5f));
        Vector3 d = Vector3Subtract(P[i + 1], P[i]); d.y = 0;
        Vector3 side = Vector3Normalize({ -d.z, 0, d.x });
        for (int lane = -1; lane <= 1; lane += 2) {   // two streaks (heels / body edges)
            float off = lane * width * 0.35f + Fbm2(i * 0.3f, lane * 3.0f, 2, 0, seed) * 0.05f;
            Vector3 a = Vector3Add(P[i], Vector3Scale(side, off - w0 * 0.3f)), b = Vector3Add(P[i], Vector3Scale(side, off + w0 * 0.3f));
            Vector3 c = Vector3Add(P[i + 1], Vector3Scale(side, off + w1 * 0.3f)), e = Vector3Add(P[i + 1], Vector3Scale(side, off - w1 * 0.3f));
            for (Vector3* v : { &a, &b, &c, &e }) v->y = SurfaceY(v->x, v->z, v->y + 0.6f) + 0.013f;
            m.TriN(a, c, e, { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 });
            m.TriN(a, b, c, { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 });
        }
        // drips and palm smears
        if (r.Chance(0.04f)) {
            Vector3 p = Vector3Add(P[i], Vector3Scale(side, r.Range(-width, width)));
            float s = r.Range(0.03f, 0.08f);
            p.y = SurfaceY(p.x, p.z, p.y + 0.6f) + 0.013f;
            for (int k = 0; k < 6; k++) {
                float a0 = k * PI / 3, a1 = (k + 1) * PI / 3;
                m.TriN(p, { p.x + cosf(a1) * s, p.y, p.z + sinf(a1) * s }, { p.x + cosf(a0) * s, p.y, p.z + sinf(a0) * s }, { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 });
            }
        }
    }
    models.push_back(mb.Build(false));
    visible.push_back(true);
    alpha.push_back(1.0f);
}

void Decals::Skid(const std::vector<Vector3>& pts, float width) {
    Trail(pts, width, MAT_RUBBER, 77, 0.0f);
}

void Decals::Draw() {
    for (size_t i = 0; i < models.size(); i++)
        if (visible[i]) Rdr().Draw(models[i], MatrixIdentity(), WHITE, false);
}

void Decals::Clear() { models.clear(); visible.clear(); alpha.clear(); }
