// Prologue (the drive, the crash, death, the soul, the Dragger) and Awakening.
#include "story_impl.h"
#include <cstdlib>
#include <cstdio>

static bool Cross(float t, float dt, float at) { return t - dt < at && t >= at; }

// Seat an actor inside a car-like transform
static void Seat(Actor* a, const Matrix& carXf, float carYaw, float x, float lean = 0.0f) {
    if (!a) return;
    float hip = a->model->hipHeight;
    a->pos = Vector3Transform({ x, kSeatY + 0.1f - hip, kSeatZ + lean }, carXf);
    a->yaw = carYaw;
}

// Camera for the driver POV: car-relative eye with free look
static Camera3D DriverCam(const CarRig& car, Vector2 look, float fov, float shake) {
    Camera3D c{};
    c.position = car.Local({ kDriverX, kEyeY, kEyeZ });
    float yaw = car.yaw + look.x, pitch = look.y - 0.05f;
    if (shake > 0) { yaw += Frand(-1, 1) * shake * 0.04f; pitch += Frand(-1, 1) * shake * 0.04f; c.position.y += Frand(-1, 1) * shake * 0.02f; }
    c.target = Vector3Add(c.position, DirFromYawPitch(yaw, pitch));
    c.up = Vector3RotateByAxisAngle({ 0, 1, 0 }, DirFromYawPitch(yaw, pitch), car.roll * 0.6f);
    c.fovy = fov;
    c.projection = CAMERA_PERSPECTIVE;
    return c;
}

static void ReadLook(Vector2& look, float sens, float maxYaw, float maxPitch) {
    if (G().shotFile) return;
    Vector2 md = GetMouseDelta();
    look.x = Clamp(look.x - md.x * 0.0022f * sens, -maxYaw, maxYaw);
    look.y = Clamp(look.y - md.y * 0.0022f * sens, -maxPitch, maxPitch);
}

