#include "atmospheric_particles.h"
#include "shop_lighting.h"
#include "procedural_math.inl"
#include "../core/game_types.h"
#include <rlgl.h>
#include <cmath>
#include <algorithm>

std::vector<ShopProduct> g_shopProducts;
std::vector<ShopParticle> g_shopParticles;
std::vector<DustParticle> g_dustParticles;
std::vector<FootstepTrail> g_footstepTrails;
std::vector<GunSpark> g_gunSparks;
std::vector<ShellCasing> g_shellCasings;

float g_gunRecoilTimer = 0.0f;
float g_gunMuzzleFlashTimer = 0.0f;
bool  g_isLeftFootStep = false;

extern Model   g_bloodLiquidModel;
extern Model   g_milkLiquidModel;
extern Model   g_breadModel;
extern Vector3 g_cartPos;
extern float   g_cartWheelSpin;
extern float   g_cartYaw;
extern int     g_heldProductIndex;
extern float   g_vmSwayX;
extern float   g_vmSwayY;

// ----------------------------------------------------------------------

// ATMOSPHERIC VOLUMETRIC DUST & FOOTSTEP TRAILS SYSTEM (ZERO-HEAP FRAGMENTATION)

// ----------------------------------------------------------------------

void SpawnPickupDust(Vector3 center, int count)

{

    int spawnCount = count;

    if (g_dustParticles.size() + spawnCount > 64) {

        int overflow = (int)(g_dustParticles.size() + spawnCount) - 64;

        if (overflow > (int)g_dustParticles.size()) overflow = (int)g_dustParticles.size();

        g_dustParticles.erase(g_dustParticles.begin(), g_dustParticles.begin() + overflow);

    }

    for (int i = 0; i < spawnCount; i++) {

        DustParticle p;

        float r = Frand(0.02f, 0.18f);

        float a = Frand(0.0f, 2.0f * PI);

        p.pos = Vector3{ center.x + cosf(a) * r, center.y + Frand(-0.01f, 0.04f), center.z + sinf(a) * r };

        p.vel = Vector3{ cosf(a) * Frand(0.06f, 0.28f), Frand(0.12f, 0.38f), sinf(a) * Frand(0.06f, 0.28f) };

        p.size = Frand(0.008f, 0.018f);

        p.maxLife = Frand(1.4f, 2.4f);

        p.life = p.maxLife;

        unsigned char shade = (unsigned char)GetRandomValue(195, 240);

        p.color = Color{ shade, (unsigned char)(shade * 0.96f), (unsigned char)(shade * 0.88f), 220 };

        p.spin = Frand(0.0f, 360.0f);

        p.spinSpeed = Frand(-120.0f, 120.0f);

        g_dustParticles.push_back(p);

    }

}



void UpdateDustParticles(float dt)

{

    for (size_t i = 0; i < g_dustParticles.size(); ) {

        DustParticle &p = g_dustParticles[i];

        p.life -= dt;

        if (p.life <= 0.0f) {

            // O(1) swap-and-pop: zero memory shifting or heap churn

            g_dustParticles[i] = g_dustParticles.back();

            g_dustParticles.pop_back();

            continue;

        }

        p.vel.x *= (1.0f - 1.5f * dt);

        p.vel.z *= (1.0f - 1.5f * dt);

        p.vel.y -= 0.06f * dt;

        p.pos.x += p.vel.x * dt;

        p.pos.y += p.vel.y * dt;

        p.pos.z += p.vel.z * dt;

        p.spin += p.spinSpeed * dt;

        i++;

    }

}



// Single-batch hardware billboard rendering: replaces 300 DrawCube calls with 1 batch pass

void DrawDustParticles(const Camera3D &camera)

{

    if (g_dustParticles.empty()) return;

    Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwd, camera.up));

    Vector3 up  = Vector3CrossProduct(rgt, fwd);



    rlBegin(RL_QUADS);

    for (const auto &p : g_dustParticles) {

        float alphaNorm = Clamp(p.life / p.maxLife, 0.0f, 1.0f);

        unsigned char a = (unsigned char)(p.color.a * alphaNorm);

        if (a < 4) continue;

        Color c = ApplyShopLighting(p.pos, p.color);

        rlColor4ub(c.r, c.g, c.b, a);



        float hs = p.size * 0.85f;

        Vector3 rx = Vector3Scale(rgt, hs);

        Vector3 uy = Vector3Scale(up, hs);



        rlVertex3f(p.pos.x - rx.x - uy.x, p.pos.y - rx.y - uy.y, p.pos.z - rx.z - uy.z);

        rlVertex3f(p.pos.x - rx.x + uy.x, p.pos.y - rx.y + uy.y, p.pos.z - rx.z - uy.z);

        rlVertex3f(p.pos.x + rx.x + uy.x, p.pos.y + rx.y + uy.y, p.pos.z + rx.z + uy.z);

        rlVertex3f(p.pos.x + rx.x - uy.x, p.pos.y + rx.y - uy.y, p.pos.z + rx.z - uy.z);

    }

    rlEnd();

}



void AddFootstepTrail(Vector3 pos, float yaw, bool isLeft)

{

    if (g_footstepTrails.size() >= 32) {

        g_footstepTrails.erase(g_footstepTrails.begin());

    }

    FootstepTrail t;

    t.pos = pos;

    t.yaw = yaw;

    t.maxLife = 10.0f;

    t.life = t.maxLife;

    t.isLeft = isLeft;

    g_footstepTrails.push_back(t);

}



