#include "characters.h"
#include "engine/sdf.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

using namespace sdf;

// ---------------------------------------------------------------------------
// Noise for skin / hair detail
// ---------------------------------------------------------------------------
static float VN3(Vector3 p) {
    int xi = (int)floorf(p.x), yi = (int)floorf(p.y), zi = (int)floorf(p.z);
    float fx = p.x - xi, fy = p.y - yi, fz = p.z - zi;
    auto h = [](int x, int y, int z) { return Hash01(Hash2i(x, y, (uint32_t)z * 7919u)); };
    fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy); fz = fz * fz * (3 - 2 * fz);
    float a = Lerp(Lerp(h(xi, yi, zi), h(xi + 1, yi, zi), fx), Lerp(h(xi, yi + 1, zi), h(xi + 1, yi + 1, zi), fx), fy);
    float b = Lerp(Lerp(h(xi, yi, zi + 1), h(xi + 1, yi, zi + 1), fx), Lerp(h(xi, yi + 1, zi + 1), h(xi + 1, yi + 1, zi + 1), fx), fy);
    return Lerp(a, b, fz) * 2.0f - 1.0f;
}
static float FBM3(Vector3 p) { return VN3(p) * 0.6f + VN3(Vector3Scale(p, 2.1f)) * 0.3f + VN3(Vector3Scale(p, 4.3f)) * 0.1f; }

// ---------------------------------------------------------------------------
// Pose helpers
// ---------------------------------------------------------------------------
Pose PoseLerp(const Pose& a, const Pose& b, float t) {
    Pose p;
    for (int i = 0; i < B_COUNT; i++) p.rot[i] = Vector3Lerp(a.rot[i], b.rot[i], t);
    p.root = Vector3Lerp(a.root, b.root, t);
    p.rootYaw = Lerp(a.rootYaw, b.rootYaw, t);
    return p;
}

static Pose Rest() {
    Pose p;
    p.rot[B_LUARM].z = 0.09f; p.rot[B_RUARM].z = -0.09f;
    p.rot[B_LFARM].x = -0.12f; p.rot[B_RFARM].x = -0.12f;
    return p;
}

Pose PoseStand(float t, float breath) {
    Pose p = Rest();
    float b = sinf(t * 1.55f) * breath;
    p.rot[B_CHEST].x = -0.02f - b * 0.012f;
    p.rot[B_NECK].x = b * 0.006f;
    p.rot[B_LUARM].z += b * 0.01f; p.rot[B_RUARM].z -= b * 0.01f;
    float shift = sinf(t * 0.31f) * 0.025f;
    p.rot[B_PELVIS].z = shift;
    p.rot[B_LTHIGH].z = -shift; p.rot[B_RTHIGH].z = -shift;
    p.rot[B_HEAD].y = sinf(t * 0.23f) * 0.06f;
    p.rot[B_HEAD].x = sinf(t * 0.17f) * 0.03f;
    p.root.y = b * 0.003f;
    return p;
}

Pose PoseWalk(float phase, float amt, int style) {
    Pose p = Rest();
    float f = phase * 2 * PI;
    float stride = style == 1 ? 0.58f : (style == 3 ? 0.35f : 0.45f);
    float armSw = style == 1 ? 0.1f : (style == 3 ? 0.18f : 0.34f);
    p.rot[B_LTHIGH].x = -sinf(f) * stride * amt;
    p.rot[B_RTHIGH].x = sinf(f) * stride * amt;
    p.rot[B_LSHIN].x = (1 - cosf(f - 0.6f)) * 0.42f * amt * (sinf(f) > -0.2f ? 1.0f : 0.35f);
    p.rot[B_RSHIN].x = (1 - cosf(f + PI - 0.6f)) * 0.42f * amt * (sinf(f + PI) > -0.2f ? 1.0f : 0.35f);
    p.rot[B_LFOOT].x = -(p.rot[B_LTHIGH].x + p.rot[B_LSHIN].x) * 0.5f;
    p.rot[B_RFOOT].x = -(p.rot[B_RTHIGH].x + p.rot[B_RSHIN].x) * 0.5f;
    p.rot[B_LUARM].x = sinf(f) * armSw * amt;
    p.rot[B_RUARM].x = -sinf(f) * armSw * amt;
    p.rot[B_LFARM].x = -0.2f - fmaxf(0.0f, -sinf(f)) * 0.3f * amt;
    p.rot[B_RFARM].x = -0.2f - fmaxf(0.0f, sinf(f)) * 0.3f * amt;
    p.rot[B_SPINE].y = sinf(f) * 0.07f * amt;
    p.rot[B_PELVIS].y = -sinf(f) * 0.08f * amt;
    p.rot[B_PELVIS].z = cosf(f) * 0.035f * amt;
    p.root.y = (cosf(2 * f) * 0.022f - 0.012f) * amt;
    if (style == 1) {  // stalk: hunched, head thrust forward, arms dead
        p.rot[B_SPINE].x = 0.28f; p.rot[B_CHEST].x = 0.18f; p.rot[B_NECK].x = -0.1f; p.rot[B_HEAD].x = -0.35f;
        p.rot[B_LFARM].x = -0.05f; p.rot[B_RFARM].x = -0.05f;
        p.rot[B_HEAD].z = sinf(f * 0.5f) * 0.12f;
    } else if (style == 3) {
        p.rot[B_SPINE].x = 0.35f; p.rot[B_CHEST].x = 0.12f; p.rot[B_HEAD].x = -0.3f;
    } else if (style == 2) {
        p.rot[B_RTHIGH].x *= 0.5f; p.rot[B_RSHIN].x *= 0.3f; p.rot[B_PELVIS].z += sinf(f) * 0.06f;
    }
    return p;
}

Pose PoseRun(float phase) {
    Pose p = PoseWalk(phase, 1.6f, 0);
    p.rot[B_SPINE].x = 0.18f;
    p.rot[B_LFARM].x = -1.3f; p.rot[B_RFARM].x = -1.3f;
    p.root.y -= 0.04f;
    return p;
}

Pose PoseCrawl(float phase) {
    Pose p = Rest();
    float f = phase * 2 * PI;
    p.rot[B_PELVIS].x = 1.35f;
    p.rot[B_SPINE].x = 0.05f;
    p.rot[B_NECK].x = -0.7f; p.rot[B_HEAD].x = -0.6f;
    p.rot[B_HEAD].z = sinf(f * 0.5f) * 0.25f;    // twitching head tilt
    p.rot[B_LTHIGH].x = -1.35f - 0.35f + sinf(f) * 0.35f; p.rot[B_RTHIGH].x = -1.35f - 0.35f - sinf(f) * 0.35f;
    p.rot[B_LTHIGH].z = 0.35f; p.rot[B_RTHIGH].z = -0.35f;
    p.rot[B_LSHIN].x = 1.9f; p.rot[B_RSHIN].x = 1.9f;
    p.rot[B_LFOOT].x = 0.6f; p.rot[B_RFOOT].x = 0.6f;
    p.rot[B_LUARM].x = -1.45f - sinf(f) * 0.35f; p.rot[B_RUARM].x = -1.45f + sinf(f) * 0.35f;
    p.rot[B_LUARM].z = 0.3f; p.rot[B_RUARM].z = -0.3f;
    p.rot[B_LFARM].x = 0.15f + fmaxf(0.0f, sinf(f)) * 0.5f; p.rot[B_RFARM].x = 0.15f + fmaxf(0.0f, -sinf(f)) * 0.5f;
    p.root.y = -0.35f;
    p.root.z = -0.1f;
    return p;
}

static Pose SitBase() {
    Pose p = Rest();
    p.rot[B_LTHIGH].x = -1.5f; p.rot[B_RTHIGH].x = -1.5f;
    p.rot[B_LTHIGH].z = 0.06f; p.rot[B_RTHIGH].z = -0.06f;
    p.rot[B_LSHIN].x = 1.25f; p.rot[B_RSHIN].x = 1.25f;
    p.rot[B_LFOOT].x = 0.15f; p.rot[B_RFOOT].x = 0.15f;
    p.rot[B_SPINE].x = -0.12f;
    return p;
}

Pose PoseSitDrive(float t, float steer) {
    Pose p = SitBase();
    p.rot[B_LUARM].x = -0.95f + steer * 0.25f; p.rot[B_RUARM].x = -0.95f - steer * 0.25f;
    p.rot[B_LUARM].z = 0.05f; p.rot[B_RUARM].z = -0.05f;
    p.rot[B_LFARM].x = -0.75f; p.rot[B_RFARM].x = -0.75f;
    p.rot[B_CHEST].x = -0.02f + sinf(t * 1.5f) * 0.01f;
    p.rot[B_HEAD].y = steer * 0.2f;
    return p;
}

