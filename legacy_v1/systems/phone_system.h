#pragma once

#include <raylib.h>
#include <vector>
#include <string>

class PhoneSystem {
public:
    PhoneSystem();
    ~PhoneSystem();

    void Update(float dt);
    void HandleInput();
    void Draw(RenderTexture2D target, Camera3D camera, float vmSwayX, float vmSwayY, float dayCycleTime, float dayCycleDuration, int waterState);

    bool IsActive() const { return m_active; }
    int GetZoomMode() const { return m_zoomMode; }
    float GetAnim() const { return m_anim; }

private:
    bool m_active = false;
    int m_zoomMode = 0;
    float m_anim = 0.0f;
    float m_radarPulse = 0.0f;
    float m_signalFlicker = 0.0f;
};
