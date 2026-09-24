// Story director: chapters as scripted step sequences, plus the systems that
// only exist for the story (the car rig, decals, particles, the job, creatures, fire).
#pragma once
#include "game.h"
#include <functional>
#include <vector>

// ---------------------------------------------------------------------------
// Step scripts: each step has an enter action and an update that returns true when done.
// ---------------------------------------------------------------------------
struct Step {
    std::function<void()> enter;
    std::function<bool(float t, float dt)> update;
};

class Script {
public:
    std::vector<Step> steps;
    size_t i = 0;
    float t = 0.0f;
    bool started = false;
    void Update(float dt);
    bool Done() const { return i >= steps.size(); }
    void Clear() { steps.clear(); i = 0; t = 0; started = false; }
    // builders
    Script& Do(std::function<void()> fn);
    Script& Wait(float seconds);
    Script& Until(std::function<bool()> pred);
    Script& Run(std::function<bool(float t, float dt)> fn, std::function<void()> enter = nullptr);
    Script& Say(const std::string& who, const std::string& text, float dur = -1.0f, float gapAfter = 0.35f);
    Script& Objective(const std::string& text);
    // insert steps to run right after the current one (branching)
    void InsertNext(const std::vector<Step>& st) { steps.insert(steps.begin() + (long)(i + 1), st.begin(), st.end()); }
};

// Camera path keyframes for cutscenes
struct CamKey { float t; Vector3 pos; Vector3 target; float fov; };
Camera3D SampleCamPath(const std::vector<CamKey>& keys, float t);

enum Chapter {
    CH_PROLOGUE = 0, CH_AWAKENING, CH_SHIFT1, CH_SHIFT2, CH_SHIFT3, CH_KEY, CH_BELOW, CH_BURN, CH_END
};
const char* ChapterTitle(int ch);

class Story {
public:
    Story();
    ~Story();
    void Begin(int chapter);
    void Update(float dt);
    void Draw3D();
    void DrawTransparent();
    void DrawOverlay();
    bool InCutscene() const { return cutscene; }
    void SkipCutscene();
    int chapter = 0;
    int pendingChapter = -1;
    bool cutscene = false;
    bool skippable = false;
    Script script;      // main chapter script
    Script side;        // parallel ambient events
    struct Impl;
    Impl* impl;
};
