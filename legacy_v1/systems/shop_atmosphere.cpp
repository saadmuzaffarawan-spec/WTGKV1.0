#include "shop_atmosphere.h"
#include "rlgl.h"
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>

// =============================================================================
// PHASE 3: ADVANCED SHOP ATMOSPHERE, HIGH-RES TEXTURING & DYNAMIC LIGHTING
// =============================================================================

// GPU Textures
static Texture2D g_texWoodPanels   = { 0 };
static Texture2D g_texGrunge       = { 0 };
static Texture2D g_texCeilingTile  = { 0 };
static Texture2D g_texPosterMissing= { 0 };
static Texture2D g_texPosterTinSign= { 0 };
static Texture2D g_texShopFloor    = { 0 };
static Texture2D g_texWashroomTiles= { 0 };
static Texture2D g_texMirrorGrime  = { 0 };
static RenderTexture2D g_washroomMirrorRT = { 0 };
static bool g_shopAtmosphereLoaded = false;

// Dust Particles
struct ShopDustMote {
    Vector3 pos;
    Vector3 vel;
    float alpha;
    float size;
    float phase;
};
static const int SHOP_DUST_COUNT = 96;
static ShopDustMote g_dustMotes[SHOP_DUST_COUNT];

// Fluttering Moth Entity
struct ShopMoth {
    Vector3 pos;
    Vector3 targetPos;
    float yaw;
    float wingTimer;
    float retargetTimer;
};
static ShopMoth g_moth;

// Water Puddle Locations (Under roof ceiling leak stains)
struct ShopWaterPuddle {
    Vector3 pos;
    float radiusX;
    float radiusZ;
    float rippleTimer;
};
static const int SHOP_PUDDLE_COUNT = 3;
static ShopWaterPuddle g_puddles[SHOP_PUDDLE_COUNT] = {
    { { 93.2f, 10.022f, 135.8f }, 0.95f, 0.75f, 0.0f },
    { { 101.4f, 10.022f, 147.2f }, 0.85f, 1.10f, 1.4f },
    { { 88.6f, 10.022f, 149.0f }, 0.65f, 0.55f, 2.7f }
};

// =============================================================================
// HIGH-FIDELITY 512x512 PROCEDURAL TEXTURE GENERATION
// =============================================================================

static Image GenerateProceduralWoodPanels(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    Color* pixels = (Color*)img.data;

    const int plankWidth = 32; // 16 tongue-and-groove planks across 512px

    for (int y = 0; y < height; y++) {
        float ny = (float)y / (float)height;
        for (int x = 0; x < width; x++) {
            int plankIdx = x / plankWidth;
            int plankX = x % plankWidth;

            // Individual plank tone variance (Appalachian aged walnut & chestnut pine)
            float plankHash = sinf((float)plankIdx * 37.719f + 1.234f) * 43758.5453f;
            float plankTone = (plankHash - floorf(plankHash)) * 0.26f - 0.13f;

            // Multi-octave organic vertical wood grain
            float grain1 = sinf((float)y * 0.09f + sinf((float)x * 0.45f) * 2.2f) * 0.12f;
            float grain2 = sinf((float)y * 0.35f + cosf((float)x * 1.2f) * 1.4f) * 0.06f;
            float fineFiber = ((float)rand() / (float)RAND_MAX - 0.5f) * 0.08f;

            // Simulated wood knot on planks 4, 9, 13
            float knotEffect = 0.0f;
            if (plankIdx == 4 || plankIdx == 9 || plankIdx == 13) {
                float knotY = (plankIdx == 4) ? 0.32f : ((plankIdx == 9) ? 0.74f : 0.52f);
                float kdx = (float)(plankX - plankWidth / 2) / (float)(plankWidth / 2);
                float kdy = (ny - knotY) * 6.0f;
                float kDist = sqrtf(kdx * kdx + kdy * kdy);
                if (kDist < 1.0f) {
                    knotEffect = (1.0f - kDist) * sinf(kDist * 16.0f) * 0.28f - (1.0f - kDist) * 0.35f;
                }
            }

            // Routed tongue-and-groove shadow groove & bevel highlight
            float seamShade = 1.0f;
            if (plankX == 0 || plankX == plankWidth - 1) seamShade = 0.28f;      // Deep routed shadow gap
            else if (plankX == 1) seamShade = 1.22f;                             // Specular bevel highlight (left)
            else if (plankX == plankWidth - 2) seamShade = 0.55f;                // Shadow bevel (right)
            else if (plankX == 2) seamShade = 1.08f;

            // Aged tobacco varnish color grading
            float r = (112.0f + plankTone * 40.0f + (grain1 + grain2) * 50.0f + fineFiber * 30.0f + knotEffect * 70.0f) * seamShade;
            float g = (74.0f  + plankTone * 32.0f + (grain1 + grain2) * 36.0f + fineFiber * 22.0f + knotEffect * 50.0f) * seamShade;
            float b = (46.0f  + plankTone * 22.0f + (grain1 + grain2) * 24.0f + fineFiber * 16.0f + knotEffect * 30.0f) * seamShade;

            // Occasional tarnished brass brad nail heads
            if ((y == 24 || y == height - 24) && (plankX == 6 || plankX == plankWidth - 6)) {
                r = 180.0f; g = 145.0f; b = 70.0f;
            } else if ((y == 25 || y == height - 23) && (plankX == 6 || plankX == plankWidth - 6)) {
                r = 40.0f; g = 30.0f; b = 15.0f;
            }

            pixels[y * width + x] = (Color){
                (unsigned char)Clamp(r, 10.0f, 235.0f),
                (unsigned char)Clamp(g, 6.0f, 195.0f),
                (unsigned char)Clamp(b, 4.0f, 140.0f),
                255
            };
        }
    }
    return img;
}

static Image GenerateProceduralGrunge(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    Color* pixels = (Color*)img.data;

    for (int y = 0; y < height; y++) {
        float normY = (float)y / (float)height; // 0.0 at top (ceiling), 1.0 at bottom (floor)
        for (int x = 0; x < width; x++) {
            // Multi-frequency vertical water drip trails from roofline leaks
            float drip1 = sinf((float)x * 0.08f + sinf((float)y * 0.02f) * 2.0f);
            float drip2 = cosf((float)x * 0.22f + (float)y * 0.03f);
            float topLeak = (1.0f - normY * 2.2f);
            if (topLeak < 0.0f) topLeak = 0.0f;
            float dripPattern = topLeak * fmaxf(0.0f, 0.45f + 0.35f * drip1 + 0.20f * drip2);

            // Rising damp & dark mold creeping up from baseboards
            float moldNoise1 = sinf((float)x * 0.16f + 1.2f) * cosf((float)x * 0.06f);
            float moldNoise2 = sinf((float)x * 0.42f + (float)y * 0.05f) * 0.4f;
            float botMold = (normY - 0.72f) / 0.28f;
            if (botMold < 0.0f) botMold = 0.0f;
            float moldPattern = botMold * fmaxf(0.0f, 0.50f + 0.35f * moldNoise1 + moldNoise2);

            // Fine grime particles & water ring edges
            float speckle = (((float)rand() / (float)RAND_MAX) > 0.94f) ? 0.35f : 0.0f;

            // Tint: Amber oxidized water trails (top) + Black/Olive toxic mold (bottom)
            float r = dripPattern * 75.0f + moldPattern * 22.0f + speckle * 35.0f;
            float g = dripPattern * 48.0f + moldPattern * 28.0f + speckle * 35.0f;
            float b = dripPattern * 18.0f + moldPattern * 14.0f + speckle * 25.0f;
            float a = dripPattern * 215.0f + moldPattern * 230.0f + speckle * 180.0f;

            pixels[y * width + x] = (Color){
                (unsigned char)Clamp(r, 0.0f, 255.0f),
                (unsigned char)Clamp(g, 0.0f, 255.0f),
                (unsigned char)Clamp(b, 0.0f, 255.0f),
                (unsigned char)Clamp(a, 0.0f, 245.0f)
            };
        }
    }
    return img;
}

static Image GenerateProceduralCeilingTile(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    Color* pixels = (Color*)img.data;

    float stainCenterX = width * 0.42f;
    float stainCenterY = height * 0.56f;
    float maxStainR    = width * 0.40f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Textured mineral fiber acoustic tile pattern
            float noise = ((float)rand() / (float)RAND_MAX - 0.5f) * 0.16f;
            float baseGrey = 175.0f + noise * 55.0f;

            // Water stain oxidized circular rings
            float dx = (float)x - stainCenterX;
            float dy = (float)y - stainCenterY;
            float dist = sqrtf(dx * dx + dy * dy);

            float r = baseGrey;
            float g = baseGrey - 12.0f;
            float b = baseGrey - 25.0f;

            if (dist < maxStainR) {
                float ring = sinf(dist * 0.40f);
                float stainIntensity = (1.0f - dist / maxStainR);
                r = r * (1.0f - stainIntensity * 0.42f) + (155.0f + ring * 28.0f) * stainIntensity;
                g = g * (1.0f - stainIntensity * 0.54f) + (112.0f + ring * 22.0f) * stainIntensity;
                b = b * (1.0f - stainIntensity * 0.72f) + (58.0f  + ring * 16.0f) * stainIntensity;
            }

            // Outer edge metal T-bar bevel track
            if (x < 3 || x >= width - 3 || y < 3 || y >= height - 3) {
                r *= 0.42f; g *= 0.42f; b *= 0.44f;
            }

            pixels[y * width + x] = (Color){
                (unsigned char)Clamp(r, 15.0f, 255.0f),
                (unsigned char)Clamp(g, 12.0f, 240.0f),
                (unsigned char)Clamp(b, 8.0f, 210.0f),
                255
            };
        }
    }
    return img;
}

static Image GeneratePosterMissing(int width, int height) {
    Image img = GenImageColor(width, height, (Color){ 232, 224, 208, 255 }); // Aged sepia paper
    Color* pixels = (Color*)img.data;

    // Bold crimson header band: "MISSING"
    int headerH = height / 5;
    for (int y = 4; y < headerH; y++) {
        for (int x = 4; x < width - 4; x++) {
            pixels[y * width + x] = (Color){ 175, 26, 20, 255 };
        }
    }

    // Photo frame area
    int photoTop = headerH + 8;
    int photoBottom = height * 3 / 5;
    int photoLeft = width / 5;
    int photoRight = width * 4 / 5;

    for (int y = photoTop; y < photoBottom; y++) {
        for (int x = photoLeft; x < photoRight; x++) {
            float dither = ((float)rand() / (float)RAND_MAX);
            float faceShade = sinf((float)(x - photoLeft) / (float)(photoRight - photoLeft) * 3.1415f) *
                              cosf((float)(y - photoTop) / (float)(photoBottom - photoTop) * 3.1415f);
            unsigned char c = (faceShade > dither * 0.65f) ? 148 : 42;
            pixels[y * width + x] = (Color){ c, (unsigned char)(c * 0.95f), (unsigned char)(c * 0.88f), 255 };
        }
    }

    // Body text typewriter lines
    for (int line = 0; line < 5; line++) {
        int ly = photoBottom + 12 + line * 16;
        if (ly >= height - 12) break;
        for (int x = 12; x < width - 12; x++) {
            if ((x / 4) % 3 != 0) {
                pixels[ly * width + x] = (Color){ 32, 30, 26, 235 };
                pixels[(ly + 1) * width + x] = (Color){ 32, 30, 26, 235 };
            }
        }
    }

    // Masking tape corner tabs
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 14; x++) {
            pixels[y * width + x] = (Color){ 215, 200, 145, 190 };
            pixels[y * width + (width - 1 - x)] = (Color){ 215, 200, 145, 190 };
        }
    }
    return img;
}

static Image GeneratePosterTinSign(int width, int height) {
    Image img = GenImageColor(width, height, (Color){ 28, 42, 62, 255 }); // Dark enamel tin
    Color* pixels = (Color*)img.data;

    float cx = width * 0.5f;
    float cy = height * 0.5f;
    float rx = width * 0.42f;
    float ry = height * 0.38f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float dx = fabsf((float)x - cx) / rx;
            float dy = fabsf((float)y - cy) / ry;
            if (dx + dy <= 1.0f) {
                float rust = ((float)rand() / (float)RAND_MAX) > 0.88f ? 0.42f : 1.0f;
                pixels[y * width + x] = (Color){
                    (unsigned char)(225 * rust),
                    (unsigned char)(175 * rust),
                    (unsigned char)(50 * rust),
                    255
                };
            }
            if (x <= 3 || x >= width - 4 || y <= 3 || y >= height - 4) {
                pixels[y * width + x] = (Color){ 130, 68, 38, 255 };
            }
        }
    }
    return img;
}

