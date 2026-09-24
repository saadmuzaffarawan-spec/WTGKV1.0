#include "story_impl.h"
#include "menus.h"
#include <rlgl.h>

using namespace ui;

// ---------------------------------------------------------------------------
// Script
// ---------------------------------------------------------------------------
void Script::Update(float dt) {
    while (i < steps.size()) {
        if (!started) { if (steps[i].enter) steps[i].enter(); started = true; t = 0; }
        t += dt;
        if (steps[i].update && !steps[i].update(t, dt)) return;
        i++; started = false; dt = 0;
    }
}
Script& Script::Do(std::function<void()> fn) { steps.push_back({ fn, nullptr }); return *this; }
Script& Script::Wait(float s) { steps.push_back({ nullptr, [s](float t, float) { return t >= s; } }); return *this; }
Script& Script::Until(std::function<bool()> p) { steps.push_back({ nullptr, [p](float, float) { return p(); } }); return *this; }
Script& Script::Run(std::function<bool(float, float)> fn, std::function<void()> enter) { steps.push_back({ enter, fn }); return *this; }
Script& Script::Say(const std::string& who, const std::string& text, float dur, float gap) {
    float d = dur < 0 ? 1.6f + text.size() * 0.055f : dur;
    steps.push_back({ [who, text, d]() { G().hud.Say(who, text, d); }, [d, gap](float t, float) { return t >= d + gap; } });
    return *this;
}
Script& Script::Objective(const std::string& text) { return Do([text]() { G().hud.Objective(text); }); }

Camera3D SampleCamPath(const std::vector<CamKey>& k, float t) {
    Camera3D c{};
    c.up = { 0, 1, 0 };
    c.projection = CAMERA_PERSPECTIVE;
    if (k.empty()) return c;
    if (t <= k.front().t) { c.position = k.front().pos; c.target = k.front().target; c.fovy = k.front().fov; return c; }
    if (t >= k.back().t) { c.position = k.back().pos; c.target = k.back().target; c.fovy = k.back().fov; return c; }
    size_t i = 0;
    while (i + 1 < k.size() && k[i + 1].t < t) i++;
    float u = (t - k[i].t) / fmaxf(k[i + 1].t - k[i].t, 1e-4f);
    const CamKey& p0 = k[i > 0 ? i - 1 : i];
    const CamKey& p1 = k[i];
    const CamKey& p2 = k[i + 1];
    const CamKey& p3 = k[i + 2 < k.size() ? i + 2 : i + 1];
    c.position = CatmullRom(p0.pos, p1.pos, p2.pos, p3.pos, u);
    c.target = CatmullRom(p0.target, p1.target, p2.target, p3.target, u);
    c.fovy = Lerp(p1.fov, p2.fov, u);
    return c;
}

const char* ChapterTitle(int ch) {
    switch (ch) {
    case CH_PROLOGUE: return "prologue";
    case CH_AWAKENING: return "awakening";
    case CH_SHIFT1: return "night one";
    case CH_SHIFT2: return "night two";
    case CH_SHIFT3: return "night three";
    case CH_KEY: return "what the cows keep";
    case CH_BELOW: return "below";
    case CH_BURN: return "burn";
    default: return "";
    }
}

// ---------------------------------------------------------------------------
// Car rig
// ---------------------------------------------------------------------------
void CarRig::Update(float dt) {
    spin += speed * dt / 0.31f;
}

