// Minimal diegetic-leaning HUD: crosshair, prompts, objectives, subtitles,
// notifications, chapter titles, fades and letterboxing. One typographic system.
#pragma once
#include "engine/ui.h"
#include <deque>
#include <string>

class Hud {
public:
    void Update(float dt);
    void Draw(bool playing);

    void Say(const std::string& speaker, const std::string& text, float duration = -1.0f);
    void ClearSubtitles();
    bool Speaking() const { return !subs_.empty(); }
    void Objective(const std::string& text);
    void Notify(const std::string& text);
    void Title(const std::string& title, const std::string& sub, float duration = 5.0f);
    void SetPrompt(const std::string& text, float hold) { prompt_ = text; hold_ = hold; }
    void SetFocus(bool f) { focus_ = f; }
    void FadeTo(float target, float speed = 1.0f) { fadeTarget_ = target; fadeSpeed_ = speed; }
    void FadeInstant(float v) { fade_ = fadeTarget_ = v; }
    float Fade() const { return fade_; }
    void Letterbox(bool on) { letterboxTarget_ = on ? 1.0f : 0.0f; }
    void ShowTasks(bool s) { showTasks_ = s; }
    void SetTaskList(const std::vector<std::pair<std::string, bool>>& t) { tasks_ = t; }
    void SetMoney(float m) { money_ = m; }
    void SetBattery(float b, bool on) { battery_ = b; flashOn_ = on; }
    void Hint(const std::string& text, float dur = 4.0f) { hint_ = text; hintT_ = dur; }
    std::string objective;
    float subtitleScale = 1.0f;
    bool subtitlesOn = true;
    bool hidden = false;

private:
    struct Sub { std::string speaker, text; float t, dur; };
    std::deque<Sub> subs_;
    struct Note { std::string text; float t; };
    std::deque<Note> notes_;
    std::string prompt_;
    float hold_ = 0.0f;
    bool focus_ = false;
    float focusAnim_ = 0.0f;
    float objT_ = 0.0f;
    std::string title_, titleSub_;
    float titleT_ = 0.0f, titleDur_ = 0.0f;
    float fade_ = 0.0f, fadeTarget_ = 0.0f, fadeSpeed_ = 1.0f;
    float letterbox_ = 0.0f, letterboxTarget_ = 0.0f;
    bool showTasks_ = false;
    float tasksAnim_ = 0.0f;
    std::vector<std::pair<std::string, bool>> tasks_;
    float money_ = 0.0f, moneyShown_ = 0.0f, moneyT_ = 0.0f;
    float battery_ = 1.0f;
    bool flashOn_ = false;
    std::string hint_;
    float hintT_ = 0.0f;
    float time_ = 0.0f;
};
