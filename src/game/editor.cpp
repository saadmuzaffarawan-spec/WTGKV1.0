#include "editor.h"
#include "game.h"
#include <rlgl.h>

using namespace ui;

namespace {
Vector3 camPos{};
float yaw = 0, pitch = 0;
Entity* sel = nullptr;
int paletteIdx = 0;
bool grabbing = false;
std::string status;
float statusT = 0;
std::vector<std::string> palette;

void Status(const std::string& s) { status = s; statusT = 3.0f; }

Ray MouseRay() {
    Camera3D c = EditorCamera();
    return GetScreenToWorldRay(GetMousePosition(), c);
}

Entity* Pick(Ray r, float* outT = nullptr) {
    Entity* best = nullptr; float bestT = 1e9f;
    for (auto& up : Scn().ents) {
        Entity& e = *up;
        if (!e.model || e.tag == "runtime") continue;
        Matrix inv = MatrixInvert(e.xf);
        Vector3 o = Vector3Transform(r.position, inv);
        Vector3 d = Vector3Normalize(XfDir(inv, r.direction));
        BoundingBox bb{ e.model->bmin, e.model->bmax };
        RayCollision rc = GetRayCollisionBox(Ray{ o, d }, bb);
        if (!rc.hit) continue;
        float t = Vector3Distance(r.position, Vector3Transform(rc.point, e.xf));
        if (t < bestT) { bestT = t; best = &e; }
    }
    if (outT) *outT = bestT;
    return best;
}

bool GroundPoint(Ray r, Vector3& out) {
    RayHit h;
    if (Phys().Raycast(r.position, r.direction, 400.0f, h)) { out = h.point; return true; }
    return false;
}

void SetWorldPos(Entity& e, Vector3 world) {
    if (e.Has("parent")) {
        if (Entity* p = Scn().Find(e.Str("parent"))) {
            Vector3 l = Vector3Transform(world, MatrixInvert(MatPose(p->base, p->worldYaw)));
            e.pos = { l.x, e.pos.y, l.z };
        }
    } else {
        e.pos.x = world.x; e.pos.z = world.z;
    }
    Scn().UpdateTransform(e);
}
}  // namespace

Camera3D EditorCamera() {
    Camera3D c{};
    c.position = camPos;
    c.target = Vector3Add(camPos, DirFromYawPitch(yaw, pitch));
    c.up = { 0, 1, 0 };
    c.fovy = 70;
    c.projection = CAMERA_PERSPECTIVE;
    return c;
}

void EditorEnter() {
    camPos = G().player.EyePos();
    yaw = G().player.yaw; pitch = G().player.pitch;
    EnableCursor();
    palette.clear();
    for (auto& p : AllPrefabs()) if (p.category != "internal") palette.push_back(p.name);
    Status("editor: F10 to return, Ctrl+S to save");
}