void UpdateFootstepTrails(float dt)

{

    for (size_t i = 0; i < g_footstepTrails.size(); ) {

        g_footstepTrails[i].life -= dt;

        if (g_footstepTrails[i].life <= 0.0f) {

            // O(1) swap-and-pop

            g_footstepTrails[i] = g_footstepTrails.back();

            g_footstepTrails.pop_back();

        } else {

            i++;

        }

    }

}



void DrawFootstepTrails()

{

    if (g_footstepTrails.empty()) return;

    rlBegin(RL_QUADS);

    rlNormal3f(0.0f, 1.0f, 0.0f);

    for (const auto &t : g_footstepTrails) {

        float alphaNorm = Clamp(t.life / t.maxLife, 0.0f, 1.0f);

        unsigned char a = (unsigned char)(115.0f * alphaNorm);

        if (a < 2) continue;

        Color col = ApplyShopLighting(t.pos, Color{ 16, 18, 22, a });

        rlColor4ub(col.r, col.g, col.b, a);



        float rad = t.yaw * DEG2RAD;

        float fx = sinf(rad), fz = cosf(rad);

        float rx = cosf(rad), rz = -sinf(rad);

        float y = 10.021f;



        // Front sole pad

        float fcx = t.pos.x + fx * 0.055f;

        float fcz = t.pos.z + fz * 0.055f;

        float sw = 0.046f, sl = 0.070f;

        rlVertex3f(fcx - rx * sw - fx * sl, y, fcz - rz * sw - fz * sl);

        rlVertex3f(fcx - rx * sw + fx * sl, y, fcz - rz * sw + fz * sl);

        rlVertex3f(fcx + rx * sw + fx * sl, y, fcz + rz * sw + fz * sl);

        rlVertex3f(fcx + rx * sw - fx * sl, y, fcz + rz * sw - fz * sl);



        // Rear heel pad

        float hcx = t.pos.x - fx * 0.065f;

        float hcz = t.pos.z - fz * 0.065f;

        float hw = 0.040f, hl = 0.038f;

        rlVertex3f(hcx - rx * hw - fx * hl, y, hcz - rz * hw - fz * hl);

        rlVertex3f(hcx - rx * hw + fx * hl, y, hcz - rz * hw + fz * hl);

        rlVertex3f(hcx + rx * hw + fx * hl, y, hcz + rz * hw + fz * hl);

        rlVertex3f(hcx + rx * hw - fx * hl, y, hcz + rz * hw - fz * hl);

    }

    rlEnd();

}



void UpdateShellCasingsAndSparks(float dt)

{

    for (size_t i = 0; i < g_shellCasings.size(); ) {

        ShellCasing &sc = g_shellCasings[i];

        sc.life -= dt;

        if (sc.life <= 0.0f) {

            g_shellCasings[i] = g_shellCasings.back();

            g_shellCasings.pop_back();

            continue;

        }

        if (!sc.landed) {

            sc.vel.y -= 9.8f * dt;

            sc.pos.x += sc.vel.x * dt;

            sc.pos.y += sc.vel.y * dt;

            sc.pos.z += sc.vel.z * dt;

            sc.rot.x += sc.rotVel.x * dt;

            sc.rot.y += sc.rotVel.y * dt;

            sc.rot.z += sc.rotVel.z * dt;

            if (sc.pos.y <= 10.024f) {

                sc.pos.y = 10.024f;

                sc.vel.y *= -0.28f;

                sc.vel.x *= 0.55f;

                sc.vel.z *= 0.55f;

                sc.rotVel = Vector3Scale(sc.rotVel, 0.35f);

                if (fabsf(sc.vel.y) < 0.25f) {

                    sc.landed = true;

                    sc.vel = Vector3{0, 0, 0};

                }

            }

        }

        i++;

    }



    for (size_t i = 0; i < g_gunSparks.size(); ) {

        GunSpark &gs = g_gunSparks[i];

        gs.life -= dt;

        if (gs.life <= 0.0f) {

            g_gunSparks[i] = g_gunSparks.back();

            g_gunSparks.pop_back();

            continue;

        }

        gs.vel.y -= 9.8f * dt;

        gs.pos.x += gs.vel.x * dt;

        gs.pos.y += gs.vel.y * dt;

        gs.pos.z += gs.vel.z * dt;

        i++;

    }

}



void DrawShellCasingsAndSparks()

{

    for (const auto &sc : g_shellCasings) {

        rlPushMatrix();

            rlTranslatef(sc.pos.x, sc.pos.y, sc.pos.z);

            rlRotatef(sc.rot.y, 0, 1, 0);

            rlRotatef(sc.rot.x, 1, 0, 0);

            rlRotatef(sc.rot.z, 0, 0, 1);

            Color brass = ApplyShopLighting(sc.pos, Color{ 225, 185, 65, 255 });

            DrawCylinder(Vector3{0,0,0}, 0.005f, 0.005f, 0.019f, 6, brass);

        rlPopMatrix();

    }

    for (const auto &gs : g_gunSparks) {

        DrawSphere(gs.pos, 0.012f, gs.color);

        DrawSphere(gs.pos, 0.024f, Fade(gs.color, 0.4f));

    }

}



void DrawChocolateBar(Vector3 pos, int subType, bool opened, bool held, Vector3 fwdDir, float distSq)

