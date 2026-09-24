// Game hub: modes, world, player, actors, actions, settings, save data.
#pragma once
#include "engine/scene.h"
#include "engine/audio.h"
#include "engine/ui.h"
#include "player.h"
#include "characters.h"
#include "hud.h"
#include <set>
#include <map>
#include <memory>
#include <functional>

enum class Mode { Loading, Menu, Play, Pause, Editor, Credits };

struct Settings {
    float master = 0.85f, sensitivity = 1.0f, fov = 72.0f, brightness = 1.0f, renderScale = 0.8f;
    float grass = 0.6f, ascii = 1.0f, subtitleSize = 1.0f;
    bool invertY = false, subtitles = true, fullscreen = false, vsync = true, headBob = true;
    float moveSpeed = 1.0f;       // walking/running speed multiplier
    bool volumetrics = true;      // light shafts / glow in the fog (expensive)
    bool shadows = true;          // moon and flashlight shadows
    int quality = 1;              // 0 low, 1 medium, 2 high (preset for the options above)
    void Load();
    void Save() const;
};

struct ActionHandler {
    std::function<std::string(Entity&)> prompt;     // empty string = not interactable right now
    std::function<void(Entity&)> use;
    float holdTime = 0.0f;                           // > 0: hold E to complete
};

struct ActorSlot {
    std::unique_ptr<Actor> actor;
    std::string action;         // interaction action id ("" = none)
    float radius = 2.0f;
};

struct Weather {
    float rain = 0.0f, fog = 1.0f, wind = 0.5f, storm = 0.0f;
    float lightning = 0.0f, nextStrike = 20.0f;
};

class Story;

class Game {
public:
    // --- core ---
    Settings settings;
    Mode mode = Mode::Loading;
    float time = 0.0f;          // real time
    float gameTime = 0.0f;      // scaled time
    float timeScale = 1.0f;
    float dt = 0.0f;
    bool quit = false;

    // --- player & camera ---
    Player player;
    Camera3D camera{};
    bool camOverride = false;   // cutscenes drive camOverrideCam
    Camera3D camOverrideCam{};
    float camShake = 0.0f;
    bool drawPlayerBody = false;

    // --- actors ---
    std::map<std::string, ActorSlot> actors;
    Actor* SpawnActor(const std::string& name, const BodySpec& spec, Vector3 pos, float yawDeg);
    Actor* A(const std::string& name);
    void RemoveActor(const std::string& name);

    // --- interaction ---
    std::map<std::string, ActionHandler> actions;
    Entity* focus = nullptr;
    std::string focusActor;
    std::string focusPrompt;
    float holdProgress = 0.0f;
    std::function<void(const std::string& actor)> onTalk;

    // --- story state (saved) ---
    int chapter = 0;
    float money = 0.0f;
    std::set<std::string> flags;
    std::map<std::string, int> items;
    bool Flag(const std::string& f) const { return flags.count(f) > 0; }
    void SetFlag(const std::string& f) { flags.insert(f); }
    int Item(const std::string& i) const { auto it = items.find(i); return it == items.end() ? 0 : it->second; }
    void GiveItem(const std::string& i, int n = 1);
    bool TakeItem(const std::string& i, int n = 1);
    std::string held;                      // item held in hand ("", "mop", "stock_box", "fuel_can", "shovel", "key", "matches")

    // --- environment ---
    Weather weather;
    float spirit = 0.15f;                  // base ASCII spirit sight (grows with the story)
    float fearPulse = 0.0f;
    bool inUnderground = false;

    // --- systems ---
    Hud hud;
    std::unique_ptr<Story> story;

    // --- lifecycle ---
    bool Init(int argc, char** argv);
    void Run();
    void Shutdown();
    void StartNewGame();
    void ContinueGame();
    void ReturnToMenu();
    bool SaveGame();
    bool LoadGame();
    bool HasSave() const;

    void ApplySettings();

    // world helpers
    void SetTagVisible(const std::string& tag, bool visible);
    void ShakeCamera(float amount) { camShake = fmaxf(camShake, amount); }
    Vector3 PlayerPos() const { return player.feet; }
    float DistToPlayer(Vector3 p) const { return Vector3Distance(p, player.feet); }

    // test harness
    const char* shotFile = nullptr;
    int shotFrames = 0;
    int frameCount = 0;
    std::string testMode;
    float testTime = 0.0f;

private:
    void LoadWorld(void (*progress)(float, const char*));
    void UpdatePlay(float dt);
    void UpdateInteraction(const PlayerInput& in, float dt);
    void UpdateAmbience(float dt);
    void UpdateWeather(float dt);
    void RenderWorld();
    void DrawOverlay();
    bool worldLoaded_ = false;
};

Game& G();
