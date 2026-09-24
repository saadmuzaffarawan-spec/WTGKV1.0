#include "game.h"
#include "story.h"
#include "menus.h"
#include "editor.h"
#include "vegetation.h"
#include "prefab_util.h"
#include <rlgl.h>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>

void RegisterAllPrefabs();

static Game g_game;
Game& G() { return g_game; }

// ---------------------------------------------------------------------------
// Settings
// ---------------------------------------------------------------------------
void Settings::Load() {
    std::ifstream in("settings.cfg");
    std::string line;
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = TrimStr(line.substr(0, eq)), v = TrimStr(line.substr(eq + 1));
        float f = (float)atof(v.c_str());
        if (k == "master") master = f; else if (k == "sensitivity") sensitivity = f; else if (k == "fov") fov = f;
        else if (k == "brightness") brightness = f; else if (k == "render_scale") renderScale = f; else if (k == "grass") grass = f;
        else if (k == "ascii") ascii = f; else if (k == "subtitle_size") subtitleSize = f;
        else if (k == "invert_y") invertY = f > 0.5f; else if (k == "subtitles") subtitles = f > 0.5f;
        else if (k == "move_speed") moveSpeed = f; else if (k == "volumetrics") volumetrics = f > 0.5f;
        else if (k == "shadows") shadows = f > 0.5f; else if (k == "quality") quality = (int)f;
        else if (k == "fullscreen") fullscreen = f > 0.5f; else if (k == "vsync") vsync = f > 0.5f; else if (k == "head_bob") headBob = f > 0.5f;
    }
}
void Settings::Save() const {
    std::ofstream o("settings.cfg");
    o << "master=" << master << "\nsensitivity=" << sensitivity << "\nfov=" << fov << "\nbrightness=" << brightness
      << "\nrender_scale=" << renderScale << "\ngrass=" << grass << "\nascii=" << ascii << "\nsubtitle_size=" << subtitleSize
      << "\ninvert_y=" << invertY << "\nsubtitles=" << subtitles << "\nfullscreen=" << fullscreen << "\nvsync=" << vsync
      << "\nhead_bob=" << headBob << "\nmove_speed=" << moveSpeed << "\nvolumetrics=" << volumetrics
      << "\nshadows=" << shadows << "\nquality=" << quality << "\n";
}

void Game::ApplySettings() {
    audio::SetMasterVolume(settings.master);
    Rdr().s.renderScale = Clamp(settings.renderScale, 0.5f, 1.0f);
    Veg().density = settings.grass;
    Rdr().s.volumetrics = settings.volumetrics;
    Rdr().s.shadowsEnabled = settings.shadows;
    player.walkSpeed = 1.55f * settings.moveSpeed;
    player.sprintSpeed = 3.7f * settings.moveSpeed;
    player.crouchSpeed = 0.85f * settings.moveSpeed;
    hud.subtitleScale = settings.subtitleSize;
    hud.subtitlesOn = settings.subtitles;
}

// ---------------------------------------------------------------------------
// The cast each chapter needs, so it can be sculpted in the background before it appears
// ---------------------------------------------------------------------------
static std::vector<BodySpec> CastFor(int ch) {
    std::vector<BodySpec> c;
    auto customers = [&]() { for (uint32_t s = 3; s <= 6; s++) c.push_back(SpecCustomer(s)); c.push_back(SpecCustomer(71)); };
    switch (ch) {
    case 0: c = { SpecAdam(), SpecZain(), SpecDragger() }; break;
    case 1: c = { SpecDragger(), SpecGrethnar() }; break;
    case 2: case 3: c = { SpecGrethnar() }; customers(); break;
    case 4: c = { SpecGrethnar(), SpecOldMan() }; break;
    case 5: c = { SpecGrethnar(), SpecOldMan() }; break;
    case 6:
        c = { SpecOldMan(), SpecCrawler(1), SpecCrawler(2), SpecCrawler(3), SpecZain(), SpecDragger(), SpecGoatMan(3), SpecPigMan(4) };
        for (uint32_t s : { 5u, 6u, 7u, 8u, 9u, 40u, 41u, 42u, 43u }) c.push_back(SpecCustomer(s));
        break;
    case 7: c = { SpecZain(), SpecAdam(), SpecCrawler(1), SpecCrawler(2), SpecCrawler(3) }; break;
    default: c = { SpecZain() }; break;
    }
    return c;
}
// Only this chapter's and the next chapter's cast are prepared, so memory holds what is about
// to be used rather than the whole game's cast.
static int g_castPrepared = -1;
void PrepareCastFrom(int chapter) {
    if (chapter == g_castPrepared) return;
    g_castPrepared = chapter;
    PrepareCharacters(CastFor(chapter), true);            // needed now
    if (chapter < 8) PrepareCharacters(CastFor(chapter + 1));   // needed next
}