void CarRig::Draw() const {
    if (!visible) return;
    if (!body) return;
    Matrix xf = Xf();
    Rdr().Draw(body, xf);
    const SedanParts& p = GetSedanParts();
    for (int i = 0; i < 4; i++) {
        float sx = (i & 1) ? -kTrack : kTrack;
        float sz = (i & 2) ? -kWheelZ : kWheelZ;
        float st = (i & 2) ? 0.0f : steer * 0.5f;
        Matrix w = MatrixMultiply(MatrixMultiply(MatrixRotateX(spin), MatrixRotateY(st)), MatrixTranslate(sx, kWheelY, sz));
        Rdr().Draw((i & 1) ? p.wheelR : p.wheel, MatrixMultiply(w, xf));
    }
    if (interior) {
        Matrix sw = MatrixMultiply(MatrixMultiply(MatrixRotateZ(-steer * 1.6f), MatrixRotateX(-kSteerTilt * DEG2RAD)), MatrixTranslate(kDriverX, kSteerY, kSteerZ));
        Rdr().Draw(p.steering, MatrixMultiply(sw, xf));
        Matrix gm = MatrixMultiply(MatrixMultiply(MatrixRotateY(PI), MatrixRotateX(-0.25f)), MatrixTranslate(kDriverX, 1.0f, 0.5f));
        Rdr().Draw(p.gauges, MatrixMultiply(gm, xf), WHITE, false);
        float mph = speed * 2.237f;
        float a = (210.0f - mph / 120.0f * 240.0f) * DEG2RAD - PI * 0.5f;
        Matrix nm = MatrixMultiply(MatrixMultiply(MatrixRotateZ(-a), MatrixRotateY(PI)), MatrixMultiply(MatrixRotateX(-0.25f), MatrixTranslate(kDriverX + 0.085f, 1.0f, 0.498f)));
        Rdr().Draw(p.needle, MatrixMultiply(nm, xf), WHITE, false);
    }
}

void CarRig::Lights() const {
    if (!visible) return;
    if (headlights) {
        for (int s = -1; s <= 1; s += 2) {
            Light l;
            l.spot = true;
            l.pos = Local({ s * 0.6f, 0.66f, 2.45f });
            l.dir = Vector3Normalize(XfDir(Xf(), { 0, -0.09f, 1.0f }));
            l.color = { 1.0f, 0.9f, 0.72f };
            l.intensity = 380.0f * headFlicker;
            l.range = 95.0f;
            l.innerDeg = 12; l.outerDeg = 30;
            l.volumetric = 0.06f;
            l.shadow = s > 0;
            l.priority = 0.8f;
            Rdr().AddLight(l);
        }
    }
    for (int s = -1; s <= 1; s += 2) {
        Light t; t.pos = Local({ s * 0.62f, 0.7f, -2.7f }); t.color = { 1.0f, 0.05f, 0.03f }; t.intensity = 1.2f; t.range = 2.2f; t.volumetric = 0.15f;
        Rdr().AddLight(t);
    }
    // dash glow on the brothers' faces
    Light d; d.pos = Local({ 0.1f, 1.0f, 0.45f }); d.color = { 0.75f, 0.8f, 1.0f }; d.intensity = 1.1f; d.range = 2.2f;
    Rdr().AddLight(d);
}

// ---------------------------------------------------------------------------
// Story
// ---------------------------------------------------------------------------
Story::Story() : impl(new Impl(G())) {}
Story::~Story() { delete impl; }

void Story::SkipCutscene() {
    // skipping jumps the chapter script to its next non-cutscene step
    G().hud.ClearSubtitles();
    if (impl) impl->g.timeScale = 1.0f;
    script.Update(1000.0f);
}

