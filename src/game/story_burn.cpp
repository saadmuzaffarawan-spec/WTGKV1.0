// Fuel, fire, the sacrifice and the dawn.
#include "story_impl.h"
#include "prefab_util.h"
#include "menus.h"

#include <algorithm>

static Vector3 Flat(Vector3 v) { v.y = 0; return v; }
static float EntYawDeg(const char* name) { Entity* e = Scn().Find(name); return e ? e->worldYaw * RAD2DEG : 0.0f; }

// Burn-chapter state that doesn't need to live in Impl
struct BurnState {
    std::vector<float> lit;          // seconds since each trail point caught (-1 = not yet)
    std::vector<float> yaw;          // puddle orientation
    std::vector<Vector3> bigFires;   // things that caught properly: pumps, store, hatch
    std::vector<float> bigT;
    bool pumpsGone = false, storeGone = false, hatchGone = false;
    bool zainRun = false, covered = false;
    float pourStep = 0;
    Vector3 lastPour{ 1e9f, 0, 0 };
};
static BurnState F;

static int FuelMat() {
    static int m = -1;
    if (m < 0) {
        SurfaceMat s = Mat(MAT_BLOOD_SMEAR);
        s.name = "fuel_wet"; s.tint = Color{ 34, 30, 24, 185 }; s.wet = 1.0f; s.rough = 0.2f;
        m = AddMaterial(s);
    }
    return m;
}
static int AshMat() {
    static int m = -1;
    if (m < 0) {
        SurfaceMat s = Mat(MAT_BLOOD_SMEAR);   // translucent, so the scorch fades into the ground
        s.name = "ash"; s.tint = Color{ 26, 24, 23, 150 }; s.wet = 0.0f; s.rough = 1.0f;
        m = AddMaterial(s);
    }
    return m;
}
static Model3D* PuddleModel() {
    static Model3D* m = nullptr;
    if (m) return m;
    ModelBuilder mb;
    MeshBuilder& mm = mb.M(FuelMat());
    const int segs = 14;
    for (int i = 0; i < segs; i++) {
        float a0 = (float)i / segs * 2 * PI, a1 = (float)(i + 1) / segs * 2 * PI;
        float r0 = 0.32f * (0.75f + 0.35f * sinf(a0 * 3.0f + 1.0f)), r1 = 0.32f * (0.75f + 0.35f * sinf(a1 * 3.0f + 1.0f));
        mm.TriN({ 0, 0.006f, 0 }, { cosf(a1) * r1 * 1.4f, 0.006f, sinf(a1) * r1 }, { cosf(a0) * r0 * 1.4f, 0.006f, sinf(a0) * r0 }, { 0, 1, 0 }, { 0, 1, 0 }, { 0, 1, 0 });
    }
    m = mb.Build(false);
    return m;
}

static Vector3 HatchP() { Entity* e = Scn().Find("tank_hatch"); return e ? e->base : Vector3{ 26, 0, -12 }; }
static Vector3 PumpsP() { Entity* e = Scn().Find("canopy"); return e ? e->base : Vector3{ 16, 0, 0 }; }
static Vector3 StoreDoorP() { Entity* e = Scn().Find("store_front_door"); return e ? e->base : Vector3{ 38, 0, 0 }; }

static void RegisterBurnActions(Story::Impl& im) {
    Game& gg = im.g;
    Story::Impl* pim = &im;
    gg.actions["fuel_can"] = { [&gg](Entity&) -> std::string { return gg.held.empty() ? "Take the fuel can" : ""; },
                               [pim, &gg](Entity& e) {
                                   gg.held = "fuel_can";
                                   e.visible = false; e.interact.enabled = false;
                                   Phys().SetOwnerEnabled(e.id, false);
                                   audio::Play3D("pickup", e.FocusPoint(), 0.8f, 0.8f);
                                   pim->canFuel = 0.0f;
                                   pim->CompleteTask("can");
                                   gg.hud.Hint("Empty. The pumps still work.", 3.0f);
                               } };
    gg.actions["pump"] = { [pim, &gg](Entity&) -> std::string {
                               if (gg.held != "fuel_can") return "";
                               return pim->canFuel > 0.95f ? "" : "Fill the can";
                           },
                           [pim, &gg](Entity& e) {
                               audio::Play3D("pump_shutoff", e.FocusPoint(), 0.7f);
                               audio::Play3D("nozzle_hang", e.FocusPoint(), 0.6f);
                               audio::Amb().pumpMotor = 0; audio::Amb().fuelFlow = 0;
                               pim->canFuel = 1.0f;
                               pim->CompleteTask("fill");
                               gg.hud.Hint("Hold the left mouse button to pour.", 4.0f);
                           },
                           3.0f };
}