Pose PoseSitPassenger(float t, float laugh) {
    Pose p = SitBase();
    p.rot[B_LUARM].x = -0.35f; p.rot[B_RUARM].x = -0.35f;
    p.rot[B_LFARM].x = -1.1f; p.rot[B_RFARM].x = -1.0f;
    p.rot[B_LUARM].z = -0.05f; p.rot[B_RUARM].z = 0.05f;
    float shake = sinf(t * 15.0f) * laugh;
    p.rot[B_CHEST].x = -0.03f + shake * 0.03f - laugh * 0.1f;
    p.rot[B_HEAD].x = -laugh * 0.18f + shake * 0.03f;
    p.rot[B_NECK].x = -laugh * 0.08f;
    p.rot[B_LUARM].z += shake * 0.02f;
    p.root.y = shake * 0.004f;
    return p;
}

Pose PoseSlumped(float) {
    Pose p = SitBase();
    p.rot[B_SPINE].x = 0.3f; p.rot[B_CHEST].x = 0.28f;
    p.rot[B_NECK].x = 0.45f; p.rot[B_HEAD].x = 0.4f; p.rot[B_HEAD].z = 0.35f; p.rot[B_HEAD].y = -0.2f;
    p.rot[B_LUARM].x = -0.25f; p.rot[B_RUARM].x = -0.1f; p.rot[B_LUARM].z = 0.02f; p.rot[B_RUARM].z = -0.25f;
    p.rot[B_LFARM].x = -0.2f; p.rot[B_RFARM].x = -0.05f;
    p.rot[B_LHAND].x = 0.3f; p.rot[B_RHAND].x = 0.3f;
    return p;
}

Pose PoseLyingBack(float t) {
    Pose p = Rest();
    p.rot[B_LUARM].z = 0.45f; p.rot[B_RUARM].z = -0.3f;
    p.rot[B_LFARM].x = -0.3f; p.rot[B_RFARM].x = -0.1f;
    p.rot[B_HEAD].y = 0.35f; p.rot[B_HEAD].x = -0.1f;
    p.rot[B_LTHIGH].z = 0.08f; p.rot[B_RTHIGH].z = -0.12f;
    p.rot[B_LSHIN].x = 0.1f;
    p.rot[B_CHEST].x = sinf(t * 1.2f) * 0.01f;
    p.rot[B_LFOOT].x = 0.6f; p.rot[B_RFOOT].x = 0.6f;
    return p;
}

Pose PoseDragged(float t) {
    Pose p = PoseLyingBack(t);
    p.rot[B_LTHIGH].x = -0.55f;      // held by the ankle
    p.rot[B_LSHIN].x = 0.0f;
    p.rot[B_LUARM].x = -2.7f; p.rot[B_RUARM].x = -2.9f;   // arms trailing above the head, limp (not fighting)
    p.rot[B_LUARM].z = 0.25f; p.rot[B_RUARM].z = -0.2f;
    p.rot[B_LFARM].x = -0.2f; p.rot[B_RFARM].x = -0.3f;
    p.rot[B_HEAD].y = 0.5f + sinf(t * 2.3f) * 0.08f;
    return p;
}

Pose PoseDragging(float phase) {
    Pose p = PoseWalk(phase, 0.8f, 1);
    p.rot[B_RUARM].x = 0.55f; p.rot[B_RUARM].z = -0.15f;
    p.rot[B_RFARM].x = -0.1f;
    p.rot[B_RHAND].x = 0.4f;
    p.rot[B_SPINE].x = 0.42f;
    return p;
}

Pose PoseTiedChair(float t, float struggle) {
    Pose p = SitBase();
    p.rot[B_SPINE].x = 0.0f;
    p.rot[B_LUARM].x = 0.45f; p.rot[B_RUARM].x = 0.45f;
    p.rot[B_LUARM].z = 0.12f; p.rot[B_RUARM].z = -0.12f;
    p.rot[B_LFARM].x = -1.25f; p.rot[B_RFARM].x = -1.25f;
    p.rot[B_NECK].x = 0.35f; p.rot[B_HEAD].x = 0.3f;
    float s = sinf(t * 9.0f) * struggle;
    p.rot[B_CHEST].y = s * 0.08f; p.rot[B_HEAD].y = s * 0.15f;
    return p;
}

Pose PoseCounterLean(float t) {
    Pose p = PoseStand(t, 0.6f);
    p.rot[B_SPINE].x = 0.22f; p.rot[B_CHEST].x = 0.1f;
    p.rot[B_LUARM].x = -0.75f; p.rot[B_RUARM].x = -0.75f;
    p.rot[B_LUARM].z = -0.12f; p.rot[B_RUARM].z = 0.12f;
    p.rot[B_LFARM].x = -0.55f; p.rot[B_RFARM].x = -0.55f;
    p.rot[B_LHAND].x = -1.2f; p.rot[B_RHAND].x = -1.2f;
    p.rot[B_NECK].x = -0.2f; p.rot[B_HEAD].x = -0.15f; p.rot[B_HEAD].z = 0.14f;
    return p;
}

Pose PoseLoom(float t) {
    // the Dragger at rest: hunched, head cocked, arms hanging past the knees, a slow sway
    Pose p = PoseStand(t, 0.4f);
    p.rot[B_SPINE].x = 0.3f; p.rot[B_CHEST].x = 0.2f;
    p.rot[B_NECK].x = -0.25f; p.rot[B_HEAD].x = -0.2f; p.rot[B_HEAD].z = 0.45f + sinf(t * 0.6f) * 0.05f;
    p.rot[B_LUARM].x = 0.1f; p.rot[B_RUARM].x = 0.15f; p.rot[B_LUARM].z = 0.05f; p.rot[B_RUARM].z = -0.05f;
    p.rot[B_LFARM].x = -0.05f; p.rot[B_RFARM].x = -0.15f;
    p.rot[B_LHAND].x = 0.3f; p.rot[B_RHAND].x = 0.4f;
    p.rot[B_PELVIS].z = sinf(t * 0.45f) * 0.03f;
    return p;
}

Pose PoseReach(float t) {
    Pose p = PoseStand(t, 1.0f);
    p.rot[B_RUARM].x = -1.4f; p.rot[B_RUARM].z = -0.05f;
    p.rot[B_RFARM].x = -0.15f;
    p.rot[B_SPINE].x = 0.1f;
    return p;
}

Pose PoseKneel(float t) {
    Pose p = Rest();
    p.rot[B_LTHIGH].x = -0.05f; p.rot[B_RTHIGH].x = -0.05f;
    p.rot[B_LSHIN].x = 1.55f; p.rot[B_RSHIN].x = 1.55f;
    p.rot[B_LFOOT].x = 0.5f; p.rot[B_RFOOT].x = 0.5f;
    p.rot[B_CHEST].x = sinf(t * 1.5f) * 0.01f;
    p.root.y = -0.4f;
    return p;
}

Pose PoseCry(float t) {
    Pose p = PoseKneel(t);
    float sob = fmaxf(0.0f, sinf(t * 8.0f)) * (0.6f + 0.4f * sinf(t * 0.9f));
    p.rot[B_SPINE].x = 0.35f + sob * 0.05f; p.rot[B_CHEST].x = 0.25f + sob * 0.06f;
    p.rot[B_NECK].x = 0.3f; p.rot[B_HEAD].x = 0.35f;
    p.rot[B_LUARM].x = -1.15f; p.rot[B_RUARM].x = -1.15f;
    p.rot[B_LUARM].z = -0.25f; p.rot[B_RUARM].z = 0.25f;
    p.rot[B_LFARM].x = -1.9f; p.rot[B_RFARM].x = -1.9f;
    p.root.y += sob * 0.01f;
    return p;
}

Pose PoseCoverOver(float t) {
    Pose p = PoseKneel(t);
    p.rot[B_SPINE].x = 0.7f; p.rot[B_CHEST].x = 0.35f;
    p.rot[B_NECK].x = 0.3f; p.rot[B_HEAD].x = 0.2f;
    p.rot[B_LUARM].x = -1.0f; p.rot[B_RUARM].x = -1.0f;
    p.rot[B_LUARM].z = -0.55f; p.rot[B_RUARM].z = 0.55f;
    p.rot[B_LFARM].x = -1.2f; p.rot[B_RFARM].x = -1.2f;
    float shake = sinf(t * 21.0f) * 0.02f;
    p.rot[B_CHEST].y = shake;
    return p;
}

Pose PoseRide(float t) {
    Pose p = SitBase();
    p.rot[B_LTHIGH].z = 0.45f; p.rot[B_RTHIGH].z = -0.45f;
    p.rot[B_LUARM].x = -0.7f; p.rot[B_RUARM].x = -1.2f;
    p.rot[B_RFARM].x = -0.4f + sinf(t * 2.0f) * 0.4f;   // whipping arm
    p.rot[B_LFARM].x = -0.8f;
    p.rot[B_SPINE].x = 0.05f;
    p.rot[B_HEAD].y = sinf(t * 0.4f) * 0.3f;
    return p;
}

