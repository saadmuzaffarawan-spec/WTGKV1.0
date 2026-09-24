#include "player.h"
#include "engine/audio.h"

PlayerInput ReadPlayerInput(float sens, bool invertY) {
    PlayerInput in;
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) in.move.y += 1;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) in.move.y -= 1;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) in.move.x += 1;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) in.move.x -= 1;
    Vector2 md = GetMouseDelta();
    in.look = { -md.x * 0.0022f * sens, -md.y * 0.0022f * sens * (invertY ? -1.0f : 1.0f) };
    in.sprint = IsKeyDown(KEY_LEFT_SHIFT);
    in.crouch = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_C);
    in.jump = IsKeyPressed(KEY_SPACE);
    in.interact = IsKeyPressed(KEY_E);
    in.interactHeld = IsKeyDown(KEY_E);
    in.flashlight = IsKeyPressed(KEY_F);
    in.use = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    in.useHeld = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    // Gamepad
    if (IsGamepadAvailable(0)) {
        auto dz = [](float v) { return fabsf(v) < 0.15f ? 0.0f : v; };
        in.move.x += dz(GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X));
        in.move.y -= dz(GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y));
        float dt = GetFrameTime();
        in.look.x -= dz(GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X)) * 2.4f * dt * sens;
        in.look.y -= dz(GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y)) * 1.8f * dt * sens * (invertY ? -1.0f : 1.0f);
        in.sprint |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_THUMB);
        in.crouch |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_THUMB);
        in.interact |= IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        in.interactHeld |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        in.flashlight |= IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_FACE_UP);
        in.use |= IsGamepadButtonPressed(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
        in.useHeld |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
    }
    if (Vector2Length(in.move) > 1.0f) in.move = Vector2Normalize(in.move);
    return in;
}

void Player::Spawn(Vector3 p, float yawDeg) {
    feet = p;
    float g = Phys().Ground(p.x, p.z, p.y + 1.0f, radius);
    if (g > -1e8f) feet.y = g;
    vel = { 0, 0, 0 };
    yaw = yawDeg * DEG2RAD; pitch = 0;
    grounded = true;
    lastY_ = feet.y;
}

void Player::Footstep(float speed) {
    float vol = Clamp(0.25f + speed * 0.12f, 0.2f, 0.75f) * (0.45f + weight * 0.55f);
    if (crouchT > 0.5f) vol *= 0.5f;
    audio::PlayFootstep(surface, feet, vol, indoor);
}