void Story::Impl::SetupWorldForChapter(int ch) {
    Game& G_ = g;
    bool afterCrash = ch >= CH_AWAKENING;
    G_.SetTagVisible("after_crash", afterCrash);
    G_.SetTagVisible("below_open", ch >= CH_BELOW);
    G_.SetTagVisible("burnt", false);
    for (auto& e : Scn().ents) e->tint = WHITE;   // undo the epilogue's char
    decals.Clear();
    if (afterCrash) {
        // Skid marks from the yank to the tower, blood at the wreck, and the drag trail to the hatch.
        std::vector<Vector3> skidL, skidR;
        for (int i = 0; i <= 20; i++) {
            float u = i / 20.0f;
            Vector3 a = RoadPoint(118.0f + u * 30.0f, 1.8f + u * u * 7.0f);
            skidL.push_back(a);
            skidR.push_back(Vector3Add(a, { 1.5f - u * 0.8f, 0, u * 0.9f }));
        }
        decals.Skid(skidL, 0.22f);
        decals.Skid(skidR, 0.22f);
        decals.Pool({ 17.6f, 0, 165.2f }, 0.9f, MAT_BLOOD, 11, 1.4f, 0.8f);
        decals.Pool({ 16.4f, 0, 162.8f }, 0.5f, MAT_BLOOD, 12);
        std::vector<Vector3> drag;
        drag.push_back({ 16.2f, 0, 162.0f });
        for (float z = 158.0f; z > 8.0f; z -= 6.0f) {
            float wob = sinf(z * 0.07f) * 0.6f;
            drag.push_back(RoadPoint(z, 1.2f + wob));
        }
        drag.push_back({ 8.0f, 0, 2.0f });
        drag.push_back({ 16.0f, 0, -6.0f });
        drag.push_back({ 22.0f, 0, -10.0f });
        drag.push_back({ 26.4f, 0, -12.4f });
        decals.Trail(drag, 0.5f, MAT_BLOOD_SMEAR, 99, 0.5f);
        decals.Pool({ 26.4f, 0, -12.45f }, 0.7f, MAT_BLOOD, 13);
    }
    // world mood per chapter
    Weather& w = g.weather;
    w.rain = (ch == CH_SHIFT2) ? 0.7f : 0.0f;
    w.storm = (ch == CH_SHIFT2 || ch == CH_BURN) ? 0.6f : 0.0f;
    w.fog = ch == CH_END ? 0.7f : ch >= CH_SHIFT3 ? 1.35f : (ch >= CH_AWAKENING ? 1.15f : 0.9f);
    w.wind = ch == CH_BURN ? 0.9f : 0.5f;
    float spiritFor[] = { 0.0f, 0.22f, 0.26f, 0.32f, 0.4f, 0.48f, 0.58f, 0.66f, 0.3f };
    g.spirit = spiritFor[ch < CH_END ? ch : CH_END];
    g.player.weight = Clamp(0.2f + (ch - CH_AWAKENING) * 0.13f, 0.15f, 1.0f);
    Scn().SetLightGroup("store", true);
    Scn().SetLightGroup("canopy", true);
    g.flags.erase("power_out");
}

void Story::Impl::SpawnCast(int ch) {
    g.actors.clear();
    Entity* store = Scn().Find("store");
    if (store && ch >= CH_AWAKENING && ch < CH_BELOW) {
        Vector3 p = Vector3Transform({ 4.5f, 0.15f, 2.05f }, MatPose(store->base, store->worldYaw));
        Actor* gr = g.SpawnActor("grethnar", SpecGrethnar(), p, store->worldYaw * RAD2DEG);
        gr->SetPose(PoseCounterLean(0), true);
        g.actors["grethnar"].action = "talk_grethnar";
        g.actors["grethnar"].radius = 4.0f;
    }
}

void Story::Begin(int ch) {
    // every chapter starts from the same night sky (the epilogue's dawn must not leak into a replay)
    {
        static bool captured = false;
        static RenderSettings night;
        RenderSettings& r = Rdr().s;
        if (!captured) { night = r; captured = true; }
        r.moonDir = night.moonDir; r.moonColor = night.moonColor; r.moonBright = night.moonBright;
        r.skyAmbient = night.skyAmbient; r.groundAmbient = night.groundAmbient; r.fogColor = night.fogColor;
        r.skyZenith = night.skyZenith; r.skyHorizon = night.skyHorizon; r.glowColor = night.glowColor; r.glowDir = night.glowDir;
        r.stars = night.stars; r.exposure = night.exposure; r.asciiFull = 0.0f; r.white = 0.0f;
    }
    chapter = ch;
    G().chapter = ch;
    script.Clear();
    side.Clear();
    impl->SetupWorldForChapter(ch);
    impl->SpawnCast(ch);
    impl->RegisterActions();
    Game& g = G();
    g.camOverride = false;
    g.hud.Letterbox(false);
    g.hud.Objective("");
    g.timeScale = 1.0f;
    g.inUnderground = false;
    g.player.locked = false; g.player.lookLocked = false;
    switch (ch) {
    case CH_PROLOGUE: impl->BuildPrologue(script); break;
    case CH_AWAKENING: impl->BuildAwakening(script); break;
    case CH_SHIFT1: case CH_SHIFT2: case CH_SHIFT3: impl->BuildShift(script, ch - CH_SHIFT1 + 1); break;
    case CH_KEY: impl->BuildKey(script); break;
    case CH_BELOW: impl->BuildBelow(script); break;
    case CH_BURN: impl->BuildBurn(script); break;
    case CH_END: BuildEpilogue(*impl, script); break;
    default: break;
    }
    if (ch != CH_PROLOGUE) g.SaveGame();
}

