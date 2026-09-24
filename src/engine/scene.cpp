#include "scene.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

static Scene g_scene;
std::function<void(const std::string&, Entity&, Vector3)> OnEntityEvent;
Scene& Scn() { return g_scene; }

static std::vector<PrefabInfo>& Registry() { static std::vector<PrefabInfo> r; return r; }
void RegisterPrefab(const PrefabInfo& p) { Registry().push_back(p); }
const PrefabInfo* FindPrefab(const std::string& n) {
    for (auto& p : Registry()) if (p.name == n) return &p;
    return nullptr;
}
const std::vector<PrefabInfo>& AllPrefabs() { return Registry(); }

float Entity::Num(const std::string& k, float def) const {
    auto it = props.find(k);
    return it == props.end() ? def : (float)atof(it->second.c_str());
}
std::string Entity::Str(const std::string& k, const std::string& def) const {
    auto it = props.find(k);
    return it == props.end() ? def : it->second;
}

// ---------------------------------------------------------------------------
// PrefabBuild helpers
// ---------------------------------------------------------------------------
void PrefabBuild::Solid(int mat, Vector3 c, Vector3 size, float bevel, int surf, float yawDeg) {
    if (yawDeg != 0.0f) M(mat).BoxRot(c, size, { 0, yawDeg, 0 }, bevel);
    else M(mat).Box(c, size, bevel);
    Collider(c, size, surf, yawDeg);
}
void PrefabBuild::Collider(Vector3 c, Vector3 size, int surf, float yawDeg, bool walkable, bool solid) {
    CBox b;
    b.c = Vector3Transform(c, mb.Current());
    b.h = Vector3Scale(size, 0.5f);
    b.yaw = yawDeg * DEG2RAD;
    // include the current builder rotation around Y
    Matrix m = mb.Current();
    b.yaw += atan2f(m.m8, m.m0);
    b.surface = surf; b.walkable = walkable; b.solid = solid;
    boxes.push_back(b);
}
void PrefabBuild::ColliderCyl(Vector3 base, float r, float h, int surf) {
    CCyl c; c.base = Vector3Transform(base, mb.Current()); c.r = r; c.h = h; c.surface = surf;
    cyls.push_back(c);
}
LightDef& PrefabBuild::AddLight(Vector3 pos, Vector3 color, float intensity, float range, float volumetric) {
    LightDef d;
    d.light.pos = Vector3Transform(pos, mb.Current());
    d.light.color = color; d.light.intensity = intensity; d.light.range = range; d.light.volumetric = volumetric;
    lights.push_back(d);
    return lights.back();
}
LightDef& PrefabBuild::AddSpot(Vector3 pos, Vector3 dir, Vector3 color, float intensity, float range, float inner, float outer, float volumetric) {
    LightDef& d = AddLight(pos, color, intensity, range, volumetric);
    d.light.spot = true;
    d.light.dir = Vector3Normalize(XfDir(mb.Current(), dir));
    d.light.innerDeg = inner; d.light.outerDeg = outer;
    return d;
}
void PrefabBuild::Interact(const char* prompt, Vector3 offset, float radius, const char* action) {
    interact.enabled = true;
    interact.prompt = prompt;
    interact.offset = Vector3Transform(offset, mb.Current());
    interact.radius = radius;
    if (action) interact.action = action;
}

// ---------------------------------------------------------------------------
// Prefab result cache (identical prefab + geometry props share one mesh)
// ---------------------------------------------------------------------------
struct CachedPrefab {
    Model3D* model = nullptr;
    std::vector<CBox> boxes;
    std::vector<CCyl> cyls;
    std::vector<LightDef> lights;
    Interactable interact;
    bool castShadow = true;
};
static std::map<std::string, CachedPrefab> g_cache;

static std::string CacheKey(const PrefabInfo& p, const Entity& e) {
    std::string k = p.name;
    for (const auto& gk : p.geometryKeys) k += "|" + gk + "=" + e.Str(gk);
    return k;
}

