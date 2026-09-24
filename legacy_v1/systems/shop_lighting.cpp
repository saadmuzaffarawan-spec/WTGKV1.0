#include "shop_lighting.h"
#include "procedural_math.inl"
#include "receipt_printer.h"
#include <rlgl.h>
#include <cmath>

extern float   g_curExtDayFactor;
extern float   g_curExtNightFactor;
extern float   g_curLightningFlash;
extern Vector3 g_curSunDir;
extern bool    g_flashlightActive;
extern Vector3 g_playerCamFwd;
extern Vector3 g_playerCamPos;
extern bool    g_shopLightsOn;

ShopLightingContext g_shopLighting;

// =========================================================================

// SHOP LIGHTING CONTEXT & PHYSICAL MULTI-LIGHT ILLUMINATION ENGINE

// Evaluates light contributions from the central swaying bulb and ceiling tubelights

// =========================================================================


void SetShopLight(int idx, Vector3 pos, Color color, float intensity, float radius) {

    if (idx < 0 || idx >= 8) return;

    ShopLightSource& lt = g_shopLighting.lights[idx];

    lt.pos = pos;

    lt.color = color;

    lt.intensity = intensity;

    lt.radius = radius;

    lt.radiusSq = radius * radius;

    lt.invRadius = (radius > 0.0001f) ? (1.0f / radius) : 0.0f;

    lt.colR = color.r * (1.0f / 255.0f);

    lt.colG = color.g * (1.0f / 255.0f);

    lt.colB = color.b * (1.0f / 255.0f);

}



// (Planar shadow projections completely removed to prevent all stretching barrier artifacts)



// Physically grounded dynamic shop lighting:

// - Full interior boundary enclosing all 4 walls, ceiling, and entrance door

// - Lambertian diffuse (N . L) with soft bounce wrap

// - Smooth inverse-square law attenuation with windowed boundary

