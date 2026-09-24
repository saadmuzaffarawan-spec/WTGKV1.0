#include "menus.h"
#include "game.h"
#include "story.h"

using namespace ui;

namespace {

struct MenuState {
    int sel = 0;
    int prevSel = -1;
    float t = 0;
    float hover[8] = { 0 };
    bool settingsOpen = false;
    int settingsSel = 0;
    bool confirmNew = false;
    float enterT = 0;
    float startT = -1;      // new game transition
    bool startContinue = false;
    bool guideOpen = false;
    float guideScroll = 0;
    float creditsT = 0;
};
MenuState M;

bool Up() { return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP); }
bool Down() { return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN); }
bool Left() { return IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT); }
bool Right() { return IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT); }
bool Accept() { return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_E) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN); }
bool Back() { return IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE) || IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT); }

// ---- list widget shared by main menu and pause ----
int DrawList(const std::vector<std::string>& items, int& sel, float x, float y, float size, float* hover, bool mouseEnabled, bool& clicked) {
    float s = UiScale();
    clicked = false;
    Vector2 mp = GetMousePosition();
    for (int i = 0; i < (int)items.size(); i++) {
        float iy = y + i * size * 1.9f;
        Vector2 m = Measure(F_MONO, items[i].c_str(), size, 2.0f * s);
        Rectangle r{ x - 10 * s, iy - 6 * s, m.x + 60 * s, size + 12 * s };
        if (mouseEnabled && CheckCollisionPointRec(mp, r)) {
            if (Vector2Length(GetMouseDelta()) > 0.5f) sel = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { sel = i; clicked = true; }
        }
        hover[i] = Damp(hover[i], i == sel ? 1.0f : 0.0f, 14.0f, GetFrameTime());
        float h = hover[i];
        Color c = ColorLerp3(kFaint, kInk, h);
        float ox = h * 22 * s;
        if (h > 0.02f) Hairline(x, iy + size * 0.55f, x + ox * 0.7f, iy + size * 0.55f, Alpha(kBlood, h));
        TextDecay(F_MONO, items[i].c_str(), x + ox, iy, size, c, 2.0f * s, i == sel ? Saturate(0.25f - (M.t - M.enterT) * 0.2f) * 0.0f : 0.0f, M.t);
    }
    return sel;
}

struct SettingRow { const char* label; std::function<std::string()> value; std::function<void(int)> change; };