static Image GenerateProceduralShopFloor(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    Color* pixels = (Color*)img.data;

    const int tileSize = 64; // 8x8 checkered commercial VCT tiles across 512px

    for (int y = 0; y < height; y++) {
        int tileY = y / tileSize;
        int py = y % tileSize;
        for (int x = 0; x < width; x++) {
            int tileX = x / tileSize;
            int px = x % tileSize;

            bool isDark = ((tileX + tileY) % 2 == 1);

            // Subtle organic noise in vinyl linoleum
            float noise1 = sinf((float)x * 0.15f + cosf((float)y * 0.12f) * 2.0f) * 0.08f;
            float noise2 = cosf((float)x * 0.35f - sinf((float)y * 0.28f) * 1.5f) * 0.05f;
            float fineGrain = (((float)rand() / (float)RAND_MAX) - 0.5f) * 0.08f;

            // Base tile tones (Aged 1970s Appalachian store):
            // Light tile: Worn aged mustard/cream linoleum
            // Dark tile: Mottled charcoal/slate-olive linoleum
            float r, g, b;
            if (!isDark) {
                // Cream / buff
                r = 172.0f + (noise1 + noise2 + fineGrain) * 45.0f;
                g = 158.0f + (noise1 + noise2 + fineGrain) * 42.0f;
                b = 130.0f + (noise1 + noise2 + fineGrain) * 35.0f;
            } else {
                // Slate / charcoal
                r = 44.0f + (noise1 + noise2 + fineGrain) * 26.0f;
                g = 47.0f + (noise1 + noise2 + fineGrain) * 28.0f;
                b = 46.0f + (noise1 + noise2 + fineGrain) * 30.0f;
            }

            // Grout seam line (2px edge border around each tile)
            float seamDarken = 1.0f;
            if (px == 0 || px == tileSize - 1 || py == 0 || py == tileSize - 1) {
                seamDarken = 0.30f; // Deep dark recessed grout
                r *= seamDarken; g *= seamDarken; b *= seamDarken;
            } else if (px == 1 || py == 1) {
                seamDarken = 1.18f; // Light specular bevel rim
                r *= seamDarken; g *= seamDarken; b *= seamDarken;
            }

            // Random heel scuff marks / scratches across floor
            float scuffHash = sinf((float)tileX * 17.13f + (float)tileY * 43.19f);
            if (scuffHash > 0.40f) {
                int scuffY = (int)(fabsf(sinf(scuffHash * 100.0f)) * (tileSize - 12)) + 6;
                if (abs(py - scuffY) <= 1 && px > 10 && px < tileSize - 10) {
                    float scuffFade = 1.0f - (float)abs(py - scuffY) * 0.4f;
                    r = r * (1.0f - 0.48f * scuffFade) + 18.0f * (0.48f * scuffFade);
                    g = g * (1.0f - 0.48f * scuffFade) + 18.0f * (0.48f * scuffFade);
                    b = b * (1.0f - 0.48f * scuffFade) + 20.0f * (0.48f * scuffFade);
                }
            }

            // Tile corner yellowed wax accumulation
            int minEdgeX = px < tileSize / 2 ? px : tileSize - 1 - px;
            int minEdgeY = py < tileSize / 2 ? py : tileSize - 1 - py;
            float cornerDist = sqrtf((float)(minEdgeX * minEdgeX + minEdgeY * minEdgeY));
            if (cornerDist < 9.0f) {
                float wax = (1.0f - cornerDist / 9.0f) * 0.28f;
                r += wax * 38.0f; g += wax * 28.0f; b -= wax * 18.0f;
            }

            pixels[y * width + x] = (Color){
                (unsigned char)Clamp(r, 10.0f, 248.0f),
                (unsigned char)Clamp(g, 10.0f, 238.0f),
                (unsigned char)Clamp(b, 8.0f, 218.0f),
                255
            };
        }
    }
    return img;
}

static Image GenerateProceduralWashroomTiles(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    Color* pixels = (Color*)img.data;

    const int tileSize = 48; // Dirty ceramic bathroom tiles

    for (int y = 0; y < height; y++) {
        int py = y % tileSize;
        for (int x = 0; x < width; x++) {
            int px = x % tileSize;

            // Aged dingy institutional restroom ceramic tile
            float tileNoise = (((float)rand() / (float)RAND_MAX) - 0.5f) * 0.12f;
            float r = 182.0f + tileNoise * 35.0f;
            float g = 188.0f + tileNoise * 32.0f;
            float b = 176.0f + tileNoise * 30.0f;

            // Recessed dark moldy mildew grout
            if (px <= 1 || px >= tileSize - 2 || py <= 1 || py >= tileSize - 2) {
                r = 25.0f + (((float)rand() / (float)RAND_MAX)) * 14.0f;
                g = 32.0f + (((float)rand() / (float)RAND_MAX)) * 16.0f;
                b = 22.0f + (((float)rand() / (float)RAND_MAX)) * 10.0f;
            }

            // Dripping grime & water rust trails from ceiling leaks
            float dripNoise = sinf((float)x * 0.12f + sinf((float)y * 0.05f) * 3.0f);
            if (dripNoise > 0.62f) {
                float dripStr = (dripNoise - 0.62f) / 0.38f;
                r = r * (1.0f - dripStr * 0.45f) + 115.0f * (dripStr * 0.45f);
                g = g * (1.0f - dripStr * 0.60f) + 68.0f  * (dripStr * 0.60f);
                b = b * (1.0f - dripStr * 0.80f) + 24.0f  * (dripStr * 0.80f);
            }

            // Splattered dried blood streaks (procedural horror decals)
            float bloodX1 = width * 0.38f;
            float bloodY1 = height * 0.62f;
            float bdx1 = (float)x - bloodX1;
            float bdy1 = (float)y - bloodY1;
            float bdist1 = sqrtf(bdx1 * bdx1 + bdy1 * bdy1);
            if (bdist1 < 36.0f) {
                float bRad = 36.0f + sinf(atan2f(bdy1, bdx1) * 7.0f) * 12.0f;
                if (bdist1 < bRad) {
                    float edge = bdist1 / bRad;
                    // Dark oxidized arterial blood with dark coagulated clotted edge
                    r = 115.0f * (1.0f - edge * 0.45f);
                    g = 12.0f * (1.0f - edge * 0.30f);
                    b = 16.0f * (1.0f - edge * 0.30f);
                }
            }

            // Secondary blood drip trail
            float bloodX2 = width * 0.72f;
            float bloodY2 = height * 0.40f;
            if (fabsf((float)x - bloodX2) < 6.0f && y > bloodY2 && y < bloodY2 + 120.0f) {
                float trailFade = 1.0f - (float)(y - bloodY2) / 120.0f;
                r = r * (1.0f - trailFade * 0.75f) + 95.0f * (trailFade * 0.75f);
                g = g * (1.0f - trailFade * 0.90f) + 8.0f * (trailFade * 0.90f);
                b = b * (1.0f - trailFade * 0.90f) + 12.0f * (trailFade * 0.90f);
            }

            pixels[y * width + x] = (Color){
                (unsigned char)Clamp(r, 8.0f, 250.0f),
                (unsigned char)Clamp(g, 6.0f, 245.0f),
                (unsigned char)Clamp(b, 5.0f, 235.0f),
                255
            };
        }
    }
    return img;
}

static Image GenerateProceduralMirrorOverlay(int width, int height) {
    Image img = GenImageColor(width, height, BLANK);
    Color* pixels = (Color*)img.data;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            float r = 0, g = 0, b = 0, a = 0;

            // 1. Edge silvering rot (blackened oxidized silver along perimeter)
            int borderDist = std::min({ x, width - 1 - x, y, height - 1 - y });
            if (borderDist < 24) {
                float rotNoise = sinf((float)x * 0.18f) * cosf((float)y * 0.22f);
                float rotThresh = (float)(24 - borderDist) / 24.0f + rotNoise * 0.25f;
                if (rotThresh > 0.38f) {
                    float rotA = Clamp((rotThresh - 0.38f) / 0.62f, 0.0f, 1.0f);
                    r = 34.0f + (((float)rand() / (float)RAND_MAX) * 16.0f);
                    g = 28.0f + (((float)rand() / (float)RAND_MAX) * 12.0f);
                    b = 20.0f;
                    a = rotA * 235.0f;
                }
            }

            // 2. Hairline cracked mirror spiderweb fractures from top right
            float cx = width * 0.75f;
            float cy = height * 0.22f;
            float cdx = (float)x - cx;
            float cdy = (float)y - cy;
            float cdist = sqrtf(cdx * cdx + cdy * cdy);
            if (cdist < 140.0f && cdist > 2.0f) {
                float angle = atan2f(cdy, cdx);
                for (int ray = 0; ray < 6; ray++) {
                    float rayAngle = (float)ray * 1.047f + 0.35f;
                    float diffAngle = fabsf(angle - rayAngle);
                    if (diffAngle < 0.020f) {
                        r = 240.0f; g = 245.0f; b = 255.0f;
                        a = std::max((float)a, (1.0f - cdist / 140.0f) * 215.0f);
                    }
                }
            }

            // 3. Bloody handprint on mirror glass (center-left)
            float px = (float)x - 210.0f;
            float py = (float)y - 270.0f;
            float palmDist = sqrtf(px * px * 0.8f + py * py * 1.2f);
            if (palmDist < 35.0f) {
                float pA = (1.0f - palmDist / 35.0f) * 195.0f;
                r = 130.0f; g = 14.0f; b = 18.0f;
                a = std::max((float)a, pA);
            }
            float fingersX[5] = { -32.0f, -16.0f, 0.0f, 16.0f, 32.0f };
            float fingerLen[5] = { 45.0f, 65.0f, 75.0f, 60.0f, 40.0f };
            for (int f = 0; f < 5; f++) {
                float fx = px - fingersX[f];
                float fy = py + 20.0f;
                if (fabsf(fx) < 6.5f && fy > -fingerLen[f] && fy < 35.0f) {
                    float fA = (1.0f - fabsf(fx) / 6.5f) * 185.0f;
                    r = 135.0f; g = 12.0f; b = 18.0f;
                    a = std::max((float)a, fA);
                }
            }

            pixels[y * width + x] = (Color){
                (unsigned char)Clamp(r, 0.0f, 255.0f),
                (unsigned char)Clamp(g, 0.0f, 255.0f),
                (unsigned char)Clamp(b, 0.0f, 255.0f),
                (unsigned char)Clamp(a, 0.0f, 255.0f)
            };
        }
    }
    return img;
}

// =============================================================================
// PUBLIC API IMPLEMENTATION
// =============================================================================

void InitShopAtmosphere() {
    if (g_shopAtmosphereLoaded) return;

    // 1. Generate & Upload High-Resolution Procedural Textures
    Image imgPanels = GenerateProceduralWoodPanels(512, 512);
    g_texWoodPanels = LoadTextureFromImage(imgPanels);
    SetTextureFilter(g_texWoodPanels, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_texWoodPanels, TEXTURE_WRAP_REPEAT);
    UnloadImage(imgPanels);

    Image imgGrunge = GenerateProceduralGrunge(512, 512);
    g_texGrunge = LoadTextureFromImage(imgGrunge);
    SetTextureFilter(g_texGrunge, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_texGrunge, TEXTURE_WRAP_REPEAT);
    UnloadImage(imgGrunge);

    Image imgCeiling = GenerateProceduralCeilingTile(256, 256);
    g_texCeilingTile = LoadTextureFromImage(imgCeiling);
    SetTextureFilter(g_texCeilingTile, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_texCeilingTile, TEXTURE_WRAP_REPEAT);
    UnloadImage(imgCeiling);

    Image imgMissing = GeneratePosterMissing(128, 192);
    g_texPosterMissing = LoadTextureFromImage(imgMissing);
    SetTextureFilter(g_texPosterMissing, TEXTURE_FILTER_BILINEAR);
    UnloadImage(imgMissing);

    Image imgTin = GeneratePosterTinSign(192, 128);
    g_texPosterTinSign = LoadTextureFromImage(imgTin);
    SetTextureFilter(g_texPosterTinSign, TEXTURE_FILTER_BILINEAR);
    UnloadImage(imgTin);

    // Procedural Floor Textures & Mirror Overlay
    Image imgShopFloor = GenerateProceduralShopFloor(512, 512);
    g_texShopFloor = LoadTextureFromImage(imgShopFloor);
    SetTextureFilter(g_texShopFloor, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_texShopFloor, TEXTURE_WRAP_REPEAT);
    UnloadImage(imgShopFloor);

    Image imgWashroom = GenerateProceduralWashroomTiles(512, 512);
    g_texWashroomTiles = LoadTextureFromImage(imgWashroom);
    SetTextureFilter(g_texWashroomTiles, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_texWashroomTiles, TEXTURE_WRAP_REPEAT);
    UnloadImage(imgWashroom);

    Image imgMirrorOverlay = GenerateProceduralMirrorOverlay(512, 512);
    g_texMirrorGrime = LoadTextureFromImage(imgMirrorOverlay);
    SetTextureFilter(g_texMirrorGrime, TEXTURE_FILTER_BILINEAR);
    UnloadImage(imgMirrorOverlay);

    // Real Planar Reflection Mirror Render Target (512x512)
    g_washroomMirrorRT = LoadRenderTexture(512, 512);
    SetTextureFilter(g_washroomMirrorRT.texture, TEXTURE_FILTER_BILINEAR);

    // 2. Initialize Dust Particles inside store volume
    for (int i = 0; i < SHOP_DUST_COUNT; i++) {
        g_dustMotes[i].pos = (Vector3){
            88.0f + ((float)rand() / (float)RAND_MAX) * 18.0f,
            10.2f + ((float)rand() / (float)RAND_MAX) * 4.4f,
            128.0f + ((float)rand() / (float)RAND_MAX) * 24.0f
        };
        g_dustMotes[i].vel = (Vector3){
            (((float)rand() / (float)RAND_MAX) - 0.5f) * 0.04f,
            (((float)rand() / (float)RAND_MAX) - 0.5f) * 0.02f,
            (((float)rand() / (float)RAND_MAX) - 0.5f) * 0.04f
        };
        g_dustMotes[i].alpha = 0.35f + ((float)rand() / (float)RAND_MAX) * 0.60f;
        g_dustMotes[i].size  = 0.012f + ((float)rand() / (float)RAND_MAX) * 0.016f;
        g_dustMotes[i].phase = ((float)rand() / (float)RAND_MAX) * 6.28f;
    }

    // 3. Initialize Fluttering Moth
    g_moth.pos = (Vector3){ 95.0f, 14.2f, 143.5f };
    g_moth.targetPos = g_moth.pos;
    g_moth.yaw = 0.0f;
    g_moth.wingTimer = 0.0f;
    g_moth.retargetTimer = 0.0f;

    g_shopAtmosphereLoaded = true;
}

