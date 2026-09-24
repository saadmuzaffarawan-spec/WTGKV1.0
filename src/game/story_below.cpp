// The iron door, the invasion, the underground, and Zain.
// Also the creature AI shared by the later chapters (crawlers, the skeleton cow,
// reanimated corpses, the Dragger).
#include "story_impl.h"
#include "prefab_util.h"

static Vector3 EntP(const char* name, float x, float y, float z) {
    Entity* e = Scn().Find(name);
    if (!e) return { x, y, z };
    return Vector3Transform({ x, y, z }, MatPose(e->base, e->worldYaw));
}
static float EntYaw(const char* name) { Entity* e = Scn().Find(name); return e ? e->worldYaw : 0.0f; }
static Vector3 CaveP(float x, float y, float z) { return EntP("cavern", x, y, z); }
static Vector3 StairP(float x, float y, float z) { return EntP("under_stairs", x, y, z); }
static Vector3 Flat(Vector3 v) { v.y = 0; return v; }
void UpdateBelowWorld(Story::Impl& im, float dt);
static void WakeCorpse(Story::Impl& im, const std::string& n);

// ---------------------------------------------------------------------------
// Creature management
// ---------------------------------------------------------------------------
Creature* Story::Impl::FindCreature(const std::string& name) {
    for (auto& c : creatures) if (c.actor == name) return &c;
    return nullptr;
}

void Story::Impl::AddCreature(const std::string& name, int kind, Vector3 pos) {
    Creature c;
    c.actor = name;
    c.kind = kind;
    c.pos = pos; c.home = pos;
    c.state = kind == 5 ? 5 : 2;
    c.wake = kind == 0 ? 1.0f : 0.0f;
    c.phase = Frand(0, 10);
    if (kind == 0 || kind == 5) {
        if (!g.A(name)) g.SpawnActor(name, SpecCrawler((uint32_t)(1 + creatures.size() % 3)), pos, 0);
        if (kind == 5) { c.kind = 0; c.state = 5; }
    } else if (kind == 2) {
        if (!g.A(name)) g.SpawnActor(name, SpecCrawler(2), pos, 0);
    } else if (kind == 3) {
        if (!g.A(name)) g.SpawnActor(name, SpecDragger(), pos, 0);
        c.wake = 1.0f;
    }
    if (Actor* a = g.A(name)) { c.yaw = a->yaw; g.actors[name].action = ""; }
    creatures.push_back(c);
}

// A creature landed a blow. Three and the ground has you (back to the checkpoint).
static void HitPlayer(Story::Impl& im, Creature& c) {
    Game& g = im.g;
    c.hitCd = c.kind == 3 ? 2.0f : 1.3f;
    Vector3 dir = Vector3Normalize(Flat(Vector3Subtract(g.player.feet, c.pos)));
    g.player.vel = Vector3Add(g.player.vel, Vector3Scale(dir, 5.0f));
    g.player.fear = 1.0f;
    g.ShakeCamera(0.6f);
    g.fearPulse = 1.0f;
    im.blood = fminf(1.0f, im.blood + 0.4f);
    audio::Play3D(c.kind == 1 ? "bone_crack" : "growl", c.pos, 1.0f, c.kind == 3 ? 0.6f : 1.0f);
    audio::Play("gasp", 0.8f);
    im.playerHits++;
    if (im.playerHits >= 3 && !g.Flag("caught")) {
        g.SetFlag("caught");
        g.player.locked = true;
        g.hud.FadeTo(1.0f, 1.5f);
        audio::Amb().muffle = 0.8f;
        Story::Impl* pim = &im;
        g.story->side.Wait(1.6f).Do([pim]() {
            Game& gg = pim->g;
            gg.player.Spawn(pim->checkpoint, pim->checkpointYaw);
            pim->playerHits = 0;
            pim->blood = 0;
            for (auto& cr : pim->creatures) { cr.hitCd = 3.0f; cr.fear = 0; }
            if (pim->onCaught) pim->onCaught();
            gg.hud.FadeTo(0.0f, 0.6f);
            audio::Amb().muffle = 0.0f;
            gg.player.locked = false;
            gg.flags.erase("caught");
            gg.hud.Hint("The ground lets you go. This time.", 3.0f);
        });
    }
}