{

    Color wrapCol;

    Color foilCol;

    Color accentCol;

    if (subType == 0) { // Dark Noir 85% Cacao

        wrapCol   = ApplyShopLighting(pos, Color{ 24, 24, 26, 255 });

        foilCol   = ApplyShopLighting(pos, Color{ 235, 195, 75, 255 }); // Gold foil

        accentCol = Color{ 245, 215, 110, 255 };

    } else if (subType == 1) { // Alpine Milk Chocolate

        wrapCol   = ApplyShopLighting(pos, Color{ 28, 65, 155, 255 }); // Royal Blue

        foilCol   = ApplyShopLighting(pos, Color{ 215, 222, 230, 255 }); // Silver foil

        accentCol = Color{ 240, 245, 255, 255 };

    } else { // Sea Salt Caramel

        wrapCol   = ApplyShopLighting(pos, Color{ 180, 95, 32, 255 }); // Amber bronze

        foilCol   = ApplyShopLighting(pos, Color{ 225, 165, 95, 255 }); // Copper foil

        accentCol = Color{ 255, 225, 160, 255 };

    }



    float w = 0.088f;  // Width

    float h = 0.016f;  // Height/thickness

    float l = 0.180f;  // Length



    // Fast distant LOD when observing the whole cooler: single clean slab (1 draw call)

    if (!held && distSq > 5.5f * 5.5f) {

        DrawCube(pos, w, h, l, wrapCol);

        return;

    }



    rlPushMatrix();

    rlTranslatef(pos.x, pos.y, pos.z);



    if (held) {

        Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwdDir, Vector3{0, 1, 0}));

        rlRotatef(25.0f, rgt.x, rgt.y, rgt.z);

        rlRotatef(-15.0f, 0, 1, 0);

    }



    Color chocDark = ApplyShopLighting(pos, Color{ 52, 28, 18, 255 });



    // Inner chocolate bar body (dark rich chocolate)

    DrawCube(Vector3{ 0, 0, 0 }, w, h, l, chocDark);



    if (opened) {

        // Exposed breakable chocolate grid on top half (Z > 0)

        for (int row = 0; row < 2; row++) {

            for (int col = 0; col < 2; col++) {

                float sx = -w * 0.25f + col * (w * 0.5f);

                float sz = 0.020f + row * 0.045f;

                DrawCube(Vector3{ sx, h * 0.52f, sz }, w * 0.40f, 0.004f, 0.038f, chocDark);

                DrawCubeWires(Vector3{ sx, h * 0.52f, sz }, w * 0.40f, 0.004f, 0.038f, ApplyShopLighting(pos, Color{ 35, 18, 10, 255 }));

            }

        }

        // Crinkled peeled foil boundary

        DrawCube(Vector3{ 0, 0.002f, -0.005f }, w * 1.03f, h * 1.06f, 0.015f, foilCol);

        // Bottom sleeve wrapper (Z <= 0)

        DrawCube(Vector3{ 0, 0, -l * 0.25f }, w * 1.02f, h * 1.04f, l * 0.50f, wrapCol);

        // Gold/silver brand accent band

        DrawCube(Vector3{ 0, 0, -l * 0.25f }, w * 1.025f, h * 1.05f, 0.035f, foilCol);

    } else {

        // Fully wrapped bar

        DrawCube(Vector3{ 0, 0, 0 }, w * 1.02f, h * 1.04f, l * 0.94f, wrapCol);

        // Shiny foil ends peeking out

        DrawCube(Vector3{ 0, 0,  l * 0.49f }, w * 0.98f, h * 0.90f, 0.025f, foilCol);

        DrawCube(Vector3{ 0, 0, -l * 0.49f }, w * 0.98f, h * 0.90f, 0.025f, foilCol);

        // Center printed label band & brand accent stripe

        DrawCube(Vector3{ 0, 0, 0 }, w * 1.025f, h * 1.05f, 0.070f, accentCol);

        DrawCube(Vector3{ 0, 0, 0 }, w * 1.030f, h * 1.06f, 0.045f, wrapCol);

    }



    rlPopMatrix();

}



void DrawGunWorld(Vector3 pos, bool held)