void Player::Update(const PlayerInput& in, float dt) {
    dt = fminf(dt, 0.05f);
    if (!lookLocked) {
        yaw += in.look.x;
        pitch = Clamp(pitch + in.look.y, -1.45f, 1.45f);
    }
    PlayerInput i = in;
    if (locked) { i.move = { 0, 0 }; i.sprint = i.crouch = i.jump = false; }

    // --- desired velocity ----------------------------------------------------
    crouchT = Damp(crouchT, i.crouch ? 1.0f : 0.0f, 10.0f, dt);
    bool sprinting = i.sprint && i.move.y > 0.3f && stamina > 0.05f && crouchT < 0.3f;
    float speed = crouchT > 0.5f ? crouchSpeed : (sprinting ? sprintSpeed : walkSpeed);
    speed *= 1.0f - weight * 0.12f;
    Vector3 fwd = YawDir(yaw);
    Vector3 right{ -fwd.z, 0, fwd.x };
    Vector3 wish = Vector3Add(Vector3Scale(fwd, i.move.y), Vector3Scale(right, i.move.x));
    wish = Vector3Scale(wish, speed);
    float accel = grounded ? (Vector2Length(i.move) > 0.01f ? 9.0f : 11.0f) : 1.5f;
    vel.x = Damp(vel.x, wish.x, accel, dt);
    vel.z = Damp(vel.z, wish.z, accel, dt);
    stamina = Clamp(stamina + (sprinting ? -0.09f : 0.06f) * dt, 0.0f, 1.0f);

    if (noclip) {
        Vector3 f3 = Forward();
        feet = Vector3Add(feet, Vector3Scale(f3, i.move.y * speed * 4 * dt));
        feet = Vector3Add(feet, Vector3Scale(right, i.move.x * speed * 4 * dt));
        return;
    }

    // --- horizontal move with collision --------------------------------------
    const float stepH = 0.4f;
    float h = height - crouchT * 0.6f;
    Vector3 next = feet;
    next.x += vel.x * dt;
    next.z += vel.z * dt;
    Phys().ResolveCylinder(next, radius, h, stepH);
    int surf = SURF_NONE, owner = -1;
    float g = Phys().Ground(next.x, next.z, next.y + stepH, radius, &surf, &owner);
    float moved = sqrtf((next.x - feet.x) * (next.x - feet.x) + (next.z - feet.z) * (next.z - feet.z));
    // terrain slope limit (~42 degrees); steps on boxes are handled by stepH
    if (owner == -100 && g > feet.y + 0.02f && g - feet.y > moved * 0.9f + 0.02f) {
        next.x = feet.x; next.z = feet.z;
        g = Phys().Ground(next.x, next.z, next.y + stepH, radius, &surf, &owner);
        vel.x *= 0.5f; vel.z *= 0.5f;
    }
    // low ceilings block
    float ceil = Phys().Ceiling(next.x, next.z, next.y + stepH, radius);
    if (ceil < next.y + h * 0.95f && ceil > next.y + stepH) { next.x = feet.x; next.z = feet.z; }

    // --- vertical ---------------------------------------------------------------
    if (grounded && g > -1e8f && next.y - g < 0.45f) {
        // follow the ground (stairs down / step up), smoothing the camera
        stepSmooth += next.y - g;
        next.y = g;
        vel.y = 0;
        if (i.jump && !locked) { vel.y = 3.4f; grounded = false; }
    } else {
        vel.y -= 9.81f * dt;
        next.y += vel.y * dt;
        grounded = false;
        airTime_ += dt;
        if (g > -1e8f && next.y <= g) {
            next.y = g;
            if (airTime_ > 0.25f) {
                landDip = Clamp(-vel.y * 0.035f, 0.03f, 0.18f);
                surface = surf;
                Footstep(4.0f);
                if (vel.y < -7.0f) audio::Play3D("body_thud", next, 0.7f);
            }
            vel.y = 0;
            grounded = true;
            airTime_ = 0;
        }
    }
    if (g > -1e8f) surface = surf;
    feet = next;
    if (feet.y < -300) Spawn({ 0, 5, 0 }, 0);

    // --- indoor detection (roof overhead) --------------------------------------
    indoor = Phys().Ceiling(feet.x, feet.z, feet.y + 1.0f, 0.1f) < feet.y + 8.0f;

    // --- camera feel ---------------------------------------------------------
    float spd = Speed2D();
    float strideLen = sprinting ? 1.15f : (crouchT > 0.5f ? 0.55f : 0.78f);
    if (grounded) bobPhase += spd * dt / strideLen * 0.5f;
    bobAmt = Damp(bobAmt, grounded ? Clamp(spd / 3.5f, 0.0f, 1.0f) : 0.0f, 6.0f, dt);
    float stepPhase = floorf(bobPhase * 2.0f);
    if (stepPhase != prevStepPhase_ && grounded && spd > 0.4f) Footstep(spd);
    prevStepPhase_ = stepPhase;
    landDip = Damp(landDip, 0.0f, 6.0f, dt);
    stepSmooth = Damp(stepSmooth, 0.0f, 12.0f, dt);
    roll = Damp(roll, -i.move.x * 0.012f - in.look.x * 0.8f, 5.0f, dt);
    sway.x = Damp(sway.x, in.look.x * 3.0f, 8.0f, dt);
    sway.y = Damp(sway.y, in.look.y * 3.0f, 8.0f, dt);
    fovKick = Damp(fovKick, sprinting ? 4.0f : 0.0f, 3.0f, dt);
    shake = Damp(shake, 0.0f, 3.0f, dt);

    // --- flashlight -------------------------------------------------------------
    if (in.flashlight) {
        if (battery > 0.0f) { flashOn = !flashOn; }
        audio::Play("flashlight", 0.6f);
    }
    if (flashOn) {
        battery = fmaxf(0.0f, battery - dt / 900.0f);   // 15 minutes per battery
        float lowBat = SmoothStep(0.15f, 0.0f, battery);
        if (lowBat > 0 && Frand(0, 1) < lowBat * 0.08f) flashFlicker = Frand(0.0f, 0.6f);
        else flashFlicker = Damp(flashFlicker, 1.0f - lowBat * 0.5f, 18.0f, dt);
        if (battery <= 0.0f) flashOn = false;
    }

    // --- breathing / fear ----------------------------------------------------
    auto& amb = audio::Amb();
    float exert = 1.0f - stamina;
    amb.breath = Clamp(exert * 0.8f + fear * 0.6f, 0.0f, 1.0f) * 0.7f;
    amb.breathRate = 12.0f + exert * 22.0f + fear * 16.0f;
    amb.indoor = indoor ? 1.0f : 0.0f;
}