Color ApplyShopLighting(Vector3 pos, Color baseAlbedo, Vector3 normal, int occludeAisle) {

    bool insideMainShop = (pos.x >= 85.8f && pos.x <= 108.15f && pos.z >= 125.8f && pos.z <= 154.2f);
    bool insideWashroom = (pos.x >= 85.2f && pos.x <= 92.8f && pos.z >= 153.5f && pos.z <= 161.2f);
    bool insideStore = (insideMainShop || insideWashroom) && (pos.y >= 9.8f && pos.y <= 16.0f);

    bool inBunker    = (pos.y < 9.5f);



    float rAcc = 0.0f;

    float gAcc = 0.0f;

    float bAcc = 0.0f;



    // 1. Indoor Shop Fixtures (Swinging tungsten pendant, ceiling tubelights, checkout spotlight, freezer LEDs, washroom light)

    // Only active when the master store power switch is ON!

    if (g_shopLightsOn && insideStore) {

        for (int i = 0; i < 8; i++) {

            const ShopLightSource& lt = g_shopLighting.lights[i];

            if (lt.intensity <= 0.001f) continue;



            // Rack divider occlusion (only in main shop aisle):

            if (!insideWashroom) {
                // occludeAisle == 1: Target is South of Rack 1 (Z < 140). Block light from Aisle 2 & 3 (Z > 140)!
                if (occludeAisle == 1 && lt.pos.z > 140.0f) continue;
                // occludeAisle == 2: Target is North of Rack 2 (Z > 147). Block light from Aisle 1 & 2 (Z < 147)!
                if (occludeAisle == 2 && lt.pos.z < 147.0f) continue;
            }



            float dx = lt.pos.x - pos.x;

            float dy = lt.pos.y - pos.y;

            float dz = lt.pos.z - pos.z;

            float distSq = dx * dx + dy * dy + dz * dz;



            if (distSq < lt.radiusSq) {

                float dist = sqrtf(distSq);

                if (dist < 0.001f) dist = 0.001f;

                float invDist = 1.0f / dist;

                float lx = dx * invDist;

                float ly = dy * invDist;

                float lz = dz * invDist;



                float nDotL = normal.x * lx + normal.y * ly + normal.z * lz;
                // Physically grounded half-Lambert diffuse with material specular sheen
                float diffuse = (nDotL > 0.0f) ? (0.32f + 0.68f * nDotL) : 0.22f;
                float spec = (nDotL > 0.0f) ? (powf(nDotL, 14.0f) * 0.28f) : 0.0f;

                float win = 1.0f - (distSq / lt.radiusSq);
                float atten = (win * win) / (1.0f + 0.04f * dist + 0.025f * distSq);

                // Primary central swaying bulb: rich omnidirectional radiance across meat and aisle
                if (i == 0) {
                    atten *= 1.35f;
                }

                float eff = lt.intensity * atten * (diffuse + spec);
                rAcc += lt.colR * eff;
                gAcc += lt.colG * eff;
                bAcc += lt.colB * eff;

            }

        }

    }



    // 2. Exterior Lighting Pipeline (Direct Sun, Moon, Twilight, Storm Lightning, Streetlamps)

    if (!insideStore && !inBunker) {

        float extSunVal = fmaxf(0.0f, g_curSunDir.y) * g_curExtDayFactor;

        float extMoonVal = fmaxf(0.0f, -g_curSunDir.y) * g_curExtNightFactor;



        rAcc += extSunVal * 1.15f;

        gAcc += extSunVal * 1.05f;

        bAcc += extSunVal * 0.85f;



        rAcc += extMoonVal * 0.25f;

        gAcc += extMoonVal * 0.35f;

        bAcc += extMoonVal * 0.60f;



        float amb = 0.06f + g_curExtDayFactor * 0.38f + g_curExtNightFactor * 0.05f;

        rAcc += amb * 0.90f;

        gAcc += amb * 0.95f;

        bAcc += amb * 1.10f;



        if (g_curLightningFlash > 0.01f) {

            float flash = g_curLightningFlash * 1.6f;

            rAcc += flash * 0.95f;

            gAcc += flash * 1.05f;

            bAcc += flash * 1.20f;

        }



        // Exterior Sodium & Fluorescent Light Pools

        struct ExtLamp { Vector3 p; float rSq; float r, g, b, power; };

        ExtLamp lamps[4] = {

            { { 112.6f, 14.65f, 127.0f }, 196.0f, 1.25f, 1.05f, 0.55f, 1.8f },

            { { 112.6f, 14.65f, 153.0f }, 196.0f, 1.25f, 1.05f, 0.55f, 1.8f },

            { {  82.0f, 14.55f, 140.0f }, 144.0f, 1.30f, 0.95f, 0.35f, 1.6f },

            { { 128.0f, 15.00f, 140.0f }, 256.0f, 1.05f, 1.15f, 1.30f, 2.2f }

        };

        for (int l = 0; l < 4; l++) {

            float ldx = lamps[l].p.x - pos.x;

            float ldy = lamps[l].p.y - pos.y;

            float ldz = lamps[l].p.z - pos.z;

            float lDistSq = ldx * ldx + ldy * ldy + ldz * ldz;

            if (lDistSq < lamps[l].rSq) {

                float lDist = sqrtf(lDistSq);

                float lAtten = (1.0f - lDistSq / lamps[l].rSq) / (1.0f + 0.1f * lDist + 0.05f * lDistSq);

                float lEff = lAtten * lamps[l].power;

                rAcc += lamps[l].r * lEff;

                gAcc += lamps[l].g * lEff;

                bAcc += lamps[l].b * lEff;

            }

        }

    } else if (insideStore) {

        if (g_shopLightsOn) {

            // Grounded Industrial Realism: contrast-heavy moody ambient that lets spotlights carve the space
            rAcc += 0.08f * 1.02f;
            gAcc += 0.08f * 1.00f;
            bAcc += 0.09f * 0.94f;

        } else {

            // Realistic atmospheric horror darkness:
            // Shapes, contours, textures, and silhouettes remain perceptible in the deep nocturnal gloom (not flat black!)
            float darkGloom = 0.054f;
            rAcc += darkGloom * 0.72f;
            gAcc += darkGloom * 0.84f;
            bAcc += darkGloom * 1.18f; // Cold atmospheric indigo-slate nocturne

            // Soft natural light from outside (pale cool-blue moonlight at night, sun/skylight during day)
            // spills through the transparent glass front entrance doorway into the front lobby.
            if (pos.x > 102.5f && pos.z >= 136.0f && pos.z <= 144.0f) {

                float doorDist = (pos.x - 102.5f) / 5.5f; // 0.0 at interior threshold, 1.0 at glass door

                float daySpill  = g_curExtDayFactor * doorDist * 0.35f;

                float moonSpill = g_curExtNightFactor * doorDist * 0.18f;

                // Daylight is warm white; moonlight is cold pale blue:

                rAcc += daySpill * 1.05f + moonSpill * 0.22f;

                gAcc += daySpill * 1.00f + moonSpill * 0.32f;

                bAcc += daySpill * 0.90f + moonSpill * 0.58f;

            }

        }

    } else {

        // Bunker interior

        rAcc += 0.020f * 0.95f;

        gAcc += 0.020f * 1.00f;

        bAcc += 0.020f * 1.10f;

    }



    // 3. Handheld Player Flashlight ([F]) with Parabolic Reflector Double-Cone Optics

    if (g_flashlightActive) {

        Vector3 toPos = Vector3Subtract(pos, g_playerCamPos);

        float distSq = Vector3LengthSqr(toPos);

        if (distSq < 0.92f * 0.92f) {

            // First-person held items & weapon viewmodel illumination

            rAcc += 1.35f;

            gAcc += 1.32f;

            bAcc += 1.25f;

        } else if (distSq < 42.0f * 42.0f) {

            float dist = sqrtf(distSq);

            Vector3 dir = Vector3Scale(toPos, 1.0f / dist);

            float dotFwd = Vector3DotProduct(g_playerCamFwd, dir);

            

            // Double-cone parabolic optics:

            // Hotspot core: dotFwd >= 0.975 (~12.8 deg half-angle)

            // Penumbra spill: dotFwd >= 0.848 (~32 deg half-angle)

            if (dotFwd > 0.848f) {

                float spotFactor = 0.0f;

                if (dotFwd >= 0.975f) {

                    spotFactor = 1.0f + (dotFwd - 0.975f) / 0.025f * 0.85f; // Intense bright core

                } else {

                    float s = (dotFwd - 0.848f) / (0.975f - 0.848f);

                    spotFactor = s * s * (3.0f - 2.0f * s);

                }

                

                // Physical inverse-square attenuation

                float atten = 1.0f / (1.0f + 0.035f * dist + 0.012f * distSq);

                float flashEff = spotFactor * atten * 4.8f;

                

                // High-CRI crisp white LED illumination

                rAcc += 1.25f * flashEff;

                gAcc += 1.22f * flashEff;

                bAcc += 1.15f * flashEff;

            }

        }

    }



    int outR = (int)(baseAlbedo.r * rAcc);

    int outG = (int)(baseAlbedo.g * gAcc);

    int outB = (int)(baseAlbedo.b * bAcc);



    return Color{
        (unsigned char)Clamp((float)outR, 0.0f, 255.0f),
        (unsigned char)Clamp((float)outG, 0.0f, 255.0f),
        (unsigned char)Clamp((float)outB, 0.0f, 255.0f),
        baseAlbedo.a
    };

}