// ---------------------------------------------------------------------------
// Actors, items, tags
// ---------------------------------------------------------------------------
Actor* Game::SpawnActor(const std::string& name, const BodySpec& spec, Vector3 pos, float yawDeg) {
    CharModel* cm = GetCharacter(spec);
    ActorSlot slot;
    slot.actor = std::make_unique<Actor>();
    slot.actor->name = name;
    slot.actor->model = cm;
    slot.actor->pos = pos;
    slot.actor->yaw = yawDeg * DEG2RAD;
    slot.actor->SetPose(PoseStand(0), true);
    Actor* a = slot.actor.get();
    actors[name] = std::move(slot);
    return a;
}
Actor* Game::A(const std::string& name) {
    auto it = actors.find(name);
    return it == actors.end() ? nullptr : it->second.actor.get();
}
void Game::RemoveActor(const std::string& name) { actors.erase(name); }
void Game::GiveItem(const std::string& i, int n) { items[i] += n; }
bool Game::TakeItem(const std::string& i, int n) {
    auto it = items.find(i);
    if (it == items.end() || it->second < n) return false;
    it->second -= n;
    return true;
}
void Game::SetTagVisible(const std::string& tag, bool v) {
    for (Entity* e : Scn().FindTag(tag)) {
        e->visible = v;
        Phys().SetOwnerEnabled(e->id, v);
        if (e->interact.enabled || !v) e->interact.enabled = v && (e->interact.action.size() > 0);
        for (auto& l : e->lights) l.on = v;
    }
}