void Story::Update(float dt) {
    float prevT = impl->t;
    impl->t += dt;
    if (prevT < 1.5f && impl->t >= 1.5f)
        if (const char* th = getenv("WTGK_TEST")) StoryTestHook(*impl, th);   // headless test hook
    script.Update(dt);
    side.Update(dt);
    if (pendingChapter >= 0) { int ch = pendingChapter; pendingChapter = -1; Begin(ch); return; }
    impl->UpdateChoice();
    impl->UpdateCommon(dt);
    Game& g = G();
    if (impl->camPathActive) {
        impl->camT += dt;
        g.camOverride = true;
        g.camOverrideCam = SampleCamPath(impl->camPath, impl->camT);
    }
    impl->fx.Update(dt, g.camera.position, g.weather.rain, g.inUnderground);
}

void Story::Draw3D() {
    impl->DrawCreatures();
    DrawFireTrail(*impl);
    if (impl->wreckLights) {
        if (Entity* w = Scn().Find("wreck")) {
            float t = impl->t;
            // one headlight survived, knocked sideways across the road
            Light h; h.spot = true;
            Vector3 lamp = w->LocalToWorld({ -0.62f, 0.7f, 1.35f });
            Vector3 aim = RoadPoint(147.0f, -1.5f);
            aim.y = lamp.y - 0.35f;   // roughly level: the beam rakes across the road
            h.dir = Vector3Normalize(Vector3Subtract(aim, lamp));
            h.pos = Vector3Add(lamp, Vector3Scale(h.dir, 0.9f));   // just outside the crumpled bodywork
            float fl = (fmodf(t * 3.1f, 7.0f) < 0.12f) ? 0.2f : 1.0f;
            h.color = { 1.0f, 0.9f, 0.72f }; h.intensity = 260.0f * fl; h.range = 70.0f;
            h.innerDeg = 10; h.outerDeg = 26; h.volumetric = 0.25f; h.shadow = true; h.priority = 0.9f;
            Rdr().AddLight(h);
            // hazard lights and the dome light
            bool on = fmodf(t, 1.1f) < 0.55f;
            if (on) for (Vector3 c : { Vector3{ 0.8f, 0.65f, 1.4f }, Vector3{ -0.8f, 0.65f, 1.6f }, Vector3{ 0.8f, 0.8f, -2.3f }, Vector3{ -0.8f, 0.8f, -2.3f } }) {
                Light a; a.pos = w->LocalToWorld(c); a.color = { 1.0f, 0.55f, 0.1f }; a.intensity = 2.5f; a.range = 6.0f; a.volumetric = 0.2f;
                Rdr().AddLight(a);
            }
            Light d; d.pos = w->LocalToWorld({ 0.05f, 1.36f, -0.45f }); d.color = { 1.0f, 0.8f, 0.58f }; d.intensity = 0.55f; d.range = 2.6f;
            Rdr().AddLight(d);
        }
    }
    impl->car.Lights();
    impl->car.Draw();
    for (auto& c : impl->customers) { c.car.Lights(); c.car.Draw(); }
    impl->decals.Draw();
    if (impl->stainModel) {
        for (size_t i = 0; i < impl->stains.size(); i++) {
            if (impl->stainAlpha[i] <= 0.01f) continue;
            float s = 0.4f + 0.6f * impl->stainAlpha[i];
            Rdr().Draw(impl->stainModel, MatPose(impl->stains[i], (float)i * 1.7f, 0, 0, { s, 1, s }), WHITE, false);
        }
    }
    // fire lights
    for (size_t i = 0; i < impl->trail.size(); i += 6) {
        float f = impl->trailFire.size() > i ? impl->trailFire[i] : 0.0f;
        if (f > 0.05f && f < 1.9f) {
            Light l; l.pos = Vector3Add(impl->trail[i], { 0, 0.8f, 0 }); l.color = { 1.0f, 0.5f, 0.15f };
            l.intensity = 25.0f * (0.8f + 0.2f * sinf(impl->t * 17 + i)); l.range = 12.0f; l.volumetric = 0.3f;
            Rdr().AddLight(l);
        }
    }
}