void EditorUpdate(float dt) {
    Game& g = G();
    statusT -= dt;
    if (IsKeyPressed(KEY_F10) || IsKeyPressed(KEY_ESCAPE)) {
        g.mode = Mode::Play;
        g.player.Spawn({ camPos.x, camPos.y - 1.6f, camPos.z }, yaw * RAD2DEG);
        g.player.pitch = pitch;
        DisableCursor();
        return;
    }
    // fly camera (hold right mouse to look)
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        Vector2 md = GetMouseDelta();
        yaw -= md.x * 0.003f; pitch = Clamp(pitch - md.y * 0.003f, -1.5f, 1.5f);
    }
    Vector3 f = DirFromYawPitch(yaw, pitch), r = Vector3Normalize(Vector3CrossProduct(f, { 0, 1, 0 }));
    float sp = (IsKeyDown(KEY_LEFT_SHIFT) ? 30.0f : 8.0f) * dt;
    bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    if (!ctrl) {
        if (IsKeyDown(KEY_W)) camPos = Vector3Add(camPos, Vector3Scale(f, sp));
        if (IsKeyDown(KEY_S)) camPos = Vector3Subtract(camPos, Vector3Scale(f, sp));
        if (IsKeyDown(KEY_D)) camPos = Vector3Add(camPos, Vector3Scale(r, sp));
        if (IsKeyDown(KEY_A)) camPos = Vector3Subtract(camPos, Vector3Scale(r, sp));
        if (IsKeyDown(KEY_E)) camPos.y += sp;
        if (IsKeyDown(KEY_Q)) camPos.y -= sp;
    }
    Ray ray = MouseRay();
    if (grabbing && sel) {
        Vector3 gp;
        if (GroundPoint(ray, gp)) SetWorldPos(*sel, gp);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { grabbing = false; Status("placed"); }
        return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { sel = Pick(ray); if (sel) Status("selected " + sel->prefab); }
    if (IsKeyPressed(KEY_TAB)) { paletteIdx = (paletteIdx + (IsKeyDown(KEY_LEFT_SHIFT) ? (int)palette.size() - 1 : 1)) % (int)palette.size(); }
    if (IsKeyPressed(KEY_P) && !palette.empty()) {
        Vector3 gp;
        if (GroundPoint(ray, gp)) {
            sel = Scn().Spawn(palette[paletteIdx], { gp.x, 0, gp.z }, { yaw * RAD2DEG + 180.0f, 0, 0 });
            Status("placed " + palette[paletteIdx]);
        }
    }
    if (!sel) {
        if (ctrl && IsKeyPressed(KEY_S)) { Scn().Save(Scn().path); if (FileExists("../assets/scenes/world.scene")) Scn().Save("../assets/scenes/world.scene"); Status("saved world.scene"); }
        return;
    }
    Entity& e = *sel;
    float step = IsKeyDown(KEY_LEFT_SHIFT) ? 0.05f : (IsKeyDown(KEY_LEFT_ALT) ? 1.0f : 0.25f);
    // move relative to the camera's dominant axes
    Vector3 fwdAxis = fabsf(f.x) > fabsf(f.z) ? Vector3{ f.x > 0 ? 1.0f : -1.0f, 0, 0 } : Vector3{ 0, 0, f.z > 0 ? 1.0f : -1.0f };
    Vector3 rightAxis{ -fwdAxis.z, 0, fwdAxis.x };
    Vector3 delta{};
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) delta = Vector3Add(delta, fwdAxis);
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) delta = Vector3Subtract(delta, fwdAxis);
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) delta = Vector3Subtract(delta, rightAxis);
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) delta = Vector3Add(delta, rightAxis);
    if (Vector3LengthSqr(delta) > 0) SetWorldPos(e, Vector3Add(e.base, Vector3Scale(delta, step)));
    if (IsKeyPressed(KEY_PAGE_UP) || IsKeyPressedRepeat(KEY_PAGE_UP)) { e.pos.y += step; Scn().UpdateTransform(e); }
    if (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressedRepeat(KEY_PAGE_DOWN)) { e.pos.y -= step; Scn().UpdateTransform(e); }
    if (IsKeyPressed(KEY_R) || IsKeyPressedRepeat(KEY_R)) { e.rot.x += (IsKeyDown(KEY_LEFT_SHIFT) ? -1 : 1) * (ctrl ? 1.0f : 15.0f); Scn().UpdateTransform(e); }
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_MINUS)) {
        float k = IsKeyPressed(KEY_EQUAL) ? 1.1f : 1.0f / 1.1f;
        e.scale = Vector3Scale(e.scale, k);
        e.props["scale"] = TextFormat("%.2f", e.scale.x);
        Scn().UpdateTransform(e);
    }
    if (IsKeyPressed(KEY_V)) {
        e.props["variant"] = std::to_string((int)e.Num("variant", 0) + 1);
        Scn().Rebuild(e);
    }
    if (IsKeyPressed(KEY_G)) { grabbing = true; Status("grab: click to drop"); }
    if (ctrl && IsKeyPressed(KEY_D)) {
        auto props = e.props; props.erase("name");
        sel = Scn().Spawn(e.prefab, Vector3Add(e.pos, { 1.0f, 0, 1.0f }), e.rot, props, e.absY);
        Status("duplicated");
    }
    if (IsKeyPressed(KEY_DELETE)) { Scn().Remove(sel); sel = nullptr; Status("deleted"); return; }
    if (ctrl && IsKeyPressed(KEY_S)) {
        Scn().Save(Scn().path);
        if (FileExists("../assets/scenes/world.scene")) Scn().Save("../assets/scenes/world.scene");
        Status("saved world.scene");
    }
}