Pose PoseHarnessed(float phase) {
    Pose p = PoseCrawl(phase * 0.7f);
    p.rot[B_NECK].x = -0.2f; p.rot[B_HEAD].x = 0.1f;   // head bowed under the yoke
    p.root.y = -0.3f;
    return p;
}

// ---------------------------------------------------------------------------
// Sculpting
// ---------------------------------------------------------------------------
// Parts are collected as jobs, polygonised with the work split across cores (the SDF fields
// capture by value, so they are safe to evaluate concurrently) and uploaded afterwards on the
// main thread, in the original order.
struct SculptJob {
    int bone, mat;
    Field f;
    Vector3 bmin, bmax;
    float cell;
    MeshBuilder mb;
    MeshBuilder lod[2];   // same surface at 2.5x and 6x the cell size, for distance and shadows
};

// Beyond these distances a level's triangles are smaller than a pixel at 720p, so the next,
// coarser level looks the same.
static const float kLodCell[2] = { 2.5f, 6.0f };
static const float kLodDist[2] = { 5.0f, 14.0f };

struct CharBuild {
    CharModel* cm = nullptr;
    std::vector<std::unique_ptr<SculptJob>> jobs;
};

struct Sculptor {
    CharBuild* cb;
    void Part(int bone, int mat, const Field& f, Vector3 bmin, Vector3 bmax, float cell) {
        auto j = std::make_unique<SculptJob>();
        j->bone = bone; j->mat = mat; j->f = f; j->bmin = bmin; j->bmax = bmax; j->cell = cell;
        cb->jobs.push_back(std::move(j));
    }
};

static void RunSculptJobs(CharBuild* b, int maxThreads) {
    for (auto& j : b->jobs) {
        Polygonise(j->mb, j->f, j->bmin, j->bmax, j->cell, maxThreads);
        for (int l = 0; l < 2; l++) Polygonise(j->lod[l], j->f, j->bmin, j->bmax, j->cell * kLodCell[l], maxThreads);
        j->f = nullptr;
    }
}

static int g_pigMat = -1, g_goatMat = -1, g_hornMat = -1;
static void EnsureCreatureMats() {
    if (g_pigMat >= 0) return;
    SurfaceMat pig = Mat(MAT_SKIN); pig.name = "pig_skin"; pig.tint = Color{ 205, 150, 140, 255 }; pig.wet = 0.25f;
    g_pigMat = AddMaterial(pig);
    SurfaceMat goat = Mat(MAT_HAIR); goat.name = "goat_fur"; goat.tint = Color{ 120, 110, 95, 255 }; goat.scale = 5.0f; goat.normalStr = 2.0f;
    g_goatMat = AddMaterial(goat);
    SurfaceMat horn = Mat(MAT_BONE); horn.name = "horn"; horn.tint = Color{ 90, 80, 65, 255 };
    g_hornMat = AddMaterial(horn);
}