float GetShopLightFactorAt(Vector3 pos) {

    float lum = 0.25f;

    for (int i = 0; i < 6; i++) {

        const ShopLightSource& lt = g_shopLighting.lights[i];

        if (lt.intensity <= 0.001f) continue;

        float dx = pos.x - lt.pos.x;

        float dy = pos.y - lt.pos.y;

        float dz = pos.z - lt.pos.z;

        float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < lt.radiusSq) {

            float dist = sqrtf(distSq);

            float win = 1.0f - (dist * lt.invRadius);

            float atten = (win * win) / (1.0f + 0.10f * dist + 0.08f * distSq);

            lum += lt.intensity * atten;

        }

    }

    return lum;

}



// ----------------------------------------------------------------------

// Drawing routines for each hero superstore prop.

// All shapes procedural — layered primitives + soft lighting + textures.

// ----------------------------------------------------------------------



void DrawPopcornTin(Vector3 pos, bool opened, float wobble, bool held, float distSq)

{

    float r = 0.13f, h = 0.22f;

    Color tinRed = ApplyShopLighting(pos, Color{ 210, 40, 40, 255 });

    // Body cylinder

    DrawCylinder(pos, r, r, h, 14, tinRed);



    // High-detail stripes and wireframe outlines only when up close (< 3.5m) or held

    if (distSq < 12.0f || held) {

        DrawCylinderWires(pos, r, r, h, 14, ApplyShopLighting(pos, Fade(BLACK, 0.45f)));

        for (int i = 0; i < 8; i++) {

            float a0 = (float)i / 8.0f * 2.0f * PI;

            float a1 = (float)(i + 1) / 8.0f * 2.0f * PI;

            Color stripe = (i % 2 == 0) ? WHITE : Color{20, 40, 160, 255};

            Vector3 p0 = { pos.x + cosf(a0) * (r + 0.002f), pos.y + h * 0.55f, pos.z + sinf(a0) * (r + 0.002f) };

            Vector3 p1 = { pos.x + cosf(a1) * (r + 0.002f), pos.y + h * 0.55f, pos.z + sinf(a1) * (r + 0.002f) };

            DrawCylinderEx(p0, p1, 0.012f, 0.012f, 4, ApplyShopLighting(p0, stripe));

        }

    }



    // Lid

    Vector3 lidBase = { pos.x, pos.y + h + (opened ? (0.05f + wobble) : 0.0f), pos.z };

    Color lidCol = ApplyShopLighting(lidBase, Color{ 230, 230, 230, 255 });

    DrawCylinder(lidBase, r * 1.03f, r * 1.03f, 0.02f, 14, lidCol);

    if (distSq < 12.0f || held) {

        DrawCylinderWires(lidBase, r * 1.03f, r * 1.03f, 0.02f, 14, ApplyShopLighting(lidBase, Fade(BLACK, 0.4f)));

    }



    // Popcorn peeking out the open top

    if (opened) {

        for (int i = 0; i < 5; i++) {

            float poffX = cosf((float)i * 1.25f) * 0.06f;

            float poffZ = sinf((float)i * 1.25f) * 0.06f;

            Vector3 popPos = { pos.x + poffX, pos.y + h + 0.025f, pos.z + poffZ };

            DrawSphere(popPos, 0.026f, (i % 2 == 0) ? Color{ 255, 248, 205, 255 } : Color{ 245, 215, 110, 255 });

        }

    }

}