void UpdateShopAtmosphere(float dt, float timeVal, Vector3 playerPos, bool isMoving, bool lightsOn, float bulbSwayX, float bulbSwayZ) {
    if (!g_shopAtmosphereLoaded) return;

    // 1. Update Dust Motes with convective drift & player turbulence
    for (int i = 0; i < SHOP_DUST_COUNT; i++) {
        ShopDustMote& m = g_dustMotes[i];
        m.phase += dt * 1.8f;

        m.pos.x += m.vel.x * dt + sinf(m.phase + (float)i) * 0.003f * dt;
        m.pos.y += m.vel.y * dt + cosf(m.phase * 0.7f) * 0.002f * dt;
        m.pos.z += m.vel.z * dt + sinf(m.phase * 1.3f) * 0.003f * dt;

        if (isMoving) {
            float distSq = Vector3DistanceSqr(m.pos, playerPos);
            if (distSq < 2.5f * 2.5f && distSq > 0.01f) {
                Vector3 away = Vector3Normalize(Vector3Subtract(m.pos, playerPos));
                m.pos = Vector3Add(m.pos, Vector3Scale(away, dt * 0.85f));
            }
        }

        if (m.pos.x < 87.5f)  m.pos.x = 106.5f;
        if (m.pos.x > 106.5f) m.pos.x = 87.5f;
        if (m.pos.y < 10.2f)  m.pos.y = 14.8f;
        if (m.pos.y > 14.8f)  m.pos.y = 10.2f;
        if (m.pos.z < 127.5f) m.pos.z = 152.5f;
        if (m.pos.z > 152.5f) m.pos.z = 127.5f;
    }

    // 2. Update Moth fluttering around swaying central tungsten bulb
    if (lightsOn) {
        g_moth.retargetTimer -= dt;
        if (g_moth.retargetTimer <= 0.0f) {
            g_moth.retargetTimer = 0.35f + ((float)rand() / (float)RAND_MAX) * 0.70f;
            float angle = ((float)rand() / (float)RAND_MAX) * 6.28f;
            float radius = 0.22f + ((float)rand() / (float)RAND_MAX) * 0.65f;
            g_moth.targetPos = (Vector3){
                95.0f + bulbSwayX + cosf(angle) * radius,
                14.25f + sinf(angle * 2.0f) * 0.22f,
                143.5f + bulbSwayZ + sinf(angle) * radius
            };
        }
        g_moth.pos = Vector3Lerp(g_moth.pos, g_moth.targetPos, dt * 4.8f);
        g_moth.wingTimer += dt * 44.0f;
    }

    // 3. Update Puddle ripple timers
    for (int p = 0; p < SHOP_PUDDLE_COUNT; p++) {
        g_puddles[p].rippleTimer += dt * 1.5f;
    }
}

void DrawShopAtmosphereWalls(ShopLightFn lightFn, bool lightsOn, float timeVal) {
    if (!g_shopAtmosphereLoaded) return;

    rlSetTexture(g_texWoodPanels.id);
    rlBegin(RL_QUADS);

    // =========================================================================
    // 1. WEST INTERIOR WALL (X = 86.25, facing +X) - PER-VERTEX VERTICAL GRADIENT
    // =========================================================================
    Vector3 normW = { 1.0f, 0.0f, 0.0f };
    rlNormal3f(normW.x, normW.y, normW.z);

    for (float wz = 127.25f; wz <= 152.75f; wz += 2.55f) {
        float hz = 2.58f * 0.5f;
        float u0 = (wz - 127.25f) / 2.55f;
        float u1 = u0 + 1.0f;

        Vector3 pBL = { 85.50f, 10.0f, wz - hz };
        Vector3 pTL = { 85.50f, 15.2f, wz - hz };
        Vector3 pTR = { 85.50f, 15.2f, wz + hz };
        Vector3 pBR = { 85.50f, 10.0f, wz + hz };

        Color cBL = lightFn(pBL, (Color){ 250, 245, 235, 255 }, normW, 0);
        Color cTL = lightFn(pTL, (Color){ 250, 245, 235, 255 }, normW, 0);
        Color cTR = lightFn(pTR, (Color){ 250, 245, 235, 255 }, normW, 0);
        Color cBR = lightFn(pBR, (Color){ 250, 245, 235, 255 }, normW, 0);

        rlColor4ub(cBL.r, cBL.g, cBL.b, cBL.a); rlTexCoord2f(u0, 2.2f); rlVertex3f(pBL.x, pBL.y, pBL.z);
        rlColor4ub(cTL.r, cTL.g, cTL.b, cTL.a); rlTexCoord2f(u0, 0.0f); rlVertex3f(pTL.x, pTL.y, pTL.z);
        rlColor4ub(cTR.r, cTR.g, cTR.b, cTR.a); rlTexCoord2f(u1, 0.0f); rlVertex3f(pTR.x, pTR.y, pTR.z);
        rlColor4ub(cBR.r, cBR.g, cBR.b, cBR.a); rlTexCoord2f(u1, 2.2f); rlVertex3f(pBR.x, pBR.y, pBR.z);
    }

    // =========================================================================
    // 2. SOUTH INTERIOR WALL (Z = 126.25, facing +Z) - PER-VERTEX VERTICAL GRADIENT
    // =========================================================================
    Vector3 normS = { 0.0f, 0.0f, 1.0f };
    rlNormal3f(normS.x, normS.y, normS.z);

    for (float wx = 87.25f; wx <= 106.75f; wx += 2.45f) {
        float hx = 2.48f * 0.5f;
        float u0 = (wx - 87.25f) / 2.45f;
        float u1 = u0 + 1.0f;

        Vector3 pBL = { wx + hx, 10.0f, 126.25f };
        Vector3 pTL = { wx + hx, 15.2f, 126.25f };
        Vector3 pTR = { wx - hx, 15.2f, 126.25f };
        Vector3 pBR = { wx - hx, 10.0f, 126.25f };

        Color cBL = lightFn(pBL, (Color){ 250, 245, 235, 255 }, normS, 0);
        Color cTL = lightFn(pTL, (Color){ 250, 245, 235, 255 }, normS, 0);
        Color cTR = lightFn(pTR, (Color){ 250, 245, 235, 255 }, normS, 0);
        Color cBR = lightFn(pBR, (Color){ 250, 245, 235, 255 }, normS, 0);

        rlColor4ub(cBL.r, cBL.g, cBL.b, cBL.a); rlTexCoord2f(u0, 2.2f); rlVertex3f(pBL.x, pBL.y, pBL.z);
        rlColor4ub(cTL.r, cTL.g, cTL.b, cTL.a); rlTexCoord2f(u0, 0.0f); rlVertex3f(pTL.x, pTL.y, pTL.z);
        rlColor4ub(cTR.r, cTR.g, cTR.b, cTR.a); rlTexCoord2f(u1, 0.0f); rlVertex3f(pTR.x, pTR.y, pTR.z);
        rlColor4ub(cBR.r, cBR.g, cBR.b, cBR.a); rlTexCoord2f(u1, 2.2f); rlVertex3f(pBR.x, pBR.y, pBR.z);
    }

    // =========================================================================
    // 3. NORTH INTERIOR WALL (Z = 153.75, facing -Z) - PER-VERTEX VERTICAL GRADIENT
    // =========================================================================
    Vector3 normN = { 0.0f, 0.0f, -1.0f };
    rlNormal3f(normN.x, normN.y, normN.z);

    for (float wx = 87.25f; wx <= 106.75f; wx += 2.45f) {
        float hx = 2.48f * 0.5f;
        float u0 = (wx - 87.25f) / 2.45f;
        float u1 = u0 + 1.0f;

        Vector3 pBL = { wx - hx, 10.0f, 153.75f };
        Vector3 pTL = { wx - hx, 15.2f, 153.75f };
        Vector3 pTR = { wx + hx, 15.2f, 153.75f };
        Vector3 pBR = { wx + hx, 10.0f, 153.75f };

        Color cBL = lightFn(pBL, (Color){ 250, 245, 235, 255 }, normN, 0);
        Color cTL = lightFn(pTL, (Color){ 250, 245, 235, 255 }, normN, 0);
        Color cTR = lightFn(pTR, (Color){ 250, 245, 235, 255 }, normN, 0);
        Color cBR = lightFn(pBR, (Color){ 250, 245, 235, 255 }, normN, 0);

        rlColor4ub(cBL.r, cBL.g, cBL.b, cBL.a); rlTexCoord2f(u0, 2.2f); rlVertex3f(pBL.x, pBL.y, pBL.z);
        rlColor4ub(cTL.r, cTL.g, cTL.b, cTL.a); rlTexCoord2f(u0, 0.0f); rlVertex3f(pTL.x, pTL.y, pTL.z);
        rlColor4ub(cTR.r, cTR.g, cTR.b, cTR.a); rlTexCoord2f(u1, 0.0f); rlVertex3f(pTR.x, pTR.y, pTR.z);
        rlColor4ub(cBR.r, cBR.g, cBR.b, cBR.a); rlTexCoord2f(u1, 2.2f); rlVertex3f(pBR.x, pBR.y, pBR.z);
    }

    // =========================================================================
    // 4. EAST INTERIOR FACADE WALL (X = 107.85, flanking entrance)
    // =========================================================================
    Vector3 normE = { -1.0f, 0.0f, 0.0f };
    rlNormal3f(normE.x, normE.y, normE.z);

    // South flank
    Color eBLS = lightFn((Vector3){ 107.85f, 10.0f, 138.55f }, (Color){ 250, 245, 235, 255 }, normE, 0);
    Color eTLS = lightFn((Vector3){ 107.85f, 15.2f, 138.55f }, (Color){ 250, 245, 235, 255 }, normE, 0);
    Color eTRS = lightFn((Vector3){ 107.85f, 15.2f, 126.00f }, (Color){ 250, 245, 235, 255 }, normE, 0);
    Color eBRS = lightFn((Vector3){ 107.85f, 10.0f, 126.00f }, (Color){ 250, 245, 235, 255 }, normE, 0);

    rlColor4ub(eBLS.r, eBLS.g, eBLS.b, eBLS.a); rlTexCoord2f(0.0f, 2.2f); rlVertex3f(107.85f, 10.0f, 138.55f);
    rlColor4ub(eTLS.r, eTLS.g, eTLS.b, eTLS.a); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(107.85f, 15.2f, 138.55f);
    rlColor4ub(eTRS.r, eTRS.g, eTRS.b, eTRS.a); rlTexCoord2f(4.0f, 0.0f); rlVertex3f(107.85f, 15.2f, 126.00f);
    rlColor4ub(eBRS.r, eBRS.g, eBRS.b, eBRS.a); rlTexCoord2f(4.0f, 2.2f); rlVertex3f(107.85f, 10.0f, 126.00f);

    // North flank
    Color eBLN = lightFn((Vector3){ 107.85f, 10.0f, 154.00f }, (Color){ 250, 245, 235, 255 }, normE, 0);
    Color eTLN = lightFn((Vector3){ 107.85f, 15.2f, 154.00f }, (Color){ 250, 245, 235, 255 }, normE, 0);
    Color eTRN = lightFn((Vector3){ 107.85f, 15.2f, 141.45f }, (Color){ 250, 245, 235, 255 }, normE, 0);
    Color eBRN = lightFn((Vector3){ 107.85f, 10.0f, 141.45f }, (Color){ 250, 245, 235, 255 }, normE, 0);

    rlColor4ub(eBLN.r, eBLN.g, eBLN.b, eBLN.a); rlTexCoord2f(0.0f, 2.2f); rlVertex3f(107.85f, 10.0f, 154.00f);
    rlColor4ub(eTLN.r, eTLN.g, eTLN.b, eTLN.a); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(107.85f, 15.2f, 154.00f);
    rlColor4ub(eTRN.r, eTRN.g, eTRN.b, eTRN.a); rlTexCoord2f(4.0f, 0.0f); rlVertex3f(107.85f, 15.2f, 141.45f);
    rlColor4ub(eBRN.r, eBRN.g, eBRN.b, eBRN.a); rlTexCoord2f(4.0f, 2.2f); rlVertex3f(107.85f, 10.0f, 141.45f);

    // Header over door
    Color eBLH = lightFn((Vector3){ 107.85f, 13.40f, 141.45f }, (Color){ 250, 245, 235, 255 }, normE, 0);
    Color eTLH = lightFn((Vector3){ 107.85f, 15.20f, 141.45f }, (Color){ 250, 245, 235, 255 }, normE, 0);
    Color eTRH = lightFn((Vector3){ 107.85f, 15.20f, 138.55f }, (Color){ 250, 245, 235, 255 }, normE, 0);
    Color eBRH = lightFn((Vector3){ 107.85f, 13.40f, 138.55f }, (Color){ 250, 245, 235, 255 }, normE, 0);

    rlColor4ub(eBLH.r, eBLH.g, eBLH.b, eBLH.a); rlTexCoord2f(0.0f, 1.0f); rlVertex3f(107.85f, 13.40f, 141.45f);
    rlColor4ub(eTLH.r, eTLH.g, eTLH.b, eTLH.a); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(107.85f, 15.20f, 141.45f);
    rlColor4ub(eTRH.r, eTRH.g, eTRH.b, eTRH.a); rlTexCoord2f(2.0f, 0.0f); rlVertex3f(107.85f, 15.20f, 138.55f);
    rlColor4ub(eBRH.r, eBRH.g, eBRH.b, eBRH.a); rlTexCoord2f(2.0f, 1.0f); rlVertex3f(107.85f, 13.40f, 138.55f);

    rlEnd();

    // =========================================================================
    // 5. SECONDARY GRUNGE & WATER SEEPAGE OVERLAY (PER-VERTEX GRADIENTS)
    // =========================================================================
    rlSetTexture(g_texGrunge.id);
    rlBegin(RL_QUADS);

    // West wall moisture seep
    rlNormal3f(normW.x, normW.y, normW.z);
    for (float wz = 127.25f; wz <= 152.75f; wz += 2.55f) {
        float hz = 2.58f * 0.5f;
        Vector3 pBL = { 85.51f, 10.0f, wz - hz };
        Vector3 pTL = { 85.51f, 15.2f, wz - hz };
        Vector3 pTR = { 85.51f, 15.2f, wz + hz };
        Vector3 pBR = { 85.51f, 10.0f, wz + hz };

        Color cBL = lightFn(pBL, (Color){ 190, 190, 190, 220 }, normW, 0);
        Color cTL = lightFn(pTL, (Color){ 190, 190, 190, 220 }, normW, 0);
        Color cTR = lightFn(pTR, (Color){ 190, 190, 190, 220 }, normW, 0);
        Color cBR = lightFn(pBR, (Color){ 190, 190, 190, 220 }, normW, 0);

        rlColor4ub(cBL.r, cBL.g, cBL.b, (unsigned char)(cBL.a * 0.70f)); rlTexCoord2f(0.0f, 1.0f); rlVertex3f(pBL.x, pBL.y, pBL.z);
        rlColor4ub(cTL.r, cTL.g, cTL.b, (unsigned char)(cTL.a * 0.70f)); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(pTL.x, pTL.y, pTL.z);
        rlColor4ub(cTR.r, cTR.g, cTR.b, (unsigned char)(cTR.a * 0.70f)); rlTexCoord2f(1.0f, 0.0f); rlVertex3f(pTR.x, pTR.y, pTR.z);
        rlColor4ub(cBR.r, cBR.g, cBR.b, (unsigned char)(cBR.a * 0.70f)); rlTexCoord2f(1.0f, 1.0f); rlVertex3f(pBR.x, pBR.y, pBR.z);
    }

    // South wall moisture seep
    rlNormal3f(normS.x, normS.y, normS.z);
    for (float wx = 87.25f; wx <= 106.75f; wx += 2.45f) {
        float hx = 2.48f * 0.5f;
        Vector3 pBL = { wx + hx, 10.0f, 126.26f };
        Vector3 pTL = { wx + hx, 15.2f, 126.26f };
        Vector3 pTR = { wx - hx, 15.2f, 126.26f };
        Vector3 pBR = { wx - hx, 10.0f, 126.26f };

        Color cBL = lightFn(pBL, (Color){ 190, 190, 190, 220 }, normS, 0);
        Color cTL = lightFn(pTL, (Color){ 190, 190, 190, 220 }, normS, 0);
        Color cTR = lightFn(pTR, (Color){ 190, 190, 190, 220 }, normS, 0);
        Color cBR = lightFn(pBR, (Color){ 190, 190, 190, 220 }, normS, 0);

        rlColor4ub(cBL.r, cBL.g, cBL.b, (unsigned char)(cBL.a * 0.70f)); rlTexCoord2f(0.0f, 1.0f); rlVertex3f(pBL.x, pBL.y, pBL.z);
        rlColor4ub(cTL.r, cTL.g, cTL.b, (unsigned char)(cTL.a * 0.70f)); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(pTL.x, pTL.y, pTL.z);
        rlColor4ub(cTR.r, cTR.g, cTR.b, (unsigned char)(cTR.a * 0.70f)); rlTexCoord2f(1.0f, 0.0f); rlVertex3f(pTR.x, pTR.y, pTR.z);
        rlColor4ub(cBR.r, cBR.g, cBR.b, (unsigned char)(cBR.a * 0.70f)); rlTexCoord2f(1.0f, 1.0f); rlVertex3f(pBR.x, pBR.y, pBR.z);
    }

    // North wall moisture seep
    rlNormal3f(normN.x, normN.y, normN.z);
    for (float wx = 87.25f; wx <= 106.75f; wx += 2.45f) {
        float hx = 2.48f * 0.5f;
        Vector3 pBL = { wx - hx, 10.0f, 153.74f };
        Vector3 pTL = { wx - hx, 15.2f, 153.74f };
        Vector3 pTR = { wx + hx, 15.2f, 153.74f };
        Vector3 pBR = { wx + hx, 10.0f, 153.74f };

        Color cBL = lightFn(pBL, (Color){ 190, 190, 190, 220 }, normN, 0);
        Color cTL = lightFn(pTL, (Color){ 190, 190, 190, 220 }, normN, 0);
        Color cTR = lightFn(pTR, (Color){ 190, 190, 190, 220 }, normN, 0);
        Color cBR = lightFn(pBR, (Color){ 190, 190, 190, 220 }, normN, 0);

        rlColor4ub(cBL.r, cBL.g, cBL.b, (unsigned char)(cBL.a * 0.70f)); rlTexCoord2f(0.0f, 1.0f); rlVertex3f(pBL.x, pBL.y, pBL.z);
        rlColor4ub(cTL.r, cTL.g, cTL.b, (unsigned char)(cTL.a * 0.70f)); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(pTL.x, pTL.y, pTL.z);
        rlColor4ub(cTR.r, cTR.g, cTR.b, (unsigned char)(cTR.a * 0.70f)); rlTexCoord2f(1.0f, 0.0f); rlVertex3f(pTR.x, pTR.y, pTR.z);
        rlColor4ub(cBR.r, cBR.g, cBR.b, (unsigned char)(cBR.a * 0.70f)); rlTexCoord2f(1.0f, 1.0f); rlVertex3f(pBR.x, pBR.y, pBR.z);
    }

    rlEnd();
    rlSetTexture(0);
}