Vector3 Player::EyePos() const {
    float e = eye - crouchT * 0.55f;
    float bobY = sinf(bobPhase * 2 * PI * 2.0f) * 0.035f * bobAmt;
    return { feet.x, feet.y + e + bobY - landDip + stepSmooth * 0.8f, feet.z };
}

Camera3D Player::GetCamera(float fov) const {
    Camera3D c{};
    Vector3 eyeP = EyePos();
    Vector3 fwd = YawDir(yaw);
    Vector3 right{ -fwd.z, 0, fwd.x };
    float lat = sinf(bobPhase * 2 * PI) * 0.02f * bobAmt;
    eyeP = Vector3Add(eyeP, Vector3Scale(right, lat));
    float t = (float)GetTime();
    // breathing idle sway + external shake
    float idle = (1.0f - bobAmt) * 0.004f;
    float sy = sinf(t * 1.3f) * idle + (shake > 0.001f ? Frand(-1, 1) * shake * 0.05f : 0.0f);
    float sx = sinf(t * 0.9f) * idle * 0.6f + (shake > 0.001f ? Frand(-1, 1) * shake * 0.05f : 0.0f);
    Vector3 dir = DirFromYawPitch(yaw + sx, pitch + sy);
    c.position = eyeP;
    c.target = Vector3Add(eyeP, dir);
    float r = roll + sinf(bobPhase * 2 * PI) * 0.004f * bobAmt;
    Vector3 up = Vector3RotateByAxisAngle({ 0, 1, 0 }, dir, r);
    c.up = up;
    c.fovy = fov + fovKick;
    c.projection = CAMERA_PERSPECTIVE;
    return c;
}

Light Player::FlashLight() const {
    Light l;
    Vector3 fwd = Forward();
    Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, { 0, 1, 0 }));
    l.pos = Vector3Add(EyePos(), Vector3Add(Vector3Scale(right, 0.18f), Vector3{ 0, -0.22f, 0 }));
    l.pos = Vector3Add(l.pos, Vector3Scale(fwd, 0.1f));
    // the beam lags slightly behind the view like a hand-held torch
    l.dir = Vector3Normalize(Vector3Add(fwd, Vector3{ -sway.x * 0.05f, -sway.y * 0.05f - 0.03f, 0 }));
    l.color = { 1.0f, 0.9f, 0.76f };
    l.intensity = 19.0f * flashFlicker * (0.55f + 0.45f * SmoothStep(0.0f, 0.3f, battery));
    l.range = 30.0f;
    l.innerDeg = 9.0f; l.outerDeg = 25.0f;
    l.shadow = true;
    l.volumetric = 0.05f;
    l.priority = 1.0f;
    return l;
}