{

    if (held) return;



    // Luxurious executive presentation velvet tray on checkout counter

    Vector3 trayPos = { pos.x, pos.y - 0.03f, pos.z };

    Color velvetDark = ApplyShopLighting(trayPos, Color{ 75, 12, 20, 255 });

    Color brassTrim  = ApplyShopLighting(trayPos, Color{ 175, 140, 60, 255 });

    DrawCube(trayPos, 0.46f, 0.022f, 0.32f, velvetDark);

    DrawCubeWires(trayPos, 0.462f, 0.024f, 0.322f, brassTrim);

    DrawCube(Vector3{ trayPos.x, trayPos.y + 0.012f, trayPos.z - 0.15f }, 0.46f, 0.015f, 0.02f, brassTrim);

    DrawCube(Vector3{ trayPos.x, trayPos.y + 0.012f, trayPos.z + 0.15f }, 0.46f, 0.015f, 0.02f, brassTrim);

    DrawCube(Vector3{ trayPos.x - 0.22f, trayPos.y + 0.012f, trayPos.z }, 0.02f, 0.015f, 0.32f, brassTrim);

    DrawCube(Vector3{ trayPos.x + 0.22f, trayPos.y + 0.012f, trayPos.z }, 0.02f, 0.015f, 0.32f, brassTrim);



    // Box of 9mm Ammunition beside the gun

    Vector3 boxPos = { pos.x - 0.13f, pos.y - 0.005f, pos.z + 0.06f };

    Color ammoBoxCol = ApplyShopLighting(boxPos, Color{ 42, 65, 45, 255 });

    DrawCube(boxPos, 0.10f, 0.045f, 0.075f, ammoBoxCol);

    DrawCubeWires(boxPos, 0.102f, 0.046f, 0.076f, ApplyShopLighting(boxPos, Color{ 180, 195, 120, 255 }));

    // Loose brass cartridges resting on tray

    Color brass = ApplyShopLighting(boxPos, Color{ 225, 185, 65, 255 });

    DrawCylinderEx(Vector3{ pos.x - 0.12f, pos.y - 0.015f, pos.z - 0.06f }, Vector3{ pos.x - 0.10f, pos.y - 0.015f, pos.z - 0.06f }, 0.005f, 0.005f, 6, brass);

    DrawCylinderEx(Vector3{ pos.x - 0.12f, pos.y - 0.015f, pos.z - 0.04f }, Vector3{ pos.x - 0.10f, pos.y - 0.015f, pos.z - 0.04f }, 0.005f, 0.005f, 6, brass);



    // The Pistol Model resting on the tray

    rlPushMatrix();

    rlTranslatef(pos.x + 0.05f, pos.y, pos.z);

    rlRotatef(-25.0f, 0, 1, 0); // Angled presentation

    rlRotatef(90.0f, 0, 0, 1);  // Resting on its side on the velvet

    rlScalef(0.85f, 0.85f, 0.85f);



    Color frameCol = ApplyShopLighting(pos, Color{ 30, 32, 35, 255 });

    Color slideCol = ApplyShopLighting(pos, Color{ 44, 46, 50, 255 });

    Color metalCol = ApplyShopLighting(pos, Color{ 160, 165, 170, 255 });

    Color sightDot = Color{ 90, 255, 100, 255 };



    // Slide

    DrawCube(Vector3{ 0, 0.038f, -0.04f }, 0.030f, 0.034f, 0.180f, slideCol);

    DrawCubeWires(Vector3{ 0, 0.038f, -0.04f }, 0.031f, 0.035f, 0.181f, ApplyShopLighting(pos, Fade(BLACK, 0.4f)));

    // Barrel chamber

    DrawCube(Vector3{ 0.008f, 0.040f, -0.02f }, 0.016f, 0.018f, 0.040f, metalCol);

    // Sights

    DrawCube(Vector3{ 0, 0.058f, -0.12f }, 0.008f, 0.010f, 0.012f, slideCol); // Front sight

    DrawSphere(Vector3{ 0, 0.060f, -0.12f }, 0.003f, sightDot);

    DrawCube(Vector3{ 0, 0.058f, 0.045f }, 0.020f, 0.010f, 0.012f, slideCol); // Rear sight

    // Frame & Grip

    DrawCube(Vector3{ 0, 0.018f, -0.04f }, 0.028f, 0.016f, 0.170f, frameCol); // Picatinny frame

    rlPushMatrix();

        rlTranslatef(0, -0.035f, 0.020f);

        rlRotatef(-16.0f, 1, 0, 0); // Grip rake angle

        DrawCube(Vector3{ 0, 0, 0 }, 0.027f, 0.105f, 0.048f, frameCol);

        // Stippled grip panels

        DrawCube(Vector3{ 0, 0, 0 }, 0.029f, 0.080f, 0.036f, ApplyShopLighting(pos, Color{ 20, 20, 22, 255 }));

    rlPopMatrix();

    // Trigger guard & skeleton trigger

    DrawCylinderEx(Vector3{ 0, 0.010f, -0.010f }, Vector3{ 0, -0.025f, -0.010f }, 0.004f, 0.004f, 6, frameCol);

    DrawCylinderEx(Vector3{ 0, -0.025f, -0.010f }, Vector3{ 0, -0.020f, 0.022f }, 0.004f, 0.004f, 6, frameCol);

    DrawCube(Vector3{ 0, -0.008f, 0.006f }, 0.006f, 0.018f, 0.008f, metalCol);



    rlPopMatrix();

}



void DrawGunViewModel(const Camera3D &camera, float walkTime, float bobAmplitude, float dt)