void Story::Impl::UpdateCreatures(float dt) {
    if (g.chapter >= CH_KEY) {
        blood = Damp(blood, 0.0f, 0.35f, dt);
        g.fearPulse = Damp(g.fearPulse, 0.0f, 1.2f, dt);
    }
    Vector3 feet = g.player.feet, eye = g.player.EyePos(), fwd = g.player.Forward();
    bool torch = g.player.flashOn && g.player.battery > 0.02f;
    bool paused = g.story && g.story->InCutscene();
    for (auto& c : creatures) {
        if (c.state == 4) continue;
        Actor* a = c.actor.empty() || c.kind == 1 ? nullptr : g.A(c.actor);
        c.t += dt; c.hitCd -= dt;
        Vector3 to = Flat(Vector3Subtract(feet, c.pos));
        float d = Vector3Length(to);
        // burning (the finale)
        if (c.burn > 0.0f) {
            c.burn += dt;
            if (a) { a->tint = ColorLerp(a->tint, Color{ 30, 20, 18, 255 }, Saturate(dt * 0.8f)); a->SetPose(PoseCrawl(c.t * 3.0f)); }
            if (fmodf(c.t, 0.15f) < dt) fx.Emit(P_FIRE, Vector3Add(c.pos, { Frand(-0.3f, 0.3f), 0.5f, Frand(-0.3f, 0.3f) }), { 0, 1.5f, 0 }, 0.8f, 0.4f, Color{ 255, 150, 60, 255 });
            if (c.burn > 3.0f) { c.state = 4; if (a) a->visible = false; audio::Play3D("screech_creature", c.pos, 0.8f, 0.7f); }
            continue;
        }
        // waking up
        if (c.wake < 1.0f) {
            c.wake = fminf(1.0f, c.wake + dt / (c.kind == 1 ? 2.8f : 1.4f));
            if (a) {
                a->pitch = Lerp(-1.5f, 0.0f, SmoothStep(0.3f, 1.0f, c.wake));
                a->pos = c.pos; a->pos.y = SurfaceY(c.pos.x, c.pos.z, c.pos.y + 1.0f) + (1.0f - c.wake) * 0.12f;
                a->SetPose(PoseCrawl(c.t * 5.0f));
            }
            if (fmodf(c.t, 0.35f) < dt) audio::Play3D(c.kind == 1 ? "rattle_bones" : "bone_crack", c.pos, 0.6f, Frand(0.8f, 1.2f));
            continue;
        }
        if (paused) continue;
        // the torch: crawlers and corpses hate it; the cow only slows; the Dragger doesn't care
        Vector3 head = Vector3Add(c.pos, { 0, 0.6f, 0 });
        Vector3 toC = Vector3Subtract(head, eye);
        float dc = Vector3Length(toC);
        bool lit = torch && dc < 15.0f && Vector3DotProduct(Vector3Scale(toC, 1.0f / fmaxf(dc, 0.01f)), fwd) > 0.94f && Phys().LineOfSight(eye, head);
        c.fear = lit ? c.fear + dt : fmaxf(0.0f, c.fear - dt * 0.6f);
        float speed = 0.0f;
        Vector3 goal = c.pos;
        int kind = c.kind;
        if (c.state == 5) {   // feeding: hunched over whatever it found, until you come close
            if (a) { a->SetPose(PoseCrawl(0.15f + sinf(c.t * 7.0f) * 0.04f)); a->lookWeight = 0; }
            if (fmodf(c.t + c.phase, 2.3f) < dt) audio::Play3D("squelch", c.pos, 0.6f, Frand(0.7f, 1.0f));
            if (d < 7.0f || (lit && d < 12.0f)) { c.state = lit ? 3 : 2; c.t = 0; audio::Play3D("screech_creature", c.pos, 0.9f); }
            continue;
        }
        if (c.state == 0) { if (d < 14.0f) c.state = 2; }
        if (c.state == 2) {
            if ((kind == 0 || kind == 2) && c.fear > 0.7f) { c.state = 3; c.t = 0; audio::Play3D("screech_creature", c.pos, 0.8f, 1.2f); }
            goal = feet;
            speed = kind == 0 ? 3.1f : kind == 1 ? (lit ? 2.2f : 3.35f) : kind == 2 ? 2.3f : 2.7f;
            if (d < 1.6f) speed *= 0.3f;
        } else if (c.state == 3) {
            goal = Vector3Subtract(c.pos, Vector3Scale(to, 3.0f / fmaxf(d, 0.1f)));
            speed = 3.6f;
            if (c.t > 2.5f) { c.state = 2; c.fear = 0; }
        }
        if (speed > 0.0f) {
            Vector3 dir = Flat(Vector3Subtract(goal, c.pos));
            float len = Vector3Length(dir);
            if (len > 0.05f) {
                dir = Vector3Scale(dir, 1.0f / len);
                float want = atan2f(dir.x, dir.z);
                c.yaw = DampAngle(c.yaw, want, kind == 1 ? 3.0f : 6.0f, dt);
                Vector3 step = Vector3Scale(YawDir(c.yaw), speed * dt * Saturate(1.2f - fabsf(WrapAngle(want - c.yaw))));
                Vector3 np = Vector3Add(c.pos, step);
                // don't walk through walls: probe ahead
                RayHit hit;
                if (!Phys().Raycast(Vector3Add(c.pos, { 0, 0.5f, 0 }), YawDir(c.yaw), 0.6f, hit)) c.pos = np;
                c.gait += speed * dt * (kind == 1 ? 0.45f : 0.9f);
            }
        }
        c.pos.y = SurfaceY(c.pos.x, c.pos.z, c.pos.y + 1.0f);
        if (a) {
            a->pos = c.pos; a->yaw = c.yaw; a->pitch = 0;
            if (kind == 3) a->SetPose(PoseWalk(c.gait * 0.5f, 1.0f, 1));
            else a->SetPose(PoseCrawl(c.gait));
            a->lookAt = eye; a->lookWeight = 0.8f;
        }
        // sounds
        if (kind == 1 && fmodf(c.gait, 0.5f) < speed * dt * 0.45f) audio::Play3D("rattle_bones", c.pos, 0.5f, Frand(0.9f, 1.1f));
        if (kind != 1 && fmodf(c.t + c.phase, kind == 3 ? 1.1f : 0.32f) < dt && speed > 0) audio::PlayFootstep(kind == 3 ? SURF_MUD : SURF_FLESH, c.pos, 0.4f, false);
        if (fmodf(c.t + c.phase * 3.0f, 6.0f) < dt) audio::Play3D(kind == 3 ? "breath_out" : (kind == 1 ? "growl" : "screech_creature"), c.pos, 0.7f, kind == 1 ? 0.5f : Frand(0.8f, 1.2f));
        // attack
        float reach = kind == 1 ? 1.7f : kind == 3 ? 1.3f : 1.15f;
        if (d < reach && c.hitCd <= 0.0f && c.state == 2 && !g.Flag("caught") && !g.player.locked) HitPlayer(*this, c);
        g.player.fear = fmaxf(g.player.fear, Saturate(1.0f - d / 16.0f));
    }
    // the rest of the world below moves too
    UpdateBelowWorld(*this, dt);
}