static const CachedPrefab& BuildPrefab(const PrefabInfo& p, const Entity& e) {
    std::string key = CacheKey(p, e);
    auto it = g_cache.find(key);
    if (it != g_cache.end()) return it->second;
    PrefabBuild b;
    if (p.build) p.build(b, e);
    CachedPrefab c;
    c.model = b.mb.Build(b.castShadow);
    c.boxes = b.boxes; c.cyls = b.cyls; c.lights = b.lights; c.interact = b.interact; c.castShadow = b.castShadow;
    if (c.interact.enabled && c.interact.action.empty()) c.interact.action = p.name;
    return g_cache.emplace(key, std::move(c)).first->second;
}

Model3D* GetPrefabModel(const std::string& prefab, const std::map<std::string, std::string>& props) {
    const PrefabInfo* p = FindPrefab(prefab);
    if (!p) return nullptr;
    Entity tmp; tmp.prefab = prefab; tmp.props = props;
    return BuildPrefab(*p, tmp).model;
}

// ---------------------------------------------------------------------------
// Scene
// ---------------------------------------------------------------------------
static std::vector<std::string> Tokenize(const std::string& line) {
    std::vector<std::string> out;
    std::string cur; bool q = false;
    for (char c : line) {
        if (c == '"') { q = !q; continue; }
        if (!q && (c == ' ' || c == '\t')) { if (!cur.empty()) { out.push_back(cur); cur.clear(); } continue; }
        cur += c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}
static bool IsNumber(const std::string& s) {
    if (s.empty()) return false;
    char* end = nullptr;
    strtod(s.c_str(), &end);
    return end && *end == '\0';
}

bool Scene::Parse(const std::string& file) {
    records.clear();
    std::ifstream in(file);
    if (!in) { TraceLog(LOG_WARNING, "Scene: cannot open %s", file.c_str()); return false; }
    path = file;
    std::string line;
    while (std::getline(in, line)) {
        std::string t = TrimStr(line);
        if (t.empty() || t[0] == '#') continue;
        auto tok = Tokenize(t);
        if (tok.size() < 5) continue;
        Record r;
        r.prefab = tok[0];
        r.pos = { (float)atof(tok[1].c_str()), (float)atof(tok[2].c_str()), (float)atof(tok[3].c_str()) };
        r.rot = { (float)atof(tok[4].c_str()), 0, 0 };
        size_t i = 5;
        if (i < tok.size() && IsNumber(tok[i])) { r.rot.y = (float)atof(tok[i].c_str()); i++; }
        if (i < tok.size() && IsNumber(tok[i])) { r.rot.z = (float)atof(tok[i].c_str()); i++; }
        r.absY = false;
        for (; i < tok.size(); i++) {
            size_t eq = tok[i].find('=');
            if (eq == std::string::npos) continue;
            std::string k = tok[i].substr(0, eq), v = tok[i].substr(eq + 1);
            if (k == "abs") r.absY = (v == "1");
            else r.props[k] = v;
        }
        records.push_back(r);
    }
    return true;
}

void Scene::CollectTerrain(std::vector<TerrainPad>& pads, std::vector<Rectangle>& holes, std::vector<TerrainPath>& paths) const {
    for (const Record& r : records) {
        if (r.prefab == "path") {
            TerrainPath p;
            auto it = r.props.find("pts");
            if (it != r.props.end())
                for (auto& pair : SplitStr(it->second, ';')) {
                    auto xz = SplitStr(pair, ',');
                    if (xz.size() == 2) p.pts.push_back({ (float)atof(xz[0].c_str()), (float)atof(xz[1].c_str()) });
                }
            auto w = r.props.find("width");
            if (w != r.props.end()) p.width = (float)atof(w->second.c_str());
            paths.push_back(p);
            continue;
        }
        const PrefabInfo* p = FindPrefab(r.prefab);
        if (!p || !p->terrain) continue;
        Entity tmp;
        tmp.prefab = r.prefab; tmp.pos = r.pos; tmp.rot = r.rot; tmp.props = r.props; tmp.absY = r.absY;
        p->terrain(tmp, pads, holes);
    }
}

void Scene::SpawnRecords() {
    for (const Record& r : records) {
        if (r.prefab == "path") continue;
        Spawn(r.prefab, r.pos, r.rot, r.props, r.absY);
    }
}

bool Scene::Load(const std::string& file) {
    Clear();
    if (!Parse(file)) return false;
    SpawnRecords();
    return true;
}

bool Scene::Save(const std::string& file) const {
    std::ofstream out(file);
    if (!out) return false;
    out << "# What The Ground Keeps - world layout (edit by hand or with the in-game editor, F10)\n";
    out << "# prefab  x  y  z  yaw  [pitch roll]  [key=value ...]   (metres, degrees; y is above terrain unless abs=1)\n";
    // paths first (they shape the terrain)
    for (const Record& r : records) if (r.prefab == "path") out << r.prefab << " 0 0 0 0 pts=" << r.props.at("pts")
        << (r.props.count("width") ? " width=" + r.props.at("width") : std::string()) << "\n";
    std::string lastCat;
    for (const auto& up : ents) {
        const Entity& e = *up;
        if (e.tag == "runtime") continue;
        char buf[256];
        snprintf(buf, sizeof(buf), "%s %.2f %.2f %.2f %.1f", e.prefab.c_str(), e.pos.x, e.pos.y, e.pos.z, e.rot.x);
        out << buf;
        if (e.rot.y != 0.0f || e.rot.z != 0.0f) { snprintf(buf, sizeof(buf), " %.1f %.1f", e.rot.y, e.rot.z); out << buf; }
        if (e.absY) out << " abs=1";
        for (const auto& kv : e.props) {
            bool q = kv.second.find(' ') != std::string::npos;
            out << " " << kv.first << "=" << (q ? "\"" : "") << kv.second << (q ? "\"" : "");
        }
        out << "\n";
    }
    return true;
}

Entity* Scene::Spawn(const std::string& prefab, Vector3 pos, Vector3 rotDeg, const std::map<std::string, std::string>& props, bool absY) {
    const PrefabInfo* p = FindPrefab(prefab);
    if (!p) { TraceLog(LOG_WARNING, "Scene: unknown prefab '%s'", prefab.c_str()); return nullptr; }
    auto e = std::make_unique<Entity>();
    e->id = nextId++;
    e->prefab = prefab; e->pos = pos; e->rot = rotDeg; e->props = props; e->absY = absY || !p->snap;
    e->name = e->Str("name");
    e->tag = e->Str("tag");
    float s = e->Num("scale", 1.0f);
    e->scale = { e->Num("sx", s), e->Num("sy", s), e->Num("sz", s) };
    Entity* raw = e.get();
    ents.push_back(std::move(e));
    Rebuild(*raw);
    return raw;
}

void Scene::Rebuild(Entity& e) {
    const PrefabInfo* p = FindPrefab(e.prefab);
    if (!p) return;
    e.name = e.Str("name");
    e.tag = e.Str("tag", e.tag);
    const CachedPrefab& c = BuildPrefab(*p, e);
    e.model = c.model;
    e.boxes = c.boxes; e.cyls = c.cyls; e.lights = c.lights;
    e.interact = c.interact;
    e.castShadow = c.castShadow;
    if (p->behaviour) e.beh = p->behaviour(e); else e.beh.reset();
    UpdateTransform(e);
}

void Scene::UpdateTransform(Entity& e) {
    Entity* parent = e.Has("parent") ? Find(e.Str("parent")) : nullptr;
    if (parent && parent != &e) {
        // position/yaw are local to the parent (y relative to the parent origin)
        e.base = Vector3Transform(e.pos, MatPose(parent->base, parent->worldYaw));
        e.worldYaw = parent->worldYaw + e.rot.x * DEG2RAD;
    } else {
        e.base = e.pos;
        if (!e.absY) e.base.y = World().Height(e.pos.x, e.pos.z) + e.pos.y;
        e.worldYaw = e.rot.x * DEG2RAD;
    }
    e.xf = MatPose(e.base, e.worldYaw, e.rot.y * DEG2RAD, e.rot.z * DEG2RAD, e.scale);
    UnregisterColliders(e);
    RegisterColliders(e);
    if (!e.name.empty())
        for (auto& c : ents)
            if (c.get() != &e && c->Has("parent") && c->Str("parent") == e.name) UpdateTransform(*c);
}

void Scene::RegisterColliders(Entity& e) {
    float yaw = e.worldYaw;
    for (const CBox& lb : e.boxes) {
        CBox b = lb;
        Vector3 lc = { lb.c.x * e.scale.x, lb.c.y * e.scale.y, lb.c.z * e.scale.z };
        b.c = Vector3Add(e.base, Vector3Transform(lc, MatrixRotateY(yaw)));
        b.h = { fabsf(lb.h.x * e.scale.x), fabsf(lb.h.y * e.scale.y), fabsf(lb.h.z * e.scale.z) };
        b.yaw = lb.yaw + yaw;
        b.owner = e.id;
        e.boxIds.push_back(Phys().AddBox(b));
    }
    for (const CCyl& lc : e.cyls) {
        CCyl c = lc;
        Vector3 l = { lc.base.x * e.scale.x, lc.base.y * e.scale.y, lc.base.z * e.scale.z };
        c.base = Vector3Add(e.base, Vector3Transform(l, MatrixRotateY(yaw)));
        c.r = lc.r * fmaxf(e.scale.x, e.scale.z); c.h = lc.h * e.scale.y;
        c.owner = e.id;
        Phys().AddCyl(c);
    }
}

void Scene::UnregisterColliders(Entity& e) {
    Phys().RemoveOwner(e.id);
    e.boxIds.clear();
}

void Scene::Remove(Entity* e) {
    if (!e) return;
    UnregisterColliders(*e);
    ents.erase(std::remove_if(ents.begin(), ents.end(), [e](const std::unique_ptr<Entity>& u) { return u.get() == e; }), ents.end());
}

Entity* Scene::Find(const std::string& name) const {
    for (const auto& e : ents) if (e->name == name) return e.get();
    return nullptr;
}
std::vector<Entity*> Scene::FindAll(const std::string& prefab) const {
    std::vector<Entity*> v;
    for (const auto& e : ents) if (e->prefab == prefab) v.push_back(e.get());
    return v;
}
std::vector<Entity*> Scene::FindTag(const std::string& tag) const {
    std::vector<Entity*> v;
    for (const auto& e : ents) if (e->tag == tag) v.push_back(e.get());
    return v;
}
void Scene::SetLightGroup(const std::string& g, bool on) {
    for (auto& e : ents) for (auto& l : e->lights) if (l.group == g) l.on = on;
}
void Scene::SetLightGroupFlicker(const std::string& g, float f) {
    for (auto& e : ents) for (auto& l : e->lights) if (l.group == g) l.flicker = f;
}

void Scene::Update(float dt) {
    for (auto& up : ents) {
        Entity& e = *up;
        for (LightDef& l : e.lights) {
            if (l.flicker <= 0.0f) { l.cur = 1.0f; continue; }
            // Realistic failing-ballast behaviour: mostly steady, sudden dips, occasional stutter bursts.
            l.timer -= dt;
            if (l.timer <= 0.0f) {
                float r = Frand(0, 1);
                if (r < l.flicker * 0.5f) { l.cur = Frand(0.0f, 0.3f); l.timer = Frand(0.03f, 0.09f); }
                else if (r < l.flicker) { l.cur = Frand(0.5f, 0.85f); l.timer = Frand(0.02f, 0.06f); }
                else { l.cur = 1.0f; l.timer = Frand(0.2f, 3.0f) * (1.2f - l.flicker); }
            }
        }
        if (e.beh) e.beh->Update(e, dt);
    }
}

void Scene::Draw() {
    for (auto& up : ents) {
        Entity& e = *up;
        if (!e.visible) continue;
        if (e.model) Rdr().Draw(e.model, e.xf, e.tint, e.castShadow);
        if (e.beh) e.beh->Draw(e);
    }
}

void Scene::SubmitLights(Vector3 camPos) {
    for (auto& up : ents) {
        Entity& e = *up;
        for (const LightDef& ld : e.lights) {
            if (!ld.on || ld.cur <= 0.01f) continue;
            Light l = ld.light;
            l.pos = Vector3Transform(ld.light.pos, e.xf);
            if (Vector3Distance(l.pos, camPos) > l.range + 120.0f) continue;
            l.dir = Vector3Normalize(XfDir(e.xf, ld.light.dir));
            l.intensity *= ld.cur;
            Rdr().AddLight(l);
        }
    }
}

void Scene::Clear() {
    for (auto& e : ents) UnregisterColliders(*e);
    ents.clear();
    nextId = 1;
}
