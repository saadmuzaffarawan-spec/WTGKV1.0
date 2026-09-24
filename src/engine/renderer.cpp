#include "renderer.h"
#include "shaders.h"
#include <rlgl.h>
#include <algorithm>
#include <unordered_map>

#ifndef MAX_MATERIAL_MAPS
#define MAX_MATERIAL_MAPS 12
#endif

static Renderer g_renderer;
Renderer& Rdr() { return g_renderer; }

// ---------------------------------------------------------------------------
// Render target helpers
// ---------------------------------------------------------------------------
RenderTexture2D LoadRenderTextureHDR(int w, int h, bool depth) {
    RenderTexture2D t{};
    t.id = rlLoadFramebuffer();
    rlEnableFramebuffer(t.id);
    t.texture.id = rlLoadTexture(nullptr, w, h, PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, 1);
    t.texture.width = w; t.texture.height = h; t.texture.mipmaps = 1;
    t.texture.format = PIXELFORMAT_UNCOMPRESSED_R16G16B16A16;
    rlFramebufferAttach(t.id, t.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
    if (depth) {
        t.depth.id = rlLoadTextureDepth(w, h, false);
        t.depth.width = w; t.depth.height = h; t.depth.mipmaps = 1; t.depth.format = 19;
        rlFramebufferAttach(t.id, t.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    }
    if (!rlFramebufferComplete(t.id)) TraceLog(LOG_WARNING, "HDR framebuffer incomplete");
    rlDisableFramebuffer();
    SetTextureFilter(t.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(t.texture, TEXTURE_WRAP_CLAMP);
    return t;
}

RenderTexture2D LoadShadowTarget(int size) {
    RenderTexture2D t{};
    t.id = rlLoadFramebuffer();
    t.texture.width = size; t.texture.height = size;
    rlEnableFramebuffer(t.id);
    t.depth.id = rlLoadTextureDepth(size, size, false);
    t.depth.width = size; t.depth.height = size; t.depth.format = 19; t.depth.mipmaps = 1;
    rlFramebufferAttach(t.id, t.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);
    if (!rlFramebufferComplete(t.id)) TraceLog(LOG_WARNING, "Shadow framebuffer incomplete");
    rlDisableFramebuffer();
    return t;
}

static void UnloadRT(RenderTexture2D& t) {
    if (t.id == 0) return;
    if (t.texture.id) rlUnloadTexture(t.texture.id);
    if (t.depth.id) rlUnloadTexture(t.depth.id);
    rlUnloadFramebuffer(t.id);
    t = RenderTexture2D{};
}

// ---------------------------------------------------------------------------
// Uniform location cache
// ---------------------------------------------------------------------------
static std::unordered_map<std::string, int> g_locCache;
static int L(Shader sh, const char* name) {
    std::string key = std::to_string(sh.id) + ":" + name;
    auto it = g_locCache.find(key);
    if (it != g_locCache.end()) return it->second;
    int loc = GetShaderLocation(sh, name);
    g_locCache[key] = loc;
    return loc;
}
static void U1(Shader sh, const char* n, float v) { int l = L(sh, n); if (l >= 0) SetShaderValue(sh, l, &v, SHADER_UNIFORM_FLOAT); }
static void U3(Shader sh, const char* n, Vector3 v) { int l = L(sh, n); if (l >= 0) SetShaderValue(sh, l, &v, SHADER_UNIFORM_VEC3); }
static void U4(Shader sh, const char* n, Vector4 v) { int l = L(sh, n); if (l >= 0) SetShaderValue(sh, l, &v, SHADER_UNIFORM_VEC4); }
static void U2(Shader sh, const char* n, Vector2 v) { int l = L(sh, n); if (l >= 0) SetShaderValue(sh, l, &v, SHADER_UNIFORM_VEC2); }
static void UI(Shader sh, const char* n, int v) { int l = L(sh, n); if (l >= 0) SetShaderValue(sh, l, &v, SHADER_UNIFORM_INT); }
static void UM(Shader sh, const char* n, Matrix m) { int l = L(sh, n); if (l >= 0) SetShaderValueMatrix(sh, l, m); }

static std::string WithDefine(const char* src, const char* define) {
    std::string s(src);
    size_t nl = s.find('\n', s.find("#version"));
    return s.substr(0, nl + 1) + define + "\n" + s.substr(nl + 1);
}

static void SetupLitLocs(Shader& sh, bool instanced) {
    sh.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(sh, "matView");
    sh.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(sh, "matProjection");
    if (instanced) sh.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(sh, "instanceTransform");
    else sh.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(sh, "matModel");
    sh.locs[SHADER_LOC_COLOR_DIFFUSE] = GetShaderLocation(sh, "colDiffuse");
    sh.locs[SHADER_LOC_MAP_ALBEDO] = GetShaderLocation(sh, "texture0");
    sh.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(sh, "texture1");
    sh.locs[SHADER_LOC_MAP_NORMAL] = GetShaderLocation(sh, "texture2");
    sh.locs[SHADER_LOC_MAP_ROUGHNESS] = GetShaderLocation(sh, "texture3");
    sh.locs[SHADER_LOC_MAP_OCCLUSION] = GetShaderLocation(sh, "texture4");
    sh.locs[SHADER_LOC_MAP_EMISSION] = GetShaderLocation(sh, "texture5");
    sh.locs[SHADER_LOC_MAP_HEIGHT] = GetShaderLocation(sh, "texture6");
    sh.locs[SHADER_LOC_MAP_BRDF] = GetShaderLocation(sh, "texture10");
}

static MaterialMap g_maps[MAX_MATERIAL_MAPS];

// Level of detail for a mesh seen from `dist` metres.
static const MeshAsset* PickLod(const MeshAsset* a, float dist) {
    while (a->lod && dist > a->lodDist) a = a->lod;
    return a;
}
static const MeshAsset* CoarsestLod(const MeshAsset* a) {
    while (a->lod) a = a->lod;
    return a;
}

// A mesh asset can be split into several indexed chunks; draw them all.
static void DrawAsset(const MeshAsset* a, const Material& m, const Matrix& xf) {
    DrawMesh(a->mesh, m, xf);
    for (const Mesh& c : a->more) DrawMesh(c, m, xf);
}

void Renderer::Init() {
    rlSetClipPlanes(0.05, 900.0);
    std::string common = std::string("#version 330\n") + kShaderCommon;
    std::string litFS = common + kLitFS;
    lit_ = LoadShaderFromMemory(kLitVS, litFS.c_str());
    SetupLitLocs(lit_, false);
    std::string instVS = WithDefine(kLitVS, "#define INSTANCED");
    litInst_ = LoadShaderFromMemory(instVS.c_str(), litFS.c_str());
    SetupLitLocs(litInst_, true);
    depth_ = LoadShaderFromMemory(kDepthVS, kDepthFS);
    std::string skyFS = common + kSkyFS;
    sky_ = LoadShaderFromMemory(kSkyVS, skyFS.c_str());
    sky_.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(sky_, "matModel");
    sky_.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(sky_, "matView");
    sky_.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(sky_, "matProjection");
    sky_.locs[SHADER_LOC_MAP_METALNESS] = GetShaderLocation(sky_, "texture1");
    std::string partFS = common + kParticleFS;
    particleShader = LoadShaderFromMemory(kParticleVS, partFS.c_str());
    bright_ = LoadShaderFromMemory(kPostVS, kBrightFS);
    blur_ = LoadShaderFromMemory(kPostVS, kBlurFS);
    composite_ = LoadShaderFromMemory(kPostVS, kCompositeFS);
    std::string scatterFS = common + kScatterFS;
    scatter_ = LoadShaderFromMemory(kPostVS, scatterFS.c_str());
    add_ = LoadShaderFromMemory(kPostVS, kAddFS);

    skyBox_ = GenMeshCube(1.0f, 1.0f, 1.0f);
    skyMat_ = LoadMaterialDefault();
    skyMat_.shader = sky_;
    depthMat_ = LoadMaterialDefault();
    depthMat_.shader = depth_;

    moonShadow_ = LoadShadowTarget(2048);
    spotShadow_ = LoadShadowTarget(1024);
    radialTex = MakeRadialTexture(64, 1.0f);
    EnsureTargets();
}

void Renderer::Shutdown() {
    UnloadShader(lit_); UnloadShader(litInst_); UnloadShader(depth_); UnloadShader(sky_);
    UnloadShader(bright_); UnloadShader(blur_); UnloadShader(composite_); UnloadShader(particleShader);
    UnloadShader(scatter_); UnloadShader(add_); UnloadRT(scatterRT_);
    UnloadRT(hdr_); UnloadRT(bloomA_); UnloadRT(bloomB_); UnloadRT(bloomC_); UnloadRT(bloomD_);
    UnloadRT(moonShadow_); UnloadRT(spotShadow_);
}

void Renderer::EnsureTargets() {
    int w = (int)(GetScreenWidth() * s.renderScale), h = (int)(GetScreenHeight() * s.renderScale);
    if (w < 64) w = 64;
    if (h < 64) h = 64;
    if (w == lastW_ && h == lastH_ && hdr_.id) return;
    UnloadRT(hdr_); UnloadRT(bloomA_); UnloadRT(bloomB_); UnloadRT(bloomC_); UnloadRT(bloomD_); UnloadRT(scatterRT_);
    hdr_ = LoadRenderTextureHDR(w, h, true);
    scatterRT_ = LoadRenderTextureHDR(w, h, false);
    bloomA_ = LoadRenderTextureHDR(w / 2, h / 2, false);
    bloomB_ = LoadRenderTextureHDR(w / 2, h / 2, false);
    bloomC_ = LoadRenderTextureHDR(w / 4, h / 4, false);
    bloomD_ = LoadRenderTextureHDR(w / 4, h / 4, false);
    lastW_ = w; lastH_ = h;
}

void Renderer::ExtractFrustum(const Matrix& m, Vector4 p[6]) {
    Vector4 r0{ m.m0, m.m4, m.m8, m.m12 }, r1{ m.m1, m.m5, m.m9, m.m13 };
    Vector4 r2{ m.m2, m.m6, m.m10, m.m14 }, r3{ m.m3, m.m7, m.m11, m.m15 };
    auto add = [](Vector4 a, Vector4 b, float s) { return Vector4{ a.x + b.x * s, a.y + b.y * s, a.z + b.z * s, a.w + b.w * s }; };
    p[0] = add(r3, r0, 1); p[1] = add(r3, r0, -1); p[2] = add(r3, r1, 1);
    p[3] = add(r3, r1, -1); p[4] = add(r3, r2, 1); p[5] = add(r3, r2, -1);
    for (int i = 0; i < 6; i++) {
        float l = sqrtf(p[i].x * p[i].x + p[i].y * p[i].y + p[i].z * p[i].z);
        p[i] = Vector4{ p[i].x / l, p[i].y / l, p[i].z / l, p[i].w / l };
    }
}

bool Renderer::SphereVisible(Vector3 c, float r) const {
    for (int i = 0; i < 6; i++)
        if (frustum_[i].x * c.x + frustum_[i].y * c.y + frustum_[i].z * c.z + frustum_[i].w < -r) return false;
    return true;
}

void Renderer::BeginFrame(const Camera3D& c, float t) {
    cam = c; time = t;
    lights_.clear(); items_.clear(); transparent_.clear(); instanced_.clear();
    drawCalls = 0;
    EnsureTargets();
    float aspect = (float)hdr_.texture.width / (float)hdr_.texture.height;
    Matrix view = MatrixLookAt(cam.position, cam.target, cam.up);
    Matrix proj = MatrixPerspective(cam.fovy * DEG2RAD, aspect, 0.05, 900.0);
    viewProj = MatrixMultiply(view, proj);
    ExtractFrustum(viewProj, frustum_);
}

static float MaxScale(const Matrix& m) {
    float sx = sqrtf(m.m0 * m.m0 + m.m1 * m.m1 + m.m2 * m.m2);
    float sy = sqrtf(m.m4 * m.m4 + m.m5 * m.m5 + m.m6 * m.m6);
    float sz = sqrtf(m.m8 * m.m8 + m.m9 * m.m9 + m.m10 * m.m10);
    return fmaxf(sx, fmaxf(sy, sz));
}

void Renderer::DrawPart(const MeshAsset* mesh, int mat, const Matrix& xf, Color tint, bool castShadow) {
    if (!mesh) return;
    Vector3 c = Vector3Transform(mesh->center, xf);
    float r = mesh->radius * MaxScale(xf);
    const SurfaceMat& m = Mat(mat);
    Item it{ mesh, mat, xf, tint, castShadow && m.castShadow, Vector3DistanceSqr(c, cam.position) };
    // Only what is in view (and within the draw distance) is drawn. Off-screen objects are
    // kept only as nearby shadow casters, and only when shadows are on.
    if (it.d2 > (s.drawDistance + r) * (s.drawDistance + r)) return;
    bool vis = SphereVisible(c, r);
    if (!vis && (!it.shadow || !s.shadowsEnabled || it.d2 > 60.0f * 60.0f)) return;
    if (!vis) it.tint.a = 0;   // marker: shadow-only
    if (m.transparent) { if (vis) transparent_.push_back(it); }
    else items_.push_back(it);
}

void Renderer::Draw(const Model3D* model, const Matrix& xf, Color tint, bool castShadow) {
    if (!model) return;
    Vector3 c = Vector3Transform(model->center, xf);
    float r = model->radius * MaxScale(xf);
    bool shadow = castShadow && model->castShadow;
    if (!shadow && !SphereVisible(c, r)) return;
    // cull far shadow-only objects
    if (!SphereVisible(c, r) && Vector3Distance(c, cam.position) > 90.0f + r) return;
    for (const auto& p : model->parts) DrawPart(p.mesh, p.mat, xf, tint, shadow);
}

void Renderer::DrawInstanced(const MeshAsset* mesh, int mat, const std::vector<Matrix>& xfs, float maxDist, float minDist) {
    if (!mesh || xfs.empty()) return;
    Inst inst{ mesh, mat, {} };
    inst.xfs.reserve(xfs.size());
    maxDist = fminf(maxDist, s.drawDistance);
    float md2 = maxDist * maxDist, mn2 = minDist * minDist;
    for (const Matrix& m : xfs) {
        Vector3 c{ m.m12, m.m13, m.m14 };
        float d2 = Vector3DistanceSqr(c, cam.position);
        if (d2 > md2 || d2 < mn2) continue;
        if (!SphereVisible(Vector3Add(c, mesh->center), mesh->radius * MaxScale(m))) continue;
        inst.xfs.push_back(m);
    }
    if (!inst.xfs.empty()) instanced_.push_back(std::move(inst));
}

void Renderer::SelectLights() {
    active_.clear();
    spotShadowIndex_ = -1;
    std::vector<std::pair<float, int>> scored;
    for (int i = 0; i < (int)lights_.size(); i++) {
        const Light& l = lights_[i];
        float d = Vector3Distance(l.pos, cam.position);
        bool near = d < l.range;
        if (!near && !SphereVisible(l.pos, l.range) && l.volumetric <= 0.0f) continue;
        float score = l.intensity * l.range / (d * d + 1.0f) + l.priority * 1000.0f + (l.shadow ? 500.0f : 0.0f);
        scored.push_back({ score, i });
    }
    std::sort(scored.begin(), scored.end(), [](auto& a, auto& b) { return a.first > b.first; });
    for (auto& sc : scored) {
        if ((int)active_.size() >= WTGK_MAX_LIGHTS) break;
        Light l = lights_[sc.second];
        if (l.shadow && l.spot && spotShadowIndex_ < 0) spotShadowIndex_ = (int)active_.size();
        else l.shadow = false;
        active_.push_back(l);
    }
}

void Renderer::ShadowPass() {
    moonShadowOn_ = false;
    spotShadowOn_ = false;
    rlDisableColorBlend();
    // --- Moon ---------------------------------------------------------------
    Vector3 md = Vector3Normalize(s.moonDir);
    if (s.moonShadows && s.shadowsEnabled && md.y > 0.05f) {
        const float extent = 110.0f;
        Vector3 center = Vector3Add(cam.position, Vector3Scale(Vector3Normalize(Vector3Subtract(cam.target, cam.position)), 25.0f));
        center.y = cam.position.y - 1.0f;
        Vector3 up = fabsf(md.y) > 0.98f ? Vector3{ 0, 0, 1 } : Vector3{ 0, 1, 0 };
        Matrix lv = MatrixLookAt(Vector3Add(center, Vector3Scale(md, 250.0f)), center, up);
        Vector3 cls = Vector3Transform(center, lv);
        float texel = extent / 2048.0f;
        cls.x = floorf(cls.x / texel) * texel; cls.y = floorf(cls.y / texel) * texel;
        center = Vector3Transform(cls, MatrixInvert(lv));
        Camera3D lc{};
        lc.position = Vector3Add(center, Vector3Scale(md, 250.0f));
        lc.target = center; lc.up = up; lc.fovy = extent; lc.projection = CAMERA_ORTHOGRAPHIC;
        BeginTextureMode(moonShadow_);
        ClearBackground(WHITE);
        rlSetClipPlanes(1.0, 600.0);
        BeginMode3D(lc);
        moonVP_ = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
        float r2 = (extent * 0.8f) * (extent * 0.8f);
        for (const Item& it : items_) {
            if (!it.shadow) continue;
            Vector3 c = Vector3Transform(it.mesh->center, it.xf);
            Vector3 dc = Vector3Subtract(c, center); dc.y = 0;
            if (Vector3LengthSqr(dc) > r2) continue;
            DrawAsset(CoarsestLod(it.mesh), depthMat_, it.xf);   // 5 cm shadow texels: fine detail can't show
        }
        EndMode3D();
        rlSetClipPlanes(0.05, 900.0);
        EndTextureMode();
        moonShadowOn_ = true;
    }
    // --- Spot ---------------------------------------------------------------
    if (spotShadowIndex_ >= 0 && s.shadowsEnabled) {
        const Light& l = active_[spotShadowIndex_];
        Camera3D lc{};
        lc.position = l.pos;
        lc.target = Vector3Add(l.pos, l.dir);
        lc.up = fabsf(l.dir.y) > 0.98f ? Vector3{ 0, 0, 1 } : Vector3{ 0, 1, 0 };
        lc.fovy = fminf(l.outerDeg * 2.0f + 6.0f, 150.0f);
        lc.projection = CAMERA_PERSPECTIVE;
        BeginTextureMode(spotShadow_);
        ClearBackground(WHITE);
        rlSetClipPlanes(0.1, (double)l.range);
        BeginMode3D(lc);
        spotVP_ = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
        for (const Item& it : items_) {
            if (!it.shadow) continue;
            Vector3 c = Vector3Transform(it.mesh->center, it.xf);
            if (Vector3Distance(c, l.pos) > l.range + it.mesh->radius * MaxScale(it.xf)) continue;
            DrawAsset(it.mesh->lod ? it.mesh->lod : it.mesh, depthMat_, it.xf);
        }
        EndMode3D();
        rlSetClipPlanes(0.05, 900.0);
        EndTextureMode();
        spotShadowOn_ = true;
    }
    rlEnableColorBlend();
}

void Renderer::ApplyCommonUniforms(Shader sh) {
    U3(sh, "uCamPos", cam.position);
    U1(sh, "uTime", time);
    U3(sh, "uFogCol", Vector3Scale(s.fogColor, 1.0f + s.lightning * 1.5f));
    U4(sh, "uFog", Vector4{ s.fogDensity, s.fogHeightFalloff, s.fogBase, s.fogMax });
    U1(sh, "uScatter", s.scatter);
    int n = (int)active_.size();
    UI(sh, "uNumLights", n);
    Vector4 pos[WTGK_MAX_LIGHTS], col[WTGK_MAX_LIGHTS], dir[WTGK_MAX_LIGHTS], ext[WTGK_MAX_LIGHTS];
    for (int i = 0; i < n; i++) {
        const Light& l = active_[i];
        pos[i] = { l.pos.x, l.pos.y, l.pos.z, l.range };
        col[i] = { l.color.x * l.intensity, l.color.y * l.intensity, l.color.z * l.intensity, l.spot ? 1.0f : 0.0f };
        Vector3 d = Vector3Normalize(l.dir);
        dir[i] = { d.x, d.y, d.z, cosf(l.outerDeg * DEG2RAD) };
        ext[i] = { cosf(l.innerDeg * DEG2RAD), (i == spotShadowIndex_ && spotShadowOn_) ? 1.0f : 0.0f, l.volumetric, 0 };
    }
    if (n > 0) {
        SetShaderValueV(sh, L(sh, "uLPos[0]"), pos, SHADER_UNIFORM_VEC4, n);
        SetShaderValueV(sh, L(sh, "uLCol[0]"), col, SHADER_UNIFORM_VEC4, n);
        SetShaderValueV(sh, L(sh, "uLDir[0]"), dir, SHADER_UNIFORM_VEC4, n);
        SetShaderValueV(sh, L(sh, "uLExt[0]"), ext, SHADER_UNIFORM_VEC4, n);
    }
    UM(sh, "uSpotVP", spotVP_);
    U1(sh, "uSpotShadowOn", spotShadowOn_ ? 1.0f : 0.0f);
}

void Renderer::BindMaterial(Shader sh, const SurfaceMat& m, Color tint) {
    for (int i = 0; i < MAX_MATERIAL_MAPS; i++) { g_maps[i].texture = Texture2D{}; g_maps[i].color = WHITE; g_maps[i].value = 0; }
    Color c{ (unsigned char)(m.tint.r * tint.r / 255), (unsigned char)(m.tint.g * tint.g / 255),
             (unsigned char)(m.tint.b * tint.b / 255), (unsigned char)(m.tint.a * tint.a / 255) };
    g_maps[MATERIAL_MAP_ALBEDO].color = c;
    if (m.mode == MODE_UV || m.mode == MODE_EMISSIVE || m.mode == MODE_GLASS) {
        g_maps[MATERIAL_MAP_ALBEDO].texture = m.custom.id ? m.custom : GetWhiteTexture();
        g_maps[MATERIAL_MAP_NORMAL].texture = GetFlatNormalTexture();
    } else {
        const TexSet& a = GetTexSet(m.tex);
        g_maps[MATERIAL_MAP_ALBEDO].texture = a.albedo;
        g_maps[MATERIAL_MAP_NORMAL].texture = a.normal;
        if (m.mode == MODE_TERRAIN) {
            const TexSet& b = GetTexSet(m.texB >= 0 ? m.texB : m.tex);
            const TexSet& cc = GetTexSet(m.texC >= 0 ? m.texC : m.tex);
            g_maps[MATERIAL_MAP_ROUGHNESS].texture = b.albedo;
            g_maps[MATERIAL_MAP_OCCLUSION].texture = b.normal;
            g_maps[MATERIAL_MAP_EMISSION].texture = cc.albedo;
            g_maps[MATERIAL_MAP_HEIGHT].texture = cc.normal;
        }
    }
    g_maps[MATERIAL_MAP_METALNESS].texture = spotShadow_.depth;
    g_maps[MATERIAL_MAP_BRDF].texture = moonShadow_.depth;
    U4(sh, "uMat", Vector4{ m.scale, m.rough, m.metal, (float)m.mode });
    U4(sh, "uMat2", Vector4{ m.normalStr, m.wet, m.emissive, m.wrap });
    U3(sh, "uEmissive", m.emissiveCol);
}

void Renderer::DrawItem(const Item& it, Shader sh) {
    const SurfaceMat& m = Mat(it.mat);
    BindMaterial(sh, m, it.tint);
    Material mat{};
    mat.shader = sh;
    mat.maps = g_maps;
    if (m.doubleSided) rlDisableBackfaceCulling();
    DrawAsset(PickLod(it.mesh, sqrtf(it.d2)), mat, it.xf);
    if (m.doubleSided) rlEnableBackfaceCulling();
    drawCalls++;
}

void Renderer::Render(const std::function<void()>& customOpaque, const std::function<void()>& customTransparent) {
    SelectLights();
    ShadowPass();

    BeginTextureMode(hdr_);
    ClearBackground(BLACK);
    BeginMode3D(cam);

    for (Shader sh : { lit_, litInst_, sky_, particleShader }) ApplyCommonUniforms(sh);
    // scattering is added afterwards, once per pixel (ScatterPass)
    for (Shader sh : { lit_, litInst_, sky_ }) U1(sh, "uScatter", 0.0f);
    sceneVP_ = MatrixMultiply(rlGetMatrixModelview(), rlGetMatrixProjection());
    for (Shader sh : { lit_, litInst_ }) {
        U3(sh, "uMoonDir", Vector3Normalize(s.moonDir));
        U3(sh, "uMoonCol", Vector3Scale(s.moonColor, s.moonBright));
        U3(sh, "uSkyAmb", Vector3Scale(s.skyAmbient, s.brightness));
        U3(sh, "uGroundAmb", Vector3Scale(s.groundAmbient, s.brightness));
        UM(sh, "uMoonVP", moonVP_);
        U1(sh, "uMoonShadowOn", moonShadowOn_ ? 1.0f : 0.0f);
        U1(sh, "uLightning", s.lightning);
        U1(sh, "uWetWorld", s.wetWorld);
        U4(sh, "uWind", s.wind);
    }

    if (s.sky) {
        U3(sky_, "uMoonDir", Vector3Normalize(s.moonDir));
        U3(sky_, "uZenith", s.skyZenith);
        U3(sky_, "uHorizon", s.skyHorizon);
        U3(sky_, "uGlowCol", s.glowColor);
        U3(sky_, "uGlowDir", s.glowDir);
        U1(sky_, "uClouds", s.clouds);
        U1(sky_, "uLightning", s.lightning);
        U1(sky_, "uStars", s.stars);
        U1(sky_, "uMoonBright", s.moonBright);
        skyMat_.maps[MATERIAL_MAP_METALNESS].texture = spotShadow_.depth;
        rlDisableDepthMask();
        rlDisableBackfaceCulling();
        DrawMesh(skyBox_, skyMat_, MatrixMultiply(MatrixScale(800, 800, 800), MatrixTranslate(cam.position.x, cam.position.y, cam.position.z)));
        rlEnableBackfaceCulling();
        rlEnableDepthMask();
    }

    std::sort(items_.begin(), items_.end(), [](const Item& a, const Item& b) { return a.d2 < b.d2; });
    for (const Item& it : items_) {
        if (it.tint.a == 0) continue;  // shadow-only
        DrawItem(it, lit_);
    }
    for (const Inst& in : instanced_) {
        const SurfaceMat& m = Mat(in.mat);
        BindMaterial(litInst_, m, WHITE);
        Material mat{}; mat.shader = litInst_; mat.maps = g_maps;
        if (m.doubleSided) rlDisableBackfaceCulling();
        DrawMeshInstanced(in.mesh->mesh, mat, in.xfs.data(), (int)in.xfs.size());
        for (const Mesh& c : in.mesh->more) DrawMeshInstanced(c, mat, in.xfs.data(), (int)in.xfs.size());
        if (m.doubleSided) rlEnableBackfaceCulling();
        drawCalls++;
    }
    if (customOpaque) customOpaque();
    EndMode3D();
    EndTextureMode();

    // volumetric light over the opaque scene, once per pixel; transparent surfaces then blend
    // over it and add their own share (inline, as before), exactly like the single-pass order
    ScatterPass();
    U1(lit_, "uScatter", s.volumetrics ? s.scatter : 0.0f);
    BeginTextureMode(hdr_);
    BeginMode3D(cam);

    std::sort(transparent_.begin(), transparent_.end(), [](const Item& a, const Item& b) { return a.d2 > b.d2; });
    rlDisableDepthMask();
    BeginBlendMode(BLEND_ALPHA);
    for (const Item& it : transparent_) DrawItem(it, lit_);
    if (customTransparent) customTransparent();
    EndBlendMode();
    rlEnableDepthMask();

    EndMode3D();
    EndTextureMode();

    // --- Bloom ---------------------------------------------------------------
    auto pass = [](RenderTexture2D& dst, Texture2D src, Shader sh) {
        BeginTextureMode(dst);
        ClearBackground(BLACK);
        BeginShaderMode(sh);
        DrawTexturePro(src, Rectangle{ 0, 0, (float)src.width, (float)-src.height },
                       Rectangle{ 0, 0, (float)dst.texture.width, (float)dst.texture.height }, Vector2{ 0, 0 }, 0, WHITE);
        EndShaderMode();
        EndTextureMode();
    };
    U1(bright_, "uThreshold", s.bloomThreshold);
    pass(bloomA_, hdr_.texture, bright_);
    U2(blur_, "uDir", Vector2{ 1, 0 }); pass(bloomB_, bloomA_.texture, blur_);
    U2(blur_, "uDir", Vector2{ 0, 1 }); pass(bloomA_, bloomB_.texture, blur_);
    U2(blur_, "uDir", Vector2{ 1, 0 }); pass(bloomD_, bloomA_.texture, blur_);
    U2(blur_, "uDir", Vector2{ 0, 1 }); pass(bloomC_, bloomD_.texture, blur_);
    U2(blur_, "uDir", Vector2{ 2, 0 }); pass(bloomD_, bloomC_.texture, blur_);
    U2(blur_, "uDir", Vector2{ 0, 2 }); pass(bloomC_, bloomD_.texture, blur_);
}

void Renderer::ScatterPass() {
    if (s.fogDensity * s.scatter <= 0.0f || !s.volumetrics) return;
    int w = hdr_.texture.width, h = hdr_.texture.height;
    // 1. evaluate the in-scattered light for every pixel from the depth buffer
    BeginTextureMode(scatterRT_);
    ClearBackground(BLACK);
    ApplyCommonUniforms(scatter_);
    UM(scatter_, "uInvVP", MatrixInvert(sceneVP_));
    U2(scatter_, "uSize", Vector2{ (float)w, (float)h });
    U1(scatter_, "uHasSky", s.sky ? 1.0f : 0.0f);
    BeginShaderMode(scatter_);
    SetShaderValueTexture(scatter_, L(scatter_, "uDepth"), hdr_.depth);
    SetShaderValueTexture(scatter_, L(scatter_, "texture1"), spotShadow_.depth);
    DrawRectangle(0, 0, w, h, WHITE);
    EndShaderMode();
    EndTextureMode();
    // 2. add it to the scene (before bloom, as before)
    BeginTextureMode(hdr_);
    BeginBlendMode(BLEND_ADDITIVE);
    BeginShaderMode(add_);
    SetShaderValueTexture(add_, L(add_, "uSrc"), scatterRT_.texture);
    DrawRectangle(0, 0, w, h, WHITE);
    EndShaderMode();
    EndBlendMode();
    EndTextureMode();
}

void Renderer::Present() {
    Shader sh = composite_;
    BeginShaderMode(sh);
    U2(sh, "uRes", Vector2{ (float)GetScreenWidth(), (float)GetScreenHeight() });
    U1(sh, "uTime", time);
    U1(sh, "uExposure", s.exposure * s.brightness);
    U1(sh, "uBloom", s.bloom);
    U1(sh, "uGrain", s.grain);
    U1(sh, "uVignette", s.vignette);
    U1(sh, "uCA", s.ca);
    U1(sh, "uAscii", s.ascii);
    U1(sh, "uAsciiFull", s.asciiFull);
    U1(sh, "uCell", fmaxf(7.0f, roundf(GetScreenHeight() / 90.0f)));
    U1(sh, "uGlyphs", (float)glyphCount);
    U1(sh, "uBlink", s.blink);
    U1(sh, "uFade", s.fade);
    U1(sh, "uWhite", s.white);
    U1(sh, "uDesat", s.desat);
    U1(sh, "uRedPulse", s.redPulse);
    U1(sh, "uBlur", s.blur);
    U1(sh, "uGamma", s.gamma);
    U3(sh, "uGradeShadow", s.gradeShadow);
    U3(sh, "uGradeHigh", s.gradeHigh);
    SetShaderValueTexture(sh, L(sh, "texture1"), bloomA_.texture);
    SetShaderValueTexture(sh, L(sh, "texture2"), bloomC_.texture);
    SetShaderValueTexture(sh, L(sh, "texture3"), glyphAtlas.id ? glyphAtlas : GetWhiteTexture());
    DrawTexturePro(hdr_.texture, Rectangle{ 0, 0, (float)hdr_.texture.width, (float)-hdr_.texture.height },
                   Rectangle{ 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() }, Vector2{ 0, 0 }, 0, WHITE);
    EndShaderMode();
}
