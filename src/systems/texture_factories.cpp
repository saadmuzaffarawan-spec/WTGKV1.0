#include "texture_factories.h"

RenderTexture2D GenerateGlyphTexture(const char* glyph, Color bg, Color fg, int texSize, int fontSize, int jitter) {
    RenderTexture2D rt = LoadRenderTexture(texSize, texSize);
    BeginTextureMode(rt);
        ClearBackground(bg);
        int step = fontSize - fontSize / 3;
        if (step < 4) step = 4;
        for (int y = -fontSize; y < texSize + fontSize; y += step) {
            for (int x = -fontSize; x < texSize + fontSize; x += step) {
                int jx = GetRandomValue(-jitter, jitter);
                int jy = GetRandomValue(-jitter, jitter);
                Color shade = fg;
                int variance = GetRandomValue(-18, 18);
                shade.r = (unsigned char)Clamp((float)fg.r + variance, 0.0f, 255.0f);
                shade.g = (unsigned char)Clamp((float)fg.g + variance, 0.0f, 255.0f);
                shade.b = (unsigned char)Clamp((float)fg.b + variance, 0.0f, 255.0f);
                DrawText(glyph, x + jx, y + jy, fontSize, shade);
            }
        }
    EndTextureMode();
    return rt;
}

RenderTexture2D GenerateCrustTexture(int texSize) {
    RenderTexture2D rt = LoadRenderTexture(texSize, texSize);
    BeginTextureMode(rt);
        ClearBackground(Color{ 196, 148, 84, 255 });
        for (int i = 0; i < 900; i++) {
            int x = GetRandomValue(0, texSize);
            int y = GetRandomValue(0, texSize);
            int r = GetRandomValue(2, 10);
            int dark = GetRandomValue(0, 70);
            Color c = Color{ (unsigned char)Clamp(196 - dark, 90, 255),
                               (unsigned char)Clamp(148 - dark, 60, 255),
                               (unsigned char)Clamp(84  - dark, 20, 255), 255 };
            DrawCircle(x, y, (float)r, c);
        }
        for (int i = 0; i < 6; i++) {
            int x = GetRandomValue(20, texSize - 20);
            DrawRectangle(x, 10, 6, texSize - 20, Color{ 120, 78, 40, 180 });
        }
    EndTextureMode();
    return rt;
}

Model MakeTexturedCylinder(float radius, float height, int slices, Texture2D tex) {
    Mesh mesh = GenMeshCylinder(radius, height, slices);
    Model model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex;
    return model;
}

Model MakeTexturedSphere(float radius, int rings, int slices, Texture2D tex) {
    Mesh mesh = GenMeshSphere(radius, rings, slices);
    Model model = LoadModelFromMesh(mesh);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = tex;
    return model;
}
