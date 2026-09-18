#include "systems/grethnar_system.h"
#include "raymath.h"
#include <algorithm>

GrethnarSystem::GrethnarSystem() {}
GrethnarSystem::~GrethnarSystem() {}

void GrethnarSystem::Update(float dt, Vector3 playerPos, Vector3 playerCamFwd, bool playerInShop) {
    Vector3 toGrethnar = Vector3Normalize(Vector3Subtract(m_headPos, playerPos));
    float lookDot = Vector3DotProduct(playerCamFwd, toGrethnar);
    float distToGrethnar = Vector3Distance(playerPos, m_headPos);

    bool isLookingAtGrethnar = (playerInShop && distToGrethnar < 11.0f && lookDot > 0.88f);
    bool isLookingAwayFromCounter = (lookDot < 0.35f);

    if (m_state == GRETHNAR_NORMAL) {
        if (isLookingAtGrethnar) {
            m_state = GRETHNAR_STARING;
        }
    } else if (m_state == GRETHNAR_STARING) {
        if (isLookingAtGrethnar) {
            m_stareTimer += dt;

            if (m_stareTimer >= 5.0f) {
                float bloodProg = Clamp((m_stareTimer - 5.0f) / 1.0f, 0.0f, 1.0f);
                m_eyeScale = 1.0f + bloodProg * 0.18f;
                m_bloodIntensity = bloodProg;

                m_bloodSpawnTimer += dt;
                float spawnRate = 0.14f - bloodProg * 0.08f;
                if (m_bloodSpawnTimer >= spawnRate) {
                    m_bloodSpawnTimer = 0.0f;
                    float side = (GetRandomValue(0, 1) == 0) ? -0.09f : 0.09f;
                    GrethnarEyeBloodDrop bdrop;
                    bdrop.pos = { m_headPos.x + side + (float)GetRandomValue(-12, 12) / 1000.0f, 12.18f, m_headPos.z + (float)GetRandomValue(-8, 8) / 1000.0f };
                    bdrop.prevPos = bdrop.pos;
                    bdrop.vel = { (float)GetRandomValue(-15, 15) / 1000.0f, -0.45f, (float)GetRandomValue(5, 20) / 1000.0f };
                    bdrop.life = 3.5f;
                    bdrop.maxLife = 3.5f;
                    bdrop.scale = 0.20f + (float)GetRandomValue(0, 6) / 100.0f;
                    m_bloodDrops.push_back(bdrop);
                }
            }
        } else {
            m_stareTimer = fmaxf(0.0f, m_stareTimer - dt * 2.5f);
            m_eyeScale = 1.0f;
            m_bloodIntensity = 0.0f;
            if (m_stareTimer <= 0.05f) {
                m_state = GRETHNAR_NORMAL;
            }
        }

        if (m_stareTimer >= 6.0f) {
            m_state = GRETHNAR_PRIMED;
        }
    } else if (m_state == GRETHNAR_PRIMED) {
        if (isLookingAwayFromCounter) {
            m_state = GRETHNAR_VANISHED;
            m_seenEmptyCounter = false;
        }
    } else if (m_state == GRETHNAR_VANISHED) {
        if (lookDot > 0.55f) {
            m_seenEmptyCounter = true;
        }
        bool turnedAround = (m_seenEmptyCounter && lookDot < 0.15f) || (lookDot < -0.22f);
        if (turnedAround && playerInShop) {
            m_state = GRETHNAR_JUMPSCARE;
            m_jumpscareTimer = 0.45f;
            m_jumpscareShake = 0.85f;
            PlaySound(g_sndJumpscare);
        }
    } else if (m_state == GRETHNAR_JUMPSCARE) {
        m_jumpscareTimer -= dt;
        m_jumpscareShake = Lerp(m_jumpscareShake, 0.0f, dt * 6.0f);
        if (m_jumpscareTimer <= 0.0f) {
            m_state = GRETHNAR_NORMAL;
            m_stareTimer = 0.0f;
            m_eyeScale = 1.0f;
            m_bloodIntensity = 0.0f;
            m_seenEmptyCounter = false;
            m_jumpscareFov = 60.0f;
        }
    } else if (m_state == GRETHNAR_COOLDOWN) {
        m_state = GRETHNAR_NORMAL;
    }

    // Update blood drops
    for (size_t i = 0; i < m_bloodDrops.size(); ) {
        GrethnarEyeBloodDrop& bd = m_bloodDrops[i];
        bd.life -= dt;
        if (bd.life <= 0.0f) {
            m_bloodDrops[i] = m_bloodDrops.back();
            m_bloodDrops.pop_back();
            continue;
        }
        bd.prevPos = bd.pos;
        bd.pos = Vector3Add(bd.pos, Vector3Scale(bd.vel, dt));
        bd.vel.y -= 14.0f * dt;
        bd.vel.x *= (1.0f - 0.08f * dt);
        bd.vel.z *= (1.0f - 0.08f * dt);

        if (bd.pos.y <= 10.026f) {
            bd.pos.y = 10.026f;
            bd.vel = { 0, 0, 0 };
        }
        i++;
    }
}