{

    (void)dt;

    Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwd, camera.up));

    Vector3 up  = Vector3CrossProduct(rgt, fwd);



    // Standing Idle Breathing Animation

    float t = (float)GetTime();

    float breathY = sinf(t * 1.8f) * 0.0035f;

    float breathX = cosf(t * 0.9f) * 0.0020f;

    float breathPitch = sinf(t * 1.8f) * 0.6f;

    float breathRoll  = cosf(t * 1.1f) * 0.4f;



    // Walking Idle Sway & Bobbing Animation

    float walkSwayX = sinf(walkTime * 0.5f) * 0.018f * bobAmplitude;

    float walkDipY  = (cosf(walkTime) - 0.5f) * 0.014f * bobAmplitude;

    float walkRoll  = sinf(walkTime * 0.5f) * 2.4f * bobAmplitude;

    float walkYaw   = -cosf(walkTime * 0.5f) * 1.8f * bobAmplitude;



    // Shooting Recoil & Blowback Animation

    float recNorm = Clamp(g_gunRecoilTimer / 0.16f, 0.0f, 1.0f);

    float recCurve = powf(recNorm, 0.65f);

    float recPitch = recCurve * 14.5f;

    float recBack  = recCurve * 0.034f;

    float recUp    = recCurve * 0.012f;

    float slideBlowback = (recNorm > 0.28f) ? (0.038f * (recNorm - 0.28f) / 0.72f) : 0.0f;



    // Hand origin position with inertial sway and locomotion lag

    Vector3 gunPos = Vector3Add(camera.position, Vector3Scale(fwd, 0.44f - recBack));

    gunPos = Vector3Add(gunPos, Vector3Scale(rgt, 0.17f + breathX + walkSwayX + g_vmSwayX));

    gunPos = Vector3Subtract(gunPos, Vector3Scale(up, 0.14f - (breathY + walkDipY + recUp - g_vmSwayY)));



    rlPushMatrix();

    rlTranslatef(gunPos.x, gunPos.y, gunPos.z);



    // Orientation aligned to camera look direction

    float yawDeg = atan2f(fwd.x, fwd.z) * RAD2DEG;

    float pitchDeg = asinf(-fwd.y) * RAD2DEG;

    rlRotatef(yawDeg, 0, 1, 0);

    rlRotatef(pitchDeg, 1, 0, 0);



    // Viewmodel inertial angular lag when turning

    float vmRotYaw   = g_vmSwayX * 125.0f;

    float vmRotPitch = -g_vmSwayY * 105.0f;

    float vmRotRoll  = g_vmSwayX * 85.0f;



    // Weapon Animation rotations:

    rlRotatef(walkYaw + vmRotYaw, 0, 1, 0);

    rlRotatef(-(breathPitch + recPitch + vmRotPitch), 1, 0, 0); // Muzzle kicks up with inertia

    rlRotatef(breathRoll + walkRoll + vmRotRoll, 0, 0, 1);



    Color frameCol = ApplyShopLighting(gunPos, Color{ 28, 30, 34, 255 });

    Color slideCol = ApplyShopLighting(gunPos, Color{ 46, 48, 52, 255 });

    Color metalCol = ApplyShopLighting(gunPos, Color{ 175, 180, 185, 255 });

    Color sightDot = Color{ 80, 255, 95, 255 };



    // Slide (reciprocating backward by slideBlowback)

    rlPushMatrix();

        rlTranslatef(0, 0.038f, -0.04f + slideBlowback);

        DrawCube(Vector3{ 0, 0, 0 }, 0.030f, 0.034f, 0.180f, slideCol);

        DrawCubeWires(Vector3{ 0, 0, 0 }, 0.031f, 0.035f, 0.181f, ApplyShopLighting(gunPos, Fade(BLACK, 0.45f)));



        // Rear cocking serrations

        for (int s = 0; s < 5; s++) {

            float sz = 0.055f + s * 0.007f;

            DrawLine3D(Vector3{ -0.0155f, -0.012f, sz }, Vector3{ -0.0155f, 0.012f, sz }, ApplyShopLighting(gunPos, Color{ 18, 18, 20, 255 }));

            DrawLine3D(Vector3{  0.0155f, -0.012f, sz }, Vector3{  0.0155f, 0.012f, sz }, ApplyShopLighting(gunPos, Color{ 18, 18, 20, 255 }));

        }



        // High-contrast tactical sights

        DrawCube(Vector3{ 0, 0.021f, -0.080f }, 0.007f, 0.009f, 0.012f, slideCol);

        DrawSphere(Vector3{ 0, 0.021f, -0.074f }, 0.0028f, sightDot);

        DrawCube(Vector3{ -0.008f, 0.021f, 0.082f }, 0.007f, 0.009f, 0.010f, slideCol);

        DrawCube(Vector3{  0.008f, 0.021f, 0.082f }, 0.007f, 0.009f, 0.010f, slideCol);

        DrawSphere(Vector3{ -0.008f, 0.021f, 0.082f }, 0.0025f, sightDot);

        DrawSphere(Vector3{  0.008f, 0.021f, 0.082f }, 0.0025f, sightDot);

    rlPopMatrix();



    // Fixed Barrel & Chamber (exposed when slide cycles back!)

    DrawCube(Vector3{ 0, 0.038f, -0.020f }, 0.022f, 0.022f, 0.050f, metalCol);

    DrawCylinderEx(Vector3{ 0, 0.038f, -0.020f }, Vector3{ 0, 0.038f, -0.145f }, 0.008f, 0.008f, 10, metalCol);

    DrawCylinder(Vector3{ 0, 0.038f, -0.146f }, 0.005f, 0.005f, 0.002f, 8, BLACK);



    // Frame, Picatinny accessory rail & trigger guard

    DrawCube(Vector3{ 0, 0.018f, -0.04f }, 0.028f, 0.016f, 0.170f, frameCol);

    for (int r = 0; r < 3; r++) {

        DrawCube(Vector3{ 0, 0.008f, -0.085f - r * 0.016f }, 0.029f, 0.004f, 0.008f, ApplyShopLighting(gunPos, Color{ 20, 20, 22, 255 }));

    }



    // Ergonomic stippled pistol grip

    rlPushMatrix();

        rlTranslatef(0, -0.035f, 0.020f);

        rlRotatef(-16.0f, 1, 0, 0);

        DrawCube(Vector3{ 0, 0, 0 }, 0.027f, 0.105f, 0.048f, frameCol);

        DrawCube(Vector3{ 0, 0, 0 }, 0.029f, 0.080f, 0.036f, ApplyShopLighting(gunPos, Color{ 18, 18, 20, 255 }));

        DrawCube(Vector3{ 0, -0.054f, 0.004f }, 0.031f, 0.012f, 0.054f, ApplyShopLighting(gunPos, Color{ 36, 38, 42, 255 }));

    rlPopMatrix();



    // Trigger guard and trigger

    DrawCylinderEx(Vector3{ 0, 0.010f, -0.010f }, Vector3{ 0, -0.025f, -0.010f }, 0.004f, 0.004f, 6, frameCol);

    DrawCylinderEx(Vector3{ 0, -0.025f, -0.010f }, Vector3{ 0, -0.020f, 0.022f }, 0.004f, 0.004f, 6, frameCol);

    DrawCube(Vector3{ 0, -0.008f, 0.006f }, 0.006f, 0.018f, 0.008f, metalCol);



    // Muzzle Flash Effect!

    if (g_gunMuzzleFlashTimer > 0.0f) {

        Vector3 mPos = { 0, 0.038f, -0.150f };

        DrawLine3D(Vector3{ mPos.x - 0.07f, mPos.y, mPos.z }, Vector3{ mPos.x + 0.07f, mPos.y, mPos.z }, Color{ 255, 235, 160, 255 });

        DrawLine3D(Vector3{ mPos.x, mPos.y - 0.07f, mPos.z }, Vector3{ mPos.x, mPos.y + 0.07f, mPos.z }, Color{ 255, 235, 160, 255 });

        DrawLine3D(Vector3{ mPos.x - 0.05f, mPos.y - 0.05f, mPos.z }, Vector3{ mPos.x + 0.05f, mPos.y + 0.05f, mPos.z }, Color{ 255, 200, 100, 255 });

        DrawLine3D(Vector3{ mPos.x - 0.05f, mPos.y + 0.05f, mPos.z }, Vector3{ mPos.x + 0.05f, mPos.y - 0.05f, mPos.z }, Color{ 255, 200, 100, 255 });

        DrawSphere(mPos, 0.032f, Color{ 255, 255, 240, 255 });

        DrawSphere(mPos, 0.085f, Color{ 255, 185, 45, 180 });

        DrawSphere(mPos, 0.220f, Color{ 255, 130, 20, 75 });

    }



    rlPopMatrix();

}



