// Moving parts of the brothers' sedan used by the driving rig and cutscenes.
#pragma once
#include "engine/mesh_builder.h"

struct SedanParts {
    Model3D* wheel = nullptr;     // left side (hub cap facing +X)
    Model3D* wheelR = nullptr;    // right side
    Model3D* steering = nullptr;  // centred at origin, rim in XY, column towards +Z
    Model3D* needle = nullptr;
    Model3D* gauges = nullptr;
};
const SedanParts& GetSedanParts();

// Seat / eye anchors in car-local space
constexpr float kDriverX = 0.38f, kPassengerX = -0.38f;
constexpr float kSeatY = 0.55f, kSeatZ = -0.26f;
constexpr float kEyeY = 1.2f, kEyeZ = -0.3f;
constexpr float kWheelY = 0.31f, kWheelZ = 1.35f, kTrack = 0.76f;
constexpr float kSteerY = 0.98f, kSteerZ = 0.3f, kSteerTilt = -22.0f;