void DrawShopAtmosphereCeiling(ShopLightFn lightFn, bool lightsOn) {
    if (!g_shopAtmosphereLoaded) return;

    rlSetTexture(g_texCeilingTile.id);
    rlBegin(RL_QUADS);
    rlNormal3f(0.0f, -1.0f, 0.0f);

    // Weathered acoustic ceiling tiles (3.2m x 3.2m modules) with PER-VERTEX illumination
    for (float cx = 87.5f; cx <= 106.5f; cx += 3.2f) {
        for (float cz = 128.0f; cz <= 152.0f; cz += 3.2f) {
            float h = 3.18f * 0.5f;
            float y = 15.13f;

            Vector3 v0 = { cx - h, y, cz - h };
            Vector3 v1 = { cx + h, y, cz - h };
            Vector3 v2 = { cx + h, y, cz + h };
            Vector3 v3 = { cx - h, y, cz + h };

            Color c0 = lightFn(v0, (Color){ 235, 230, 220, 255 }, (Vector3){ 0.0f, -1.0f, 0.0f }, 0);
            Color c1 = lightFn(v1, (Color){ 235, 230, 220, 255 }, (Vector3){ 0.0f, -1.0f, 0.0f }, 0);
            Color c2 = lightFn(v2, (Color){ 235, 230, 220, 255 }, (Vector3){ 0.0f, -1.0f, 0.0f }, 0);
            Color c3 = lightFn(v3, (Color){ 235, 230, 220, 255 }, (Vector3){ 0.0f, -1.0f, 0.0f }, 0);

            rlColor4ub(c0.r, c0.g, c0.b, c0.a); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(v0.x, v0.y, v0.z);
            rlColor4ub(c1.r, c1.g, c1.b, c1.a); rlTexCoord2f(1.0f, 0.0f); rlVertex3f(v1.x, v1.y, v1.z);
            rlColor4ub(c2.r, c2.g, c2.b, c2.a); rlTexCoord2f(1.0f, 1.0f); rlVertex3f(v2.x, v2.y, v2.z);
            rlColor4ub(c3.r, c3.g, c3.b, c3.a); rlTexCoord2f(0.0f, 1.0f); rlVertex3f(v3.x, v3.y, v3.z);
        }
    }

    rlEnd();
    rlSetTexture(0);
}

void DrawShopAtmosphereDetails(ShopLightFn lightFn, bool lightsOn, float timeVal) {
    if (!g_shopAtmosphereLoaded) return;

    // =========================================================================
    // 1. ENVIRONMENTAL STORYTELLING POSTERS ON SHOP WALLS
    // =========================================================================

    // A. Missing Person Flyer (South Wall behind Cash Counter at X = 105.2, Z = 126.30)
    rlSetTexture(g_texPosterMissing.id);
    rlBegin(RL_QUADS);
    rlNormal3f(0.0f, 0.0f, 1.0f);
    Color postCol = lightFn((Vector3){ 105.2f, 12.2f, 126.35f }, (Color){ 245, 240, 230, 255 }, (Vector3){ 0, 0, 1 }, 0);
    rlColor4ub(postCol.r, postCol.g, postCol.b, 255);
    float pw = 0.38f * 0.5f;
    float ph = 0.54f * 0.5f;
    float px = 105.2f;
    float py = 12.25f;
    float pz = 126.28f;
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(px + pw, py - ph, pz);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(px + pw, py + ph, pz);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f(px - pw, py + ph, pz);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f(px - pw, py - ph, pz);
    rlEnd();

    // B. Vintage Appalachian Coal & Oil Tin Sign (West Wall above beverage coolers at Z = 143.5, Y = 13.6)
    rlSetTexture(g_texPosterTinSign.id);
    rlBegin(RL_QUADS);
    rlNormal3f(1.0f, 0.0f, 0.0f);
    Color tinCol = lightFn((Vector3){ 86.30f, 13.6f, 143.5f }, (Color){ 240, 235, 220, 255 }, (Vector3){ 1, 0, 0 }, 0);
    rlColor4ub(tinCol.r, tinCol.g, tinCol.b, 255);
    float tw = 0.95f * 0.5f;
    float th = 0.65f * 0.5f;
    float tx = 86.28f;
    float ty = 13.60f;
    float tz = 143.5f;
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(tx, ty - th, tz - tw);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(tx, ty + th, tz - tw);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f(tx, ty + th, tz + tw);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f(tx, ty - th, tz + tw);
    rlEnd();
    rlSetTexture(0);

    // =========================================================================
    // 2. ARCHITECTURAL BASEBOARD & WAINSCOT CHAIR-RAIL MOLDING
    // =========================================================================
    // Baseboard trim at floor level (X=86.26, Z=126.26, Z=153.74)
    Color baseboardCol = lightFn((Vector3){ 95.0f, 10.15f, 140.0f }, (Color){ 45, 30, 20, 255 }, (Vector3){ 0, 1, 0 }, 0);
    DrawCube((Vector3){ 86.30f, 10.12f, 140.0f }, 0.06f, 0.22f, 25.5f, baseboardCol);
    DrawCube((Vector3){ 97.0f, 10.12f, 126.30f }, 19.5f, 0.22f, 0.06f, baseboardCol);
    DrawCube((Vector3){ 97.0f, 10.12f, 153.70f }, 19.5f, 0.22f, 0.06f, baseboardCol);

    // Chair-rail wainscoting molding running around the store at Y = 11.95m
    Color chairRailCol = lightFn((Vector3){ 95.0f, 11.95f, 140.0f }, (Color){ 75, 48, 30, 255 }, (Vector3){ 0, 1, 0 }, 0);
    DrawCube((Vector3){ 86.28f, 11.95f, 140.0f }, 0.04f, 0.08f, 25.5f, chairRailCol);
    DrawCube((Vector3){ 97.0f, 11.95f, 126.28f }, 19.5f, 0.08f, 0.04f, chairRailCol);
    DrawCube((Vector3){ 97.0f, 11.95f, 153.72f }, 19.5f, 0.08f, 0.04f, chairRailCol);

    // =========================================================================
    // 3. WATER PUDDLES WITH SPECULAR REFLECTIONS ON CHECKERED FLOOR
    // =========================================================================
    if (lightsOn) {
        for (int p = 0; p < SHOP_PUDDLE_COUNT; p++) {
            const ShopWaterPuddle& pud = g_puddles[p];
            Color pudLit = lightFn(pud.pos, (Color){ 20, 26, 32, 255 }, (Vector3){ 0, 1, 0 }, 0);

            // Dark wet floor base
            DrawCircle3D(pud.pos, pud.radiusX, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ (unsigned char)(pudLit.r * 0.4f), (unsigned char)(pudLit.g * 0.4f), (unsigned char)(pudLit.b * 0.4f), 195 });

            // Animated concentric ripple rings
            float rip = fmodf(pud.rippleTimer, 1.0f);
            float ripR = pud.radiusX * rip;
            unsigned char ripA = (unsigned char)(Clamp(1.0f - rip, 0.0f, 1.0f) * 110.0f);
            DrawCircle3D(pud.pos, ripR, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 180, 215, 235, ripA });

            // Glancing specular gleam from overhead tungsten pendant
            Vector3 specPt = { pud.pos.x - 0.12f, pud.pos.y + 0.001f, pud.pos.z - 0.08f };
            DrawCircle3D(specPt, 0.20f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 255, 240, 195, 95 });
            DrawCircle3D(specPt, 0.07f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 255, 255, 245, 175 });
        }
    }
}