static void SculptCharacter(CharBuild* cb, const BodySpec& sp) {
    CharModel* cm = cb->cm;
    cm->spec = sp;
    const float s = sp.height / 1.75f;
    const float L = sp.limbLen;
    const float rm = Lerp(1.12f, 0.68f, sp.thin);
    const float sw = sp.shoulders;
    float thighL = 0.43f * s * L, shinL = 0.42f * s * L, footH = 0.08f * s;
    float uarmL = 0.29f * s * L, farmL = 0.26f * s * L;
    cm->hipHeight = thighL + shinL + footH + 0.05f * s;
    cm->offset[B_PELVIS] = { 0, cm->hipHeight, 0 };
    cm->offset[B_SPINE] = { 0, 0.10f * s, 0 };
    cm->offset[B_CHEST] = { 0, 0.22f * s, 0 };
    cm->offset[B_NECK] = { 0, 0.225f * s, 0.0f };
    cm->offset[B_HEAD] = { 0, 0.075f * s * sp.neckLen, 0.012f * s };
    cm->offset[B_LUARM] = { 0.185f * s * sw, 0.205f * s, 0 };
    cm->offset[B_RUARM] = { -0.185f * s * sw, 0.205f * s, 0 };
    cm->offset[B_LFARM] = cm->offset[B_RFARM] = { 0, -uarmL, 0 };
    cm->offset[B_LHAND] = cm->offset[B_RHAND] = { 0, -farmL, 0 };
    cm->offset[B_LTHIGH] = { 0.095f * s, -0.05f * s, 0 };
    cm->offset[B_RTHIGH] = { -0.095f * s, -0.05f * s, 0 };
    cm->offset[B_LSHIN] = cm->offset[B_RSHIN] = { 0, -thighL, 0 };
    cm->offset[B_LFOOT] = cm->offset[B_RFOOT] = { 0, -shinL, 0 };

    Sculptor sc{ cb };
    const float cBody = 0.013f * s, cFine = 0.0055f * s;
    bool clothedTop = sp.top != TOP_BARE;
    int topMat = clothedTop ? sp.topMat : sp.skinMat;
    if (sp.top == TOP_RAGS) topMat = MAT_CLOTH_BROWN;
    int pantsMat = sp.pants ? sp.pantsMat : sp.skinMat;
    float clothPad = (sp.top == TOP_HOODIE || sp.top == TOP_JACKET) ? 0.018f : (clothedTop ? 0.006f : 0.0f);
    uint32_t seed = sp.seed;

    // ---- pelvis
    sc.Part(B_PELVIS, pantsMat, [=](Vector3 p) {
        float d = Ellipsoid(p, { 0, -0.02f * s, -0.005f }, { 0.155f * s * rm, 0.115f * s, 0.105f * s * rm });
        d = SUnion(d, Sphere(p, { 0.095f * s, -0.07f * s, 0 }, 0.088f * s * rm), 0.04f * s);
        d = SUnion(d, Sphere(p, { -0.095f * s, -0.07f * s, 0 }, 0.088f * s * rm), 0.04f * s);
        d = SUnion(d, Ellipsoid(p, { 0, -0.05f * s, -0.05f * s }, { 0.13f * s * rm, 0.09f * s, 0.07f * s * rm }), 0.03f * s);
        if (sp.pants) d = SUnion(d, Box(p, { 0, 0.07f * s, 0 }, { 0.15f * s * rm, 0.02f * s, 0.105f * s * rm }, 0.015f * s), 0.01f * s);   // waistband
        return d;
    }, { -0.3f * s, -0.22f * s, -0.22f * s }, { 0.3f * s, 0.12f * s, 0.2f * s }, cBody);
    if (sp.top == TOP_SHIRT_APRON || sp.top == TOP_OVERALLS)
        sc.Part(B_PELVIS, sp.accentMat, [=](Vector3 p) {
            return Box(p, { 0, -0.12f * s, 0.105f * s * rm + 0.012f }, { 0.13f * s, 0.2f * s, 0.008f }, 0.004f);
        }, { -0.2f * s, -0.4f * s, 0.0f }, { 0.2f * s, 0.12f * s, 0.2f * s }, cBody);

    // ---- abdomen
    sc.Part(B_SPINE, topMat, [=](Vector3 p) {
        Vector3 q{ p.x, p.y, p.z / 0.74f };
        float d = RoundCone(q, { 0, -0.03f * s, 0 }, { 0, 0.21f * s, 0 }, 0.13f * s * rm + clothPad, 0.14f * s * rm + clothPad) * 0.8f;
        if (sp.thin > 0.75f && !clothedTop) d = SSub(d, Ellipsoid(p, { 0, 0.07f * s, 0.13f * s * rm }, { 0.1f * s, 0.08f * s, 0.05f * s }), 0.03f * s);
        return d;
    }, { -0.25f * s, -0.1f * s, -0.2f * s }, { 0.25f * s, 0.3f * s, 0.2f * s }, cBody);

    // ---- chest + shoulders
    sc.Part(B_CHEST, topMat, [=](Vector3 p) {
        float d = Ellipsoid(p, { 0, 0.11f * s, 0 }, { 0.16f * s * rm * sw + clothPad, 0.165f * s, 0.105f * s * rm + clothPad });
        d = SUnion(d, RoundCone(p, { 0.0f, 0.19f * s, -0.01f * s }, { 0.185f * s * sw, 0.2f * s, 0 }, 0.07f * s * rm + clothPad, 0.058f * s * rm + clothPad), 0.05f * s);
        d = SUnion(d, RoundCone(p, { 0.0f, 0.19f * s, -0.01f * s }, { -0.185f * s * sw, 0.2f * s, 0 }, 0.07f * s * rm + clothPad, 0.058f * s * rm + clothPad), 0.05f * s);
        if (sp.ribs && !clothedTop) {
            float band = sinf(p.y / s * 95.0f) * 0.5f + 0.5f;
            float front = SmoothStep(-0.02f, 0.08f * s, p.z) + SmoothStep(0.05f * s, 0.12f * s, fabsf(p.x));
            d += (band * band) * 0.006f * s * Saturate(front) * SmoothStep(0.25f * s, 0.05f * s, p.y);
            d = SUnion(d, Capsule(p, { -0.09f * s, 0.24f * s, 0.07f * s }, { 0.09f * s, 0.24f * s, 0.07f * s }, 0.012f * s), 0.01f * s);  // collarbones
        }
        if (sp.top == TOP_HOODIE) d = SUnion(d, Ellipsoid(p, { 0, 0.27f * s, -0.075f * s }, { 0.12f * s, 0.065f * s, 0.075f * s }), 0.03f * s);
        if (sp.top == TOP_JACKET) d = SUnion(d, RoundCone(p, { -0.08f * s, 0.27f * s, 0.02f * s }, { 0.08f * s, 0.27f * s, 0.02f * s }, 0.03f * s, 0.03f * s), 0.02f * s);
        d += FBM3(Vector3Scale(p, 40.0f)) * (clothedTop ? 0.0025f : 0.0012f);   // fabric folds / skin
        return d;
    }, { -0.34f * s, -0.1f * s, -0.22f * s }, { 0.34f * s, 0.36f * s, 0.2f * s }, cBody);
    if (sp.top == TOP_SHIRT_APRON || sp.top == TOP_OVERALLS)
        sc.Part(B_CHEST, sp.accentMat, [=](Vector3 p) {
            float d = Box(p, { 0, 0.1f * s, 0.1f * s * rm + 0.015f }, { 0.11f * s, 0.13f * s, 0.008f }, 0.004f);
            d = Union(d, Capsule(p, { 0.1f * s, 0.23f * s, 0.09f * s }, { 0.1f * s, 0.26f * s, -0.08f * s }, 0.012f * s));
            d = Union(d, Capsule(p, { -0.1f * s, 0.23f * s, 0.09f * s }, { -0.1f * s, 0.26f * s, -0.08f * s }, 0.012f * s));
            return d;
        }, { -0.2f * s, -0.1f * s, -0.15f * s }, { 0.2f * s, 0.32f * s, 0.2f * s }, cBody);

    // ---- neck
    sc.Part(B_NECK, sp.skinMat, [=](Vector3 p) {
        float d = RoundCone(p, { 0, -0.03f * s, -0.005f * s }, { 0, 0.09f * s * sp.neckLen, 0.012f * s }, 0.056f * s * rm, 0.047f * s * rm);
        d = SUnion(d, Sphere(p, { 0, 0.05f * s, 0.035f * s * rm }, 0.012f * s), 0.01f * s);   // larynx
        if (sp.thin > 0.6f) {
            d = SUnion(d, Capsule(p, { 0.03f * s, 0.0f, 0.03f * s }, { 0.02f * s, 0.1f * s * sp.neckLen, 0.0f }, 0.012f * s), 0.01f * s);
            d = SUnion(d, Capsule(p, { -0.03f * s, 0.0f, 0.03f * s }, { -0.02f * s, 0.1f * s * sp.neckLen, 0.0f }, 0.012f * s), 0.01f * s);
        }
        return d;
    }, { -0.1f * s, -0.08f * s, -0.1f * s }, { 0.1f * s, 0.2f * s * sp.neckLen, 0.1f * s }, cFine * 1.5f);

    // ---- head
    const float hs = sp.headScale * s;
    int headMat = sp.skinMat;
    if (sp.head == HEAD_PIG) headMat = g_pigMat;
    if (sp.head == HEAD_GOAT || sp.head == HEAD_DOG) headMat = g_goatMat;
    // Eye placement shared by the face sculpt and the eyeball meshes
    const float eyeR = 0.0122f * hs;
    const Vector3 eyeC1{ 0.0315f * hs, 0.099f * hs, 0.079f * hs - sp.eyeSink * 0.006f * hs };
    auto humanHead = [=](Vector3 p) {
        // skull and face masses
        float d = Ellipsoid(p, { 0, 0.12f * hs, -0.016f * hs }, { 0.074f * hs, 0.096f * hs, 0.092f * hs });
        d = SUnion(d, Ellipsoid(p, { 0, 0.074f * hs, 0.034f * hs }, { 0.063f * hs * rm, 0.07f * hs, 0.062f * hs }), 0.028f * hs);
        // mandible: wedge + gonial angles + chin
        float jawY = 0.004f * hs - sp.jawDrop * 0.045f * hs;
        d = SUnion(d, RoundCone(p, { 0, 0.06f * hs, -0.004f * hs }, { 0, jawY + 0.006f * hs, 0.068f * hs }, 0.047f * hs * rm, 0.019f * hs), 0.02f * hs);
        d = SUnion(d, Sphere(p, { 0.047f * hs * rm, 0.046f * hs, -0.012f * hs }, 0.014f * hs), 0.018f * hs);
        d = SUnion(d, Sphere(p, { -0.047f * hs * rm, 0.046f * hs, -0.012f * hs }, 0.014f * hs), 0.018f * hs);
        d = SUnion(d, Ellipsoid(p, { 0, 0.038f * hs - sp.jawDrop * 0.02f * hs, 0.032f * hs }, { 0.05f * hs * rm, 0.036f * hs, 0.05f * hs }), 0.024f * hs);
        d = SUnion(d, Sphere(p, { 0, jawY + 0.008f * hs, 0.07f * hs }, 0.016f * hs), 0.016f * hs);
        if (sp.head == HEAD_FACELESS) {
            d += FBM3(Vector3Scale(p, 60.0f)) * 0.0006f;
            return d;
        }
        // cheekbones, cheeks, brow ridge
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            d = SUnion(d, Sphere(p, { sgn * 0.047f * hs, 0.083f * hs, 0.061f * hs }, 0.017f * hs), 0.018f * hs);
            d = SUnion(d, Sphere(p, { sgn * 0.036f * hs, 0.066f * hs, 0.068f * hs }, 0.015f * hs * (1.15f - sp.thin * 0.5f)), 0.018f * hs);
        }
        d = SUnion(d, Capsule(p, { -0.037f * hs, 0.119f * hs, 0.084f * hs }, { 0.037f * hs, 0.119f * hs, 0.084f * hs }, 0.0115f * hs), 0.02f * hs);
        // shallow orbital depressions, then eyelids wrapped round the eyeballs, then the almond slit
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            Vector3 ec{ sgn * eyeC1.x, eyeC1.y, eyeC1.z };
            d = SSub(d, Sphere(p, { ec.x, ec.y + 0.002f * hs, ec.z + 0.022f * hs }, 0.019f * hs + sp.eyeSink * 0.004f * hs), 0.016f * hs);
            d = SUnion(d, Sphere(p, ec, eyeR + 0.0026f * hs), 0.004f * hs);
            Vector3 slit{ ec.x, ec.y, ec.z + eyeR + 0.001f * hs };
            float open = sp.head == HEAD_HOLLOW ? 0.012f : (0.0052f + sp.eyeSink * 0.002f);
            d = SSub(d, Ellipsoid(p, slit, { 0.0128f * hs, open * hs, 0.0065f * hs }), 0.0015f * hs);
        }
        // maxilla: fills the area between cheeks and mouth so the mouth doesn't read as a muzzle
        d = SUnion(d, Ellipsoid(p, { 0, 0.056f * hs, 0.062f * hs }, { 0.047f * hs * rm, 0.028f * hs, 0.03f * hs }), 0.02f * hs);
        // nose: bridge, tip, alae, nostrils
        float nl = sp.noseLen;
        d = SUnion(d, RoundCone(p, { 0, 0.109f * hs, 0.091f * hs }, { 0, 0.074f * hs, (0.096f + 0.02f * nl) * hs }, 0.0072f * hs, 0.0095f * hs), 0.007f * hs);
        d = SUnion(d, Sphere(p, { 0, 0.072f * hs, (0.1f + 0.018f * nl) * hs }, 0.0088f * hs), 0.006f * hs);
        d = SUnion(d, Sphere(p, { 0.0105f * hs, 0.066f * hs, 0.102f * hs }, 0.0064f * hs), 0.005f * hs);
        d = SUnion(d, Sphere(p, { -0.0105f * hs, 0.066f * hs, 0.102f * hs }, 0.0064f * hs), 0.005f * hs);
        d = SSub(d, Ellipsoid(p, { 0.0068f * hs, 0.0605f * hs, 0.109f * hs }, { 0.0035f * hs, 0.0024f * hs, 0.0045f * hs }), 0.0015f * hs);
        d = SSub(d, Ellipsoid(p, { -0.0068f * hs, 0.0605f * hs, 0.109f * hs }, { 0.0035f * hs, 0.0024f * hs, 0.0045f * hs }), 0.0015f * hs);
        // lips and the mouth line (corners pull up/back with `smile`)
        float mw = 0.025f * hs * (1.0f + sp.smile * 0.6f);
        float my = 0.043f * hs - sp.jawDrop * 0.022f * hs;
        d = SUnion(d, Ellipsoid(p, { 0, 0.05f * hs, 0.087f * hs }, { 0.022f * hs, 0.011f * hs, 0.0075f * hs }), 0.008f * hs);
        d = SUnion(d, Ellipsoid(p, { 0, my - 0.006f * hs, 0.085f * hs }, { 0.018f * hs, 0.0065f * hs, 0.007f * hs }), 0.007f * hs);
        float cornerY = my + sp.smile * 0.006f * hs;
        d = SSub(d, Capsule(p, { -mw, cornerY, 0.094f * hs - sp.smile * 0.006f * hs }, { 0, my, 0.1f * hs }, 0.0017f * hs + sp.jawDrop * 0.012f * hs), 0.0018f * hs);
        d = SSub(d, Capsule(p, { 0, my, 0.1f * hs }, { mw, cornerY, 0.094f * hs - sp.smile * 0.006f * hs }, 0.0017f * hs + sp.jawDrop * 0.012f * hs), 0.0018f * hs);
        // ears
        for (int sgn = -1; sgn <= 1; sgn += 2) {
            d = SUnion(d, Ellipsoid(p, { sgn * 0.074f * hs, 0.094f * hs, -0.008f * hs }, { 0.0085f * hs, 0.029f * hs, 0.018f * hs }), 0.005f * hs);
            d = SSub(d, Ellipsoid(p, { sgn * 0.081f * hs, 0.092f * hs, -0.006f * hs }, { 0.004f * hs, 0.017f * hs, 0.01f * hs }), 0.002f * hs);
        }
        // temples hollow on thin faces
        if (sp.thin > 0.6f) for (int sgn = -1; sgn <= 1; sgn += 2)
            d = SSub(d, Sphere(p, { sgn * 0.083f * hs, 0.118f * hs, 0.04f * hs }, 0.02f * hs), 0.02f * hs);
        d += FBM3(Vector3Scale(p, 110.0f)) * 0.0004f * (1.0f + sp.thin * 2.0f);   // pores, age
        return d;
    };
    auto pigHead = [=](Vector3 p) {
        float d = Ellipsoid(p, { 0, 0.1f * hs, 0.0f }, { 0.095f * hs, 0.1f * hs, 0.11f * hs });
        d = SUnion(d, RoundCone(p, { 0, 0.07f * hs, 0.07f * hs }, { 0, 0.06f * hs, 0.2f * hs }, 0.06f * hs, 0.048f * hs), 0.04f * hs);
        d = fmaxf(d, p.z - 0.205f * hs);
        d = SSub(d, Sphere(p, { 0.016f * hs, 0.063f * hs, 0.21f * hs }, 0.011f * hs), 0.004f * hs);
        d = SSub(d, Sphere(p, { -0.016f * hs, 0.063f * hs, 0.21f * hs }, 0.011f * hs), 0.004f * hs);
        d = SSub(d, Sphere(p, { 0.045f * hs, 0.125f * hs, 0.09f * hs }, 0.013f * hs), 0.008f * hs);
        d = SSub(d, Sphere(p, { -0.045f * hs, 0.125f * hs, 0.09f * hs }, 0.013f * hs), 0.008f * hs);
        d = SSub(d, Capsule(p, { -0.045f * hs, 0.02f * hs, 0.14f * hs }, { 0.045f * hs, 0.02f * hs, 0.14f * hs }, 0.006f * hs), 0.004f * hs);
        for (int sgn = -1; sgn <= 1; sgn += 2)
            d = SUnion(d, Ellipsoid(p, { sgn * 0.07f * hs, 0.18f * hs, 0.04f * hs }, { 0.045f * hs, 0.012f * hs, 0.05f * hs }), 0.01f * hs);
        d += FBM3(Vector3Scale(p, 60.0f)) * 0.002f;
        return d;
    };
    auto goatHead = [=](Vector3 p) {
        float d = Ellipsoid(p, { 0, 0.12f * hs, -0.01f * hs }, { 0.07f * hs, 0.08f * hs, 0.08f * hs });
        d = SUnion(d, RoundCone(p, { 0, 0.11f * hs, 0.03f * hs }, { 0, 0.02f * hs, 0.17f * hs }, 0.058f * hs, 0.032f * hs), 0.03f * hs);
        d = SSub(d, Sphere(p, { 0.05f * hs, 0.12f * hs, 0.07f * hs }, 0.014f * hs), 0.006f * hs);
        d = SSub(d, Sphere(p, { -0.05f * hs, 0.12f * hs, 0.07f * hs }, 0.014f * hs), 0.006f * hs);
        for (int sgn = -1; sgn <= 1; sgn += 2)
            d = SUnion(d, Ellipsoid(p, { sgn * 0.09f * hs, 0.13f * hs, -0.01f * hs }, { 0.05f * hs, 0.014f * hs, 0.022f * hs }), 0.01f * hs);
        if (sp.beard || true) d = SUnion(d, RoundCone(p, { 0, 0.0f, 0.12f * hs }, { 0, -0.07f * hs, 0.1f * hs }, 0.02f * hs, 0.006f * hs), 0.01f * hs);
        d += FBM3(Vector3Scale(p, 120.0f)) * 0.003f;
        return d;
    };
    Vector3 hmin{ -0.14f * hs, -0.12f * hs, -0.14f * hs }, hmax{ 0.14f * hs, 0.25f * hs, 0.25f * hs };
    if (sp.head == HEAD_PIG) sc.Part(B_HEAD, headMat, pigHead, hmin, hmax, cFine);
    else if (sp.head == HEAD_GOAT || sp.head == HEAD_DOG) sc.Part(B_HEAD, headMat, goatHead, hmin, hmax, cFine);
    else sc.Part(B_HEAD, headMat, humanHead, hmin, hmax, 0.0034f * hs);
    if (sp.head == HEAD_GOAT) {
        // curling horns
        sc.Part(B_HEAD, g_hornMat, [=](Vector3 p) {
            float d = 1e9f;
            for (int sgn = -1; sgn <= 1; sgn += 2) {
                Vector3 prev{ sgn * 0.035f * hs, 0.18f * hs, 0.01f * hs };
                for (int k = 1; k <= 8; k++) {
                    float a = k * 0.42f;
                    Vector3 cur{ sgn * (0.035f + k * 0.012f) * hs, (0.18f + sinf(a) * 0.07f) * hs, (0.01f - (1 - cosf(a)) * 0.08f) * hs };
                    float r = (0.022f - k * 0.0022f) * hs;
                    d = fminf(d, RoundCone(p, prev, cur, r + 0.0022f * hs, r));
                    prev = cur;
                }
            }
            d += sinf(p.y * 900.0f + p.z * 300.0f) * 0.0012f;   // ridges
            return d;
        }, { -0.2f * hs, 0.05f * hs, -0.2f * hs }, { 0.2f * hs, 0.32f * hs, 0.1f * hs }, cFine);
    }
    // eyes
    if (sp.head == HEAD_HUMAN || sp.head == HEAD_PIG || sp.head == HEAD_GOAT) {
        Vector3 e1 = eyeC1, e2{ -eyeC1.x, eyeC1.y, eyeC1.z };
        if (sp.head == HEAD_PIG) { e1 = { 0.045f * hs, 0.125f * hs, 0.083f * hs }; e2 = { -e1.x, e1.y, e1.z }; }
        if (sp.head == HEAD_GOAT) { e1 = { 0.05f * hs, 0.12f * hs, 0.063f * hs }; e2 = { -e1.x, e1.y, e1.z }; }
        float er = eyeR;
        sc.Part(B_HEAD, MAT_EYE, [=](Vector3 p) { return fminf(Sphere(p, e1, er), Sphere(p, e2, er)); },
                { -0.08f * hs, 0.06f * hs, 0.03f * hs }, { 0.08f * hs, 0.14f * hs, 0.12f * hs }, 0.0012f * hs);
        float pr = sp.head == HEAD_GOAT ? 0.0065f : 0.0056f;
        Vector3 i1 = Vector3Add(e1, { 0, 0, er * 0.8f }), i2 = Vector3Add(e2, { 0, 0, er * 0.8f });
        sc.Part(B_HEAD, MAT_PAINT_BLACK, [=](Vector3 p) {
            Vector3 q1 = { (p.x - i1.x) * (sp.head == HEAD_GOAT ? 0.45f : 1.0f), p.y - i1.y, (p.z - i1.z) * 2.2f };
            Vector3 q2 = { (p.x - i2.x) * (sp.head == HEAD_GOAT ? 0.45f : 1.0f), p.y - i2.y, (p.z - i2.z) * 2.2f };
            return fminf(Vector3Length(q1), Vector3Length(q2)) - pr * hs;
        }, { -0.08f * hs, 0.06f * hs, 0.05f * hs }, { 0.08f * hs, 0.14f * hs, 0.12f * hs }, 0.0009f * hs);
    } else if (sp.head == HEAD_HOLLOW) {
        sc.Part(B_HEAD, MAT_PAINT_BLACK, [=](Vector3 p) {
            return fminf(Sphere(p, { 0.031f * hs, 0.097f * hs, 0.075f * hs }, 0.02f * hs), Sphere(p, { -0.031f * hs, 0.097f * hs, 0.075f * hs }, 0.02f * hs));
        }, { -0.08f * hs, 0.06f * hs, 0.03f * hs }, { 0.08f * hs, 0.14f * hs, 0.12f * hs }, cFine * 0.6f);
    }
    // teeth for gaping mouths
    if (sp.jawDrop > 0.25f && sp.head != HEAD_FACELESS) {
        float my = 0.042f * hs - sp.jawDrop * 0.02f * hs;
        sc.Part(B_HEAD, MAT_TEETH, [=](Vector3 p) {
            float d = 1e9f;
            for (int k = -4; k <= 4; k++) {
                float x = k * 0.0055f * hs;
                d = fminf(d, Box(p, { x, my + 0.009f * hs, 0.092f * hs - fabsf((float)k) * 0.0015f * hs }, { 0.0024f * hs, 0.005f * hs, 0.003f * hs }, 0.001f * hs));
                d = fminf(d, Box(p, { x, my - 0.012f * hs - sp.jawDrop * 0.02f * hs, 0.088f * hs - fabsf((float)k) * 0.0015f * hs }, { 0.0024f * hs, 0.0045f * hs, 0.003f * hs }, 0.001f * hs));
            }
            return d;
        }, { -0.05f * hs, -0.03f * hs, 0.05f * hs }, { 0.05f * hs, 0.08f * hs, 0.12f * hs }, cFine * 0.35f);
    }
    // hair / beard
    if (sp.hair != HAIR_BALD && sp.head == HEAD_HUMAN) {
        HairStyle hst = sp.hair;
        sc.Part(B_HEAD, sp.hairMat, [=](Vector3 p) {
            float shell = Ellipsoid(p, { 0, 0.126f * hs, -0.018f * hs }, { 0.083f * hs, 0.106f * hs, 0.102f * hs });
            float hair = shell;
            // keep only the top/back of the skull: cut the face and neck region
            // hairline: high on the forehead, down to the nape at the back, above the ears
            float hz = p.z / hs;
            float line = hz > 0.02f ? 0.148f + (hz - 0.02f) * 0.45f : 0.075f - (0.02f - hz) * 0.36f;
            if (fabsf(p.x) > 0.05f * hs && hz > -0.04f && hz < 0.03f) line = fmaxf(line, 0.118f);   // clear the ears
            if (hst == HAIR_LONG) line = hz > 0.03f ? 0.15f : -0.2f;
            float cut = line * hs - p.y;
            hair = SInter(hair, cut, 0.006f * hs);
            if (hst == HAIR_MESSY) hair += FBM3(Vector3Scale(p, 55.0f)) * 0.007f * hs - 0.004f * hs;
            if (hst == HAIR_SHORT) hair += FBM3(Vector3Scale(p, 120.0f)) * 0.0015f * hs;
            if (hst == HAIR_WISPS) hair += 0.004f * hs + fmaxf(0.0f, FBM3(Vector3Scale(p, 50.0f))) * 0.02f * hs;
            if (hst == HAIR_LONG) hair = SUnion(hair, RoundCone(p, { 0, 0.1f * hs, -0.07f * hs }, { 0, -0.15f * hs, -0.09f * hs }, 0.07f * hs, 0.05f * hs) + FBM3(Vector3Scale(p, 60.0f)) * 0.01f * hs, 0.03f * hs);
            return hair;
        }, { -0.13f * hs, -0.3f * hs, -0.16f * hs }, { 0.13f * hs, 0.25f * hs, 0.15f * hs }, cFine);
    }
    if (sp.head == HEAD_HUMAN && sp.hair != HAIR_BALD) {
        sc.Part(B_HEAD, sp.hairMat, [=](Vector3 p) {
            float d = 1e9f;
            for (int sgn = -1; sgn <= 1; sgn += 2)
                d = fminf(d, RoundCone(p, { sgn * 0.016f * hs, 0.117f * hs, 0.093f * hs }, { sgn * 0.048f * hs, 0.119f * hs, 0.083f * hs }, 0.0042f * hs, 0.0024f * hs));
            return d + FBM3(Vector3Scale(p, 300.0f)) * 0.0008f;
        }, { -0.07f * hs, 0.1f * hs, 0.06f * hs }, { 0.07f * hs, 0.14f * hs, 0.11f * hs }, 0.0014f * hs);
    }
    if (sp.beard) {
        sc.Part(B_HEAD, sp.hairMat, [=](Vector3 p) {
            float d = RoundCone(p, { 0, 0.04f * hs, 0.06f * hs }, { 0, -0.06f * hs, 0.07f * hs }, 0.06f * hs, 0.025f * hs);
            d = SSub(d, Ellipsoid(p, { 0, 0.07f * hs, 0.02f * hs }, { 0.06f * hs, 0.06f * hs, 0.07f * hs }), 0.01f * hs);
            d += FBM3(Vector3Scale(p, 80.0f)) * 0.006f * hs;
            return d;
        }, { -0.1f * hs, -0.1f * hs, -0.02f * hs }, { 0.1f * hs, 0.1f * hs, 0.16f * hs }, cFine);
    }

    // ---- arms
    bool longSleeves = sp.top == TOP_HOODIE || sp.top == TOP_JACKET || sp.top == TOP_SHIRT_APRON || sp.top == TOP_OVERALLS;
    for (int side = 0; side < 2; side++) {
        int ua = side == 0 ? B_LUARM : B_RUARM, fa = side == 0 ? B_LFARM : B_RFARM, hb = side == 0 ? B_LHAND : B_RHAND;
        float sx = side == 0 ? 1.0f : -1.0f;
        auto upper = [=](Vector3 p) {
            float d = RoundCone(p, { 0, 0.0f, 0 }, { 0, -uarmL, 0 }, 0.054f * s * rm, 0.041f * s * rm);
            d = SUnion(d, Sphere(p, { sx * 0.005f, -0.01f * s, 0 }, 0.06f * s * rm), 0.03f * s);
            if (sp.thin > 0.7f) d = SUnion(d, Sphere(p, { 0, -uarmL, -0.01f * s }, 0.032f * s), 0.02f * s);  // bony elbow
            return d;
        };
        auto fore = [=](Vector3 p) {
            float d = RoundCone(p, { 0, 0.01f * s, 0 }, { 0, -farmL, 0 }, 0.043f * s * rm, 0.028f * s * rm);
            return d;
        };
        Vector3 umin{ -0.1f * s, -uarmL - 0.07f * s, -0.1f * s }, umax{ 0.1f * s, 0.08f * s, 0.1f * s };
        Vector3 fmin{ -0.08f * s, -farmL - 0.05f * s, -0.08f * s }, fmax{ 0.08f * s, 0.06f * s, 0.08f * s };
        if (sp.top == TOP_TSHIRT) {
            float cutY = -0.12f * s;
            sc.Part(ua, topMat, [=](Vector3 p) { return SInter(upper(p) - 0.006f, p.y - cutY, 0.003f); }, umin, umax, cBody * 0.8f);
            sc.Part(ua, sp.skinMat, [=](Vector3 p) { return SInter(upper(p), cutY + 0.01f - p.y, 0.003f); }, umin, umax, cBody * 0.8f);
            sc.Part(fa, sp.skinMat, fore, fmin, fmax, cBody * 0.7f);
        } else {
            int m = longSleeves ? topMat : sp.skinMat;
            float pad = longSleeves ? clothPad * 0.6f : 0.0f;
            sc.Part(ua, m, [=](Vector3 p) { return upper(p) - pad; }, umin, umax, cBody * 0.8f);
            sc.Part(fa, m, [=](Vector3 p) { return fore(p) - pad; }, fmin, fmax, cBody * 0.7f);
        }
        // hand: palm in the YZ plane, fingers pointing down, thumb forward
        float fl = 0.085f * s * sp.fingerLen;
        sc.Part(hb, sp.skinMat, [=](Vector3 p) {
            float d = Box(p, { 0, -0.048f * s, 0.004f * s }, { 0.014f * s * rm, 0.048f * s, 0.041f * s }, 0.012f * s);
            for (int k = 0; k < 4; k++) {
                float z = (-0.028f + k * 0.019f) * s;
                float len = fl * (k == 0 ? 0.8f : (k == 3 ? 0.85f : 1.0f));
                Vector3 a{ 0, -0.09f * s, z + 0.004f * s }, b{ sx * 0.008f * s, -0.09f * s - len * 0.55f, z + 0.012f * s };
                Vector3 c{ sx * 0.004f * s, -0.09f * s - len, z + 0.028f * s };
                d = SUnion(d, RoundCone(p, a, b, 0.0085f * s * rm, 0.0075f * s * rm), 0.004f * s);
                d = SUnion(d, RoundCone(p, b, c, 0.0075f * s * rm, 0.006f * s * rm), 0.003f * s);
            }
            d = SUnion(d, RoundCone(p, { sx * -0.004f * s, -0.035f * s, 0.035f * s }, { sx * 0.004f * s, -0.075f * s, 0.058f * s }, 0.011f * s * rm, 0.008f * s * rm), 0.008f * s);
            return d;
        }, { -0.06f * s, -0.09f * s - fl - 0.03f * s, -0.07f * s }, { 0.06f * s, 0.03f * s, 0.09f * s }, cFine * 0.7f);
    }

    // ---- legs
    for (int side = 0; side < 2; side++) {
        int th = side == 0 ? B_LTHIGH : B_RTHIGH, sh = side == 0 ? B_LSHIN : B_RSHIN, ft = side == 0 ? B_LFOOT : B_RFOOT;
        sc.Part(th, pantsMat, [=](Vector3 p) {
            float d = RoundCone(p, { 0, 0.02f * s, 0 }, { 0, -thighL, 0 }, 0.088f * s * rm, 0.058f * s * rm);
            if (sp.pants) d += FBM3(Vector3Scale(p, 30.0f)) * 0.003f;
            return d;
        }, { -0.13f * s, -thighL - 0.08f * s, -0.13f * s }, { 0.13f * s, 0.12f * s, 0.13f * s }, cBody);
        sc.Part(sh, pantsMat, [=](Vector3 p) {
            float d = RoundCone(p, { 0, 0.0f, 0 }, { 0, -shinL, 0 }, 0.059f * s * rm, 0.042f * s * rm);
            d = SUnion(d, Ellipsoid(p, { 0, -0.13f * s, -0.022f * s }, { 0.05f * s * rm, 0.1f * s, 0.055f * s * rm }), 0.02f * s);
            if (sp.thin > 0.7f) d = SUnion(d, Sphere(p, { 0, 0.0f, 0.03f * s }, 0.035f * s), 0.02f * s);   // knee
            return d;
        }, { -0.1f * s, -shinL - 0.06f * s, -0.1f * s }, { 0.1f * s, 0.08f * s, 0.1f * s }, cBody);
        int fm = sp.barefoot ? sp.skinMat : sp.shoeMat;
        sc.Part(ft, fm, [=](Vector3 p) {
            float d = RoundCone(p, { 0, -0.035f * s, -0.035f * s }, { 0, -0.055f * s, 0.14f * s }, 0.046f * s, 0.038f * s);
            if (!sp.barefoot) d = SUnion(d, Capsule(p, { 0, 0, 0 }, { 0, -0.04f * s, 0 }, 0.045f * s), 0.02f * s);
            else d = SUnion(d, Capsule(p, { 0, 0, 0 }, { 0, -0.04f * s, 0 }, 0.038f * s), 0.02f * s);
            d = fmaxf(d, -(p.y + 0.083f * s));   // flat sole
            return d;
        }, { -0.07f * s, -0.1f * s, -0.1f * s }, { 0.07f * s, 0.06f * s, 0.2f * s }, cFine * 1.3f);
        if (!sp.barefoot)
            sc.Part(ft, MAT_RUBBER, [=](Vector3 p) {
                float d = RoundCone(p, { 0, -0.07f * s, -0.04f * s }, { 0, -0.072f * s, 0.15f * s }, 0.046f * s, 0.041f * s);
                return fmaxf(fmaxf(d, -(p.y + 0.085f * s)), p.y + 0.068f * s);
            }, { -0.07f * s, -0.1f * s, -0.1f * s }, { 0.07f * s, -0.05f * s, 0.21f * s }, cFine * 1.3f);
    }
}

