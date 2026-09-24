#pragma once

#include <stdint.h>

#include <raylib.h>

#ifndef LOGICAL_W
#define LOGICAL_W 1280
#endif

#ifndef LOGICAL_H
#define LOGICAL_H 720
#endif

#ifndef CHUNK_W
#define CHUNK_W 256
#endif

#ifndef CHUNK_H
#define CHUNK_H 32
#endif

#ifndef CHUNK_D
#define CHUNK_D 256
#endif

#ifndef CYAN
#define CYAN Color{ 0, 255, 255, 255 }
#endif

struct Voxel {
    uint8_t glyphIndex;
    Color fgColor;
    bool isSolid;
};

struct PhysicsParticle {
    Vector3 pos;
    Vector3 vel;
    uint8_t glyphIndex;
    Color color;
    float life;
};

struct Star {
    Vector3 basePos;
    float phase;
    bool isBig;
};

enum ShovelAnimState {
    SHOVEL_ANIM_IDLE,
    SHOVEL_ANIM_DIG,
    SHOVEL_ANIM_ATTACK
};