// ---------------------------------------------------------------------------
// The skeleton cow, drawn from parts
// ---------------------------------------------------------------------------
static Model3D* CowModel(int part) {
    static Model3D* m[4] = {};
    if (!m[part]) { ModelBuilder mb; CowPartGeometry(mb, part, 3); m[part] = mb.Build(true); }
    return m[part];
}

// Props that move with the procession (cart, bicycle)
struct BelowState {
    bool active = false;
    float cartS = 0, bikeA = 0, t = 0;
    bool zainFollow = false;
    std::vector<std::string> corpses;
    struct Rusher { std::string actor; std::vector<Vector3> path; size_t i = 0; float delay = 0; bool bumped = false; };
    std::vector<Rusher> rushers;
};
static BelowState B;

static Vector3 CartPos(float s, float& heading) {
    float a = s / 10.5f;   // ellipse: 12 x 9 m around (-8, 40)
    Vector3 p = CaveP(-8.0f + cosf(a) * 12.0f, 0, 40.0f + sinf(a) * 9.0f);
    Vector3 q = CaveP(-8.0f + cosf(a + 0.01f) * 12.0f, 0, 40.0f + sinf(a + 0.01f) * 9.0f);
    heading = atan2f(q.x - p.x, q.z - p.z);
    p.y = SurfaceY(p.x, p.z, p.y + 2.0f);
    return p;
}
static Vector3 BikePos(float a, float& heading) {
    Vector3 p = CaveP(12.0f + cosf(a) * 5.0f, 0, 58.0f + sinf(a) * 5.0f);
    Vector3 q = CaveP(12.0f + cosf(a + 0.01f) * 5.0f, 0, 58.0f + sinf(a + 0.01f) * 5.0f);
    heading = atan2f(q.x - p.x, q.z - p.z);
    p.y = SurfaceY(p.x, p.z, p.y + 2.0f);
    return p;
}