// ---------------------------------------------------------------------------
// Save / load
// ---------------------------------------------------------------------------
bool Game::HasSave() const { return FileExists("wtgk.sav"); }
bool Game::SaveGame() {
    std::ofstream o("wtgk.sav");
    if (!o) return false;
    o << "chapter=" << chapter << "\nmoney=" << money << "\n";
    o << "battery=" << player.battery << "\nweight=" << player.weight << "\nspirit=" << spirit << "\n";
    o << "flags=";
    for (auto& f : flags) o << f << ",";
    o << "\nitems=";
    for (auto& kv : items) o << kv.first << ":" << kv.second << ",";
    o << "\n";
    return true;
}
bool Game::LoadGame() {
    std::ifstream in("wtgk.sav");
    if (!in) return false;
    std::string line;
    flags.clear(); items.clear();
    while (std::getline(in, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (k == "chapter") chapter = atoi(v.c_str());
        else if (k == "money") money = (float)atof(v.c_str());
        else if (k == "battery") player.battery = (float)atof(v.c_str());
        else if (k == "weight") player.weight = (float)atof(v.c_str());
        else if (k == "spirit") spirit = (float)atof(v.c_str());
        else if (k == "flags") for (auto& f : SplitStr(v, ',')) flags.insert(f);
        else if (k == "items") for (auto& it : SplitStr(v, ',')) {
            auto p = SplitStr(it, ':');
            if (p.size() == 2) items[p[0]] = atoi(p[1].c_str());
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Boot
// ---------------------------------------------------------------------------
static void LoadingProgress(float p, const char* status) {
    BeginDrawing();
    DrawLoadingScreen(p, status);
    EndDrawing();
}

static Texture2D BuildGlyphAtlas(int& count) {
    // brightness ramp: darkest -> brightest
    const char* ramp = " .,:;-~=+*!?%#&@";
    count = (int)strlen(ramp);
    const int cell = 32;
    RenderTexture2D rt = LoadRenderTexture(cell * count, cell);
    BeginTextureMode(rt);
    ClearBackground(BLACK);
    Font f = ui::GetFont(ui::F_MONO);
    for (int i = 0; i < count; i++) {
        char s[2] = { ramp[i], 0 };
        Vector2 m = MeasureTextEx(f, s, 30, 0);
        DrawTextEx(f, s, { i * (float)cell + (cell - m.x) * 0.5f, (cell - m.y) * 0.5f }, 30, 0, WHITE);
    }
    EndTextureMode();
    Image img = LoadImageFromTexture(rt.texture);
    UnloadRenderTexture(rt);
    Texture2D t = LoadTextureFromImage(img);   // keep GL orientation (sampled with v up)
    UnloadImage(img);
    SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(t, TEXTURE_WRAP_CLAMP);
    return t;
}

bool Game::Init(int argc, char** argv) {
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--shot") && i + 1 < argc) shotFile = argv[++i];
        else if (!strcmp(argv[i], "--frames") && i + 1 < argc) shotFrames = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--mode") && i + 1 < argc) testMode = argv[++i];
        else if (!strcmp(argv[i], "--chapter") && i + 1 < argc) chapter = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--time") && i + 1 < argc) testTime = (float)atof(argv[++i]);
    }
    settings.Load();
    unsigned flagsW = FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT;
    if (settings.vsync) flagsW |= FLAG_VSYNC_HINT;
    SetConfigFlags(flagsW);
    InitWindow(1280, 720, "What The Ground Keeps");
    SetWindowMinSize(960, 540);
    SetExitKey(KEY_NULL);
    if (FileExists("assets/images/WhatTheGroundKeeps_LogoDesign.jpg")) {
        Image icon = LoadImage("assets/images/WhatTheGroundKeeps_LogoDesign.jpg");
        ImageFormat(&icon, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        SetWindowIcon(icon);
        UnloadImage(icon);
    }
    if (settings.fullscreen && !shotFile) ToggleBorderlessWindowed();
    ui::Init();
    LoadingProgress(0.02f, "waking");
    audio::Init();
    LoadingProgress(0.1f, "listening");
    GenerateTextureSets(512, LoadingProgress);
    InitMaterials();
    Rdr().Init();
    Rdr().glyphAtlas = BuildGlyphAtlas(Rdr().glyphCount);
    RegisterAllPrefabs();
    LoadWorld(LoadingProgress);
    ApplySettings();
    story = std::make_unique<Story>();
    LoadingProgress(1.0f, "the ground keeps");
    // start sculpting the cast in the background while the menu is up
    PrepareCastFrom(testMode == "play" || testMode == "chapter" ? chapter : 0);
    if (testMode == "play" || testMode == "chapter") {
        mode = Mode::Play;
        story->Begin(chapter);
    } else {
        mode = Mode::Menu;
        MenuEnter();
    }
    if (!shotFile) SetTargetFPS(0);
    return true;
}

void Game::LoadWorld(void (*progress)(float, const char*)) {
    progress(0.55f, "surveying route 9");
    Scn().Parse("assets/scenes/world.scene");
    std::vector<TerrainPad> pads; std::vector<Rectangle> holes; std::vector<TerrainPath> paths;
    Scn().CollectTerrain(pads, holes, paths);
    World().Build(pads, holes, paths);
    World().RegisterCollision();
    progress(0.72f, "placing what was left behind");
    Scn().SpawnRecords();
    BuildPowerLines(Scn());
    progress(0.85f, "growing the dead grass");
    std::vector<ExclusionZone> extra;
    for (auto& e : Scn().ents) {
        if (e->prefab == "gas_canopy" || e->prefab == "radio_tower") extra.push_back({ { e->base.x, e->base.z }, { 6, 10 }, e->worldYaw, 2 });
        // open ground (fields, yards): no trees, grass stays
        if (e->prefab == "clearing") extra.push_back({ { e->base.x, e->base.z }, { e->Num("w", 20) * 0.5f, e->Num("d", 20) * 0.5f }, e->worldYaw, 3, true });
    }
    Veg().Build(pads, extra, 1234);
    worldLoaded_ = true;
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
void Game::StartNewGame() {
    flags.clear(); items.clear();
    money = 0; chapter = CH_PROLOGUE; spirit = 0.12f;
    player.battery = 1.0f; player.weight = 0.2f;
    story = std::make_unique<Story>();
    mode = Mode::Play;
    story->Begin(CH_PROLOGUE);
}
void Game::ContinueGame() {
    if (!LoadGame()) { StartNewGame(); return; }
    PrepareCastFrom(chapter);
    story = std::make_unique<Story>();
    mode = Mode::Play;
    story->Begin(chapter);
}
void Game::ReturnToMenu() {
    SaveGame();
    actors.clear();
    story = std::make_unique<Story>();
    camOverride = false;
    hud = Hud();
    ApplySettings();
    mode = Mode::Menu;
    MenuEnter();
}

void Game::Run() {
    while (!quit) {
        if (WindowShouldClose()) { if (mode == Mode::Play) SaveGame(); break; }
        float rawDt = shotFile ? 1.0f / 30.0f : fminf(GetFrameTime(), 0.1f);
        dt = rawDt * timeScale;
        time += rawDt;
        if (IsKeyPressed(KEY_F11)) ToggleBorderlessWindowed();

        switch (mode) {
        case Mode::Menu: MenuUpdate(rawDt); break;
        case Mode::Pause: PauseUpdate(rawDt); break;
        case Mode::Credits: CreditsUpdate(rawDt); break;
        case Mode::Editor: EditorUpdate(rawDt); break;
        case Mode::Play:
            if (IsKeyPressed(KEY_ESCAPE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) {
                if (story && story->InCutscene() && story->skippable) story->SkipCutscene();
                else { mode = Mode::Pause; PauseEnter(); }
            } else if (IsKeyPressed(KEY_F10)) { mode = Mode::Editor; EditorEnter(); }
            else UpdatePlay(dt);
            break;
        default: break;
        }
        hud.Update(rawDt);
        if (mode == Mode::Play) PrepareCastFrom(chapter);   // a new chapter queues the next one's cast
        PumpCharacterUploads();

        // test harness: fast-forward without rendering until the last frames
        frameCount++;
        bool render = !shotFile || frameCount >= shotFrames - 2;
        if (render) {
            RenderWorld();
            BeginDrawing();
            ClearBackground(BLACK);
            Rdr().Present();
            DrawOverlay();
            EndDrawing();
        }
        if (shotFile && frameCount >= shotFrames) { TakeScreenshot(shotFile); break; }
    }
}

void Game::UpdatePlay(float dt) {
    gameTime += dt;
    if (mode == Mode::Play && !IsCursorHidden() && !shotFile) DisableCursor();
    PlayerInput in = shotFile ? PlayerInput{} : ReadPlayerInput(settings.sensitivity, settings.invertY);

    bool cut = story && story->InCutscene();
    if (cut) { in.move = { 0, 0 }; in.interact = in.flashlight = in.use = false; }
    player.Update(in, dt);
    if (!cut) UpdateInteraction(in, dt);
    else { focus = nullptr; focusActor.clear(); hud.SetFocus(false); }
    hud.ShowTasks(IsKeyDown(KEY_TAB) && !cut);
    if (story) story->Update(dt);
    // test hook: WTGK_WARP="x,z,yawDeg[,pitchDeg[,y]]" teleports once the story releases the camera
    static bool warped = false;
    if (!warped && !camOverride && gameTime > 0.5f) {
        if (const char* w = getenv("WTGK_WARP")) {
            float x = 0, z = 0, yw = 0, pt = 0, wy = NAN;
            if (sscanf(w, "%f,%f,%f,%f,%f", &x, &z, &yw, &pt, &wy) >= 3) {
                player.Spawn({ x, std::isnan(wy) ? World().Height(x, z) + 1.0f : wy, z }, yw);
                player.pitch = pt * DEG2RAD;
            }
            if (getenv("WTGK_FLASH")) player.flashOn = true;
        }
        warped = true;
    }
    Scn().Update(dt);
    for (auto& kv : actors) kv.second.actor->Update(dt);
    UpdateWeather(dt);
    UpdateAmbience(dt);
    hud.SetMoney(money);
    hud.SetBattery(player.battery, player.flashOn);
    player.shake = fmaxf(player.shake, camShake);
    camShake = Damp(camShake, 0.0f, 4.0f, dt);
    audio::Update(camera.position, Vector3Normalize(Vector3Subtract(camera.target, camera.position)));
}

// ---------------------------------------------------------------------------
// Interaction: aim at things within reach; prompts come from action handlers
// ---------------------------------------------------------------------------
void Game::UpdateInteraction(const PlayerInput& in, float dt) {
    Vector3 eye = player.EyePos();
    Vector3 fwd = player.Forward();
    Entity* best = nullptr; std::string bestActor;
    float bestScore = 1e9f;
    RayHit wallHit;
    float wallDist = Phys().Raycast(eye, fwd, 4.0f, wallHit) ? wallHit.t : 4.0f;
    for (auto& up : Scn().ents) {
        Entity& e = *up;
        if (!e.visible || !e.interact.enabled) continue;
        Vector3 fp = e.FocusPoint();
        Vector3 d = Vector3Subtract(fp, eye);
        float dist = Vector3Length(d);
        if (dist > e.interact.radius) continue;
        float ang = acosf(Clamp(Vector3DotProduct(Vector3Scale(d, 1.0f / fmaxf(dist, 0.001f)), fwd), -1.0f, 1.0f));
        float maxAng = Clamp(0.5f / fmaxf(dist, 0.3f), 0.12f, 0.6f);
        if (ang > maxAng) continue;
        if (dist > wallDist + 0.6f && wallHit.owner != e.id) continue;   // something solid in the way
        float score = ang * 2.0f + dist * 0.15f;
        if (score < bestScore) { bestScore = score; best = &e; bestActor.clear(); }
    }
    for (auto& kv : actors) {
        if (kv.second.action.empty() || !kv.second.actor->visible) continue;
        Vector3 fp = kv.second.actor->BonePos(B_CHEST, { 0, 0.15f, 0 });
        Vector3 d = Vector3Subtract(fp, eye);
        float dist = Vector3Length(d);
        if (dist > kv.second.radius) continue;
        float ang = acosf(Clamp(Vector3DotProduct(Vector3Scale(d, 1.0f / fmaxf(dist, 0.001f)), fwd), -1.0f, 1.0f));
        if (ang > 0.5f) continue;
        float score = ang * 2.0f + dist * 0.15f - 0.35f;   // people win over props next to them (the register)
        if (score < bestScore) { bestScore = score; best = nullptr; bestActor = kv.first; }
    }
    // resolve prompt
    std::string prompt;
    ActionHandler* h = nullptr;
    if (best) {
        auto it = actions.find(best->interact.action);
        if (it != actions.end()) { h = &it->second; prompt = h->prompt ? h->prompt(*best) : best->interact.prompt; }
        else prompt = best->interact.prompt;
        if (prompt.empty()) best = nullptr;
    } else if (!bestActor.empty()) {
        auto it = actions.find(actors[bestActor].action);
        static Entity dummy;
        dummy.name = bestActor;
        if (it != actions.end()) { h = &it->second; prompt = h->prompt ? h->prompt(dummy) : "Talk"; }
        if (prompt.empty()) bestActor.clear();
    }
    if (best != focus || bestActor != focusActor) holdProgress = 0.0f;
    focus = best; focusActor = bestActor;
    bool has = focus || !focusActor.empty();
    hud.SetFocus(has);
    // hold-to-use actions
    if (has && h && h->holdTime > 0.0f) {
        if (in.interactHeld) holdProgress += dt / h->holdTime;
        else holdProgress = Damp(holdProgress, 0.0f, 6.0f, dt);
        hud.SetPrompt("Hold  " + prompt, holdProgress);
        if (holdProgress >= 1.0f) {
            holdProgress = 0.0f;
            if (focus) h->use(*focus);
            else { static Entity dummy; dummy.name = focusActor; h->use(dummy); }
        }
        return;
    }
    hud.SetPrompt(prompt, 0.0f);
    if (has && in.interact) {
        if (h && h->use) {
            if (focus) h->use(*focus);
            else { static Entity dummy; dummy.name = focusActor; h->use(dummy); }
        }
    }
}

// ---------------------------------------------------------------------------
// Ambience and weather
// ---------------------------------------------------------------------------
void Game::UpdateWeather(float dt) {
    Weather& w = weather;
    auto& s = Rdr().s;
    if (w.storm > 0.01f) {
        w.nextStrike -= dt;
        if (w.nextStrike <= 0) {
            w.lightning = 1.0f;
            w.nextStrike = Frand(8.0f, 26.0f) / w.storm;
            float delay = Frand(0.6f, 3.0f);
            (void)delay;
            audio::Play("thunder", 0.5f + w.storm * 0.5f);
        }
    }
    w.lightning = fmaxf(0.0f, w.lightning - dt * (w.lightning > 0.5f ? 5.0f : 2.0f));
    float flick = w.lightning > 0.05f ? (Frand(0, 1) > 0.3f ? w.lightning : w.lightning * 0.3f) : 0.0f;
    s.lightning = flick * (inUnderground ? 0.0f : 1.0f);
    s.wetWorld = Damp(s.wetWorld, w.rain > 0.1f ? 0.8f : 0.0f, 0.2f, dt);
    s.fogDensity = 0.018f * w.fog;
    // below ground: no sky, no moon, a close brown murk lit only by fire and your torch
    static bool wasUnder = false;
    static RenderSettings surface;
    if (inUnderground && !wasUnder) surface = s;
    if (!inUnderground && wasUnder) {
        s.moonColor = surface.moonColor; s.skyAmbient = surface.skyAmbient; s.groundAmbient = surface.groundAmbient;
        s.fogColor = surface.fogColor; s.sky = surface.sky; s.moonShadows = surface.moonShadows;
        s.fogBase = surface.fogBase;
    }
    wasUnder = inUnderground;
    if (inUnderground) {
        s.moonColor = { 0, 0, 0 };
        s.skyAmbient = { 0.010f, 0.007f, 0.006f };
        s.groundAmbient = { 0.006f, 0.004f, 0.003f };
        s.fogColor = { 0.022f, 0.013f, 0.009f };
        s.fogDensity = 0.03f;
        s.fogBase = player.feet.y - 1.0f;   // height fog is relative to the cavern floor, not sea level
        s.sky = false;
        s.moonShadows = false;
    }
}

void Game::UpdateAmbience(float dt) {
    auto& a = audio::Amb();
    Vector3 p = player.feet;
    // default outdoor night
    float wind = inUnderground ? 0.0f : 0.35f + weather.wind * 0.6f;
    a.wind = Damp(a.wind, wind, 1.0f, dt);
    a.gust = weather.wind;
    a.trees = inUnderground ? 0.0f : 0.5f;
    a.crickets = inUnderground ? 0.0f : 0.7f * (1.0f - weather.rain);
    a.rain = Damp(a.rain, inUnderground ? 0.0f : weather.rain, 0.5f, dt);
    // electrical hum near the lit forecourt, store interior hum
    float hum = 0.0f, fridge = 0.0f, ballast = 0.0f;
    if (Entity* c = Scn().Find("canopy")) {
        float d = Vector3Distance(p, c->base);
        bool lit = !c->lights.empty() && c->lights[0].on;
        hum = lit ? Saturate(1.0f - d / 22.0f) * 0.7f : 0.0f;
    }
    if (Entity* st = Scn().Find("store")) {
        Vector3 l = st->WorldToLocal(p);
        bool inside = fabsf(l.x) < 8.0f && fabsf(l.z) < 6.0f && l.y > -0.5f && l.y < 3.5f;
        bool powered = !Flag("power_out");
        if (inside && powered) { fridge = 0.55f; ballast = 0.5f; hum = fmaxf(hum, 0.35f); }
        else if (powered) fridge = Saturate(1.0f - Vector3Distance(p, st->base) / 16.0f) * 0.25f;
    }
    a.hum = Damp(a.hum, hum, 2.0f, dt);
    a.fridge = Damp(a.fridge, fridge, 2.0f, dt);
    a.ballast = ballast;
    a.drone = Damp(a.drone, inUnderground ? 0.8f : 0.0f, 0.5f, dt);
    a.drips = inUnderground ? 0.8f : 0.0f;
    a.heart = Damp(a.heart, Saturate(player.fear - 0.3f) * 0.8f, 1.5f, dt);
    a.heartRate = 70.0f + player.fear * 70.0f;
}

// ---------------------------------------------------------------------------
// Rendering
// ---------------------------------------------------------------------------
void Game::RenderWorld() {
    if (mode == Mode::Menu || mode == Mode::Credits) MenuCamera();
    else if (mode == Mode::Editor) camera = EditorCamera();
    else if (camOverride) camera = camOverrideCam;
    else camera = player.GetCamera(settings.fov);
    if (const char* dc = getenv("WTGK_CAM")) {   // debug: fixed camera "x,y,z,yawDeg,pitchDeg"
        float x, y, z, yw, pt;
        if (sscanf(dc, "%f,%f,%f,%f,%f", &x, &y, &z, &yw, &pt) == 5) {
            camera.position = { x, y, z };
            camera.target = Vector3Add(camera.position, DirFromYawPitch(yw * DEG2RAD, pt * DEG2RAD));
            camera.up = { 0, 1, 0 };
        }
    }
    if (!settings.headBob && mode == Mode::Play && !camOverride) {
        camera.up = { 0, 1, 0 };
    }
    auto& s = Rdr().s;
    s.brightness = settings.brightness;
    if (const char* b = getenv("WTGK_BRIGHT")) s.brightness *= (float)atof(b);   // debug: inspection exposure
    s.ascii = Clamp(spirit * settings.ascii, 0.0f, 1.0f);
    s.redPulse = Damp(s.redPulse, fearPulse, 3.0f, GetFrameTime());

    Rdr().BeginFrame(camera, time);
    Scn().SubmitLights(camera.position);
    if (mode == Mode::Play && player.flashOn && !camOverride) Rdr().AddLight(player.FlashLight());
    if (story) story->Draw3D();
    World().Draw();
    Veg().Draw(camera.position);
    Scn().Draw();
    for (auto& kv : actors) kv.second.actor->Draw();
    Rdr().Render(nullptr, [&]() { if (story) story->DrawTransparent(); });
}

void Game::DrawOverlay() {
    if (mode == Mode::Menu) { MenuDraw(); return; }
    if (mode == Mode::Credits) { CreditsDraw(); return; }
    if (story) story->DrawOverlay();
    hud.Draw(mode == Mode::Play);
    if (mode == Mode::Pause) PauseDraw();
    if (mode == Mode::Editor) EditorDraw();
}

void Game::Shutdown() {
    settings.Save();
    story.reset();
    actors.clear();
    audio::Shutdown();
    ShutdownCharacters();
    Rdr().Shutdown();
    ui::Shutdown();
    CloseWindow();
}