void DrawShopAtmosphereParticles(Camera3D camera, bool lightsOn, float timeVal) {
    if (!g_shopAtmosphereLoaded) return;

    // =========================================================================
    // 1. AIRBORNE VOLUMETRIC DUST MOTES FLOATING IN LIGHT CONES
    // =========================================================================
    if (lightsOn) {
        Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        Vector3 right = Vector3Normalize(Vector3CrossProduct(fwd, camera.up));
        Vector3 up = Vector3CrossProduct(right, fwd);

        Vector3 bulbPos = { 95.0f, 14.5f, 143.5f };

        rlBegin(RL_QUADS);
        for (int i = 0; i < SHOP_DUST_COUNT; i++) {
            const ShopDustMote& m = g_dustMotes[i];

            float distToBulbSq = Vector3DistanceSqr(m.pos, bulbPos);
            float illum = Clamp(1.0f - distToBulbSq / (6.0f * 6.0f), 0.18f, 1.0f);
            unsigned char alpha = (unsigned char)(m.alpha * illum * 200.0f);
            if (alpha < 8) continue;

            Color dustCol = { (unsigned char)(255 * illum), (unsigned char)(235 * illum), (unsigned char)(190 * illum), alpha };
            rlColor4ub(dustCol.r, dustCol.g, dustCol.b, dustCol.a);

            float s = m.size;
            Vector3 v0 = Vector3Add(m.pos, Vector3Add(Vector3Scale(right, -s), Vector3Scale(up, -s)));
            Vector3 v1 = Vector3Add(m.pos, Vector3Add(Vector3Scale(right,  s), Vector3Scale(up, -s)));
            Vector3 v2 = Vector3Add(m.pos, Vector3Add(Vector3Scale(right,  s), Vector3Scale(up,  s)));
            Vector3 v3 = Vector3Add(m.pos, Vector3Add(Vector3Scale(right, -s), Vector3Scale(up,  s)));

            rlVertex3f(v0.x, v0.y, v0.z);
            rlVertex3f(v1.x, v1.y, v1.z);
            rlVertex3f(v2.x, v2.y, v2.z);
            rlVertex3f(v3.x, v3.y, v3.z);
        }
        rlEnd();

        // Downlight radial warm pool from central swaying bulb
        DrawCircle3D((Vector3){ bulbPos.x, 10.024f, bulbPos.z }, 3.8f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 255, 215, 135, 42 });
        DrawCircle3D((Vector3){ bulbPos.x, 10.025f, bulbPos.z }, 1.6f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 255, 235, 175, 75 });
    }

    // =========================================================================
    // 2. FLUTTERING MOTH CIRCLING THE HOT TUNGSTEN BULB
    // =========================================================================
    if (lightsOn) {
        float wingSpan = 0.026f;
        float wingAngle = sinf(g_moth.wingTimer) * 0.018f;

        Vector3 mPos = g_moth.pos;
        Color mothCol = { 205, 185, 155, 240 };
        Color mothBody = { 68, 55, 42, 255 };

        DrawSphere(mPos, 0.005f, mothBody);

        Vector3 leftWingTip  = { mPos.x - wingSpan, mPos.y + wingAngle, mPos.z + wingAngle * 0.5f };
        Vector3 rightWingTip = { mPos.x + wingSpan, mPos.y + wingAngle, mPos.z - wingAngle * 0.5f };
        DrawLine3D(mPos, leftWingTip, mothCol);
        DrawLine3D(mPos, rightWingTip, mothCol);
    }
}

// =============================================================================
// PHASE 3: HIGH-FIDELITY TEXTURED FLOOR SYSTEM
// =============================================================================

void DrawShopAtmosphereFloor(ShopLightFn lightFn, bool lightsOn, float timeVal) {
    if (!g_shopAtmosphereLoaded) return;

    rlSetTexture(g_texShopFloor.id);
    rlBegin(RL_QUADS);
    Vector3 normFloor = { 0.0f, 1.0f, 0.0f };
    rlNormal3f(0.0f, 1.0f, 0.0f);

    // Floor area: X in [86.25, 107.85], Z in [126.25, 153.75]
    // Tiled in 1.8m x 1.8m quads with per-vertex lighting calculation!
    for (float tx = 85.50f; tx <= 107.85f; tx += 1.80f) {
        for (float tz = 126.25f; tz <= 153.75f; tz += 1.80f) {
            float hx = 0.90f;
            float hz = 0.90f;
            float y = 10.015f;

            Vector3 v0 = { tx - hx, y, tz - hz };
            Vector3 v1 = { tx + hx, y, tz - hz };
            Vector3 v2 = { tx + hx, y, tz + hz };
            Vector3 v3 = { tx - hx, y, tz + hz };

            Color c0 = lightFn(v0, (Color){ 245, 240, 235, 255 }, normFloor, 0);
            Color c1 = lightFn(v1, (Color){ 245, 240, 235, 255 }, normFloor, 0);
            Color c2 = lightFn(v2, (Color){ 245, 240, 235, 255 }, normFloor, 0);
            Color c3 = lightFn(v3, (Color){ 245, 240, 235, 255 }, normFloor, 0);

            float u0 = (tx - hx - 85.50f) / 1.80f;
            float u1 = (tx + hx - 85.50f) / 1.80f;
            float w0 = (tz - hz - 126.25f) / 1.80f;
            float w1 = (tz + hz - 126.25f) / 1.80f;

            rlColor4ub(c0.r, c0.g, c0.b, c0.a); rlTexCoord2f(u0, w0); rlVertex3f(v0.x, v0.y, v0.z);
            rlColor4ub(c1.r, c1.g, c1.b, c1.a); rlTexCoord2f(u1, w0); rlVertex3f(v1.x, v1.y, v1.z);
            rlColor4ub(c2.r, c2.g, c2.b, c2.a); rlTexCoord2f(u1, w1); rlVertex3f(v2.x, v2.y, v2.z);
            rlColor4ub(c3.r, c3.g, c3.b, c3.a); rlTexCoord2f(u0, w1); rlVertex3f(v3.x, v3.y, v3.z);
        }
    }
    rlEnd();
    rlSetTexture(0);
}

// =============================================================================
// PHASE 3: EXPANDED SHOP INTERIOR PROPS & ENVIRONMENTAL STORYTELLING
// =============================================================================