// Draw the soaked ground and submit the fire's light
void DrawFireTrail(Story::Impl& im) {
    auto& tr = im.trail;
    Model3D* pm = PuddleModel();
    Vector3 cam = im.g.camera.position;
    for (size_t i = 0; i < tr.size(); i++) {
        if (Vector3Distance(cam, tr[i]) > 70.0f) continue;
        float l = i < F.lit.size() ? F.lit[i] : -1.0f;
        Color tint = l < 0 ? WHITE : (l < 12.0f ? Color{ 60, 40, 30, 255 } : Color{ 40, 38, 36, 255 });
        Rdr().Draw(pm, MatPose(tr[i], i < F.yaw.size() ? F.yaw[i] : 0.0f), tint, false);
    }
    if (!im.fireLit) return;
    // a handful of fire lights over the burning clusters (nearest first)
    std::vector<std::pair<float, Vector3>> hot;
    for (size_t i = 0; i < tr.size(); i += 3)
        if (i < F.lit.size() && F.lit[i] >= 0.0f && F.lit[i] < 13.0f) hot.push_back({ Vector3Distance(cam, tr[i]), tr[i] });
    for (size_t k = 0; k < F.bigFires.size(); k++) hot.push_back({ Vector3Distance(cam, F.bigFires[k]) - 20.0f, F.bigFires[k] });
    std::sort(hot.begin(), hot.end(), [](auto& a, auto& b) { return a.first < b.first; });
    float t = (float)GetTime();
    for (size_t k = 0; k < hot.size() && k < 7; k++) {
        Light L;
        L.pos = Vector3Add(hot[k].second, { 0, 1.2f, 0 });
        bool big = hot[k].first < Vector3Distance(cam, hot[k].second) - 10.0f;
        float fl = 0.8f + 0.2f * sinf(t * 13.0f + k * 3.1f) * sinf(t * 7.3f + k);
        L.color = { 1.0f, 0.5f, 0.18f };
        L.intensity = (big ? 26.0f : 7.0f) * fl;
        L.range = big ? 28.0f : 11.0f;
        L.volumetric = big ? 0.12f : 0.05f;
        L.priority = 0.8f;
        Rdr().AddLight(L);
    }
}

