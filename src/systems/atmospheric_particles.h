#pragma once

#include <raylib.h>
#include <raymath.h>
#include "shop_item_types.inl"

// ======================================================================
// ATMOSPHERIC VOLUMETRIC DUST & FOOTSTEP TRAILS SYSTEM (ZERO-HEAP FRAGMENTATION)
// In-World Shop Products, Gun Viewmodel & World Display, Refrigerator Appliance
// ======================================================================

void SpawnPickupDust(Vector3 center, int count = 20);
void UpdateDustParticles(float dt);
void DrawDustParticles(const Camera3D &camera);

void AddFootstepTrail(Vector3 pos, float yaw, bool isLeft);
void UpdateFootstepTrails(float dt);
void DrawFootstepTrails();

void UpdateShellCasingsAndSparks(float dt);
void DrawShellCasingsAndSparks();

void DrawChocolateBar(Vector3 pos, int subType, bool opened, bool held, Vector3 fwdDir, float distSq = 0.0f);
void DrawGunWorld(Vector3 pos, bool held);
void DrawGunViewModel(const Camera3D &camera, float walkTime, float bobAmplitude, float dt);
void DrawHorizontalRefrigerator(Vector3 center, const Camera3D &camera);
void DrawShopProductsAndParticles(const Camera3D &camera, float walkTime = 0.0f, float bobAmplitude = 0.0f, float dt = 0.016f);
int  GetCrosshairFocusedProduct(const Camera3D &camera, float maxReach = 2.8f, bool allowCart = false);

void ToggleGameFullscreen();