void DrawShopInteriorProps(ShopLightFn lightFn, bool lightsOn, float timeVal) {
    if (!g_shopAtmosphereLoaded) return;

    // -------------------------------------------------------------------------
    // 1. COMMERCIAL REACH-IN 3-DOOR BEVERAGE COOLER (West Wall at Z = 133.5)
    // -------------------------------------------------------------------------
    {
        Vector3 cPos = { 86.62f, 11.80f, 133.5f };
        Color frameCol = lightFn(cPos, (Color){ 30, 32, 36, 255 }, (Vector3){ 1, 0, 0 }, 0);
        Color kickCol  = lightFn(cPos, (Color){ 20, 22, 25, 255 }, (Vector3){ 1, 0, 0 }, 0);

        // Main housing outer shell
        DrawCube(cPos, 0.68f, 3.60f, 5.80f, frameCol);
        DrawCubeWires(cPos, 0.69f, 3.61f, 5.81f, (Color){ 55, 58, 64, 255 });

        // Base kickplate with intake louvers
        DrawCube({ cPos.x + 0.05f, 10.16f, cPos.z }, 0.72f, 0.32f, 5.76f, kickCol);
        for (int l = 0; l < 8; l++) {
            DrawCube({ cPos.x + 0.36f, 10.10f + l * 0.025f, cPos.z }, 0.02f, 0.012f, 5.2f, (Color){ 10, 12, 14, 255 });
        }

        // Backlit header marquee sign ("ICE COLD DRINKS")
        Color headerCol = lightsOn ? (Color){ 215, 240, 255, 255 } : (Color){ 45, 52, 58, 255 };
        DrawCube({ cPos.x + 0.35f, 13.35f, cPos.z }, 0.04f, 0.40f, 5.60f, headerCol);
        DrawCubeWires({ cPos.x + 0.35f, 13.35f, cPos.z }, 0.05f, 0.41f, 5.61f, (Color){ 80, 85, 95, 255 });

        // Interior refrigerated bay cavity
        Color cavityCol = lightsOn ? (Color){ 220, 235, 245, 255 } : (Color){ 18, 22, 26, 255 };
        DrawCube({ cPos.x + 0.02f, 11.75f, cPos.z }, 0.58f, 2.65f, 5.56f, cavityCol);

        // 3 Glass Doors & Wire Shelves with Rows of Beverage Cans/Bottles
        float doorW = 1.78f;
        for (int d = 0; d < 3; d++) {
            float dz = cPos.z - 1.82f + d * doorW;

            // Chrome vertical tubular door handle
            DrawCylinderEx({ cPos.x + 0.36f, 11.20f, dz + 0.65f }, { cPos.x + 0.36f, 12.25f, dz + 0.65f }, 0.016f, 0.016f, 8, (Color){ 210, 215, 225, 255 });

            // 3 Wire Shelves inside this bay
            for (int s = 0; s < 3; s++) {
                float sy = 10.65f + s * 0.85f;
                DrawCube({ cPos.x + 0.02f, sy, dz }, 0.54f, 0.025f, doorW - 0.08f, (Color){ 165, 175, 185, 255 });

                // Colorful soda cans / bottles on each shelf
                for (int c = 0; c < 6; c++) {
                    float cz = dz - 0.65f + c * 0.26f;
                    Color canColor;
                    if ((c + s) % 5 == 0)      canColor = (Color){ 220, 35, 40, 255 };   // Cola Red
                    else if ((c + s) % 5 == 1) canColor = (Color){ 45, 175, 65, 255 };  // Ginger Ale
                    else if ((c + s) % 5 == 2) canColor = (Color){ 235, 195, 45, 255 }; // Lemon Soda
                    else if ((c + s) % 5 == 3) canColor = (Color){ 185, 80, 195, 255 }; // Grape
                    else                       canColor = (Color){ 200, 205, 215, 255 }; // Silver Beer Can

                    Color litCan = lightFn((Vector3){ cPos.x + 0.08f, sy + 0.12f, cz }, canColor, (Vector3){ 1, 0, 0 }, 0);
                    DrawCylinder({ cPos.x + 0.08f, sy + 0.02f, cz }, 0.045f, 0.045f, 0.18f, 8, litCan);
                    // Aluminum pull tab lid
                    DrawCylinder({ cPos.x + 0.08f, sy + 0.20f, cz }, 0.042f, 0.042f, 0.01f, 8, (Color){ 220, 225, 230, 255 });
                }
            }

            // Glass door pane with subtle frost & reflection sheen
            Color glassTint = lightsOn ? (Color){ 180, 220, 245, 55 } : (Color){ 40, 60, 80, 75 };
            DrawCube({ cPos.x + 0.33f, 11.75f, dz }, 0.02f, 2.65f, doorW - 0.06f, glassTint);
        }
    }

    // -------------------------------------------------------------------------
    // 2. VINTAGE WIRE MAGAZINE & NEWSPAPER CAROUSEL STAND (X = 104.5, Z = 146.0)
    // -------------------------------------------------------------------------
    {
        Vector3 mPos = { 104.5f, 10.0f, 146.0f };
        Color wireCol = lightFn(mPos, (Color){ 45, 48, 52, 255 }, (Vector3){ 0, 1, 0 }, 0);

        // Circular cast iron base & center spindle rod
        DrawCylinder(mPos, 0.34f, 0.34f, 0.04f, 12, wireCol);
        DrawCylinderEx(mPos, { mPos.x, 11.65f, mPos.z }, 0.022f, 0.022f, 8, wireCol);

        // Tier 1 (Newspapers): folded newspaper bundles
        for (int a = 0; a < 4; a++) {
            float ang = (float)a * 1.5707f + 0.2f;
            Vector3 bPos = { mPos.x + cosf(ang) * 0.22f, 10.45f, mPos.z + sinf(ang) * 0.22f };
            DrawCube(bPos, 0.28f, 0.06f, 0.38f, lightFn(bPos, (Color){ 218, 212, 198, 255 }, (Vector3){ 0, 1, 0 }, 0));
            // Dark headline text bar
            DrawCube({ bPos.x, bPos.y + 0.035f, bPos.z }, 0.20f, 0.01f, 0.04f, (Color){ 40, 38, 35, 255 });
        }

        // Tier 2 & 3 (Retro Magazines & Comic Books)
        for (int tier = 0; tier < 2; tier++) {
            float ty = 10.85f + tier * 0.45f;
            DrawCylinder({ mPos.x, ty, mPos.z }, 0.28f, 0.28f, 0.02f, 10, wireCol);
            for (int r = 0; r < 4; r++) {
                float ang = (float)r * 1.5707f + (tier * 0.785f);
                Vector3 magPos = { mPos.x + cosf(ang) * 0.24f, ty + 0.12f, mPos.z + sinf(ang) * 0.24f };
                Color coverCol = (r == 0) ? (Color){ 195, 45, 45, 255 } : ((r == 1) ? (Color){ 45, 120, 185, 255 } : ((r == 2) ? (Color){ 215, 165, 35, 255 } : (Color){ 55, 155, 75, 255 }));
                DrawCube(magPos, 0.18f, 0.26f, 0.03f, lightFn(magPos, coverCol, (Vector3){ 0, 1, 0 }, 0));
            }
        }
    }

    // -------------------------------------------------------------------------
    // 3. COFFEE MAKER STATION ON SIDE COUNTER (X = 102.8, Z = 133.5, Y = 11.55)
    // -------------------------------------------------------------------------
    {
        Vector3 cfePos = { 102.8f, 11.55f, 133.5f };
        Color ssCol = lightFn(cfePos, (Color){ 195, 200, 208, 255 }, (Vector3){ 0, 1, 0 }, 0);
        Color baseCol = lightFn(cfePos, (Color){ 35, 38, 42, 255 }, (Vector3){ 0, 1, 0 }, 0);

        // Commercial twin-plate brewer body
        DrawCube({ cfePos.x, cfePos.y + 0.20f, cfePos.z }, 0.32f, 0.40f, 0.45f, ssCol);
        DrawCube({ cfePos.x, cfePos.y + 0.02f, cfePos.z }, 0.36f, 0.04f, 0.50f, baseCol);
        // Red rocker power switch
        DrawCube({ cfePos.x - 0.15f, cfePos.y + 0.32f, cfePos.z - 0.14f }, 0.03f, 0.04f, 0.03f, lightsOn ? (Color){ 255, 45, 35, 255 } : (Color){ 80, 15, 15, 255 });

        // Glass coffee carafe with black handle and dark liquid
        Vector3 potPos = { cfePos.x - 0.02f, cfePos.y + 0.14f, cfePos.z };
        DrawCylinder(potPos, 0.09f, 0.06f, 0.18f, 10, (Color){ 215, 225, 235, 140 });
        // Dark brewed coffee liquid inside
        DrawCylinder({ potPos.x, potPos.y + 0.02f, potPos.z }, 0.08f, 0.055f, 0.10f, 8, (Color){ 35, 18, 12, 240 });
        // Plastic spout rim & handle
        DrawCylinder({ potPos.x, potPos.y + 0.17f, potPos.z }, 0.092f, 0.092f, 0.02f, 8, (Color){ 25, 25, 28, 255 });
        DrawCylinderEx({ potPos.x - 0.11f, potPos.y + 0.16f, potPos.z }, { potPos.x - 0.11f, potPos.y + 0.04f, potPos.z }, 0.014f, 0.014f, 6, (Color){ 25, 25, 28, 255 });

        // Stack of disposable Styrofoam cups
        Vector3 cupPos = { cfePos.x + 0.08f, cfePos.y + 0.12f, cfePos.z + 0.16f };
        DrawCylinder(cupPos, 0.042f, 0.032f, 0.22f, 8, (Color){ 240, 242, 245, 255 });
    }

    // -------------------------------------------------------------------------
    // 4. WALL-MOUNTED ROTARY PAYPHONE (East Wall at X = 107.75, Z = 145.2, Y = 12.3)
    // -------------------------------------------------------------------------
    {
        Vector3 phPos = { 107.75f, 12.30f, 145.2f };
        Color phBody = lightFn(phPos, (Color){ 28, 30, 34, 255 }, (Vector3){ -1, 0, 0 }, 0);

        // Cast metal telephone housing box
        DrawCube(phPos, 0.10f, 0.52f, 0.28f, phBody);
        DrawCubeWires(phPos, 0.105f, 0.525f, 0.285f, (Color){ 65, 68, 75, 255 });

        // Chrome coin slot & release lever at top
        DrawCube({ phPos.x - 0.055f, phPos.y + 0.21f, phPos.z }, 0.02f, 0.025f, 0.08f, (Color){ 195, 200, 205, 255 });

        // Chrome rotary dial wheel (circular)
        DrawCylinderEx({ phPos.x - 0.055f, phPos.y + 0.04f, phPos.z }, { phPos.x - 0.075f, phPos.y + 0.04f, phPos.z }, 0.065f, 0.065f, 12, (Color){ 215, 220, 225, 255 });
        DrawCylinderEx({ phPos.x - 0.076f, phPos.y + 0.04f, phPos.z }, { phPos.x - 0.080f, phPos.y + 0.04f, phPos.z }, 0.022f, 0.022f, 8, (Color){ 40, 42, 45, 255 });

        // Handset resting on cradle (or askew)
        Vector3 crdPos = { phPos.x - 0.075f, phPos.y - 0.02f, phPos.z - 0.16f };
        DrawCylinderEx({ crdPos.x, crdPos.y - 0.14f, crdPos.z }, { crdPos.x, crdPos.y + 0.14f, crdPos.z }, 0.024f, 0.024f, 6, (Color){ 20, 22, 25, 255 });
        DrawSphere({ crdPos.x, crdPos.y - 0.14f, crdPos.z }, 0.038f, (Color){ 20, 22, 25, 255 }); // Mouthpiece
        DrawSphere({ crdPos.x, crdPos.y + 0.14f, crdPos.z }, 0.038f, (Color){ 20, 22, 25, 255 }); // Earpiece
        // Coiled cord dangling below
        DrawLine3D({ crdPos.x, crdPos.y - 0.16f, crdPos.z }, { phPos.x - 0.05f, phPos.y - 0.22f, phPos.z }, (Color){ 35, 38, 42, 255 });
    }

    // -------------------------------------------------------------------------
    // 5. STACKED WOODEN CRATES & CARDBOARD BOXES (Rear Corner X = 87.2, Z = 151.2)
    // -------------------------------------------------------------------------
    {
        Vector3 crt1 = { 87.25f, 10.35f, 151.2f };
        Color crateCol1 = lightFn(crt1, (Color){ 85, 60, 38, 255 }, (Vector3){ 0, 1, 0 }, 0);
        DrawCube(crt1, 0.90f, 0.70f, 0.90f, crateCol1);
        DrawCubeWires(crt1, 0.91f, 0.71f, 0.91f, (Color){ 45, 32, 20, 255 });

        // Top crate offset slightly
        Vector3 crt2 = { 87.30f, 10.95f, 151.15f };
        Color crateCol2 = lightFn(crt2, (Color){ 95, 68, 44, 255 }, (Vector3){ 0, 1, 0 }, 0);
        DrawCube(crt2, 0.75f, 0.50f, 0.75f, crateCol2);
        DrawCubeWires(crt2, 0.76f, 0.51f, 0.76f, (Color){ 52, 36, 22, 255 });

        // Cardboard boxes beside the crates
        Vector3 bx1 = { 87.25f, 10.25f, 149.8f };
        Color boxCol1 = lightFn(bx1, (Color){ 145, 118, 78, 255 }, (Vector3){ 0, 1, 0 }, 0);
        DrawCube(bx1, 0.65f, 0.50f, 0.55f, boxCol1);
        DrawCubeWires(bx1, 0.66f, 0.51f, 0.56f, (Color){ 95, 75, 48, 255 });
        // Packaging tape line across top
        DrawCube({ bx1.x, bx1.y + 0.255f, bx1.z }, 0.66f, 0.005f, 0.08f, (Color){ 175, 145, 95, 230 });
    }

    // -------------------------------------------------------------------------
    // 6. HANGING VINTAGE AISLE SIGNS (Over Aisle 1 and Aisle 2)
    // -------------------------------------------------------------------------
    {
        // Aisle 1 Sign (Over Z = 139.0, X = 95.0, Y = 14.5)
        Vector3 s1 = { 95.0f, 14.45f, 139.0f };
        Color signCol1 = lightFn(s1, (Color){ 32, 48, 42, 255 }, (Vector3){ 0, 0, 1 }, 0);
        DrawCube(s1, 2.20f, 0.40f, 0.04f, signCol1);
        DrawCubeWires(s1, 2.21f, 0.41f, 0.05f, (Color){ 180, 145, 55, 255 }); // Brass trim
        // Hanging suspension wires up to ceiling
        DrawLine3D({ s1.x - 0.95f, s1.y + 0.20f, s1.z }, { s1.x - 0.95f, 15.18f, s1.z }, (Color){ 120, 125, 130, 255 });
        DrawLine3D({ s1.x + 0.95f, s1.y + 0.20f, s1.z }, { s1.x + 0.95f, 15.18f, s1.z }, (Color){ 120, 125, 130, 255 });

        // Aisle 2 Sign (Over Z = 147.0, X = 95.0, Y = 14.5)
        Vector3 s2 = { 95.0f, 14.45f, 147.0f };
        Color signCol2 = lightFn(s2, (Color){ 32, 48, 42, 255 }, (Vector3){ 0, 0, 1 }, 0);
        DrawCube(s2, 2.20f, 0.40f, 0.04f, signCol2);
        DrawCubeWires(s2, 2.21f, 0.41f, 0.05f, (Color){ 180, 145, 55, 255 });
        DrawLine3D({ s2.x - 0.95f, s2.y + 0.20f, s2.z }, { s2.x - 0.95f, 15.18f, s2.z }, (Color){ 120, 125, 130, 255 });
        DrawLine3D({ s2.x + 0.95f, s2.y + 0.20f, s2.z }, { s2.x + 0.95f, 15.18f, s2.z }, (Color){ 120, 125, 130, 255 });
    }

    // -------------------------------------------------------------------------
    // 7. TIPPED YELLOW "CAUTION WET FLOOR" CONE (X = 93.8, Z = 136.2)
    // -------------------------------------------------------------------------
    {
        Vector3 conePos = { 93.8f, 10.08f, 136.2f };
        Color coneYellow = lightFn(conePos, (Color){ 245, 205, 25, 255 }, (Vector3){ 0, 1, 0 }, 0);
        // Tipped on side: drawn horizontal
        DrawCylinderEx(conePos, { conePos.x + 0.42f, conePos.y + 0.06f, conePos.z + 0.18f }, 0.16f, 0.02f, 6, coneYellow);
        // Black warning band
        DrawCylinderEx({ conePos.x + 0.15f, conePos.y + 0.02f, conePos.z + 0.06f }, { conePos.x + 0.22f, conePos.y + 0.03f, conePos.z + 0.09f }, 0.11f, 0.08f, 6, (Color){ 25, 25, 25, 255 });
    }
}

// =============================================================================
// PHASE 3: HAUNTED WASHROOM WITH REAL PLANAR REFLECTION MIRROR
// =============================================================================

void UpdateShopWashroomMirror(Camera3D playerCam, ShopLightFn lightFn, bool lightsOn, float timeVal) {
    if (!g_shopAtmosphereLoaded) return;

    // Only update mirror reflection if player is near or inside the washroom / rear shop area
    bool inMirrorRange = (playerCam.position.x >= 84.0f && playerCam.position.x <= 96.0f &&
                          playerCam.position.z >= 146.0f && playerCam.position.z <= 163.0f);
    if (!inMirrorRange) return;

    // Mirrored camera: reflect Z across mirror plane Z = 160.85f
    Camera3D reflectCam = playerCam;
    const float mirrorZ = 160.85f;
    reflectCam.position.z = 2.0f * mirrorZ - playerCam.position.z;
    reflectCam.target.z   = 2.0f * mirrorZ - playerCam.target.z;

    BeginTextureMode(g_washroomMirrorRT);
    ClearBackground((Color){ 12, 14, 16, 255 });
    BeginMode3D(reflectCam);

    // 1. Reflected Washroom Shell: Floor, Ceiling, Walls
    Color rfFloor = lightFn((Vector3){ 89.0f, 10.0f, 157.0f }, (Color){ 180, 185, 175, 255 }, (Vector3){ 0, 1, 0 }, 0);
    DrawCube((Vector3){ 89.0f, 10.01f, 157.37f }, 7.0f, 0.01f, 7.25f, rfFloor);
    DrawCube((Vector3){ 89.0f, 14.50f, 157.37f }, 7.0f, 0.01f, 7.25f, (Color){ 35, 38, 42, 255 });

    // Reflected South wall looking back into shop with open doorway
    Color rfWall = lightFn((Vector3){ 89.0f, 12.0f, 153.8f }, (Color){ 140, 145, 135, 255 }, (Vector3){ 0, 0, 1 }, 0);
    DrawCube((Vector3){ 86.75f, 12.25f, 153.75f }, 2.5f, 4.5f, 0.15f, rfWall);
    DrawCube((Vector3){ 91.50f, 12.25f, 153.75f }, 2.0f, 4.5f, 0.15f, rfWall);
    DrawCube((Vector3){ 89.25f, 13.75f, 153.75f }, 2.5f, 1.5f, 0.15f, rfWall);

    // Reflected East and West side walls
    DrawCube((Vector3){ 85.50f, 12.25f, 157.37f }, 0.15f, 4.5f, 7.25f, rfWall);
    DrawCube((Vector3){ 92.50f, 12.25f, 157.37f }, 0.15f, 4.5f, 7.25f, rfWall);

    // 2. Reflected Toilet Fixture
    Vector3 rfTlt = { 87.2f, 10.0f, 159.2f };
    Color rfTltCol = lightFn(rfTlt, (Color){ 215, 218, 210, 255 }, (Vector3){ 0, 1, 0 }, 0);
    DrawCylinder(rfTlt, 0.22f, 0.28f, 0.45f, 10, rfTltCol);
    DrawCube({ rfTlt.x, 11.20f, 160.35f }, 0.58f, 0.65f, 0.28f, rfTltCol);

    // 3. Reflected Player Entity (Silhouette in front of the mirror!)
    Vector3 pPos = playerCam.position;
    // Dark silhouette of player standing in front of the mirror
    DrawCylinder({ pPos.x, pPos.y - 1.60f, pPos.z }, 0.30f, 0.26f, 1.35f, 10, (Color){ 16, 18, 22, 255 });
    DrawSphere({ pPos.x, pPos.y - 0.12f, pPos.z }, 0.20f, (Color){ 28, 30, 34, 255 });
    // Eerie reflective eyes glinting in the dark mirror glass
    float eyeBrightness = lightsOn ? 245.0f : 85.0f;
    Color eyeGleam = { (unsigned char)eyeBrightness, (unsigned char)eyeBrightness, 225, 255 };
    DrawSphere({ pPos.x - 0.065f, pPos.y - 0.10f, pPos.z - 0.14f }, 0.022f, eyeGleam);
    DrawSphere({ pPos.x + 0.065f, pPos.y - 0.10f, pPos.z - 0.14f }, 0.022f, eyeGleam);

    // 4. Reflected Washroom Overhead Light
    Vector3 wBulbPos = { 89.0f, 14.2f, 157.5f };
    Color bulbC = lightsOn ? (Color){ 245, 250, 200, 255 } : (Color){ 25, 28, 30, 255 };
    DrawSphere(wBulbPos, 0.10f, bulbC);

    EndMode3D();
    EndTextureMode();
}