void Story::Impl::UpdateFire(float dt) {
    if (g.chapter != CH_BURN) return;
    // pouring
    bool pouring = g.held == "fuel_can" && canFuel > 0.0f && !g.story->InCutscene() && !g.player.locked &&
                   (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2));
    auto& a = audio::Amb();
    a.pour = Damp(a.pour, pouring ? 0.7f : 0.0f, 10.0f, dt);
    if (g.held == "fuel_can") g.hud.Hint(canFuel > 0 ? TextFormat("fuel  %d%%", (int)(canFuel * 100)) : "the can is empty", 0.2f);
    if (pouring) {
        canFuel = fmaxf(0.0f, canFuel - dt / 30.0f);
        Vector3 f = Vector3Add(g.player.feet, Vector3Scale(Flat(g.player.Forward()), 0.5f));
        f.y = SurfaceY(f.x, f.z, g.player.feet.y + 1.0f);
        if (Vector3Distance(f, F.lastPour) > 0.55f) {
            trail.push_back(f);
            trailFire.push_back(0.0f);
            F.lit.push_back(-1.0f);
            F.yaw.push_back(Frand(0, 6.28f));
            F.lastPour = f;
            // whatever you soak near counts
            if (Vector3Distance(Flat(f), Flat(HatchP())) < 3.0f) CompleteTask("hatch");
            if (Vector3Distance(Flat(f), Flat(PumpsP())) < 4.5f) CompleteTask("pumps");
            if (Vector3Distance(Flat(f), Flat(StoreDoorP())) < 3.5f) CompleteTask("store");
        }
        if (canFuel <= 0.0f) { audio::Play("drip", 0.5f); g.hud.Hint("Empty. Fill it again at a pump.", 3.0f); }
    }
    Rdr().s.white = Damp(Rdr().s.white, 0.0f, 3.0f, dt);
    if (!fireLit) return;
    fireT += dt;
    // spread: a burning puddle lights its neighbours after a moment
    int burning = 0;
    float nearest = 1e9f;
    for (size_t i = 0; i < trail.size(); i++) {
        if (F.lit[i] < 0.0f) continue;
        F.lit[i] += dt;
        if (F.lit[i] < 13.0f) {
            burning++;
            nearest = fminf(nearest, Vector3Distance(trail[i], g.player.feet));
            float str = Saturate(F.lit[i] * 2.0f) * Saturate((13.0f - F.lit[i]) / 4.0f);
            if (GetRandomValue(0, 100) < (int)(55 * str)) fx.Emit(P_FIRE, Vector3Add(trail[i], { Frand(-0.35f, 0.35f), 0.05f, Frand(-0.3f, 0.3f) }), { Frand(-0.15f, 0.15f), Frand(1.0f, 2.2f), Frand(-0.15f, 0.15f) }, Frand(0.3f, 0.6f), Frand(0.1f, 0.2f) * (0.6f + str * 0.6f), Color{ 255, 130, 45, 200 });
            if (GetRandomValue(0, 100) < 6) fx.Emit(P_SMOKE, Vector3Add(trail[i], { 0, 1.2f, 0 }), { Frand(-0.2f, 0.2f), 1.2f, Frand(-0.2f, 0.2f) }, 4.0f, 0.8f, Color{ 30, 28, 26, 160 });
        }
        if (F.lit[i] > 0.35f)
            for (size_t j = 0; j < trail.size(); j++)
                if (F.lit[j] < 0.0f && Vector3Distance(trail[i], trail[j]) < 1.7f) F.lit[j] = 0.0f;
    }
    // it reaches the things you soaked
    auto boom = [&](Vector3 p, const char* what) {
        F.bigFires.push_back(p); F.bigT.push_back(0);
        fx.Burst(P_FIRE, Vector3Add(p, { 0, 1.0f, 0 }), 160, 6.0f, 1.1f, 0.45f, Color{ 255, 140, 50, 220 }, { 0, 4, 0 });
        fx.Burst(P_SMOKE, Vector3Add(p, { 0, 3.0f, 0 }), 30, 3.0f, 5.0f, 1.5f, Color{ 25, 22, 20, 200 }, { 0, 2, 0 });
        fx.Burst(P_SPARK, Vector3Add(p, { 0, 1.0f, 0 }), 80, 9.0f, 1.2f, 0.05f, Color{ 255, 200, 120, 255 });
        audio::Play3D("crash", p, 1.0f, 0.6f, 6.0f, 120.0f);
        audio::Play3D("fire_ignite", p, 1.0f, 0.7f, 6.0f, 120.0f);
        audio::Play("thunder", 0.6f, 0.6f);
        Rdr().s.white = fmaxf(Rdr().s.white, 0.35f);
        g.ShakeCamera(Saturate(1.2f - Vector3Distance(p, g.player.feet) / 60.0f));
        (void)what;
    };
    for (size_t i = 0; i < trail.size(); i++) {
        if (F.lit[i] < 0.0f) continue;
        if (!F.pumpsGone && Vector3Distance(Flat(trail[i]), Flat(PumpsP())) < 4.5f) {
            F.pumpsGone = true; boom(PumpsP(), "pumps");
            Scn().SetLightGroup("canopy", false); Scn().SetLightGroup("station", false);
            for (int k = 1; k <= 4; k++) if (Entity* p = Scn().Find(TextFormat("pump_%d", k))) F.bigFires.push_back(p->base), F.bigT.push_back(0);
        }
        if (!F.storeGone && Vector3Distance(Flat(trail[i]), Flat(StoreDoorP())) < 3.5f) {
            F.storeGone = true; boom(StoreDoorP(), "store");
            for (const char* grp : { "store", "storage", "store_sign", "open_sign" }) Scn().SetLightGroup(grp, false);
            if (Entity* st = Scn().Find("store")) for (Vector3 lp : { Vector3{ -4, 0.3f, 0 }, Vector3{ 3, 0.3f, 3 }, Vector3{ 5, 0.3f, -4 } }) F.bigFires.push_back(st->LocalToWorld(lp)), F.bigT.push_back(0);
        }
        if (!F.hatchGone && Vector3Distance(Flat(trail[i]), Flat(HatchP())) < 3.0f) {
            F.hatchGone = true; boom(HatchP(), "hatch");
            // what lives under the tanks screams
            Vector3 h = HatchP();
            g.story->side.Wait(0.6f).Do([h]() { audio::Play3D("screech_creature", h, 1.0f, 0.6f, 6.0f, 150.0f); })
                .Wait(0.9f).Do([h]() { audio::Play3D("growl", h, 1.0f, 0.4f, 6.0f, 150.0f); })
                .Wait(1.2f).Do([h]() { audio::Play3D("screech_creature", h, 0.9f, 0.45f, 6.0f, 150.0f); });
        }
    }
    // big fires: tall flames and smoke columns
    for (size_t k = 0; k < F.bigFires.size(); k++) {
        F.bigT[k] += dt;
        Vector3 p = F.bigFires[k];
        for (int n = 0; n < 5; n++)
            fx.Emit(P_FIRE, Vector3Add(p, { Frand(-1.4f, 1.4f), Frand(0.0f, 1.2f), Frand(-1.4f, 1.4f) }), { Frand(-0.4f, 0.4f), Frand(2.0f, 4.0f), Frand(-0.4f, 0.4f) }, Frand(0.5f, 1.0f), Frand(0.25f, 0.55f), Color{ 255, 125, 40, 190 });
        if (GetRandomValue(0, 100) < 25) fx.Emit(P_SMOKE, Vector3Add(p, { Frand(-1, 1), 4.0f, Frand(-1, 1) }), { Frand(-0.3f, 0.6f), 2.2f, Frand(-0.3f, 0.3f) }, 7.0f, 2.2f, Color{ 22, 20, 19, 190 });
        if (GetRandomValue(0, 100) < 8) fx.Emit(P_EMBER, Vector3Add(p, { 0, 2.0f, 0 }), { Frand(-1, 1), Frand(2, 5), Frand(-1, 1) }, 3.0f, 0.03f, Color{ 255, 170, 90, 255 });
        nearest = fminf(nearest, Vector3Distance(p, g.player.feet) - 4.0f);
    }
    a.fire = Damp(a.fire, Saturate(1.0f - nearest / 40.0f) * (burning > 0 || !F.bigFires.empty() ? 1.0f : 0.0f), 2.0f, dt);
    // creatures caught in it
    for (auto& c : creatures) {
        if (c.state == 4 || c.burn > 0) continue;
        for (size_t i = 0; i < trail.size(); i += 2)
            if (F.lit[i] >= 0 && F.lit[i] < 13.0f && Vector3Distance(c.pos, trail[i]) < 2.2f) { c.burn = 0.01f; audio::Play3D("screech_creature", c.pos, 1.0f); break; }
        for (Vector3 p : F.bigFires) if (c.burn <= 0 && Vector3Distance(c.pos, p) < 5.0f) c.burn = 0.01f;
    }
    // standing in it hurts even the dead
    for (size_t i = 0; i < trail.size(); i++)
        if (F.lit[i] >= 0 && F.lit[i] < 13.0f && Vector3Distance(Flat(trail[i]), Flat(g.player.feet)) < 0.7f && !F.covered) {
            g.fearPulse = 1.0f;
            g.player.fear = 1.0f;
            if (fmodf(fireT, 1.0f) < dt) { audio::Play("gasp", 0.6f); g.hud.Hint("Get out of the fire.", 1.5f); }
            break;
        }
}