void DrawBottle(Vector3 pos, float bottleH, float bottleR,

                       Model &liquidModel, float fill, Color glassColor,

                       bool opened, bool held, Vector3 fwdDir, float distSq)

{

    rlPushMatrix();

    rlTranslatef(pos.x, pos.y, pos.z);



    // If held and pouring, tilt the bottle forward to pour!

    if (held && opened) {

        Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwdDir, Vector3{0, 1, 0}));

        rlRotatef(65.0f, rgt.x, rgt.y, rgt.z);

    }



    Vector3 localPos = { 0, 0, 0 };

    Color litGlass = ApplyShopLighting(pos, glassColor);



    // Fast LOD for distant bottles (> 4m): Single cylinder captures the exact color and silhouette

    if (distSq >= 16.0f && !held) {

        DrawCylinder(localPos, bottleR, bottleR * 0.92f, bottleH, 8, litGlass);

        rlPopMatrix();

        return;

    }



    // Translucent glass outer wall

    DrawCylinder(localPos, bottleR, bottleR * 0.92f, bottleH * 0.82f, 14, Fade(litGlass, 0.32f));

    if (distSq < 12.0f || held) {

        DrawCylinderWires(localPos, bottleR, bottleR * 0.92f, bottleH * 0.82f, 14, Fade(litGlass, 0.55f));

    }



    // Neck

    Vector3 neckBase = { 0, bottleH * 0.82f, 0 };

    DrawCylinder(neckBase, bottleR * 0.45f, bottleR * 0.4f, bottleH * 0.14f, 12, Fade(litGlass, 0.32f));



    // Cap/lip

    Vector3 lip = { 0, bottleH * 0.96f, 0 };

    DrawCylinder(lip, bottleR * 0.46f, bottleR * 0.46f, bottleH * 0.04f, 12, ApplyShopLighting(pos, LIGHTGRAY));



    // Inner liquid cylinder scaled by fill ratio

    if (fill > 0.01f) {

        float liquidH = bottleH * 0.80f * fill;

        float innerR  = bottleR * 0.85f;

        rlPushMatrix();

            rlTranslatef(0, 0.01f, 0);

            rlScalef(innerR / 0.5f, liquidH / 1.0f, innerR / 0.5f);

            Color liquidTint = ApplyShopLighting(pos, WHITE);

            DrawModel(liquidModel, Vector3{ 0, 0, 0 }, 1.0f, liquidTint);

        rlPopMatrix();

    }



    rlPopMatrix();

}



void DrawBread(Vector3 pos, Model &crustModel, bool held, float distSq)

{

    Color litTint = ApplyShopLighting(pos, WHITE);

    // Main oblong loaf body

    rlPushMatrix();

        rlTranslatef(pos.x, pos.y + 0.09f, pos.z);

        rlScalef(1.7f, 0.75f, 1.0f);

        DrawModel(crustModel, Vector3{ 0, 0, 0 }, 1.0f, litTint);

    rlPopMatrix();



    // Baked domed top hump only when close (< 3.5m) or held

    if (distSq < 12.0f || held) {

        rlPushMatrix();

            rlTranslatef(pos.x, pos.y + 0.155f, pos.z);

            rlScalef(1.25f, 0.55f, 0.8f);

            DrawModel(crustModel, Vector3{ 0, 0, 0 }, 1.0f, ApplyShopLighting(pos, Fade(WHITE, 0.95f)));

        rlPopMatrix();

    }



    // Flour dusting flecks on top (only close up)

    if (distSq < 9.0f || held) {

        for (int i = 0; i < 6; i++) {

            Vector3 fp = { pos.x + Frand(-0.14f, 0.14f), pos.y + 0.20f, pos.z + Frand(-0.08f, 0.08f) };

            DrawCube(fp, 0.015f, 0.005f, 0.015f, ApplyShopLighting(fp, Fade(WHITE, 0.8f)));

        }

    }

}



void DrawShoppingCartLocal(float wheelSpinDeg, Vector3 worldPos, float distSq)