void Story::Impl::BuildPrologue(Script& s) {
    Game& gg = g;
    s.Do([this, &gg]() {
        gg.story->cutscene = true;
        gg.story->skippable = false;
        gg.hud.FadeInstant(1.0f);
        gg.hud.hidden = true;
        gg.spirit = 0.0f;
        gg.player.flashOn = false;
        car = CarRig();
        car.visible = true;
        car.body = GetPrefabModel("sedan", { { "color", "blue" }, { "state", "intact" } });
        car.pos = RoadPoint(-330.0f, 1.85f);
        car.yaw = RoadYaw(-330.0f);
        car.speed = 11.0f;
        Actor* adam = gg.SpawnActor("adam", SpecAdam(), car.pos, 0);
        adam->hideHead = true;
        adam->SetPose(PoseSitDrive(0, 0), true);
        Actor* zain = gg.SpawnActor("zain", SpecZain(), car.pos, 0);
        zain->SetPose(PoseSitPassenger(0, 0), true);
        zain->blendSpeed = 4.0f;
        auto& a = audio::Amb();
        a.engine = 0.9f; a.rpm = 1700; a.road = 0.6f; a.cabin = 0.5f; a.wind = 0.15f; a.crickets = 0.05f; a.indoor = 1.0f;
        lookOffset = { 0, 0 };
        if (const char* lk = getenv("WTGK_LOOK")) sscanf(lk, "%f,%f", &lookOffset.x, &lookOffset.y);
        gg.weather.fog = 0.55f;
    });
    // One continuous timeline: T = seconds since the start of the drive.
    s.Run([this, &gg](float T, float dt) {
        auto& a = audio::Amb();
        Actor* zain = gg.A("zain");
        Actor* adam = gg.A("adam");
        auto& rs = Rdr().s;
        const float Tyank = 41.0f, Timpact = 45.4f;
        // ---------------- dialogue ----------------
        struct Line { float at; const char* who; const char* text; };
        static const Line lines[] = {
            { 0.8f, "ZAIN", "Adam." }, { 2.2f, "ZAIN", "Adaaam." }, { 3.6f, "ADAM", "What." },
            { 4.8f, "ZAIN", "Say I'm the better driver." }, { 6.9f, "ADAM", "You're fourteen, Zain." },
            { 8.6f, "ZAIN", "Fourteen and a half." },
            { 11.5f, "ZAIN", "Let me drive. Just this bit. It's straight." }, { 14.6f, "ADAM", "No." },
            { 16.4f, "ZAIN", "Relax! I'm kidding. I'm kidding." }, { 19.0f, "ADAM", "Do that again and you're walking home." },
            { 21.8f, "ZAIN", "Worth it." },
            { 27.0f, "ZAIN", "Who even stops there?" }, { 29.4f, "ADAM", "Mum did. Every Sunday. Those orange sweets." },
            { 32.4f, "ZAIN", "...Yeah." }, { 34.6f, "ZAIN", "Do you think she can see us?" },
            { 37.2f, "ADAM", "I think she'd tell you to sit properly." },
            { 39.9f, "ADAM", "What is th-" }, { 40.4f, "ZAIN", "Don't stop." },
        };
        for (const Line& l : lines) if (Cross(T, dt, l.at)) gg.hud.Say(l.who, l.text, l.at > 39.0f ? 1.2f : -1.0f);
        if (Cross(T, dt, 9.0f)) { gg.hud.FadeTo(0.0f, 0.45f); gg.hud.hidden = true; }
        if (Cross(T, dt, 9.0f)) gg.spirit = 0.0f;

        // ---------------- driving ----------------
        if (T < Tyank) {
            float z = -330.0f + 11.0f * T;
            float lane = 1.85f;
            float grab = (T > 15.8f && T < 17.6f) ? sinf((T - 15.8f) * 9.0f) * (1.0f - (T - 15.8f) / 1.8f) : 0.0f;
            lane += grab * 0.45f;
            car.pos = RoadPoint(z, lane);
            car.yaw = RoadYaw(z) + grab * 0.06f;
            car.steer = grab * 0.8f + (RoadYaw(z + 10) - RoadYaw(z)) * 4.0f;
            car.roll = -grab * 0.02f + sinf(T * 1.3f) * 0.002f;
            car.pitch = sinf(T * 2.1f) * 0.002f;
            a.rpm = 1650 + sinf(T * 0.3f) * 60;
            if (Cross(T, dt, 16.0f)) audio::Play("screech", 0.12f, 1.4f);
            if (Cross(T, dt, 44.0f)) audio::Play("horn");
            // Zain: animated passenger
            float laugh = (T > 9.5f && T < 11.4f) || (T > 16.3f && T < 19.0f) || (T > 21.8f && T < 23.5f) ? 1.0f : 0.0f;
            Pose zp = PoseSitPassenger(T, laugh);
            if (T > 15.8f && T < 17.3f) { zp.rot[B_RUARM].x = -1.2f; zp.rot[B_RUARM].z = 0.55f; zp.rot[B_RFARM].x = -0.3f; }
            if (T > 26.0f && T < 33.0f) { zp.rot[B_HEAD].y = 0.9f; zp.rot[B_NECK].y = 0.3f; }   // looks out at the station
            if (T > 39.5f) { zp.rot[B_RUARM].x = -1.1f; zp.rot[B_RUARM].z = 0.5f; zp.rot[B_RFARM].x = -0.35f; }   // hand to the wheel
            zain->SetPose(zp);
            // Zain looks back at you when you look at him
            if (lookOffset.x > 0.6f && T > 9.5f && T < 39.0f) { zain->lookAt = gg.camera.position; zain->lookWeight = Damp(zain->lookWeight, 0.8f, 3.0f, dt); }
            else zain->lookWeight = Damp(zain->lookWeight, 0.0f, 3.0f, dt);
            adam->SetPose(PoseSitDrive(T, car.steer));
            // hands on the wheel at ten and two (they follow the rim as it turns)
            for (int side = 0; side < 2; side++) {
                float ang = (side == 0 ? 0.38f : 2.76f) - car.steer * 1.6f;
                Vector3 grip{ kDriverX + cosf(ang) * 0.18f, kSteerY + sinf(ang) * 0.18f * 0.93f, kSteerZ - sinf(ang) * 0.18f * 0.37f };
                adam->Reach(side, car.Local(grip));
            }
            if (T > 15.8f && T < 17.3f) zain->Reach(1, car.Local({ kDriverX + 0.16f, kSteerY + 0.08f, kSteerZ }));
            else if (T > 39.5f) zain->Reach(1, car.Local({ kDriverX + 0.14f, kSteerY + 0.12f, kSteerZ }));
            else zain->ReleaseIK();
        } else if (T < Timpact) {
            // the yank: skid, drift, slide off the road into the tower
            float u = Saturate((T - Tyank) / (Timpact - Tyank));
            float ue = 1.0f - (1.0f - u) * (1.0f - u) * 0.6f - 0.4f * (1.0f - u);
            Vector3 p0 = RoadPoint(-330.0f + 11.0f * Tyank, 1.85f);
            Vector3 p1 = RoadPoint(-330.0f + 11.0f * Tyank + 16.0f, 3.2f);
            Vector3 p2 = { 15.5f, 0, 158.0f };
            Vector3 p3 = { 19.2f, 0, 166.2f };
            Vector3 q = CatmullRom(p0, p1, p2, p3, 0.0f);
            // cubic bezier through the four points
            float v = ue;
            float b0 = (1 - v) * (1 - v) * (1 - v), b1 = 3 * v * (1 - v) * (1 - v), b2 = 3 * v * v * (1 - v), b3 = v * v * v;
            q = Vector3Add(Vector3Add(Vector3Scale(p0, b0), Vector3Scale(p1, b1)), Vector3Add(Vector3Scale(p2, b2), Vector3Scale(p3, b3)));
            q.y = World().Height(q.x, q.z) + 0.05f;
            if (fabsf(q.x - Terrain::RoadX(q.z)) < 3.7f) q.y = Terrain::RoadY(q.z) + 0.05f;
            car.pos = q;
            float yaw0 = RoadYaw(p0.z);
            car.yaw = Lerp(yaw0, 52.0f * DEG2RAD, powf(u, 0.8f)) + sinf(u * PI) * 0.75f;
            car.roll = sinf(u * PI) * 0.09f;
            car.steer = u < 0.3f ? 1.0f : -0.8f;
            car.speed = 13.0f;
            if (Cross(T, dt, Tyank)) { audio::Play("screech", 0.95f); a.screech = 0.6f; gg.player.fear = 1.0f; }
            a.screech = Damp(a.screech, 0.8f, 3.0f, dt);
            a.rpm = 3800;
            if (T > Timpact - 1.0f) gg.timeScale = Damp(gg.timeScale, 0.25f, 5.0f, dt / fmaxf(gg.timeScale, 0.1f));
            Pose zp = PoseSitPassenger(T, 0.0f);
            zp.rot[B_RUARM].x = -1.1f; zp.rot[B_RUARM].z = 0.5f; zp.rot[B_RFARM].x = -0.35f;
            zp.rot[B_HEAD].y = 0.0f; zp.rot[B_HEAD].x = -0.05f;   // calm, looking straight at the figure
            zain->SetPose(zp);
            zain->Reach(1, car.Local({ kDriverX + 0.14f, kSteerY + 0.12f, kSteerZ }));
            adam->SetPose(PoseSitDrive(T, car.steer));
            for (int side = 0; side < 2; side++) {
                float ang = (side == 0 ? 0.38f : 2.76f) - car.steer * 1.6f;
                adam->Reach(side, car.Local({ kDriverX + cosf(ang) * 0.18f, kSteerY + sinf(ang) * 0.17f, kSteerZ - sinf(ang) * 0.07f }));
            }
            gg.camShake = 0.4f + u;
        }
        // the figure in the headlights
        if (Cross(T, dt, 36.0f)) {
            Actor* d = gg.SpawnActor("dragger", SpecDragger(), RoadPoint(140.0f, 2.0f), 180.0f + RoadYaw(140.0f) * RAD2DEG);
            d->SetPose(PoseLoom(0), true);
            d->pos.y = World().Height(d->pos.x, d->pos.z);
            if (fabsf(d->pos.x - Terrain::RoadX(140.0f)) < 3.7f) d->pos.y = Terrain::RoadY(140.0f) + 0.05f;
        }
        if (Actor* d = gg.A("dragger")) if (T < Timpact) { d->SetPose(PoseLoom(T)); d->lookAt = gg.camera.position; d->lookWeight = 0.7f; }

        // ---------------- impact ----------------
        if (Cross(T, dt, Timpact)) {
            if (adam) adam->ReleaseIK();
            if (zain) zain->ReleaseIK();
            gg.timeScale = 1.0f;
            car.visible = false;
            gg.SetTagVisible("after_crash", true);
            audio::Play("crash", 1.0f);
            a.screech = 0; a.engine = 0; a.road = 0; a.cabin = 0;
            rs.white = 1.0f;
            gg.camShake = 3.0f;
            Entity* w = Scn().Find("wreck");
            Vector3 ws = w ? w->LocalToWorld({ 0, 1.1f, 1.0f }) : car.pos;
            fx.Burst(P_GLASS, ws, 160, 6.0f, 1.6f, 0.02f, Color{ 220, 230, 240, 200 }, { 0, 2, 0 });
            fx.Burst(P_SPARK, Vector3Add(ws, { 0, -0.5f, 0.5f }), 60, 7.0f, 0.7f, 0.015f, Color{ 255, 190, 90, 255 });
            gg.RemoveActor("dragger");
            a.tinnitus = 1.0f; a.muffle = 0.9f;
        }
        if (T > Timpact) {
            rs.white = Damp(rs.white, 0.0f, 9.0f, dt);
            if (Cross(T, dt, Timpact + 0.25f)) gg.hud.FadeInstant(1.0f);
        }
        // ---------------- dying ----------------
        Entity* wreck = Scn().Find("wreck");
        if (T > Timpact + 2.5f && T < 61.0f && wreck) {
            if (Cross(T, dt, Timpact + 2.6f)) {
                gg.hud.FadeTo(0.15f, 0.4f);
                wreckLights = true;
                a.heart = 0.9f; a.heartRate = 64; a.breath = 0.5f; a.breathRate = 9; a.tinnitus = 0.35f; a.muffle = 0.7f;
            }
            float dT = T - (Timpact + 2.6f);
            Seat(adam, wreck->xf, wreck->worldYaw, kDriverX);
            adam->SetPose(PoseSlumped(T), true);
            Seat(zain, wreck->xf, wreck->worldYaw, kPassengerX);
            // Zain comes round: head down, then up, then turns to his brother
            float wake = SmoothStep(49.5f, 52.0f, T);
            Pose zp = PoseLerp(PoseSlumped(T), PoseSitPassenger(T, 0.0f), wake);
            zp.rot[B_HEAD].z = Lerp(-0.35f, 0.08f, wake);
            if (T > 54.0f && T < 58.5f) zain->Reach(0, wreck->LocalToWorld({ kDriverX + 0.18f, 1.02f, -0.35f }), SmoothStep(54.0f, 55.0f, T));
            else zain->ReleaseIK();
            zain->SetPose(zp);
            // POV: your head resting sideways on the airbag, looking across at him
            Vector3 eye = wreck->LocalToWorld({ kDriverX + 0.02f, 0.97f, 0.12f });
            Vector3 zh = zain->HeadPos();
            zain->lookAt = eye; zain->lookWeight = Damp(zain->lookWeight, T > 51.5f ? 1.0f : 0.0f, 2.0f, dt);
            Vector3 tgt = Vector3Lerp(wreck->LocalToWorld({ kPassengerX, 0.85f, -0.1f }), zh, SmoothStep(49.0f, 52.5f, T));
            tgt.y += sinf(T * 0.5f) * 0.02f;
            gg.camOverride = true;
            gg.camOverrideCam.position = eye;
            gg.camOverrideCam.target = tgt;
            gg.camOverrideCam.up = Vector3RotateByAxisAngle({ 0, 1, 0 }, Vector3Normalize(Vector3Subtract(tgt, eye)), -0.75f);
            gg.camOverrideCam.fovy = 50.0f;
            rs.blur = Lerp(0.9f, 0.35f, Saturate(dT / 5.0f)) + sinf(T * 0.9f) * 0.1f;
            rs.desat = 0.55f;
            gg.fearPulse = 0.4f;
            blood = Saturate(dT / 9.0f) * 0.85f;
            a.heartRate = Lerp(64.0f, 30.0f, Saturate(dT / 9.0f));
            a.breathRate = Lerp(9.0f, 4.0f, Saturate(dT / 7.0f));
            if (Cross(T, dt, 51.8f)) gg.hud.Say("ZAIN", "...Adam?", 2.0f);
            if (Cross(T, dt, 54.4f)) gg.hud.Say("ZAIN", "Adam. Wake up.", 2.4f);
            if (Cross(T, dt, 57.0f)) { a.breath = 0; audio::Play("breath_out", 0.5f); }
            if (Cross(T, dt, 58.5f)) { a.heart = 0; gg.hud.FadeTo(1.0f, 0.5f); }
            if (T > 58.5f) a.tinnitus = Damp(a.tinnitus, 0.0f, 0.8f, dt);
        }
        // ---------------- the soul rises ----------------
        if (Cross(T, dt, 61.0f) && wreck) {
            blood = 0; rs.blur = 0; rs.desat = 0.35f; gg.fearPulse = 0;
            gg.hud.Letterbox(true);
            gg.hud.FadeTo(0.0f, 0.6f);
            rs.asciiFull = 0.85f;
            gg.spirit = 0.5f;
            a.muffle = 0.45f; a.tinnitus = 0.15f; a.wind = 0.5f; a.crickets = 0.25f; a.indoor = 0;
            audio::Play("stinger_low", 0.6f);
            Vector3 head = wreck->LocalToWorld({ kDriverX + 0.04f, 1.02f, 0.05f });
            Vector3 seatsDown = wreck->LocalToWorld({ 0.0f, 0.6f, -0.2f });
            // up through the roof, turn, and hang above the car on Zain's side, facing the road
            SetCamPath({
                { 0.0f, head, wreck->LocalToWorld({ kPassengerX + 0.12f, 0.82f, -0.1f }), 58 },
                { 2.5f, wreck->LocalToWorld({ -0.2f, 2.2f, -0.2f }), seatsDown, 62 },
                { 5.5f, wreck->LocalToWorld({ 2.6f, 4.4f, -1.8f }), wreck->LocalToWorld({ 0.0f, 0.8f, 0.3f }), 60 },
                { 9.0f, wreck->LocalToWorld({ 4.6f, 5.0f, -3.6f }), RoadPoint(152.0f, 0.0f), 58 },
                { 11.0f, wreck->LocalToWorld({ 4.7f, 5.0f, -3.7f }), RoadPoint(152.0f, 0.0f), 58 },
            });
            allowLook = true;
            lookOffset = { 0, 0 };
        }
        if (T > 61.0f) rs.asciiFull = Damp(rs.asciiFull, T > 93.0f ? 1.0f : 0.0f, T > 93.0f ? 0.8f : 0.5f, dt);
        // ---------------- the Dragger takes Zain ----------------
        if (Cross(T, dt, 68.0f) && wreck) {
            Actor* d = gg.SpawnActor("dragger", SpecDragger(), RoadPoint(152.0f, -1.0f), 60.0f);
            d->SetPose(PoseWalk(0, 1, 1), true);
            audio::Play3D("growl", d->pos, 0.5f, 0.8f, 4.0f, 60.0f);
            a.crickets = 0.0f;
        }
        Actor* dr = gg.A("dragger");
        if (dr && wreck && T > 68.0f) {
            Vector3 door = wreck->LocalToWorld({ 1.35f, 0, -0.15f });
            door.y = World().Height(door.x, door.z);
            Vector3 road = RoadPoint(156.0f, 2.2f);
            if (T < 75.0f) {
                Vector3 from = RoadPoint(152.0f, -1.0f);
                float u = (T - 68.0f) / 7.0f;
                dr->pos = Vector3Lerp(from, door, u);
                dr->pos.y = World().Height(dr->pos.x, dr->pos.z);
                dr->yaw = atan2f(door.x - from.x, door.z - from.z);
                dr->SetPose(PoseWalk(T * 0.55f, 1.0f, 1));
                if (fmodf(T, 1.8f) < dt) audio::PlayFootstep(SURF_GRAVEL, dr->pos, 0.6f, false);
            } else if (T < 79.0f) {
                Vector3 in = wreck->LocalToWorld({ kPassengerX, 0.9f, -0.2f });
                dr->yaw = atan2f(in.x - dr->pos.x, in.z - dr->pos.z);
                Pose p = PoseReach(T);
                p.rot[B_SPINE].x = 0.5f; p.rot[B_RUARM].x = -1.2f;
                if (T > 77.5f) { p.rot[B_RUARM].x = -0.3f; p.rot[B_RFARM].x = -0.9f; }
                dr->SetPose(p);
                if (Cross(T, dt, 76.0f)) { audio::Play3D("glass_break", in, 1.0f, 0.9f, 3.0f, 60.0f); fx.Burst(P_GLASS, in, 80, 3.0f, 1.2f, 0.018f, Color{ 220, 230, 240, 200 }); }
                if (T > 77.5f && zain) {
                    // hauled out through the window
                    float u = Saturate((T - 77.5f) / 1.4f);
                    Vector3 seat = wreck->LocalToWorld({ kPassengerX, 0.0f, kSeatZ });
                    Vector3 outp = Vector3Lerp(seat, road, u * u);
                    outp.y = Lerp(seat.y, World().Height(outp.x, outp.z) + 0.15f, u) + sinf(u * PI) * 0.7f;
                    zain->pos = outp;
                    zain->pitch = -u * 1.45f;
                    zain->SetPose(PoseDragged(T));
                }
            } else {
                if (Cross(T, dt, 79.0f) && zain) {
                    audio::Play3D("body_thud", zain->pos, 1.0f, 1.0f, 3.0f, 50.0f);
                    fx.Burst(P_BLOOD, Vector3Add(zain->pos, { 0, 0.3f, 0 }), 20, 1.5f, 1.0f, 0.02f, Color{ 90, 5, 5, 230 });
                }
                // drag south along the road
                float u = fmaxf(0.0f, T - 80.5f);
                Vector3 start = road;
                float z = 156.0f - u * 1.35f;
                Vector3 p = RoadPoint(z, 1.2f + sinf(z * 0.07f) * 0.6f);
                if (T < 80.5f) {
                    Vector3 toFeet = Vector3Lerp(dr->pos, Vector3Add(start, { 0, 0, 1.9f }), Saturate((T - 79.0f) / 1.5f));
                    dr->pos = toFeet; dr->pos.y = World().Height(toFeet.x, toFeet.z);
                    dr->SetPose(PoseWalk(T * 0.6f, 0.8f, 1));
                } else {
                    dr->pos = p;
                    dr->yaw = PI + RoadYaw(z);
                    dr->SetPose(PoseDragging(T * 0.5f));
                    if (fmodf(T, 1.9f) < dt) audio::PlayFootstep(SURF_ASPHALT, dr->pos, 0.5f, false);
                    a.drag = 0.5f * Saturate(1.0f - Vector3Distance(p, gg.camera.position) / 45.0f) + 0.1f;
                    if (zain) {
                        Vector3 zp = RoadPoint(z + 2.0f, 1.2f + sinf((z + 2) * 0.07f) * 0.6f);
                        zain->pos = zp;
                        zain->pos.y += 0.12f;
                        zain->yaw = RoadYaw(z);
                        zain->pitch = -1.45f;
                        zain->SetPose(PoseDragged(T));
                        // the clue: he doesn't struggle - and he looks up at you
                        if (T > 86.0f && T < 90.0f) { zain->lookAt = gg.camera.position; zain->lookWeight = Damp(zain->lookWeight, 1.0f, 2.0f, dt); }
                        else zain->lookWeight = Damp(zain->lookWeight, 0.0f, 2.0f, dt);
                    }
                }
            }
        }
        // once the soul settles above the car, the camera follows the Dragger and Zain
        if (T > 70.0f && wreck && gg.A("dragger")) {
            EndCamPath();
            static Vector3 focus{};
            Actor* d = gg.A("dragger");
            Vector3 want = d->HeadPos();
            if (zain && T > 77.5f) want = Vector3Lerp(want, zain->BonePos(B_CHEST), 0.4f);
            if (T < 70.1f) focus = want;
            focus = DampV(focus, want, 2.0f, dt);
            Vector3 pos = wreck->LocalToWorld({ 3.6f, 4.6f, -2.8f });
            Vector3 dir = Vector3Normalize(Vector3Subtract(focus, pos));
            float yaw = atan2f(dir.x, dir.z) + lookOffset.x, pitch = asinf(Clamp(dir.y, -1.0f, 1.0f)) + lookOffset.y;
            gg.camOverride = true;
            gg.camOverrideCam.position = pos;
            gg.camOverrideCam.target = Vector3Add(pos, DirFromYawPitch(yaw, pitch));
            gg.camOverrideCam.up = { 0, 1, 0 };
            gg.camOverrideCam.fovy = 55.0f;
        }
        if (getenv("WTGK_DEBUG") && fmodf(T, 1.0f) < dt) {
            Actor* dd = gg.A("dragger");
            fprintf(stderr, "T=%.1f cam=(%.1f %.1f %.1f) tgt=(%.1f %.1f %.1f) dragger=%s %.1f %.1f %.1f wreck=%d lights=%d\n", T,
                gg.camOverrideCam.position.x, gg.camOverrideCam.position.y, gg.camOverrideCam.position.z,
                gg.camOverrideCam.target.x, gg.camOverrideCam.target.y, gg.camOverrideCam.target.z,
                dd ? "yes" : "no", dd ? dd->pos.x : 0, dd ? dd->pos.y : 0, dd ? dd->pos.z : 0, wreck ? 1 : 0, (int)wreckLights);
        }
        // spirit POV: free look, no movement
        if (allowLook) {
            ReadLook(lookOffset, gg.settings.sensitivity, 1.4f, 0.8f);
            if (!gg.shotFile && (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D))) {
                gg.camShake = 0.25f;
                gg.hud.Hint("you can't move. you can't do anything.", 2.0f);
            }
        }
        if (Cross(T, dt, 93.0f)) { audio::Play("stinger_low", 0.8f); a.drag = 0; }
        if (Cross(T, dt, 96.0f)) gg.hud.FadeTo(1.0f, 1.0f);
        // ---------------- camera for the drive ----------------
        if (T < Timpact + 0.25f) {
            ReadLook(lookOffset, gg.settings.sensitivity, 1.7f, 0.9f);
            gg.camOverride = true;
            gg.camOverrideCam = DriverCam(car, lookOffset, 68.0f + (T > Tyank ? 6.0f : 0.0f), gg.camShake);
            Seat(adam, car.Xf(), car.yaw, kDriverX);
            Seat(zain, car.Xf(), car.yaw, kPassengerX);
        }
        return T > 98.0f;
    });
    s.Do([this, &gg]() {
        audio::Amb().muffle = 0; audio::Amb().tinnitus = 0; audio::Amb().drag = 0;
        Rdr().s.asciiFull = 0.0f;
        allowLook = false;
        EndCamPath();
        gg.hud.hidden = false;
        gg.story->pendingChapter = CH_AWAKENING;
    });
}