void EditorDraw() {
    float s = UiScale();
    Camera3D c = EditorCamera();
    if (sel && sel->model) {
        Vector3 mn = sel->model->bmin, mx = sel->model->bmax;
        Vector3 corners[8];
        for (int i = 0; i < 8; i++)
            corners[i] = Vector3Transform({ (i & 1) ? mx.x : mn.x, (i & 2) ? mx.y : mn.y, (i & 4) ? mx.z : mn.z }, sel->xf);
        const int edges[12][2] = { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 }, { 4, 6 }, { 5, 7 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
        for (auto& ed : edges) {
            Vector3 a = corners[ed[0]], b = corners[ed[1]];
            Vector3 fa = Vector3Subtract(a, c.position), fb = Vector3Subtract(b, c.position);
            Vector3 fwd = Vector3Subtract(c.target, c.position);
            if (Vector3DotProduct(fa, fwd) < 0 || Vector3DotProduct(fb, fwd) < 0) continue;
            DrawLineEx(GetWorldToScreen(a, c), GetWorldToScreen(b, c), 1.5f, Color{ 240, 200, 80, 200 });
        }
    }
    Panel({ 16 * s, 16 * s, 420 * s, 250 * s }, 0.9f);
    float y = 26 * s;
    Text(F_MONO_BOLD, "WORLD EDITOR", 28 * s, y, 15 * s, kInk, 2 * s); y += 24 * s;
    if (sel) {
        Text(F_MONO, TextFormat("%s  %s", sel->prefab.c_str(), sel->name.c_str()), 28 * s, y, 14 * s, Color{ 240, 200, 80, 255 }); y += 20 * s;
        Text(F_MONO, TextFormat("pos %.2f %.2f %.2f  yaw %.0f", sel->pos.x, sel->pos.y, sel->pos.z, sel->rot.x), 28 * s, y, 13 * s, kDim); y += 18 * s;
        std::string props;
        for (auto& kv : sel->props) props += kv.first + "=" + kv.second + " ";
        TextWrapped(F_MONO_LIGHT, props.c_str(), 28 * s, y, 390 * s, 12 * s, kFaint); y += 34 * s;
    } else { Text(F_MONO_LIGHT, "click an object to select it", 28 * s, y, 13 * s, kDim); y += 20 * s; }
    Text(F_MONO_LIGHT, TextFormat("palette [Tab]: %s   P = place", palette.empty() ? "-" : palette[paletteIdx].c_str()), 28 * s, y, 13 * s, kInk); y += 22 * s;
    const char* help = "RMB+WASD/QE fly  arrows move  PgUp/Dn height  R rotate\nG grab  V variant  +/- scale  Ctrl+D duplicate  Del delete\nShift fine  Alt coarse  Ctrl+S save  F10 exit";
    TextWrapped(F_MONO_LIGHT, help, 28 * s, y, 390 * s, 12 * s, kDim, 1.5f);
    if (statusT > 0) TextCentered(F_MONO, status.c_str(), GetScreenWidth() * 0.5f, GetScreenHeight() - 50 * s, 15 * s, Alpha(kInk, Saturate(statusT)));
}