void GrethnarSystem::Draw(float timeVal) {
    if (m_state == GRETHNAR_VANISHED || m_state == GRETHNAR_JUMPSCARE) return;

    rlPushMatrix();
    rlTranslatef(m_npcPos.x, m_npcPos.y, m_npcPos.z);

    // Draw the NPC base
    DrawCylinder({0,0,0}, 0.22f, 0.27f, 1.95f, 14, ApplyShopLighting(m_npcPos, { 16, 16, 18, 255 }));

    // Draw eyes
    float curEyeRad = 0.042f * m_eyeScale;
    bool isBloody = (m_bloodIntensity > 0.01f);

    Color eyeballCol = isBloody ? Color{ 255, 18, 22, 255 } : Color{ 225, 225, 220, 255 };
    Color pupilCol   = isBloody ? Color{ 20, 0, 0, 255 }    : Color{ 22, 24, 28, 255 };

    if (isBloody) {
        float throb = sinf(timeVal * 16.0f) * 0.15f + 0.85f;
        Color engorgedSclera = { (unsigned char)(145 * throb), 8, 12, 255 };
        Color veinCol = { (unsigned char)(85 * throb), 4, 6, 255 };

        DrawSphere({ -0.09f, 0.27f, 0.222f }, curEyeRad * 1.08f, engorgedSclera);
        DrawSphere({  0.09f, 0.27f, 0.222f }, curEyeRad * 1.08f, engorgedSclera);
        DrawSphereWires({ -0.09f, 0.27f, 0.222f }, curEyeRad * 1.10f, 8, 8, veinCol);
        DrawSphereWires({  0.09f, 0.27f, 0.222f }, curEyeRad * 1.10f, 8, 8, veinCol);

        for (int v = 0; v < 6; v++) {
            float ang = v * 60.0f * DEG2RAD;
            float vx = cosf(ang) * curEyeRad * 1.10f;
            float vy = sinf(ang) * curEyeRad * 1.10f;
            DrawLine3D({ -0.09f, 0.27f, 0.22f }, { -0.09f + vx, 0.27f + vy, 0.222f }, veinCol);
            DrawLine3D({  0.09f, 0.27f, 0.22f }, {  0.09f + vx, 0.27f + vy, 0.222f }, veinCol);
        }
    }

    DrawSphere({ -0.09f, 0.27f, 0.222f }, curEyeRad, eyeballCol);
    DrawSphere({  0.09f, 0.27f, 0.222f }, curEyeRad, eyeballCol);
    DrawSphere({ -0.09f, 0.27f, 0.222f + curEyeRad * 0.72f }, curEyeRad * 0.38f, pupilCol);
    DrawSphere({  0.09f, 0.27f, 0.222f + curEyeRad * 0.72f }, curEyeRad * 0.38f, pupilCol);

    if (isBloody) {
        float blLen = m_bloodIntensity * 0.16f;
        Color bCol = { 135, 8, 14, 255 };
        DrawCylinderEx({ -0.09f, 0.27f, 0.235f }, { -0.09f, 0.27f - blLen, 0.230f }, 0.012f * m_bloodIntensity, 0.006f, 6, bCol);
        DrawCylinderEx({  0.09f, 0.27f, 0.235f }, {  0.09f, 0.27f - blLen, 0.230f }, 0.012f * m_bloodIntensity, 0.006f, 6, bCol);
    }

    rlPopMatrix();

    // Draw blood drops (world space)
    for (const auto& bd : m_bloodDrops) {
        float a = Clamp(bd.life / 0.5f, 0.0f, 1.0f);
        if (bd.pos.y > 10.035f) {
            DrawLine3D(bd.prevPos, bd.pos, { 145, 6, 12, (unsigned char)(220 * a) });
        } else {
            DrawCube({ bd.pos.x, 10.022f, bd.pos.z }, 0.075f, 0.002f, 0.075f, { 115, 4, 8, (unsigned char)(200 * a) });
        }
    }
}