// CPU half: sculpt every part (thread-safe; no GPU calls).
static CharBuild* BuildCharacterCPU(const BodySpec& sp, int maxThreads) {
    CharBuild* cb = new CharBuild();
    cb->cm = new CharModel();
    cb->cm->spec = sp;
    SculptCharacter(cb, sp);
    RunSculptJobs(cb, maxThreads);
    return cb;
}

// GPU half: upload the meshes (main thread only).
static CharModel* FinishCharacter(CharBuild* cb) {
    CharModel* cm = cb->cm;
    for (auto& j : cb->jobs) {
        MeshAsset* ma = j->mb.Upload();
        if (!ma) continue;
        MeshAsset* prev = ma;
        for (int l = 0; l < 2; l++) {
            MeshAsset* lo = j->lod[l].Upload();
            if (!lo) break;   // tiny parts (eyes, teeth) keep their last level
            prev->lod = lo; prev->lodDist = kLodDist[l];
            prev = lo;
        }
        cm->parts.push_back({ j->bone, ma, j->mat });
    }
    delete cb;
    return cm;
}

CharModel* BuildCharacter(const BodySpec& sp) {
    EnsureCreatureMats();
    return FinishCharacter(BuildCharacterCPU(sp, 16));
}

// ---------------------------------------------------------------------------
// Character cache with background preparation
// ---------------------------------------------------------------------------
namespace {
struct Pending { BodySpec spec; CharBuild* cpu = nullptr; bool started = false; };
std::mutex g_cacheMx;
std::condition_variable g_cacheCv;
std::map<uint32_t, CharModel*> g_ready;       // uploaded, ready to draw
std::map<uint32_t, Pending> g_pending;        // queued or being sculpted in the background
std::vector<uint32_t> g_queue;                // background order
std::thread g_worker;
bool g_workerRunning = false;
}