void Story::DrawTransparent() { impl->fx.Draw(G().camera); }

void Story::DrawOverlay() {
    Game& g = G();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    // blood from a head wound creeping into the eye (dying POV): a soft red bleed from the brow,
    // darkening towards one side, with a few faint wet smears - no cartoon drips
    if (impl->blood > 0.01f) {
        float bl = impl->blood;
        unsigned char a0 = (unsigned char)(170 * bl), a1 = (unsigned char)(90 * bl);
        DrawRectangleGradientV(0, 0, (int)W, (int)(H * (0.25f + bl * 0.3f)), Color{ 40, 0, 0, a0 }, Color{ 40, 0, 0, 0 });
        DrawRectangleGradientH(0, 0, (int)(W * 0.35f), (int)H, Color{ 55, 2, 2, a1 }, Color{ 55, 2, 2, 0 });
        DrawCircleGradient((int)(W * 0.12f), (int)(H * 0.05f), H * 0.55f * bl, Color{ 90, 4, 4, (unsigned char)(150 * bl) }, Color{ 60, 0, 0, 0 });
        Rng r(4242);
        for (int i = 0; i < 5; i++) {
            float x = r.Range(0.03f, 0.35f) * W, w = r.Range(2.0f, 5.0f) * UiScale();
            float len = bl * r.Range(0.25f, 0.7f) * H;
            DrawRectangleGradientV((int)x, 0, (int)w, (int)len, Color{ 80, 5, 5, (unsigned char)(120 * bl) }, Color{ 80, 5, 5, 0 });
        }
    }
    impl->DrawChoice();
    // the Polaroid Grethnar hands over after night two
    if (impl->showPolaroid > 0.0f) {
        static RenderTexture2D photo{};
        if (photo.id == 0) {
            const int PW = 420, PH = 500;
            photo = LoadRenderTexture(PW, PH);
            // the picture itself is drawn small and scaled up: soft, like a cheap lens
            const float k = 0.3f;
            RenderTexture2D pic = LoadRenderTexture((int)((PW - 52) * k), (int)((PW - 52) * k));
            SetTextureFilter(pic.texture, TEXTURE_FILTER_BILINEAR);
            BeginTextureMode(pic);
            ClearBackground(BLACK);
            auto R = [k](float x, float y, float w, float h, Color c) { DrawRectangle((int)(x * k), (int)(y * k), (int)(w * k), (int)(h * k), c); };
            auto E = [k](float x, float y, float rx, float ry, Color c) { DrawEllipse((int)(x * k), (int)(y * k), rx * k, ry * k, c); };
            DrawRectangleGradientV(0, 0, pic.texture.width, pic.texture.height, Color{ 92, 78, 62, 255 }, Color{ 58, 48, 40, 255 });
            R(30, 40, 200, 150, Color{ 150, 132, 96, 255 });   // the store front: a lit window behind them
            R(36, 46, 188, 138, Color{ 176, 158, 118, 255 });
            E(190, 92, 13, 17, Color{ 205, 190, 160, 255 });   // the man in the window, watching them
            R(176, 108, 28, 60, Color{ 70, 60, 50, 255 });
            E(110, 150, 22, 27, Color{ 150, 118, 96, 255 });   // mother
            E(110, 132, 25, 20, Color{ 40, 30, 26, 255 });
            R(80, 176, 62, 190, Color{ 96, 70, 64, 255 });
            E(190, 214, 19, 23, Color{ 160, 126, 104, 255 });  // Zain, in the red hoodie
            E(190, 198, 21, 14, Color{ 30, 24, 20, 255 });
            R(166, 236, 50, 140, Color{ 140, 44, 40, 255 });
            EndTextureMode();
            BeginTextureMode(photo);
            ClearBackground(Color{ 232, 226, 212, 255 });
            Rectangle img{ 26, 26, PW - 52.0f, PW - 52.0f };
            DrawTexturePro(pic.texture, { 0, 0, (float)pic.texture.width, -(float)pic.texture.height }, img, { 0, 0 }, 0, WHITE);
            // age: grain, a light leak, a crease
            Rng r(9);
            for (int i = 0; i < 2600; i++) DrawPixel((int)(img.x + r.F() * img.width), (int)(img.y + r.F() * img.height), Color{ 255, 240, 210, (unsigned char)r.RangeI(10, 40) });
            DrawRectangleGradientH((int)img.x, (int)img.y, 90, (int)img.height, Color{ 255, 180, 120, 60 }, Color{ 255, 180, 120, 0 });
            DrawLineEx({ img.x, img.y + 250 }, { img.x + img.width, img.y + 230 }, 2, Color{ 230, 220, 200, 70 });
            EndTextureMode();
            UnloadRenderTexture(pic);
        }
        float a = Saturate(impl->showPolaroid * 1.5f) * Saturate((6.0f - impl->showPolaroid) * 3.0f);
        float sc = H * 0.62f / 500.0f;
        Rectangle src{ 0, 0, (float)photo.texture.width, -(float)photo.texture.height };
        Rectangle dst{ W * 0.5f, H * 0.47f, 420 * sc, 500 * sc };
        DrawRectangle(0, 0, (int)W, (int)H, Color{ 0, 0, 0, (unsigned char)(150 * a) });
        DrawTexturePro(photo.texture, src, dst, { dst.width * 0.5f, dst.height * 0.5f }, -4.0f, Color{ 255, 255, 255, (unsigned char)(255 * a) });
        TextCentered(F_MONO_LIGHT, "Z + mum.  sundays.", W * 0.5f + 6 * sc, H * 0.47f + 200 * sc, 20 * sc, Color{ 60, 56, 70, (unsigned char)(220 * a) });
    }
    (void)g;
}