{

    Color steel       = ApplyShopLighting(worldPos, Color{ 205, 210, 215, 255 });

    Color steelDark   = ApplyShopLighting(worldPos, Color{ 140, 145, 150, 255 });

    Color rubber      = ApplyShopLighting(worldPos, Color{ 35, 35, 38, 255 });

    Color seatPlastic = ApplyShopLighting(worldPos, Color{ 210, 60, 50, 255 });



    // basket is a TRAPEZOID: wider at the top than at the base

    float baseW = 0.42f, baseL = 0.62f;   // bottom footprint

    float topW  = 0.56f, topL  = 0.70f;   // top footprint (flares outward)

    float basketBottomY = 0.55f;          // height of basket floor off the ground

    float basketTopY    = 1.00f;



    Vector3 bottomCorners[4] = {

        { -baseW/2, basketBottomY, -baseL/2 },

        {  baseW/2, basketBottomY, -baseL/2 },

        {  baseW/2, basketBottomY,  baseL/2 },

        { -baseW/2, basketBottomY,  baseL/2 },

    };

    Vector3 topCorners[4] = {

        { -topW/2, basketTopY, -topL/2 },

        {  topW/2, basketTopY, -topL/2 },

        {  topW/2, basketTopY,  topL/2 },

        { -topW/2, basketTopY,  topL/2 },

    };



    // 4 slanted corner posts (bottom -> top, flaring outward)

    for (int i = 0; i < 4; i++)

        DrawCylinderEx(bottomCorners[i], topCorners[i], 0.010f, 0.010f, 8, steelDark);



    // horizontal bands at several heights, interpolated between the

    // bottom and top footprints so the whole basket tapers smoothly

    int bands = (distSq > 7.0f * 7.0f) ? 3 : 7;

    for (int b = 0; b <= bands; b++) {

        float t = (float)b / bands;

        Vector3 ring[4];

        for (int i = 0; i < 4; i++) ring[i] = Vector3Lerp(bottomCorners[i], topCorners[i], t);

        for (int i = 0; i < 4; i++)

            DrawCylinderEx(ring[i], ring[(i+1)%4], 0.005f, 0.005f, 6, steel);

    }

    // diagonal cross-bracing on the two long side walls (sub-pixel beyond 7m)

    if (distSq <= 7.0f * 7.0f) {

        int diag = 7;

        for (int i = 0; i <= diag; i++) {

            float t = (float)i / diag;

            Vector3 leftBot  = Vector3Lerp(bottomCorners[0], bottomCorners[3], t);

            Vector3 leftTop  = Vector3Lerp(topCorners[0],    topCorners[3],    t);

            Vector3 rightBot = Vector3Lerp(bottomCorners[1], bottomCorners[2], t);

            Vector3 rightTop = Vector3Lerp(topCorners[1],    topCorners[2],    t);

            DrawCylinderEx(leftBot,  leftTop,  0.0035f, 0.0035f, 6, steel);

            DrawCylinderEx(rightBot, rightTop, 0.0035f, 0.0035f, 6, steel);

        }

    }

    // basket floor grid

    int fx = (distSq > 7.0f * 7.0f) ? 2 : 6;

    int fz = (distSq > 7.0f * 7.0f) ? 2 : 8;

    for (int i = 0; i <= fx; i++) {

        float t = (float)i / fx;

        Vector3 a = Vector3Lerp(bottomCorners[0], bottomCorners[1], t);

        Vector3 bnd = Vector3Lerp(bottomCorners[3], bottomCorners[2], t);

        DrawCylinderEx(a, bnd, 0.004f, 0.004f, 6, steelDark);

    }

    for (int i = 0; i <= fz; i++) {

        float t = (float)i / fz;

        Vector3 a = Vector3Lerp(bottomCorners[0], bottomCorners[1], t);

        Vector3 bnd = Vector3Lerp(bottomCorners[3], bottomCorners[2], t);

        DrawCylinderEx(a, bnd, 0.004f, 0.004f, 6, steelDark);

    }



    // fold-down child seat flap at the back (angled little plastic seat)

    Vector3 seatHingeL = Vector3Lerp(topCorners[0], topCorners[1], 0.15f);

    Vector3 seatHingeR = Vector3Lerp(topCorners[0], topCorners[1], 0.85f);

    Vector3 seatFrontL = { seatHingeL.x, seatHingeL.y - 0.03f, seatHingeL.z - 0.16f };

    Vector3 seatFrontR = { seatHingeR.x, seatHingeR.y - 0.03f, seatHingeR.z - 0.16f };

    DrawCylinderEx(seatHingeL, seatFrontL, 0.006f, 0.006f, 6, steelDark);

    DrawCylinderEx(seatHingeR, seatFrontR, 0.006f, 0.006f, 6, steelDark);

    Vector3 seatCenter = { (seatFrontL.x + seatFrontR.x)/2, (seatFrontL.y+seatFrontR.y)/2 - 0.01f, (seatFrontL.z+seatFrontR.z)/2 };

    DrawCube(seatCenter, fabsf(seatFrontR.x-seatFrontL.x), 0.01f, 0.16f, seatPlastic);



    // angled leg frame from basket underside down to the wheel axle height

    float axleY = 0.14f;

    Vector3 legTargets[4] = {

        { -baseW*0.42f, axleY, -baseL*0.42f },

        {  baseW*0.42f, axleY, -baseL*0.42f },

        { -baseW*0.42f, axleY,  baseL*0.42f },

        {  baseW*0.42f, axleY,  baseL*0.42f },

    };

    for (int i = 0; i < 4; i++)

        DrawCylinderEx(bottomCorners[i], legTargets[i], 0.013f, 0.013f, 8, steelDark);

    // a low stabilizer bar tying the front and back leg pairs together

    DrawCylinderEx(legTargets[0], legTargets[1], 0.008f, 0.008f, 6, steelDark);

    DrawCylinderEx(legTargets[2], legTargets[3], 0.008f, 0.008f, 6, steelDark);



    // push handle: angled tube + rubber grip across the top-back

    Vector3 hB1 = Vector3Lerp(topCorners[0], topCorners[1], 0.05f);

    Vector3 hB2 = Vector3Lerp(topCorners[0], topCorners[1], 0.95f);

    Vector3 hT1 = { hB1.x, hB1.y + 0.16f, hB1.z - 0.14f };

    Vector3 hT2 = { hB2.x, hB2.y + 0.16f, hB2.z - 0.14f };

    DrawCylinderEx(hB1, hT1, 0.012f, 0.012f, 8, steelDark);

    DrawCylinderEx(hB2, hT2, 0.012f, 0.012f, 8, steelDark);

    DrawCylinderEx(hT1, hT2, 0.020f, 0.020f, 12, rubber); // grip



    // ---- wheels: 2 bigger fixed rear casters + 2 smaller front swivel casters ----

    float rearR = 0.06f, frontR = 0.045f;

    Vector3 rearAxleL  = { -baseW*0.42f, rearR,  baseL*0.42f };

    Vector3 rearAxleR  = {  baseW*0.42f, rearR,  baseL*0.42f };

    Vector3 frontAxleL = { -baseW*0.42f, frontR, -baseL*0.42f };

    Vector3 frontAxleR = {  baseW*0.42f, frontR, -baseL*0.42f };



    Color spokeCol = ApplyShopLighting(worldPos, LIGHTGRAY);

    auto drawWheel = [&](Vector3 axlePos, float radius, float spinDeg) {

        Vector3 forkTop = { axlePos.x, axlePos.y + 0.10f, axlePos.z };

        DrawCylinderEx(forkTop, axlePos, 0.008f, 0.008f, 6, steelDark);

        rlPushMatrix();

            rlTranslatef(axlePos.x, axlePos.y, axlePos.z);

            rlRotatef(spinDeg, 1, 0, 0); // rolling spin around the axle (local X)

            Vector3 axA = { -0.018f, 0, 0 }, axB = { 0.018f, 0, 0 };

            DrawCylinderEx(axA, axB, radius, radius, 18, rubber);

            DrawCylinderEx(axA, axB, radius * 1.02f, radius * 1.02f, 18, Fade(BLACK, 0.2f));

            DrawCylinderEx(Vector3{0,0,0}, Vector3{0, radius*0.9f, 0}, 0.004f, 0.004f, 6, spokeCol);

        rlPopMatrix();

    };

    drawWheel(rearAxleL,  rearR,  wheelSpinDeg);

    drawWheel(rearAxleR,  rearR,  wheelSpinDeg);

    drawWheel(frontAxleL, frontR, wheelSpinDeg);

    drawWheel(frontAxleR, frontR, wheelSpinDeg);



    // little plastic corner bumpers at the bottom corners

    Color bumperCol = ApplyShopLighting(worldPos, Color{60,60,65,255});

    for (int i = 0; i < 4; i++)

        DrawSphere(bottomCorners[i], 0.018f, bumperCol);

}