uint32_t CharacterKey(const BodySpec& spec) {
    return HashU32(spec.seed * 131u + (uint32_t)(spec.height * 1000) + (uint32_t)spec.head * 7u + (uint32_t)spec.top * 13u);
}

// Builds the given cast now (on the main thread, using all cores), so it is ready before it
// appears. Called while the screen is black (loading, chapter titles). The earlier background
// thread was removed for robustness; `urgent == false` requests are ignored.
void PrepareCharacters(const std::vector<BodySpec>& specs, bool urgent) {
    if (!urgent) return;
    for (const BodySpec& sp : specs) GetCharacter(sp);
}

void PumpCharacterUploads() {
    // upload at most one finished character per frame (a few milliseconds)
    CharBuild* cb = nullptr; uint32_t k = 0;
    {
        std::lock_guard<std::mutex> lk(g_cacheMx);
        for (auto& kv : g_pending) if (kv.second.cpu) { k = kv.first; cb = kv.second.cpu; g_pending.erase(kv.first); break; }
    }
    if (cb) g_ready[k] = FinishCharacter(cb);
}

CharModel* GetCharacter(const BodySpec& spec) {
    uint32_t k = CharacterKey(spec);
    auto it = g_ready.find(k);
    if (it != g_ready.end()) return it->second;
    CharBuild* cb = nullptr;
    {
        std::unique_lock<std::mutex> lk(g_cacheMx);
        auto pit = g_pending.find(k);
        if (pit != g_pending.end() && pit->second.started) {
            // the worker is sculpting it right now: wait for it
            g_cacheCv.wait(lk, [&]() { return g_pending[k].cpu != nullptr; });
            cb = g_pending[k].cpu;
            g_pending.erase(k);
        } else if (pit != g_pending.end()) {
            g_pending.erase(pit);   // queued but not started: take it and build it here
        }
    }
    if (!cb) { EnsureCreatureMats(); cb = BuildCharacterCPU(spec, 16); }
    CharModel* cm = FinishCharacter(cb);
    g_ready[k] = cm;
    return cm;
}