// Zain beside you, then running for the hatch
static void UpdateZain(Story::Impl& im, float dt, float t) {
    Game& g = im.g;
    Actor* z = g.A("zain");
    if (!z || F.covered) return;
    Vector3 target;
    float sp;
    if (F.zainRun) { target = Vector3Add(HatchP(), { -1.2f, 0, 1.0f }); sp = 3.2f; }
    else {
        target = Vector3Subtract(g.player.feet, Vector3Scale(Flat(g.player.Forward()), 1.8f));
        float len = Vector3Length(Flat(Vector3Subtract(target, z->pos)));
        sp = len > 6.0f ? 3.8f : len > 1.6f ? 2.2f : 0.0f;
    }
    Vector3 to = Flat(Vector3Subtract(target, z->pos));
    float len = Vector3Length(to);
    if (sp > 0 && len > 0.3f) {
        z->pos = Vector3Add(z->pos, Vector3Scale(to, fminf(sp * dt, len) / len));
        z->yaw = DampAngle(z->yaw, atan2f(to.x, to.z), 8.0f, dt);
        z->SetPose(sp > 3.0f ? PoseRun(t * 1.4f) : PoseWalk(t, 1.0f, 0));
        if (fmodf(t, sp > 3.0f ? 0.3f : 0.48f) < dt) audio::PlayFootstep(SURF_ASPHALT, z->pos, 0.35f, false);
    } else if (F.zainRun) z->SetPose(PoseCry(t));
    else { z->SetPose(PoseStand(t)); z->lookAt = g.player.EyePos(); z->lookWeight = 0.5f; }
    z->pos.y = SurfaceY(z->pos.x, z->pos.z, z->pos.y + 1.0f);
}

