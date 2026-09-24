// Hinged doors with physically swinging leaves and moving colliders.
//
// Local frame: origin at the bottom centre of the opening, leaf in the XY plane.
// Props: w, h, swing (+1/-1), locked=1, open=1
#include "prefab_util.h"
#include <map>

struct DoorStyle { Model3D* leaf; float w, h; };
static std::map<std::string, DoorStyle> g_doorStyles;

static Model3D* BuildLeaf(const std::string& style, float W, float H) {
    ModelBuilder mb;
    const float T = 0.045f;
    // leaf geometry spans x = 0..W from the hinge
    if (style == "door_glass") {
        mb.M(MAT_STEEL).Box({ 0.05f, H * 0.5f, 0 }, { 0.1f, H, T }, 0.01f);
        mb.M(MAT_STEEL).Box({ W - 0.05f, H * 0.5f, 0 }, { 0.1f, H, T }, 0.01f);
        mb.M(MAT_STEEL).Box({ W * 0.5f, 0.12f, 0 }, { W, 0.24f, T }, 0.01f);
        mb.M(MAT_STEEL).Box({ W * 0.5f, H - 0.06f, 0 }, { W, 0.12f, T }, 0.01f);
        mb.M(MAT_GLASS_DIRTY).Box({ W * 0.5f, H * 0.5f + 0.06f, 0 }, { W - 0.2f, H - 0.36f, 0.008f });
        mb.M(MAT_CHROME).Box({ W * 0.5f, 1.02f, 0.05f }, { W * 0.75f, 0.04f, 0.04f }, 0.01f);
        mb.M(MAT_CHROME).Box({ W * 0.5f, 1.02f, -0.05f }, { W * 0.75f, 0.04f, 0.04f }, 0.01f);
        int push = SignMaterial("push", "PUSH", ui::F_MONO_BOLD, 40, Color{ 230, 225, 210, 255 }, Color{ 150, 30, 25, 255 }, 160, 60, 0.3f);
        mb.M(push).BoxUV({ W * 0.5f, 1.25f, 0.006f }, { 0.22f, 0.08f, 0.002f });
    } else if (style == "door_metal") {
        mb.M(MAT_PAINT_GREY).Box({ W * 0.5f, H * 0.5f, 0 }, { W, H, T }, 0.01f);
        mb.M(MAT_RUST).Box({ W * 0.5f, 0.15f, 0.024f }, { W - 0.1f, 0.25f, 0.004f });
        mb.M(MAT_STEEL).Box({ W * 0.5f, 1.0f, -0.05f }, { W * 0.7f, 0.05f, 0.05f }, 0.01f);
        mb.M(MAT_STEEL).Cylinder({ W - 0.1f, 1.0f, 0.02f }, { W - 0.1f, 1.0f, 0.08f }, 0.03f, 0.03f, 10, true);
        int s = SignMaterial("emp_only", "EMPLOYEES\nONLY", ui::F_MONO_BOLD, 36, Color{ 20, 20, 20, 255 }, Color{ 225, 200, 60, 255 }, 256, 110, 0.5f);
        mb.M(s).BoxUV({ W * 0.5f, 1.55f, 0.025f }, { 0.4f, 0.17f, 0.002f });
    } else if (style == "door_iron") {
        mb.M(MAT_RUST).Box({ W * 0.5f, H * 0.5f, 0 }, { W, H, 0.09f }, 0.015f);
        for (int i = 0; i < 4; i++) mb.M(MAT_RUST).Box({ W * 0.5f, 0.3f + i * (H - 0.6f) / 3, 0.05f }, { W - 0.08f, 0.1f, 0.02f }, 0.01f);
        for (int i = 0; i < 4; i++) for (int k = 0; k < 7; k++)
            mb.M(MAT_STEEL).Sphere({ 0.08f + k * (W - 0.16f) / 6, 0.3f + i * (H - 0.6f) / 3, 0.065f }, 0.015f, 4, 6);
        mb.M(MAT_STEEL).Box({ W - 0.14f, 1.05f, 0.08f }, { 0.12f, 0.2f, 0.06f }, 0.01f);
        mb.M(MAT_STEEL).Cylinder({ W - 0.14f, 1.02f, 0.11f }, { W - 0.14f, 1.02f, 0.12f }, 0.02f, 0.02f, 8, true);
    } else {  // door_wood
        mb.M(MAT_PAINT_CREAM).Box({ W * 0.5f, H * 0.5f, 0 }, { W, H, T }, 0.008f);
        for (int r = 0; r < 2; r++) for (int c = 0; c < 2; c++) {
            Vector3 p{ W * (0.28f + c * 0.44f), 0.55f + r * 0.95f, 0 };
            for (int s = -1; s <= 1; s += 2) mb.M(MAT_PAINT_CREAM).Box({ p.x, p.y, s * 0.024f }, { W * 0.34f, 0.75f, 0.006f }, 0.004f);
        }
        mb.M(MAT_CHROME).Sphere({ W - 0.08f, 0.98f, 0.06f }, 0.03f, 6, 10);
        mb.M(MAT_CHROME).Sphere({ W - 0.08f, 0.98f, -0.06f }, 0.03f, 6, 10);
        mb.M(MAT_CHROME).Cylinder({ W - 0.08f, 0.98f, -0.06f }, { W - 0.08f, 0.98f, 0.06f }, 0.008f, 0.008f, 6, false);
    }
    return mb.Build(true);
}

