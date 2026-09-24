// Temporary world viewer (replaced by the game entry point).
#include "engine/renderer.h"
#include "engine/scene.h"
#include "engine/ui.h"
#include "game/vegetation.h"
#include "game/prefab_util.h"
#include "game/characters.h"
#include <rlgl.h>
#include <cstring>
#include <cstdlib>

void RegisterAllPrefabs();

int main(int argc, char** argv) {
    const char* shot = nullptr;
    Vector3 camPos{ 0, 1.7f, -10 };
    float yaw = 0, pitch = 0;
    bool flash = true;
    float bright = 1.0f;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--shot") && i + 1 < argc) shot = argv[++i];
        else if (!strcmp(argv[i], "--cam") && i + 1 < argc) {
            sscanf(argv[++i], "%f,%f,%f,%f,%f", &camPos.x, &camPos.y, &camPos.z, &yaw, &pitch);
            yaw *= DEG2RAD; pitch *= DEG2RAD;
        } else if (!strcmp(argv[i], "--noflash")) flash = false;
        else if (!strcmp(argv[i], "--bright") && i + 1 < argc) bright = (float)atof(argv[++i]);
    }
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "WTGK viewer");
    ui::Init();
    GenerateTextureSets(512);
    InitMaterials();
    Rdr().Init();
    RegisterAllPrefabs();

    Scn().Parse("assets/scenes/world.scene");
    std::vector<TerrainPad> pads; std::vector<Rectangle> holes; std::vector<TerrainPath> paths;
    Scn().CollectTerrain(pads, holes, paths);
    World().Build(pads, holes, paths);
    World().RegisterCollision();
    Scn().SpawnRecords();
    BuildPowerLines(Scn());
    Veg().Build(pads, {}, 1234);
    Rdr().s.brightness = bright;
    camPos.y += World().Height(camPos.x, camPos.z);
    printf("entities=%zu terrainH=%.2f boxes=%zu\n", Scn().ents.size(), World().Height(camPos.x, camPos.z), Phys().Boxes().size());

    std::vector<Actor> cast;
    bool lineup = false;
    for (int i = 1; i < argc; i++) if (!strcmp(argv[i], "--lineup")) lineup = true;
    if (lineup) {
        BodySpec specs[] = { SpecAdam(), SpecZain(), SpecGrethnar(), SpecDragger(), SpecOldMan(), SpecCustomer(3), SpecCrawler(4), SpecPigMan(5), SpecGoatMan(6) };
        int n = 0;
        for (auto& sp : specs) {
            Actor a; a.model = BuildCharacter(sp);
            a.pos = { 14.0f + n * 1.4f, World().Height(14.0f + n * 1.4f, -8.0f) + 0.03f, -8.0f };
            a.yaw = PI;
            a.SetPose(n == 6 ? PoseCrawl(0.2f) : (n == 3 ? PoseWalk(0.25f, 1.0f, 1) : PoseStand(0)), true);
            cast.push_back(a); n++;
        }
    }
    int frame = 0;
    if (!shot) DisableCursor();
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Vector2 md = shot ? Vector2{ 0, 0 } : GetMouseDelta();
        yaw -= md.x * 0.003f; pitch -= md.y * 0.003f;
        pitch = Clamp(pitch, -1.5f, 1.5f);
        Vector3 fwd = DirFromYawPitch(yaw, pitch);
        Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, { 0, 1, 0 }));
        float sp = IsKeyDown(KEY_LEFT_SHIFT) ? 25.0f : 6.0f;
        if (IsKeyDown(KEY_W)) camPos = Vector3Add(camPos, Vector3Scale(fwd, sp * dt));
        if (IsKeyDown(KEY_S)) camPos = Vector3Subtract(camPos, Vector3Scale(fwd, sp * dt));
        if (IsKeyDown(KEY_D)) camPos = Vector3Add(camPos, Vector3Scale(right, sp * dt));
        if (IsKeyDown(KEY_A)) camPos = Vector3Subtract(camPos, Vector3Scale(right, sp * dt));
        Camera3D cam{};
        cam.position = camPos; cam.target = Vector3Add(camPos, fwd); cam.up = { 0, 1, 0 }; cam.fovy = 65; cam.projection = CAMERA_PERSPECTIVE;
        Scn().Update(dt);
        Rdr().BeginFrame(cam, (float)GetTime());
        Scn().SubmitLights(camPos);
        if (lineup) {
            Light key; key.pos = Vector3Add(camPos, { 0.8f, 0.5f, 0.3f }); key.color = { 1.0f, 0.9f, 0.8f }; key.intensity = 3.0f; key.range = 6.0f;
            Rdr().AddLight(key);
        }
        if (flash) {
            Light fl; fl.spot = true; fl.pos = Vector3Add(camPos, Vector3Add(Vector3Scale(right, 0.2f), { 0, -0.2f, 0 })); fl.dir = fwd;
            fl.color = { 1.0f, 0.93f, 0.8f }; fl.intensity = 30; fl.range = 28; fl.innerDeg = 10; fl.outerDeg = 24; fl.shadow = true; fl.volumetric = 0.25f;
            Rdr().AddLight(fl);
        }
        World().Draw();
        Veg().Draw(camPos);
        Scn().Draw();
        for (auto& a : cast) a.Draw();
        Rdr().Render();
        BeginDrawing();
        ClearBackground(BLACK);
        Rdr().Present();
        DrawText(TextFormat("%d fps  %d draws  pos %.1f %.1f %.1f", GetFPS(), Rdr().drawCalls, camPos.x, camPos.y, camPos.z), 10, 10, 10, GREEN);
        EndDrawing();
        if (shot && ++frame == 4) { TakeScreenshot(shot); break; }
    }
    Rdr().Shutdown();
    CloseWindow();
    return 0;
}