std::vector<SettingRow> Rows() {
    Settings& st = G().settings;
    auto pct = [](float v) { return std::string(TextFormat("%d%%", (int)roundf(v * 100))); };
    auto onoff = [](bool b) { return std::string(b ? "on" : "off"); };
    return {
        { "master volume", [&] { return pct(st.master); }, [&](int d) { st.master = Clamp(st.master + d * 0.05f, 0, 1); } },
        { "mouse sensitivity", [&] { return std::string(TextFormat("%.2f", st.sensitivity)); }, [&](int d) { st.sensitivity = Clamp(st.sensitivity + d * 0.1f, 0.1f, 4.0f); } },
        { "invert look", [&] { return onoff(st.invertY); }, [&](int) { st.invertY = !st.invertY; } },
        { "field of view", [&] { return std::string(TextFormat("%d", (int)st.fov)); }, [&](int d) { st.fov = Clamp(st.fov + d * 2.0f, 55, 100); } },
        { "brightness", [&] { return pct(st.brightness); }, [&](int d) { st.brightness = Clamp(st.brightness + d * 0.05f, 0.5f, 2.0f); } },
        { "spirit sight (ascii)", [&] { return pct(st.ascii); }, [&](int d) { st.ascii = Clamp(st.ascii + d * 0.1f, 0.0f, 1.5f); } },
        { "graphics quality", [&] { static const char* q[] = { "low", "medium", "high" }; return std::string(q[st.quality < 0 ? 0 : st.quality > 2 ? 2 : st.quality]); },
          [&](int d) {
              st.quality = (st.quality + (d < 0 ? 2 : 1)) % 3;
              // presets: low favours speed on weak / integrated GPUs
              if (st.quality == 0) { st.renderScale = 0.6f; st.grass = 0.2f; st.volumetrics = false; st.shadows = false; st.drawDistance = 100; }
              if (st.quality == 1) { st.renderScale = 0.8f; st.grass = 0.5f; st.volumetrics = false; st.shadows = true; st.drawDistance = 180; }
              if (st.quality == 2) { st.renderScale = 1.0f; st.grass = 1.0f; st.volumetrics = true; st.shadows = true; st.drawDistance = 320; }
              G().ApplySettings();
          } },
        { "volumetric fog", [&] { return onoff(st.volumetrics); }, [&](int) { st.volumetrics = !st.volumetrics; G().ApplySettings(); } },
        { "shadows", [&] { return onoff(st.shadows); }, [&](int) { st.shadows = !st.shadows; G().ApplySettings(); } },
        { "fps limit", [&] { return st.fpsLimit ? std::to_string(st.fpsLimit) : std::string("unlimited"); },
          [&](int d) { static const int v[] = { 30, 60, 90, 120, 144, 165, 240, 0 }; int i = 0; while (i < 7 && v[i] != st.fpsLimit) i++;
                       i = (i + (d < 0 ? 7 : 1)) % 8; st.fpsLimit = v[i]; G().ApplySettings(); } },
        { "show fps", [&] { return onoff(st.showFps); }, [&](int) { st.showFps = !st.showFps; } },
        { "draw distance", [&] { return std::string(TextFormat("%d m", (int)st.drawDistance)); }, [&](int d) { st.drawDistance = Clamp(st.drawDistance + d * 20.0f, 60.0f, 320.0f); G().ApplySettings(); } },
        { "movement speed", [&] { return pct(st.moveSpeed); }, [&](int d) { st.moveSpeed = Clamp(st.moveSpeed + d * 0.1f, 0.6f, 2.0f); G().ApplySettings(); } },
        { "vsync", [&] { return onoff(st.vsync); }, [&](int) { st.vsync = !st.vsync; if (st.vsync) SetWindowState(FLAG_VSYNC_HINT); else ClearWindowState(FLAG_VSYNC_HINT); } },
        { "render scale", [&] { return pct(st.renderScale); }, [&](int d) { st.renderScale = Clamp(st.renderScale + d * 0.05f, 0.5f, 1.0f); } },
        { "grass density", [&] { return pct(st.grass); }, [&](int d) { st.grass = Clamp(st.grass + d * 0.1f, 0.0f, 1.0f); G().ApplySettings(); } },
        { "head bob", [&] { return onoff(st.headBob); }, [&](int) { st.headBob = !st.headBob; } },
        { "subtitles", [&] { return onoff(st.subtitles); }, [&](int) { st.subtitles = !st.subtitles; } },
        { "subtitle size", [&] { return pct(st.subtitleSize); }, [&](int d) { st.subtitleSize = Clamp(st.subtitleSize + d * 0.1f, 0.7f, 1.8f); } },
        { "fullscreen", [&] { return onoff(st.fullscreen); }, [&](int) { st.fullscreen = !st.fullscreen; ToggleBorderlessWindowed(); } },
        { "back", [] { return std::string(); }, [](int) {} },
    };
}

void SettingsUpdate() {
    auto rows = Rows();
    int n = (int)rows.size();
    if (Up()) { M.settingsSel = (M.settingsSel + n - 1) % n; audio::Play("ui_hover", 0.5f); }
    if (Down()) { M.settingsSel = (M.settingsSel + 1) % n; audio::Play("ui_hover", 0.5f); }
    if (Left()) { rows[M.settingsSel].change(-1); audio::Play("ui_hover", 0.6f); }
    if (Right()) { rows[M.settingsSel].change(1); audio::Play("ui_hover", 0.6f); }
    bool back = Back() || (Accept() && M.settingsSel == n - 1);
    if (Accept() && M.settingsSel < n - 1) { rows[M.settingsSel].change(1); audio::Play("ui_select", 0.6f); }
    // mouse
    float s = UiScale();
    float x = GetScreenWidth() * 0.1f, y = GetScreenHeight() * 0.2f;
    float rh = fminf(30 * s, (GetScreenHeight() * 0.68f) / n);
    Vector2 mp = GetMousePosition();
    for (int i = 0; i < n; i++) {
        Rectangle r{ x, y + i * rh - 4 * s, 560 * s, rh };
        if (CheckCollisionPointRec(mp, r)) {
            if (Vector2Length(GetMouseDelta()) > 0.5f) M.settingsSel = i;
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { if (i == n - 1) back = true; else rows[i].change(1); audio::Play("ui_select", 0.6f); }
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { rows[i].change(-1); audio::Play("ui_select", 0.6f); }
        }
    }
    audio::SetMasterVolume(G().settings.master);
    Rdr().s.renderScale = G().settings.renderScale;
    if (back) { M.settingsOpen = false; G().settings.Save(); audio::Play("ui_back", 0.6f); }
}

