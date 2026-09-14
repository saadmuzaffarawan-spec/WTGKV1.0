#pragma once
#include <raylib.h>

// =========================================================================
// DYNAMIC FOOTPRINT DECAL SYSTEM
// Alternating left/right boot imprints stamped into dust/grime or dark blood,
// dissolving smoothly over 8 seconds.
// =========================================================================

struct FootprintDecal {
    Vector3 pos;
    float yaw;
    float life;
    float maxLife;
    bool isLeft;
    bool isBloody;
};

#define MAX_FOOTPRINTS 200

// Initialize the footprint decal system
void InitFootprints();

// Updates footprints: stride detection, blood pool step-in, buffer advancement, and dissolving
void UpdateFootprints(Vector3 camPos, Vector3 viewDir, bool onGround, float hitStopTimer, float dt);

// Renders active footprints (soles, heels, tread ridges, blood spatters)
void DrawFootprints();