void Story::Impl::DrawCreatures() {
    for (auto& c : creatures) {
        if (c.kind != 1 || c.state == 4) continue;
        float w = SmoothStep(0.0f, 1.0f, c.wake);
        float jerk = c.wake < 1.0f ? sinf(c.t * 23.0f) * 0.1f * (1.0f - w) : 0.0f;
        float ph = c.gait * 2 * PI;
        float bob = fabsf(sinf(ph)) * 0.05f * w;
        float roll = Lerp(78.0f * DEG2RAD, 0.0f, w) + jerk;
        Matrix body = MatPose({ c.pos.x, c.pos.y + Lerp(0.3f, 1.27f, w) + bob, c.pos.z }, c.yaw, -0.06f * w + jerk * 0.5f, roll);
        Rdr().Draw(CowModel(COW_BODY), body);
        float headPitch = Lerp(0.8f, 0.15f + sinf(ph * 0.5f) * 0.12f, w);
        Matrix head = MatrixMultiply(MatrixMultiply(MatrixRotateX(headPitch), MatrixTranslate(0, 0.02f, 0.85f)), body);
        Rdr().Draw(CowModel(COW_HEAD), head);
        for (int leg = 0; leg < 4; leg++) {
            float lp = ph + ((leg == 0 || leg == 3) ? 0.0f : PI);
            float swing = sinf(lp) * 0.5f * w + (1.0f - w) * (leg < 2 ? -1.3f : 1.3f);
            float knee = fmaxf(0.0f, cosf(lp)) * 0.7f * w + (1.0f - w) * 1.5f;
            Matrix up = MatrixMultiply(MatrixMultiply(MatrixRotateX(swing), MatrixTranslate(CowHip(leg).x, CowHip(leg).y, CowHip(leg).z)), body);
            Matrix lo = MatrixMultiply(MatrixMultiply(MatrixRotateX(leg < 2 ? knee : -knee), MatrixTranslate(0, -0.52f, 0)), up);
            Rdr().Draw(CowModel(COW_UPPER), up);
            Rdr().Draw(CowModel(COW_LOWER), lo);
        }
    }
    if (B.active) {
        float h;
        Vector3 cp = CartPos(B.cartS, h);
        Rdr().Draw(GetPrefabModel("cart", {}), MatPose(cp, h));
        Vector3 bp = BikePos(B.bikeA, h);
        Rdr().Draw(GetPrefabModel("bicycle", {}), MatPose(bp, h, 0, -0.12f));
    }
}

// ---------------------------------------------------------------------------
// The world below: the procession, the rider, the watchers, the corpses, Zain
// ---------------------------------------------------------------------------
static void SpawnBelowCast(Story::Impl& im) {
    Game& g = im.g;
    B = BelowState();
    B.active = true;
    float cy = EntYaw("cavern");
    // the procession: two people in a yoke, a goat-headed driver on the cart
    g.SpawnActor("yoke_l", SpecCustomer(5), CaveP(0, 0, 0), 0);
    g.SpawnActor("yoke_r", SpecCustomer(6), CaveP(0, 0, 0), 0);
    g.SpawnActor("driver", SpecGoatMan(3), CaveP(0, 0, 0), 0);
    g.SpawnActor("rider", SpecPigMan(4), CaveP(0, 0, 0), 0);
    // the watchers: faceless, facing the rock, until you pass
    const Vector3 wp[] = { { -9, 0, 20 }, { 9.5f, 0, 26 }, { -7, 0, 50 }, { 8, 0, 71 } };
    for (int i = 0; i < 4; i++) {
        Actor* a = g.SpawnActor(TextFormat("watcher_%d", i), SpecCustomer(40 + i), CaveP(wp[i].x, wp[i].y, wp[i].z), 0);
        a->yaw = cy + (wp[i].x < 0 ? -PI * 0.5f : PI * 0.5f);
        a->pos.y = SurfaceY(a->pos.x, a->pos.z, a->pos.y + 2.0f);
        a->SetPose(PoseStand(i * 1.3f, 0.3f), true);
    }
    // the dead along the road. Don't touch them.
    const Vector3 cpz[] = { { -3.2f, 0, 14 }, { 3.5f, 0, 22 }, { -3.0f, 0, 31 }, { 3.4f, 0, 40 }, { -3.3f, 0, 55 }, { 3.1f, 0, 63 }, { -3.6f, 0, 72 } };
    for (int i = 0; i < 7; i++) {
        std::string n = TextFormat("corpse_%d", i);
        Actor* a = g.SpawnActor(n, i % 2 ? SpecCustomer(7 + i % 3) : SpecCrawler(1 + i % 3), CaveP(cpz[i].x, 0, cpz[i].z), 0);
        a->yaw = cy + (i * 1.7f);
        a->pos.y = SurfaceY(a->pos.x, a->pos.z, a->pos.y + 2.0f) + 0.12f;
        a->pitch = -1.5f;
        a->tint = Color{ 170, 165, 160, 255 };
        a->SetPose(PoseLyingBack(0), true);
        g.actors[n].action = "corpse";
        g.actors[n].radius = 2.3f;
        B.corpses.push_back(n);
    }
    // Zain, tied to the chair under the roots
    Vector3 chair = CaveP(0, 0, 84);
    Actor* z = g.SpawnActor("zain", SpecZain(), chair, 0);
    z->yaw = cy + PI;
    z->pos.y = chair.y + 0.46f + 0.08f - z->model->hipHeight;   // the cavern floor is at the chair's origin
    z->SetPose(PoseTiedChair(0, 0), true);
}