void DrawHorizontalRefrigerator(Vector3 center, const Camera3D &camera)

{

    // Center at X = 94.5, Y = 10.015, Z = 133.5

    // Footprint: Length X = 3.60m, Width Z = 1.28m, Height Y = 0.94m

    float dx = center.x - camera.position.x;

    float dy = (center.y + 0.5f) - camera.position.y;

    float dz = center.z - camera.position.z;

    float distSq = dx * dx + dy * dy + dz * dz;



    // 1. Distance culling (skip completely beyond 32 meters)

    if (distSq > 32.0f * 32.0f) return;



    // 2. Frustum culling (if > 3.0m away, skip if behind camera)

    if (distSq > 3.0f * 3.0f) {

        Vector3 fwdDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

        Vector3 toObj = Vector3Normalize(Vector3{ dx, dy, dz });

        if (Vector3DotProduct(fwdDir, toObj) < -0.25f) return;

    }



    // 1. Recessed dark matte compressor kickplate base

    Vector3 basePos = { center.x, 10.09f, center.z };

    Color kickplateCol = ApplyShopLighting(basePos, Color{ 26, 28, 32, 255 });

    DrawCube(basePos, 3.44f, 0.15f, 1.16f, kickplateCol);



    // 2. Commercial Appliance White Powder-Coated Chest Body

    Vector3 bodyPos = { center.x, 10.53f, center.z };

    Color whiteEnamel = ApplyShopLighting(bodyPos, Color{ 238, 240, 244, 255 });

    DrawCube(bodyPos, 3.60f, 0.72f, 1.26f, whiteEnamel);



    // Heavy-duty perimeter cart bumper rubber rail at mid-height

    Vector3 bumperPos = { center.x, 10.52f, center.z };

    Color bumperRubber = ApplyShopLighting(bumperPos, Color{ 36, 38, 42, 255 });

    DrawCube(bumperPos, 3.65f, 0.065f, 1.31f, bumperRubber);



    // 3. Top Deck Stainless Steel Trim Frame (perimeter lip)

    Color stainless = ApplyShopLighting(Vector3{ center.x, 10.90f, center.z }, Color{ 215, 220, 226, 255 });

    DrawCube(Vector3{ center.x, 10.90f, center.z - 0.58f }, 3.62f, 0.035f, 0.11f, stainless);

    DrawCube(Vector3{ center.x, 10.90f, center.z + 0.58f }, 3.62f, 0.035f, 0.11f, stainless);

    DrawCube(Vector3{ center.x - 1.76f, 10.90f, center.z }, 0.12f, 0.035f, 1.28f, stainless);

    DrawCube(Vector3{ center.x + 1.76f, 10.90f, center.z }, 0.12f, 0.035f, 1.28f, stainless);



    // 4. Interior Refrigerated Compartment & Cold Arctic LED Glow

    Color ledStripCol = Color{ 175, 240, 255, 255 };

    DrawLine3D(Vector3{ center.x - 1.66f, 10.87f, center.z - 0.50f }, Vector3{ center.x + 1.66f, 10.87f, center.z - 0.50f }, ledStripCol);

    DrawLine3D(Vector3{ center.x - 1.66f, 10.87f, center.z + 0.50f }, Vector3{ center.x + 1.66f, 10.87f, center.z + 0.50f }, ledStripCol);

    DrawCube(Vector3{ center.x, 10.60f, center.z }, 3.32f, 0.44f, 0.98f, Color{ 160, 230, 255, 28 });



    // Dual Sliding Glass Lids (Translucent Cyan-Tinted Tempered Glass)

    Color glassTint = Color{ 190, 235, 255, 68 };

    DrawCube(Vector3{ center.x, 10.925f, center.z }, 3.44f, 0.012f, 1.06f, glassTint);



    // DISTANT LOD: If camera is more than 6.5m away, simplified clean model is sufficient

    if (distSq > 6.5f * 6.5f) return;



    // HIGH DETAIL LOD (within 6.5m):

    // Compressor ventilation louvers

    Color louverSlot = ApplyShopLighting(basePos, Color{ 12, 14, 16, 255 });

    for (int v = 0; v < 3; v++) {

        float vy = 10.06f + v * 0.030f;

        DrawCube(Vector3{ center.x - 0.90f, vy, center.z - 0.582f }, 0.85f, 0.012f, 0.015f, louverSlot);

        DrawCube(Vector3{ center.x + 0.90f, vy, center.z - 0.582f }, 0.85f, 0.012f, 0.015f, louverSlot);

    }



    // 4 Corner Protector Moldings

    float halfX = 1.80f, halfZ = 0.63f;

    Vector3 cCorners[4] = {

        { center.x - halfX, 10.53f, center.z - halfZ },

        { center.x + halfX, 10.53f, center.z - halfZ },

        { center.x - halfX, 10.53f, center.z + halfZ },

        { center.x + halfX, 10.53f, center.z + halfZ }

    };

    Color cornerCol = ApplyShopLighting(bodyPos, Color{ 55, 58, 65, 255 });

    for (int c = 0; c < 4; c++) {

        DrawCylinder(cCorners[c], 0.025f, 0.025f, 0.72f, 8, cornerCol);

    }



    // Chrome Wire Divider Baskets

    Color wireCol = ApplyShopLighting(Vector3{ center.x, 10.60f, center.z }, Color{ 200, 205, 215, 255 });

    const float dividersX[3] = { center.x - 0.85f, center.x, center.x + 0.85f };

    for (int d = 0; d < 3; d++) {

        DrawCube(Vector3{ dividersX[d], 10.60f, center.z }, 0.015f, 0.45f, 1.02f, Fade(wireCol, 0.65f));

        DrawCylinderEx(Vector3{ dividersX[d], 10.82f, center.z - 0.51f }, Vector3{ dividersX[d], 10.82f, center.z + 0.51f }, 0.007f, 0.007f, 6, wireCol);

    }



    // Aluminum Handles & Diagonal Glass Highlights

    Color handleAlum = ApplyShopLighting(Vector3{ center.x, 10.93f, center.z }, Color{ 220, 225, 232, 255 });

    DrawCube(Vector3{ center.x - 0.86f, 10.932f, center.z - 0.49f }, 1.50f, 0.018f, 0.035f, handleAlum);

    DrawCube(Vector3{ center.x + 0.86f, 10.944f, center.z - 0.49f }, 1.50f, 0.018f, 0.035f, handleAlum);

    DrawLine3D(Vector3{ center.x - 1.56f, 10.926f, center.z - 0.35f }, Vector3{ center.x - 0.31f, 10.926f, center.z + 0.35f }, Color{ 255, 255, 255, 110 });

    DrawLine3D(Vector3{ center.x + 0.16f, 10.938f, center.z - 0.35f }, Vector3{ center.x + 1.41f, 10.938f, center.z + 0.35f }, Color{ 255, 255, 255, 110 });



    // Digital Microprocessor Temperature Display (Front South Face, Z = 132.86)

    Vector3 dispPos = { center.x, 10.66f, center.z - 0.635f };

    DrawCube(dispPos, 0.28f, 0.09f, 0.015f, Color{ 14, 16, 18, 255 });



    // Digital readout: "-18°C" in glowing arctic cyan

    Color ledText = Color{ 50, 240, 255, 255 };

    float textZ = dispPos.z - 0.010f;

    DrawLine3D(Vector3{ center.x - 0.095f, 10.66f, textZ }, Vector3{ center.x - 0.075f, 10.66f, textZ }, ledText);

    DrawLine3D(Vector3{ center.x - 0.055f, 10.635f, textZ }, Vector3{ center.x - 0.055f, 10.685f, textZ }, ledText);

    DrawCube(Vector3{ center.x - 0.025f, 10.66f, textZ }, 0.024f, 0.050f, 0.002f, ledText);

    DrawCube(Vector3{ center.x + 0.005f, 10.680f, textZ }, 0.008f, 0.008f, 0.002f, ledText);

    DrawCube(Vector3{ center.x + 0.032f, 10.66f, textZ }, 0.024f, 0.050f, 0.002f, ledText);

    // Green operational status LED dot

    DrawSphere(Vector3{ center.x + 0.095f, 10.66f, textZ }, 0.005f, Color{ 45, 255, 95, 255 });

}