void ShutdownCharacters() {
    {
        std::lock_guard<std::mutex> lk(g_cacheMx);
        g_queue.clear();
    }
    if (g_worker.joinable()) g_worker.join();
}

// ---------------------------------------------------------------------------
// Actor
// ---------------------------------------------------------------------------
static void SolveArm(Actor& a, int side, Vector3 target, float weight);
Matrix Actor::RootXf() const { return MatPose(pos, yaw, pitch, roll, { scale, scale, scale }); }

void Actor::BoneMatrices(Matrix out[B_COUNT]) const {
    static const int parent[B_COUNT] = { -1, B_PELVIS, B_SPINE, B_CHEST, B_NECK, B_CHEST, B_LUARM, B_LFARM, B_CHEST, B_RUARM, B_RFARM,
                                         B_PELVIS, B_LTHIGH, B_LSHIN, B_PELVIS, B_RTHIGH, B_RSHIN };
    Matrix root = RootXf();
    for (int b = 0; b < B_COUNT; b++) {
        Vector3 off = model->offset[b];
        if (b == B_PELVIS) off = Vector3Add(off, pose.root);
        Vector3 r = pose.rot[b];
        if (b == B_PELVIS) r.y += pose.rootYaw;
        Matrix local = MatrixMultiply(MatrixRotateXYZ(r), MatrixTranslate(off.x, off.y, off.z));
        out[b] = MatrixMultiply(local, parent[b] < 0 ? root : out[parent[b]]);
    }
}

Vector3 Actor::BonePos(int bone, Vector3 local) const {
    Matrix m[B_COUNT];
    BoneMatrices(m);
    return Vector3Transform(local, m[bone]);
}