// ---------------------------------------------------------------------------
// Chapter: Burn
// ---------------------------------------------------------------------------
void Story::Impl::BuildBurn(Script& s) {
    Game& gg = g;
    s.Do([this, &gg]() {
        F = BurnState();
        trail.clear(); trailFire.clear(); fireLit = false; fireT = 0; canFuel = 0;
        creatures.clear();
        gg.story->cutscene = false;
        gg.hud.FadeInstant(1.0f);
        gg.hud.FadeTo(0.0f, 0.35f);
        gg.hud.Title("BURN", "fire is the only thing the ground gives back", 6.0f);
        Vector3 p = CollegeP(0.0f, 0.3f, 15.0f);
        gg.player.Spawn(p, EntYawDeg("college"));
        gg.player.flashOn = true;
        gg.held = "";
        Actor* z = gg.SpawnActor("zain", SpecZain(), CollegeP(1.2f, 0.3f, 13.8f), EntYawDeg("college"));
        z->pos.y = SurfaceY(z->pos.x, z->pos.z, z->pos.y + 1.0f);
        // they're still at the pumps
        for (int i = 0; i < 2; i++) {
            Entity* pm = Scn().Find(TextFormat("pump_%d", i + 2));
            Vector3 at = pm ? pm->LocalToWorld({ 0, 0, 0.9f }) : Vector3{ 16, 0, 0 };
            AddCreature(TextFormat("drinker_%d", i), 5, at);
        }
        Scn().Spawn("fuel_can", { 37.6f, 0.03f, 3.2f }, { 30, 0, 0 }, { { "name", "fuel_can" }, { "tag", "runtime" } });
        tasks = { { "can", "find a fuel can (by the store)" }, { "fill", "fill it at a pump" }, { "hatch", "soak the tank hatch" },
                  { "pumps", "soak the pumps" }, { "store", "soak the store" }, { "light", "light it" } };
        RegisterBurnActions(*this);
        checkpoint = p; checkpointYaw = EntYawDeg("college");
        onCaught = [this]() { for (auto& c : creatures) { c.pos = c.home; c.state = 5; c.t = 0; } };
    });
    s.Wait(2.5f).Say("ZAIN", "What are you going to do?", 2.2f, 0.3f)
     .Say("ADAM", "Give it back what it's owed.", 2.4f);
    s.Objective("Find fuel. Soak the hatch, the pumps and the store. (hold Tab for the list)");
    s.Run([this](float t, float dt) { UpdateZain(*this, dt, t); JobTask* h = Task("hatch"), *p = Task("pumps"), *st = Task("store"); return h->done && p->done && st->done; });
    s.Do([&gg]() { gg.hud.Objective("Light it. Stand at the end of a trail and strike a match."); });
    // strike a match over a soaked puddle
    s.Run([this, &gg](float t, float dt) {
        UpdateZain(*this, dt, t);
        int best = -1; float bd = 1.6f;
        for (size_t i = 0; i < trail.size(); i++) { float d = Vector3Distance(Flat(trail[i]), Flat(gg.player.feet)); if (d < bd) { bd = d; best = (int)i; } }
        if (best < 0 || gg.player.locked) return false;
        gg.hud.SetFocus(true);
        gg.hud.SetPrompt("E  Strike a match", 0.0f);
        if (IsKeyPressed(KEY_E) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) {
            audio::Play("match", 0.9f);
            F.lit[best] = 0.0f;
            fireLit = true;
            gg.TakeItem("matches");
            CompleteTask("light");
            audio::Play3D("fire_ignite", trail[best], 1.0f);
            fx.Burst(P_FIRE, trail[best], 30, 2.0f, 0.8f, 0.4f, Color{ 255, 160, 60, 255 }, { 0, 2, 0 });
            return true;
        }
        return false;
    });
    s.Do([&gg]() { gg.hud.Objective(""); gg.player.fear = 0.8f; });
    s.Run([this](float t, float dt) { UpdateZain(*this, dt, t); return t > 14.0f || (F.pumpsGone && F.storeGone && F.hatchGone && t > 6.0f); });
    // Zain breaks for the hatch
    s.Say("ZAIN", "Mum. MUM - she's still down there!", 2.4f, 0.1f);
    s.Do([this, &gg]() {
        F.zainRun = true;
        gg.timeScale = 0.75f;
        gg.hud.Objective("ZAIN.");
        audio::Play("heartbeat", 0.9f);
        // the fire comes around the hatch towards him
        Vector3 h = HatchP();
        for (int k = 0; k < 14; k++) {
            float ang = k / 14.0f * 2 * PI;
            Vector3 p = Vector3Add(h, { cosf(ang) * 4.5f, 0, sinf(ang) * 4.5f });
            p.y = SurfaceY(p.x, p.z, p.y + 3.0f);
            trail.push_back(p); trailFire.push_back(0); F.lit.push_back(k == 0 ? 0.0f : -1.0f); F.yaw.push_back(Frand(0, 6.28f));
        }
    });
    s.Run([this, &gg](float t, float dt) {
        UpdateZain(*this, dt, t);
        Actor* z = gg.A("zain");
        return z && Vector3Distance(Flat(z->pos), Flat(gg.player.feet)) < 2.2f && t > 1.0f;
    });
    // ---- he covers him
    s.Do([this, &gg]() {
        F.covered = true;
        gg.player.locked = true;
        gg.story->cutscene = true;
        gg.timeScale = 1.0f;
        gg.hud.Objective("");
        gg.hud.Letterbox(true);
        Actor* z = gg.A("zain");
        Vector3 zp = z ? z->pos : gg.player.feet;
        if (z) z->SetPose(PoseCry(0), true);
        Actor* ad = gg.SpawnActor("adam", SpecAdam(), zp, 0);
        ad->yaw = z ? z->yaw : 0; ad->pos = Vector3Subtract(zp, Vector3Scale(YawDir(ad->yaw), 0.35f));
        ad->SetPose(PoseCoverOver(0), true);
        SetCamPath({ { 0.0f, Vector3Add(zp, { 3.2f, 1.2f, 2.4f }), Vector3Add(zp, { 0, 0.6f, 0 }), 48 },
                     { 6.0f, Vector3Add(zp, { 2.2f, 0.9f, 1.6f }), Vector3Add(zp, { 0, 0.55f, 0 }), 40 },
                     { 12.0f, Vector3Add(zp, { 1.2f, 3.5f, 1.0f }), Vector3Add(zp, { 0, 0.3f, 0 }), 42 } });
        // the ring closes
        for (size_t i = 0; i < F.lit.size(); i++) if (F.lit[i] < 0) F.lit[i] = 0.0f;
        F.bigFires.push_back(zp); F.bigT.push_back(0);
        audio::Play("fire_ignite", 1.0f, 0.8f);
    });
    s.Say("ZAIN", "Adam - ADAM -", 1.8f, 0.2f)
     .Say("ADAM", "I've got you. Close your eyes.", 2.8f, 0.6f)
     .Do([&gg]() {
         if (Actor* ad = gg.A("adam")) ad->tint = Color{ 90, 60, 50, 255 };
         Rdr().s.asciiFull = 0.4f;
         audio::Amb().muffle = 0.5f;
     })
     .Wait(2.5f)
     .Do([&gg]() { if (Actor* ad = gg.A("adam")) ad->tint = Color{ 30, 26, 24, 255 }; Rdr().s.asciiFull = 0.8f; })
     .Wait(2.0f)
     .Do([&gg]() { gg.hud.FadeTo(1.0f, 0.6f); audio::Amb().muffle = 0.9f; })
     .Wait(2.4f)
     .Do([this, &gg]() {
         EndCamPath();
         Rdr().s.asciiFull = 0.0f;
         audio::Amb().muffle = 0.0f;
         audio::Amb().fire = 0.0f;
         gg.story->pendingChapter = CH_END;
     });
}

