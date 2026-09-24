// First-person player: movement, collision, camera feel, footsteps, flashlight.
#pragma once
#include "engine/scene.h"

struct PlayerInput {
    Vector2 move{};       // x strafe, y forward
    Vector2 look{};       // radians this frame
    bool sprint = false, crouch = false, jump = false;
    bool interact = false, interactHeld = false, flashlight = false, use = false, useHeld = false;
};

PlayerInput ReadPlayerInput(float sensitivity, bool invertY);

class Player {
public:
    Vector3 feet{ 0, 0, 0 };
    Vector3 vel{};
    float yaw = 0, pitch = 0;
    bool grounded = true;
    bool indoor = false;
    int surface = SURF_GRASS;
    float radius = 0.3f, height = 1.8f, eye = 1.66f;
    float walkSpeed = 1.55f, sprintSpeed = 3.7f, crouchSpeed = 0.85f;
    float weight = 0.25f;          // 0 = weightless soul, 1 = fully "kept" (heavier steps)
    float stamina = 1.0f;
    float fear = 0.0f;             // drives breathing / heartbeat
    bool locked = false;           // movement disabled (cutscenes, dialogue)
    bool lookLocked = false;
    bool noclip = false;

    // Flashlight
    bool flashOn = false;
    float battery = 1.0f;          // 0..1
    float flashFlicker = 1.0f;

    // Camera feel
    float bobPhase = 0, bobAmt = 0, landDip = 0, crouchT = 0, roll = 0, stepSmooth = 0;
    float fovKick = 0;
    Vector2 sway{};
    float shake = 0;               // external camera shake amount

    void Spawn(Vector3 p, float yawDeg);
    void Update(const PlayerInput& in, float dt);
    Camera3D GetCamera(float fov) const;
    Vector3 EyePos() const;
    Vector3 Forward() const { return DirFromYawPitch(yaw, pitch); }
    Light FlashLight() const;
    float Speed2D() const { return sqrtf(vel.x * vel.x + vel.z * vel.z); }

private:
    float prevStepPhase_ = 0;
    float airTime_ = 0;
    float lastY_ = 0;
    void Footstep(float speed);
};