void Story::Impl::UpdateChoice() {
    if (!choiceActive) return;
    int n = (int)choiceOpts.size();
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) { choiceSel = (choiceSel + n - 1) % n; audio::Play("ui_hover", 0.5f); }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) { choiceSel = (choiceSel + 1) % n; audio::Play("ui_hover", 0.5f); }
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight(), s = UiScale();
    Vector2 mp = GetMousePosition();
    for (int i = 0; i < n; i++) {
        Rectangle r{ W * 0.5f - 200 * s, H * 0.62f + i * 34 * s - 6 * s, 400 * s, 30 * s };
        if (CheckCollisionPointRec(mp, r)) { if (Vector2Length(GetMouseDelta()) > 0.5f) choiceSel = i; if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { choiceResult = i; } }
    }
    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) choiceResult = choiceSel;
    if (choiceResult >= 0) { choiceActive = false; audio::Play("ui_select", 0.7f); if (!g.shotFile) DisableCursor(); }
}

void Story::Impl::DrawChoice() {
    if (!choiceActive) return;
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight(), s = UiScale();
    for (int i = 0; i < (int)choiceOpts.size(); i++) {
        bool on = i == choiceSel;
        std::string t = (on ? "> " : "  ") + choiceOpts[i];
        TextCentered(F_MONO, t.c_str(), W * 0.5f, H * 0.62f + i * 34 * s, 18 * s, on ? kInk : kFaint);
    }
}

