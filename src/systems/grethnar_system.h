#pragma once

#include <raylib.h>
#include <raymath.h>
#include <vector>
#include "core/game_context.h"
#include "systems/shop_lighting.h"

enum GrethnarState {
    GRETHNAR_NORMAL = 0,
    GRETHNAR_STARING,
    GRETHNAR_PRIMED,
    GRETHNAR_VANISHED,
    GRETHNAR_JUMPSCARE,
    GRETHNAR_COOLDOWN
};

struct GrethnarEyeBloodDrop {
    Vector3 pos;
    Vector3 prevPos;
    Vector3 vel;
    float life;
    float maxLife;
    float scale;
};

class GrethnarSystem {
public:
    GrethnarSystem();
    ~GrethnarSystem();

    void Update(float dt, Vector3 playerPos, Vector3 playerCamFwd, bool playerInShop);
    void Draw(float timeVal);

    GrethnarState GetState() const { return m_state; }
    float GetJumpscareShake() const { return m_jumpscareShake; }
    float GetJumpscareFov() const { return m_jumpscareFov; }
    float GetJumpscareTimer() const { return m_jumpscareTimer; }

private:
    GrethnarState m_state = GRETHNAR_NORMAL;
    float m_stareTimer = 0.0f;
    float m_eyeScale = 1.0f;
    float m_bloodIntensity = 0.0f;
    bool m_seenEmptyCounter = false;
    float m_jumpscareTimer = 0.0f;
    float m_jumpscareShake = 0.0f;
    float m_jumpscareFov = 60.0f;
    float m_bloodSpawnTimer = 0.0f;

    std::vector<GrethnarEyeBloodDrop> m_bloodDrops;

    // Constants
    const Vector3 m_headPos = { 104.5f, 12.2f, 131.8f };
    const Vector3 m_npcPos = { 104.5f, 10.0f, 131.8f };
};