// ---------------------------------------------------------------------------
// Epilogue: dawn. Ash, smoke, a boy crying, and the ground breathing.
// ---------------------------------------------------------------------------
void BuildEpilogue(Story::Impl& im, Script& s) {
    Game& gg = im.g;
    Story::Impl* pim = &im;
    s.Do([pim, &gg]() {
        gg.story->cutscene = true;
        gg.player.locked = true;
        gg.hud.FadeInstant(1.0f);
        gg.hud.FadeTo(0.0f, 0.2f);
        gg.hud.Letterbox(true);
        gg.hud.ShowTasks(false);
        pim->tasks.clear();
        pim->creatures.clear();
        // dawn: grey-pink light, no moon, everything that burned still smoking
        auto& r = Rdr().s;
        r.moonColor = { 1.2f, 0.88f, 0.72f };   // a low, cold sun through smoke
        r.moonDir = Vector3Normalize({ 0.8f, 0.22f, 0.2f });
        r.moonBright = 1.0f;
        r.skyAmbient = { 0.34f, 0.32f, 0.36f };
        r.groundAmbient = { 0.12f, 0.10f, 0.09f };
        r.fogColor = { 0.42f, 0.38f, 0.38f };
        r.exposure = 2.3f;
        r.skyZenith = { 0.10f, 0.13f, 0.2f };
        r.skyHorizon = { 0.55f, 0.38f, 0.32f };
        r.glowColor = { 0.3f, 0.12f, 0.05f };
        r.glowDir = { 0.8f, 0.0f, 0.2f };
        r.stars = 0.0f;
        for (const char* grp : { "store", "storage", "store_sign", "open_sign", "canopy", "station", "street" }) Scn().SetLightGroup(grp, false);
        Vector3 h = HatchP();
        for (Vector3 p : { PumpsP(), StoreDoorP() }) pim->decals.Pool({ p.x, 0, p.z }, 3.2f, AshMat(), (uint32_t)(p.x * 10), 1.3f);
        pim->decals.Pool({ h.x, 0, h.z }, 1.6f, AshMat(), 31, 1.2f);
        // everything that caught is charred
        for (auto& e : Scn().ents) {
            const std::string& pf = e->prefab;
            if (pf == "store_building" || pf == "gas_canopy" || pf == "gas_pump" || pf == "pump_island" || pf == "price_sign" ||
                pf == "store_counter" || pf == "store_shelf" || pf == "coolers" || pf == "door_glass" || pf == "tank_hatch")
                e->tint = Color{ 70, 64, 60, 255 };
        }
        pim->decals.Pool({ h.x - 1.2f, 0, h.z + 1.0f }, 0.9f, AshMat(), 77, 2.0f, 0.4f);   // where he was
        Actor* z = gg.SpawnActor("zain", SpecZain(), Vector3Add(h, { -0.4f, 0, 1.9f }), 200.0f);
        z->pos.y = SurfaceY(z->pos.x, z->pos.z, z->pos.y + 1.0f);
        z->SetPose(PoseCry(0), true);
        Vector3 zp = z->pos;
        pim->SetCamPath({ { 0.0f, Vector3Add(zp, { 2.4f, 1.1f, 2.6f }), Vector3Add(zp, { 0, 0.7f, 0 }), 40 },
                          { 10.0f, Vector3Add(zp, { 1.6f, 1.6f, 1.8f }), Vector3Add(zp, { 0, 0.6f, 0 }), 44 },
                          { 22.0f, Vector3Add(zp, { 0.6f, 9.0f, 1.0f }), Vector3Add(zp, { 0, 0, 0 }), 52 },
                          { 34.0f, Vector3Add(zp, { 0.2f, 40.0f, 0.4f }), Vector3Add(zp, { 0, 0, 0 }), 58 } });
        gg.hud.Title("DAWN", "", 4.0f);
    });
    s.Run([pim, &gg](float t, float dt) {
        if (Actor* z = gg.A("zain")) z->SetPose(PoseCry(t));
        for (Vector3 p : { PumpsP(), StoreDoorP() })
            if (GetRandomValue(0, 100) < 20) pim->fx.Emit(P_SMOKE, Vector3Add(p, { Frand(-2, 2), 1.0f, Frand(-2, 2) }), { Frand(0.1f, 0.5f), 1.2f, 0 }, 9.0f, 2.0f, Color{ 120, 116, 114, 60 });
        if (fmodf(t, 3.1f) < dt && t < 20.0f) audio::Play("breath_out", 0.35f, 1.5f);   // sobbing, small
        Rdr().s.asciiFull = Saturate((t - 18.0f) / 14.0f);
        return t > 33.0f;
    });
    s.Say("ZAIN", "I'm sorry.", 2.0f, 1.0f).Say("ZAIN", "I'm sorry. I'm sorry.", 3.0f, 1.0f);
    s.Do([&gg]() { gg.hud.FadeTo(1.0f, 0.5f); });
    s.Wait(2.5f);
    // the last thing: the burnt hatch, and something under it breathing
    s.Do([pim, &gg]() {
        Rdr().s.asciiFull = 0.0f;
        Vector3 h = HatchP();
        pim->SetCamPath({ { 0.0f, Vector3Add(h, { 1.2f, 0.5f, 1.4f }), Vector3Add(h, { 0, 0, 0 }), 38 }, { 7.0f, Vector3Add(h, { 1.0f, 0.4f, 1.1f }), Vector3Add(h, { 0, 0, 0 }), 34 } });
        if (Actor* z = gg.A("zain")) z->visible = false;
        gg.hud.FadeTo(0.0f, 0.4f);
    });
    s.Wait(3.0f).Do([pim]() { Vector3 h = HatchP(); audio::Play3D("breath_out", h, 1.0f, 0.5f); pim->fx.Burst(P_ASH, Vector3Add(h, { 0, 0.1f, 0 }), 25, 0.8f, 2.0f, 0.03f, Color{ 60, 58, 56, 200 }, { 0, 1.0f, 0 }); });
    s.Wait(2.5f).Do([&gg]() { gg.hud.FadeTo(1.0f, 2.0f); });
    s.Wait(1.2f).Do([pim, &gg]() {
        pim->EndCamPath();
        gg.hud.Letterbox(false);
        gg.flags.insert("finished");
        gg.mode = Mode::Credits;
        CreditsEnter();
    });
}

// Test hook for headless screenshots (WTGK_TEST=cow|fire), fired once shortly after a chapter begins.
void StoryTestHook(Story::Impl& im, const std::string& what) {
    Game& g = im.g;
    Vector3 f = Vector3Add(g.player.feet, Vector3Scale(Flat(g.player.Forward()), 5.0f));
    if (what == "polaroid") im.showPolaroid = 6.0f;
    if (what == "cow") {
        im.AddCreature("cow", 1, f);
        if (Creature* c = im.FindCreature("cow")) { c->yaw = g.player.yaw + PI; c->hitCd = 99; }
    } else if (what == "fire") {
        Vector3 a = g.player.feet, b = PumpsP();
        for (float u = 0; u <= 1.0f; u += 0.02f) {
            Vector3 p = Vector3Lerp(a, b, u);
            p.x += sinf(u * 20) * 0.5f;
            p.y = SurfaceY(p.x, p.z, 50.0f);
            im.trail.push_back(p); im.trailFire.push_back(0); F.lit.push_back(u < 0.02f ? 0.0f : -1.0f); F.yaw.push_back(u * 40);
        }
        im.fireLit = true;
    }
}