// ---------------------------------------------------------------------------
// Always-on story systems
// ---------------------------------------------------------------------------
void Story::Impl::UpdateCommon(float dt) {
    showPolaroid = fmaxf(0.0f, showPolaroid - dt);
    car.Update(dt);
    for (auto& c : customers) c.car.Update(dt);
    UpdateGrethnar(dt);
    UpdatePhone(dt);
    UpdateJob(dt);
    UpdateCustomers(dt);
    UpdateCreatures(dt);
    UpdateFire(dt);
    UpdateBoundary(dt);
    // keep the player's hands/eyes "heavy" as the ground claims him
    g.player.fear = Damp(g.player.fear, 0.0f, 0.3f, dt);
}

// Grethnar stares back if you stare at him (ported from v1): his head follows you,
// and if you keep looking the lights fail and he says your name.
void Story::Impl::UpdateGrethnar(float dt) {
    Actor* gr = g.A("grethnar");
    if (!gr) return;
    Vector3 eye = g.player.EyePos();
    Vector3 head = gr->HeadPos();
    Vector3 d = Vector3Subtract(head, eye);
    float dist = Vector3Length(d);
    float dotv = Vector3DotProduct(Vector3Scale(d, 1.0f / fmaxf(dist, 0.01f)), g.player.Forward());
    bool looking = dist < 14.0f && dotv > 0.985f && Phys().LineOfSight(eye, head);
    gr->lookAt = eye;
    // he always half-watches you; stare and he fully turns
    float want = dist < 10.0f ? 0.45f : 0.0f;
    if (looking) { stareTimer += dt; want = Saturate(stareTimer / 1.5f); }
    else stareTimer = fmaxf(0.0f, stareTimer - dt * 2.0f);
    gr->lookWeight = Damp(gr->lookWeight, want, 2.5f, dt);
    if (!g.story || g.story->InCutscene()) return;
    if (stareTimer > 6.0f && !g.Flag(TextFormat("stare_%d", shift))) {
        g.SetFlag(TextFormat("stare_%d", shift));
        Scn().SetLightGroupFlicker("store", 0.9f);
        g.hud.Say("GRETHNAR", "It's rude to stare, Adam.", 3.0f);
        audio::Play3D("stinger_low", head, 0.8f);
        g.player.fear = 1.0f;
        g.ShakeCamera(0.3f);
        g.story->side.Wait(3.0f).Do([]() { Scn().SetLightGroupFlicker("store", 0.0f); });
        stareTimer = 0;
    }
}

void Story::Impl::UpdatePhone(float dt) {
    if (phoneRingT < 0) return;
    phoneRingT += dt;
    Entity* ph = Scn().Find("payphone");
    if (!ph) return;
    if (fmodf(phoneRingT, 4.5f) < dt) audio::Play3D("phone_ring", ph->FocusPoint(), 0.8f, 1.0f, 3.0f, 60.0f);
}

// The road keeps you: walk too far and the fog turns you around.
void Story::Impl::UpdateBoundary(float dt) {
    (void)dt;
    if (g.camOverride || g.inUnderground) return;
    Vector3 p = g.player.feet;
    float lim = 230.0f;
    bool out = fabsf(p.z) > lim || fabsf(p.x) > 170.0f;
    if (!out) { leavingWarned = false; return; }
    if (!leavingWarned) { leavingWarned = true; audio::Play("whisper", 0.6f); g.hud.Hint("the fog is thicker here", 3.0f); }
    if (fabsf(p.z) > lim + 25.0f || fabsf(p.x) > 195.0f) {
        // step out of the fog walking back towards the station
        Vector3 back{ Clamp(p.x, -150.0f, 150.0f) * 0.9f, 0, Clamp(p.z, -lim + 10, lim - 10) * 0.9f };
        g.player.Spawn({ back.x, World().Height(back.x, back.z) + 0.5f, back.z }, atan2f(-back.x, -back.z) * RAD2DEG);
        g.hud.FadeInstant(0.8f); g.hud.FadeTo(0.0f, 0.8f);
        audio::Play("whisper", 0.8f);
        g.hud.Hint("the road always brings you back", 4.0f);
        g.spirit += 0.02f;
    }
}

void Story::Impl::Pay(float amount, const std::string& why) {
    g.money += amount;
    g.hud.Notify(TextFormat("+ $%.2f  %s", amount, why.c_str()));
    audio::Play("coins", 0.5f);
}