// Specialized Ethereal Renderer for Nocturnal Ghost Shopping Cart

void DrawGhostShoppingCartLocal(float wheelSpinDeg, Vector3 worldPos, float alpha, float timeVal)

{
    (void)wheelSpinDeg;
    (void)worldPos;

    float pulse = 0.82f + 0.18f * sinf(timeVal * 4.5f);

    Color ghostSteel     = Color{  80, 240, 215, (unsigned char)(195 * alpha * pulse) };

    Color ghostSteelDark = Color{  45, 175, 160, (unsigned char)(210 * alpha) };

    Color ghostGlow      = Color{ 130, 255, 235, (unsigned char)(245 * alpha * pulse) };

    Color ghostWheel     = Color{  90, 255, 230, (unsigned char)(255 * alpha) };



    float baseW = 0.42f, baseL = 0.62f;

    float topW  = 0.56f, topL  = 0.70f;

    float basketBottomY = 0.55f;

    float basketTopY    = 1.00f;



    Vector3 bottomCorners[4] = {

        Vector3{ -baseW/2, basketBottomY, -baseL/2 },

        Vector3{  baseW/2, basketBottomY, -baseL/2 },

        Vector3{  baseW/2, basketBottomY,  baseL/2 },

        Vector3{ -baseW/2, basketBottomY,  baseL/2 }

    };

    Vector3 topCorners[4] = {

        Vector3{ -topW/2, basketTopY, -topL/2 },

        Vector3{  topW/2, basketTopY, -topL/2 },

        Vector3{  topW/2, basketTopY,  topL/2 },

        Vector3{ -topW/2, basketTopY,  topL/2 }

    };



    // Main structural frame

    for (int i = 0; i < 4; i++) {

        DrawLine3D(topCorners[i], topCorners[(i+1)%4], ghostGlow);

        DrawLine3D(bottomCorners[i], bottomCorners[(i+1)%4], ghostSteelDark);

        DrawLine3D(bottomCorners[i], topCorners[i], ghostSteel);

    }



    // Grid wire bands

    for (int i = 1; i <= 4; i++) {

        float f = (float)i / 5.0f;

        float y = basketBottomY + (basketTopY - basketBottomY) * f;

        float w = baseW + (topW - baseW) * f;

        float l = baseL + (topL - baseL) * f;

        DrawCubeWires(Vector3{ 0.0f, y, 0.0f }, w, 0.01f, l, ghostSteel);

    }

    for (int k = -2; k <= 2; k++) {

        float x = (float)k * 0.09f;

        DrawLine3D(Vector3{ x, basketBottomY, -baseL/2 }, Vector3{ x, basketTopY, -topL/2 }, ghostSteelDark);

        DrawLine3D(Vector3{ x, basketBottomY,  baseL/2 }, Vector3{ x, basketTopY,  topL/2 }, ghostSteelDark);

    }



    // Handle bar

    Vector3 handleLeft  = Vector3{ -topW/2 * 1.06f, 1.05f, -topL/2 - 0.16f };

    Vector3 handleRight = Vector3{  topW/2 * 1.06f, 1.05f, -topL/2 - 0.16f };

    DrawCylinderEx(handleLeft, handleRight, 0.024f, 0.024f, 8, ghostGlow);

    DrawLine3D(topCorners[0], handleLeft, ghostSteel);

    DrawLine3D(topCorners[1], handleRight, ghostSteel);



    // Lower chassis tubular struts

    DrawCubeWires(Vector3{ 0.0f, 0.16f, 0.0f }, 0.44f, 0.12f, 0.66f, ghostSteelDark);



    // 4 Glowing Caster Wheels

    Vector3 wheelOffsets[4] = {

        Vector3{ -0.21f, 0.08f, -0.28f },

        Vector3{  0.21f, 0.08f, -0.28f },

        Vector3{ -0.21f, 0.08f,  0.28f },

        Vector3{  0.21f, 0.08f,  0.28f }

    };

    for (int i = 0; i < 4; i++) {

        DrawSphere(wheelOffsets[i], 0.052f, ghostWheel);

        DrawSphereWires(wheelOffsets[i], 0.054f, 6, 6, ghostGlow);

    }

}