void DrawShopProductsAndParticles(const Camera3D &camera, float walkTime, float bobAmplitude, float dt)

{

    float wobble = sinf((float)GetTime() * 6.0f) * 0.004f;

    Vector3 fwdDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));



    bool canSeeShopInterior = (camera.position.x <= 135.0f && camera.position.x >= 70.0f &&
                               camera.position.z >= 110.0f && camera.position.z <= 175.0f);



    for (size_t i = 0; i < g_shopProducts.size(); i++) {

        const ShopProduct &it = g_shopProducts[i];

        bool isHeld = ((int)i == g_heldProductIndex);



        if (!isHeld && it.type != PROD_CART && !canSeeShopInterior) continue;



        float dx = it.homePos.x - camera.position.x;

        float dy = it.homePos.y - camera.position.y;

        float dz = it.homePos.z - camera.position.z;

        float distSq = dx * dx + dy * dy + dz * dz;



        if (!isHeld) {

            if (it.type == PROD_CART) {

                // Cart visible up to 55m

                if (distSq > 55.0f * 55.0f) continue;

                if (distSq > 4.0f * 4.0f) {

                    Vector3 toCart = Vector3Normalize(Vector3{ dx, dy, dz });

                    if (Vector3DotProduct(fwdDir, toCart) < -0.35f) continue;

                }

            } else {

                // Shelf backplate occlusion culling:

                if (fabsf(it.originalPos.z - 140.0f) < 1.0f) {

                    if (it.homePos.z > 140.0f && camera.position.z < 140.0f) continue;

                    if (it.homePos.z < 140.0f && camera.position.z > 140.0f) continue;

                } else if (fabsf(it.originalPos.z - 147.0f) < 1.0f) {

                    if (it.homePos.z > 147.0f && camera.position.z < 147.0f) continue;

                    if (it.homePos.z < 147.0f && camera.position.z > 147.0f) continue;

                }



                // Tight distance culling for shelf items: 11 meters

                if (distSq > 11.0f * 11.0f) continue;



                // Camera frustum culling

                if (distSq > 1.2f * 1.2f) {

                    Vector3 toProd = Vector3Normalize(Vector3{ dx, dy, dz });

                    if (Vector3DotProduct(fwdDir, toProd) < 0.35f) continue;

                }

            }

        }



        switch (it.type) {

            case PROD_TIN:

                DrawPopcornTin(it.homePos, it.opened, wobble, isHeld, distSq);

                break;

            case PROD_MILK:

                DrawBottle(it.homePos, 0.34f, 0.055f, g_milkLiquidModel, it.fill, Color{ 200, 230, 255, 255 }, it.opened, isHeld, fwdDir, distSq);

                break;

            case PROD_BLOOD:

                DrawBottle(it.homePos, 0.34f, 0.055f, g_bloodLiquidModel, it.fill, Color{ 180, 200, 190, 255 }, it.opened, isHeld, fwdDir, distSq);

                break;

            case PROD_BREAD:

                DrawBread(it.homePos, g_breadModel, isHeld, distSq);

                break;

            case PROD_CART:

                rlPushMatrix();

                    rlTranslatef(g_cartPos.x, g_cartPos.y, g_cartPos.z);

                    rlRotatef(g_cartYaw, 0, 1, 0);

                    DrawShoppingCartLocal(g_cartWheelSpin, g_cartPos, distSq);

                rlPopMatrix();

                break;

            case PROD_CHOCOLATE:

                DrawChocolateBar(it.homePos, it.subType, it.opened, isHeld, fwdDir, distSq);

                break;

            case PROD_GUN:

                if (!isHeld) {

                    DrawGunWorld(it.homePos, false);

                } else {

                    DrawGunViewModel(camera, walkTime, bobAmplitude, dt);

                }

                break;

        }

    }



    if (canSeeShopInterior) {

        for (const auto &p : g_shopParticles) {

            DrawShopParticle(p);

        }

        DrawDustParticles(camera);

        DrawFootstepTrails();

        DrawShellCasingsAndSparks();

    }

}



