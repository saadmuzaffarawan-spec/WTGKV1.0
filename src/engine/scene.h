// Entities, prefabs and the world scene file.
//
// Every object placed in the world is an Entity created from a named prefab.
// The layout lives in assets/scenes/world.scene (plain text, one entity per line):
//
//   # prefab   x      y     z      yaw   [pitch roll]  [key=value ...]
//   gas_pump   14.0   0.0   -3.0   90    name=pump_1
//   dead_tree  -32.5  0.0   51.0   12    variant=3 scale=1.2
//
// y is relative to the terrain unless abs=1. Angles are degrees.
#pragma once
#include "common.h"
#include "renderer.h"
#include "collision.h"
#include "terrain.h"
#include <map>
#include <memory>
#include <functional>

struct Entity;

// Optional per-entity logic (doors, flickering lamps, pumps...).
struct Behaviour {
    virtual ~Behaviour() = default;
    virtual void Update(Entity&, float) {}
    virtual void Draw(Entity&) {}
};

struct LightDef {
    Light light;             // position/direction in entity-local space
    float flicker = 0.0f;    // 0 steady .. 1 dying
    std::string group;       // light group for game control (e.g. "store")
    bool on = true;
    float cur = 1.0f;        // current flicker multiplier
    float timer = 0.0f;
    int emissivePart = -1;   // unused hook
};

struct Interactable {
    std::string action;      // game action id; defaults to prefab name
    std::string prompt;      // default prompt text
    float radius = 2.0f;     // max reach
    Vector3 offset{ 0, 1, 0 };  // local focus point
    bool enabled = false;
};

struct Entity {
    int id = 0;
    std::string prefab, name, tag;
    Vector3 pos{};           // as written in the file
    Vector3 rot{};           // degrees: yaw, pitch, roll
    Vector3 scale{ 1, 1, 1 };
    bool absY = false;
    std::map<std::string, std::string> props;

    // runtime
    Vector3 base{};          // resolved world position (terrain snapped / parent space)
    float worldYaw = 0.0f;   // radians, including parent yaw
    Matrix xf = MatrixIdentity();
    Model3D* model = nullptr;
    Color tint = WHITE;
    bool visible = true;
    bool castShadow = true;
    std::vector<CBox> boxes;       // local space
    std::vector<CCyl> cyls;        // local space
    std::vector<int> boxIds;       // registered in Phys()
    std::vector<LightDef> lights;
    Interactable interact;
    std::unique_ptr<Behaviour> beh;
    std::map<std::string, float> state;   // free-form runtime state for game logic

    float Num(const std::string& k, float def = 0.0f) const;
    std::string Str(const std::string& k, const std::string& def = "") const;
    bool Has(const std::string& k) const { return props.count(k) > 0; }
    Vector3 LocalToWorld(Vector3 p) const { return Vector3Transform(p, xf); }
    Vector3 WorldToLocal(Vector3 p) const { return Vector3Transform(p, MatrixInvert(xf)); }
    Vector3 Forward() const { return Vector3Normalize(XfDir(xf, { 0, 0, 1 })); }
    Vector3 Right() const { return Vector3Normalize(XfDir(xf, { 1, 0, 0 })); }
    Vector3 FocusPoint() const { return LocalToWorld(interact.offset); }
    float Yaw() const { return worldYaw; }
};

// Build context handed to prefab builders.
struct PrefabBuild {
    ModelBuilder mb;
    std::vector<CBox> boxes;
    std::vector<CCyl> cyls;
    std::vector<LightDef> lights;
    Interactable interact;
    bool castShadow = true;

    MeshBuilder& M(int mat) { return mb.M(mat); }
    // Geometry + collider in one call (local, axis aligned or yawed in degrees)
    void Solid(int mat, Vector3 c, Vector3 size, float bevel = 0.02f, int surf = SURF_CONCRETE, float yawDeg = 0.0f);
    void Collider(Vector3 c, Vector3 size, int surf = SURF_CONCRETE, float yawDeg = 0.0f, bool walkable = true, bool solid = true);
    void ColliderCyl(Vector3 base, float r, float h, int surf = SURF_METAL);
    LightDef& AddLight(Vector3 pos, Vector3 color, float intensity, float range, float volumetric = 0.0f);
    LightDef& AddSpot(Vector3 pos, Vector3 dir, Vector3 color, float intensity, float range, float inner, float outer, float volumetric = 0.0f);
    void Interact(const char* prompt, Vector3 offset, float radius = 2.0f, const char* action = nullptr);
};

struct PrefabInfo {
    std::string name;
    std::string category;
    std::function<void(PrefabBuild&, const Entity&)> build;
    // terrain influence (flatten pads, holes, paths)
    std::function<void(const Entity&, std::vector<TerrainPad>&, std::vector<Rectangle>&)> terrain;
    std::function<std::unique_ptr<Behaviour>(Entity&)> behaviour;
    std::vector<std::string> geometryKeys;   // props that change the mesh (cache key)
    bool snap = true;                        // y relative to terrain
};

void RegisterPrefab(const PrefabInfo& p);
const PrefabInfo* FindPrefab(const std::string& name);
const std::vector<PrefabInfo>& AllPrefabs();
// Build (or fetch cached) model for a prefab with the given props (used by scatter systems).
Model3D* GetPrefabModel(const std::string& prefab, const std::map<std::string, std::string>& props);

class Scene {
public:
    std::vector<std::unique_ptr<Entity>> ents;
    std::string path;

    bool Load(const std::string& file);
    bool Save(const std::string& file) const;
    Entity* Spawn(const std::string& prefab, Vector3 pos, Vector3 rotDeg, const std::map<std::string, std::string>& props = {}, bool absY = false);
    void Rebuild(Entity& e);           // re-run prefab build (after editing props)
    void UpdateTransform(Entity& e);   // after moving/rotating (also updates children)
    void Remove(Entity* e);
    Entity* Find(const std::string& name) const;
    std::vector<Entity*> FindAll(const std::string& prefab) const;
    std::vector<Entity*> FindTag(const std::string& tag) const;
    void SetLightGroup(const std::string& group, bool on);
    void SetLightGroupFlicker(const std::string& group, float flicker);

    void Update(float dt);
    void Draw();
    void SubmitLights(Vector3 camPos);
    void Clear();
    int nextId = 1;

    // Collect terrain pads/holes/paths from the parsed file without spawning.
    struct Record { std::string prefab; Vector3 pos, rot; std::map<std::string, std::string> props; bool absY; std::string raw; };
    std::vector<Record> records;
    bool Parse(const std::string& file);
    void CollectTerrain(std::vector<TerrainPad>& pads, std::vector<Rectangle>& holes, std::vector<TerrainPath>& paths) const;
    void SpawnRecords();
private:
    void RegisterColliders(Entity& e);
    void UnregisterColliders(Entity& e);
};

Scene& Scn();

// Game-side hook for entity events raised by behaviours (sounds, triggers).
extern std::function<void(const std::string& event, Entity& e, Vector3 pos)> OnEntityEvent;
inline void RaiseEvent(const std::string& ev, Entity& e, Vector3 pos) { if (OnEntityEvent) OnEntityEvent(ev, e, pos); }