#include "systems/receipt_printer.h"



void UpdateGhostCart(float dt, float nightFactor) {

    if (nightFactor < 0.25f) {

        g_ghostCart.active = false;

        g_ghostCart.alpha = 0.0f;

        g_ghostCart.waitTimer = 0.0f;

        return;

    }



    struct Waypoint {

        Vector3 pos;

        float waitTime;

        int targetItems;

    };

    // Visible path: Center aisle -> shelf pause -> walkway turn -> front of checkout counter and printer

    const Waypoint wps[5] = {

        { Vector3{  97.0f, 10.02f, 142.5f }, 1.2f, 0 }, // WP 0: Materializes right in central walkway in plain sight!

        { Vector3{  91.5f, 10.02f, 142.5f }, 2.0f, 1 }, // WP 1: Rolls down aisle to shelf, pauses & takes phantom item

        { Vector3{  97.0f, 10.02f, 137.0f }, 1.5f, 2 }, // WP 2: Turns and rolls toward checkout walkway

        { Vector3{ 104.5f, 10.02f, 135.0f }, 3.5f, 2 }, // WP 3: Pulls right up to counter & printer in front of player

        { Vector3{ 104.5f, 10.02f, 135.0f }, 2.0f, 2 }  // WP 4: Dissolves into floor with printer ringing up

    };



    if (!g_ghostCart.active) {

        g_ghostCart.waitTimer += dt;

        // Inactive cooldown between loops (only 3.5s so player quickly sees it repeat!)

        if (g_ghostCart.waitTimer >= 3.5f) {

            g_ghostCart.active = true;

            g_ghostCart.waypoint = 0;

            g_ghostCart.pos = wps[0].pos;

            g_ghostCart.yaw = 180.0f;

            g_ghostCart.alpha = 0.0f;

            g_ghostCart.waitTimer = 0.0f;

            g_ghostCart.itemsInCart = 0;

            g_ghostCart.rattleTimer = 0.0f;

            g_ghostCart.rattleIntensity = 0.0f;

        }

        return;

    }



    if (g_ghostCart.waypoint == 0) {

        // Materialize smoothly

        g_ghostCart.alpha = fminf(0.88f, g_ghostCart.alpha + dt * 1.4f);

        g_ghostCart.waitTimer += dt;

        if (g_ghostCart.waitTimer >= wps[0].waitTime) {

            g_ghostCart.waitTimer = 0.0f;

            g_ghostCart.waypoint = 1;

        }

        return;

    }

    // Hook D: Violent cart rattle jitter when phantom item is stolen
    if (g_ghostCart.rattleTimer > 0.0f) {
        g_ghostCart.rattleTimer -= dt;
        float rFrac = fmaxf(0.0f, g_ghostCart.rattleTimer / 2.5f);
        float rDist = rFrac * 0.045f;
        g_ghostCart.pos.x += (((float)rand() / (float)RAND_MAX) - 0.5f) * rDist;
        g_ghostCart.pos.z += (((float)rand() / (float)RAND_MAX) - 0.5f) * rDist;
        g_ghostCart.yaw   += (((float)rand() / (float)RAND_MAX) - 0.5f) * 26.0f * rFrac;
    }

    if (g_ghostCart.waypoint <= 3) {

        const Waypoint &curWP = wps[g_ghostCart.waypoint];

        float dx = curWP.pos.x - g_ghostCart.pos.x;

        float dz = curWP.pos.z - g_ghostCart.pos.z;

        float dist = sqrtf(dx * dx + dz * dz);



        if (dist > 0.15f) {

            float moveSpeed = 1.6f;

            float step = moveSpeed * dt;

            if (step > dist) step = dist;

            g_ghostCart.pos.x += (dx / dist) * step;

            g_ghostCart.pos.z += (dz / dist) * step;

            g_ghostCart.pos.y = 10.02f;



            float targetYaw = atan2f(dx, dz) * RAD2DEG;

            g_ghostCart.yaw = LerpAngleDeg(g_ghostCart.yaw, targetYaw, 6.0f * dt);

            g_ghostCart.wheelSpin += (step / 0.06f) * RAD2DEG;

        } else {

            g_ghostCart.waitTimer += dt;

            if (curWP.targetItems > g_ghostCart.itemsInCart && g_ghostCart.waitTimer > 0.8f) {

                g_ghostCart.itemsInCart = curWP.targetItems;

            }

            if (g_ghostCart.waypoint == 3 && g_ghostCart.waitTimer > 1.0f && g_printerState == PRINTER_IDLE) {

                g_printerState = PRINTER_PRINTING;

                g_printerProgress = 0.0f;

            }

            if (g_ghostCart.waitTimer >= curWP.waitTime) {

                g_ghostCart.waitTimer = 0.0f;

                g_ghostCart.waypoint++;

            }

        }

    } else {

        // Dematerialize into the floor

        g_ghostCart.alpha = fmaxf(0.0f, g_ghostCart.alpha - dt * 0.6f);

        if (g_ghostCart.alpha <= 0.01f) {

            g_ghostCart.active = false;

            g_ghostCart.waitTimer = 0.0f;

        }

    }

}