int GetCrosshairFocusedProduct(const Camera3D &camera, float maxReach, bool allowCart)

{

    Vector3 rayOrigin = camera.position;

    Vector3 rayDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));



    int bestIdx = -1;

    float bestScore = 1e9f;



    for (size_t i = 0; i < g_shopProducts.size(); i++) {

        const ShopProduct &p = g_shopProducts[i];

        if (p.held) continue;

        if (p.type == PROD_CART && !allowCart) continue;



        // Divider wall occlusion check for double-sided racks:

        // If an item is on the opposite face of the rack divider wall relative to player, skip!

        if (p.homePos.x >= 88.8f && p.homePos.x <= 101.2f) {

            if (fabsf(p.originalPos.z - 140.0f) < 1.0f) {

                if (p.homePos.z > 140.0f && rayOrigin.z < 140.0f) continue;

                if (p.homePos.z < 140.0f && rayOrigin.z > 140.0f) continue;

            } else if (fabsf(p.originalPos.z - 147.0f) < 1.0f) {

                if (p.homePos.z > 147.0f && rayOrigin.z < 147.0f) continue;

                if (p.homePos.z < 147.0f && rayOrigin.z > 147.0f) continue;

            }

        }



        Vector3 v = Vector3Subtract(p.homePos, rayOrigin);

        float t = Vector3DotProduct(v, rayDir);

        if (t <= 0.2f || t > maxReach) continue;



        // Closest point on ray to product center

        Vector3 rayPt = Vector3Add(rayOrigin, Vector3Scale(rayDir, t));

        float perpDist = Vector3Distance(p.homePos, rayPt);



        // Shopping cart has a larger bounding reach than small shelf bottles

        float maxPerp = (p.type == PROD_CART) ? 0.55f : 0.24f;

        if (perpDist <= maxPerp) {

            float score = perpDist * 2.0f + t * 0.08f;

            if (score < bestScore) {

                bestScore = score;

                bestIdx = (int)i;

            }

        }

    }

    return bestIdx;

}

