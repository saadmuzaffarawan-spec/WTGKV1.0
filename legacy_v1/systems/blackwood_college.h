#pragma once

#include <raylib.h>
#include <raymath.h>

// =========================================================================
// ABANDONED BLACKWOOD COLLEGE: ADVANCED DYNAMIC LIGHTING & PROCEDURAL SHADER
// Features: Embedded multi-point-light + distance-fog shader, procedural grime
// and water streak textures, institutional checkered tile & acid-etched soapstone,
// 6 dynamic flickering horror point lights, and textured 3D architectural boxes.
// =========================================================================

#define COLLEGE_MAX_LIGHTS 6

extern float g_collegeFlickerTimer;
extern float g_collegeCreakTimer;
extern bool g_collegeLightOn;

void InitCollegeShaderAndTextures();
void UnloadCollegeShaderAndTextures();
void DrawAbandonedCollege(Camera3D camera, float timeVal, float dt, float extDayFactor, float extNightFactor, Vector3 sunDir);

