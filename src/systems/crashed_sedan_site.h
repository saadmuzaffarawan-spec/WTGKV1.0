#pragma once
#include <raylib.h>

// =========================================================================
// CRASHED SILVER SEDAN & ROADSIDE IMPACT SITE (Route 9 Mile Marker 14)
// Tilted into ditch verge at X = 143.8, Z = 136.5 with crumpled accordion
// hood, steaming cracked radiator, spiderweb fractured windshield,
// deflated airbag, sprung ajar driver door, sheared utility pole, and skid ruts.
// =========================================================================

extern const Vector3 g_crashedCarPos;

// Computes exterior environmental lighting tint for crashed vehicle geometry
Color CrashCarTint(Color base, float dayF, float nightF, float vertBias = 1.0f);

// Renders the full crashed vehicle impact scene, skid marks, sheared utility pole,
// shattered safety glass, crushed engine cavity, and rising radiator steam particles.
void DrawCrashedSedan(Vector3 carPos, float timeVal, float extDayFactor, float extNightFactor, const Camera3D& camera);
