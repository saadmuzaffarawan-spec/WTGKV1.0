#include "hud.h"
#include "engine/common.h"

using namespace ui;

void Hud::Say(const std::string& speaker, const std::string& text, float duration) {
    if (duration < 0) duration = 1.6f + text.size() * 0.055f;
    // a new line always replaces whatever is being said (scripts time their own lines)
    subs_.clear();
    subs_.push_back({ speaker, text, 0.0f, duration });
}
void Hud::ClearSubtitles() { subs_.clear(); }
void Hud::Objective(const std::string& t) {
    if (t == objective) return;
    objective = t;
    objT_ = 0.0f;
}
void Hud::Notify(const std::string& t) { notes_.push_back({ t, 0.0f }); if (notes_.size() > 4) notes_.pop_front(); }
void Hud::Title(const std::string& t, const std::string& s, float d) { title_ = t; titleSub_ = s; titleT_ = 0; titleDur_ = d; }

void Hud::Update(float dt) {
    time_ += dt;
    // only the oldest subtitle advances; the rest queue
    if (!subs_.empty()) {
        subs_.front().t += dt;
        if (subs_.front().t > subs_.front().dur) subs_.pop_front();
    }
    for (auto& n : notes_) n.t += dt;
    while (!notes_.empty() && notes_.front().t > 4.0f) notes_.pop_front();
    objT_ += dt;
    titleT_ += dt;
    hintT_ -= dt;
    float step = fadeSpeed_ * dt;
    if (fade_ < fadeTarget_) fade_ = fminf(fade_ + step, fadeTarget_); else fade_ = fmaxf(fade_ - step, fadeTarget_);
    letterbox_ = Damp(letterbox_, letterboxTarget_, 3.0f, dt);
    focusAnim_ = Damp(focusAnim_, focus_ ? 1.0f : 0.0f, 12.0f, dt);
    tasksAnim_ = Damp(tasksAnim_, showTasks_ ? 1.0f : 0.0f, 10.0f, dt);
    if (fabsf(money_ - moneyShown_) > 0.005f) { moneyT_ = 0; moneyShown_ = Damp(moneyShown_, money_, 4.0f, dt); if (fabsf(money_ - moneyShown_) < 0.01f) moneyShown_ = money_; }
    moneyT_ += dt;
}