static void WakeCorpse(Story::Impl& im, const std::string& n) {
    Game& g = im.g;
    Actor* a = g.A(n);
    if (!a || im.FindCreature(n)) return;
    Vector3 p = a->pos; p.y -= 0.12f;
    im.AddCreature(n, 2, p);
    if (Creature* c = im.FindCreature(n)) { c->yaw = a->yaw; c->hitCd = 1.5f; }
    audio::Play3D("gasp", p, 0.9f, 0.6f);
    audio::Play3D("bone_crack", p, 1.0f);
}

void UpdateBelowWorld(Story::Impl& im, float dt) {
    Game& g = im.g;
    // rushers: things coming up out of the tunnel, running past you and out into the night
    for (auto& r : B.rushers) {
        Actor* a = g.A(r.actor);
        if (!a || r.i >= r.path.size()) continue;
        if (r.delay > 0) { r.delay -= dt; a->visible = false; continue; }
        a->visible = true;
        Vector3 to = Vector3Subtract(r.path[r.i], a->pos);
        float len = Vector3Length(to);
        float step = 6.0f * dt;
        if (len <= step) { a->pos = r.path[r.i]; r.i++; if (r.i >= r.path.size()) { a->visible = false; } continue; }
        a->pos = Vector3Add(a->pos, Vector3Scale(to, step / len));
        a->yaw = atan2f(to.x, to.z);
        a->SetPose(PoseCrawl(g.gameTime * 2.6f + r.delay));
        if (fmodf(g.gameTime, 0.18f) < dt) audio::PlayFootstep(SURF_FLESH, a->pos, 0.6f, true);
        if (!r.bumped && Vector3Distance(a->pos, g.player.feet) < 1.0f) {
            r.bumped = true;
            g.ShakeCamera(0.45f);
            g.player.fear = 1.0f;
            audio::Play3D("body_thud", a->pos, 0.8f);
        }
    }
    if (!B.active || !g.inUnderground) return;
    B.t += dt;
    // the cart procession
    float h;
    B.cartS += dt * 0.9f;
    Vector3 cp = CartPos(B.cartS, h);
    Matrix cx = MatPose(cp, h);
    for (int s = 0; s < 2; s++) {
        if (Actor* y = g.A(s ? "yoke_r" : "yoke_l")) {
            y->pos = Vector3Transform({ s ? 0.45f : -0.45f, 0, 3.5f }, cx);
            y->pos.y = SurfaceY(y->pos.x, y->pos.z, y->pos.y + 2.0f);
            y->yaw = h;
            y->SetPose(PoseHarnessed(B.t * 0.9f + s * 0.5f));
        }
    }
    if (Actor* d = g.A("driver")) {
        d->pos = Vector3Transform({ 0, 0.95f + 0.08f - d->model->hipHeight, 0.3f }, cx);
        d->yaw = h;
        d->SetPose(PoseRide(B.t));
        d->lookAt = g.player.EyePos(); d->lookWeight = Vector3Distance(d->pos, g.player.feet) < 12.0f ? 0.8f : 0.0f;
    }
    if (fmodf(B.t, 2.2f) < dt) audio::Play3D("drag_short", cp, 0.5f, 0.6f, 3.0f, 30.0f);
    if (fmodf(B.t + 1.1f, 4.4f) < dt) audio::Play3D("twig", cp, 0.7f, 0.5f, 3.0f, 30.0f);   // the switch coming down
    // the rider circling his column
    B.bikeA += dt * 0.32f;
    Vector3 bp = BikePos(B.bikeA, h);
    if (Actor* r = g.A("rider")) {
        r->pos = Vector3Transform({ 0, 0.9f + 0.08f - r->model->hipHeight, -0.2f }, MatPose(bp, h));
        r->yaw = h;
        r->SetPose(PoseRide(B.t * 0.3f));
        float dist = Vector3Distance(bp, g.player.feet);
        r->lookAt = g.player.EyePos(); r->lookWeight = dist < 14.0f ? 1.0f : 0.0f;
        if (dist < 12.0f && fmodf(B.t, 7.0f) < dt) audio::Play3D("counter_bell", bp, 0.6f, 1.3f);
    }
    // watchers turn their heads as you pass
    for (int i = 0; i < 4; i++)
        if (Actor* w = g.A(TextFormat("watcher_%d", i))) {
            float dist = Vector3Distance(w->pos, g.player.feet);
            w->lookAt = g.player.EyePos();
            w->lookWeight = Damp(w->lookWeight, dist < 8.0f ? 1.0f : 0.0f, 1.5f, dt);
            w->SetPose(PoseStand(B.t + i, 0.3f));
        }
    // brushing past the dead wakes them
    for (auto& n : B.corpses) {
        Actor* a = g.A(n);
        if (a && !im.FindCreature(n) && Vector3Distance(Flat(a->pos), Flat(g.player.feet)) < 0.9f) WakeCorpse(im, n);
    }
    // Zain follows you out
    if (B.zainFollow)
        if (Actor* z = g.A("zain")) {
            Vector3 target = Vector3Subtract(g.player.feet, Vector3Scale(Flat(g.player.Forward()), 1.6f));
            Vector3 to = Flat(Vector3Subtract(target, z->pos));
            float len = Vector3Length(to);
            float sp = len > 5.0f ? 3.9f : len > 1.5f ? 2.6f : 0.0f;
            if (sp > 0) {
                z->pos = Vector3Add(z->pos, Vector3Scale(to, fminf(sp * dt, len) / len));
                z->yaw = DampAngle(z->yaw, atan2f(to.x, to.z), 8.0f, dt);
                z->SetPose(sp > 3.0f ? PoseRun(B.t * 1.4f) : PoseWalk(B.t * 1.0f, 1.0f, 0));
                if (fmodf(B.t, sp > 3.0f ? 0.3f : 0.48f) < dt) audio::PlayFootstep(SURF_MUD, z->pos, 0.4f, false);
            } else z->SetPose(PoseStand(B.t));
            z->pos.y = SurfaceY(z->pos.x, z->pos.z, z->pos.y + 1.0f);
            z->pitch = 0;
        }
}