void UpdateShopParticle(ShopParticle &p, float dt, float floorY)

{

    if (p.landed) {

        p.settleTimer -= dt;

        return;

    }

    const float gravity = 9.8f;

    p.vel.y -= gravity * dt;

    p.pos.x += p.vel.x * dt;

    p.pos.y += p.vel.y * dt;

    p.pos.z += p.vel.z * dt;



    if (p.pos.y - p.radius <= floorY) {

        p.pos.y = floorY + p.radius;

        p.vel.x *= 0.35f;

        p.vel.z *= 0.35f;

        p.vel.y *= -0.20f;

        if (fabsf(p.vel.y) < 0.6f) {

            p.vel = Vector3{ 0, 0, 0 };

            p.landed = true;

            p.settleTimer = 1.8f; // Settled particles smoothly fade out after 1.8s

        }

    }

}



void DrawShopParticle(const ShopParticle &p)

{

    Color litCol = ApplyShopLighting(p.pos, p.color);

    if (p.landed && p.settleTimer < 1.0f) {

        litCol.a = (unsigned char)(litCol.a * Clamp(p.settleTimer, 0.0f, 1.0f));

    }

    if (litCol.a < 4) return;

    if (p.isCube) {

        DrawCube(p.pos, p.radius * 1.6f, p.radius * 1.2f, p.radius * 1.6f, litCol);

    } else {

        // High-performance low-poly kernel/drop cube without CPU sphere triangulation

        DrawCube(p.pos, p.radius * 1.5f, p.radius * 1.5f, p.radius * 1.5f, litCol);

    }

}
