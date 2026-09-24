// Forward HDR renderer: shadows (moon + one spot), sky, lit geometry,
// transparent pass, particles and the post-processing chain.
#pragma once
#include "common.h"
#include "materials.h"
#include "mesh_builder.h"
#include <functional>

struct Light {
    Vector3 pos{ 0, 0, 0 };
    Vector3 color{ 1, 1, 1 };
    float intensity = 1.0f;
    float range = 10.0f;
    bool spot = false;
    Vector3 dir{ 0, -1, 0 };
    float innerDeg = 20.0f, outerDeg = 32.0f;
    bool shadow = false;
    float volumetric = 0.0f;   // strength of the visible beam / halo in fog
    float priority = 0.0f;     // added to the selection score
};

struct RenderSettings {
    // Atmosphere
    Vector3 moonDir = Vector3Normalize({ -0.35f, 0.55f, 0.45f });
    Vector3 moonColor = { 0.050f, 0.060f, 0.085f };
    Vector3 skyAmbient = { 0.010f, 0.012f, 0.018f };
    Vector3 groundAmbient = { 0.006f, 0.005f, 0.005f };
    Vector3 fogColor = { 0.020f, 0.023f, 0.030f };
    float fogDensity = 0.018f, fogHeightFalloff = 0.09f, fogBase = 0.0f, fogMax = 0.97f;
    Vector3 skyZenith = { 0.0015f, 0.002f, 0.004f };
    Vector3 skyHorizon = { 0.018f, 0.020f, 0.026f };
    Vector3 glowColor = { 0.05f, 0.028f, 0.012f };
    Vector3 glowDir = { 0.3f, 0.0f, -1.0f };
    float clouds = 0.55f, stars = 1.0f, moonBright = 1.0f, lightning = 0.0f;
    Vector4 wind = { 0.8f, 0.6f, 0.12f, 1.0f };
    float wetWorld = 0.0f;
    float scatter = 1.0f;
    bool sky = true;
    bool moonShadows = true;
    // Post
    float exposure = 1.6f, bloom = 0.5f, bloomThreshold = 1.2f, grain = 0.05f, vignette = 0.55f, ca = 0.35f;
    float ascii = 0.0f, asciiFull = 0.0f, blink = 0.0f, fade = 0.0f, white = 0.0f, desat = 0.2f;
    float redPulse = 0.0f, blur = 0.0f, gamma = 1.0f, brightness = 1.0f;
    Vector3 gradeShadow = { 0.2f, 0.45f, 0.6f }, gradeHigh = { 0.9f, 0.7f, 0.5f };
    float renderScale = 1.0f;
};

class Renderer {
public:
    void Init();
    void Shutdown();
    void BeginFrame(const Camera3D& cam, float time);
    void AddLight(const Light& l) { lights_.push_back(l); }
    void Draw(const Model3D* model, const Matrix& xf, Color tint = WHITE, bool castShadow = true);
    void DrawPart(const MeshAsset* mesh, int mat, const Matrix& xf, Color tint = WHITE, bool castShadow = true);
    void DrawInstanced(const MeshAsset* mesh, int mat, const std::vector<Matrix>& xfs, float maxDist = 1e9f);
    // Renders everything submitted this frame into the HDR buffer.
    void Render(const std::function<void()>& customOpaque = nullptr, const std::function<void()>& customTransparent = nullptr);
    // Composites to the backbuffer (call between BeginDrawing/EndDrawing).
    void Present();

    RenderSettings s;
    Camera3D cam{};
    float time = 0.0f;
    Shader particleShader{};
    Texture2D radialTex{};
    Texture2D glyphAtlas{};
    int glyphCount = 10;
    int RenderWidth() const { return hdr_.texture.width; }
    int RenderHeight() const { return hdr_.texture.height; }
    bool SphereVisible(Vector3 c, float r) const;
    int drawCalls = 0;
    // Apply the common (fog/lights) uniforms to a custom shader that includes kShaderCommon.
    void ApplyCommonUniforms(Shader sh);
    Matrix viewProj{};

private:
    struct Item { const MeshAsset* mesh; int mat; Matrix xf; Color tint; bool shadow; float d2; };
    struct Inst { const MeshAsset* mesh; int mat; std::vector<Matrix> xfs; };
    void EnsureTargets();
    void ShadowPass();
    void SelectLights();
    void BindMaterial(Shader sh, const SurfaceMat& m, Color tint);
    void DrawItem(const Item& it, Shader sh);
    void ExtractFrustum(const Matrix& vp, Vector4 planes[6]);

    std::vector<Light> lights_;
    std::vector<Light> active_;
    std::vector<Item> items_;
    std::vector<Item> transparent_;
    std::vector<Inst> instanced_;
    Vector4 frustum_[6];

    Shader lit_{}, litInst_{}, depth_{}, sky_{}, bright_{}, blur_{}, composite_{};
    Mesh skyBox_{};
    Material skyMat_{}, depthMat_{};
    RenderTexture2D hdr_{}, bloomA_{}, bloomB_{}, bloomC_{}, bloomD_{};
    RenderTexture2D moonShadow_{}, spotShadow_{};
    Matrix moonVP_{}, spotVP_{};
    bool spotShadowOn_ = false, moonShadowOn_ = false;
    int spotShadowIndex_ = -1;
    int lastW_ = 0, lastH_ = 0;
};

Renderer& Rdr();
RenderTexture2D LoadRenderTextureHDR(int w, int h, bool depth);
RenderTexture2D LoadShadowTarget(int size);
