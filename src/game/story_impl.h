// Internal state shared by the story_*.cpp files.
#pragma once
#include "story.h"
#include "sedan.h"
#include "fx.h"
#include <memory>

// The brothers' car (and customer cars) as a driveable rig
struct CarRig {
    Vector3 pos{};
    float yaw = 0;            // radians (0 = +Z)
    float speed = 0;          // m/s
    float steer = 0;          // -1..1 (wheel angle ~ steer * 30 deg)
    float spin = 0;           // wheel rotation
    float roll = 0, pitch = 0;
    bool headlights = true;
    float headFlicker = 1.0f;
    int paint = MAT_CARPAINT_BLUE;
    Model3D* body = nullptr;
    bool visible = false;
    bool interior = true;     // draw interior parts (steering wheel, gauges)
    Matrix Xf() const { return MatPose(pos, yaw, pitch, roll); }
    Vector3 Local(Vector3 p) const { return Vector3Transform(p, Xf()); }
    void Update(float dt);
    void Draw() const;
    void Lights() const;
};

// One task on tonight's list
struct JobTask {
    std::string id, label;
    bool done = false;
    int count = 0, need = 1;
};

struct Customer {
    CarRig car;
    std::string driver;       // actor name ("" = nobody inside)
    int state = 0;            // 0 arriving, 1 waiting, 2 fuelling, 3 paid/leaving, 4 gone
    int pump = 0;
    float target = 0;         // dollars requested
    float pumped = 0;
    float t = 0;
    bool ghost = false;       // nobody inside
    Vector3 stopPos{};
    float stopYaw = 0;
};

struct Creature {
    std::string actor;
    int kind = 0;             // 0 crawler, 1 skeleton cow, 2 corpse (reanimated)
    int state = 0;            // 0 idle, 1 wander, 2 hunt, 3 flee, 4 dead, 5 feeding
    Vector3 home{};
    Vector3 target{};
    float t = 0, phase = 0, speed = 0;
    float fear = 0;           // light exposure
    float hp = 1.0f;
    float burn = 0.0f;
    Vector3 pos{};            // feet position (skeleton cows are drawn from parts, not actors)
    float yaw = 0;
    float gait = 0;           // walk cycle phase
    float wake = 0;           // 0..1 assembling itself out of the grass
    float hitCd = 0;          // cooldown between attacks
    uint32_t seed = 1;
};

struct Story::Impl {
    Game& g;
    explicit Impl(Game& gg) : g(gg) {}
    // car
    CarRig car;
    std::vector<Customer> customers;
    // cutscene camera
    std::vector<CamKey> camPath;
    float camT = 0.0f;
    bool camPathActive = false;
    Vector2 lookOffset{};     // player look offset during cutscenes (yaw, pitch)
    bool allowLook = false;
    // FX
    Particles fx;
    Decals decals;
    // job
    std::vector<JobTask> tasks;
    int shift = 0;
    float shiftT = 0;
    float powerOutT = -1;
    int stainsLeft = 0;
    std::vector<Vector3> stains;
    std::vector<float> stainAlpha;
    Model3D* stainModel = nullptr;
    // creatures
    std::vector<Creature> creatures;
    // fuel trail + fire
    std::vector<Vector3> trail;
    std::vector<float> trailFire;   // 0 unlit .. 1 burning .. 2 burnt out
    float canFuel = 0.0f;
    bool fireLit = false;
    float fireT = 0.0f;
    // misc state
    float t = 0.0f;
    float blood = 0.0f;       // blood running down the screen (overlay)
    float stareTimer = 0.0f;
    bool grethnarStaring = false;
    float phoneRingT = -1.0f;
    int phoneCalls = 0;
    bool leavingWarned = false;
    bool wreckLights = false;
    float showPolaroid = 0.0f;
    // choices
    std::vector<std::string> choiceOpts;
    int choiceSel = 0, choiceResult = -1;
    bool choiceActive = false;
    void Choose(const std::vector<std::string>& opts) { choiceOpts = opts; choiceSel = 0; choiceResult = -1; choiceActive = true; EnableCursor(); }
    void UpdateChoice();
    void DrawChoice();

    // helpers
    void SetCamPath(const std::vector<CamKey>& keys) { camPath = keys; camT = 0; camPathActive = true; }
    void EndCamPath() { camPathActive = false; }
    void SetupWorldForChapter(int ch);
    void SpawnCast(int ch);
    void RegisterActions();

    // chapters
    void BuildPrologue(Script& s);
    void BuildAwakening(Script& s);
    void BuildShift(Script& s, int n);
    void BuildKey(Script& s);
    void BuildBelow(Script& s);
    void BuildBurn(Script& s);
    void UpdateCommon(float dt);
    void UpdateJob(float dt);
    void UpdateCustomers(float dt);
    void UpdateCreatures(float dt);
    void UpdateFire(float dt);
    void UpdateGrethnar(float dt);
    void UpdatePhone(float dt);
    void UpdateBoundary(float dt);
    void StartShiftTasks(int n);
    JobTask* Task(const std::string& id);
    void CompleteTask(const std::string& id);
    bool AllTasksDone() const;
    void SpawnCustomer(bool ghost);
    void SpawnStains(int n);
    void AddCreature(const std::string& name, int kind, Vector3 pos);
    void DrawCreatures();
    Creature* FindCreature(const std::string& name);
    int playerHits = 0;       // attacks taken since the last checkpoint
    Vector3 checkpoint{};
    float checkpointYaw = 0;
    std::function<void()> onCaught;   // chapter-specific reset when a creature gets you
    void Pay(float amount, const std::string& why);
};

// Road helpers
inline Vector3 RoadPoint(float z, float lane) {
    float x = Terrain::RoadX(z) + lane;
    return { x, Terrain::RoadY(z) + 0.05f, z };
}
inline float RoadYaw(float z) {
    float dx = Terrain::RoadX(z + 1.0f) - Terrain::RoadX(z - 1.0f);
    return atan2f(dx, 2.0f);
}

float StoreYawFix();
// College-local point to world (the college entity is named "college")
Vector3 CollegeP(float x, float y, float z);

// A run of dialogue chosen when the step starts (for branches after a choice)
struct Line { std::string who, text; float dur = -1.0f; };
inline void SayLines(Script& s, std::function<std::vector<Line>()> pick) {
    auto lines = std::make_shared<std::vector<Line>>();
    auto idx = std::make_shared<size_t>(0);
    auto next = std::make_shared<float>(0.0f);
    s.Run([lines, idx, next](float t, float) {
        if (*idx >= lines->size()) return t >= *next;
        if (t >= *next) {
            const Line& l = (*lines)[*idx];
            float d = l.dur < 0 ? 1.6f + l.text.size() * 0.055f : l.dur;
            G().hud.Say(l.who, l.text, d);
            *next = t + d + 0.35f;
            (*idx)++;
        }
        return false;
    }, [lines, idx, next, pick]() { *lines = pick(); *idx = 0; *next = 0; });
}
void RegisterCollegeActions(Story::Impl& im);
void BuildCollegeDiscovery(Story::Impl& im, Script& s);
void RegisterBelowActions(Story::Impl& im);
void BuildEpilogue(Story::Impl& im, Script& s);
void DrawFireTrail(Story::Impl& im);