void DrawShopHauntedWashroom(Camera3D camera, ShopLightFn lightFn, bool lightsOn, float timeVal) {
    if (!g_shopAtmosphereLoaded) return;

    // Washroom Bounding Volume: X in [85.5, 92.5], Z in [153.75, 161.0], Y in [10.0, 14.5]

    // -------------------------------------------------------------------------
    // 1. WASHROOM CERAMIC TILED FLOOR (Dirty, moldy, stained)
    // -------------------------------------------------------------------------
    rlSetTexture(g_texWashroomTiles.id);
    rlBegin(RL_QUADS);
    Vector3 normUp = { 0.0f, 1.0f, 0.0f };
    rlNormal3f(0.0f, 1.0f, 0.0f);

    for (float wx = 85.5f; wx <= 92.5f; wx += 1.75f) {
        for (float wz = 153.75f; wz <= 161.0f; wz += 1.81f) {
            float hx = 1.75f * 0.5f;
            float hz = 1.81f * 0.5f;
            float y = 10.018f;

            Vector3 v0 = { wx - hx, y, wz - hz };
            Vector3 v1 = { wx + hx, y, wz - hz };
            Vector3 v2 = { wx + hx, y, wz + hz };
            Vector3 v3 = { wx - hx, y, wz + hz };

            Color c0 = lightFn(v0, (Color){ 215, 218, 212, 255 }, normUp, 0);
            Color c1 = lightFn(v1, (Color){ 215, 218, 212, 255 }, normUp, 0);
            Color c2 = lightFn(v2, (Color){ 215, 218, 212, 255 }, normUp, 0);
            Color c3 = lightFn(v3, (Color){ 215, 218, 212, 255 }, normUp, 0);

            float u0 = (wx - hx - 85.5f) / 1.75f;
            float u1 = (wx + hx - 85.5f) / 1.75f;
            float w0 = (wz - hz - 153.75f) / 1.81f;
            float w1 = (wz + hz - 153.75f) / 1.81f;

            rlColor4ub(c0.r, c0.g, c0.b, c0.a); rlTexCoord2f(u0, w0); rlVertex3f(v0.x, v0.y, v0.z);
            rlColor4ub(c1.r, c1.g, c1.b, c1.a); rlTexCoord2f(u1, w0); rlVertex3f(v1.x, v1.y, v1.z);
            rlColor4ub(c2.r, c2.g, c2.b, c2.a); rlTexCoord2f(u1, w1); rlVertex3f(v2.x, v2.y, v2.z);
            rlColor4ub(c3.r, c3.g, c3.b, c3.a); rlTexCoord2f(u0, w1); rlVertex3f(v3.x, v3.y, v3.z);
        }
    }

    // -------------------------------------------------------------------------
    // 2. WASHROOM TILED WALLS
    // -------------------------------------------------------------------------
    // North Wall (Z = 160.95, facing -Z)
    Vector3 normN = { 0.0f, 0.0f, -1.0f };
    rlNormal3f(0.0f, 0.0f, -1.0f);
    for (float wx = 85.5f; wx <= 92.5f; wx += 1.75f) {
        float hx = 1.75f * 0.5f;
        Vector3 pBL = { wx - hx, 10.0f, 160.95f };
        Vector3 pTL = { wx - hx, 14.5f, 160.95f };
        Vector3 pTR = { wx + hx, 14.5f, 160.95f };
        Vector3 pBR = { wx + hx, 10.0f, 160.95f };

        Color cBL = lightFn(pBL, (Color){ 215, 218, 212, 255 }, normN, 0);
        Color cTL = lightFn(pTL, (Color){ 215, 218, 212, 255 }, normN, 0);
        Color cTR = lightFn(pTR, (Color){ 215, 218, 212, 255 }, normN, 0);
        Color cBR = lightFn(pBR, (Color){ 215, 218, 212, 255 }, normN, 0);

        rlColor4ub(cBL.r, cBL.g, cBL.b, cBL.a); rlTexCoord2f(0.0f, 2.0f); rlVertex3f(pBL.x, pBL.y, pBL.z);
        rlColor4ub(cTL.r, cTL.g, cTL.b, cTL.a); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(pTL.x, pTL.y, pTL.z);
        rlColor4ub(cTR.r, cTR.g, cTR.b, cTR.a); rlTexCoord2f(1.0f, 0.0f); rlVertex3f(pTR.x, pTR.y, pTR.z);
        rlColor4ub(cBR.r, cBR.g, cBR.b, cBR.a); rlTexCoord2f(1.0f, 2.0f); rlVertex3f(pBR.x, pBR.y, pBR.z);
    }

    // West Wall (X = 85.55, facing +X)
    Vector3 normW = { 1.0f, 0.0f, 0.0f };
    rlNormal3f(1.0f, 0.0f, 0.0f);
    for (float wz = 153.75f; wz <= 161.0f; wz += 1.81f) {
        float hz = 1.81f * 0.5f;
        Vector3 pBL = { 85.55f, 10.0f, wz - hz };
        Vector3 pTL = { 85.55f, 14.5f, wz - hz };
        Vector3 pTR = { 85.55f, 14.5f, wz + hz };
        Vector3 pBR = { 85.55f, 10.0f, wz + hz };

        Color cBL = lightFn(pBL, (Color){ 215, 218, 212, 255 }, normW, 0);
        Color cTL = lightFn(pTL, (Color){ 215, 218, 212, 255 }, normW, 0);
        Color cTR = lightFn(pTR, (Color){ 215, 218, 212, 255 }, normW, 0);
        Color cBR = lightFn(pBR, (Color){ 215, 218, 212, 255 }, normW, 0);

        rlColor4ub(cBL.r, cBL.g, cBL.b, cBL.a); rlTexCoord2f(0.0f, 2.0f); rlVertex3f(pBL.x, pBL.y, pBL.z);
        rlColor4ub(cTL.r, cTL.g, cTL.b, cTL.a); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(pTL.x, pTL.y, pTL.z);
        rlColor4ub(cTR.r, cTR.g, cTR.b, cTR.a); rlTexCoord2f(1.0f, 0.0f); rlVertex3f(pTR.x, pTR.y, pTR.z);
        rlColor4ub(cBR.r, cBR.g, cBR.b, cBR.a); rlTexCoord2f(1.0f, 2.0f); rlVertex3f(pBR.x, pBR.y, pBR.z);
    }

    // East Wall (X = 92.45, facing -X)
    Vector3 normE = { -1.0f, 0.0f, 0.0f };
    rlNormal3f(-1.0f, 0.0f, 0.0f);
    for (float wz = 153.75f; wz <= 161.0f; wz += 1.81f) {
        float hz = 1.81f * 0.5f;
        Vector3 pBL = { 92.45f, 10.0f, wz + hz };
        Vector3 pTL = { 92.45f, 14.5f, wz + hz };
        Vector3 pTR = { 92.45f, 14.5f, wz - hz };
        Vector3 pBR = { 92.45f, 10.0f, wz - hz };

        Color cBL = lightFn(pBL, (Color){ 215, 218, 212, 255 }, normE, 0);
        Color cTL = lightFn(pTL, (Color){ 215, 218, 212, 255 }, normE, 0);
        Color cTR = lightFn(pTR, (Color){ 215, 218, 212, 255 }, normE, 0);
        Color cBR = lightFn(pBR, (Color){ 215, 218, 212, 255 }, normE, 0);

        rlColor4ub(cBL.r, cBL.g, cBL.b, cBL.a); rlTexCoord2f(0.0f, 2.0f); rlVertex3f(pBL.x, pBL.y, pBL.z);
        rlColor4ub(cTL.r, cTL.g, cTL.b, cTL.a); rlTexCoord2f(0.0f, 0.0f); rlVertex3f(pTL.x, pTL.y, pTL.z);
        rlColor4ub(cTR.r, cTR.g, cTR.b, cTR.a); rlTexCoord2f(1.0f, 0.0f); rlVertex3f(pTR.x, pTR.y, pTR.z);
        rlColor4ub(cBR.r, cBR.g, cBR.b, cBR.a); rlTexCoord2f(1.0f, 2.0f); rlVertex3f(pBR.x, pBR.y, pBR.z);
    }
    rlEnd();
    rlSetTexture(0);

    // Washroom Ceiling
    DrawCube({ 89.0f, 14.50f, 157.37f }, 7.0f, 0.08f, 7.25f, (Color){ 28, 30, 34, 255 });

    // -------------------------------------------------------------------------
    // 3. RESTROOM ENTRANCE DOORWAY (In North Shop Wall at X = 89.25)
    // -------------------------------------------------------------------------
    {
        Vector3 jambL = { 88.0f, 11.5f, 153.75f };
        Vector3 jambR = { 90.5f, 11.5f, 153.75f };
        Color frameWood = lightFn(jambL, (Color){ 45, 32, 22, 255 }, (Vector3){ 0, 0, -1 }, 0);
        DrawCube(jambL, 0.14f, 3.0f, 0.22f, frameWood);
        DrawCube(jambR, 0.14f, 3.0f, 0.22f, frameWood);
        DrawCube({ 89.25f, 13.0f, 153.75f }, 2.64f, 0.14f, 0.22f, frameWood);

        // Use the interaction/collision state; it was previously always drawn open.
        rlPushMatrix();
        rlTranslatef(88.05f, 10.0f, 153.75f);
        rlRotatef(g_washroomDoorAngle, 0.0f, 1.0f, 0.0f);
        DrawCube({ 1.15f, 1.45f, 0.0f }, 2.30f, 2.90f, 0.06f, lightFn((Vector3){ 89.0f, 11.5f, 154.5f }, (Color){ 58, 40, 28, 255 }, (Vector3){ 0, 0, 1 }, 0));
        // Tarnished brass door knob
        DrawSphere({ 2.10f, 1.45f, 0.04f }, 0.035f, (Color){ 195, 160, 65, 255 });
        // Tarnished brass "RESTROOM" nameplate
        DrawCube({ 1.15f, 1.95f, -0.035f }, 0.45f, 0.12f, 0.015f, (Color){ 180, 145, 55, 255 });
        rlPopMatrix();
    }

    // -------------------------------------------------------------------------
    // 4. HAUNTED PORCELAIN TOILET (North-West Corner at X = 87.2, Z = 159.2)
    // -------------------------------------------------------------------------
    {
        Vector3 tltPos = { 87.20f, 10.0f, 159.20f };
        Color porcelainCol = lightFn(tltPos, (Color){ 220, 222, 215, 255 }, (Vector3){ 0, 1, 0 }, 0);
        Color dirtCol      = lightFn(tltPos, (Color){ 85, 45, 28, 255 }, (Vector3){ 0, 1, 0 }, 0);

        // Coagulated floor blood puddle pooling beneath the toilet flange
        DrawCircle3D({ tltPos.x, 10.024f, tltPos.z }, 0.85f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 95, 10, 14, 240 });
        DrawCircle3D({ tltPos.x + 0.25f, 10.025f, tltPos.z - 0.20f }, 0.52f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 75, 6, 10, 255 });

        // Porcelain pedestal base
        DrawCylinder(tltPos, 0.22f, 0.28f, 0.44f, 12, porcelainCol);
        // Flared oval bowl
        DrawCylinder({ tltPos.x, tltPos.y + 0.44f, tltPos.z }, 0.28f, 0.25f, 0.32f, 12, porcelainCol);

        // Discolored murky stagnant water in bowl
        DrawCircle3D({ tltPos.x, tltPos.y + 0.62f, tltPos.z }, 0.18f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 58, 32, 20, 240 });

        // Dark dried blood drip marks running down the porcelain bowl
        DrawLine3D({ tltPos.x - 0.12f, tltPos.y + 0.72f, tltPos.z - 0.15f }, { tltPos.x - 0.14f, tltPos.y + 0.25f, tltPos.z - 0.18f }, (Color){ 90, 8, 12, 255 });
        DrawLine3D({ tltPos.x + 0.14f, tltPos.y + 0.70f, tltPos.z - 0.12f }, { tltPos.x + 0.16f, tltPos.y + 0.18f, tltPos.z - 0.16f }, (Color){ 95, 10, 14, 255 });

        // Toilet seat (tilted slightly crooked/askew)
        rlPushMatrix();
        rlTranslatef(tltPos.x, tltPos.y + 0.76f, tltPos.z);
        rlRotatef(12.0f, 0.0f, 1.0f, 0.0f); // Crooked seat
        DrawCylinder({ 0, 0, 0 }, 0.29f, 0.29f, 0.045f, 12, (Color){ 35, 30, 28, 255 }); // Dark plastic seat
        DrawCylinder({ 0, 0.005f, 0 }, 0.18f, 0.18f, 0.05f, 10, porcelainCol); // Inner hole
        rlPopMatrix();

        // Water tank against the North wall
        Vector3 tankPos = { tltPos.x, tltPos.y + 1.15f, 160.35f };
        DrawCube(tankPos, 0.58f, 0.65f, 0.28f, porcelainCol);
        DrawCubeWires(tankPos, 0.59f, 0.66f, 0.29f, dirtCol);
        // Tank lid
        DrawCube({ tankPos.x, tankPos.y + 0.35f, tankPos.z }, 0.62f, 0.06f, 0.32f, porcelainCol);
        // Chrome flush lever
        DrawCylinderEx({ tankPos.x - 0.31f, tankPos.y + 0.20f, tankPos.z }, { tankPos.x - 0.35f, tankPos.y + 0.16f, tankPos.z + 0.06f }, 0.012f, 0.012f, 6, (Color){ 200, 205, 210, 255 });
        // Chrome water supply line into wall
        DrawCylinderEx({ tltPos.x + 0.18f, 10.02f, 160.4f }, { tltPos.x + 0.18f, tankPos.y - 0.32f, 160.4f }, 0.012f, 0.012f, 6, (Color){ 180, 185, 190, 255 });
    }

    // -------------------------------------------------------------------------
    // 5. PORCELAIN WALL SINK BASIN (Mounted below mirror at X = 89.25, Z = 160.65)
    // -------------------------------------------------------------------------
    {
        Vector3 snkPos = { 89.25f, 11.20f, 160.65f };
        Color sinkCol = lightFn(snkPos, (Color){ 225, 228, 222, 255 }, (Vector3){ 0, 1, 0 }, 0);
        Color dirtRim = lightFn(snkPos, (Color){ 45, 38, 30, 255 }, (Vector3){ 0, 1, 0 }, 0);

        // Rectangular porcelain sink basin
        DrawCube(snkPos, 0.82f, 0.35f, 0.50f, sinkCol);
        DrawCubeWires(snkPos, 0.83f, 0.36f, 0.51f, dirtRim);

        // Concave basin bowl cavity
        DrawCube({ snkPos.x, snkPos.y + 0.08f, snkPos.z }, 0.68f, 0.22f, 0.38f, (Color){ 190, 195, 188, 255 });

        // Grimy black drain flange
        DrawCircle3D({ snkPos.x, snkPos.y - 0.02f, snkPos.z }, 0.045f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 18, 20, 22, 255 });

        // Tarnished chrome double faucets (Hot & Cold) dripping rust water
        DrawCylinderEx({ snkPos.x - 0.18f, snkPos.y + 0.18f, 160.85f }, { snkPos.x - 0.18f, snkPos.y + 0.28f, 160.75f }, 0.015f, 0.015f, 6, (Color){ 195, 200, 205, 255 });
        DrawCylinderEx({ snkPos.x + 0.18f, snkPos.y + 0.18f, 160.85f }, { snkPos.x + 0.18f, snkPos.y + 0.28f, 160.75f }, 0.015f, 0.015f, 6, (Color){ 195, 200, 205, 255 });
        // Center spout
        DrawCylinderEx({ snkPos.x, snkPos.y + 0.18f, 160.88f }, { snkPos.x, snkPos.y + 0.26f, 160.70f }, 0.018f, 0.018f, 6, (Color){ 195, 200, 205, 255 });
        // Red rusty water droplet dripping from spout
        DrawSphere({ snkPos.x, snkPos.y + 0.08f, 160.70f }, 0.014f, (Color){ 125, 20, 24, 220 });

        // The faucet interaction now has an in-world result, not just HUD text.
        if (IsWashroomSinkRunning()) {
            Color water = lightsOn ? Color{ 125, 190, 220, 210 } : Color{ 55, 85, 100, 190 };
            DrawCylinderEx({ snkPos.x, snkPos.y + 0.22f, 160.70f },
                           { snkPos.x, snkPos.y - 0.03f, 160.70f },
                           0.018f, 0.010f, 6, water);
            DrawCircle3D({ snkPos.x, snkPos.y - 0.025f, 160.70f }, 0.11f,
                         { 1, 0, 0 }, 90.0f, Fade(water, 0.55f));
        }

        // Chrome P-trap pipe underneath going into North wall
        DrawCylinderEx({ snkPos.x, snkPos.y - 0.18f, snkPos.z }, { snkPos.x, snkPos.y - 0.42f, snkPos.z }, 0.018f, 0.018f, 6, (Color){ 180, 185, 190, 255 });
        DrawCylinderEx({ snkPos.x, snkPos.y - 0.42f, snkPos.z }, { snkPos.x, snkPos.y - 0.42f, 160.92f }, 0.018f, 0.018f, 6, (Color){ 180, 185, 190, 255 });
    }

    // -------------------------------------------------------------------------
    // 6. REAL PLANAR REFLECTION MIRROR (North Wall at X = 89.25, Z = 160.83)
    // -------------------------------------------------------------------------
    {
        float mw = 1.40f * 0.5f;
        float mh = 1.10f * 0.5f;
        float mx = 89.25f;
        float my = 12.35f;
        float mz = 160.83f;

        // Heavy tarnished brass / rusted steel frame
        Color frameCol = lightFn((Vector3){ mx, my, mz }, (Color){ 165, 125, 45, 255 }, (Vector3){ 0, 0, -1 }, 0);
        DrawCube((Vector3){ mx, my, mz + 0.015f }, 1.48f, 1.18f, 0.04f, frameCol);
        DrawCubeWires((Vector3){ mx, my, mz + 0.015f }, 1.49f, 1.19f, 0.045f, (Color){ 45, 30, 15, 255 });

        // A. Live Mirrored Reflection Quad (RenderTexture2D)
        rlSetTexture(g_washroomMirrorRT.texture.id);
        rlBegin(RL_QUADS);
        rlNormal3f(0.0f, 0.0f, -1.0f);
        rlColor4ub(255, 255, 255, 255);
        // Horizontally flipped for true optical mirror behavior
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(mx - mw, my - mh, mz);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(mx + mw, my - mh, mz);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(mx + mw, my + mh, mz);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(mx - mw, my + mh, mz);
        rlEnd();
        rlSetTexture(0);

        // B. Mirror Grime, Silvering Rot & Bloody Handprint Overlay
        rlSetTexture(g_texMirrorGrime.id);
        rlBegin(RL_QUADS);
        rlNormal3f(0.0f, 0.0f, -1.0f);
        rlColor4ub(255, 255, 255, 245);
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(mx - mw, my - mh, mz - 0.003f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(mx + mw, my - mh, mz - 0.003f);
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(mx + mw, my + mh, mz - 0.003f);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(mx - mw, my + mh, mz - 0.003f);
        rlEnd();
        rlSetTexture(0);
    }

    // -------------------------------------------------------------------------
    // 7. WASHROOM OVERHEAD FLICKERING BARE BULB FIXTURE
    // -------------------------------------------------------------------------
    {
        Vector3 fixtureBase = { 89.0f, 14.48f, 157.5f };
        Vector3 bulbPos     = { 89.0f, 13.85f, 157.5f };

        // Ceiling porcelain rosette
        DrawCylinder(fixtureBase, 0.12f, 0.12f, 0.04f, 10, (Color){ 215, 210, 200, 255 });
        // Twisted drop cord
        DrawLine3D(fixtureBase, bulbPos, (Color){ 25, 25, 28, 255 });
        // Brass lamp socket
        DrawCylinder({ bulbPos.x, bulbPos.y + 0.08f, bulbPos.z }, 0.04f, 0.04f, 0.08f, 8, (Color){ 175, 140, 50, 255 });

        // Flickering sick greenish-white tungsten bulb
        float flick = lightsOn ? (0.88f + 0.12f * sinf(timeVal * 19.5f) * ((GetRandomValue(0, 100) > 4) ? 1.0f : 0.35f)) : 0.0f;
        Color bCol = lightsOn ?
            (Color){ (unsigned char)(235 * flick), (unsigned char)(248 * flick), (unsigned char)(205 * flick), 255 } :
            (Color){ 28, 30, 32, 255 };

        DrawSphere(bulbPos, 0.085f, bCol);
        DrawCubeWires(bulbPos, 0.22f, 0.22f, 0.22f, (Color){ 110, 90, 45, 180 }); // Wire cage

        if (lightsOn) {
            // Sickly radial light bloom halo
            DrawSphere(bulbPos, 0.32f, (Color){ 220, 245, 195, (unsigned char)(45 * flick) });
            // Floor light pool
            DrawCircle3D({ bulbPos.x, 10.025f, bulbPos.z }, 3.4f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 215, 240, 190, (unsigned char)(45 * flick) });
            DrawCircle3D({ bulbPos.x, 10.026f, bulbPos.z }, 1.4f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 235, 250, 210, (unsigned char)(75 * flick) });
        }
    }
}