struct DoorBeh : Behaviour {
    DoorStyle style;
    float angle = 0, vel = 0;
    bool wasMoving = false;
    void Update(Entity& e, float dt) override {
        float target = e.state["open"] > 0.5f ? e.Num("maxangle", 100.0f) * DEG2RAD * (e.Num("swing", 1) >= 0 ? 1 : -1) : 0.0f;
        // heavier doors are slower; old hinges resist a little
        float k = e.prefab == "door_iron" ? 6.0f : 14.0f;
        float damp = e.prefab == "door_iron" ? 5.0f : 6.5f;
        vel += ((target - angle) * k - vel * damp) * dt;
        angle += vel * dt;
        bool moving = fabsf(vel) > 0.05f;
        if (!moving && wasMoving && fabsf(angle) < 0.02f) {
            angle = 0; vel = 0;
            RaiseEvent("door_latch", e, e.base);
        }
        wasMoving = moving;
        // idle doors drift a hair in the draft
        if (!moving && e.state["open"] > 0.5f) angle += sinf((float)GetTime() * 0.7f + e.id) * 0.0004f;
        UpdateCollider(e);
    }
    Matrix LeafXf(const Entity& e) const {
        Matrix local = MatrixMultiply(MatrixRotateY(angle), MatrixTranslate(-style.w * 0.5f, 0, 0));
        return MatrixMultiply(local, e.xf);
    }
    void UpdateCollider(Entity& e) {
        if (e.boxIds.empty()) return;
        Vector3 c = Vector3Transform({ style.w * 0.5f, style.h * 0.5f, 0 }, LeafXf(e));
        Phys().UpdateBox(e.boxIds[0], c, e.worldYaw + angle);
    }
    void Draw(Entity& e) override { Rdr().Draw(style.leaf, LeafXf(e)); }
};

static void BuildDoorFrame(PrefabBuild& b, const Entity& e) {
    float W = e.Num("w", 0.95f), H = e.Num("h", 2.1f);
    // leaf collider (updated by the behaviour); threshold plate
    b.Collider({ 0, H * 0.5f, 0 }, { W, H, 0.06f }, SURF_WOOD);
    b.M(MAT_STEEL).Box({ 0, 0.005f, 0 }, { W, 0.01f, 0.12f });
    const char* prompt = "Open";
    b.Interact(prompt, { 0, 1.0f, 0 }, 2.0f, "door");
    b.castShadow = true;
}

static std::unique_ptr<Behaviour> MakeDoorBeh(Entity& e) {
    float W = e.Num("w", 0.95f), H = e.Num("h", 2.1f);
    std::string key = e.prefab + ":" + std::to_string(W) + ":" + std::to_string(H);
    auto it = g_doorStyles.find(key);
    if (it == g_doorStyles.end()) it = g_doorStyles.emplace(key, DoorStyle{ BuildLeaf(e.prefab, W, H), W, H }).first;
    auto d = std::make_unique<DoorBeh>();
    d->style = it->second;
    e.state["open"] = e.Num("open", 0);
    e.state["locked"] = e.Num("locked", 0);
    if (e.state["open"] > 0.5f) d->angle = e.Num("maxangle", 100.0f) * DEG2RAD * (e.Num("swing", 1) >= 0 ? 1 : -1);
    return d;
}

void RegisterDoorPrefabs() {
    for (const char* n : { "door_wood", "door_glass", "door_metal", "door_iron" }) {
        PrefabInfo p; p.name = n; p.category = "doors"; p.build = BuildDoorFrame; p.behaviour = MakeDoorBeh;
        p.geometryKeys = { "w", "h" };
        RegisterPrefab(p);
    }
}