void SettingsDraw() {
    float s = UiScale();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    DrawRectangleGradientH(0, 0, (int)(W * 0.7f), (int)H, Alpha(BLACK, 0.85f), Alpha(BLACK, 0.0f));
    float x = W * 0.1f, y = H * 0.28f;
    Text(F_MONO_THIN, "settings", x, H * 0.2f - 60 * s, 34 * s, kInk, 6 * s);
    auto rows = Rows();
    y = H * 0.2f;
    float rh = fminf(30 * s, (H * 0.68f) / rows.size());
    float fs = fminf(16 * s, rh * 0.62f);
    for (int i = 0; i < (int)rows.size(); i++) {
        bool on = i == M.settingsSel;
        Color c = on ? kInk : kFaint;
        Text(F_MONO, rows[i].label, x + (on ? 18 * s : 0), y + i * rh, fs, c);
        std::string v = rows[i].value();
        if (!v.empty()) Text(F_MONO, (on ? "< " + v + " >" : v).c_str(), x + 330 * s, y + i * rh, fs, on ? kInk : kDim);
        if (on) Hairline(x, y + i * rh + fs * 0.55f, x + 12 * s, y + i * rh + fs * 0.55f, kBlood);
    }
    Text(F_MONO_LIGHT, "left/right to change   esc to go back", x, H - 60 * s, 13 * s, kFaint, 1 * s);
    if (rows[M.settingsSel].label == std::string("brightness")) {
        // calibration: the left mark should be barely visible
        float bx = x + 560 * s, by = H * 0.3f;
        DrawRectangle((int)bx, (int)by, (int)(40 * s), (int)(40 * s), Color{ (unsigned char)(8 * G().settings.brightness), (unsigned char)(8 * G().settings.brightness), (unsigned char)(8 * G().settings.brightness), 255 });
        DrawRectangle((int)(bx + 50 * s), (int)by, (int)(40 * s), (int)(40 * s), Color{ 30, 30, 30, 255 });
        Text(F_MONO_LIGHT, "left square: barely visible", bx, by + 48 * s, 12 * s, kFaint);
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------
void DrawLoadingScreen(float p, const char* status) {
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight(), s = UiScale();
    ClearBackground(Color{ 3, 3, 4, 255 });
    float t = (float)GetTime();
    TextDecay(F_MONO_THIN, "WHAT THE GROUND KEEPS", W * 0.5f, H * 0.42f, 40 * s, kInk, 12 * s, Saturate(0.6f - p), t, true);
    int n = 32, k = (int)(p * n);
    std::string bar = "";
    for (int i = 0; i < n; i++) bar += i < k ? "/" : ".";
    TextCentered(F_MONO_LIGHT, bar.c_str(), W * 0.5f, H * 0.42f + 70 * s, 14 * s, kFaint, 3 * s);
    TextCentered(F_MONO_LIGHT, status, W * 0.5f, H * 0.42f + 96 * s, 13 * s, kDim, 2 * s);
}

// ---------------------------------------------------------------------------
// Main menu
// ---------------------------------------------------------------------------
void MenuCamera() {
    Game& g = G();
    float t = M.t;
    // slow drift up the empty road towards the tower's beacon
    float z = 120.0f + fmodf(t * 0.9f, 30.0f) * 0.0f + sinf(t * 0.05f) * 6.0f;
    Vector3 pos{ Terrain::RoadX(z) - 1.2f + sinf(t * 0.11f) * 0.6f, 0, z };
    pos.y = World().Height(pos.x, pos.z) + 1.55f + sinf(t * 0.37f) * 0.04f;
    Vector3 tgt{ 23.0f, 18.0f + sinf(t * 0.07f) * 4.0f, 171.0f };
    g.camera.position = pos;
    g.camera.target = tgt;
    g.camera.up = Vector3RotateByAxisAngle({ 0, 1, 0 }, Vector3Normalize(Vector3Subtract(tgt, pos)), sinf(t * 0.13f) * 0.01f);
    g.camera.fovy = 55.0f;
    g.camera.projection = CAMERA_PERSPECTIVE;
}

// ---------------------------------------------------------------------------
// How to play (same text as GUIDE.md)
// ---------------------------------------------------------------------------
static const char* kGuide[] = {
    "HOW TO PLAY",
    "",
    "CONTROLS",
    "  WASD move    mouse look    Shift run    Ctrl/C crouch    Space jump",
    "  E  interact (look at something until a prompt appears; some prompts say HOLD - keep E down)",
    "  F  flashlight    left mouse  pour fuel (last chapter)    Tab  hold for tonight's task list",
    "  Esc  pause (settings, save, quit)",
    "",
    "HOW THE GAME PROGRESSES",
    "  The top-left objective always says what to do next. The game saves at the start of every chapter.",
    "  Prologue  - a cutscene. Just watch (Esc skips where allowed).",
    "  Awakening - follow the blood trail down the road to the gas station hatch, then walk into the",
    "              store and talk to Mr. Grethnar behind the counter (look at him, press E). Accept the deal.",
    "  Nights 1-3 - it is always night: each chapter is one night shift. Hold Tab to see the tasks.",
    "              Do every task, then go back to Grethnar and press E to report. He gives you a clue,",
    "              and the next night begins. Talking to him early just reminds you what is left.",
    "              Night 1: pump fuel for the customer, restock shelves (boxes in the storage room),",
    "                       mop the stains (take the mop, hold E on a stain), trash to the dumpster.",
    "              Night 2: pump + ring up the customer at the register, dip the tank at the hatch,",
    "                       fetch the ledger from the office of Blackwood College (across the road).",
    "              Night 3: mop, reset the breakers in the storage room when the power dies, then",
    "                       relight the boiler in the college basement (stairs at the back of the building).",
    "  The Key   - find the barn past the cow skeletons (south-east field) and take the shovel.",
    "              Count the cows from the fence by the road; hold E beside the third one to dig. Then RUN",
    "              back over the fence. Use the key on the iron door in the college basement.",
    "  Below     - go down the stairs, follow the torches to the far end, find Zain. Don't touch the",
    "              bodies. Then run back to the stairs. Shine your flashlight at creatures to scare them.",
    "  Burn      - take the red fuel can next to the store, fill it at a pump (hold E), pour (hold",
    "              left mouse) near the hatch, the pumps and the store, stand on the trail and press E.",
    "",
    "TIPS",
    "  If a game runs slowly: settings > graphics quality > low (or lower render scale).",
    "  Movement speed, field of view, shadows and volumetric fog are in settings too.",
    "  Getting hit three times sends you back to a checkpoint. The flashlight battery refills",
    "  at the register (buy batteries).",
    "",
    "Esc / Enter to close",
};
static void GuideUpdate() {
    if (Back() || Accept() || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { M.guideOpen = false; audio::Play("ui_back", 0.6f); }
}
static void GuideDraw() {
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight(), s = UiScale();
    DrawRectangle(0, 0, (int)W, (int)H, Alpha(BLACK, 0.9f));
    int n = (int)(sizeof(kGuide) / sizeof(kGuide[0]));
    float lh = fminf(22 * s, (H * 0.9f) / n), fs = lh * 0.68f;
    float x = W * 0.06f, y = H * 0.05f;
    for (int i = 0; i < n; i++) {
        bool head = kGuide[i][0] != ' ' && kGuide[i][0] != 0 && i < n - 1;
        Text(head ? F_MONO_BOLD : F_MONO_LIGHT, kGuide[i], x, y + i * lh, head ? fs * 1.1f : fs, head ? kInk : kDim);
    }
}

void MenuEnter() {
    M = MenuState();
    M.sel = G().HasSave() ? 0 : 1;
    EnableCursor();
    G().SetTagVisible("after_crash", false);
    G().spirit = 0.35f;
    G().hud.FadeInstant(1.0f);
    G().hud.FadeTo(0.0f, 0.4f);
    auto& a = audio::Amb();
    a.wind = 0.6f; a.crickets = 0.7f; a.engine = 0; a.road = 0; a.drone = 0; a.fire = 0; a.heart = 0; a.breath = 0;
}

void MenuUpdate(float dt) {
    Game& g = G();
    M.t += dt;
    g.hud.Update(0);   // fade handled in main loop
    Scn().Update(dt);
    auto& a = audio::Amb();
    a.wind = 0.55f; a.gust = 0.7f; a.crickets = 0.7f; a.trees = 0.6f; a.indoor = 0; a.hum = 0; a.fridge = 0;
    audio::Update(g.camera.position, Vector3Normalize(Vector3Subtract(g.camera.target, g.camera.position)));
    if (M.startT >= 0) {
        M.startT += dt;
        if (M.startT > 2.2f) {
            if (M.startContinue) g.ContinueGame(); else g.StartNewGame();
        }
        return;
    }
    if (M.settingsOpen) { SettingsUpdate(); return; }
    if (M.guideOpen) { GuideUpdate(); return; }
    std::vector<std::string> items;
    bool hasSave = g.HasSave();
    int n = 5;
    if (Up()) { M.sel = (M.sel + n - 1) % n; if (!hasSave && M.sel == 0) M.sel = n - 1; }
    if (Down()) { M.sel = (M.sel + 1) % n; if (!hasSave && M.sel == 0) M.sel = 1; }
    if (M.sel != M.prevSel) { if (M.prevSel >= 0) audio::Play("ui_hover", 0.5f); M.prevSel = M.sel; }
    bool accept = Accept() || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && M.hover[M.sel] > 0.5f && M.t > 0.3f);
    if (accept) {
        audio::Play("ui_select", 0.8f);
        if (M.sel == 0 && hasSave) { M.startT = 0; M.startContinue = true; g.hud.FadeTo(1.0f, 0.6f); }
        else if (M.sel == 1) { M.startT = 0; M.startContinue = false; g.hud.FadeTo(1.0f, 0.6f); }
        else if (M.sel == 2) { M.settingsOpen = true; M.settingsSel = 0; }
        else if (M.sel == 3) M.guideOpen = true;
        else if (M.sel == 4) g.quit = true;
    }
}

void MenuDraw() {
    Game& g = G();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight(), s = UiScale();
    DrawRectangleGradientH(0, 0, (int)(W * 0.55f), (int)H, Alpha(BLACK, 0.7f), Alpha(BLACK, 0.0f));
    if (M.settingsOpen) { SettingsDraw(); g.hud.Draw(false); return; }
    if (M.guideOpen) { GuideDraw(); return; }
    float x = W * 0.1f, y = H * 0.3f;
    float glitch = (fmodf(M.t, 7.3f) < 0.18f) ? 0.5f : 0.0f;
    TextDecay(F_MONO_THIN, "WHAT THE", x, y, 30 * s, kInk, 10 * s, Saturate(1.2f - M.t * 0.6f) + glitch * 0.3f, M.t);
    TextDecay(F_MONO_THIN, "GROUND KEEPS", x, y + 38 * s, 48 * s, kInk, 10 * s, Saturate(1.4f - M.t * 0.6f) + glitch, M.t);
    Text(F_MONO_LIGHT, "a night on route 9", x + 2 * s, y + 100 * s, 14 * s, Alpha(kDim, Saturate(M.t - 1.0f)), 3 * s);
    bool hasSave = g.HasSave();
    std::vector<std::string> items{ "continue", "new game", "settings", "how to play", "quit" };
    float ly = y + 170 * s;
    bool clicked;
    int sel = M.sel;
    DrawList(items, sel, x, ly, 21 * s, M.hover, M.startT < 0, clicked);
    if (!hasSave) Text(F_MONO_LIGHT, "(no save)", x + 140 * s, ly + 2 * s, 13 * s, kFaint);
    if (sel != M.sel && !(sel == 0 && !hasSave)) M.sel = sel;
    Text(F_MONO_LIGHT, "headphones recommended. there is no music - only what you hear out there.", x, H - 56 * s, 12 * s, Alpha(kFaint, Saturate(M.t - 2.0f)), 1 * s);
    TextRight(F_MONO_LIGHT, "v2.0", W - 30 * s, H - 56 * s, 12 * s, kFaint);
    g.hud.Draw(false);
}

// ---------------------------------------------------------------------------
// Pause
// ---------------------------------------------------------------------------
void PauseEnter() {
    M.sel = 0; M.settingsOpen = false; M.guideOpen = false; M.prevSel = 0;
    EnableCursor();
    audio::Amb().muffle = 0.6f;
}

void PauseUpdate(float dt) {
    Game& g = G();
    M.t += dt;
    if (M.settingsOpen) { SettingsUpdate(); return; }
    if (M.guideOpen) { GuideUpdate(); return; }
    int n = 5;
    if (Up()) M.sel = (M.sel + n - 1) % n;
    if (Down()) M.sel = (M.sel + 1) % n;
    if (M.sel != M.prevSel) { audio::Play("ui_hover", 0.5f); M.prevSel = M.sel; }
    bool accept = Accept() || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && M.hover[M.sel] > 0.5f);
    auto resume = [&]() { g.mode = Mode::Play; audio::Amb().muffle = 0.0f; DisableCursor(); g.ApplySettings(); };
    if (Back()) { resume(); return; }
    if (accept) {
        audio::Play("ui_select", 0.7f);
        if (M.sel == 0) resume();
        else if (M.sel == 1) { M.settingsOpen = true; M.settingsSel = 0; }
        else if (M.sel == 2) M.guideOpen = true;
        else if (M.sel == 3) { audio::Amb().muffle = 0.0f; g.ReturnToMenu(); }
        else if (M.sel == 4) { g.SaveGame(); g.quit = true; }
    }
}

void PauseDraw() {
    Game& g = G();
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight(), s = UiScale();
    DrawRectangle(0, 0, (int)W, (int)H, Alpha(BLACK, 0.55f));
    if (M.settingsOpen) { SettingsDraw(); return; }
    if (M.guideOpen) { GuideDraw(); return; }
    float x = W * 0.1f, y = H * 0.32f;
    Text(F_MONO_THIN, "paused", x, y, 36 * s, kInk, 8 * s);
    if (g.story) Text(F_MONO_LIGHT, ChapterTitle(g.story->chapter), x + 2 * s, y + 50 * s, 13 * s, kDim, 3 * s);
    if (!g.hud.objective.empty()) Text(F_MONO_LIGHT, g.hud.objective.c_str(), x + 2 * s, y + 70 * s, 13 * s, kFaint);
    std::vector<std::string> items{ "resume", "settings", "how to play", "save and return to menu", "save and quit" };
    bool clicked;
    DrawList(items, M.sel, x, y + 120 * s, 20 * s, M.hover, true, clicked);
    if (clicked) {}
}

// ---------------------------------------------------------------------------
// Credits
// ---------------------------------------------------------------------------
void CreditsEnter() { M.creditsT = 0; EnableCursor(); }
void CreditsUpdate(float dt) {
    M.creditsT += dt;
    M.t += dt;
    if (M.creditsT > 58.0f || (M.creditsT > 3.0f && (Accept() || Back()))) G().ReturnToMenu();
}
void CreditsDraw() {
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight(), s = UiScale();
    DrawRectangle(0, 0, (int)W, (int)H, Alpha(BLACK, 0.8f));
    static const char* lines[] = {
        "WHAT THE GROUND KEEPS", "", "a game by", "Saad Muzaffar Awan", "", "", "for every older brother", "who would do it again", "", "", "",
        "engine, sound and world", "built from nothing in C++ and raylib", "", "every sound you heard was made by code", "there was no music", "", "", "",
        "the ground keeps what it is given", "", "", "", "thank you for playing" };
    float y = H - M.creditsT * 38 * s;
    for (const char* l : lines) {
        bool title = l == lines[0];
        TextCentered(title ? F_MONO_THIN : F_MONO_LIGHT, l, W * 0.5f, y, (title ? 34 : 16) * s, title ? kInk : kDim, (title ? 8 : 2) * s);
        y += (title ? 60 : 30) * s;
    }
}