void UnloadShopAtmosphere() {
    if (!g_shopAtmosphereLoaded) return;

    UnloadTexture(g_texWoodPanels);
    UnloadTexture(g_texGrunge);
    UnloadTexture(g_texCeilingTile);
    UnloadTexture(g_texPosterMissing);
    UnloadTexture(g_texPosterTinSign);
    UnloadTexture(g_texShopFloor);
    UnloadTexture(g_texWashroomTiles);
    UnloadTexture(g_texMirrorGrime);
    UnloadRenderTexture(g_washroomMirrorRT);

    g_shopAtmosphereLoaded = false;
}

// -------------------------------------------------------------------------
// WASHROOM INTERACTION & SECURITY MONITOR IMPLEMENTATIONS
// -------------------------------------------------------------------------

static bool s_washroomSinkRunning = false;
static bool s_washroomDoorOpen = false;
float g_washroomDoorAngle = 0.0f;

bool IsPlayerNearWashroomSink(Vector3 playerPos) {
    Vector3 sinkPos = { 89.25f, 11.20f, 160.65f };
    return Vector3Distance(playerPos, sinkPos) < 2.0f;
}

void ToggleWashroomSinkFaucet() {
    s_washroomSinkRunning = !s_washroomSinkRunning;
}

bool IsWashroomSinkRunning() {
    return s_washroomSinkRunning;
}

bool IsPlayerNearWashroomDoor(Vector3 playerPos) {
    Vector3 doorPos = { 89.25f, 11.5f, 153.75f };
    return Vector3Distance(playerPos, doorPos) < 2.4f;
}

void ToggleWashroomDoor() {
    s_washroomDoorOpen = !s_washroomDoorOpen;
    g_washroomDoorAngle = s_washroomDoorOpen ? 85.0f : 0.0f;
}

bool IsWashroomDoorOpen() {
    return s_washroomDoorOpen;
}

void UpdateCounterCCTV(Camera3D playerCam, ShopLightFn lightFn, bool lightsOn, float timeVal) {
    (void)playerCam;
    (void)lightFn;
    (void)lightsOn;
    (void)timeVal;
}

void DrawCounterSecurityMonitor(Vector3 pos, ShopLightFn lightFn, bool lightsOn, float timeVal) {
    (void)pos;
    (void)lightFn;
    (void)lightsOn;
    (void)timeVal;
}

void DrawShopSuperstoreGondolas(ShopLightFn lightFn, bool lightsOn, float timeVal) {
    (void)lightFn;
    (void)lightsOn;
    (void)timeVal;
    if (!g_shopAtmosphereLoaded) return;
    (void)lightsOn; (void)timeVal;

    float shelfY[4] = { 10.39f, 11.43f, 12.48f, 13.53f };
    float zCenters[2] = { 140.0f, 147.0f };
    float cX = 95.0f; 
    float xLen = 12.0f; // 89 to 101
    
    Color metalCol = { 205, 210, 215, 255 };
    Color backCol = { 220, 225, 230, 255 };
    Color baseCol = { 45, 50, 55, 255 };
    
    for (int i = 0; i < 2; i++) {
        float zC = zCenters[i];
        
        Vector3 basePos = { cX, 10.15f, zC };
        DrawCube(basePos, xLen, 0.3f, 1.2f, lightFn(basePos, baseCol, (Vector3){0,1,0}, 0));
        DrawCubeWires(basePos, xLen+0.01f, 0.31f, 1.21f, (Color){ 20, 25, 30, 255 });
        
        Vector3 backPos = { cX, 12.0f, zC };
        DrawCube(backPos, xLen, 4.0f, 0.05f, lightFn(backPos, backCol, (Vector3){0,1,0}, 0));
        
        for (int s = 0; s < 4; s++) {
            float sy = shelfY[s] - 0.04f;
            Vector3 northPos = { cX, sy, zC + 0.30f };
            DrawCube(northPos, xLen, 0.04f, 0.5f, lightFn(northPos, metalCol, (Vector3){0,1,0}, 0));
            DrawCubeWires(northPos, xLen+0.01f, 0.05f, 0.51f, (Color){ 150, 160, 170, 255 });
            Vector3 southPos = { cX, sy, zC - 0.30f };
            DrawCube(southPos, xLen, 0.04f, 0.5f, lightFn(southPos, metalCol, (Vector3){0,1,0}, 0));
            DrawCubeWires(southPos, xLen+0.01f, 0.05f, 0.51f, (Color){ 150, 160, 170, 255 });
        }
    }
}