void Actor::Update(float dt) {
    float k = 1.0f - expf(-blendSpeed * dt);
    pose = PoseLerp(pose, target, k);
    for (int s = 0; s < 2; s++) if (ikOn[s] && model) SolveArm(*this, s, ikTarget[s], ikWeight[s]);
    if (lookWeight > 0.001f && model) {
        // turn neck+head towards the look target (in body space)
        Vector3 head = BonePos(B_NECK, { 0, 0.1f, 0 });
        Vector3 d = Vector3Subtract(lookAt, head);
        float wantYaw = WrapAngle(atan2f(d.x, d.z) - yaw);
        float wantPitch = -atan2f(d.y, sqrtf(d.x * d.x + d.z * d.z));
        wantYaw = Clamp(wantYaw, -1.3f, 1.3f);
        wantPitch = Clamp(wantPitch, -0.7f, 0.7f);
        pose.rot[B_NECK].y = Lerp(pose.rot[B_NECK].y, wantYaw * 0.4f, lookWeight);
        pose.rot[B_HEAD].y = Lerp(pose.rot[B_HEAD].y, wantYaw * 0.6f, lookWeight);
        pose.rot[B_HEAD].x = Lerp(pose.rot[B_HEAD].x, wantPitch * 0.8f, lookWeight);
    }
}

// Numerical two-bone arm IK: coordinate descent on shoulder (x, y, z) and elbow (x)
// so the palm reaches the target while staying close to the animated pose.
static void SolveArm(Actor& a, int side, Vector3 target, float weight) {
    int ua = side == 0 ? B_LUARM : B_RUARM, fa = side == 0 ? B_LFARM : B_RFARM, hb = side == 0 ? B_LHAND : B_RHAND;
    Pose base = a.pose;
    auto err = [&](const Pose& p) {
        Pose saved = a.pose; a.pose = p;
        Vector3 h = a.BonePos(hb, { 0, -0.07f, 0.01f });
        a.pose = saved;
        float d = Vector3Distance(h, target);
        float reg = 0.02f * (fabsf(p.rot[ua].y - base.rot[ua].y) + fabsf(p.rot[fa].x - base.rot[fa].x) * 0.2f);
        return d + reg;
    };
    Pose p = base;
    float* vars[4] = { &p.rot[ua].x, &p.rot[ua].z, &p.rot[ua].y, &p.rot[fa].x };
    float step = 0.35f;
    float e = err(p);
    for (int it = 0; it < 18; it++) {
        for (float* v : vars) {
            float old = *v;
            *v = old + step; float e1 = err(p);
            if (e1 < e) { e = e1; continue; }
            *v = old - step; float e2 = err(p);
            if (e2 < e) { e = e2; continue; }
            *v = old;
        }
        p.rot[fa].x = Clamp(p.rot[fa].x, -2.6f, 0.0f);   // elbows only bend one way
        step *= 0.62f;
    }
    a.pose.rot[ua] = Vector3Lerp(base.rot[ua], p.rot[ua], weight);
    a.pose.rot[fa] = Vector3Lerp(base.rot[fa], p.rot[fa], weight);
}

void Actor::Draw() const {
    if (!visible || !model) return;
    Matrix m[B_COUNT];
    BoneMatrices(m);
    for (const auto& p : model->parts) {
        if (hideHead && (p.bone == B_HEAD || p.bone == B_NECK)) continue;
        Rdr().DrawPart(p.mesh, p.mat, m[p.bone], tint, true);
    }
}

// ---------------------------------------------------------------------------
// Cast
// ---------------------------------------------------------------------------
BodySpec SpecAdam() {
    BodySpec b; b.height = 1.8f; b.thin = 0.35f; b.top = TOP_JACKET; b.topMat = MAT_CLOTH_DARK; b.pantsMat = MAT_DENIM;
    b.hair = HAIR_SHORT; b.shoeMat = MAT_LEATHER; b.seed = 11;
    return b;
}
BodySpec SpecZain() {
    BodySpec b; b.height = 1.6f; b.thin = 0.45f; b.headScale = 1.04f; b.top = TOP_HOODIE; b.topMat = MAT_CLOTH_RED;
    b.pantsMat = MAT_DENIM; b.hair = HAIR_MESSY; b.shoeMat = MAT_CLOTH_WHITE; b.seed = 12;
    return b;
}
BodySpec SpecGrethnar() {
    BodySpec b; b.height = 1.93f; b.thin = 0.72f; b.limbLen = 1.06f; b.neckLen = 1.35f; b.fingerLen = 1.4f;
    b.eyeSink = 0.9f; b.noseLen = 1.4f; b.smile = 0.8f; b.headScale = 1.02f;
    b.top = TOP_SHIRT_APRON; b.topMat = MAT_CLOTH_WHITE; b.accentMat = MAT_CLOTH_BROWN; b.pantsMat = MAT_CLOTH_DARK;
    b.hair = HAIR_WISPS; b.hairMat = MAT_CLOTH_GREY; b.skinMat = MAT_SKIN_DEAD; b.shoeMat = MAT_LEATHER; b.seed = 13;
    return b;
}
BodySpec SpecDragger() {
    BodySpec b; b.height = 2.3f; b.thin = 0.86f; b.limbLen = 1.18f; b.neckLen = 1.6f; b.fingerLen = 1.9f; b.shoulders = 0.9f;
    b.eyeSink = 1.0f; b.jawDrop = 0.9f; b.head = HEAD_HOLLOW; b.ribs = true; b.top = TOP_BARE; b.pants = false;
    b.hair = HAIR_BALD; b.skinMat = MAT_SKIN_GREY; b.barefoot = true; b.seed = 14;
    return b;
}
BodySpec SpecOldMan() {
    BodySpec b; b.height = 1.7f; b.thin = 0.85f; b.eyeSink = 0.6f; b.top = TOP_RAGS; b.pantsMat = MAT_CLOTH_GREY;
    b.hair = HAIR_LONG; b.hairMat = MAT_CLOTH_WHITE; b.beard = true; b.skinMat = MAT_SKIN_DEAD; b.barefoot = true; b.seed = 15;
    return b;
}
BodySpec SpecCustomer(uint32_t seed) {
    Rng r(seed * 77 + 5);
    BodySpec b; b.height = r.Range(1.62f, 1.9f); b.thin = r.Range(0.2f, 0.6f); b.head = HEAD_FACELESS;
    const int tops[] = { MAT_CLOTH_BLUE, MAT_CLOTH_GREEN, MAT_CLOTH_GREY, MAT_CLOTH_BROWN, MAT_CLOTH_DARK };
    b.top = r.Chance(0.5f) ? TOP_JACKET : TOP_TSHIRT; b.topMat = tops[r.RangeI(0, 4)];
    b.hair = r.Chance(0.5f) ? HAIR_SHORT : HAIR_BALD; b.seed = seed;
    return b;
}
BodySpec SpecCrawler(uint32_t seed) {
    Rng r(seed * 31 + 3);
    BodySpec b; b.height = r.Range(1.65f, 1.95f); b.thin = 0.92f; b.limbLen = r.Range(1.05f, 1.2f); b.fingerLen = 1.6f;
    b.jawDrop = r.Range(0.5f, 1.0f); b.eyeSink = 1.0f; b.head = r.Chance(0.5f) ? HEAD_HOLLOW : HEAD_HUMAN;
    b.ribs = true; b.top = TOP_BARE; b.pants = false; b.barefoot = true; b.hair = HAIR_WISPS; b.hairMat = MAT_HAIR;
    b.skinMat = r.Chance(0.5f) ? MAT_SKIN_GREY : MAT_SKIN_DEAD; b.seed = seed;
    return b;
}
BodySpec SpecPigMan(uint32_t seed) {
    BodySpec b; b.height = 1.85f; b.thin = 0.1f; b.head = HEAD_PIG; b.headScale = 1.3f; b.top = TOP_OVERALLS;
    b.topMat = MAT_CLOTH_WHITE; b.accentMat = MAT_DENIM; b.pantsMat = MAT_DENIM; b.hair = HAIR_BALD; b.seed = seed;
    return b;
}
BodySpec SpecGoatMan(uint32_t seed) {
    BodySpec b; b.height = 1.9f; b.thin = 0.6f; b.head = HEAD_GOAT; b.headScale = 1.25f; b.top = TOP_JACKET;
    b.topMat = MAT_CLOTH_DARK; b.pantsMat = MAT_CLOTH_DARK; b.hair = HAIR_BALD; b.barefoot = true; b.skinMat = MAT_SKIN_GREY; b.seed = seed;
    return b;
}
BodySpec SpecMother() {
    BodySpec b; b.height = 1.66f; b.thin = 0.5f; b.top = TOP_RAGS; b.hair = HAIR_LONG; b.skinMat = MAT_SKIN_DEAD; b.head = HEAD_FACELESS; b.barefoot = true; b.seed = 16;
    return b;
}