void Hud::Draw(bool playing) {
    float W = (float)GetScreenWidth(), H = (float)GetScreenHeight();
    float s = UiScale();
    // letterbox
    if (letterbox_ > 0.01f) {
        float bh = H * 0.11f * letterbox_;
        DrawRectangle(0, 0, (int)W, (int)bh, BLACK);
        DrawRectangle(0, (int)(H - bh), (int)W, (int)bh + 1, BLACK);
    }
    bool showWorldHud = playing && !hidden && letterbox_ < 0.2f;
    if (showWorldHud) {
        // crosshair: a faint dot that opens into a ring when something can be used
        Vector2 c{ W * 0.5f, H * 0.5f };
        DrawCircleV(c, 1.6f * s, Alpha(kInk, 0.35f + focusAnim_ * 0.4f));
        if (focusAnim_ > 0.02f) {
            float r = (5.0f + focusAnim_ * 5.0f) * s;
            DrawRing(c, r - 1.0f * s, r, 0, 360, 36, Alpha(kInk, focusAnim_ * 0.55f));
            if (hold_ > 0.0f) DrawRing(c, r - 2.2f * s, r + 1.2f * s, -90, -90 + 360 * hold_, 36, Alpha(kInk, 0.9f));
        }
        if (!prompt_.empty() && focusAnim_ > 0.05f) {
            float fs = 15.0f * s;
            Vector2 m = Measure(F_MONO, prompt_.c_str(), fs);
            float kx = c.x - (m.x + fs * 1.7f) * 0.5f, ky = c.y + 34 * s;
            KeyCap("E", kx, ky - fs * 0.15f, fs, Alpha(kInk, focusAnim_));
            Text(F_MONO, prompt_.c_str(), kx + fs * 1.7f, ky, fs, Alpha(kInk, focusAnim_ * 0.9f));
        }
        if (hintT_ > 0.0f && !hint_.empty()) {
            float a = Saturate(hintT_) * Saturate((4.0f - hintT_) * 3.0f + 1.0f);
            TextCentered(F_MONO_LIGHT, hint_.c_str(), c.x, c.y + 64 * s, 14.0f * s, Alpha(kDim, a));
        }
        // objective (fades after a while; Tab brings it back)
        float oa = (objT_ < 8.0f ? Saturate(objT_ * 2.0f) * Saturate((8.0f - objT_) * 0.6f) : 0.0f);
        oa = fmaxf(oa, tasksAnim_);
        if (!objective.empty() && oa > 0.01f) {
            Text(F_MONO_LIGHT, "objective", 36 * s, 30 * s, 12.0f * s, Alpha(kFaint, oa), 2.0f * s);
            TextDecay(F_MONO, objective.c_str(), 36 * s, 46 * s, 17.0f * s, Alpha(kInk, oa), 0.0f, Saturate(0.4f - objT_ * 0.8f), time_);
        }
        // task sheet
        if (tasksAnim_ > 0.01f && !tasks_.empty()) {
            float x = W - 360 * s + (1.0f - tasksAnim_) * 40 * s, y = 34 * s;
            Text(F_MONO_LIGHT, "tonight's shift", x, y, 12.0f * s, Alpha(kFaint, tasksAnim_), 2.0f * s);
            y += 22 * s;
            for (auto& t : tasks_) {
                std::string line = std::string(t.second ? "[x] " : "[ ] ") + t.first;
                Text(F_MONO, line.c_str(), x, y, 15.0f * s, Alpha(t.second ? kFaint : kInk, tasksAnim_));
                y += 22 * s;
            }
        }
        // money
        float ma = fmaxf(tasksAnim_, moneyT_ < 4.0f ? Saturate((4.0f - moneyT_)) : 0.0f);
        if (ma > 0.01f) TextRight(F_MONO, TextFormat("$%.2f", moneyShown_), W - 36 * s, H - 48 * s, 16.0f * s, Alpha(kInk, ma));
        // battery only matters when it's running out
        if (flashOn_ && battery_ < 0.3f) {
            int bars = (int)ceilf(battery_ / 0.3f * 5);
            std::string b = "[";
            for (int i = 0; i < 5; i++) b += i < bars ? "|" : ".";
            b += "] battery";
            float blink = battery_ < 0.1f ? (sinf(time_ * 8) > 0 ? 1.0f : 0.3f) : 0.8f;
            Text(F_MONO_LIGHT, b.c_str(), 36 * s, H - 48 * s, 13.0f * s, Alpha(kDim, blink));
        }
        // notifications
        float ny = H - 80 * s;
        for (auto it = notes_.rbegin(); it != notes_.rend(); ++it) {
            float a = Saturate(it->t * 4.0f) * Saturate(4.0f - it->t);
            TextRight(F_MONO_LIGHT, it->text.c_str(), W - 36 * s, ny, 14.0f * s, Alpha(kDim, a));
            ny -= 20 * s;
        }
    }
    // chapter titles
    if (titleT_ < titleDur_) {
        float a = Saturate(titleT_ / 1.2f) * Saturate((titleDur_ - titleT_) / 1.2f);
        float decay = Saturate(1.0f - titleT_ / 1.6f);
        TextDecay(F_MONO_THIN, title_.c_str(), W * 0.5f, H * 0.42f, 44.0f * s, Alpha(kInk, a), 10.0f * s, decay, time_, true);
        TextCentered(F_MONO_LIGHT, titleSub_.c_str(), W * 0.5f, H * 0.42f + 60 * s, 15.0f * s, Alpha(kDim, a), 3.0f * s);
    }
    // fade to black (subtitles stay readable above it)
    if (fade_ > 0.001f) DrawRectangle(0, 0, (int)W + 1, (int)H + 1, Alpha(BLACK, fade_));
    // subtitles
    if (subtitlesOn && !subs_.empty()) {
        const Sub& sb = subs_.front();
        float a = Saturate(sb.t * 5.0f) * Saturate((sb.dur - sb.t) * 3.0f);
        float fs = 19.0f * s * subtitleScale;
        // type-on reveal
        size_t shown = (size_t)fminf((float)sb.text.size(), sb.t * 55.0f);
        std::string vis = sb.text.substr(0, shown);
        float y = H * (letterbox_ > 0.5f ? 0.84f : 0.82f);
        Vector2 m = Measure(F_MONO, sb.text.c_str(), fs);
        if (m.x > W * 0.7f) {
            // wrap long lines
            TextWrapped(F_MONO, vis.c_str(), W * 0.15f, y, W * 0.7f, fs, Alpha(kInk, a));
        } else {
            float x = W * 0.5f - m.x * 0.5f;
            DrawRectangleGradientH((int)(x - 60 * s), (int)(y - 8 * s), (int)(m.x * 0.5f + 60 * s), (int)(fs + 16 * s), Alpha(BLACK, 0.0f), Alpha(BLACK, 0.45f * a));
            DrawRectangleGradientH((int)(x + m.x * 0.5f), (int)(y - 8 * s), (int)(m.x * 0.5f + 60 * s), (int)(fs + 16 * s), Alpha(BLACK, 0.45f * a), Alpha(BLACK, 0.0f));
            Text(F_MONO, vis.c_str(), x, y, fs, Alpha(kInk, a));
        }
        if (!sb.speaker.empty())
            TextCentered(F_MONO_LIGHT, sb.speaker.c_str(), W * 0.5f, y - 22 * s * subtitleScale, 12.0f * s * subtitleScale, Alpha(sb.speaker == "GRETHNAR" ? Color{ 170, 60, 50, 255 } : kDim, a), 3.0f * s);
    }
}