// ---------------------------------------------------------------------------
// Awakening: eyes open on the road; follow the marks to Grethnar
// ---------------------------------------------------------------------------
void Story::Impl::BuildAwakening(Script& s) {
    Game& gg = g;
    s.Do([this, &gg]() {
        gg.story->cutscene = true;
        gg.story->skippable = false;
        wreckLights = true;
        gg.hud.FadeInstant(1.0f);
        gg.hud.Letterbox(false);
        gg.player.Spawn({ 11.2f, 0.5f, 153.0f }, 190.0f);
        Rdr().s.blink = 1.0f;
        Rdr().s.desat = 0.3f;
        auto& a = audio::Amb();
        a.muffle = 0.8f; a.tinnitus = 0.3f; a.engine = 0; a.road = 0; a.cabin = 0; a.indoor = 0;
        // your body is still in the car
        if (Entity* w = Scn().Find("wreck")) {
            Actor* body = gg.SpawnActor("adam_body", SpecAdam(), w->base, 0);
            Seat(body, w->xf, w->worldYaw, kDriverX);
            body->SetPose(PoseSlumped(0), true);
        }
        Vector3 lie = { 11.2f, World().Height(11.2f, 153.0f) + 0.25f, 153.0f };
        SetCamPath({
            { 0.0f, lie, Vector3Add(lie, { 0.3f, 1.2f, -2.0f }), 70 },
            { 7.0f, lie, Vector3Add(lie, { 0.8f, 0.3f, -3.0f }), 70 },
            { 9.5f, Vector3Add(lie, { 0.0f, 0.8f, 0.0f }), Vector3Add(lie, { 0.3f, 0.9f, -4.0f }), 70 },
            { 11.0f, gg.player.EyePos(), Vector3Add(gg.player.EyePos(), DirFromYawPitch(190.0f * DEG2RAD, 0)), gg.settings.fov },
        });
    });
    s.Run([this, &gg](float t, float dt) {
        auto& rs = Rdr().s;
        auto& a = audio::Amb();
        // eyelids: flutter, close, open
        float b = 1.0f;
        if (t > 1.5f) b = 1.0f - 0.7f * Saturate((t - 1.5f) / 0.5f);
        if (t > 2.4f) b = 0.3f + 0.7f * Saturate((t - 2.4f) / 0.25f);
        if (t > 3.2f) b = 1.0f - Saturate((t - 3.2f) / 0.6f) * 0.9f;
        if (t > 4.6f) b = 0.1f + 0.9f * Saturate((t - 4.6f) / 0.15f);
        if (t > 4.9f) b = 1.0f - Saturate((t - 4.9f) / 1.2f);
        rs.blink = b;
        if (Cross(t, dt, 0.3f)) gg.hud.FadeTo(0.0f, 0.5f);
        if (Cross(t, dt, 1.6f)) audio::Play("gasp", 0.8f);
        if (Cross(t, dt, 5.0f)) audio::Play("gasp", 0.6f);
        a.muffle = Damp(a.muffle, 0.0f, 0.4f, dt);
        a.tinnitus = Damp(a.tinnitus, 0.0f, 0.3f, dt);
        rs.desat = Damp(rs.desat, 0.2f, 0.3f, dt);
        return t > 11.0f;
    });
    s.Do([this, &gg]() {
        EndCamPath();
        gg.camOverride = false;
        gg.story->cutscene = false;
        Rdr().s.blink = 0.0f;
        gg.hud.Title("AWAKENING", "the ground keeps what it is given", 5.0f);
    });
    s.Wait(3.0f).Say("ADAM", "Zain...?", 2.0f).Objective("Follow the marks.");
    s.Do([&gg]() { gg.hud.Hint("Your torch is still in your jacket.   [F]", 6.0f); });
    // wait until the player walks south along the trail
    s.Until([&gg]() { return gg.player.feet.z < 98.0f; });
    s.Do([this, &gg]() {
        // a figure between the trees, gone when you look straight at it
        Actor* d = gg.SpawnActor("watcher", SpecDragger(), { 38.0f, World().Height(38.0f, 72.0f), 72.0f }, -90.0f);
        d->SetPose(PoseLoom(0), true);
        audio::Amb().crickets = 0.0f;
        audio::Amb().threat = 1.0f;
    });
    s.Run([&gg](float t, float dt) {
        (void)dt;
        Actor* d = gg.A("watcher");
        if (!d) return true;
        Vector3 to = Vector3Subtract(d->HeadPos(), gg.player.EyePos());
        float dot = Vector3DotProduct(Vector3Normalize(to), gg.player.Forward());
        d->lookAt = gg.player.EyePos(); d->lookWeight = 1.0f;
        if ((dot > 0.97f && t > 0.5f) || t > 25.0f) {
            gg.RemoveActor("watcher");
            audio::Play("twig", 0.7f);
            gg.player.fear = 0.8f;
            audio::Amb().threat = 0.0f;
            return true;
        }
        if (dot > 0.9f) gg.player.fear = fmaxf(gg.player.fear, 0.4f);
        return false;
    });
    s.Until([&gg]() { return gg.player.feet.z < 45.0f; });
    s.Do([this]() { phoneRingT = 0.0f; });
    s.Objective("Answer the phone.");
    s.Until([&gg]() { return gg.Flag("phone_answered_1"); });
    s.Objective("Follow the marks.");
    s.Until([&gg]() { return gg.Flag("hatch_seen"); });
    s.Objective("Find someone. Anyone.");
    s.Until([&gg]() { return gg.Flag("talk_request"); });
    // --- meeting Grethnar ---
    s.Do([&gg]() { gg.flags.erase("talk_request"); gg.player.locked = true; gg.hud.Objective(""); });
    s.Say("GRETHNAR", "Well. You're a fresh one.")
     .Say("GRETHNAR", "You made the bell ring. Most of you are too light for that, the first night.")
     .Say("ADAM", "My brother. Something took my brother - on the road, it-")
     .Say("GRETHNAR", "The ground took him. The ground takes a lot of things on this road.")
     .Say("GRETHNAR", "I can tell you where it keeps them. But nothing's free out here, son.")
     .Say("GRETHNAR", "Work my station. Three nights. Pump the gas. Stock the shelves. Keep the floor clean.")
     .Say("GRETHNAR", "Every night you finish, I tell you one true thing about your brother.");
    s.Do([this]() { Choose({ "\"Fine. Three nights.\"", "\"I'll find him myself.\"" }); });
    s.Until([this]() { return choiceResult >= 0; });
    // refusing is allowed: he lets you walk, and the road brings you back to him
    s.Run([this, &gg](float, float) {
        if (choiceResult == 0) return true;
        if (choiceResult == 1) {
            choiceResult = -3;
            gg.player.locked = false;
            gg.flags.erase("talk_request");
            gg.hud.Say("GRETHNAR", "Then walk. See how far the road goes.", 3.0f);
            gg.hud.Objective("Find him yourself.");
        }
        if (choiceResult == -3 && gg.Flag("talk_request")) {
            gg.flags.erase("talk_request");
            gg.player.locked = true;
            gg.hud.Say("GRETHNAR", "Back already. They always come back.", 3.0f);
            Choose({ "\"Fine. Three nights.\"", "\"No.\"" });
        }
        return false;
    });
    s.Do([&gg]() { gg.player.locked = true; });
    s.Say("GRETHNAR", "Good boy.")
     .Say("GRETHNAR", "Your shift starts now. The list's in your head already, isn't it. Funny how that works.", 4.0f)
     .Do([&gg]() { gg.player.locked = false; gg.SetFlag("deal"); gg.story->pendingChapter = CH_SHIFT1; });
}