// Down the stairs and into the world below (and back)
static void EnterCavern(Story::Impl& im) {
    Game& g = im.g;
    g.inUnderground = true;
    Vector3 p = CaveP(0, 0.1f, 2.5f);
    g.player.Spawn(p, EntYaw("cavern") * RAD2DEG);
    g.hud.FadeInstant(1.0f);
    g.hud.FadeTo(0.0f, 0.3f);
    SpawnBelowCast(im);
    im.checkpoint = p; im.checkpointYaw = EntYaw("cavern") * RAD2DEG;
    im.onCaught = [&im]() {
        for (auto& c : im.creatures) { c.pos = c.home; c.state = c.kind == 3 ? 2 : 0; c.t = 0; }
    };
    audio::Play("breath_out", 0.5f, 0.8f);
}

// ---------------------------------------------------------------------------
// Chapter: Below
// ---------------------------------------------------------------------------
void Story::Impl::BuildBelow(Script& s) {
    Game& gg = g;
    s.Do([this, &gg]() {
        gg.story->cutscene = false;
        creatures.clear();
        B = BelowState();
        gg.hud.FadeInstant(1.0f);
        gg.hud.FadeTo(0.0f, 0.4f);
        gg.hud.Title("BELOW", "what the ground keeps", 5.0f);
        gg.player.Spawn(CollegeP(1.6f, -2.95f, -7.5f), EntYaw("college") * RAD2DEG + 90.0f);
        gg.player.flashOn = true;
        if (!gg.Item("matches")) gg.GiveItem("matches");
        if (!gg.Item("iron_key")) gg.GiveItem("iron_key");
        checkpoint = CollegeP(3.0f, -2.95f, -7.5f); checkpointYaw = EntYaw("college") * RAD2DEG + 90.0f;
    });
    s.Wait(3.0f);
    s.Do([&gg]() {
        audio::Play("thunder", 0.7f, 0.4f);
        gg.ShakeCamera(0.35f);
        gg.hud.Say("", "(something is coming up the stairs. a lot of somethings.)", 3.0f);
        gg.player.fear = 0.9f;
    });
    s.Wait(1.5f);
    s.Do([this, &gg]() {
        // they pour out of the tunnel, past you, up through the college and out into the night
        std::vector<Vector3> path = { StairP(0, -4.9f, 10.5f), StairP(0, -2.5f, 6.0f), StairP(0, 0.0f, 1.5f),
                                      CollegeP(2.2f, -3.0f, -7.6f), CollegeP(5.6f, -3.0f, -7.8f), CollegeP(9.2f, -3.0f, -4.6f),
                                      CollegeP(12.0f, -3.0f, -4.5f), CollegeP(14.2f, -3.0f, -8.4f), CollegeP(20.8f, 0.3f, -8.4f),
                                      CollegeP(19.0f, 0.3f, -3.0f), CollegeP(14.0f, 0.3f, 0.0f), CollegeP(0.0f, 0.3f, 0.0f), CollegeP(0.0f, 0.3f, 16.0f) };
        for (int i = 0; i < 5; i++) {
            std::string n = TextFormat("rusher_%d", i);
            Actor* a = gg.SpawnActor(n, SpecCrawler((uint32_t)(1 + i % 3)), path[0], 0);
            a->visible = false;
            BelowState::Rusher r; r.actor = n; r.path = path; r.delay = i * 0.55f;
            r.path[3].z += (i % 2 ? 0.35f : -0.35f);
            B.rushers.push_back(r);
        }
        audio::Play("screech_creature", 0.9f, 0.8f);
    });
    s.Wait(7.0f);
    // meanwhile, up top: they drink from the pumps and pick at the cows
    s.Do([this, &gg]() {
        for (auto& r : B.rushers) gg.RemoveActor(r.actor);
        B.rushers.clear();
        for (int i = 0; i < 3; i++) {
            Entity* p = Scn().Find(TextFormat("pump_%d", i + 1));
            Vector3 at = p ? p->LocalToWorld({ 0, 0, 0.9f }) : Vector3{ 16, 0, 0 };
            AddCreature(TextFormat("drinker_%d", i), 5, at);
            if (Creature* c = FindCreature(TextFormat("drinker_%d", i))) { c->yaw = p ? p->worldYaw + PI : 0; if (Actor* a = gg.A(c->actor)) { a->pos = at; a->yaw = c->yaw; } }
        }
        if (Entity* cow = Scn().Find("cow_1")) {
            Vector3 at = cow->LocalToWorld({ 0.9f, 0, 0.3f });
            AddCreature("eater", 5, at);
            if (Creature* c = FindCreature("eater")) { c->yaw = cow->worldYaw - PI * 0.5f; if (Actor* a = gg.A("eater")) { a->pos = at; a->yaw = c->yaw; } }
        }
        gg.story->cutscene = true;
        gg.hud.Letterbox(true);
        Entity* canopy = Scn().Find("canopy");
        Vector3 c = canopy ? canopy->base : Vector3{ 16, 0, 0 };
        Vector3 cowp = Scn().Find("cow_1") ? Scn().Find("cow_1")->base : Vector3{ 48, 0, -107 };
        SetCamPath({ { 0.0f, Vector3Add(c, { -9.0f, 1.3f, 7.0f }), Vector3Add(c, { 0, 0.6f, 0 }), 50 },
                     { 4.5f, Vector3Add(c, { -7.5f, 1.2f, 5.0f }), Vector3Add(c, { 0, 0.5f, 0 }), 44 },
                     { 4.51f, Vector3Add(cowp, { -4.0f, 1.0f, 3.0f }), Vector3Add(cowp, { 0.5f, 0.3f, 0 }), 45 },
                     { 8.5f, Vector3Add(cowp, { -3.0f, 0.8f, 2.2f }), Vector3Add(cowp, { 0.6f, 0.2f, 0 }), 38 } });
        gg.player.lookLocked = true;
    });
    s.Wait(8.5f);
    s.Do([this, &gg]() {
        EndCamPath();
        gg.camOverride = false;
        gg.story->cutscene = false;
        gg.player.lookLocked = false;
        gg.hud.Letterbox(false);
        gg.hud.Say("ADAM", "They're up there now. At the pumps.", 2.5f);
    });
    s.Wait(2.0f).Objective("Go down.");
    s.Until([&gg]() {
        Vector3 bottom = StairP(0, -4.94f, 11.5f);
        return Vector3Distance(gg.player.feet, bottom) < 1.8f;
    });
    s.Do([&gg]() { gg.hud.FadeTo(1.0f, 1.2f); gg.player.locked = true; audio::Play("breath_out", 0.4f, 0.6f); });
    s.Wait(1.0f).Say("", "(the stairs go on for a long time.)", 2.6f, 0.4f);
    s.Do([this, &gg]() { gg.player.locked = false; EnterCavern(*this); });
    s.Wait(2.0f).Objective("Find Zain.");
    s.Wait(5.0f).Do([&gg]() { gg.hud.Hint("The dead lie along the road. Don't touch them.", 4.0f); });
    s.Until([&gg]() {
        Vector3 chair = CaveP(0, 0, 84);
        return Vector3Distance(Flat(gg.player.feet), Flat(chair)) < 2.6f && !gg.Flag("caught");
    });
    // ---- the truth
    s.Do([this, &gg]() {
        gg.player.locked = true;
        gg.hud.Letterbox(true);
        gg.hud.Objective("");
        for (auto& c : creatures) c.hitCd = 99.0f;
        if (Actor* z = gg.A("zain")) { z->lookAt = gg.player.EyePos(); z->lookWeight = 1.0f; z->SetPose(PoseTiedChair(0, 0.6f)); }
    });
    s.Say("ZAIN", "Adam? You're... how are you here?")
     .Say("ADAM", "Hold still. I've got you.")
     .Do([&gg]() { audio::Play("drag_short", 0.4f, 1.6f); })
     .Wait(1.2f)
     .Say("ADAM", "...These knots are in front of you. You could have undone them yourself.", 3.6f, 1.6f)
     .Say("ZAIN", "Mum's down here. He showed me. She's in the roots, Adam. She's warm.")
     .Say("ZAIN", "He said the ground keeps everything that's given to it. And it gives things back, if you pay.")
     .Say("ADAM", "Pay with what?")
     .Say("ZAIN", "A life. Of the same blood.", 3.0f, 0.6f);
    // the crash again, remembered in text
    s.Do([&gg]() {
        Rdr().s.asciiFull = 1.0f;
        audio::Play("screech", 0.8f);
        gg.ShakeCamera(0.2f);
    });
    s.Wait(1.3f).Do([&gg]() { audio::Play("crash", 0.9f); Rdr().s.white = 1.0f; gg.ShakeCamera(0.8f); });
    s.Wait(0.4f).Do([]() { Rdr().s.white = 0.0f; });
    s.Say("ZAIN", "(in the car) Relax. I'm kidding.", 2.4f, 0.8f);
    s.Do([]() { Rdr().s.asciiFull = 0.0f; });
    s.Say("ADAM", "The wheel. You grabbed the wheel.")
     .Say("ZAIN", "He promised it wouldn't hurt. He said you wouldn't even feel it.")
     .Say("ZAIN", "I'm sorry. I'm so sorry. I just wanted her back.", 3.4f);
    s.Do([this]() { Choose({ "\"Get up. We're leaving.\"", "(say nothing. untie the last knot.)" }); });
    s.Until([this]() { return choiceResult >= 0; });
    SayLines(s, [this]() -> std::vector<Line> {
        if (choiceResult == 0) return { { "ZAIN", "You're not... you're not angry?" }, { "ADAM", "Later. Get up." } };
        return { { "ZAIN", "Adam. Say something. Please." } };
    });
    s.Do([this, &gg]() {
        // the roots move. He's here.
        audio::Play("growl", 1.0f, 0.4f);
        audio::Play("stinger_low", 0.7f);
        gg.ShakeCamera(0.5f);
        Scn().SetLightGroupFlicker("below_torch", 0.9f);
        Vector3 at = CaveP(0, 0, 92.0f);
        AddCreature("dragger", 3, at);
        if (Creature* c = FindCreature("dragger")) { c->yaw = EntYaw("cavern") + PI; c->hitCd = 3.0f; }
        if (Actor* z = gg.A("zain")) { z->SetPose(PoseStand(0)); z->pos.y = SurfaceY(z->pos.x, z->pos.z, z->pos.y + 2.0f); z->lookWeight = 0; }
    });
    s.Say("ZAIN", "He's here.", 1.4f, 0.2f);
    s.Say("ADAM", "RUN.", 1.0f, 0.1f);
    s.Do([this, &gg]() {
        gg.player.locked = false;
        gg.hud.Letterbox(false);
        B.zainFollow = true;
        for (auto& c : creatures) c.hitCd = 1.0f;
        for (size_t i = 0; i < B.corpses.size(); i++) {
            std::string n = B.corpses[i];
            gg.story->side.Wait(0.4f + i * 0.35f).Do([this, n]() { WakeCorpse(*this, n); });
        }
        checkpoint = CaveP(0, 0.1f, 80.0f); checkpointYaw = EntYaw("cavern") * RAD2DEG + 180.0f;
        onCaught = [this]() {
            for (auto& c : creatures) { c.pos = c.home; c.t = 0; c.state = c.kind == 3 ? 2 : 0; }
            if (Actor* z = g.A("zain")) z->pos = CaveP(0.8f, 0, 79.0f);
        };
        gg.hud.Objective("Run. Back to the stairs.");
    });
    s.Until([&gg]() { return Vector3Distance(Flat(gg.player.feet), Flat(CaveP(0, 0, 1.5f))) < 3.5f && !gg.Flag("caught"); });
    s.Do([this, &gg]() {
        gg.player.locked = true;
        gg.hud.FadeTo(1.0f, 0.9f);
        audio::Play("breath_out", 0.7f, 1.2f);
        Scn().SetLightGroupFlicker("below_torch", 0.3f);
    });
    s.Wait(1.5f).Say("ZAIN", "(behind you, running, crying)", 2.0f, 0.4f);
    s.Do([this, &gg]() {
        B = BelowState();
        creatures.clear();
        gg.inUnderground = false;
        gg.player.locked = false;
        gg.story->pendingChapter = CH_BURN;
    });
}

void RegisterBelowActions(Story::Impl& im) {
    Story::Impl* pim = &im;
    im.g.actions["corpse"] = { [](Entity&) -> std::string { return "Touch"; },
                               [pim](Entity& e) { WakeCorpse(*pim, e.name); } };
}
