#include <cstdio>

#include <functional>

#include <raylib.h>

#include <raymath.h>

#include <rlgl.h>

#include <vector>

#include <stdint.h>

#include <math.h>

#include <stdlib.h>

#include <cstring>

#include "core/game_types.h"
#include "core/engine_systems.h"
#include "core/game_context.h"
#include "systems/intro_cinematic.h"
#include "systems/phone_system.h"




// ============================================================

// WHAT THE GROUND KEEPS - 3D ASCII ENGINE

// Features: Voxel Grid, Physics Debris, Foliage Sway, Skybox & Fog

// ============================================================










































// --- KINEMATIC HORROR CAMERA & VIEWMODEL INERTIA CONTROLLER ---




// --- DYNAMIC FLASHLIGHT & WORLD LIGHTING CONTEXT ---



// --- FUNCTIONAL GAS STATION & FUEL SELLING ECONOMY (State in hud_manager.h/.cpp)
// 3D In-World Fuel Pump CRT Monitor Texture


#include "systems/gas_station_system.h"

static inline void SpawnCustomerCar() {
    SpawnCustomerCar(g_customerCar, g_fuelPricePerGallon);
}









// --- DISTINCT STRONG HEADING & HIGH-LEGIBILITY MENU FONTS FROM ASSETS ---


#include "systems/ui_helpers.h"

// =========================================================================
// AAA POLISHED UI PRIMITIVES & DESIGN SYSTEM
// =========================================================================




// (LerpAngleDeg and Frand inline in procedural_math.inl)



#include "systems/texture_factories.h"



// (InitProceduralShopAssets extracted to systems/procedural_shop_assets.h/.cpp)




// (Shop Lighting Context & Physical Multi-Light Engine extracted to systems/shop_lighting.h/.cpp)



// (Skeleton Cow NPC System extracted to systems/skeleton_cow_npc.h/.cpp)




// (Hound NPC System extracted to systems/hound_npc.h/.cpp)






// (Atmospheric Volumetric Dust & Footstep Trails System extracted to systems/atmospheric_particles.h/.cpp)



// (Hyper-Realistic Shovel & Digging System extracted to systems/shovel_system.h/.cpp)



#include "systems/footprint_decals.h"
// (Abandoned Blackwood College System extracted to systems/blackwood_college.h/.cpp)







// (Crashed Sedan Site extracted to systems/crashed_sedan_site.h/.cpp)

// -----------------------------------------------------------------------------
// IN-WORLD 3D FUEL PUMP CRT & CATENARY HOSE RENDERING SYSTEM
// (Implementation extracted to systems/gas_station_system.h/.cpp)
// -----------------------------------------------------------------------------

static inline void UpdatePumpCrtTexture(int pumpNum, float gallons, float salePrice, bool isFlowing, float flk) {
    UpdatePumpCrtTextureEx(g_pumpScreenRT, g_pumpScreenRTLoaded, g_fontSmall, g_fontTitle, pumpNum, gallons, salePrice, isFlowing, g_fuelPricePerGallon, g_stationFuelGallons, flk);
}

// (DrawCatenaryHose & DrawFirstPersonFuelNozzle extracted to systems/gas_station_system.h/.cpp)

#include "systems/ocean_system.h"
#include "systems/shop_atmosphere.h"


int main(int argc, char** argv) {
    IntroCinematic introCinematic;
    PhoneSystem phoneSystem;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--test") == 0 && i + 1 < argc) g_testFrames = atoi(argv[i + 1]);
        if (strcmp(argv[i], "--timeofday") == 0 && i + 1 < argc) g_pinnedTimeOfDay = (float)atof(argv[i + 1]);
        if (strcmp(argv[i], "--cloud") == 0 && i + 1 < argc) g_pinnedCoverage = (float)atof(argv[i + 1]);
        if (strcmp(argv[i], "--cloudoffset") == 0 && i + 1 < argc) g_pinnedCloudOffset = (float)atof(argv[i + 1]);
        if (strcmp(argv[i], "--screenshot") == 0 && i + 1 < argc) g_testScreenshot = argv[i + 1];
        if (strcmp(argv[i], "--lookatsun") == 0) g_lookAtSun = true;
        if (strcmp(argv[i], "--lookatground") == 0) g_lookAtGround = true;
        if (strcmp(argv[i], "--lookatshop") == 0) g_lookAtShop = true;
        if (strcmp(argv[i], "--lookatwashroom") == 0) g_lookAtWashroom = true;
        if (strcmp(argv[i], "--lookatatm") == 0) g_lookAtAtm = true;
    }

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);

    InitWindow(LOGICAL_W, LOGICAL_H, "WHAT THE GROUND KEEPS - 3D ASCII ENGINE");

    // Set runtime window icon from WhatTheGroundKeeps_LogoDesign.jpg
    if (FileExists("assets/images/WhatTheGroundKeeps_LogoDesign.jpg")) {
        Image winIcon = LoadImage("assets/images/WhatTheGroundKeeps_LogoDesign.jpg");
        if (winIcon.data != NULL) {
            SetWindowIcon(winIcon);
            UnloadImage(winIcon);
        }
    }

    int startMon = GetCurrentMonitor();

    SetWindowPosition((GetMonitorWidth(startMon) - LOGICAL_W) / 2, (GetMonitorHeight(startMon) - LOGICAL_H) / 2);

    SetWindowMinSize(960, 540);

    SetExitKey(KEY_NULL); // Prevent default ESC window closing so we can handle skip intro & confirmation dialog

    InitProceduralShopAssets();

    InitCollegeShaderAndTextures();

    InitOceanSystem();

    InitShopAtmosphere();
    InitATMSystem();

    Shovel g_shovelRig = BuildShovel();



    // Load Distinct Strong Heading & High-Legibility Menu Fonts from assets/
    auto resolveFont = [](const char* name) -> const char* {
        static char paths[5][256];
        static int pathIdx = 0;
        char* buf = paths[pathIdx++ % 5];
        snprintf(buf, 256, "assets/fonts/%s", name);
        if (FileExists(buf)) return buf;
        snprintf(buf, 256, "assets/%s", name);
        return buf;
    };

    g_fontTitle   = LoadFontEx(resolveFont("alagard.ttf"), 52, NULL, 0);

    g_fontHeadSub = LoadFontEx(resolveFont("alagard.ttf"), 30, NULL, 0);

    g_fontMenu    = LoadFontEx(resolveFont("IBMPlexMono-Bold.ttf"), 28, NULL, 0);

    g_fontBody    = LoadFontEx(resolveFont("IBMPlexMono-Medium.ttf"), 20, NULL, 0);

    g_fontSmall   = LoadFontEx(resolveFont("IBMPlexMono-Regular.ttf"), 16, NULL, 0);



    if (g_fontTitle.texture.id == 0)   g_fontTitle   = GetFontDefault();

    if (g_fontHeadSub.texture.id == 0) g_fontHeadSub = GetFontDefault();

    if (g_fontMenu.texture.id == 0)    g_fontMenu    = GetFontDefault();

    if (g_fontBody.texture.id == 0)    g_fontBody    = GetFontDefault();

    if (g_fontSmall.texture.id == 0)   g_fontSmall   = GetFontDefault();



    SetTextureFilter(g_fontTitle.texture, TEXTURE_FILTER_BILINEAR);

    SetTextureFilter(g_fontHeadSub.texture, TEXTURE_FILTER_BILINEAR);

    SetTextureFilter(g_fontMenu.texture, TEXTURE_FILTER_BILINEAR);

    SetTextureFilter(g_fontBody.texture, TEXTURE_FILTER_BILINEAR);

    SetTextureFilter(g_fontSmall.texture, TEXTURE_FILTER_BILINEAR);



    auto drawLoadingBar = [&](float progress, const char* statusText) {

        BeginDrawing();

        ClearBackground(Color{ 6, 7, 9, 255 });



        int scrW = GetScreenWidth();

        int scrH = GetScreenHeight();

        if (scrW < 960) scrW = LOGICAL_W;

        if (scrH < 540) scrH = LOGICAL_H;



        // Cinematic deep horror vignette framing the screen

        int vH = scrH / 3;

        int vW = scrW / 4;

        DrawRectangleGradientV(0, 0, scrW, vH, Color{ 0, 0, 0, 245 }, BLANK);

        DrawRectangleGradientV(0, scrH - vH, scrW, vH, BLANK, Color{ 0, 0, 0, 255 });

        DrawRectangleGradientH(0, 0, vW, scrH, Color{ 0, 0, 0, 235 }, BLANK);

        DrawRectangleGradientH(scrW - vW, 0, vW, scrH, BLANK, Color{ 0, 0, 0, 235 });



        // Feather-light CRT scanlines for analog found-footage texture

        for (int y = 0; y < scrH; y += 4) {

            DrawLine(0, y, scrW, y, Color{ 0, 0, 0, 28 });

        }



        float timeSec = (float)GetTime();



        // Flashing surveillance recording indicator at top left

        bool recBlink = (fmodf(timeSec * 2.0f, 1.0f) < 0.65f);

        if (recBlink) {

            DrawCircle(45, 38, 5.0f, Color{ 225, 35, 30, 255 });

            DrawTextSharp(g_fontBody, "REC [DISPATCH EVIDENCE REEL #09]", 58, 28, 17.0f, Color{ 235, 55, 45, 240 });

        } else {

            DrawTextSharp(g_fontBody, "    [DISPATCH EVIDENCE REEL #09]", 58, 28, 17.0f, Color{ 140, 40, 35, 180 });

        }

        DrawTextSharp(g_fontSmall, "STATE POLICE CRIME LAB // UNRESOLVED CASE ARCHIVE", 45, 54, 14.0f, Color{ 140, 135, 130, 200 });





        // Forensic watermark stamp at top right

        DrawTextSharp(g_fontSmall, "DECLASSIFIED // EVIDENCE VAULT B-9", scrW - 320, 28, 14.0f, Color{ 170, 50, 45, 210 });

        DrawTextSharp(g_fontSmall, "SEC LOG: 142.85 MHz [CALIBRATED]", scrW - 320, 48, 13.0f, Color{ 120, 115, 110, 170 });



        // Ominous cryptic case header (Strong Gothic Heading Font)

        const char* titleHead = "WHAT THE GROUND KEEPS";

        DrawTextSharpCentered(g_fontTitle, titleHead, scrW/2 + 3, scrH/2 - 118, 46.0f, Color{ 120, 18, 14, 210 }, 3.0f);

        DrawTextSharpCentered(g_fontTitle, titleHead, scrW/2, scrH/2 - 120, 46.0f, Color{ 245, 240, 230, 255 }, 3.0f);



        // Cryptic warning transcript

        const char* quote = "\"Whatever falls into the Route 9 mire... does not decay.\"";

        DrawTextSharpCentered(g_fontBody, quote, scrW/2, scrH/2 - 62, 18.0f, Color{ 190, 180, 170, 230 });



        // Sleek blood-ember forensic gauge track

        int barW = (int)(scrW * 0.44f);

        if (barW < 400) barW = 400;

        if (barW > 600) barW = 600;

        int barH = 6;

        int bx = scrW / 2 - barW / 2;

        int by = scrH / 2 - 10;



        // Forensic scale tick marks

        DrawLine(bx, by - 8, bx, by + barH + 8, Color{ 90, 85, 80, 200 });

        DrawLine(bx + barW/4, by - 4, bx + barW/4, by + barH + 4, Color{ 60, 55, 50, 150 });

        DrawLine(bx + barW/2, by - 6, bx + barW/2, by + barH + 6, Color{ 90, 85, 80, 200 });

        DrawLine(bx + barW*3/4, by - 4, bx + barW*3/4, by + barH + 4, Color{ 60, 55, 50, 150 });

        DrawLine(bx + barW, by - 8, bx + barW, by + barH + 8, Color{ 90, 85, 80, 200 });



        // Dark track background

        DrawRectangle(bx, by, barW, barH, Color{ 14, 16, 18, 255 });

        DrawRectangleLines(bx - 1, by - 1, barW + 2, barH + 2, Color{ 45, 30, 28, 240 });



        // Glowing blood-ember fill

        int fillW = (int)(Clamp(progress, 0.0f, 1.0f) * barW);

        if (fillW > 0) {

            DrawRectangle(bx, by, fillW, barH, Color{ 200, 35, 25, 255 });

            DrawRectangle(bx + fillW - 3, by - 2, 4, barH + 4, Color{ 255, 90, 70, 255 });

            DrawCircle(bx + fillW, by + barH/2, 5.0f, Color{ 255, 120, 80, 110 });

        }



        // Active forensic step description

        DrawTextSharpCentered(g_fontBody, statusText, scrW / 2, by + 24, 18.0f, Color{ 215, 210, 200, 250 });



        // Percentage indicator

        const char* pct = TextFormat("%02d%%", (int)(progress * 100.0f));

        DrawTextSharp(g_fontMenu, pct, bx + barW + 18, by - 11, 22.0f, Color{ 235, 55, 45, 255 });



        // Telemetry readout

        DrawTextSharp(g_fontSmall, "ARCHIVE COORD: 44.9184° N, 71.3820° W", bx, by + 56, 14.0f, Color{ 130, 125, 120, 190 });

        const char* incTag = "INCIDENT FILE: #09-B // EVIDENCE LOGGED";

        float incW = MeasureTextSharp(g_fontSmall, incTag, 14.0f);

        DrawTextSharp(g_fontSmall, incTag, bx + barW - incW, by + 56, 14.0f, Color{ 130, 125, 120, 190 });



        EndDrawing();

    };



    // Immediately draw frame to eliminate the Windows white screen

    drawLoadingBar(0.05f, "DECRYPTING POLICE DISPATCH FREQUENCIES...");



    InitAudioDevice();

    drawLoadingBar(0.12f, "CALIBRATING LOW-LIGHT PHOTOMETRICS...");

    

    Sound sndFootstep = GenerateFootstepSound();

    Sound sndCrickets = GenerateCricketAmbience();

    Sound sndWind = GenerateWindAmbience();

    Sound sndSpark = GenerateElectricSparkSound();

    Sound sndJumpscare = GenerateJumpscareSound();

    g_sndGunshot = GenerateGunshotSound();

    g_sndStoreFootstep = GenerateStoreFootstepSound();

    g_sndFoil = GenerateFoilSound();

    g_sndMenuNav = GenerateMenuNavSound();

    g_sndMenuBoom = GenerateMenuBoomSound();

    g_sndRadioStatic = GenerateRadioStaticSound();

    g_sndWaterDrip   = GenerateWaterDripSound();

    g_sndChestOpen   = GenerateChestOpenSound();

    g_sndShovelDig  = GenerateShovelDigSound();

    g_sndPhoneSlide  = GeneratePhoneSlideSound();

    g_sndPhoneTap    = GeneratePhoneTapSound();

    g_sndFlashlightToggle = GenerateFlashlightToggleSound();
    g_sndCashRegister     = GenerateCashRegisterSound();
    g_sndDrivewayBell     = GenerateDrivewayBellSound();
    g_sndPumpFlow         = GeneratePumpFlowSound();
    g_sndNozzleLatch      = GenerateNozzleLatchSound();
    g_sndNozzleShutoff    = GenerateNozzleShutoffSound();
    g_sndStepperMotor     = GenerateStepperMotorSound();

    SetSoundVolume(g_sndNozzleLatch, 0.70f);
    SetSoundVolume(g_sndNozzleShutoff, 0.85f);
    SetSoundVolume(g_sndStepperMotor, 0.60f);

    g_pumpScreenRT        = LoadRenderTexture(320, 240);
    g_pumpScreenRTLoaded  = true;

    SetSoundVolume(g_sndFlashlightToggle, 0.60f);

    g_sndLightSwitch      = GenerateLightSwitchSound();

    SetSoundVolume(g_sndLightSwitch, 0.75f);

    SetSoundVolume(g_sndPhoneSlide, 0.65f);

    SetSoundVolume(g_sndPhoneTap, 0.50f);

    SetSoundVolume(g_sndMenuNav, 0.40f);

    SetSoundVolume(g_sndMenuBoom, 0.70f);

    SetSoundVolume(g_sndRadioStatic, 0.45f);

    SetSoundVolume(g_sndWaterDrip, 0.65f);

    SetSoundVolume(g_sndChestOpen, 0.75f);

    SetSoundVolume(g_sndShovelDig, 0.85f);

    

    // Ensure rock-solid minimum 144 FPS up to monitor's native high-refresh rate

    int monitorHz = GetMonitorRefreshRate(GetCurrentMonitor());

    int targetHz = (monitorHz > 144) ? monitorHz : 144;

    SetTargetFPS(targetHz);

    

    Camera3D camera = { 0 };

    camera.position = Vector3{ CHUNK_W/2.0f, 11.65f, CHUNK_D/2.0f };

    camera.target = Vector3{ CHUNK_W/2.0f, 12.2f, CHUNK_D/2.0f + 1.0f };

    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };

    camera.fovy = 60.0f;

    camera.projection = CAMERA_PERSPECTIVE;

    

    Shader instancedShader = LoadShaderFromMemory(instancedVS, instancedFS);

    instancedShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(instancedShader, "mvp");

    instancedShader.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(instancedShader, "matView");

    instancedShader.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(instancedShader, "matProjection");

    instancedShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocationAttrib(instancedShader, "instanceTransform");
    if (instancedShader.locs[SHADER_LOC_MATRIX_MODEL] == -1) instancedShader.locs[SHADER_LOC_MATRIX_MODEL] = 4;
    

    int uvOffsetLoc = GetShaderLocation(instancedShader, "uvOffset");

    int uvScaleLoc = GetShaderLocation(instancedShader, "uvScale");

    int timeLoc = GetShaderLocation(instancedShader, "time");
    int playerPosLoc = GetShaderLocation(instancedShader, "playerPos");

    int trailPosLoc = GetShaderLocation(instancedShader, "trailPos");

    int trailLifeLoc = GetShaderLocation(instancedShader, "trailLife");

    int lightningFlashLoc = GetShaderLocation(instancedShader, "lightningFlash");

    int sunDirLoc = GetShaderLocation(instancedShader, "sunDir");

    int dayFactorLoc = GetShaderLocation(instancedShader, "dayFactor");

    int sunColorLoc = GetShaderLocation(instancedShader, "sunColor");

    int moonDirLoc = GetShaderLocation(instancedShader, "moonDir");

    int nightFactorLoc = GetShaderLocation(instancedShader, "nightFactor");

    

    Vector3 initSunDir = { 0.0f, 1.0f, 0.0f };

    float initDayFactor = 1.0f;

    Vector3 initSunColor = { 1.0f, 0.98f, 0.90f };

    Vector3 initMoonDir = { 0.0f, -1.0f, 0.0f };

    float initNightFactor = 0.0f;

    SetShaderValue(instancedShader, sunDirLoc, &initSunDir, SHADER_UNIFORM_VEC3);

    SetShaderValue(instancedShader, dayFactorLoc, &initDayFactor, SHADER_UNIFORM_FLOAT);

    SetShaderValue(instancedShader, sunColorLoc, &initSunColor, SHADER_UNIFORM_VEC3);

    SetShaderValue(instancedShader, moonDirLoc, &initMoonDir, SHADER_UNIFORM_VEC3);

    SetShaderValue(instancedShader, nightFactorLoc, &initNightFactor, SHADER_UNIFORM_FLOAT);

    

    drawLoadingBar(0.22f, "INDEXING CRIME SCENE TEXTURE BUFFERS...");

    Texture2D atlas = CreateGlyphAtlas();

    drawLoadingBar(0.26f, "CALIBRATING ATMOSPHERIC STORM DYNAMICS...");

    InitCloudSystem();

    drawLoadingBar(0.28f, "INITIALIZING WITNESS SHADERS & FAUNA...");

    InitBovineNPCs();

    InitHoundResources();

    drawLoadingBar(0.32f, "LINKING THERMAL PRINTER TELEMETRY...");

    g_receiptTex = BuildReceiptTexture();

    SetTextureFilter(g_receiptTex, TEXTURE_FILTER_BILINEAR);

    

    int initialRenderW = GetScreenWidth();

    int initialRenderH = GetScreenHeight();

    if (initialRenderW < LOGICAL_W) initialRenderW = LOGICAL_W;

    if (initialRenderH < LOGICAL_H) initialRenderH = LOGICAL_H;

    RenderTexture2D target = LoadRenderTexture(initialRenderW, initialRenderH);

    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT); // Pixel-perfect sharp rendering

    SetTextureWrap(target.texture, TEXTURE_WRAP_CLAMP);

    

    Material material = LoadMaterialDefault();

    material.shader = instancedShader;

    material.maps[MATERIAL_MAP_ALBEDO].texture = atlas;

    

    Mesh quad = GenQuadMesh();

    drawLoadingBar(0.35f, "TOPOGRAPHIC SCAN OF ROUTE 9 CORRIDOR...");

    Chunk* chunk = new Chunk();

    

    // Generate terrain with central flat plains and distant hills

    for(int x=0; x<CHUNK_W; x++) {

        for(int z=0; z<CHUNK_D; z++) {

            float h = fbm(x * 0.02f, z * 0.02f);

            float shaped = h * h * h * 3.0f; 

            

            // Force the center area to be completely flat for the main gameplay zone

            float dx = (float)x - (CHUNK_W / 2.0f);

            float dz = (float)z - (CHUNK_D / 2.0f);

            float dist = sqrtf(dx*dx + dz*dz);

            

            float flattenFactor = 1.0f;

            if (dist < 78.0f) {

                flattenFactor = 0.0f; // 100% flat across the entire shop lot, alley, and surrounding perimeter

            } else if (dist < 110.0f) {

                flattenFactor = (dist - 78.0f) / 32.0f; 

                flattenFactor = flattenFactor * flattenFactor * (3.0f - 2.0f * flattenFactor); // smoothstep

            }

            // Cut flat dual-lane road path (width 26 units centered at X=128)

            float roadDist = fabs(dx);

            if (roadDist < 13.0f) {

                flattenFactor = 0.0f;

            } else if (roadDist < 20.0f) {

                float rf = (roadDist - 13.0f) / 7.0f;

                rf = rf * rf * (3.0f - 2.0f * rf);

                if (rf < flattenFactor) flattenFactor = rf;

            }

            

            shaped *= flattenFactor;

            

            // Base level is Y=10, hills rise up to Y=28

            int surfaceY = 10 + (int)(shaped * 18.0f); 

            

            // Road surface is flush at Y=10 across both lanes (width 26 units)

            bool isRoad = (fabs(dx) < 13.0f);

            if (isRoad) surfaceY = 10;

            // Western Ocean Coastline & Seabed Sculpting (X <= 42)
            if (x <= 42) {
                if (x >= 32) {
                    // Coastal slope down from pine woods to pebble beach (Y=9.8)
                    float coastF = (float)(x - 32) / 10.0f;
                    int coastY = 8 + (int)(coastF * coastF * (3.0f - 2.0f * coastF) * 4.0f);
                    if (coastY < surfaceY) surfaceY = coastY;
                } else {
                    // Submerged ocean seabed (Y=4..7)
                    float deepF = (float)x / 32.0f;
                    int bedY = 4 + (int)(deepF * 3.5f);
                    if (bedY < surfaceY) surfaceY = bedY;
                }
            }

            

            if (surfaceY > CHUNK_H - 1) surfaceY = CHUNK_H - 1;

            

            for(int y=0; y<CHUNK_H; y++) {

                Voxel& v = chunk->voxels[x][y][z];

                if (y < surfaceY) {

                    v.isSolid = true;

                    v.glyphIndex = (x <= 34) ? 0 : '#';

                    v.fgColor = Color{10, 7, 5, 255}; 

                } else if (y == surfaceY) {

                    v.isSolid = true;

                    if (x <= 34) {

                        v.glyphIndex = 0; // ZERO ASCII characters in ocean seabed (smooth floor mesh rendered instead)

                    } else if (isRoad) {

                        v.glyphIndex = '#';

                        v.fgColor = Color{20, 20, 22, 255}; 

                    } else if (x <= 42) {

                        v.glyphIndex = ',';

                        v.fgColor = Color{ 48, 46, 42, 255 }; // Coastal pebble beach

                    } else {

                        v.glyphIndex = (GetRandomValue(0, 1) == 0) ? '|' : '/';

                        v.fgColor = Color{50, 200, 50, 255}; 

                    }

                } else {

                    v.isSolid = false;

                    v.glyphIndex = 0;

                }

            }

        }

    }



    drawLoadingBar(0.48f, "GROWING DENSE SWAMP SPRUCE FORESTS...");

    // Generate trees across the larger map (Dense Swamp Spruce Forests across West & East zones)

    for(int x = 8; x < CHUNK_W - 8; x += GetRandomValue(14, 22)) {

        for(int z = 8; z < CHUNK_D - 8; z += GetRandomValue(14, 22)) {

        

        float dx = (float)x - (CHUNK_W / 2.0f);

        float dz = (float)z - (CHUNK_D / 2.0f);

        if (sqrtf(dx*dx + dz*dz) < 15.0f) continue;

        if (fabs(dx) < 14.0f) continue; // Keep dual-lane road and central station clear of trees

        if (x >= 75 && x <= 118 && z >= 118 && z <= 170) continue; // Keep trees off shop lot

        if (x >= 148 && x <= 195 && z >= 118 && z <= 168) continue; // Keep trees off abandoned college lot

        if (x <= 40) continue; // Keep trees off western ocean and beach!



        // Find the surface height for this X/Z

        int surfaceY = 0;

        for(int y = CHUNK_H - 1; y >= 0; y--) {

            if (chunk->voxels[x][y][z].isSolid) {

                surfaceY = y;

                break;

            }

        }

        

        int trunkHeight = GetRandomValue(4, 7);

        for(int ty = 0; ty < trunkHeight; ty++) {

            if (surfaceY + 1 + ty >= CHUNK_H) break;

            

            // 3D CIRCULAR TREE TRUNK: 

            // Built using a solid cross pattern of 5 '|' characters (Center, N, S, E, W)

            int tx[5] = {x, x+1, x-1, x, x};

            int tz[5] = {z, z, z, z+1, z-1};

            

            for(int i=0; i<5; i++) {

                int vx = tx[i], vz = tz[i];

                if (vx >= 0 && vx < CHUNK_W && vz >= 0 && vz < CHUNK_D) {

                    Voxel& v = chunk->voxels[vx][surfaceY + 1 + ty][vz];

                    v.isSolid = true;

                    v.glyphIndex = '|';

                    // Slightly darken outer edges for depth

                    v.fgColor = (i == 0) ? Color{80, 50, 30, 255} : Color{60, 35, 20, 255}; 

                }

            }

        }

        

        int cy = surfaceY + 1 + trunkHeight;

        int radius = GetRandomValue(3, 5);

        for(int dx = -radius; dx <= radius; dx++) {

            for(int dy = -radius; dy <= radius; dy++) {

                for(int dz = -radius; dz <= radius; dz++) {

                    if (dx*dx + dy*dy + dz*dz <= radius*radius) {

                        int vx = x + dx;

                        int vy = cy + dy;

                        int vz = z + dz;

                        if (vx >= 0 && vx < CHUNK_W && vy >= 0 && vy < CHUNK_H && vz >= 0 && vz < CHUNK_D) {

                            if (dx == 0 && dz == 0 && dy < 0) continue; 

                            Voxel& v = chunk->voxels[vx][vy][vz];

                            v.isSolid = true;

                            v.glyphIndex = '&';

                            v.fgColor = Color{30, (unsigned char)GetRandomValue(150, 220), 50, 255};

                        }

                    }

                }

            }

        }

        }

    }

    

// -------------------------------------------------------------

    // GENERATE THE THICKET (Rich Green Blades with Small Bone-White Flower Tips)

    // -------------------------------------------------------------

    const char grassGlyphs[] = { '\'', ',', '.', '"', ';', '`', 'v', 'w' };

    for(int x = 0; x < CHUNK_W; x++) {

        for(int z = 0; z < CHUNK_D; z++) {

            float dx = (float)x - (CHUNK_W / 2.0f);

            if (fabs(dx) < 13.5f) continue; // Keep grass off dual-lane asphalt road

            if (x >= 75 && x <= 116 && z >= 118 && z <= 170) continue; // Keep grass off shop building & interior

            if (x >= 126 && x <= 130 && z >= 134 && z <= 146) continue; // Keep grass off gas station island curb

            if (x >= 148 && x <= 195 && z >= 118 && z <= 168) continue; // Keep grass off abandoned college lot

            if (x <= 40) continue; // Keep grass and flower stems off western ocean!

            

            int surfaceY = 0;

            for(int y = CHUNK_H - 1; y >= 0; y--) {

                if (chunk->voxels[x][y][z].isSolid) {

                    surfaceY = y;

                    break;

                }

            }

            

            if (surfaceY > 0 && surfaceY < CHUNK_H - 2) {

                char floorG = chunk->voxels[x][surfaceY][z].glyphIndex;

                if (floorG == '|' || floorG == '/' || floorG == '#') {

                    // High density for a thick, carpeted field look

                    if (GetRandomValue(0, 100) < 95) {

                        // 15% chance to be a tall flower stem (2 blocks high).

                        // Otherwise, it just adds 1 block of extra green grass to thicken the field.

                        bool isTallFlower = (GetRandomValue(0, 100) < 15);

                        int h = isTallFlower ? 2 : 1; 



                        for(int gy = 1; gy <= h; gy++) {

                            Voxel& v = chunk->voxels[x][surfaceY + gy][z];

                            if (v.isSolid || v.glyphIndex != 0) break; 

                            

                            v.isSolid = false;

                            

                            // Check if this is the top-most part of the tall strand (the flower/seed head)

                            if (isTallFlower && gy == h) {

                                // Smaller, delicate font-like characters for flower heads ('\'' , '.' , ',')

                                const char flowerGlyphs[] = { '.', ',', '\'' };

                                v.glyphIndex = flowerGlyphs[GetRandomValue(0, 2)];

                                

                                // Pinkish Tip Color

                                int r = GetRandomValue(220, 255);

                                int g = GetRandomValue(140, 180);

                                int b = GetRandomValue(180, 210);

                                v.fgColor = Color{(unsigned char)r, (unsigned char)g, (unsigned char)b, 254};

                            } else {

                                // Lower blade body (or normal 1-block extra grass): vibrant green

                                v.glyphIndex = grassGlyphs[GetRandomValue(0, 7)];

                                int r = GetRandomValue(30, 70);

                                int g = GetRandomValue(160, 220);

                                int b = GetRandomValue(30, 70);

                                v.fgColor = Color{(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};

                            }

                        }

                    }

                }

            }

        }

    }



    // Stamp central Gas Station in the middle of road dividing into two equal lanes,

    // and stamp the Shop on the west bank of the road in front of it!

    BuildGasStation(chunk);

    BuildShop(chunk);

    BuildCollege(chunk);



    drawLoadingBar(0.55f, "BUILDING VOXEL MESHES...");

    chunk->BuildMesh([&](float p) {

        drawLoadingBar(0.55f + p * 0.38f, "BUILDING VOXEL MESHES...");

    });





    

    // Generate Stars (Reduced count + no glow for FPS)

    std::vector<Star> stars;

    for(int i=0; i<180; i++) {  // Reduced from 400 to 180

        float theta = GetRandomValue(0, 360) * DEG2RAD;

        float phi = GetRandomValue(15, 80) * DEG2RAD;

        float r = 400.0f;

        Star s;

        s.basePos.x = r * cosf(phi) * cosf(theta);

        s.basePos.y = r * sinf(phi);

        s.basePos.z = r * cosf(phi) * sinf(theta);

        s.phase = (float)GetRandomValue(0, 100) / 10.0f;

        s.isBig = (GetRandomValue(0, 5) == 0);  // Fewer big stars

        stars.push_back(s);

    }

    

    std::vector<PhysicsParticle> debris;

    debris.reserve(256);

    std::vector<Matrix> debrisInstances[256];

    std::vector<Matrix> starInstances[256];

    std::vector<Matrix> moonInstances[256];

    std::vector<Matrix> cloudInstances[256];

    float dayCycleTime = 45.0f;         // Start at bright morning

    float dayCycleDuration = 240.0f;    // 4 minutes per full celestial orbit

    bool  dayCyclePaused = false;

    if (g_pinnedCoverage >= 0.0f) {
        SetCloudCoverage(g_pinnedCoverage);
    }
    SetCloudOffset(g_pinnedCloudOffset);
    if (g_pinnedTimeOfDay >= 0.0f) {
        dayCycleTime = g_pinnedTimeOfDay * dayCycleDuration;
        dayCyclePaused = true;
    }

    

    float swingTimer = 0.0f;

    float walkTime = 0.0f;

    float bobAmplitude = 0.0f;

    float digShake = 0.0f;

    float hitStopTimer = 0.0f;

    bool isThirdPerson = false;

    Camera3D renderCam = camera;

    Vector3 playerVel = {0.0f, 0.0f, 0.0f};



    // Gas Station, Shopkeeper & Front Roof Wall CCTV State (defined in cctv_surveillance.cpp)
    float doorSlideProgress  = 0.0f; // 0.0f = closed, 1.0f = fully open

    float doorSlideVel       = 0.0f; // critically damped spring velocity

    float doorHoldTimer      = 0.0f; // timer keeping door open after sensor clears

    bool  doorSensorActive   = false;

    float fluorFlickerTimer = 0.0f;

    float fluorNextFlicker  = 2.5f;

    bool fluorLightOn       = true;



    // --- HANGING MEAT & HYPER-REALISTIC BLOOD FLUID DYNAMICS ---

    struct BloodDroplet {

        Vector3 pos;

        Vector3 vel;

        float length;

        float size;

        float life;

        bool active;

    };



    struct BloodSplatter {

        Vector3 pos;

        Vector3 vel;

        float size;

        float life;

        float maxLife;

    };



    struct PuddleRipple {

        Vector3 center;

        float radius;

        float maxRadius;

        float alpha;

    };



    std::vector<BloodDroplet> bloodDrops;

    std::vector<BloodSplatter> bloodSplatters;

    std::vector<PuddleRipple> bloodRipples;

    bloodDrops.reserve(64);

    bloodSplatters.reserve(128);

    bloodRipples.reserve(64);

    float bloodDripTimer1 = 0.0f;

    float bloodDripThreshold1 = 1.6f;

    float bloodDripTimer2 = 0.8f;

    float bloodDripThreshold2 = 2.4f;



    // --- INDUSTRIAL CEILING FAN ---

    float shopFanAngle = 0.0f;



    // --- SWAYING DYNAMIC CEILING LIGHT & PHYSICS ---

    float shopLightSwayX = 0.0f;

    float shopLightSwayZ = 0.0f;

    float shopLightIntensity = 1.0f;

    float shopLightFlickerTimer = 0.0f;

    float shopLightNextEvent = 3.5f;

    int   shopLightState = 0; // 0: steady, 1: micro-flicker, 2: blackout, 3: surge



    // --- 3D ELECTRICAL SPARK PARTICLE SYSTEM ---

    struct ShopSpark {

        Vector3 pos;

        Vector3 vel;

        float life;

        float maxLife;

        Color color;

        float size;

    };

    std::vector<ShopSpark> shopSparks;

    shopSparks.reserve(128);



    // --- CEILING FLUORESCENT TUBELIGHTS SIMULATION ---

    // Tubelight 1 (Violently Sparking, North Aisle: X = 91.5, Y = 15.02, Z = 148.0)

    float tube1Intensity = 0.8f;

    float tube1SparkTimer = 0.0f;

    float tube1NextSpark = 1.0f;

    bool  tube1IsArcing = false;

    float tube1ArcDuration = 0.0f;



    // Tubelight 2 (Dim / Dying Phosphor, Checkout Counter: X = 104.5, Y = 15.02, Z = 133.0)

    float tube2Intensity = 0.28f;



    // Tubelight 3 (Intermittent Stuttering, South Aisle: X = 94.0, Y = 15.02, Z = 132.0)

    float tube3Intensity = 0.72f;

    float tube3StutterTimer = 0.0f;

    float tube3NextStutter = 3.5f;

    int   tube3State = 0; // 0: steady on, 1: double-blink struggle, 2: off pause



    // --- MR. GRETHNAR WOULE: UNCANNY HORROR ABILITY & JUMPSCARE ENGINE ---

    enum GrethnarState {

        GRETHNAR_NORMAL = 0,

        GRETHNAR_STARING,

        GRETHNAR_PRIMED,

        GRETHNAR_VANISHED,

        GRETHNAR_JUMPSCARE,

        GRETHNAR_COOLDOWN

    };

    GrethnarState grethnarState = GRETHNAR_NORMAL;

    float grethnarStareTimer = 0.0f;

    float grethnarEyeScale = 1.0f; // Scales from 1.0f up to 2.85f

    float grethnarBloodIntensity = 0.0f; // 0.0f to 1.0f

    bool  grethnarSeenEmptyCounter = false;

    float grethnarJumpscareTimer = 0.0f;

    float grethnarJumpscareShake = 0.0f;

    float grethnarJumpscareFov = 60.0f;
    float grethnarBloodSpawnTimer = 0.0f;



    struct GrethnarEyeBloodDrop {

        Vector3 pos;

        Vector3 prevPos;

        Vector3 vel;

        float life;

        float maxLife;

        float scale;

    };

    std::vector<GrethnarEyeBloodDrop> grethnarBloodDrops;



    // Quit confirmation state

    bool shouldQuitGame  = false;

    bool showQuitConfirm = false;



    // Surreal Shop System

    // Realistic Store Inventory System
    bool isShopOpen = false;

    bool playerHasDuplicateKey = false; (void)playerHasDuplicateKey;

    int  playerMotorOilCount = 0;
    int  playerFlareCount = 0;
    int  playerBatteryCount = 0;
    int  playerRationCount = 0;
    int  playerWaterCount = 0;
    int  playerMatchCount = 0;
    int  playerBandageCount = 0;
    const char* shopFeedbackMsg = nullptr;

    float shopFeedbackTimer = 0.0f;



    struct TrailNode {

        Vector3 pos;

        float life;

    };

    TrailNode trail[16] = {0};

    int trailIndex = 0;

    Vector3 lastTrailPos = camera.position;





    // --- THUNDERSTORM SYSTEM ---

    float nextStormEventTimer = GetRandomValue(3, 10); // Shorter wait for first storm

    bool isStormActive = false;

    float stormDuration = 0.0f;

    float lightningFlashTimer = 0.0f;

    float groundImpactTimer = 0.0f;

    Vector3 groundImpactPos = {0,0,0};

    Sound sndThunder = GenerateThunderSound();

    Sound sndRain = GenerateRainSound();

    std::vector<std::pair<Vector3, Vector3>> lightningSegments;

    

    struct RainParticle {

        Vector3 pos;

        float life;

        char glyph;

    };

    std::vector<RainParticle> rainParticles;

    std::vector<Matrix> rainInstances[256];





// ============================================================



    // Create road mesh for intro and main game

    Mesh mRoad = GenMeshPlane(26.0f, 600.0f, 1, 1);

    

    // Helper to make materials

    Material matRoad = LoadMaterialDefault();

    matRoad.maps[MATERIAL_MAP_ALBEDO].color = { 0, 0, 0, 255 }; // Pure pitch black road



    // Solid ultra-dark ground plane covering the entire outside world (like the road material)

    Mesh mGround = GenMeshPlane(800.0f, 800.0f, 1, 1);

    Material matGround = LoadMaterialDefault();

    Color darkBrownBase = { 10, 7, 5, 255 }; // Much darker earthy shade (ultra-dark)

    matGround.maps[MATERIAL_MAP_ALBEDO].color = darkBrownBase;



    drawLoadingBar(0.96f, "SURVEILLANCE CAMERAS ARMED & READY...");

    drawLoadingBar(1.0f, "THE GROUND KEEPS. PREPARE YOURSELF.");

    WaitTime(0.2);






    // =========================================================================

    // GAME STATE MACHINE & 3D MAIN MENU STATE

    // =========================================================================






    // --- Phase 1: Ultimate Interactive Horror Menu State ---



    // Exterior Carpet & Secret Underground Tunnel & Bunker State

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    // Hyper-Realistic Phone & Live GPS Map State

    
    
    
    
    
    
    


    // User settings & calibration

    
    
    
    


    bool isCursorCaptured = false;

    EnableCursor();

    showQuitConfirm = false;

    float escCooldown = 0.6f;

    float gameIntroFade = 0.0f;

    if (g_lookAtAtm) {
        camera.position = Vector3{ 105.2f, 11.6f, 143.5f };
        camera.target = Vector3{ 107.45f, 11.4f, 143.5f };
    } else if (g_lookAtWashroom) {
        camera.position = Vector3{ 89.25f, 11.8f, 148.5f };
        camera.target = Vector3{ 89.25f, 11.6f, 158.0f };
    } else if (g_lookAtShop) {
        camera.position = Vector3{ 104.5f, 12.0f, 135.2f };
        camera.target = Vector3{ 104.4f, 11.72f, 133.5f };
    }


    while(!shouldQuitGame) {

        float rawDt = GetFrameTime();

        float dt = fminf(rawDt, 0.05f); // Prevent physics explosions during hitches

        

        if (hitStopTimer > 0.0f) {

            hitStopTimer -= rawDt;

            dt = 0.0f; // HIT-STOP PHYSICS: Freeze time!

        }

        if (g_gameState == STATE_PAUSED) {

            dt = 0.0f; // PAUSE STATE: Freeze gameplay simulation!

        }

        

        // --- DYNAMIC NATIVE RESOLUTION HANDLING (CRISP PIXELS ON ALL SCREENS) ---

        int curWinW = GetScreenWidth();

        int curWinH = GetScreenHeight();

        if (curWinW < 960) curWinW = 960;

        if (curWinH < 540) curWinH = 540;

        if (target.texture.width != curWinW || target.texture.height != curWinH) {

            UnloadRenderTexture(target);

            target = LoadRenderTexture(curWinW, curWinH);

            SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

            SetTextureWrap(target.texture, TEXTURE_WRAP_CLAMP);

        }

        

        float timeVal = (float)GetTime();

        // Strict in-hand validation: Shovel MUST be equipped, hands free of cart, fuel nozzle, and store products
        bool holdingShovel = (g_hasShovel && g_shovelEquipped && g_heldProductIndex == -1 && !g_isHoldingCart && !g_holdingFuelNozzle && !g_phoneActive && !g_inspectingReceipt && !showQuitConfirm && !isShopOpen);

        

        // Handle OS window close button

        if (WindowShouldClose()) showQuitConfirm = true;



        // Hyper-Realistic Phone & GPS Map Input Handling

        if (g_gameState == STATE_GAMEPLAY && !isRoofCamActive && !showQuitConfirm && !g_showSettingsModal && !g_showDossierModal && !isShopOpen) {
            phoneSystem.HandleInput();
        }



        // Smooth Phone Raise / Lower Animation

        phoneSystem.Update(dt);



        // Fullscreen Toggle: F11 or Alt+Enter
        if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {

            ToggleGameFullscreen();

        }



        // In Main Menu or Pause Menu, cursor is always free for menu navigation

        if (g_gameState == STATE_MAIN_MENU || g_gameState == STATE_PAUSED) {

            if (isCursorCaptured) {

                isCursorCaptured = false;

                EnableCursor();

            }

        } else {

            // Mouse Capture Toggle: Press Left Alt, Right Alt, or Tab to free/capture mouse so player can resize/move window

            if (IsKeyPressed(KEY_LEFT_ALT) || IsKeyPressed(KEY_RIGHT_ALT) || IsKeyPressed(KEY_TAB)) {

                isCursorCaptured = !isCursorCaptured;

                if (isCursorCaptured) DisableCursor();

                else EnableCursor();

            }



            if (IsKeyPressed(KEY_ESCAPE) && !showQuitConfirm && !isShopOpen && !IsATMActive() && !g_showSettingsModal && !g_showManifestModal) {
                isCursorCaptured = !isCursorCaptured;
                if (isCursorCaptured) DisableCursor();
                else EnableCursor();
            }

            // Modals and ATM automatically free cursor so user can click buttons or resize window
            if (showQuitConfirm || isShopOpen || IsATMActive() || g_showSettingsModal || g_showManifestModal) {

                if (isCursorCaptured) {

                    isCursorCaptured = false;

                    EnableCursor();

                }

            } else {

                // Re-capture cursor when player clicks inside the game window

                if (!isCursorCaptured && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_D)) && IsWindowFocused()) {

                    isCursorCaptured = true;

                    DisableCursor();

                }

            }

        }



        // Release cursor if window loses focus

        if (!IsWindowFocused() && isCursorCaptured) {

            isCursorCaptured = false;

            EnableCursor();

        }



        // Escape Key Hierarchy: Quit Confirmation / Exit Modals / Pause Menu

        if (escCooldown > 0.0f) {

            escCooldown -= rawDt;

        } else if (IsKeyPressed(KEY_ESCAPE)) {

            if (g_gameState == STATE_MAIN_MENU) {

                if (g_showSettingsModal || g_showCaseFilesModal || g_showSurvivalModal) {

                    g_showSettingsModal = false;

                    g_showCaseFilesModal = false;

                    g_showSurvivalModal = false;

                    escCooldown = 0.25f;

                } else {

                    shouldQuitGame = true;

                }

            } else if (g_gameState == STATE_GAMEPLAY) {

                if (isRoofCamActive) {

                    isRoofCamActive = false;

                    escCooldown = 0.25f;

                } else if (g_phoneActive) {

                    g_phoneActive = false;

                    PlaySound(g_sndPhoneSlide);

                    escCooldown = 0.25f;

                } else if (g_showDossierModal) {

                    g_showDossierModal = false;

                    isCursorCaptured = true;

                    DisableCursor();

                    escCooldown = 0.25f;

                } else if (IsATMActive()) {
                    CloseATMInteraction();
                    isCursorCaptured = true;
                    DisableCursor();
                    escCooldown = 0.25f;
                } else if (isShopOpen) {

                    isShopOpen = false;

                    escCooldown = 0.25f;

                } else {

                    g_gameState = STATE_PAUSED;

                    isCursorCaptured = false;

                    EnableCursor();

                    PlaySound(g_sndMenuBoom);

                    escCooldown = 0.25f;

                }

            } else if (g_gameState == STATE_PAUSED) {

                if (g_showSettingsModal || g_showCaseFilesModal) {

                    g_showSettingsModal = false;

                    g_showCaseFilesModal = false;

                    escCooldown = 0.25f;

                } else {

                    g_gameState = STATE_GAMEPLAY;

                    isCursorCaptured = true;

                    DisableCursor();

                    PlaySound(g_sndMenuNav);

                    escCooldown = 0.25f;

                }

            }

        }



        // Quit Confirmation Input (Ensure Enter doesn't trigger if Alt is held down for Alt+Enter)

        if (showQuitConfirm) {

            if (IsKeyPressed(KEY_Y) || (IsKeyPressed(KEY_ENTER) && !IsKeyDown(KEY_LEFT_ALT) && !IsKeyDown(KEY_RIGHT_ALT))) {

                shouldQuitGame = true; // Confirm exit

            } else if (IsKeyPressed(KEY_N)) {

                showQuitConfirm = false; // Resume game

            }

        }



        if (IsKeyPressed(KEY_V) && !showQuitConfirm) isThirdPerson = !isThirdPerson;

        UpdateRoofCCTV(dt, timeVal, isCursorCaptured, showQuitConfirm, isShopOpen);



        // Fluorescent light flicker logic inside gas station

        fluorFlickerTimer += dt;

        if (fluorFlickerTimer >= fluorNextFlicker) {

            fluorLightOn = !fluorLightOn;

            fluorFlickerTimer = 0.0f;

            fluorNextFlicker = fluorLightOn ? (float)GetRandomValue(15, 38) / 10.0f : (float)GetRandomValue(4, 16) / 100.0f;

        }



        // Commercial Automatic Sliding Glass Door: Motion sensor + damped pneumatic spring physics

        {

            // Sensor detection zone: player or shopping cart near doorway (X = 108.0f, Z = 140.0f)

            float playerDx = camera.position.x - 108.0f;

            float playerDz = camera.position.z - 140.0f;

            bool playerInSensor = (fabsf(playerDx) < 3.2f && fabsf(playerDz) < 2.0f && camera.position.y >= 10.0f && camera.position.y <= 14.5f);



            float cartDx = g_cartPos.x - 108.0f;

            float cartDz = g_cartPos.z - 140.0f;

            bool cartInSensor = (fabsf(cartDx) < 2.8f && fabsf(cartDz) < 1.8f);



            if (playerInSensor || cartInSensor) {

                doorSensorActive = true;

                doorHoldTimer = 1.0f; // Hold open for 1s after leaving detection zone

            } else {

                if (doorHoldTimer > 0.0f) {

                    doorHoldTimer -= dt;

                    doorSensorActive = true;

                } else {

                    doorSensorActive = false;

                }

            }



            // Critically damped spring simulation: F = k * (target - x) - c * v

            float doorTarget = doorSensorActive ? 1.0f : 0.0f;

            float springK = 32.0f;

            float damperC = 11.2f;

            float doorForce = (doorTarget - doorSlideProgress) * springK - doorSlideVel * damperC;

            doorSlideVel += doorForce * dt;

            doorSlideProgress += doorSlideVel * dt;

            doorSlideProgress = Clamp(doorSlideProgress, 0.0f, 1.0f);

            if (fabsf(doorSlideProgress - doorTarget) < 0.001f && fabsf(doorSlideVel) < 0.005f) {

                doorSlideProgress = doorTarget;

                doorSlideVel = 0.0f;

            }

        }



        // --- INDUSTRIAL CEILING FAN ROTATION ---

        shopFanAngle += 220.0f * dt;

        if (shopFanAngle >= 360.0f) shopFanAngle -= 360.0f;



        // --- DRAMATIC SWAYING CEILING HORROR PENDULUM (9.2 deg 2-axis elliptical swing) ---

        shopLightSwayX = sinf(timeVal * 1.25f) * 0.16f + sinf(timeVal * 2.45f) * 0.035f;

        shopLightSwayZ = cosf(timeVal * 1.05f) * 0.14f + cosf(timeVal * 1.95f) * 0.025f;



        shopLightFlickerTimer += dt;

        if (shopLightFlickerTimer >= shopLightNextEvent) {

            shopLightFlickerTimer = 0.0f;

            int roll = GetRandomValue(0, 100);

            if (roll < 55) {

                shopLightState = 0; // Steady warm light

                shopLightNextEvent = (float)GetRandomValue(28, 65) / 10.0f; // 2.8s to 6.5s

            } else if (roll < 80) {

                shopLightState = 1; // Micro filament buzz flutter

                shopLightNextEvent = (float)GetRandomValue(4, 12) / 10.0f; // 0.4s to 1.2s

            } else if (roll < 92) {

                shopLightState = 2; // Blackout / ballast drop

                shopLightNextEvent = (float)GetRandomValue(2, 5) / 10.0f; // 0.2s to 0.5s

            } else {

                shopLightState = 3; // Arc surge restrike

                shopLightNextEvent = 0.12f;

            }

        }



        float targetShopLight = 0.0f;

        if (g_shopLightsOn) {

            if (shopLightState == 0) {

                targetShopLight = 1.0f;

            } else if (shopLightState == 1) {

                targetShopLight = (sinf(timeVal * 48.0f) > -0.2f) ? 0.95f : 0.22f;

            } else if (shopLightState == 2) {

                targetShopLight = 0.05f; // terrifying near pitch gloom

            } else if (shopLightState == 3) {

                targetShopLight = 1.45f; // arc flash bloom

            }

        }

        shopLightIntensity = Lerp(shopLightIntensity, targetShopLight, dt * 25.0f);



        // --- TUBELIGHT 1: VIOLENT SPARKING & ARCING SIMULATION ---

        tube1SparkTimer += dt;

        if (tube1SparkTimer >= tube1NextSpark) {

            tube1SparkTimer = 0.0f;

            tube1NextSpark = (float)GetRandomValue(9, 26) / 10.0f; // Every 0.9s to 2.6s

            tube1IsArcing = true;

            tube1ArcDuration = 0.14f + (float)GetRandomValue(2, 14) / 100.0f;



            // Emit shower of 3D incandescent spark particles from damaged terminal at (90.25f, 15.02f, 148.0f)

            int numSparks = GetRandomValue(18, 34);

            for (int s = 0; s < numSparks; s++) {

                ShopSpark sp;

                sp.pos = {

                    90.25f + (float)GetRandomValue(-5, 5) / 100.0f,

                    15.00f + (float)GetRandomValue(-4, 2) / 100.0f,

                    148.0f + (float)GetRandomValue(-8, 8) / 100.0f

                };

                // High initial outward and downward velocity spray

                float spAngle = (float)GetRandomValue(0, 360) * DEG2RAD;

                float spSpeed = (float)GetRandomValue(12, 45) / 10.0f; // 1.2 to 4.5 m/s

                sp.vel = {

                    cosf(spAngle) * spSpeed * 0.85f - (float)GetRandomValue(5, 20) / 10.0f,

                    -(float)GetRandomValue(10, 35) / 10.0f,

                    sinf(spAngle) * spSpeed

                };

                sp.maxLife = (float)GetRandomValue(45, 110) / 100.0f; // 0.45s to 1.1s

                sp.life = sp.maxLife;

                sp.color = { 255, 255, 240, 255 }; // White hot

                sp.size = (float)GetRandomValue(16, 28) / 1000.0f;

                shopSparks.push_back(sp);

            }



            // Spatial audio playback for electrical snap/crackle

            float distToSpark = Vector3Distance(camera.position, Vector3{ 90.25f, 15.02f, 148.0f });

            float sparkVol = Clamp(1.0f - (distToSpark / 24.0f), 0.0f, 1.0f) * 0.85f;

            if (sparkVol > 0.01f) {

                SetSoundVolume(sndSpark, sparkVol);

                SetSoundPitch(sndSpark, 0.88f + (float)GetRandomValue(0, 24) / 100.0f);

                PlaySound(sndSpark);

            }

        }



        if (tube1IsArcing) {

            tube1ArcDuration -= dt;

            if (tube1ArcDuration <= 0.0f) {

                tube1IsArcing = false;

                tube1Intensity = 0.08f; // momentary drop after discharge

            } else {

                // High voltage arc surge: rapid violent flicker

                tube1Intensity = (fmodf(timeVal * 60.0f, 1.0f) > 0.35f) ? (2.1f + (float)GetRandomValue(0, 70)/100.0f) : 0.4f;

            }

        } else {

            // Struggling cathode buzz: low murky glow with occasional micro-flutter

            float buzz = sinf(timeVal * 95.0f) * 0.08f;

            tube1Intensity = Lerp(tube1Intensity, 0.22f + buzz, dt * 18.0f);

        }



        // --- TUBELIGHT 2: WARM FLUORESCENT TROFFER (OVER CHECKOUT COUNTER) ---

        // Generates a warm, focused spotlight-like golden glow over counter and floor

        float hum60 = sinf(timeVal * 120.0f) * 0.025f;

        float murmur = sinf(timeVal * 7.5f) * 0.020f;

        tube2Intensity = 0.95f + hum60 + murmur;



        // --- TUBELIGHT 3: INTERMITTENT BALLAST STUTTER (SOUTH AISLE) ---

        tube3StutterTimer += dt;

        if (tube3State == 0) {

            // Steady cool white glow

            tube3Intensity = 0.74f + sinf(timeVal * 60.0f) * 0.02f;

            if (tube3StutterTimer >= tube3NextStutter) {

                tube3StutterTimer = 0.0f;

                tube3State = (GetRandomValue(0, 10) < 6) ? 1 : 2;

                tube3NextStutter = (tube3State == 1) ? 0.38f : 0.18f;

            }

        } else if (tube3State == 1) {

            // Double-blink starter struggle

            tube3Intensity = (fmodf(timeVal, 0.14f) < 0.06f) ? 0.92f : 0.04f;

            if (tube3StutterTimer >= tube3NextStutter) {

                tube3StutterTimer = 0.0f;

                tube3State = 0;

                tube3NextStutter = (float)GetRandomValue(35, 75) / 10.0f;

            }

        } else {

            // Ballast dropout pause

            tube3Intensity = 0.02f;

            if (tube3StutterTimer >= tube3NextStutter) {

                tube3StutterTimer = 0.0f;

                tube3State = 1;

                tube3NextStutter = 0.28f;

            }

        }



        // --- UPDATE 3D SPARK PARTICLES (GRAVITY, DRAG, BOUNCE & THERMAL DECAY) ---

        for (size_t i = 0; i < shopSparks.size(); ) {

            ShopSpark& sp = shopSparks[i];

            sp.life -= dt;

            if (sp.life <= 0.0f) {

                shopSparks[i] = shopSparks.back();

                shopSparks.pop_back();

                continue;

            }

            sp.pos = Vector3Add(sp.pos, Vector3Scale(sp.vel, dt));

            sp.vel.y -= 9.8f * dt; // Gravity

            sp.vel.x *= (1.0f - dt * 0.45f); // Air resistance

            sp.vel.z *= (1.0f - dt * 0.45f);



            // Floor collision & bounce (Floor Y = 10.015)

            if (sp.pos.y <= 10.025f) {

                sp.pos.y = 10.025f;

                sp.vel.y = -sp.vel.y * 0.32f; // Bouncy spark

                sp.vel.x *= 0.65f;

                sp.vel.z *= 0.65f;

            }

            // Rack 2 top shelf collision if it lands on shelf (Y = 13.5)

            if (sp.pos.y <= 13.55f && sp.pos.y >= 13.40f && sp.pos.x >= 89.0f && sp.pos.x <= 101.0f && fabsf(sp.pos.z - 147.0f) < 0.5f) {

                sp.pos.y = 13.55f;

                sp.vel.y = -sp.vel.y * 0.25f;

            }



            // Incandescent color progression: White-hot -> Electric yellow -> Fiery orange -> Ember red

            float tNorm = sp.life / sp.maxLife;

            if (tNorm > 0.65f) {

                sp.color = { 255, 255, 240, 255 };

            } else if (tNorm > 0.35f) {

                sp.color = { 255, 215, 60, 255 };

            } else if (tNorm > 0.12f) {

                sp.color = { 255, 115, 20, 240 };

            } else {

                unsigned char alpha = (unsigned char)(tNorm / 0.12f * 200.0f);

                sp.color = { 180, 35, 10, alpha };

            }

            i++;

        }



        // --- UPDATE GLOBAL SHOP LIGHTING ENGINE CONTEXT ---

        // Dynamic day/night exterior spill calculated directly from celestial time

        float sunTheta_loop = (dayCycleTime / dayCycleDuration) * 2.0f * PI;

        float sunElev_loop = sinf(sunTheta_loop);

        g_curSunDir         = Vector3Normalize(Vector3{ cosf(sunTheta_loop), sunElev_loop, cosf(sunTheta_loop) * 0.28f });

        g_curExtNightFactor = Clamp((-sunElev_loop + 0.08f) / 0.28f, 0.0f, 1.0f);

        g_curExtDayFactor   = Clamp((sunElev_loop + 0.08f) / 0.28f, 0.0f, 1.0f);

        g_curLightningFlash = lightningFlashTimer;

        g_playerCamPos      = camera.position;

        g_playerCamFwd      = Vector3Normalize(Vector3Subtract(camera.target, camera.position));



        // Toggle Flashlight ([F])

        if (IsKeyPressed(KEY_F) && !isShopOpen && !showQuitConfirm && !g_showSettingsModal && !g_showManifestModal && g_gameState == STATE_GAMEPLAY) {

            float dToCartF = Vector2Distance(Vector2{ camera.position.x, camera.position.z }, Vector2{ g_cartPos.x, g_cartPos.z });

            if (!(g_heldProductIndex != -1 && (dToCartF < 2.2f || g_isHoldingCart))) {

                g_flashlightActive = !g_flashlightActive;

                SetSoundPitch(g_sndFlashlightToggle, g_flashlightActive ? 1.05f : 0.95f);

                PlaySound(g_sndFlashlightToggle);

            }

        }



        // =========================================================================

        // SHOP LIGHT SOURCES: 6-FIXTURE OVERHEAD GRID CONTROLLED BY MASTER WALL SWITCH

        // When switch is OFF, all fixtures drop to 0 intensity!

        // =========================================================================

        Vector3 bulbHeadPos = {

            95.0f + sinf(shopLightSwayX) * 2.05f,

            15.22f - cosf(shopLightSwayX) * cosf(shopLightSwayZ) * 2.05f - 0.08f,

            143.5f + sinf(shopLightSwayZ) * 2.05f

        };



        if (g_shopLightsOn) {

            // Light 0: Central Swaying Tungsten Pendant Bulb (Directly hanging beside & illuminating red meat carcass)
            SetShopLight(0, bulbHeadPos, Color{ 255, 215, 135, 255 }, shopLightIntensity * 3.60f, 18.0f);

            // Light 1: Tubelight 1 - North Aisle (Aisle 3 above north shelves)
            SetShopLight(1, Vector3{ 95.0f, 15.00f, 150.5f }, Color{ 215, 235, 255, 255 }, 2.80f, 12.0f);

            // Light 2: Tubelight 2 - Checkout Counter Task Spotlight
            SetShopLight(2, Vector3{ 104.5f, 14.20f, 133.5f }, Color{ 255, 220, 150, 255 }, 3.00f, 9.5f);

            // Light 3: Tubelight 3 - South Aisle (Aisle 1 above grocery shelves)
            SetShopLight(3, Vector3{ 95.0f, 15.00f, 136.5f }, Color{ 225, 235, 245, 255 }, 2.80f, 12.0f);

            // Light 4: Commercial Island Freezer LEDs (Icy cyan basin glow)
            SetShopLight(4, Vector3{ 94.5f, 10.75f, 133.5f }, Color{ 100, 205, 255, 255 }, 1.50f, 5.5f);

            // Light 5: Tubelight 5 - Entrance Corridor & Shopping Cart Bay
            SetShopLight(5, Vector3{ 104.5f, 15.00f, 143.5f }, Color{ 220, 230, 240, 255 }, 2.80f, 11.0f);

            // Light 6: Tubelight 6 - Rear Storage Corner
            SetShopLight(6, Vector3{ 89.0f, 15.00f, 133.5f }, Color{ 210, 230, 250, 255 }, 2.60f, 10.5f);

            // Light 7: Haunted Washroom Overhead Flickering Fixture
            SetShopLight(7, Vector3{ 89.0f, 13.85f, 157.5f }, Color{ 230, 248, 205, 255 }, 2.40f, 8.5f);

        } else {

            // Master store power switch OFF: blackout fixtures
            for (int li = 0; li < 8; li++) {
                SetShopLight(li, Vector3{ 95.0f, 15.0f, 140.0f }, Color{ 0, 0, 0, 0 }, 0.0f, 0.0f);
            }

        }

        bool isPlayerMoving = (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D));
        UpdateShopAtmosphere(dt, timeVal, camera.position, isPlayerMoving, g_shopLightsOn, sinf(shopLightSwayX) * 2.05f, sinf(shopLightSwayZ) * 2.05f);

        // 24-HR Cashpoint ATM Terminal: Proximity interaction & update
        bool nearATM = IsPlayerNearATM(camera.position);
        if (nearATM && IsKeyPressed(KEY_E) && !IsATMActive() && !isShopOpen && !showQuitConfirm) {
            StartATMInteraction();
        }
        bool atmInteracting = IsATMActive();
        UpdateATMSystem(dt, camera.position, atmInteracting);

        // --- MR. GRETHNAR WOULE: GAZE DETECTION, EYE SWELL & JUMPSCARE ENGINE ---

        bool playerInShop = (camera.position.x >= 85.5f && camera.position.x <= 109.2f &&
                             camera.position.z >= 125.5f && camera.position.z <= 168.0f);



        Vector3 grethnarHead = { 104.5f, 12.2f, 131.8f };

        Vector3 toGrethnar = Vector3Normalize(Vector3Subtract(grethnarHead, camera.position));

        Vector3 playerCamFwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

        float grethnarLookDot = Vector3DotProduct(playerCamFwd, toGrethnar);

        float distToGrethnar = Vector3Distance(camera.position, grethnarHead);



        bool isLookingAtGrethnar = (playerInShop && distToGrethnar < 11.0f && grethnarLookDot > 0.88f);

        bool isLookingAwayFromCounter = (grethnarLookDot < 0.35f);



        if (grethnarState == GRETHNAR_NORMAL) {

            if (isLookingAtGrethnar) {

                grethnarState = GRETHNAR_STARING;

            }

        } else if (grethnarState == GRETHNAR_STARING) {

            if (isLookingAtGrethnar) {

                grethnarStareTimer += dt;



                // Eye swelling and bleeding ONLY happen if player keeps staring continuously for at least 5-6 seconds!

                if (grethnarStareTimer >= 5.0f) {

                    float bloodProg = Clamp((grethnarStareTimer - 5.0f) / 1.0f, 0.0f, 1.0f);

                    // Subtle swelling (from 1.0x to max 1.18x - NOT too much bigger!)

                    grethnarEyeScale = 1.0f + bloodProg * 0.18f;

                    grethnarBloodIntensity = bloodProg;



                    // Spawn hyper-realistic blood drops made of '~' characters dripping down to floor

                    grethnarBloodSpawnTimer += dt;

                    float spawnRate = 0.14f - bloodProg * 0.08f;

                    if (grethnarBloodSpawnTimer >= spawnRate) {

                        grethnarBloodSpawnTimer = 0.0f;

                        float side = (GetRandomValue(0, 1) == 0) ? -0.09f : 0.09f;

                        GrethnarEyeBloodDrop bdrop;

                        bdrop.pos = {

                            104.5f + side + (float)GetRandomValue(-12, 12) / 1000.0f,

                            12.18f,

                            131.95f + (float)GetRandomValue(-8, 8) / 1000.0f

                        };

                        bdrop.prevPos = bdrop.pos;

                        bdrop.vel = {

                            (float)GetRandomValue(-15, 15) / 1000.0f,

                            -0.45f, // downward fluid velocity

                            (float)GetRandomValue(5, 20) / 1000.0f

                        };

                        bdrop.life = 3.5f;

                        bdrop.maxLife = 3.5f;

                        bdrop.scale = 0.20f + (float)GetRandomValue(0, 6) / 100.0f; // Sized nicely for ASCII '~'

                        grethnarBloodDrops.push_back(bdrop);

                    }

                } else {

                    // Before 5 seconds: eyes stay normal size, normal color, no blood

                    grethnarEyeScale = 1.0f;

                    grethnarBloodIntensity = 0.0f;

                }



                // If player kept staring for AT LEAST 6 SECONDS:

                if (grethnarStareTimer >= 6.0f) {

                    grethnarState = GRETHNAR_PRIMED;

                }

            } else {

                // Looked away before 6 seconds: eyes and stare timer gradually soothe back down

                grethnarStareTimer = fmaxf(0.0f, grethnarStareTimer - dt * 2.5f);

                grethnarEyeScale = 1.0f;

                grethnarBloodIntensity = 0.0f;

                if (grethnarStareTimer <= 0.05f) {

                    grethnarState = GRETHNAR_NORMAL;

                }

            }

        } else if (grethnarState == GRETHNAR_PRIMED) {

            // Player kept staring for at least 6 seconds! Eyes are weeping blood.

            // As soon as the player looks away from the counter:

            if (isLookingAwayFromCounter) {

                grethnarState = GRETHNAR_VANISHED;

                grethnarSeenEmptyCounter = false;

            }

        } else if (grethnarState == GRETHNAR_VANISHED) {

            // Turning back reveals an empty counter!

            if (grethnarLookDot > 0.55f) {

                grethnarSeenEmptyCounter = true; // Player registered that he is gone!

            }

            // Once the player turns around to head back toward aisle or exit:

            bool turnedAround = (grethnarSeenEmptyCounter && grethnarLookDot < 0.15f) || (grethnarLookDot < -0.22f);

            if (turnedAround && playerInShop) {

                // TRIGGER JUMPSCARE SEQUENCE!

                grethnarState = GRETHNAR_JUMPSCARE;

                grethnarJumpscareTimer = 0.45f; // For just a glimpse!

                grethnarJumpscareShake = 0.85f;

                SetSoundVolume(sndJumpscare, 1.0f);

                PlaySound(sndJumpscare);

            }

        } else if (grethnarState == GRETHNAR_JUMPSCARE) {

            grethnarJumpscareTimer -= dt;

            grethnarJumpscareShake = Lerp(grethnarJumpscareShake, 0.0f, dt * 6.0f);



            // Glimpse duration (0.45s): then disappears and appears back to counter continuing normal operations!

            if (grethnarJumpscareTimer <= 0.0f) {

                grethnarState = GRETHNAR_NORMAL; // Appears back at counter continuing normal operations

                grethnarStareTimer = 0.0f;

                grethnarEyeScale = 1.0f;

                grethnarBloodIntensity = 0.0f;

                grethnarSeenEmptyCounter = false;

                grethnarJumpscareFov = 60.0f; // restore FOV

            }

        } else if (grethnarState == GRETHNAR_COOLDOWN) {

            grethnarState = GRETHNAR_NORMAL;

        }



        // Update falling eye-blood drops made of '~' characters dripping to floor with fluid dynamic physics

        for (size_t i = 0; i < grethnarBloodDrops.size(); ) {

            GrethnarEyeBloodDrop& bd = grethnarBloodDrops[i];

            bd.life -= dt;

            if (bd.life <= 0.0f) {

                grethnarBloodDrops[i] = grethnarBloodDrops.back();

                grethnarBloodDrops.pop_back();

                continue;

            }

            bd.prevPos = bd.pos;

            bd.pos = Vector3Add(bd.pos, Vector3Scale(bd.vel, dt));

            bd.vel.y -= 14.0f * dt; // gravity pulling drops down to floor

            bd.vel.x *= (1.0f - 0.08f * dt);

            bd.vel.z *= (1.0f - 0.08f * dt);



            // Floor collision at Y = 10.025 (Drips all the way to floor, NOT mid air!)

            if (bd.pos.y <= 10.026f) {

                if (bd.vel.y < -0.6f) {

                    // Spawn fluid impact splatters and ripples on the floor

                    for (int sp = 0; sp < 4; sp++) {

                        float ang = (float)GetRandomValue(0, 360) * DEG2RAD;

                        float spd = (float)GetRandomValue(10, 32) / 100.0f;

                        BloodSplatter s;

                        s.pos = { bd.pos.x, 10.027f, bd.pos.z };

                        s.vel = { cosf(ang) * spd, (float)GetRandomValue(8, 26) / 100.0f, sinf(ang) * spd };

                        s.size = (float)GetRandomValue(6, 12) / 1000.0f;

                        s.life = 0.40f;

                        s.maxLife = 0.40f;

                        bloodSplatters.push_back(s);

                    }

                    PuddleRipple rip;

                    rip.center = { bd.pos.x, 10.027f, bd.pos.z };

                    rip.radius = 0.03f;

                    rip.maxRadius = 0.26f;

                    rip.alpha = 0.9f;

                    bloodRipples.push_back(rip);

                }

                bd.pos.y = 10.026f;

                bd.vel = { 0, 0, 0 };

            }

            i++;

        }



        // --- HANGING MEAT BLOOD DRIPPING FLUID DYNAMICS ---

        // Tip 1: Primary carcass bone tip at { 95.0f, 12.28f, 143.5f }

        bloodDripTimer1 += dt;

        if (bloodDripTimer1 >= bloodDripThreshold1) {

            bloodDripTimer1 = 0.0f;

            bloodDripThreshold1 = (float)GetRandomValue(14, 26) / 10.0f;

            BloodDroplet drop;

            drop.pos = { 95.0f, 12.22f, 143.5f };

            drop.vel = { (float)GetRandomValue(-8, 8) / 1000.0f, -0.25f, (float)GetRandomValue(-8, 8) / 1000.0f };

            drop.length = 0.035f;

            drop.size = 0.022f;

            drop.life = 3.0f;

            drop.active = true;

            bloodDrops.push_back(drop);

        }



        // Tip 2: Secondary trailing flank tip at { 95.14f, 12.42f, 143.25f }

        bloodDripTimer2 += dt;

        if (bloodDripTimer2 >= bloodDripThreshold2) {

            bloodDripTimer2 = 0.0f;

            bloodDripThreshold2 = (float)GetRandomValue(22, 38) / 10.0f;

            BloodDroplet drop;

            drop.pos = { 95.14f, 12.38f, 143.25f };

            drop.vel = { (float)GetRandomValue(-6, 6) / 1000.0f, -0.20f, (float)GetRandomValue(-6, 6) / 1000.0f };

            drop.length = 0.028f;

            drop.size = 0.018f;

            drop.life = 3.0f;

            drop.active = true;

            bloodDrops.push_back(drop);

        }



        // Simulate falling blood droplets

        for (size_t i = 0; i < bloodDrops.size(); ) {

            BloodDroplet& d = bloodDrops[i];

            if (!d.active) {

                bloodDrops[i] = bloodDrops.back();

                bloodDrops.pop_back();

                continue;

            }

            d.vel.y -= 16.0f * dt; // gravity

            d.vel.y *= (1.0f - 0.10f * dt); // air drag

            d.pos.x += d.vel.x * dt;

            d.pos.y += d.vel.y * dt;

            d.pos.z += d.vel.z * dt;

            d.length = Clamp(fabsf(d.vel.y) * 0.025f, 0.035f, 0.22f); // realistic teardrop stretch

            

            // Floor collision at Y = 10.025

            if (d.pos.y <= 10.025f) {

                d.active = false;

                if (bloodSplatters.size() < 32) {

                    for (int sp = 0; sp < 4; sp++) {

                        float ang = (float)GetRandomValue(0, 360) * DEG2RAD;

                        float spd = (float)GetRandomValue(15, 65) / 100.0f;

                        BloodSplatter s;

                        s.pos = { d.pos.x, 10.026f, d.pos.z };

                        s.vel = { cosf(ang) * spd, (float)GetRandomValue(20, 60) / 100.0f, sinf(ang) * spd };

                        s.size = (float)GetRandomValue(8, 16) / 1000.0f;

                        s.life = 0.45f;

                        s.maxLife = 0.45f;

                        bloodSplatters.push_back(s);

                    }

                }

                if (bloodRipples.size() < 16) {

                    PuddleRipple rip;

                    rip.center = { d.pos.x, 10.026f, d.pos.z };

                    rip.radius = 0.04f;

                    rip.maxRadius = 0.50f;

                    rip.alpha = 1.0f;

                    bloodRipples.push_back(rip);

                }

                bloodDrops[i] = bloodDrops.back();

                bloodDrops.pop_back();

            } else {

                d.life -= dt;

                if (d.life <= 0.0f) {

                    bloodDrops[i] = bloodDrops.back();

                    bloodDrops.pop_back();

                } else {

                    i++;

                }

            }

        }



        // Update splatters

        for (size_t i = 0; i < bloodSplatters.size(); ) {

            BloodSplatter& s = bloodSplatters[i];

            s.vel.y -= 14.0f * dt;

            s.pos.x += s.vel.x * dt;

            s.pos.y += s.vel.y * dt;

            s.pos.z += s.vel.z * dt;

            if (s.pos.y < 10.025f) {

                s.pos.y = 10.025f;

                s.vel.x *= 0.3f;

                s.vel.z *= 0.3f;

            }

            s.life -= dt;

            if (s.life <= 0.0f) {

                bloodSplatters[i] = bloodSplatters.back();

                bloodSplatters.pop_back();

            } else {

                i++;

            }

        }



        // Update ripples

        for (size_t i = 0; i < bloodRipples.size(); ) {

            PuddleRipple& r = bloodRipples[i];

            r.radius += 0.85f * dt;

            r.alpha -= 1.8f * dt;

            if (r.alpha <= 0.0f || r.radius >= r.maxRadius) {

                bloodRipples[i] = bloodRipples.back();

                bloodRipples.pop_back();

            } else {

                i++;

            }

        }



        // Interaction Proximities: Standalone Superstore (Start Left Counter: 104.5, 134.5)

        Vector3 counterPos = { 104.5f, 11.5f, 134.5f };

        bool nearCounter = (Vector3Distance(camera.position, counterPos) < 2.8f && camera.position.z >= 133.5f);



        // Precision crosshair raycast targeting: only products directly aimed at

        int focusedProductIdx = -1;

        int hudFocusIdx = -1;





        if (!isShopOpen && !isRoofCamActive && !showQuitConfirm) {

            focusedProductIdx = GetCrosshairFocusedProduct(camera, 2.8f, false);

            hudFocusIdx = GetCrosshairFocusedProduct(camera, 2.8f, true);

        }



        // Held item interaction (E: open/pour/unwrap, Left-Click: shoot gun, Q: drop)

        if (g_heldProductIndex != -1 && !isShopOpen && !isRoofCamActive && !showQuitConfirm) {

            ShopProduct &hp = g_shopProducts[g_heldProductIndex];

            if (IsKeyPressed(KEY_E)) {

                if (hp.type == PROD_TIN) {

                    hp.opened = true; // Pop open tin lid, popcorn bursts out!

                } else if (hp.type == PROD_MILK || hp.type == PROD_BLOOD) {

                    hp.opened = !hp.opened; // Toggle pouring on/off

                } else if (hp.type == PROD_CHOCOLATE) {

                    hp.opened = !hp.opened; // Toggle foil unwrapping

                    SetSoundPitch(g_sndFoil, Frand(0.95f, 1.10f));

                    PlaySound(g_sndFoil);

                }

            }



            // Tactical gun shooting animation & mechanics

            if (hp.type == PROD_GUN) {

                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_gunRecoilTimer <= 0.05f) {

                    g_gunRecoilTimer = 0.16f;

                    g_gunMuzzleFlashTimer = 0.05f;

                    SetSoundPitch(g_sndGunshot, Frand(0.96f, 1.04f));

                    PlaySound(g_sndGunshot);



                    // Eject brass casing

                    Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

                    Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwd, camera.up));

                    Vector3 up  = camera.up;

                    Vector3 ejectPos = Vector3Add(camera.position, Vector3Scale(fwd, 0.40f));

                    ejectPos = Vector3Add(ejectPos, Vector3Scale(rgt, 0.18f));

                    ejectPos = Vector3Subtract(ejectPos, Vector3Scale(up, 0.10f));



                    if (g_shellCasings.size() >= 16) {

                        g_shellCasings.erase(g_shellCasings.begin());

                    }

                    ShellCasing sc;

                    sc.pos = ejectPos;

                    sc.vel = Vector3Add(Vector3Scale(rgt, Frand(1.6f, 2.4f)), Vector3Scale(up, Frand(1.2f, 1.8f)));

                    sc.vel = Vector3Subtract(sc.vel, Vector3Scale(fwd, Frand(0.2f, 0.6f)));

                    sc.rot = Vector3{ Frand(0, 360), Frand(0, 360), Frand(0, 360) };

                    sc.rotVel = Vector3{ Frand(450, 950), Frand(350, 850), Frand(350, 850) };

                    sc.life = 4.0f;

                    sc.landed = false;

                    g_shellCasings.push_back(sc);



                    // Ballistic raycast hit & sparks

                    float hitDist = 25.0f;

                    if (fwd.y < -0.01f) {

                        float tFloor = (10.02f - camera.position.y) / fwd.y;

                        if (tFloor > 0.2f && tFloor < hitDist) hitDist = tFloor;

                    }

                    if (fwd.x < -0.01f) {

                        float tWest = (86.3f - camera.position.x) / fwd.x;

                        if (tWest > 0.2f && tWest < hitDist) hitDist = tWest;

                    }

                    if (fwd.x > 0.01f) {

                        float tEast = (107.9f - camera.position.x) / fwd.x;

                        if (tEast > 0.2f && tEast < hitDist) hitDist = tEast;

                    }

                    if (fwd.z < -0.01f) {

                        float tSouth = (126.3f - camera.position.z) / fwd.z;

                        if (tSouth > 0.2f && tSouth < hitDist) hitDist = tSouth;

                    }

                    if (fwd.z > 0.01f) {

                        float tNorth = (152.9f - camera.position.z) / fwd.z;

                        if (tNorth > 0.2f && tNorth < hitDist) hitDist = tNorth;

                    }



                    Vector3 hitPoint = Vector3Add(camera.position, Vector3Scale(fwd, hitDist));

                    int sparkCount = (g_gunSparks.size() + 16 > 32) ? (32 - (int)g_gunSparks.size()) : 16;

                    for (int sp = 0; sp < sparkCount; sp++) {

                        GunSpark gs;

                        gs.pos = hitPoint;

                        gs.vel = Vector3{ Frand(-2.5f, 2.5f) - fwd.x * 1.5f, Frand(0.8f, 3.2f), Frand(-2.5f, 2.5f) - fwd.z * 1.5f };

                        gs.life = Frand(0.2f, 0.45f);

                        gs.color = Color{ 255, (unsigned char)GetRandomValue(180, 240), 70, 255 };

                        g_gunSparks.push_back(gs);

                    }

                }

            }



            if (IsKeyPressed(KEY_Q)) {

                hp.held = false;

                hp.opened = false;

                hp.vel = Vector3{ 0.0f, -0.5f, 0.0f };

                g_heldProductIndex = -1;

            }

        } else if (focusedProductIdx != -1 && !nearCounter && !isShopOpen && !isRoofCamActive && !showQuitConfirm) {

            if (IsKeyPressed(KEY_E)) {

                g_heldProductIndex = focusedProductIdx;

                g_shopProducts[focusedProductIdx].held = true;

                // Atmospheric volumetric dust kickup when lifting any item from shelves/counters/freezers!

                SpawnPickupDust(g_shopProducts[focusedProductIdx].homePos, 28);

            }

        }



        // Counter shopkeeper [E] dialogue removed per user request; counter now used for printer & checkout



        // Surreal Superstore menu handling

        if (isShopOpen) {

            if (shopFeedbackTimer > 0.0f) {

                shopFeedbackTimer -= dt;

                if (shopFeedbackTimer <= 0.0f) shopFeedbackMsg = nullptr;

            }



            if (IsKeyPressed(KEY_ONE)) {
                float price = 4.99f;
                if (g_playerCash >= price) {
                    g_playerCash -= price;
                    playerMotorOilCount++;
                    shopFeedbackMsg = "Purchased 10W-40 Motor Oil ($4.99). Heavy quart bottle placed in inventory.";
                    shopFeedbackTimer = 5.0f;
                    PlaySound(g_sndCashRegister);
                    TriggerCashPopup(-price, "10W-40 MOTOR OIL");
                    g_camLandingDip = -0.010f;
                } else {
                    shopFeedbackMsg = "Insufficient funds. Requires $4.99.";
                    shopFeedbackTimer = 4.0f;
                }
            } else if (IsKeyPressed(KEY_TWO)) {
                float price = 6.50f;
                if (g_playerCash >= price) {
                    g_playerCash -= price;
                    playerFlareCount += 3;
                    shopFeedbackMsg = "Purchased Magnesium Road Flares [3-Pack] ($6.50). High-intensity emergency red.";
                    shopFeedbackTimer = 5.0f;
                    PlaySound(g_sndCashRegister);
                    TriggerCashPopup(-price, "ROAD FLARES 3PK");
                    g_camLandingDip = -0.010f;
                } else {
                    shopFeedbackMsg = "Insufficient funds. Requires $6.50.";
                    shopFeedbackTimer = 4.0f;
                }
            } else if (IsKeyPressed(KEY_THREE)) {
                float price = 3.89f;
                if (g_playerCash >= price) {
                    g_playerCash -= price;
                    playerBatteryCount += 2;
                    shopFeedbackMsg = "Purchased C-Cell Alkaline Batteries [2-Pack] ($3.89). Heavy-duty flashlight power.";
                    shopFeedbackTimer = 5.0f;
                    PlaySound(g_sndCashRegister);
                    TriggerCashPopup(-price, "C-CELL BATTERIES");
                    g_camLandingDip = -0.010f;
                } else {
                    shopFeedbackMsg = "Insufficient funds. Requires $3.89.";
                    shopFeedbackTimer = 4.0f;
                }
            } else if (IsKeyPressed(KEY_FOUR)) {
                float price = 2.75f;
                if (g_playerCash >= price) {
                    g_playerCash -= price;
                    playerRationCount++;
                    shopFeedbackMsg = "Purchased Canned Beef Rations & Stew ($2.75). Sealed 16oz tin with pull tab.";
                    shopFeedbackTimer = 5.0f;
                    PlaySound(g_sndCashRegister);
                    TriggerCashPopup(-price, "BEEF RATIONS");
                    g_camLandingDip = -0.010f;
                } else {
                    shopFeedbackMsg = "Insufficient funds. Requires $2.75.";
                    shopFeedbackTimer = 4.0f;
                }
            } else if (IsKeyPressed(KEY_FIVE)) {
                float price = 1.99f;
                if (g_playerCash >= price) {
                    g_playerCash -= price;
                    playerWaterCount++;
                    shopFeedbackMsg = "Purchased Mountain Spring Water (1 Gallon) ($1.99). Pure sealed drinking water.";
                    shopFeedbackTimer = 5.0f;
                    PlaySound(g_sndCashRegister);
                    TriggerCashPopup(-price, "SPRING WATER 1GAL");
                    g_camLandingDip = -0.010f;
                } else {
                    shopFeedbackMsg = "Insufficient funds. Requires $1.99.";
                    shopFeedbackTimer = 4.0f;
                }
            } else if (IsKeyPressed(KEY_SIX)) {
                float price = 1.25f;
                if (g_playerCash >= price) {
                    g_playerCash -= price;
                    playerMatchCount += 250;
                    shopFeedbackMsg = "Purchased Strike-Anywhere Matches [Box 250] ($1.25). Red sulfur tip matches.";
                    shopFeedbackTimer = 5.0f;
                    PlaySound(g_sndCashRegister);
                    TriggerCashPopup(-price, "STRIKE MATCHES");
                    g_camLandingDip = -0.010f;
                } else {
                    shopFeedbackMsg = "Insufficient funds. Requires $1.25.";
                    shopFeedbackTimer = 4.0f;
                }
            } else if (IsKeyPressed(KEY_SEVEN)) {
                float price = 5.45f;
                if (g_playerCash >= price) {
                    g_playerCash -= price;
                    playerBandageCount++;
                    shopFeedbackMsg = "Purchased Trauma Compression Bandage ($5.45). Sterile medical-grade dressing.";
                    shopFeedbackTimer = 5.0f;
                    PlaySound(g_sndCashRegister);
                    TriggerCashPopup(-price, "TRAUMA BANDAGE");
                    g_camLandingDip = -0.010f;
                } else {
                    shopFeedbackMsg = "Insufficient funds. Requires $5.45.";
                    shopFeedbackTimer = 4.0f;
                }
            } else if (IsKeyPressed(KEY_ESCAPE) || (IsKeyPressed(KEY_E) && !nearCounter)) {

                isShopOpen = false;

                shopFeedbackMsg = nullptr;

            }

        }



        Vector3 oldPos = camera.position;

        if (g_gameState == STATE_GAMEPLAY && hitStopTimer <= 0.0f && !isShopOpen && !IsATMActive() && !isRoofCamActive && !showQuitConfirm && !g_showSettingsModal && !g_showManifestModal) {

            if (IsWindowFocused()) {

                if (!isCursorCaptured && (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_S) || IsKeyPressed(KEY_D) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))) {

                    isCursorCaptured = true;

                    DisableCursor();

                }

                UpdateCamera(&camera, CAMERA_FIRST_PERSON);

            }

        }



        // -------------------------------------------------------------

        // SUPERSTORE PRODUCT HELD POSITIONING, PARTICLES & DROP PHYSICS

        // -------------------------------------------------------------

        if (g_heldProductIndex != -1) {

            Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

            Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwd, Vector3{0, 1, 0}));

            Vector3 upClean = Vector3CrossProduct(rgt, fwd);

            float vmBobX = cosf(walkTime * 0.5f - 0.4f) * 0.012f * bobAmplitude;

            float vmBobY = sinf(walkTime - 0.4f) * 0.010f * bobAmplitude;

            Vector3 holdPos = Vector3Add(camera.position, Vector3Scale(fwd, 0.52f));

            holdPos = Vector3Add(holdPos, Vector3Scale(rgt, 0.16f + g_vmSwayX + vmBobX));

            holdPos = Vector3Subtract(holdPos, Vector3Scale(upClean, 0.16f - g_vmSwayY + vmBobY));

            g_shopProducts[g_heldProductIndex].homePos = holdPos;



            ShopProduct &hp = g_shopProducts[g_heldProductIndex];

            if (hp.opened) {

                hp.spawnTimer -= dt;

                if (hp.type == PROD_TIN) {

                    if (hp.spawnTimer <= 0.0f && hp.spawnedCount < hp.maxSpawn) {

                        hp.spawnTimer = 0.10f;

                        hp.spawnedCount++;

                        ShopParticle p;

                        p.pos = Vector3{ hp.homePos.x + Frand(-0.03f, 0.03f), hp.homePos.y + 0.22f, hp.homePos.z + Frand(-0.03f, 0.03f) };

                        p.vel = Vector3{ Frand(-0.4f, 0.4f) + fwd.x * 0.4f, Frand(0.35f, 0.75f), Frand(-0.4f, 0.4f) + fwd.z * 0.4f };

                        p.radius = 0.013f;

                        p.color = Color{ (unsigned char)GetRandomValue(245, 255), (unsigned char)GetRandomValue(225, 245), (unsigned char)GetRandomValue(160, 195), 255 };

                        p.landed = false;

                        p.settleTimer = 0.0f;

                        p.isCube = true;

                        g_shopParticles.push_back(p);

                    }

                    if (hp.spawnedCount >= hp.maxSpawn) hp.opened = false;

                } else if (hp.type == PROD_MILK || hp.type == PROD_BLOOD) {

                    if (hp.fill <= 0.0f) {

                        hp.opened = false;

                    } else if (hp.spawnTimer <= 0.0f) {

                        hp.spawnTimer = 0.05f;

                        hp.fill -= 0.012f;

                        if (hp.fill < 0.0f) hp.fill = 0.0f;

                        ShopParticle p;

                        Vector3 mouth = Vector3{ hp.homePos.x + fwd.x * 0.20f, hp.homePos.y + 0.12f, hp.homePos.z + fwd.z * 0.20f };

                        p.pos = mouth;

                        p.vel = Vector3{ Frand(-0.1f, 0.1f) + fwd.x * 0.25f, -0.35f, Frand(-0.1f, 0.1f) + fwd.z * 0.25f };

                        p.radius = 0.010f;

                        p.color = (hp.type == PROD_MILK) ? Color{ 245, 245, 252, 255 } : Color{ 160, 10, 16, 255 };

                        p.landed = false;

                        p.settleTimer = 0.0f;

                        p.isCube = false;

                        g_shopParticles.push_back(p);

                    }

                }

            }

        }



        // Update physics for all active particles (Shop floor at Y = 10.02f)

        for (size_t pi = 0; pi < g_shopParticles.size(); ) {

            UpdateShopParticle(g_shopParticles[pi], dt, 10.02f);

            if (g_shopParticles[pi].landed && g_shopParticles[pi].settleTimer <= 0.0f) {

                g_shopParticles[pi] = g_shopParticles.back();

                g_shopParticles.pop_back();

            } else {

                pi++;

            }

        }

        if (g_shopParticles.size() > 64) {

            g_shopParticles.erase(g_shopParticles.begin(), g_shopParticles.begin() + (g_shopParticles.size() - 64));

        }



        UpdateDustParticles(dt);

        UpdateFootstepTrails(dt);

        UpdateShellCasingsAndSparks(dt);

        if (g_gunRecoilTimer > 0.0f) g_gunRecoilTimer -= dt;

        if (g_gunMuzzleFlashTimer > 0.0f) g_gunMuzzleFlashTimer -= dt;



        // Dropped items physics

        for (size_t i = 0; i < g_shopProducts.size(); i++) {

            ShopProduct &p = g_shopProducts[i];

            if (!p.held && (p.homePos.y > 10.02f + 0.05f) && (fabsf(p.homePos.y - p.originalPos.y) > 0.02f || fabsf(p.homePos.x - p.originalPos.x) > 0.1f || fabsf(p.homePos.z - p.originalPos.z) > 0.1f)) {

                p.vel.y -= 9.8f * dt;

                p.homePos.y += p.vel.y * dt;

                if (p.homePos.y <= 10.02f) {

                    p.homePos.y = 10.02f;

                    p.vel = Vector3{ 0, 0, 0 };

                }

            }

        }

        

        // -------------------------------------------------------------

        // KINEMATIC VOXEL & SHAPE COLLISION CONTROLLER

        // -------------------------------------------------------------

        auto isSolidBlock = [&](int vx, int vy, int vz) {

            // --- 1. ENTRANCE SHAFT & HEAVY STEEL HATCH COLLISION ---

            if (vx >= 82 && vx <= 86 && vz >= 137 && vz <= 143) {

                if (!g_tunnelHatchOpen) {

                    // Closed steel blast hatch & ground are 100% solid! Player walks firmly on surface!

                    if (vy <= 10) return true;

                    return false;

                } else {

                    // Open hatch: Steel stair descent from Y=10.0 at X=84.8 down to Y=6.5 at X=82.0

                    float stairFloor = 6.5f + ((float)(vx - 82) / 2.8f) * 3.5f;

                    if ((float)vy < stairFloor - 0.2f) return true; // Solid beneath stairs

                    if ((float)vy >= stairFloor - 0.2f && (float)vy <= 14.0f) return false; // Open shaft air

                    return true;

                }

            }



            // --- 2. UPPER CREEPY METALLIC CONDUIT (X: 62 to 82, Z: 137 to 143) ---

            if (vx >= 62 && vx < 82 && vz >= 137 && vz <= 143) {

                float tFloor = 1.2f + ((float)(vx - 62) / 20.0f) * 5.3f; // Y: 1.2 to 6.5

                // Cave-in obstruction at X in [61, 63] blocks passage until dug with shovel

                if (vx >= 61 && vx <= 63 && !g_tunnelDug && (float)vy <= tFloor + 3.2f) return true;

                // Solid floor beneath metallic diamond-plate

                if ((float)vy < tFloor - 0.2f) return true;

                // Open walking clearance inside massive metallic conduit (height 3.6m)

                if ((float)vy >= tFloor - 0.2f && (float)vy <= tFloor + 3.6f) return false;

                // Solid metallic ceiling & mountain bedrock above (strictly underground)

                if ((float)vy > tFloor + 3.6f && vy <= 10) return true;

            }



            // --- 3. LOWER DEEP METALLIC CONDUIT (X: 32 to 62, Z: 137 to 143) ---

            // Plunges from Y=1.2 down to Y=-16.0

            if (vx >= 32 && vx < 62 && vz >= 137 && vz <= 143) {

                float tFloor = -16.0f + ((float)(vx - 32) / 30.0f) * 17.2f;

                if ((float)vy < tFloor - 0.2f) return true;

                if ((float)vy >= tFloor - 0.2f && (float)vy <= tFloor + 3.6f) return false;

                if ((float)vy > tFloor + 3.6f) return true;

            }



            // --- 2. THE ABANDONED VILLAGE COLLISION (Y = -16 elevation) ---

            if (vx >= -45 && vx <= 32 && vz >= 105 && vz <= 175) {

                // Surrounding perimeter cliffs

                if ((vx <= -44 || (vx >= 31 && (vz < 137 || vz > 143)) || vz <= 106 || vz >= 174) && vy >= -16 && vy <= 6) return true;

                // Solid ground level beneath village

                if (vy <= -16) return true;



                // Central Stone Well (X: 7..9, Z: 139..141)

                if (abs(vx - 8) <= 1 && abs(vz - 140) <= 1 && vy >= -16 && vy <= -14) return true;



                // Elder's House walls (X: 2..10, Z: 156..164)

                if (vx >= 2 && vx <= 10 && vz >= 156 && vz <= 164 && vy >= -16 && vy <= -10) {

                    bool isDoor = (vx == 6 && vz == 156 && vy <= -13);

                    if (vx > 2 && vx < 10 && vz > 156 && vz < 164) {

                        if (vy <= -15) return true; // Wood floor

                        if (vy >= -14 && vy <= -11) return false; // Inside room

                        return true; // Roof

                    }

                    if (isDoor) return false;

                    return true;

                }



                // Sunken Chapel walls (X: -30..-18, Z: 135..145)

                if (vx >= -30 && vx <= -18 && vz >= 135 && vz <= 145 && vy >= -16 && vy <= -8) {

                    bool isDoor = (vx == -18 && abs(vz - 140) <= 1 && vy <= -12);

                    if (vx > -30 && vx < -18 && vz > 135 && vz < 145) {

                        if (vy <= -15) return true;

                        if (vy >= -14 && vy <= -9) return false;

                        return true;

                    }

                    if (isDoor) return false;

                    return true;

                }



                // Blacksmith shed posts & forge (X: 2..5, Z: 118..121)

                if (vx >= 2 && vx <= 5 && vz >= 118 && vz <= 121 && vy >= -16 && vy <= -12) return true;



                // Open village air

                if (vy >= -15) return false;

            }



            // Weathered Timber Pier Solid Deck Walkway & Pilings (X: 15..36, Z: 136..140, Deck Y: 10..11)
            if (vx >= 15 && vx <= 36 && vz >= 136 && vz <= 140) {
                if (vy >= 10 && vy <= 11) return true; // Pier wooden deck walkway surface
                // Pilings supporting pier deck down into seabed
                if (vy >= 4 && vy <= 10 && (vx % 3 == 0 || vx == 16 || vx == 35) && (vz == 136 || vz == 140)) return true;
                // Pier Head Fog Bell Gallows posts
                if (vx >= 16 && vx <= 17 && (vz == 136 || vz == 137 || vz == 139 || vz == 140) && vy >= 11 && vy <= 13) return true;
            }

            // Pier Diving Ladder Rungs (allows climbing out of ocean at Y=8 up to deck at Y=11)
            if (vx >= 16 && vx <= 17 && vz >= 135 && vz <= 136 && vy >= 8 && vy <= 10) {
                return true;
            }

            // Coastal Sea Stacks & Sunken Skiff Solid Collision
            if ((vx - 14)*(vx - 14) + (vz - 118)*(vz - 118) <= 9 && vy >= 4 && vy <= 12) return true;
            if ((vx - 8)*(vx - 8) + (vz - 165)*(vz - 165) <= 12 && vy >= 4 && vy <= 13) return true;
            if (vx >= 8 && vx <= 12 && vz >= 149 && vz <= 155 && vy >= 5 && vy <= 7) return true;

            // Western Ocean Boundary (X <= 35): Open sea for swimming & diving!
            if (vx <= 35) {
                if (vx < -18 || vz <= 2 || vz >= CHUNK_D - 3) return true; // Deep ocean safety perimeter
                // Submarine floor support: seabed slopes from Y=7.5m down to Y=3.8m in open sea
                float bedY = (vx >= 0) ? (3.8f + ((float)vx / 35.0f) * 3.7f) : 3.8f;
                if ((float)vy <= bedY) return true; // Solid seabed floor
                return false; // Water volume is open for swimming & diving!
            }

            // Treat the outer land edges of the world (and beneath y=0) as solid unbreakable walls
            if (vx < 0 || vx >= CHUNK_W || vy < 0 || vy >= CHUNK_H || vz < 0 || vz >= CHUNK_D) return true;

            // Road surface collision support: road width 14 (X: 121..135), road surface at Y=10
            if (abs(vx - (CHUNK_W / 2)) <= 7 && vy <= 10) return true;

            // Crashed Sedan & Impact Pole Solid Collision (Route 9 East Verge: X: 142..146, Z: 134..139)
            if (vx >= 142 && vx <= 146 && vz >= 134 && vz <= 139 && vy >= 10 && vy <= 12) return true;
            if (vx >= 144 && vx <= 146 && vz >= 138 && vz <= 140 && vy >= 10 && vy <= 16) return true; // Tilted Utility Pole

            // Abandoned Blackwood College Grounds & Building Collision (X: 148..194, Z: 118..166)
            if (vx >= 148 && vx <= 194 && vz >= 118 && vz <= 166) {
                // Ground support: flat surface at Y <= 10
                if (vy <= 10) return true;
                
                // Building walls and partitions (X: 152..188, Z: 124..160, Y: 11..16)
                if (vy >= 11 && vy <= 16) {
                    // West Exterior Facade (X = 152, Z: 124..160)
                    if (vx == 152 && vz >= 124 && vz <= 160) {
                        // Main double entrance doorway (Z: 139..141, Y <= 13 is open passage)
                        if (vz >= 139 && vz <= 141 && vy <= 13) return false;
                        return true;
                    }
                    // East Exterior Back Wall (X = 188, Z: 124..160)
                    if (vx == 188 && vz >= 124 && vz <= 160) return true;
                    // South Exterior Wall (Z = 124, X: 152..188)
                    if (vz == 124 && vx >= 152 && vx <= 188) return true;
                    // North Exterior Wall (Z = 160, X: 152..188)
                    if (vz == 160 && vx >= 152 && vx <= 188) return true;
                    
                    // Interior Corridor North Partition Wall (Z = 138, X: 152..174)
                    if (vz == 138 && vx >= 152 && vx <= 174) {
                        // Doorway into Lecture Hall 101 (X: 161..163, Y <= 13)
                        if (vx >= 161 && vx <= 163 && vy <= 13) return false;
                        return true;
                    }
                    // Interior Corridor South Partition Wall (Z = 142, X: 152..174)
                    if (vz == 142 && vx >= 152 && vx <= 174) {
                        // Doorway into Anatomy Lab 102 (X: 161..163, Y <= 13)
                        if (vx >= 161 && vx <= 163 && vy <= 13) return false;
                        return true;
                    }
                    // Interior East Corridor / Archive Partition Wall (X = 174, Z: 124..160)
                    if (vx == 174 && vz >= 124 && vz <= 160) {
                        // Doorway into Archive Room 103 (Z: 139..141, Y <= 13)
                        if (vz >= 139 && vz <= 141 && vy <= 13) return false;
                        return true;
                    }
                    // Solid roof slab at Y >= 16
                    if (vy >= 16) return true;
                }
            }

            // Shop lot, road, gas station & sedan grounds subfloor support (flush flat Y <= 10 across entire clearing)
            if (vx >= 60 && vx <= 147 && vz >= 108 && vz <= 172 && vy <= 10) return true;

            // Real geometric shop walls collision:
            if (vy >= 11 && vy <= 15) {
                // West Wall (X = 86, Z: 126..154)
                if (vx == 85 && vz >= 126 && vz <= 154) return true;

                // South Wall (Z = 126, X: 86..108)
                if (vz == 126 && vx >= 86 && vx <= 108) return true;

                // North Wall (Z = 154, X: 86..108) with Doorway into Haunted Washroom at X: 88..90
                if (vz == 154 && vx >= 86 && vx <= 108) {
                    bool inWashroomDoor = (vx >= 88 && vx <= 90 && vy <= 13);
                    if (inWashroomDoor) {
                        if (g_washroomDoorAngle < 15.0f) return true; // Closed door blocks player
                        return false; // Open door allows passage
                    }
                    return true; // Solid wall
                }

                // Haunted Washroom Solid Walls (X in [85..93], Z in [154..165])
                if (vz >= 154 && vz <= 165) {
                    if (vx <= 85 || vx >= 93) return true; // West & East washroom walls
                }
                if (vz >= 164 && vx >= 85 && vx <= 93) return true; // North washroom back wall

                // Washroom Fixtures (Porcelain Toilet & Wall Sink - compact to allow free movement)
                if ((vx == 86 || vx == 87) && vz >= 162 && vz <= 164 && vy <= 12) return true; // Toilet
                if ((vx >= 88 && vx <= 90) && vz >= 163 && vz <= 164 && vy <= 12) return true; // Sink

                // East Facade Wall (X = 108, Z: 126..154, with doorway at Z: 139..141, Y: 11..13)
                if (vx == 108 && vz >= 126 && vz <= 154) {
                    bool inDoorway = (vz >= 139 && vz <= 141 && vy <= 13);
                    if (inDoorway) {
                        if (doorSlideProgress < 0.55f) return true; // Closed / mostly closed glass door is solid
                    } else {
                        return true; // Wall is solid
                    }
                }

                // Exactly Two Superstore Gondola Shelving Racks (Rack 1 at Z = 140, Rack 2 at Z = 146, X: 89..101, Y: 11..13)
                if ((vz >= 139 && vz <= 141 && vx >= 89 && vx <= 101 && vy <= 13) ||
                    (vz >= 145 && vz <= 147 && vx >= 89 && vx <= 101 && vy <= 13)) return true;

                // 6-Door Cold Beverage Vault along West Wall (X = 86, Z: 128..150)
                if (vx == 86 && vz >= 128 && vz <= 150 && vy <= 14) return true;

                // Checkout Counter in Start Left Corner (X: 102..107, Z: 132..134, Y = 11)
                if (vx >= 102 && vx <= 107 && vz >= 132 && vz <= 134 && vy == 11) return true;

                // Hot Food & Coffee Convenience Island (X: 93..97, Z: 132..135, Y = 11)
                if (vx >= 93 && vx <= 97 && vz >= 132 && vz <= 135 && vy == 11) return true;

                // 24-HR ATM Terminal near Entrance (X = 107, Z: 143..144, Y: 11..13)
                if (vx == 107 && (vz == 143 || vz == 144) && vy <= 13) return true;
            }

            return chunk->voxels[vx][vy][vz].isSolid;

        };



        Vector3 moveDelta = Vector3Subtract(camera.position, oldPos);

        Vector3 viewDir = Vector3Subtract(camera.target, camera.position); // Preserve look direction mathematically

        camera.position = oldPos; 

        

        int px = roundf(camera.position.x);

        int pz = roundf(camera.position.z);

        

        // Gravity & Ground Detection (Realistic Human Eye Height: 1.65m)

        const float PLAYER_EYE_HEIGHT = 1.65f;

        // Water Locomotion, Wading, Surface Floating & 3D Underwater Diving
        UpdateWaterLocomotion(camera, playerVel, moveDelta, dt, timeVal);

        bool onGround = (g_waterState == WATER_STATE_DIVING || g_waterState == WATER_STATE_SURFACE) ? false : isSolidBlock(px, roundf(camera.position.y - PLAYER_EYE_HEIGHT), pz);

        if (onGround) {

            // Kinetic landing impact detection

            if (!g_wasOnGround && g_lastPlayerVelY < -2.5f) {

                float dip = Clamp(fabsf(g_lastPlayerVelY) * 0.015f, 0.035f, 0.16f);

                g_camLandingDip = -dip;

                // (Footstep landing audio silenced)

            }

            if (playerVel.y < 0.0f) playerVel.y = 0.0f;

            if (IsKeyPressed(KEY_SPACE) && hitStopTimer <= 0.0f) playerVel.y = 8.5f; 

        } else {

            if (g_waterState != WATER_STATE_DIVING && g_waterState != WATER_STATE_SURFACE) {

                playerVel.y -= 24.0f * dt; 

            }

        }

        g_wasOnGround = onGround;

        g_lastPlayerVelY = playerVel.y;



        // Sprint Mechanics ([Left Shift] 1.45x boost)

        bool hasMoveKeys = (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D));

        bool canSprint = (onGround || g_waterState == WATER_STATE_SURFACE || g_waterState == WATER_STATE_DIVING) && !g_isHoldingCart && !isShopOpen && !showQuitConfirm && !g_showSettingsModal && !g_showManifestModal && (g_gameState == STATE_GAMEPLAY);

        g_isSprinting = canSprint && IsKeyDown(KEY_LEFT_SHIFT) && hasMoveKeys;

        if (g_isSprinting) {

            moveDelta.x *= 1.45f;

            moveDelta.z *= 1.45f;

        }



        // Dynamic FOV kick (expands smoothly during sprint)

        float targetFov = g_isSprinting ? (g_userFov + 6.5f) : g_userFov;

        g_camDynamicFov = Lerp(g_camDynamicFov, targetFov, 9.0f * dt);



        // Mouse look delta & viewmodel lag updates

        Vector2 mDelta = GetMouseDelta();

        if (isCursorCaptured && IsWindowFocused()) {

            float vmTargetSwayX = Clamp(-mDelta.x * 0.00075f, -0.055f, 0.055f);

            float vmTargetSwayY = Clamp(-mDelta.y * 0.00075f, -0.045f, 0.045f);

            g_vmSwayX = Lerp(g_vmSwayX, vmTargetSwayX, 12.0f * dt);

            g_vmSwayY = Lerp(g_vmSwayY, vmTargetSwayY, 12.0f * dt);

        } else {

            g_vmSwayX = Lerp(g_vmSwayX, 0.0f, 12.0f * dt);

            g_vmSwayY = Lerp(g_vmSwayY, 0.0f, 12.0f * dt);

        }

        

        moveDelta.y += playerVel.y * dt;



        // X Collision

        camera.position.x += moveDelta.x;

        if (isSolidBlock(roundf(camera.position.x), roundf(camera.position.y), roundf(camera.position.z)) || 

            isSolidBlock(roundf(camera.position.x), roundf(camera.position.y - 1.0f), roundf(camera.position.z))) {

            camera.position.x -= moveDelta.x; 

        }

        

        // Z Collision

        camera.position.z += moveDelta.z;

        if (isSolidBlock(roundf(camera.position.x), roundf(camera.position.y), roundf(camera.position.z)) || 

            isSolidBlock(roundf(camera.position.x), roundf(camera.position.y - 1.0f), roundf(camera.position.z))) {

            camera.position.z -= moveDelta.z; 

        }

        // Continuous Oriented Collision against Crashed Sedan Body & Impact Pole
        if (camera.position.y >= 9.0f && camera.position.y <= 13.5f) {
            // 1. Crashed Car Body (Oriented Bounding Box at X = 143.8, Z = 136.5, yaw = -24 deg)
            float cdx = camera.position.x - 143.8f;
            float cdz = camera.position.z - 136.5f;
            float rad = 24.0f * DEG2RAD; // Rotating back by -yaw
            float cosR = cosf(rad);
            float sinR = sinf(rad);
            float localX = cdx * cosR - cdz * sinR;
            float localZ = cdx * sinR + cdz * cosR;

            float halfW = 1.25f + 0.42f; // Half-width + player radius
            float halfL = 2.65f + 0.42f; // Half-length + player radius

            if (fabsf(localX) < halfW && fabsf(localZ) < halfL) {
                float penX = halfW - fabsf(localX);
                float penZ = halfL - fabsf(localZ);
                if (penX < penZ) {
                    localX = (localX > 0.0f) ? halfW : -halfW;
                } else {
                    localZ = (localZ > 0.0f) ? halfL : -halfL;
                }
                camera.position.x = 143.8f + (localX * cosR + localZ * sinR);
                camera.position.z = 136.5f + (-localX * sinR + localZ * cosR);
            }

            // 2. Utility Pole Obstacle (Center: 145.3f, 138.8f, Radius: 0.25m + 0.42m = 0.67m)
            float poleDx = camera.position.x - 145.3f;
            float poleDz = camera.position.z - 138.8f;
            float poleDistSq = poleDx * poleDx + poleDz * poleDz;
            float poleMinDist = 0.68f;
            if (poleDistSq < poleMinDist * poleMinDist && poleDistSq > 0.0001f) {
                float poleDist = sqrtf(poleDistSq);
                float push = poleMinDist - poleDist;
                camera.position.x += (poleDx / poleDist) * push;
                camera.position.z += (poleDz / poleDist) * push;
            }
        }

        // 3. Weathered Timber Pier Solid Deck Walkway Collision (X: 15.5..36.5, Z: 135.8..140.2, Deck Y = 10.875)
        if (camera.position.x >= 15.5f && camera.position.x <= 36.5f && camera.position.z >= 135.8f && camera.position.z <= 140.2f) {
            float deckEyeY = 10.875f + PLAYER_EYE_HEIGHT; // Top of pier planks + player height = 12.525m
            if (camera.position.y >= deckEyeY - 0.45f && camera.position.y <= deckEyeY + 1.20f && playerVel.y <= 0.0f) {
                camera.position.y = deckEyeY;
                playerVel.y = 0.0f;
                onGround = true;
            }
        }

        // Y Collision (Roof and Floor with Step-Up Height Smoothing)

        camera.position.y += moveDelta.y;

        if (moveDelta.y > 0.0f && isSolidBlock(roundf(camera.position.x), roundf(camera.position.y + 0.2f), roundf(camera.position.z))) {

            camera.position.y -= moveDelta.y; 

            playerVel.y = 0.0f;

        } else if (moveDelta.y < 0.0f && isSolidBlock(roundf(camera.position.x), roundf(camera.position.y - PLAYER_EYE_HEIGHT), roundf(camera.position.z))) {

            int floorY = roundf(camera.position.y - PLAYER_EYE_HEIGHT);

            float targetFloorY = (float)floorY + PLAYER_EYE_HEIGHT;

            float stepDelta = camera.position.y - targetFloorY;

            if (g_waterState != WATER_STATE_DIVING && fabsf(stepDelta) > 0.04f && fabsf(stepDelta) <= 0.85f) {

                // Smooth step-up / step-down over curbs, planks, thresholds

                g_camStepOffset += stepDelta;

            }

            camera.position.y = targetFloorY; 

            playerVel.y = 0.0f;

        }

        

        // Reapply exactly the same look direction from the new collision-resolved position

        camera.target = Vector3Add(camera.position, viewDir);



        // Smooth decay for camera step offset and landing compression dip

        g_camStepOffset = Lerp(g_camStepOffset, 0.0f, 18.0f * dt);

        g_camLandingDip = Lerp(g_camLandingDip, 0.0f, 12.0f * dt);

        // -------------------------------------------------------------
        // DYNAMIC DISSOLVING FOOTPRINT STAMPING & HORROR ATMOSPHERE
        // -------------------------------------------------------------
        UpdateFootprints(camera.position, viewDir, onGround, hitStopTimer, dt);
        
        // Abandoned College Creepy Fluorescent Lighting Flicker & Audio Ambience
        g_collegeFlickerTimer -= dt;
        if (g_collegeFlickerTimer <= 0.0f) {
            float rVal = (float)GetRandomValue(0, 100);
            if (rVal < 25.0f) {
                g_collegeLightOn = false;
                g_collegeFlickerTimer = (float)GetRandomValue(4, 18) * 0.01f; // Quick stutter blackout
            } else if (rVal < 35.0f) {
                g_collegeLightOn = false;
                g_collegeFlickerTimer = (float)GetRandomValue(25, 75) * 0.01f; // Longer dark period
            } else {
                g_collegeLightOn = true;
                g_collegeFlickerTimer = (float)GetRandomValue(15, 120) * 0.01f; // Stable buzz period
            }
        }
        
        bool isInsideCollege = (camera.position.x >= 152.0f && camera.position.x <= 188.0f &&
                                camera.position.z >= 124.0f && camera.position.z <= 160.0f &&
                                camera.position.y >= 9.8f && camera.position.y <= 16.2f);
        if (isInsideCollege) {
            g_collegeCreakTimer -= dt;
            if (g_collegeCreakTimer <= 0.0f) {
                g_collegeCreakTimer = (float)GetRandomValue(16, 32);
                if (GetRandomValue(0, 1) == 0) PlaySound(g_sndWaterDrip);
                else PlaySound(g_sndFoil);
            }
        }



        // -------------------------------------------------------------

        // HIGH-FIDELITY SHOPPING CART PHYSICS ENGINE

        // Real push physics: momentum transfer, de-penetration, rolling friction,

        // obstacle collisions, yaw steering, and derived wheel spin.

        // -------------------------------------------------------------

        {

            Vector3 playerMoveDelta = Vector3Subtract(camera.position, oldPos);

            Vector2 playerSpeedXZ = { (dt > 0.0f) ? playerMoveDelta.x / dt : 0.0f,

                                      (dt > 0.0f) ? playerMoveDelta.z / dt : 0.0f };



            Vector2 playerXZ = { camera.position.x, camera.position.z };

            Vector2 cartXZ   = { g_cartPos.x, g_cartPos.z };

            float dist = Vector2Distance(playerXZ, cartXZ);



            // Grab / Release Shopping Cart

            if (g_isHoldingCart) {

                Vector3 camFwd = Vector3Normalize(Vector3{ camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z });

                Vector3 targetCartPos = { camera.position.x + camFwd.x * 1.15f, 10.02f, camera.position.z + camFwd.z * 1.15f };

                float moveDist = Vector2Distance(Vector2{ g_cartPos.x, g_cartPos.z }, Vector2{ targetCartPos.x, targetCartPos.z });

                g_cartPos.x = Lerp(g_cartPos.x, targetCartPos.x, 14.0f * dt);

                g_cartPos.z = Lerp(g_cartPos.z, targetCartPos.z, 14.0f * dt);

                g_cartPos.y = 10.02f;



                float targetYaw = atan2f(camFwd.x, camFwd.z) * RAD2DEG;

                g_cartYaw = LerpAngleDeg(g_cartYaw, targetYaw, 14.0f * dt);

                g_cartWheelSpin += (moveDist / 0.06f) * RAD2DEG;



                if (IsKeyPressed(KEY_E)) {

                    g_isHoldingCart = false;

                }

            } else {

                const float cartPushRadius = 0.65f;

                if (dist < cartPushRadius && dist > 0.0001f) {

                    Vector2 pushDir = Vector2Scale(Vector2Subtract(cartXZ, playerXZ), 1.0f / dist);

                    float overlap = cartPushRadius - dist;

                    g_cartPos.x += pushDir.x * overlap;

                    g_cartPos.z += pushDir.y * overlap;



                    float approachSpeed = playerSpeedXZ.x * pushDir.x + playerSpeedXZ.y * pushDir.y;

                    if (approachSpeed > 0.0f) {

                        g_cartVel.x += pushDir.x * approachSpeed * 1.15f;

                        g_cartVel.z += pushDir.y * approachSpeed * 1.15f;

                    }

                }

                if (dist < 1.85f && IsKeyPressed(KEY_E) && g_heldProductIndex == -1) {

                    g_isHoldingCart = true;

                    g_cartVel = Vector3{ 0, 0, 0 };

                }

            }



            // Drop held item into cart

            if (g_heldProductIndex != -1 && (dist < 2.2f || g_isHoldingCart)) {

                if (IsKeyPressed(KEY_F)) {

                    g_cartProductIndices.push_back(g_heldProductIndex);

                    g_shopProducts[g_heldProductIndex].held = false;

                    g_heldProductIndex = -1;

                    PlaySound(g_sndFoil);

                }

            }

            // Hook D: Take phantom item from ghost shopping cart [E]
            if (g_ghostCart.active && g_ghostCart.alpha > 0.35f && g_ghostCart.itemsInCart > 0 && !g_isHoldingCart) {
                float dGhost = Vector3Distance(camera.position, g_ghostCart.pos);
                if (dGhost < 2.4f && g_heldProductIndex == -1) {
                    if (IsKeyPressed(KEY_E)) {
                        g_ghostCart.itemsInCart--;
                        g_ghostCart.rattleTimer = 2.8f;
                        shopLightState = 1; // Violent light flicker surge
                        shopLightFlickerTimer = 0.0f;
                        PlaySound(g_sndFoil);
                    }
                }
            }



            // Sync items placed inside the cart

            for (size_t k = 0; k < g_cartProductIndices.size(); k++) {

                int pIdx = g_cartProductIndices[k];

                float lx = (k % 2 == 0) ? -0.12f : 0.12f;

                float lz = ((int)(k / 2) * 0.15f) - 0.12f;

                float cosY = cosf(g_cartYaw * DEG2RAD);

                float sinY = sinf(g_cartYaw * DEG2RAD);

                float wx = g_cartPos.x + lx * cosY + lz * sinY;

                float wz = g_cartPos.z - lx * sinY + lz * cosY;

                g_shopProducts[pIdx].homePos = Vector3{ wx, 10.64f, wz };

            }



            // Checkout at counter

            Vector3 cPos = { 104.5f, 11.5f, 134.5f };

            float distToCounter = Vector3Distance(camera.position, cPos);

            float cartDistToCounter = Vector3Distance(g_cartPos, cPos);

            // Store Master Light Switch Interaction [E]

            Vector3 storeSwitchPos = { 107.75f, 11.5f, 138.2f };

            float distToSwitch = Vector3Distance(camera.position, storeSwitchPos);

            if (distToSwitch < 2.2f && !isShopOpen && !showQuitConfirm && !g_isHoldingCart) {

                if (IsKeyPressed(KEY_E)) {

                    g_shopLightsOn = !g_shopLightsOn;

                    PlaySound(g_sndLightSwitch);

                }

            }

            // Washroom Ceramic Sink Faucet Interaction [E]
            if (IsPlayerNearWashroomSink(camera.position) && !isShopOpen && !showQuitConfirm && !g_isHoldingCart) {
                if (IsKeyPressed(KEY_E)) {
                    ToggleWashroomSinkFaucet();
                }
            }

            // Washroom Entrance Door Interaction [E]
            if (IsPlayerNearWashroomDoor(camera.position) && !isShopOpen && !showQuitConfirm && !g_isHoldingCart) {
                if (IsKeyPressed(KEY_E)) {
                    ToggleWashroomDoor();
                    PlaySound(g_sndChestOpen);
                }
            }



            if (distToCounter < 2.6f && (cartDistToCounter < 2.8f || g_cartProductIndices.size() > 0)) {

                if (IsKeyPressed(KEY_E) && g_printerState == PRINTER_IDLE) {

                    g_printerState = PRINTER_PRINTING;

                    g_printerProgress = 0.0f;

                    g_cartProductIndices.clear();

                    // (Footstep audio call removed)

                }

            }



            // Update printer animation & stepper sound
            if (g_printerState == PRINTER_PRINTING) {
                static float s_stepperTimer = 0.0f;
                s_stepperTimer += dt;
                if (s_stepperTimer >= 0.12f) {
                    s_stepperTimer = 0.0f;
                    PlaySound(g_sndStepperMotor);
                }

                g_printerProgress += dt * 2.8f;

                if (g_printerProgress >= 16.0f) {

                    g_printerProgress = 16.0f;

                    g_printerState = PRINTER_DONE;

                }

            }



            // Take receipt from printer

            Vector3 printerPosWorld = { 106.3f, 11.56f, 133.5f };

            if (Vector3Distance(camera.position, printerPosWorld) < 2.2f && g_printerState == PRINTER_DONE) {

                if (IsKeyPressed(KEY_E)) {

                    g_hasReceipt = true;

                    g_inspectingReceipt = true;

                    g_printerState = PRINTER_IDLE;

                    g_printerProgress = 0.0f;

                    PlaySound(g_sndFoil);

                }

            }



            // Receipt read/throw controls

            if (g_hasReceipt) {

                if (g_inspectingReceipt) {

                    if (IsKeyPressed(KEY_Q)) g_inspectingReceipt = false;

                    if (IsKeyPressed(KEY_G)) {

                        g_hasReceipt = false;

                        g_inspectingReceipt = false;

                        g_receiptThrown = true;

                        Vector3 camFwd = Vector3Normalize(Vector3{ camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z });

                        g_thrownReceiptPos = Vector3{ camera.position.x + camFwd.x * 1.1f, 10.03f, camera.position.z + camFwd.z * 1.1f };

                    }

                } else {

                    if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_R)) g_inspectingReceipt = true;

                }

            }



            // Pickup thrown receipt

            if (g_receiptThrown && Vector3Distance(camera.position, g_thrownReceiptPos) < 1.8f) {

                if (IsKeyPressed(KEY_E)) {

                    g_receiptThrown = false;

                    g_hasReceipt = true;

                    g_inspectingReceipt = true;

                    PlaySound(g_sndFoil);

                }

            }



            // --- CARPET, SECRET TUNNEL & BUNKER INTERACTION HANDLING ---

            if (g_carpetMoved && g_carpetAnim < 1.0f) {

                g_carpetAnim = Clamp(g_carpetAnim + dt * 2.8f, 0.0f, 1.0f);

            }

            if (g_tunnelHatchOpen && g_hatchAnim < 1.0f) {

                g_hatchAnim = Clamp(g_hatchAnim + dt * 3.0f, 0.0f, 1.0f);

            }

            if (g_chestUnlocked && g_chestLidAnim < 1.0f) {

                g_chestLidAnim = Clamp(g_chestLidAnim + dt * 2.5f, 0.0f, 1.0f);

            }

            if (g_tunnelBannerTimer > 0.0f) {

                g_tunnelBannerTimer -= dt;

            }



            Vector3 carpetWorld = { 83.8f, 10.0f, 140.0f };

            
            // --- FUNCTIONAL GAS STATION CUSTOMER CAR & FUEL SYSTEM UPDATE ---
            if (g_cashPopupTimer > 0.0f) g_cashPopupTimer -= dt;
            if (g_stationBellBannerTimer > 0.0f) g_stationBellBannerTimer -= dt;
            if (g_fuelSoundCooldown > 0.0f) g_fuelSoundCooldown -= dt;

            // Customer car spawner timer
            if (g_customerCar.state == CAR_INACTIVE) {
                g_customerCarCooldown -= dt;
                if (g_customerCarCooldown <= 0.0f) {
                    g_customerCarCooldown = 75.0f + (float)GetRandomValue(0, 45);
                    SpawnCustomerCar();
                }
            } else if (g_customerCar.state == CAR_APPROACHING) {
                float distToTarget = g_customerCar.targetZ - g_customerCar.pos.z;
                // Ring driveway pneumatic bell as vehicle crosses apron boundary
                if (g_customerCar.pos.z >= 115.0f && g_customerCar.pos.z - g_customerCar.speed * dt < 115.0f) {
                    PlaySound(g_sndDrivewayBell);
                    g_stationBellBannerTimer = 5.0f;
                    snprintf(g_stationBellBanner, sizeof(g_stationBellBanner), "[DRIVEWAY CHIME] A VEHICLE HAS ARRIVED AT PUMP 0%d", g_customerCar.targetPump + 1);
                }
                if (distToTarget > 0.15f) {
                    float slowFactor = Clamp(distToTarget / 25.0f, 0.15f, 1.0f);
                    g_customerCar.pos.z += g_customerCar.speed * slowFactor * dt;
                } else {
                    g_customerCar.pos.z = g_customerCar.targetZ;
                    g_customerCar.state = CAR_PARKED;
                }
            } else if (g_customerCar.state == CAR_PARKED) {
                if (g_autoDispenseMode && g_stationFuelGallons > 0.0f) {
                    g_customerCar.state = CAR_REFUELING;
                }
            } else if (g_customerCar.state == CAR_REFUELING) {
                if (g_autoDispenseMode) {
                    float fFlow = dt * 2.2f;
                    if (g_stationFuelGallons >= fFlow) {
                        g_stationFuelGallons -= fFlow;
                        g_customerCar.dispensedGallons += fFlow;
                    }
                    if (g_customerCar.dispensedGallons >= g_customerCar.requestedGallons) {
                        g_customerCar.state = CAR_PAID;
                        float sale = g_customerCar.totalSale;
                        g_playerCash += sale;
                        g_cashPopupAmount = sale;
                        g_cashPopupTimer = 4.5f;
                        snprintf(g_cashPopupText, sizeof(g_cashPopupText), "+$%.2f (AUTO-DISPENSE FUEL SALE)", sale);
                        PlaySound(g_sndNozzleShutoff);
                        PlaySound(g_sndCashRegister);
                        g_customerCar.waitTimer = 4.0f;
                    }
                }
            } else if (g_customerCar.state == CAR_PAID) {
                g_customerCar.waitTimer -= dt;
                if (g_customerCar.waitTimer <= 0.0f) {
                    g_customerCar.state = CAR_DEPARTING;
                    g_customerCar.speed = 0.0f;
                    if (g_nozzleInCar) {
                        g_nozzleInCar = false;
                        g_holdingFuelNozzle = false;
                        g_activePumpIndex = -1;
                        PlaySound(g_sndNozzleLatch);
                    }

                    // Hook B: Escalation on vehicle departure - thermal printer churns stalker warning
                    static int s_departureEscalation = 0;
                    const char* stalkerNotes[] = {
                        "THEY DID NOT DRIVE HOME",
                        "SOMEONE IS WATCHING FROM AISLE 2",
                        "DO NOT LOOK IN THE WASHROOM MIRROR",
                        "CHECK THE BASEMENT HATCH",
                        "THE GROUND REMEMBERS"
                    };
                    const char* note = stalkerNotes[s_departureEscalation % 5];
                    s_departureEscalation++;
                    UnloadTexture(g_receiptTex);
                    g_receiptTex = BuildReceiptTexture(note);
                    SetTextureFilter(g_receiptTex, TEXTURE_FILTER_BILINEAR);
                    g_printerState = PRINTER_PRINTING;
                    g_printerProgress = 0.0f;
                }
            } else if (g_customerCar.state == CAR_DEPARTING) {
                g_customerCar.speed += 8.0f * dt;
                if (g_customerCar.speed > 16.0f) g_customerCar.speed = 16.0f;
                g_customerCar.pos.z += g_customerCar.speed * dt;
                if (g_customerCar.pos.z > 230.0f) {
                    g_customerCar.state = CAR_INACTIVE;
                }
            }


            float dToCarpet = Vector3Distance(camera.position, carpetWorld);



            // 1. Interaction on surface outside shop behind back wall (Carpet & Heavy Steel Hatch)

            if (dToCarpet < 3.2f && camera.position.y > 9.2f) {

                if (IsKeyPressed(KEY_E)) {

                    if (!g_carpetMoved) {

                        g_carpetMoved = true;

                        PlaySound(g_sndFoil);

                        g_tunnelBannerTimer = 4.5f;

                        snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[CARPET PULLED BACK] REVEALED A HEAVY REINFORCED STEEL HATCH BOLTED INTO THE BEDROCK.");

                    } else if (!g_tunnelHatchOpen) {

                        g_tunnelHatchOpen = true;

                        PlaySound(g_sndChestOpen);

                        g_tunnelBannerTimer = 5.5f;

                        snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[HATCH UNLATCHED] A MASSIVE INDUSTRIAL METALLIC CONDUIT PLUNGES DEEP INTO THE DARKNESS.");

                    }

                }

            }



            // 2. Heavy Trench Shovel Pickup Near Carpet (X = 85.55, Y = 10.0, Z = 138.6)

            Vector3 worldShovelPos = { 84.4f, 10.0f, 138.6f };

            float dToShovel = Vector3Distance(camera.position, worldShovelPos);

            if (!g_hasShovel && dToShovel < 2.6f && !isShopOpen && !showQuitConfirm && !g_isHoldingCart) {

                if (IsKeyPressed(KEY_E)) {

                    g_hasShovel = true;

                    PlaySound(g_sndMenuNav);

                    g_tunnelBannerTimer = 6.0f;

                    snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[ITEM ACQUIRED] HEAVY TRENCH SHOVEL - [LMB] Swing | [RMB / E] Dig.");

                }

            }



            // Update Shovel animation clock & dirt particles

            g_shovelIdleClock += dt;

            UpdateDirtClods(dt);



            if (g_shovelAnimState != SHOVEL_ANIM_IDLE) {

                g_shovelAnimTime += dt;

                float dur = (g_shovelAnimState == SHOVEL_ANIM_DIG) ? SHOVEL_DIG_DURATION : SHOVEL_ATTACK_DURATION;

                if (g_shovelAnimTime >= dur) {

                    g_shovelAnimState = SHOVEL_ANIM_IDLE;

                    g_shovelAnimTime = 0.0f;

                }

            }



            // Trigger shovel animations when holding shovel ([LMB] Swing, [RMB / E] Dig)

            // Shovel Equip / Holster Toggle ([1] or [X])
            if (g_hasShovel && (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_X)) && !isShopOpen && !showQuitConfirm && !g_isHoldingCart && !g_holdingFuelNozzle) {
                g_shovelEquipped = !g_shovelEquipped;
                PlaySound(g_sndMenuNav);
                g_tunnelBannerTimer = 3.0f;
                snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), g_shovelEquipped ? "[SHOVEL EQUIPPED] [LMB] SWING | [RMB] DIG" : "[SHOVEL HOLSTERED]");
            }

            // Strict in-hand validation: Shovel MUST be equipped, hands free of cart, fuel nozzle, and store products
            bool holdingShovel = (g_hasShovel && g_shovelEquipped && g_heldProductIndex == -1 && !g_isHoldingCart && !g_holdingFuelNozzle && !g_phoneActive && !g_inspectingReceipt && !showQuitConfirm && !isShopOpen);

            if (holdingShovel && g_shovelAnimState == SHOVEL_ANIM_IDLE) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    g_shovelAnimState = SHOVEL_ANIM_ATTACK;
                    g_shovelAnimTime = 0.0f;
                    g_shovelAttackImpactDone = false;
                } else if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                    // Strict in-hand digging: ONLY Right Mouse Button when shovel is held!
                    g_shovelAnimState = SHOVEL_ANIM_DIG;
                    g_shovelAnimTime = 0.0f;
                    g_shovelDigImpactDone = false;
                    g_shovelDigThrowDone = false;
                }
            }

            // 3. Shovel Digging Interaction at Cave-In (X = 63.2, Y = 2.4, Z = 140.0)

            if (!g_tunnelDug) {

                float dToCaveIn = Vector3Distance(camera.position, Vector3{ 63.2f, 2.4f, 140.0f });

                if (dToCaveIn < 3.4f && camera.position.x > 61.5f && camera.position.y < 8.0f) {

                    if (IsKeyPressed(KEY_E) || (holdingShovel && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) {

                        if (!g_hasShovel) {

                            g_tunnelBannerTimer = 4.5f;

                            snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[BLOCKED CAVE-IN] A MASSIVE COLLAPSE OF EARTH & ROCK. YOU NEED A SHOVEL TO DIG THROUGH.");

                            PlaySound(g_sndFoil);

                        } else if (!holdingShovel) {

                            g_tunnelBannerTimer = 4.5f;

                            snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[EQUIP SHOVEL] HOLD THE SHOVEL IN HAND TO DIG THROUGH [PRESS 1 / X].");

                            PlaySound(g_sndFoil);

                        } else {

                            g_shovelAnimState = SHOVEL_ANIM_DIG;

                            g_shovelAnimTime = 0.0f;

                            g_shovelDigImpactDone = false;

                            g_shovelDigThrowDone = false;

                            g_digProgress += 0.35f;

                            PlaySound(g_sndShovelDig);

                            g_camLandingDip = -0.16f; // Dramatic physical jolt on shovel strike

                            if (g_digProgress >= 1.0f) {

                                g_tunnelDug = true;

                                PlaySound(g_sndChestOpen);

                                g_tunnelBannerTimer = 6.5f;

                                snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[TUNNEL EXCAVATED] THE CAVE-IN HAS BEEN CLEARED! THE DEEP TUNNEL CONTINUES DOWNWARD.");

                            } else {

                                g_tunnelBannerTimer = 2.5f;

                                snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[DIGGING TUNNEL] EXCAVATING DEBRIS: %d%%...", (int)(g_digProgress * 100.0f));

                            }

                        }

                    }

                }

            }



            // 3. Subterranean Bunker Systems & Interactions

            if (camera.position.y < 9.0f) {

                // A. Pail Water Dripping Audio (spatial)

                g_tunnelDripTimer += dt;

                if (g_tunnelDripTimer > 3.4f) {

                    g_tunnelDripTimer = 0.0f;

                    float dToBucket = Vector3Distance(camera.position, Vector3{ 98.5f, 5.0f, 137.6f });

                    if (dToBucket < 14.0f) {

                        float vol = Clamp(1.0f - (dToBucket / 14.0f), 0.1f, 0.75f);

                        SetSoundVolume(g_sndWaterDrip, vol);

                        SetSoundPitch(g_sndWaterDrip, Frand(0.92f, 1.08f));

                        PlaySound(g_sndWaterDrip);

                    }

                }



                // B. Flickering Caged Work-Light

                g_workLightBuzzTimer += dt;

                if (g_workLightBuzzTimer > 0.14f) {

                    g_workLightBuzzTimer = 0.0f;

                    if (rand() % 100 < 22) {

                        g_workLightFlicker = Frand(0.20f, 0.60f); // Rapid flicker dip

                    } else {

                        g_workLightFlicker = Frand(0.92f, 1.05f); // Normal amber glow

                    }

                }



                // C. Clandestine Radio Ambient Broadcast

                if (g_radioPower) {

                    g_radioAnim += dt * 3.5f;

                    g_radioMsgTimer += dt;

                    if (g_radioMsgTimer > 16.0f) {

                        g_radioMsgTimer = 0.0f;

                        float dToRadio = Vector3Distance(camera.position, Vector3{ 100.5f, 6.2f, 140.6f });

                        if (dToRadio < 11.0f) {

                            float vol = Clamp((1.0f - (dToRadio / 11.0f)) * 0.45f, 0.05f, 0.45f);

                            SetSoundVolume(g_sndRadioStatic, vol);

                            PlaySound(g_sndRadioStatic);

                        }

                    }

                }



                float dToTable = Vector3Distance(camera.position, Vector3{ 100.2f, 6.2f, 140.0f });

                float dToChest = Vector3Distance(camera.position, Vector3{ 100.4f, 5.5f, 137.8f });



                // D. Radio Interaction (South end of worktable)

                if (dToTable < 2.5f && camera.position.z >= 140.0f) {

                    if (IsKeyPressed(KEY_E) && !g_showDossierModal) {

                        g_radioPower = !g_radioPower;

                        PlaySound(g_sndRadioStatic);

                        if (g_radioPower) {

                            g_tunnelBannerTimer = 6.0f;

                            snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[142.85 MHz] ATTENDANT MONITOR ACTIVE: \"...WARNING: SEISMIC ACTIVITY BENEATH MILE 14...\"");

                        } else {

                            g_tunnelBannerTimer = 3.0f;

                            snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[142.85 MHz] RADIO RECEIVER POWER: OFF");

                        }

                    }

                }

                // E. Clandestine Dossier Interaction (North end of worktable)

                else if (dToTable < 2.5f && camera.position.z < 140.0f) {

                    if (IsKeyPressed(KEY_E) && !g_showDossierModal) {

                        g_showDossierModal = true;

                        g_dossierReadOnce  = true; // Player now knows code 0842

                        isCursorCaptured = false;

                        EnableCursor();

                        PlaySound(g_sndFoil);

                    }

                }

                // F. Padlocked Supply Chest Interaction

                else if (dToChest < 2.2f) {

                    if (IsKeyPressed(KEY_E)) {

                        if (!g_chestUnlocked) {

                            if (g_dossierReadOnce) {

                                g_chestUnlocked = true;

                                PlaySound(g_sndChestOpen);

                                g_tunnelBannerTimer = 5.0f;

                                snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[COMBINATION ACCEPTED: 0-8-4-2] CHEST UNLOCKED");

                            } else {

                                PlaySound(g_sndMenuNav);

                                g_tunnelBannerTimer = 4.5f;

                                snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[LOCKED] PADLOCK CODE REQUIRED. READ THE WORKTABLE DOSSIER.");

                            }

                        } else if (!g_chestLooted) {

                            g_chestLooted = true;

                            PlaySound(g_sndMenuBoom);

                            g_tunnelBannerTimer = 6.5f;

                            snprintf(g_tunnelBannerText, sizeof(g_tunnelBannerText), "[SURVIVAL CACHE LOOTED] ACQUIRED: 12-GAUGE MAGNUM AMMUNITION CACHE");

                        }

                    }

                }



                // (EMF detector loop removed)

            }



            // Rolling friction (exponential decay)

            const float cartFriction = 2.0f;

            float decay = expf(-cartFriction * dt);

            g_cartVel.x *= decay;

            g_cartVel.z *= decay;

            if (Vector2Length({ g_cartVel.x, g_cartVel.z }) < 0.015f) {

                g_cartVel.x = 0.0f;

                g_cartVel.z = 0.0f;

            }



            g_cartPos.x += g_cartVel.x * dt;

            g_cartPos.z += g_cartVel.z * dt;

            g_cartPos.y = 10.02f; // Keep aligned to superstore floor level



            // Obstacle & Store Boundaries Collision

            if (g_cartPos.x < 107.8f) {

                // Inside store bounds

                if (g_cartPos.x < 86.6f)  { g_cartPos.x = 86.6f;  g_cartVel.x = 0.0f; } // West wall

                if (g_cartPos.z < 126.6f) { g_cartPos.z = 126.6f; g_cartVel.z = 0.0f; } // South wall

                if (g_cartPos.z > 153.4f) { g_cartPos.z = 153.4f; g_cartVel.z = 0.0f; } // North wall

                if (g_cartPos.x > 107.4f) {

                    bool inDoorway = (g_cartPos.z >= 139.2f && g_cartPos.z <= 140.8f && doorSlideProgress >= 0.55f);

                    if (!inDoorway) {

                        g_cartPos.x = 107.4f;

                        g_cartVel.x = 0.0f;

                    }

                }

            } else {

                // Outside store bounds (apron / parking lot)

                if (g_cartPos.x > 113.8f) { g_cartPos.x = 113.8f; g_cartVel.x = 0.0f; }

                if (g_cartPos.z < 122.0f) { g_cartPos.z = 122.0f; g_cartVel.z = 0.0f; }

                if (g_cartPos.z > 158.0f) { g_cartPos.z = 158.0f; g_cartVel.z = 0.0f; }

                if (g_cartPos.x < 108.4f) {

                    bool inDoorway = (g_cartPos.z >= 139.2f && g_cartPos.z <= 140.8f && doorSlideProgress >= 0.55f);

                    if (!inDoorway) {

                        g_cartPos.x = 108.4f;

                        g_cartVel.x = 0.0f;

                    }

                }

            }



            // Shelf Racks & Counter AABB Obstacle Collision

            auto resolveCartAABB = [](Vector3 &pos, Vector3 &vel, float minX, float maxX, float minZ, float maxZ) {

                float rad = 0.35f;

                if (pos.x + rad > minX && pos.x - rad < maxX && pos.z + rad > minZ && pos.z - rad < maxZ) {

                    float penLeft   = (pos.x + rad) - minX;

                    float penRight  = maxX - (pos.x - rad);

                    float penBottom = (pos.z + rad) - minZ;

                    float penTop    = maxZ - (pos.z - rad);

                    float minPen = penLeft;

                    int axis = 0;

                    if (penRight < minPen)  { minPen = penRight; axis = 1; }

                    if (penBottom < minPen) { minPen = penBottom; axis = 2; }

                    if (penTop < minPen)    { minPen = penTop; axis = 3; }

                    if (axis == 0) { pos.x = minX - rad; vel.x = 0.0f; }

                    else if (axis == 1) { pos.x = maxX + rad; vel.x = 0.0f; }

                    else if (axis == 2) { pos.z = minZ - rad; vel.z = 0.0f; }

                    else if (axis == 3) { pos.z = maxZ + rad; vel.z = 0.0f; }

                }

            };

            resolveCartAABB(g_cartPos, g_cartVel, 88.5f, 101.5f, 139.0f, 141.0f); // Rack 1

            resolveCartAABB(g_cartPos, g_cartVel, 88.5f, 101.5f, 146.0f, 148.0f); // Rack 2

            resolveCartAABB(g_cartPos, g_cartVel, 101.8f, 107.2f, 132.8f, 134.5f); // Checkout Counter



            // Body Yaw Steering & Wheel Spin

            float speed = sqrtf(g_cartVel.x * g_cartVel.x + g_cartVel.z * g_cartVel.z);

            if (speed > 0.03f) {

                float targetYaw = atan2f(g_cartVel.x, g_cartVel.z) * RAD2DEG;

                g_cartYaw = LerpAngleDeg(g_cartYaw, targetYaw, 1.0f - expf(-6.0f * dt));

            }

            const float wheelRadius = 0.06f;

            g_cartWheelSpin += (speed / wheelRadius) * RAD2DEG * dt;



            // Sync cart position with shop products array

            for (size_t i = 0; i < g_shopProducts.size(); i++) {

                if (g_shopProducts[i].type == PROD_CART) {

                    g_shopProducts[i].homePos = g_cartPos;

                    break;

                }

            }

        }

        

        if (swingTimer > 0.0f) swingTimer -= dt;

        

        // DIG MECHANIC - STRICTLY ENFORCED: ONLY WHEN SHOVEL IS HELD IN HAND VIA [RMB]
        if (holdingShovel && g_gameState == STATE_GAMEPLAY && !isShopOpen && !showQuitConfirm && !g_showSettingsModal && !g_showManifestModal && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && swingTimer <= 0.0f) {
            swingTimer = 0.3f;

            Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

            Vector3 pos = camera.position;

            

            for (float t = 0; t < 15.0f; t += 0.2f) {

                Vector3 check = Vector3Add(pos, Vector3Scale(forward, t));

                int cx = round(check.x);

                int cy = round(check.y);

                int cz = round(check.z);

                

                if (cx >= 0 && cx < CHUNK_W && cy >= 0 && cy < CHUNK_H && cz >= 0 && cz < CHUNK_D) {

                    if (chunk->voxels[cx][cy][cz].isSolid) {

                        if (cy == 0) {

                            // Bedrock reached, cannot dig further down

                            hitStopTimer = 0.05f; // Small bump

                            break;

                        }

                        int radius = 2;

                        int debrisSpawned = 0;

                        for(int dx = -radius; dx <= radius; dx++) {

                            for(int dy = -radius; dy <= radius; dy++) {

                                for(int dz = -radius; dz <= radius; dz++) {

                                    if (dx*dx + dy*dy + dz*dz <= radius*radius) {

                                        int vx = cx + dx;

                                        int vy = cy + dy;

                                        int vz = cz + dz;

                                        if (vx >= 0 && vx < CHUNK_W && vy > 0 && vy < CHUNK_H && vz >= 0 && vz < CHUNK_D) {

                                            if (dx == 0 && dz == 0 && dy < 0) continue; 

                                            Voxel& vox = chunk->voxels[vx][vy][vz];

                                            if (vy <= 10 && (fabsf((float)vx - 128.0f) <= 7.0f || (vx >= 75 && vx <= 118 && vz >= 118 && vz <= 162) || (vx >= 148 && vx <= 194 && vz >= 118 && vz <= 166))) continue; // Protect highway, shop & college floors
                                                if (vox.isSolid) {

                                                vox.isSolid = false;

                                                // Lightweight particle burst (up to 12 particles)

                                                if (debrisSpawned < 12 && debris.size() < 64 && (GetRandomValue(0, 2) == 0)) {

                                                    debrisSpawned++;

                                                    PhysicsParticle p;

                                                    p.pos = {(float)vx, (float)vy, (float)vz};

                                                    Vector3 dir = {(float)dx, (float)dy + 1.0f, (float)dz};

                                                    if (Vector3Length(dir) > 0.001f) dir = Vector3Normalize(dir);

                                                    else dir = {0, 1.0f, 0};

                                                    float speed = GetRandomValue(30, 65) / 10.0f;

                                                    p.vel = Vector3Scale(dir, speed);

                                                    p.glyphIndex = vox.glyphIndex;

                                                    p.color = vox.fgColor;

                                                    p.life = GetRandomValue(12, 22) / 10.0f; // 1.2 to 2.2 seconds

                                                    debris.push_back(p);

                                                }

                                            }

                                        }

                                    }

                                }

                            }

                        }

                        // FAST SPATIAL REBUILD: Only rebuild affected 32x32 buckets (1-2 buckets instead of 64!)

                        int minBx = Clamp((cx - radius - 2) / BUCKET_SIZE, 0, BUCKETS_X - 1);

                        int maxBx = Clamp((cx + radius + 2) / BUCKET_SIZE, 0, BUCKETS_X - 1);

                        int minBz = Clamp((cz - radius - 2) / BUCKET_SIZE, 0, BUCKETS_Z - 1);

                        int maxBz = Clamp((cz + radius + 2) / BUCKET_SIZE, 0, BUCKETS_Z - 1);

                        for (int bx = minBx; bx <= maxBx; bx++) {

                            for (int bz = minBz; bz <= maxBz; bz++) {

                                chunk->BuildBucket(bx, bz);

                            }

                        }

                        digShake = 0.25f; 

                        hitStopTimer = 0.02f; // Smooth micro-impact without visual freezing

                        break;

                    }

                }

            }

        }



        // PHYSICS (Optimized: limit debris count + early termination)

        if (debris.size() > 256) debris.resize(256);  // Cap debris count

        for(int i = 0; i < (int)debris.size(); ) {

            PhysicsParticle& p = debris[i];

            p.vel.y -= 25.0f * dt; 

            Vector3 nextPos = Vector3Add(p.pos, Vector3Scale(p.vel, dt));

            int vx = round(nextPos.x); int vy = round(nextPos.y); int vz = round(nextPos.z);

            bool collision = false;

            if (vx >= 0 && vx < CHUNK_W && vy >= 0 && vy < CHUNK_H && vz >= 0 && vz < CHUNK_D) {

                if (chunk->voxels[vx][vy][vz].isSolid) collision = true;

            } else if (vy < 0) {

                collision = true;

            }

            if (collision) {

                p.vel.x *= 0.5f; p.vel.z *= 0.5f; p.vel.y *= -0.3f;

                if (Vector3Length(p.vel) < 1.0f) p.vel = {0, 0, 0}; 

                nextPos.y = p.pos.y; 

            } 

            p.pos = nextPos;

            p.life -= dt;

            if (p.life <= 0.0f) {

                debris[i] = debris.back();

                debris.pop_back();

            } else {

                i++;

            }

        }

        

        for(int i = 0; i < 256; i++) debrisInstances[i].clear();

        for(const auto& p : debris) {

            Matrix m = MatrixIdentity();

            m.m0 = p.color.r / 255.0f; m.m1 = p.color.g / 255.0f; m.m2 = p.color.b / 255.0f; m.m3 = p.color.a / 255.0f;

            m.m4 = 1.0f; m.m5 = 1.0f;

            m.m12 = p.pos.x; m.m13 = p.pos.y; m.m14 = p.pos.z;

            debrisInstances[p.glyphIndex].push_back(m);

        }



        // CELESTIAL DAY/NIGHT TIMING & CONTROLS

        if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_BACKSLASH)) dayCyclePaused = !dayCyclePaused;

        if (!dayCyclePaused) dayCycleTime += dt;

        if (IsKeyDown(KEY_LEFT_BRACKET))  dayCycleTime -= dt * 18.0f; // Fast rewind time of day

        if (IsKeyDown(KEY_RIGHT_BRACKET)) dayCycleTime += dt * 18.0f; // Fast forward time of day

        if (dayCycleTime < 0.0f) dayCycleTime += dayCycleDuration;

        if (dayCycleTime >= dayCycleDuration) dayCycleTime = fmodf(dayCycleTime, dayCycleDuration);

        

        float sunTheta = (dayCycleTime / dayCycleDuration) * 2.0f * PI;

        float sunElev = sinf(sunTheta);

        Vector3 sunDir = Vector3Normalize(Vector3{ cosf(sunTheta), sunElev, cosf(sunTheta) * 0.28f });

        Vector3 moonDir = Vector3Negate(sunDir);

        float nightFactor = Clamp((-sunElev + 0.08f) / 0.28f, 0.0f, 1.0f);



        // STAR INSTANCES (Parallax locked, smoothly shimmers into view at night)

        for(int i=0; i<256; i++) starInstances[i].clear();

        if (nightFactor > 0.02f) {

            for(const auto& s : stars) {

                float tw = (0.5f + 0.5f * sinf(timeVal * 1.8f + s.phase)) * nightFactor;

                Matrix m = MatrixIdentity();

                m.m0 = 1.0f; m.m1 = 1.0f; m.m2 = 1.0f; m.m3 = tw;

                float sz = s.isBig ? 6.0f : 3.0f;

                m.m4 = sz; m.m5 = sz;

                m.m9 = 1.0f; // Disable Fog

                m.m10 = 0.0f; // Billboard

                m.m11 = 1.0f;

                m.m12 = s.basePos.x + camera.position.x;

                m.m13 = s.basePos.y + camera.position.y;

                m.m14 = s.basePos.z + camera.position.z;

                uint8_t glyph = s.isBig ? '*' : '.';

                starInstances[glyph].push_back(m);

            }

        }



        // MOON INSTANCES (Orbiting along true celestial arc, fading during midday)

        for(int i=0; i<256; i++) moonInstances[i].clear();

        float moonAlpha = Clamp(nightFactor * 0.95f + 0.05f, 0.0f, 0.95f);

        if (moonDir.y > -0.15f && moonAlpha > 0.04f) {

            Vector3 moonCenter = Vector3Add(camera.position, Vector3Scale(moonDir, 280.0f));

            Vector3 moonForward = Vector3Normalize(Vector3Subtract(camera.position, moonCenter)); 

            Vector3 upRefMoon = (fabsf(moonForward.y) > 0.88f) ? Vector3{0, 0, 1} : Vector3{0, 1, 0};

            Vector3 moonRight = Vector3Normalize(Vector3CrossProduct(upRefMoon, moonForward));

            

            for(int row=0; row<5; row++) {

                for(int col=0; col<11; col++) {

                    char c = moonArt[row][col];

                    if (c == ' ') continue;

                    float dx = (col - 5) * 5.0f;

                    float dy = (2 - row) * 8.0f; 

                    Vector3 pos = Vector3Add(moonCenter, Vector3Scale(moonRight, dx));

                    pos = Vector3Add(pos, Vector3Scale(Vector3{0,1,0}, dy));

                    Matrix m = MatrixIdentity();

                    m.m0 = 220/255.0f; m.m1 = 225/255.0f; m.m2 = 240/255.0f; m.m3 = moonAlpha;

                    m.m4 = 8.0f; m.m5 = 8.0f; 

                    m.m9 = 1.0f; // Disable fog

                    m.m10 = 0.0f; // Billboard

                    m.m11 = 1.0f;

                    m.m12 = pos.x; m.m13 = pos.y; m.m14 = pos.z;

                    moonInstances[(uint8_t)c].push_back(m);

                }

            }

        }



        // ATMOSPHERIC CLOUD PHYSICS

        UpdateCloudPhysics(dt, timeVal, lightningFlashTimer);

        

        // NOCTURNAL AUTONOMOUS GHOST TROLLEY

        UpdateGhostCart(dt, nightFactor);

        

        // PROCEDURAL HORROR BOVINE SKELETON NPCS

        bool isRainingBovine = !rainParticles.empty();

        for (int b = 0; b < MAX_BOVINE_NPCS; b++) {

            UpdateBovineAI(g_bovineNPCs[b], camera.position, dt, isRainingBovine);

            UpdateBovineKinematics(g_bovineNPCs[b], dt);

        }

        

        // PROCEDURAL HORROR HOUND (DOG NPC)

        UpdateDogAI(g_houndNPC, camera.position, dt, lightningFlashTimer, nightFactor > 0.35f);

        UpdateDog(g_houndNPC, dt);

        

        // Hound Interaction (Petting & Blood Feeding Companion System)

        float distToHound = Vector3Distance(camera.position, g_houndNPC.pos);

        bool hasBloodBottle = (g_heldProductIndex != -1 && g_shopProducts[g_heldProductIndex].type == PROD_BLOOD && g_shopProducts[g_heldProductIndex].fill > 0.02f);

        

        // If pouring blood on ground near hound, hound drinks and becomes pet

        if (hasBloodBottle && g_shopProducts[g_heldProductIndex].opened && distToHound < 3.8f) {

            if (!g_houndNPC.isPet) {

                g_houndNPC.isPet = true;

                g_houndNPC.drinkTimer = 2.5f;

                PlaySound(g_sndFoil);

            }

        }



        // Context Interaction at 4m distance

        if (distToHound < 4.0f && IsKeyPressed(KEY_E) && !isShopOpen && !showQuitConfirm && !g_isHoldingCart) {

            if (hasBloodBottle) {

                // Feed blood bottle to hound

                ShopProduct &bp = g_shopProducts[g_heldProductIndex];

                bp.fill -= 0.35f;

                if (bp.fill < 0.0f) bp.fill = 0.0f;

                g_houndNPC.isPet = true;

                g_houndNPC.drinkTimer = 3.0f;

                PlaySound(g_sndFoil);

            } else if (g_heldProductIndex == -1) {

                // Pet Hound within 4.0m (narrative text removed per user instruction)

                g_houndNPC.petTimer = 3.0f;

                PlaySound(g_sndFoil);

            }

        }

        

        // VIEW BOBBING & CAMERA LOGIC

        bool isMoving = (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D));

        int lastStep = (int)(walkTime / PI);



        float strideRate = g_isSprinting ? 19.5f : 13.5f;

        if (isMoving && hitStopTimer <= 0.0f) {

            bobAmplitude = Lerp(bobAmplitude, g_isSprinting ? 1.35f : 1.0f, 10.0f * dt);

            walkTime += dt * strideRate;

        } else if (hitStopTimer <= 0.0f) {

            bobAmplitude = Lerp(bobAmplitude, 0.0f, 10.0f * dt);

            float targetWalkTime = roundf(walkTime / PI) * PI;

            walkTime = Lerp(walkTime, targetWalkTime, 10.0f * dt);

        }



        int currentStep = (int)(walkTime / PI);

        // Only trigger steps if we actually advanced across the PI boundary (lowest point of bob)

        if (isMoving && currentStep > lastStep) {

            bool inShop = (camera.position.x >= 86.0f && camera.position.x <= 108.5f &&

                           camera.position.z >= 126.0f && camera.position.z <= 153.5f);

            bool onGround = (camera.position.y <= 12.5f);

            if (onGround) {

                g_isLeftFootStep = !g_isLeftFootStep;

                Vector3 stepFwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

                Vector3 stepRgt = Vector3Normalize(Vector3CrossProduct(stepFwd, Vector3{0, 1, 0}));

                float footOffset = g_isLeftFootStep ? -0.16f : 0.16f;

                Vector3 footPos = { camera.position.x + stepRgt.x * footOffset, 10.019f, camera.position.z + stepRgt.z * footOffset };

                float footYaw = atan2f(stepFwd.x, stepFwd.z) * RAD2DEG;



                if (inShop) {

                    // Footstep audio silenced per user request

                    AddFootstepTrail(footPos, footYaw, g_isLeftFootStep);

                    // Kick up subtle micro dust speck at shoe impact if under particle limit

                    if (g_dustParticles.size() < 48) {

                        DustParticle p;

                        p.pos = Vector3{ footPos.x + Frand(-0.02f, 0.02f), 10.03f, footPos.z + Frand(-0.02f, 0.02f) };

                        p.vel = Vector3{ Frand(-0.06f, 0.06f), Frand(0.06f, 0.14f), Frand(-0.06f, 0.06f) };

                        p.size = Frand(0.006f, 0.012f);

                        p.maxLife = Frand(0.6f, 1.0f);

                        p.life = p.maxLife;

                        p.color = Color{ 165, 160, 150, 150 };

                        p.spin = Frand(0.0f, 360.0f);

                        p.spinSpeed = Frand(-60.0f, 60.0f);

                        g_dustParticles.push_back(p);

                    }

                } else {

                    // Footstep audio silenced per user request

                }

            }

        }

        

        // Loop Ambience

        if (!IsSoundPlaying(sndCrickets)) PlaySound(sndCrickets);

        if (!IsSoundPlaying(sndWind)) PlaySound(sndWind);

        

        // Natural Figure-8 Human Gait Kinematics

        Vector3 forwardBob = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

        Vector3 rightBob   = Vector3Normalize(Vector3CrossProduct(forwardBob, Vector3{ 0.0f, 1.0f, 0.0f }));

        Vector3 upClean    = Vector3CrossProduct(rightBob, forwardBob);



        // Figure-8 stride bobbing (two vertical dips per stride cycle, one horizontal sway)

        float bobX = cosf(walkTime * 0.5f) * 0.016f * bobAmplitude;

        float bobY = (sinf(walkTime) * 0.5f - 0.5f) * 0.024f * bobAmplitude;

        float bobZ = sinf(walkTime) * 0.005f * bobAmplitude;



        // Stationary idle breathing sway

        g_camIdleTimer += dt;

        float idleFactor = Clamp(1.0f - bobAmplitude, 0.0f, 1.0f);

        float idleBreatheY = sinf(g_camIdleTimer * 1.5f) * 0.0035f * idleFactor;

        float idleBreatheX = cosf(g_camIdleTimer * 0.75f) * 0.0020f * idleFactor;



        // Dynamic Camera Banking / Roll into Strafing and Turns

        float targetRoll = 0.0f;

        if (isMoving) {

            if (IsKeyDown(KEY_A)) targetRoll += 0.024f; // ~1.37 deg left bank

            if (IsKeyDown(KEY_D)) targetRoll -= 0.024f; // ~1.37 deg right bank

        }

        Vector2 mDeltaLook = GetMouseDelta();

        targetRoll += Clamp(-mDeltaLook.x * 0.00035f, -0.028f, 0.028f);

        targetRoll += sinf(walkTime * 0.5f) * 0.008f * bobAmplitude; // subtle gait sway

        if (g_waterState == WATER_STATE_SURFACE) {
            targetRoll += g_waterSmoothTiltRoll;
        }

        g_camRoll = Lerp(g_camRoll, targetRoll, 10.0f * dt);



        Vector3 rollUp = Vector3Normalize(Vector3Add(Vector3Scale(upClean, cosf(g_camRoll)), Vector3Scale(rightBob, sinf(g_camRoll))));



        // Combine bobbing, idle breathing, step smoothing, and landing compression

        Vector3 bobOffset = Vector3Add(Vector3Scale(rightBob, bobX + idleBreatheX), Vector3Scale(upClean, bobY + idleBreatheY));

        bobOffset = Vector3Add(bobOffset, Vector3Scale(forwardBob, bobZ));

        bobOffset.y += g_camStepOffset + g_camLandingDip;

        

        if (digShake > 0.0f) {

            float sx = (GetRandomValue(-100, 100) / 100.0f) * digShake;

            float sy = (GetRandomValue(-100, 100) / 100.0f) * digShake;

            bobOffset = Vector3Add(bobOffset, Vector3Scale(rightBob, sx));

            bobOffset = Vector3Add(bobOffset, Vector3Scale(camera.up, sy));

            if (hitStopTimer <= 0.0f) digShake -= rawDt * 3.0f;

            if (digShake < 0.0f) digShake = 0.0f;

        }

        

        static std::vector<Matrix> playerInstances[256];

        for (int i = 0; i < 256; i++) playerInstances[i].clear();

        

        if (isRoofCamActive) {
            SetupRoofCCTVCamera(renderCam, camera, playerInstances);
        } else if (isThirdPerson) {

            // ELASTIC THIRD PERSON CAMERA (INDOOR AWARE & WALL-COLLISION PROTECTED)

            if (playerInShop) {

                // Tighter, responsive indoor third-person camera (distance ~2.1m, height ~0.55m)

                float indoorDist = 2.1f;

                float indoorHeight = 0.55f;

                Vector3 idealPos = Vector3Subtract(camera.position, Vector3Scale(forwardBob, indoorDist));

                idealPos.y += indoorHeight;



                // Shop interior boundaries

                const float SHOP_CAM_MIN_X = 86.8f;

                const float SHOP_CAM_MAX_X = 107.2f;

                const float SHOP_CAM_MIN_Z = 126.8f;
                const float SHOP_CAM_MAX_Z = 167.2f;

                const float SHOP_CAM_MIN_Y = 10.6f;

                const float SHOP_CAM_MAX_Y = 14.4f;



                // Ray clipping against shop outer walls so camera pulls in smoothly near walls

                Vector3 camDir = Vector3Subtract(idealPos, camera.position);

                float maxT = 1.0f;

                if (camDir.x < -1e-4f && idealPos.x < SHOP_CAM_MIN_X) {

                    maxT = fminf(maxT, (SHOP_CAM_MIN_X - camera.position.x) / camDir.x);

                }

                if (camDir.x > 1e-4f && idealPos.x > SHOP_CAM_MAX_X) {

                    maxT = fminf(maxT, (SHOP_CAM_MAX_X - camera.position.x) / camDir.x);

                }

                if (camDir.z < -1e-4f && idealPos.z < SHOP_CAM_MIN_Z) {

                    maxT = fminf(maxT, (SHOP_CAM_MIN_Z - camera.position.z) / camDir.z);

                }

                if (camDir.z > 1e-4f && idealPos.z > SHOP_CAM_MAX_Z) {

                    maxT = fminf(maxT, (SHOP_CAM_MAX_Z - camera.position.z) / camDir.z);

                }

                if (camDir.y > 1e-4f && idealPos.y > SHOP_CAM_MAX_Y) {

                    maxT = fminf(maxT, (SHOP_CAM_MAX_Y - camera.position.y) / camDir.y);

                }

                if (camDir.y < -1e-4f && idealPos.y < SHOP_CAM_MIN_Y) {

                    maxT = fminf(maxT, (SHOP_CAM_MIN_Y - camera.position.y) / camDir.y);

                }

                maxT = Clamp(maxT, 0.55f, 1.0f);

                idealPos = Vector3Add(camera.position, Vector3Scale(camDir, maxT));



                // Clamp idealPos safely inside shop interior

                idealPos.x = Clamp(idealPos.x, SHOP_CAM_MIN_X, SHOP_CAM_MAX_X);

                idealPos.y = Clamp(idealPos.y, SHOP_CAM_MIN_Y, SHOP_CAM_MAX_Y);

                idealPos.z = Clamp(idealPos.z, SHOP_CAM_MIN_Z, SHOP_CAM_MAX_Z);



                if (hitStopTimer <= 0.0f) {

                    renderCam.position = Vector3Lerp(renderCam.position, idealPos, rawDt * 12.0f);

                    renderCam.position.x = Clamp(renderCam.position.x, SHOP_CAM_MIN_X, SHOP_CAM_MAX_X);

                    renderCam.position.y = Clamp(renderCam.position.y, SHOP_CAM_MIN_Y, SHOP_CAM_MAX_Y);

                    renderCam.position.z = Clamp(renderCam.position.z, SHOP_CAM_MIN_Z, SHOP_CAM_MAX_Z);

                }

                renderCam.target = Vector3{ camera.position.x, camera.position.y - 0.20f, camera.position.z };

                renderCam.up = Vector3{0, 1, 0};

            } else {

                // Outdoor elastic drone camera (5.0m distance, 1.5m height)

                Vector3 idealPos = Vector3Subtract(camera.position, Vector3Scale(forwardBob, 5.0f));

                idealPos.y += 1.5f;

                if (hitStopTimer <= 0.0f) {

                    renderCam.position = Vector3Lerp(renderCam.position, idealPos, rawDt * 8.0f);

                }

                renderCam.target = camera.position;

                renderCam.up = Vector3{0, 1, 0};

            }

            

            // Screen shake for renderCam

            renderCam.position = Vector3Add(renderCam.position, bobOffset);

            renderCam.target = Vector3Add(renderCam.target, bobOffset);



            // Apply Grethnar Jumpscare Zoom & Screen Shake to 3rd person camera as well

            if (grethnarState == GRETHNAR_JUMPSCARE) {

                renderCam.fovy = grethnarJumpscareFov;

                if (grethnarJumpscareShake > 0.005f) {

                    float jx = ((float)GetRandomValue(-100, 100) / 100.0f) * grethnarJumpscareShake * 0.18f;

                    float jy = ((float)GetRandomValue(-100, 100) / 100.0f) * grethnarJumpscareShake * 0.16f;

                    float jz = ((float)GetRandomValue(-100, 100) / 100.0f) * grethnarJumpscareShake * 0.12f;

                    renderCam.position = Vector3Add(renderCam.position, Vector3{ jx, jy, jz });

                    renderCam.target   = Vector3Add(renderCam.target,   Vector3{ jx * 1.6f, jy * 1.6f, jz * 1.6f });

                }

            } else {

                renderCam.fovy = g_camDynamicFov;

            }

            

            // PLAYER ENTITY '@' (Scaled up & 3D Volumetric Extrusion - No items held in hands)

            float playerScale = 1.45f; // Bigger size

            Matrix m = MatrixIdentity();
            if (playerInShop) {
                Color pLit = ApplyShopLighting(camera.position, { 230, 230, 235, 255 });
                m.m0 = pLit.r / 255.0f;
                m.m1 = pLit.g / 255.0f;
                m.m2 = pLit.b / 255.0f;
                m.m9 = 1.0f; // Precomputed lighting: bypass directional sun/moon override
            } else {
                m.m0 = 1.0f; m.m1 = 1.0f; m.m2 = 1.0f;
                m.m9 = 0.0f; // Exterior lighting with silhouette ambient floor in shader
            }
            m.m3 = 1.0f;
            m.m4 = playerScale; m.m5 = playerScale;
            m.m8 = 3.0f; // Transparent entity (Player '@')
            m.m10 = 0.0f; // Billboard
            m.m11 = 1.0f;

            

            float pBobY = (fabs(cosf(walkTime / 2.0f)) - 0.5f) * 0.4f * bobAmplitude;

            m.m12 = camera.position.x;

            m.m13 = camera.position.y - 0.90f + pBobY;

            m.m14 = camera.position.z;

            

            // Physical tilt momentum

            if (isMoving) {

                Vector3 tilt = Vector3Scale(forwardBob, 0.4f);

                m.m12 += tilt.x; m.m14 += tilt.z;

            }



            // 3D Volumetric Extrusion: Multi-layered depth slices create a real 3D sculpted figure

            Vector3 camToP = Vector3Normalize(Vector3Subtract(camera.position, renderCam.position));

            float depthStep = 0.035f;

            for (int slice = -2; slice <= 2; slice++) {

                Matrix sm = m;

                sm.m12 += camToP.x * (slice * depthStep);

                sm.m13 += camToP.y * (slice * depthStep);

                sm.m14 += camToP.z * (slice * depthStep);

                

                // 3D bevel shading: center slice is bright, front and back are shaded for depth

                float shade = (slice == 0) ? 1.0f : (1.0f - abs(slice) * 0.18f);

                sm.m0 *= shade; sm.m1 *= shade; sm.m2 *= shade;

                playerInstances['@'].push_back(sm);

            }

            // Ground contact drop shadow for player entity

            DrawCircle3D(Vector3{ camera.position.x, 10.018f, camera.position.z }, 0.45f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 8, 8, 12, 185 });

        } else {

            if (g_lookAtShop) {
                renderCam.position = Vector3{ 104.5f, 12.0f, 135.2f };
                renderCam.target = Vector3{ 104.4f, 11.72f, 133.5f };
                renderCam.up = Vector3{ 0.0f, 1.0f, 0.0f };
            } else if (g_lookAtWashroom) {
                renderCam.position = Vector3{ 89.25f, 11.8f, 148.5f };
                renderCam.target = Vector3{ 89.25f, 11.6f, 158.0f };
                renderCam.up = Vector3{ 0.0f, 1.0f, 0.0f };
            } else if (g_lookAtAtm) {
                renderCam.position = Vector3{ 105.2f, 11.6f, 143.5f };
                renderCam.target = Vector3{ 107.45f, 11.4f, 143.5f };
                renderCam.up = Vector3{ 0.0f, 1.0f, 0.0f };
            } else if (g_gameState == STATE_MAIN_MENU) {

                Vector2 mPos = GetMousePosition();
                float mNormX = Clamp((mPos.x / (float)curWinW - 0.5f) * 2.0f, -1.0f, 1.0f);
                float mNormY = Clamp((mPos.y / (float)curWinH - 0.5f) * 2.0f, -1.0f, 1.0f);

                // Smooth organic spring-damping parallax
                g_menuCamSmoothX += (mNormX - g_menuCamSmoothX) * Clamp(dt * 3.8f, 0.0f, 1.0f);
                g_menuCamSmoothY += (mNormY - g_menuCamSmoothY) * Clamp(dt * 3.8f, 0.0f, 1.0f);

                // Organic pupil tracking for the horned bovine skull
                g_skullEyeSmoothX += (mNormX - g_skullEyeSmoothX) * Clamp(dt * 7.5f, 0.0f, 1.0f);
                g_skullEyeSmoothY += (mNormY - g_skullEyeSmoothY) * Clamp(dt * 7.5f, 0.0f, 1.0f);

                Vector3 basePos, baseTgt;
                GetMenuCCTVCamera(g_menuCCTVFeed, timeVal, g_menuCamSmoothX, g_menuCamSmoothY, basePos, baseTgt);

                // Cinematic camera dolly surge forward down the corridor when PLAY is activated
                float targetFov = g_userFov;
                if (g_isMenuStartingGame) {
                    float startProg = Clamp(g_menuPlayTransitionTimer / 0.70f, 0.0f, 1.0f);
                    float surge = startProg * startProg * startProg; // cubic ease-in surge
                    basePos.x -= surge * 6.2f;
                    baseTgt.y += surge * 0.45f;
                    targetFov = g_userFov - surge * 12.0f;
                }

                renderCam.position = basePos;
                renderCam.target   = baseTgt;
                renderCam.up       = Vector3{ 0.0f, 1.0f, 0.0f };
                renderCam.fovy     = targetFov;

            } else {

                renderCam = camera;

                renderCam.fovy = g_camDynamicFov;

                renderCam.position = Vector3Add(renderCam.position, bobOffset);

                renderCam.target = Vector3Add(renderCam.target, bobOffset);

                if (g_lookAtSun) {
                    Vector3 camFwd = (sunElev < -0.05f) ? moonDir : sunDir;
                    Vector3 refUp = (fabsf(camFwd.y) > 0.90f) ? Vector3{ 0.0f, 0.0f, -1.0f } : Vector3{ 0.0f, 1.0f, 0.0f };
                    Vector3 camR = Vector3Normalize(Vector3CrossProduct(camFwd, refUp));
                    renderCam.target = Vector3Add(renderCam.position, Vector3Scale(camFwd, 20.0f));
                    renderCam.up = Vector3Normalize(Vector3CrossProduct(camR, camFwd));
                } else if (g_lookAtGround) {
                    renderCam.target = Vector3Add(renderCam.position, Vector3{ 8.0f, -4.5f, 14.0f });
                    renderCam.up = Vector3{ 0.0f, 1.0f, 0.0f };
                } else {
                    if (g_waterState == WATER_STATE_SURFACE) {
                        renderCam.target.y += g_waterSmoothTiltPitch;
                    }
                    renderCam.up = rollUp;
                }

            }



            // Apply Grethnar Jumpscare Zoom & Screen Shake

            if (grethnarState == GRETHNAR_JUMPSCARE) {

                renderCam.fovy = grethnarJumpscareFov;

                if (grethnarJumpscareShake > 0.005f) {

                    float jx = ((float)GetRandomValue(-100, 100) / 100.0f) * grethnarJumpscareShake * 0.18f;

                    float jy = ((float)GetRandomValue(-100, 100) / 100.0f) * grethnarJumpscareShake * 0.16f;

                    float jz = ((float)GetRandomValue(-100, 100) / 100.0f) * grethnarJumpscareShake * 0.12f;

                    renderCam.position = Vector3Add(renderCam.position, Vector3{ jx, jy, jz });

                    renderCam.target   = Vector3Add(renderCam.target,   Vector3{ jx * 1.6f, jy * 1.6f, jz * 1.6f });

                }

            } else {

                renderCam.fovy = g_camDynamicFov;

            }

        }



        // MR. GRETHNAR EYE-BLOOD: Hyper-realistic blood drops made of '~' characters dripping to ground

        for (const auto& bd : grethnarBloodDrops) {

            Matrix mb = MatrixIdentity();

            mb.m0 = 175.0f / 255.0f; // Deep arterial crimson red

            mb.m1 = 6.0f / 255.0f;

            mb.m2 = 12.0f / 255.0f;

            mb.m3 = Clamp(bd.life / 0.6f, 0.0f, 1.0f);

            mb.m4 = bd.scale;

            mb.m5 = bd.scale;

            mb.m8 = 0.0f;

            mb.m9 = 0.0f;

            mb.m10 = 0.0f; // Billboard

            mb.m11 = 1.0f;

            mb.m12 = bd.pos.x;

            mb.m13 = bd.pos.y;

            mb.m14 = bd.pos.z;

            playerInstances['~'].push_back(mb);

        }



        

        // ----------------------------------------------------

        // THUNDERSTORM LOGIC

        // ----------------------------------------------------

        if (hitStopTimer <= 0.0f) {
            if (isStormActive || g_gameState == STATE_MAIN_MENU) {
                if (isStormActive) stormDuration -= dt;

                if (g_gameState == STATE_MAIN_MENU) {
                    SetSoundVolume(sndRain, 0.30f);
                } else {
                    SetSoundVolume(sndRain, 0.70f);
                }
                if (!IsSoundPlaying(sndRain)) PlaySound(sndRain);

                

                // Lightning Flashes

                if (lightningFlashTimer > 0.0f) {

                    lightningFlashTimer -= dt * 3.0f;

                    if (lightningFlashTimer < 0.0f) lightningFlashTimer = 0.0f;

                } else if (GetRandomValue(0, 1000) < 5) { // Random chance to flash

                    lightningFlashTimer = 1.0f;

                    PlaySound(sndThunder);

                    // Ground impact flash

                    float rDist = GetRandomValue(10, 40);

                    float rAng = GetRandomValue(0, 360) * DEG2RAD;

                    groundImpactPos.x = camera.position.x + cosf(rAng) * rDist;

                    groundImpactPos.z = camera.position.z + sinf(rAng) * rDist;

                    groundImpactPos.y = camera.position.y;

                    groundImpactTimer = 0.5f;

                    

                    lightningSegments.clear();

                    Vector3 skyStart = groundImpactPos;

                    skyStart.y += 100.0f; // High up in the sky

                    GenerateLightningBolt(skyStart, groundImpactPos, 5, lightningSegments);

                }

                

                if (groundImpactTimer > 0.0f) {

                    groundImpactTimer -= dt * 2.0f;

                }

                

                // Spawn rain particles (Memory Efficient Cone)

                for (int p = 0; p < 12; p++) {

                    RainParticle rp;

                    float dist = GetRandomValue(100, 1800) / 100.0f; // up to 18 blocks away

                    float angle = (GetRandomValue(-70, 70)) * DEG2RAD; // 140 degree cone

                    

                    float cosA = cosf(angle);

                    float sinA = sinf(angle);

                    Vector3 refFwd = (g_gameState == STATE_MAIN_MENU) ? Vector3Normalize(Vector3Subtract(renderCam.target, renderCam.position)) : forwardBob;
                    if (Vector3Length(refFwd) < 0.01f) refFwd = Vector3{ 0.0f, 0.0f, 1.0f };
                    float dx = refFwd.x * cosA - refFwd.z * sinA;
                    float dz = refFwd.x * sinA + refFwd.z * cosA;
                    Vector3 rainCenter = (g_gameState == STATE_MAIN_MENU) ? renderCam.position : camera.position;

                    rp.pos.x = rainCenter.x + dx * dist;
                    rp.pos.z = rainCenter.z + dz * dist;
                    rp.pos.y = rainCenter.y + 12.0f + (GetRandomValue(0, 50)/10.0f);
                    rp.life = 1.0f;
                    rp.glyph = (GetRandomValue(0, 1) == 0) ? '|' : ',';
                    rainParticles.push_back(rp);

                }

                

                // Update rain

                for (int i = 0; i < (int)rainParticles.size(); ) {

                    rainParticles[i].pos.y -= dt * 45.0f; // fall fast

                    rainParticles[i].life -= dt * 1.2f;

                    

                    // Precise Terrain Collision Culling

                    bool hitGround = false;

                    int vx = (int)roundf(rainParticles[i].pos.x);

                    int vy = (int)roundf(rainParticles[i].pos.y);

                    int vz = (int)roundf(rainParticles[i].pos.z);

                    if (vx >= 0 && vx < CHUNK_W && vy >= 0 && vy < CHUNK_H && vz >= 0 && vz < CHUNK_D) {

                        if (chunk->voxels[vx][vy][vz].isSolid) hitGround = true;

                    } else if (vy < 0) {

                        hitGround = true;

                    }



                    if (rainParticles[i].life <= 0.0f || hitGround) {

                        rainParticles[i] = rainParticles.back();

                        rainParticles.pop_back();

                    } else {

                        i++;

                    }

                }

                

                if (stormDuration <= 0.0f) {

                    isStormActive = false;

                    StopSound(sndRain);

                    nextStormEventTimer = GetRandomValue(20, 60); // Random storm cadence after

                }

            } else {

                nextStormEventTimer -= dt;

                if (nextStormEventTimer <= 0.0f) {

                    isStormActive = true;

                    stormDuration = (float)GetRandomValue(15, 30); // 15-30s storm

                }

                if (IsSoundPlaying(sndRain)) StopSound(sndRain);

                lightningFlashTimer = 0.0f;

                groundImpactTimer = 0.0f;

                rainParticles.clear();

            }

        }

        

        // Build rain instances (suppressed underwater for 144 FPS and realism)
        bool isUnderwaterScene = (camera.position.y < 9.75f && camera.position.x <= 36.0f);

        for(int i=0; i<256; i++) rainInstances[i].clear();

        if (!isUnderwaterScene) {
            for (const auto& rp : rainParticles) {

                Matrix m = MatrixIdentity();

                m.m0 = 150/255.0f; m.m1 = 180/255.0f; m.m2 = 255/255.0f; m.m3 = rp.life * 0.7f;

                m.m4 = 0.5f; m.m5 = 0.5f;

                m.m9 = 0.0f; // Enable fog

                m.m10 = 0.0f; // Billboard

                m.m11 = 1.0f;

                m.m12 = rp.pos.x; m.m13 = rp.pos.y; m.m14 = rp.pos.z;

                rainInstances[(uint8_t)rp.glyph].push_back(m);

            }

            

            // Ground impact instance

            if (groundImpactTimer > 0.0f) {

                Matrix m = MatrixIdentity();

                m.m0 = 1.0f; m.m1 = 1.0f; m.m2 = 1.0f; m.m3 = groundImpactTimer * 0.8f;

                m.m4 = 3.0f; m.m5 = 3.0f;

                m.m9 = 1.0f; // disable fog

                m.m10 = 0.0f; // billboard

                m.m11 = 1.0f;

                m.m12 = groundImpactPos.x; m.m13 = groundImpactPos.y; m.m14 = groundImpactPos.z;

                rainInstances['*'].push_back(m);

            }
        }



        // ----------------------------------------------------

        // MEADOW WAKE TRACKER

        // ----------------------------------------------------

        for (int i = 0; i < 16; i++) {

            if (trail[i].life > 0.0f) {

                trail[i].life -= dt * 0.25f; // Heals completely over 4 seconds

                if (trail[i].life < 0.0f) trail[i].life = 0.0f;

            }

        }

        

        // Drop a new footprint when the player moves 1.2 units on the ground

        if (onGround && Vector3Distance(camera.position, lastTrailPos) > 1.2f) {

            trail[trailIndex].pos = camera.position;

            trail[trailIndex].life = 1.0f;

            lastTrailPos = camera.position;

            trailIndex = (trailIndex + 1) % 16;

        }



        Vector3 shaderTrailPos[16];

        float shaderTrailLife[16];

        for (int i = 0; i < 16; i++) {

            shaderTrailPos[i] = trail[i].pos;

            shaderTrailLife[i] = trail[i].life;

        }



        // ----------------------------------------------------

        // RENDER TO FRAMEBUFFER

        // ----------------------------------------------------

        // Update physical in-world 3D Fuel Pump CRT screen texture
        if (g_pumpScreenRTLoaded) {
            int curPumpNum = (g_activePumpIndex != -1) ? (g_activePumpIndex + 1) : ((g_customerCar.state != CAR_INACTIVE) ? (g_customerCar.targetPump + 1) : 1);
            float dispGal  = (g_customerCar.state == CAR_REFUELING || g_customerCar.state == CAR_PAID) ? g_customerCar.dispensedGallons : 0.0f;
            float dispSale = dispGal * g_fuelPricePerGallon;
            bool isPumping = (g_customerCar.state == CAR_REFUELING && (IsKeyDown(KEY_E) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)));
            float crtFlk   = fluorLightOn ? 1.0f : 0.15f;
            UpdatePumpCrtTexture(curPumpNum, dispGal, dispSale, isPumping, crtFlk);
        }

        // Update Haunted Washroom Real Planar Reflection Mirror Pre-pass (BEFORE BeginTextureMode(target) to prevent FBO conflict!)
        Camera3D shopEvalCam = (g_lookAtShop || g_lookAtWashroom || g_lookAtAtm || isThirdPerson) ? renderCam : camera;
        UpdateShopWashroomMirror(shopEvalCam, ApplyShopLighting, g_shopLightsOn, timeVal);

        // Update Midnight Security Monitor (CCTV CRT) Pre-pass
        UpdateCounterCCTV(shopEvalCam, ApplyShopLighting, g_shopLightsOn, timeVal);

        // Render Physically-Based Atmospheric Sky & Volumetric Clouds with Temporal Reconstruction
        RenderAtmosphericSkyAndClouds(renderCam, sunTheta, sunDir, sunElev, lightningFlashTimer, timeVal, isUnderwaterScene);

        BeginTextureMode(target);

        // Dynamic Celestial Atmospheric Sky Clearing (Bright realistic daylight in day, twilight in dusk, obsidian at night)
        float skyClearDay = Clamp((sunElev + 0.10f) / 0.35f, 0.0f, 1.0f);
        float skyClearTwi = Clamp(1.0f - fabsf(sunElev - 0.04f) / 0.16f, 0.0f, 1.0f);
        float skyClearNight = Clamp((-sunElev - 0.04f) / 0.22f, 0.0f, 1.0f);
        float sumSkyW = skyClearDay + skyClearTwi + skyClearNight;
        if (sumSkyW > 0.001f) { skyClearDay /= sumSkyW; skyClearTwi /= sumSkyW; skyClearNight /= sumSkyW; }
        Color baseSkyClear = {
            (unsigned char)Clamp(65.0f * skyClearDay + 35.0f * skyClearTwi + 4.0f * skyClearNight, 0.0f, 255.0f),
            (unsigned char)Clamp(145.0f * skyClearDay + 28.0f * skyClearTwi + 6.0f * skyClearNight, 0.0f, 255.0f),
            (unsigned char)Clamp(235.0f * skyClearDay + 55.0f * skyClearTwi + 14.0f * skyClearNight, 0.0f, 255.0f),
            255
        };

        CompositeAtmosphericSkyToTarget(target, isUnderwaterScene, baseSkyClear);

        

        BeginMode3D(renderCam);

        

        // 3D Atmospheric Rayleigh/Mie Scattering Sky Dome & Celestial Corona

        DrawAtmosphericSkyDome(renderCam, sunTheta, lightningFlashTimer);

        Vector2 uvScale = {1.0f / 16.0f, 1.0f / 16.0f};

        SetShaderValue(instancedShader, uvScaleLoc, &uvScale, SHADER_UNIFORM_VEC2);

        SetShaderValue(instancedShader, timeLoc, &timeVal, SHADER_UNIFORM_FLOAT);

        SetShaderValue(instancedShader, playerPosLoc, &camera.position, SHADER_UNIFORM_VEC3);

        SetShaderValueV(instancedShader, trailPosLoc, shaderTrailPos, SHADER_UNIFORM_VEC3, 16);

        SetShaderValueV(instancedShader, trailLifeLoc, shaderTrailLife, SHADER_UNIFORM_FLOAT, 16);

        SetShaderValue(instancedShader, lightningFlashLoc, &lightningFlashTimer, SHADER_UNIFORM_FLOAT);

        float rawDayFactor = Clamp((sunElev + 0.10f) / 0.35f, 0.0f, 1.0f);

        float twiFactor = Clamp(1.0f - fabsf(sunElev - 0.04f) / 0.16f, 0.0f, 1.0f);



        // Real-world physics: When Sun or Moon passes behind a cloud, direct beam dims by optical extinction!

        float celestialSunOcc = GetCelestialCloudOcclusion(renderCam, sunDir);

        Vector3 curMoonDirWorld = Vector3Negate(sunDir);

        float celestialMoonOcc = GetCelestialCloudOcclusion(renderCam, curMoonDirWorld);



        float dayFactor = rawDayFactor * (1.0f - celestialSunOcc * 0.72f);

        float nightFactorExt = nightFactor * (1.0f - celestialMoonOcc * 0.72f);

        Vector3 sunColorVec = {

            Clamp(1.0f + twiFactor * 0.15f, 0.0f, 1.15f),

            Clamp(0.96f - twiFactor * 0.35f, 0.0f, 1.0f),

            Clamp(0.85f - twiFactor * 0.55f, 0.0f, 1.0f)

        };

        SetShaderValue(instancedShader, sunDirLoc, &sunDir, SHADER_UNIFORM_VEC3);

        SetShaderValue(instancedShader, dayFactorLoc, &dayFactor, SHADER_UNIFORM_FLOAT);

        SetShaderValue(instancedShader, sunColorLoc, &sunColorVec, SHADER_UNIFORM_VEC3);

        Vector3 curMoonDir = Vector3Negate(sunDir);

        SetShaderValue(instancedShader, moonDirLoc, &curMoonDir, SHADER_UNIFORM_VEC3);

        SetShaderValue(instancedShader, nightFactorLoc, &nightFactor, SHADER_UNIFORM_FLOAT);

        

        // --- SPATIAL & FRUSTUM BUCKET CULLING (MASSIVE FPS BOOST) ---

        // Pre-calculate which buckets are within a visible radius of the camera, and in front of it.

        static std::vector<RenderBucket*> visibleBuckets;

        static bool s_mainBucketsInit = false;

        if (!s_mainBucketsInit) {

            visibleBuckets.reserve(BUCKETS_X * BUCKETS_Z);

            s_mainBucketsInit = true;

        }

        visibleBuckets.clear();

        float cullDistSq = 65.0f * 65.0f; // 65 block render distance

        

        Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

        camForward.y = 0.0f;

        camForward = Vector3Normalize(camForward);

        

        float activeCullDistSq = cullDistSq;



        for (int bx = 0; bx < BUCKETS_X; bx++) {

            for (int bz = 0; bz < BUCKETS_Z; bz++) {

                float centerX = bx * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);

                float centerZ = bz * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);

                float dx = centerX - camera.position.x;

                float dz = centerZ - camera.position.z;

                float distSq = dx*dx + dz*dz;

                

                if (distSq < activeCullDistSq) {

                    if (distSq < (BUCKET_SIZE * BUCKET_SIZE)) {

                        visibleBuckets.push_back(&chunk->buckets[bx][bz]);

                    } else {

                        Vector3 dirToBucket = Vector3Normalize({dx, 0.0f, dz});

                        float dotProd = Vector3DotProduct(camForward, dirToBucket);

                        if (dotProd > -0.4f) { // ~113 degree field of view on either side, perfect for 16:9

                            visibleBuckets.push_back(&chunk->buckets[bx][bz]);

                        }

                    }

                }

            }

        }

        

        // BATCHED INSTANCED RENDER PASS: Collapses 800+ draw calls down to ~25 calls per frame!

        static std::vector<Matrix> batchTransforms;

        static bool s_mainBatchInit = false;

        if (!s_mainBatchInit) {

            batchTransforms.reserve(65536);

            s_mainBatchInit = true;

        }

        static bool s_mainGlyphUsed[256];

        memset(s_mainGlyphUsed, 0, sizeof(s_mainGlyphUsed));

        for (RenderBucket* b : visibleBuckets) {

            for (uint8_t g : b->activeGlyphs) {

                s_mainGlyphUsed[g] = true;

            }

        }

        // Populate curated ASCII cloud letters with camera frustum culling (only exist above water)
        if (!isUnderwaterScene) {
            PopulateCloudInstances(renderCam, sunDir, sunElev, lightningFlashTimer, cloudInstances, timeVal);
        } else {
            for (int i = 0; i < 256; i++) cloudInstances[i].clear();
        }



        for (int i = 0; i < 256; i++) {

            if (!debrisInstances[i].empty() || !starInstances[i].empty() || !moonInstances[i].empty() ||

                !playerInstances[i].empty() || (!isUnderwaterScene && !rainInstances[i].empty()) || !cloudInstances[i].empty()) {

                s_mainGlyphUsed[i] = true;

            }

        }



        for(int i=0; i<256; i++) {

            if (!s_mainGlyphUsed[i]) continue;



            bool hasDebris = !isUnderwaterScene && !debrisInstances[i].empty();

            bool hasStar   = !isUnderwaterScene && !starInstances[i].empty();

            bool hasMoon   = !isUnderwaterScene && !moonInstances[i].empty();

            bool hasPlayer = !playerInstances[i].empty();

            bool hasRain   = !isUnderwaterScene && !rainInstances[i].empty();

            bool hasCloud  = !isUnderwaterScene && !cloudInstances[i].empty();

            

            int col = i % 16;

            int row = i / 16;

            Vector2 uvOffset = {col * uvScale.x, row * uvScale.y};

            SetShaderValue(instancedShader, uvOffsetLoc, &uvOffset, SHADER_UNIFORM_VEC2);

            

            // Consolidate all bucket instances and dynamic actors into one buffer

            batchTransforms.clear();

            for (RenderBucket* b : visibleBuckets) {

                if (!b->instances[i].empty()) {

                    batchTransforms.insert(batchTransforms.end(), b->instances[i].begin(), b->instances[i].end());

                }

            }

            if (hasDebris) batchTransforms.insert(batchTransforms.end(), debrisInstances[i].begin(), debrisInstances[i].end());

            if (hasStar)   batchTransforms.insert(batchTransforms.end(), starInstances[i].begin(), starInstances[i].end());

            if (hasMoon)   batchTransforms.insert(batchTransforms.end(), moonInstances[i].begin(), moonInstances[i].end());

            if (hasPlayer) batchTransforms.insert(batchTransforms.end(), playerInstances[i].begin(), playerInstances[i].end());

            if (hasRain)   batchTransforms.insert(batchTransforms.end(), rainInstances[i].begin(), rainInstances[i].end());

            if (hasCloud)  batchTransforms.insert(batchTransforms.end(), cloudInstances[i].begin(), cloudInstances[i].end());



            // Single Draw Call per unique character!

            if (!batchTransforms.empty()) {

                DrawMeshInstanced(quad, material, batchTransforms.data(), (int)batchTransforms.size());

            }

        }

        

        

        // 3D Player Ground Contact Shadow

        if (isThirdPerson || isRoofCamActive) {

            DrawCircle3D({ camera.position.x, 10.02f, camera.position.z }, 0.45f, { 1.0f, 0.0f, 0.0f }, 90.0f, { 10, 12, 16, 140 });

        }

        

        // Draw 3D Volumetric Clouds (Optimized 3D Puffs & Wisps)

        DrawHybridCloudVolumes(renderCam, sunDir, sunElev, lightningFlashTimer, timeVal);

        

        // Draw Multi-tier Ionized Plasma Lightning Bolt

        DrawPlasmaLightningBolt(lightningSegments, lightningFlashTimer, groundImpactPos, groundImpactTimer);

        

                // =========================================================================

        // GAS STATION IN THE CENTER OF THE ROAD DIVIDING ROAD INTO TWO EQUAL LANES

        // Center: X = 128.0, Z = 134..146 (Left Lane: 117..125, Right Lane: 131..139)

        // =========================================================================

        // Dynamic Exterior Illuminator: Driven strictly by Sun (Day) & Moon (Night)

        float extDayFactor = Clamp((sunElev + 0.10f) / 0.35f, 0.0f, 1.0f);

        float extNightFactor = nightFactorExt;

        float extTwiFactor = Clamp(1.0f - fabsf(sunElev - 0.04f) / 0.16f, 0.0f, 1.0f);

        auto ApplyExteriorDaylight = [&](Color baseColor, float verticalBias = 1.0f) -> Color {

            // Direct sunlight contribution

            float sunLightVal = fmaxf(0.0f, sunDir.y) * verticalBias * extDayFactor;

            // Direct moonlight contribution (illuminating from opposite celestial direction)

            Vector3 extMDir = Vector3Negate(sunDir);

            float moonLightVal = fmaxf(0.0f, extMDir.y) * verticalBias * extNightFactor;



            // Direct light from Sun (warm) and Moon (cool silver)

            float sunR = 1.25f * sunLightVal;

            float sunG = 1.15f * sunLightVal;

            float sunB = 0.95f * sunLightVal;



            float moonR = 0.22f * moonLightVal;

            float moonG = 0.32f * moonLightVal;

            float moonB = 0.55f * moonLightVal;



            // Horizon twilight glow

            float twiR = extTwiFactor * 0.28f;

            float twiG = extTwiFactor * 0.16f;

            float twiB = extTwiFactor * 0.10f;



            // Pure physical ambient: near pitch darkness at night (0.04) vs daylight ambient (0.42)

            float ambR = 0.035f + extDayFactor * 0.42f + extNightFactor * 0.045f;

            float ambG = 0.040f + extDayFactor * 0.42f + extNightFactor * 0.055f;

            float ambB = 0.065f + extDayFactor * 0.42f + extNightFactor * 0.085f;



            float rMul = ambR + sunR + moonR + twiR;

            float gMul = ambG + sunG + moonG + twiG;

            float bMul = ambB + sunB + moonB + twiB;



            return Color{

                (unsigned char)Clamp(baseColor.r * rMul, 0.0f, 255.0f),

                (unsigned char)Clamp(baseColor.g * gMul, 0.0f, 255.0f),

                (unsigned char)Clamp(baseColor.b * bMul, 0.0f, 255.0f),

                baseColor.a

            };

        };

        float distGasSq = (128.0f - camera.position.x)*(128.0f - camera.position.x) + (140.0f - camera.position.z)*(140.0f - camera.position.z);

        if (isRoofCamActive || distGasSq < 85.0f * 85.0f)

        {

            float flk = fluorLightOn ? 1.0f : 0.05f;



            // 1. Central Concrete Pump Island in the middle of the road (X: 126..130, Z: 133..147)

            DrawCube({ 128.0f, 10.20f, 140.0f }, 4.2f, 0.40f, 14.0f, ApplyExteriorDaylight({ 45, 42, 38, 255 }, 1.0f));

            DrawCubeWires({ 128.0f, 10.20f, 140.0f }, 4.25f, 0.42f, 14.05f, ApplyExteriorDaylight({ 85, 75, 65, 255 }, 1.0f));



            // 2. Overhead Canopy over Central Island (X: 128.0, Z: 140.0, Y: 15.2)

            DrawCube({ 128.0f, 15.2f, 140.0f }, 6.5f, 0.35f, 13.5f, ApplyExteriorDaylight({ 42, 38, 35, 255 }, 1.0f));

            DrawCubeWires({ 128.0f, 15.2f, 140.0f }, 6.5f, 0.35f, 13.5f, { 60, 32, 16, 255 }); // Rusted trim


                // 5. UNDERGROUND FUEL TANK INSPECTION MANHOLES (Class 3 Flammable, 10,000 Gal Reservoir)
                float manholeZs[2] = { 137.5f, 142.5f };
                for (int m = 0; m < 2; m++) {
                    Vector3 mhPos = { 130.5f, 10.025f, manholeZs[m] };
                    DrawCylinder(mhPos, 0.44f, 0.44f, 0.015f, 14, Color{ 32, 30, 28, 255 });
                    DrawCylinder(Vector3{ mhPos.x, mhPos.y + 0.016f, mhPos.z }, 0.12f, 0.12f, 0.02f, 10, Color{ 180, 145, 55, 255 });
                    DrawCircle3D(mhPos, 0.46f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 220, 180, 35, 180 });
                }

                // 6. ATTENDANT SERVICE BELL ON CENTRAL PILLAR (Z = 140.0)
                Vector3 bellPos = { 127.72f, 11.45f, 140.0f };
                DrawSphere(bellPos, 0.065f, Color{ 215, 175, 55, 255 });
                DrawCylinder(Vector3{ bellPos.x, 11.38f, bellPos.z }, 0.08f, 0.08f, 0.02f, 12, Color{ 45, 42, 38, 255 });

                // 7. 3D CUSTOMER CAR ON ROUTE 9
                DrawCustomerCar(g_customerCar, g_nozzleInCar);

                // 8. NOZZLE HELD IN PLAYER HANDS
                if (g_holdingFuelNozzle && !g_nozzleInCar && g_activePumpIndex != -1) {
                    float pz = (g_activePumpIndex == 0) ? 137.5f : 142.5f;
                    Vector3 pumpOutlet = { 127.42f, 11.2f, pz - 0.25f };
                    Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
                    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(fwd, camera.up));
                    Vector3 camUp = Vector3Normalize(Vector3CrossProduct(camRight, fwd));
                    float stepBobY = (walkTime > 0.0f) ? sinf(walkTime * 2.0f) * 0.008f : 0.0f;
                    float breathe  = sinf(timeVal * 1.8f) * 0.004f;
                    Vector3 handPos = Vector3Add(camera.position,
                        Vector3Add(Vector3Scale(camRight, 0.22f),
                        Vector3Add(Vector3Scale(camUp, -0.20f + stepBobY + breathe),
                        Vector3Scale(fwd, 0.40f))));
                    Vector3 handleBot = Vector3Add(handPos, Vector3Add(Vector3Scale(camUp, -0.12f), Vector3Scale(fwd, -0.06f)));
                    Vector3 hoseCoupling = Vector3Add(handleBot, Vector3Scale(camUp, -0.025f));

                    // Heavy catenary rubber hose from pump outlet to nozzle handle coupling
                    DrawCatenaryHose(pumpOutlet, hoseCoupling, 0.90f, 16, 0.026f, Color{ 16, 16, 18, 255 });

                    // Draw first-person nozzle viewmodel if not in roof cam
                    if (!isRoofCamActive) {
                        bool isFlowing = (g_customerCar.state == CAR_REFUELING && (IsKeyDown(KEY_E) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)));
                        DrawFirstPersonFuelNozzle(camera, isFlowing, walkTime, timeVal);
                    }
                }




            // 3. Canopy Support Pillars along the central island

            float pZs[2] = { 135.0f, 145.0f };

            for (int p = 0; p < 2; p++) {

                DrawCube({ 128.0f, 12.6f, pZs[p] }, 0.45f, 5.2f, 0.45f, { 30, 28, 25, 255 });

                DrawCube({ 128.0f, 10.6f, pZs[p] }, 0.50f, 1.2f, 0.50f, { 75, 32, 14, 255 }); // Rust footing

            }



            // 4. Dual Fuel Pumps on Central Island (Services both Left Lane and Right Lane)
            float pumpZs[2] = { 137.5f, 142.5f };
            for (int pi = 0; pi < 2; pi++) {
                float pz = pumpZs[pi];
                // Heavy pump body (weathered cream enamel with red lower skirt)
                DrawCube({ 128.0f, 11.4f, pz }, 1.15f, 2.4f, 0.85f, ApplyExteriorDaylight({ 55, 52, 46, 255 }, 1.0f));
                DrawCube({ 128.0f, 10.4f, pz }, 1.18f, 0.40f, 0.88f, ApplyExteriorDaylight({ 85, 22, 20, 255 }, 1.0f)); // Red enamel skirt
                DrawCube({ 128.0f, 12.65f, pz }, 1.10f, 0.22f, 0.80f, ApplyExteriorDaylight({ 24, 22, 20, 255 }, 1.0f)); // Top crown

                // Retro Backlit CRT Meter Screens on BOTH sides (Left Lane at X=127.40, Right Lane at X=128.60)
                float screenXs[2] = { 127.40f, 128.60f };
                for (int s = 0; s < 2; s++) {
                    float sx = screenXs[s];
                    // Outer Bezel
                    DrawCube({ sx, 11.75f, pz }, 0.035f, 0.72f, 0.62f, { 18, 20, 22, 255 });
                    // CRT Screen glass backing (dark glowing emerald green)
                    Color crtCol = fluorLightOn ? Color{ 6, 26, 12, 255 } : Color{ 2, 8, 4, 255 };
                    DrawCube({ sx, 11.75f, pz }, 0.040f, 0.64f, 0.54f, crtCol);
                    
                    // In-World 3D Textured CRT Screen
                    if (g_pumpScreenRTLoaded) {
                        float screenXOffset = (s == 0) ? -0.023f : 0.023f;
                        Color crtTint = fluorLightOn ? WHITE : Color{ 85, 95, 90, 255 };
                        DrawPumpCrtScreen3D({ sx + screenXOffset, 11.75f, pz }, 0.58f, 0.44f, g_pumpScreenRT.texture, (s == 0), crtTint);
                    }

                    // Mechanical Flow Sight-Glass with Spinning Red Turbine
                    Vector3 sightGlassPos = { sx, 11.35f, pz + 0.18f };
                    DrawCylinder(sightGlassPos, 0.045f, 0.045f, 0.08f, 10, { 180, 220, 210, 140 }); // Glass dome
                    bool isFlowing = (g_customerCar.state == CAR_REFUELING && (IsKeyDown(KEY_E) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)));
                    float turbAngle = isFlowing ? (timeVal * 720.0f) : 0.0f;
                    rlPushMatrix();
                    rlTranslatef(sightGlassPos.x, sightGlassPos.y + 0.04f, sightGlassPos.z);
                    rlRotatef(turbAngle, 0.0f, 1.0f, 0.0f);
                    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 0.06f, 0.03f, 0.015f, { 220, 25, 20, 255 }); // Red flow impeller
                    rlPopMatrix();
                }

                // Left Lane Nozzle Cradle & Heavy Rubber Hose (West face, X = 127.42)
                bool leftCradleOccupied = (!g_holdingFuelNozzle && !g_nozzleInCar) || (g_activePumpIndex != pi && (g_customerCar.state == CAR_INACTIVE || g_customerCar.targetPump != pi));
                if (leftCradleOccupied) {
                    DrawCube({ 127.42f, 11.1f, pz - 0.25f }, 0.08f, 0.18f, 0.12f, { 18, 18, 18, 255 });
                    DrawCube({ 127.38f, 11.16f, pz - 0.25f }, 0.06f, 0.14f, 0.07f, { 70, 75, 82, 255 });
                    Vector3 pumpOutlet = { 127.42f, 11.2f, pz - 0.25f };
                    Vector3 cradleBase = { 127.42f, 10.3f, pz - 0.25f };
                    DrawCatenaryHose(pumpOutlet, cradleBase, 0.35f, 8, 0.024f, Color{ 16, 16, 18, 255 });
                }
                // Right Lane Nozzle Cradle & Heavy Rubber Hose (East face, X = 128.58)
                DrawCube({ 128.58f, 11.1f, pz + 0.25f }, 0.08f, 0.18f, 0.12f, { 18, 18, 18, 255 });
                DrawCube({ 128.62f, 11.16f, pz + 0.25f }, 0.06f, 0.14f, 0.07f, { 70, 75, 82, 255 });
                Vector3 pumpOutletR = { 128.58f, 11.2f, pz + 0.25f };
                Vector3 cradleBaseR = { 128.58f, 10.3f, pz + 0.25f };
                DrawCatenaryHose(pumpOutletR, cradleBaseR, 0.35f, 8, 0.024f, Color{ 16, 16, 18, 255 });
            }


            // Customer car fuel vapor shimmer & exhaust smoke feedback
            if (g_customerCar.state == CAR_REFUELING && (IsKeyDown(KEY_E) || IsMouseButtonDown(MOUSE_BUTTON_LEFT))) {
                Vector3 flapPos = { g_customerCar.pos.x + 0.98f, g_customerCar.pos.y + 0.85f, g_customerCar.pos.z - 0.85f };
                for (int v = 0; v < 3; v++) {
                    float vy = flapPos.y + (float)v * 0.12f + sinf(timeVal * 12.0f + (float)v) * 0.05f;
                    float vz = flapPos.z + cosf(timeVal * 8.0f + (float)v) * 0.06f;
                    DrawCube(Vector3{ flapPos.x, vy, vz }, 0.06f, 0.06f, 0.06f, Color{ 200, 200, 190, 45 }); // Vapor shimmer
                }
            } else if (g_customerCar.state == CAR_DEPARTING) {
                Vector3 exhaustPos = { g_customerCar.pos.x - 0.70f, 10.35f, g_customerCar.pos.z - 2.2f };
                for (int ex = 0; ex < 4; ex++) {
                    float exZ = exhaustPos.z - (float)ex * 0.45f;
                    float exY = exhaustPos.y + (float)ex * 0.14f;
                    DrawSphere(Vector3{ exhaustPos.x, exY, exZ }, 0.12f + (float)ex * 0.08f, Color{ 45, 45, 48, (unsigned char)(140 - ex * 30) });
                }
            }

            // 5. Forecourt Oil Slicks on Asphalt in both lanes

            DrawCube({ 121.5f, 10.02f, 140.0f }, 2.8f, 0.01f, 4.5f, { 8, 16, 10, 170 }); // Left Lane

            DrawCube({ 134.5f, 10.02f, 140.0f }, 2.8f, 0.01f, 4.5f, { 8, 16, 10, 170 }); // Right Lane



            // 6. Central Roadside Sign (RUSTY OIL CO.) at Median X: 128.0, Z: 131.0

            DrawCube({ 128.0f, 13.0f, 131.0f }, 0.25f, 6.0f, 0.25f, { 32, 28, 24, 255 });

            DrawCube({ 128.0f, 16.4f, 131.0f }, 3.6f, 1.8f, 0.20f, { 18, 12, 10, 255 });

            Color signNeon = fluorLightOn ? Color{ (unsigned char)(190 * flk), 10, 20, 255 } : Color{ 25, 0, 5, 255 };

            DrawCubeWires({ 128.0f, 16.4f, 131.0f }, 3.65f, 1.85f, 0.24f, signNeon);



            // =====================================================================
            // 7. FRONT ROOF WALL GUIDE RAIL & CREEPING CCTV BOX CAMERA
            // Mounted on the front roof edge wall. Appears fully when viewed from outside,
            // but hidden while looking through it so you see the pure world!
            // =====================================================================
            DrawPhysicalCCTVCameraAssembly(timeVal);

        }



        // =========================================================================

        // STANDALONE HORROR SUPERSTORE & SURROUNDING LOT (100% REAL SHAPES, 0% ASCII)

        // Exterior: Open surrounding apron lot with curbs, gutters, puddles, lamp posts.

        // Standalone Building: Weathered exterior shell, roof fascia, glazed display windows,

        //   interactive entrance door with peephole and latch.

        // Interior: Real 3D walls (West, North, South, East), baseboards, crown molding,

        //   restroom and meat locker doors with blood seepage, steel trusses, ceiling slab.

        // Floor & Blood: Checkered tile slab, pooled blood decals, smeared drag marks.

        // Shelving: Stark white commercial supermarket gondolas with dust & crimson blood drips.

        // Items & Tags: 16 items with 3D models and fixed rectangular name tags mounted directly below each item.

        // Shrine & Counter: Glass Jar of Donkey Milk shrine, reach-in cooler, checkout counter,

        //   CRT monitor with phosphor scanlines, and Mr. Grethnar Woule.

        // =========================================================================

        {

            // ---------------------------------------------------------------------

            // 0. CRASHED SILVER SEDAN & ROADSIDE WRECK (Mile Marker 14, East Verge: X = 143.8, Z = 136.5)
            DrawCrashedSedan(g_crashedCarPos, timeVal, extDayFactor, extNightFactor, camera);

            // Contact drop shadows for exterior gas pump island & pillars

            DrawCube(Vector3{ 128.0f, 10.012f, 140.0f }, 4.6f, 0.005f, 14.6f, Color{ 10, 10, 14, 185 });

            DrawCircle3D(Vector3{ 128.0f, 10.015f, 135.0f }, 0.85f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 8, 8, 12, 195 });

            DrawCircle3D(Vector3{ 128.0f, 10.015f, 145.0f }, 0.85f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 8, 8, 12, 195 });



            // 1. EXTERIOR SURROUNDING LOT (WEST OF HIGHWAY: X = 78..115, Z = 120..160)

            // ---------------------------------------------------------------------

            float distLotSq = (96.5f - camera.position.x)*(96.5f - camera.position.x) + (140.0f - camera.position.z)*(140.0f - camera.position.z);

            if (distLotSq < 90.0f * 90.0f) {

                // A. Dark wet asphalt apron slab

                DrawCube({ 96.5f, 10.012f, 140.0f }, 37.0f, 0.02f, 40.0f, ApplyExteriorDaylight({ 32, 32, 36, 255 }, 1.0f));



                // B. Concrete Sidewalk Curb bordering the highway front (X = 114.2, Z: 122..158)

                DrawCube({ 114.2f, 10.08f, 140.0f }, 0.45f, 0.14f, 36.0f, ApplyExteriorDaylight({ 58, 54, 50, 255 }, 1.0f));

                DrawCube({ 114.55f, 10.04f, 140.0f }, 0.25f, 0.06f, 36.0f, { 16, 16, 18, 255 }); // Gutter



                // Concrete walkways leading from road curb up to shop entrance (Z = 140.0, X: 108..114)

                DrawCube({ 111.1f, 10.08f, 137.5f }, 6.2f, 0.14f, 0.35f, { 42, 40, 38, 255 });

                DrawCube({ 111.1f, 10.08f, 142.5f }, 6.2f, 0.14f, 0.35f, { 42, 40, 38, 255 });



                // C. Reflective Rain Puddles (from reference code)

                Color puddleCol = { 28, 38, 55, 210 };

                DrawCube({ 112.0f, 10.025f, 133.0f }, 3.2f, 0.005f, 2.0f, puddleCol);

                DrawCube({ 111.5f, 10.025f, 148.0f }, 2.8f, 0.005f, 2.2f, puddleCol);



                // D. Exterior Lamp Posts with Warm Light Pools (from reference code)

                // South Lot Lamp Post

                DrawCircle3D({ 113.5f, 10.015f, 127.0f }, 0.75f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 10, 10, 14, 175 });

                DrawCylinder({ 113.5f, 10.0f, 127.0f }, 0.09f, 0.09f, 4.8f, 8, { 35, 38, 42, 255 });

                DrawCube({ 113.0f, 14.7f, 127.0f }, 1.0f, 0.07f, 0.07f, { 35, 38, 42, 255 });

                DrawSphere({ 112.6f, 14.65f, 127.0f }, 0.18f, { 255, 215, 110, 245 });

                DrawCircle3D({ 112.6f, 10.028f, 127.0f }, 4.5f, { 1, 0, 0 }, 90.0f, { 75, 65, 25, 75 });



                // North Lot Lamp Post

                DrawCircle3D({ 113.5f, 10.015f, 153.0f }, 0.75f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 10, 10, 14, 175 });

                DrawCylinder({ 113.5f, 10.0f, 153.0f }, 0.09f, 0.09f, 4.8f, 8, { 35, 38, 42, 255 });

                DrawCube({ 113.0f, 14.7f, 153.0f }, 1.0f, 0.07f, 0.07f, { 35, 38, 42, 255 });

                DrawSphere({ 112.6f, 14.65f, 153.0f }, 0.18f, { 255, 215, 110, 245 });

                DrawCircle3D({ 112.6f, 10.028f, 153.0f }, 4.5f, { 1, 0, 0 }, 90.0f, { 75, 65, 25, 75 });



                // E. Exterior Roadside Signboard ("THE STRANGE HOUR")

                DrawCube({ 114.2f, 11.2f, 134.5f }, 0.10f, 2.4f, 0.10f, { 35, 28, 20, 255 });

                DrawCube({ 114.2f, 11.2f, 136.5f }, 0.10f, 2.4f, 0.10f, { 35, 28, 20, 255 });

                DrawCube({ 114.2f, 12.3f, 135.5f }, 0.08f, 1.1f, 2.2f, { 48, 36, 24, 255 });

                DrawCubeWires({ 114.2f, 12.3f, 135.5f }, 0.09f, 1.12f, 2.22f, { 180, 130, 60, 255 });



                // Rear Alley Props: only drawn when outside the store

                bool isInsideStore = (camera.position.x <= 107.5f && camera.position.x >= 86.2f &&

                                      camera.position.z >= 126.2f && camera.position.z <= 153.8f);

                if (!isInsideStore) {

                    DrawCube({ 84.0f, 10.025f, 136.0f }, 3.5f, 0.005f, 2.5f, puddleCol);

                    DrawCube({ 97.0f, 10.025f, 123.5f }, 4.0f, 0.005f, 2.2f, puddleCol);



                    // Rear Alley Lamp Post (flickering)

                    float rFlick = (sinf(timeVal * 19.0f) > -0.7f) ? 1.0f : 0.2f;

                    DrawCylinder({ 82.0f, 10.0f, 140.0f }, 0.08f, 0.08f, 4.5f, 8, { 35, 38, 42, 255 });

                    DrawSphere({ 82.0f, 14.55f, 140.0f }, 0.16f, { (unsigned char)(255 * rFlick), (unsigned char)(190 * rFlick), (unsigned char)(70 * rFlick), 240 });

                    DrawCircle3D({ 82.0f, 10.028f, 140.0f }, 3.8f, { 1, 0, 0 }, 90.0f, { (unsigned char)(60 * rFlick), (unsigned char)(50 * rFlick), 20, (unsigned char)(65 * rFlick) });



                    // F. Exterior Alley Props: Industrial Dumpster, Pallets, Oil Drums

                    DrawCube({ 83.2f, 10.85f, 128.0f }, 2.6f, 1.5f, 1.4f, { 42, 50, 40, 255 });

                    DrawCube({ 83.2f, 11.65f, 128.0f }, 2.7f, 0.10f, 1.5f, { 30, 36, 28, 255 });

                    DrawCylinder({ 83.0f, 10.0f, 150.0f }, 0.32f, 0.32f, 0.95f, 10, { 75, 45, 30, 255 });

                    DrawCylinder({ 83.0f, 10.0f, 151.0f }, 0.32f, 0.32f, 0.95f, 10, { 50, 48, 52, 255 });

                }

            }



            // ---------------------------------------------------------------------

            // 2. STANDALONE SHOP BUILDING EXTERIOR SHELL (X: 86..108, Z: 126..154)

            // ---------------------------------------------------------------------

            Color wallExtCol = ApplyExteriorDaylight({ 55, 50, 44, 255 }, 0.75f);

            // West Exterior Wall (X = 85.85)

            DrawCube({ 85.85f, 12.6f, 140.0f }, 0.35f, 5.2f, 28.0f, wallExtCol);



            // =====================================================================

            // EXTERIOR WORN CARPET, HEAVY INDUSTRIAL BLAST HATCH,

            // VAST CREEPY METALLIC CONDUIT & THE ABANDONED VILLAGE WORLD

            // =====================================================================

            {

                // A. HEAVY CAST-IRON HATCH RIM FLUSH WITH GROUND (X = 83.8, Y = 10.01, Z = 140.0)

                DrawCube({ 83.8f, 10.005f, 140.0f }, 2.4f, 0.03f, 2.8f, Color{ 30, 32, 36, 255 }); // Dark iron rim

                DrawCubeWires({ 83.8f, 10.010f, 140.0f }, 2.42f, 0.035f, 2.82f, Color{ 18, 20, 22, 255 });



                // Corner mounting anchor bolts

                DrawSphere({ 82.8f, 10.025f, 138.8f }, 0.04f, Color{ 70, 72, 78, 255 });

                DrawSphere({ 84.8f, 10.025f, 138.8f }, 0.04f, Color{ 70, 72, 78, 255 });

                DrawSphere({ 82.8f, 10.025f, 141.2f }, 0.04f, Color{ 70, 72, 78, 255 });

                DrawSphere({ 84.8f, 10.025f, 141.2f }, 0.04f, Color{ 70, 72, 78, 255 });



                // B. HEAVY REINFORCED INDUSTRIAL STEEL BLAST HATCH

                if (g_hatchAnim < 0.05f) {

                    // Closed Hatch Door Leaf

                    Vector3 hPos = { 83.8f, 10.02f, 140.0f };

                    DrawCube(hPos, 2.1f, 0.05f, 2.5f, Color{ 48, 46, 44, 255 }); // Rusted steel diamond-plate

                    DrawCubeWires(hPos, 2.11f, 0.055f, 2.51f, Color{ 24, 22, 20, 255 });

                    // Yellow & Black Industrial Hazard Stripes along border

                    DrawCube({ hPos.x, hPos.y + 0.01f, hPos.z - 1.15f }, 2.0f, 0.01f, 0.14f, Color{ 195, 155, 30, 255 });

                    DrawCube({ hPos.x, hPos.y + 0.01f, hPos.z + 1.15f }, 2.0f, 0.01f, 0.14f, Color{ 195, 155, 30, 255 });

                    DrawCube({ hPos.x - 0.95f, hPos.y + 0.01f, hPos.z }, 0.14f, 0.01f, 2.2f, Color{ 195, 155, 30, 255 });

                    DrawCube({ hPos.x + 0.95f, hPos.y + 0.01f, hPos.z }, 0.14f, 0.01f, 2.2f, Color{ 195, 155, 30, 255 });

                    // Heavy Rotary Latching Dog-Wheel & Center Lock Spindle

                    DrawCylinder({ hPos.x, hPos.y + 0.02f, hPos.z }, 0.22f, 0.22f, 0.06f, 12, Color{ 32, 34, 38, 255 });

                    DrawCylinder({ hPos.x, hPos.y + 0.05f, hPos.z }, 0.06f, 0.06f, 0.08f, 8, Color{ 65, 68, 75, 255 });

                    // Dual heavy steel slide bolts

                    DrawCube({ hPos.x, hPos.y + 0.025f, hPos.z - 0.55f }, 1.6f, 0.04f, 0.09f, Color{ 75, 78, 85, 255 });

                    DrawCube({ hPos.x, hPos.y + 0.025f, hPos.z + 0.55f }, 1.6f, 0.04f, 0.09f, Color{ 75, 78, 85, 255 });

                } else {

                    // Open Hatch Door Leaf angled smoothly back on heavy greased hinges

                    float hAngle = g_hatchAnim * 95.0f;

                    float radH = hAngle * DEG2RAD;

                    Vector3 hPivot = { 82.75f, 10.02f, 140.0f };

                    Vector3 hCenter = { hPivot.x - sinf(radH) * 1.05f, hPivot.y + cosf(radH) * 1.05f, 140.0f };

                    DrawCube(hCenter, 0.08f, 2.1f, 2.5f, Color{ 48, 46, 44, 255 });

                    DrawCubeWires(hCenter, 0.085f, 2.11f, 2.51f, Color{ 24, 22, 20, 255 });

                    // Heavy hinge brackets

                    DrawCube({ 82.75f, 10.05f, 139.2f }, 0.24f, 0.12f, 0.16f, Color{ 32, 34, 38, 255 });

                    DrawCube({ 82.75f, 10.05f, 140.8f }, 0.24f, 0.12f, 0.16f, Color{ 32, 34, 38, 255 });

                }



                // PROPPED HEAVY TRENCH SHOVEL NEAR CARPET (Blade on ground, handle resting on timber)

                if (!g_hasShovel) {

                    // Weathered timber resting block supporting the leaning shovel shaft

                    DrawCube(Vector3{ 84.40f, 10.20f, 138.15f }, 0.30f, 0.36f, 0.45f, Color{ 58, 42, 28, 255 });

                    DrawCubeWires(Vector3{ 84.40f, 10.20f, 138.15f }, 0.31f, 0.37f, 0.46f, Color{ 32, 22, 16, 255 });



                    // Shovel blade firmly on earth at Y=10.02, shaft propped up against timber at Y=10.71

                    Matrix sRot = MatrixMultiply(MatrixRotateX(45.0f * DEG2RAD), MatrixRotateY(-15.0f * DEG2RAD));

                    Matrix sWorld = MatrixMultiply(sRot, MatrixTranslate(84.40f, 10.71f, 138.20f));

                    Vector3 lDirWorld = { -0.2f, -1.0f, -0.3f };

                    DrawShovel(g_shovelRig, sWorld, camera.position, lDirWorld);



                    // Subtle atmospheric interaction glow halo where the steel blade meets the dirt

                    float shGlow = 0.5f + 0.35f * sinf(timeVal * 4.0f);

                    DrawCircle3D(Vector3{ 84.22f, 10.035f, 138.87f }, 0.45f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 255, 215, 80, (unsigned char)(45 * shGlow) });

                }



                // C. WORN BURGUNDY CARPET (Lying flat over hatch or rolled back)

                float cSlideX = g_carpetAnim * 3.2f;

                float cFoldY  = g_carpetAnim * 0.22f;

                float cScaleX = 2.6f - g_carpetAnim * 1.1f;

                Vector3 cPos = { 83.8f - cSlideX, 10.05f + cFoldY * 0.5f, 140.0f };

                // Heavy wool fabric base

                DrawCube(cPos, cScaleX, 0.03f + cFoldY, 3.4f, Color{ 105, 18, 22, 255 });

                DrawCube({ cPos.x, cPos.y + 0.006f, cPos.z }, cScaleX * 0.88f, 0.03f, 3.0f, Color{ 165, 125, 35, 255 });

                DrawCube({ cPos.x, cPos.y + 0.010f, cPos.z }, cScaleX * 0.74f, 0.03f, 2.6f, Color{ 75, 12, 15, 255 });

                // Fringe & damp mud weathering

                DrawCube({ cPos.x, cPos.y + 0.003f, cPos.z - 1.72f }, cScaleX, 0.015f, 0.12f, Color{ 175, 160, 125, 240 });

                DrawCube({ cPos.x, cPos.y + 0.003f, cPos.z + 1.72f }, cScaleX, 0.015f, 0.12f, Color{ 175, 160, 125, 240 });



                // D. ENTRANCE SHAFT & DESCENDING STEEL STAIRS (Under the Hatch, strictly Y <= 10.0)

                if (g_tunnelHatchOpen) {

                    // Dark steel & concrete lined shaft interior walls

                    DrawCube({ 83.8f, 8.0f, 138.6f }, 2.4f, 4.0f, 0.20f, Color{ 26, 28, 32, 255 }); // South wall

                    DrawCube({ 83.8f, 8.0f, 141.4f }, 2.4f, 4.0f, 0.20f, Color{ 26, 28, 32, 255 }); // North wall

                    DrawCube({ 85.0f, 8.0f, 140.0f }, 0.20f, 4.0f, 2.6f, Color{ 26, 28, 32, 255 }); // East wall



                    // Heavy welded steel diamond-plate stairs descending from Y=10.0 down to Y=6.5 at X=82.0

                    for (int step = 0; step < 8; step++) {

                        float stFrac = (float)step / 7.0f;

                        float stX = 84.6f - stFrac * 2.6f;

                        float stY = 9.8f - stFrac * 3.3f;

                        // Steel step tread

                        DrawCube({ stX, stY, 140.0f }, 0.36f, 0.08f, 2.2f, Color{ 52, 50, 48, 255 });

                        // Safety yellow abrasive nosing strip

                        DrawCube({ stX - 0.16f, stY + 0.01f, 140.0f }, 0.05f, 0.08f, 2.18f, Color{ 210, 175, 40, 255 });

                    }

                    // Tubular steel safety handrails inside the shaft

                    DrawLine3D({ 84.6f, 10.4f, 139.0f }, { 82.0f, 7.3f, 139.0f }, Color{ 55, 58, 65, 255 });

                    DrawLine3D({ 84.6f, 10.4f, 141.0f }, { 82.0f, 7.3f, 141.0f }, Color{ 55, 58, 65, 255 });

                    // Cold green-blue industrial mist wafting from shaft

                    DrawCube({ 83.4f, 7.5f, 140.0f }, 2.0f, 2.8f, 2.2f, Color{ 120, 180, 200, 28 });

                }



                // =================================================================

                // 1. VERY LARGE AND DEEP CREEPY METALLIC CONDUIT (X: 82 -> 32)

                // =================================================================

                bool nearTunnel = (camera.position.x <= 84.0f && camera.position.x >= 28.0f &&

                                  camera.position.z >= 132.0f && camera.position.z <= 148.0f);

                if (nearTunnel || camera.position.y < 9.5f) {

                    // --- UPPER CREEPY METALLIC TUNNEL (X: 82.0 -> 62.0, Y: 6.5 -> 1.2, Width 6.0m) ---

                    // Strictly subterranean: ceiling is capped at Y <= 9.80, never breaching surface!

                    for (int s = 0; s <= 12; s++) {

                        float frac0 = (float)s / 12.0f;

                        float frac1 = (float)(s + 1) / 12.0f;

                        float x0 = 82.0f - frac0 * 20.0f;

                        float x1 = 82.0f - frac1 * 20.0f;

                        float y0 = 6.5f - frac0 * 5.3f;

                        float y1 = 6.5f - frac1 * 5.3f;

                        float cx = (x0 + x1) * 0.5f;

                        float cy = (y0 + y1) * 0.5f; // floor level

                        float dx = fabsf(x0 - x1) + 0.15f;

                        float ceilY = cy + 3.8f;

                        if (ceilY > 9.80f) ceilY = 9.80f; // HARD CLAMP: zero geometry above ground!

                        float wallH = ceilY - cy;



                        // 1. Rusted Heavy Diamond-Plate Steel Floor with Center Drainage Grate

                        DrawCube({ cx, cy - 0.12f, 140.0f }, dx, 0.24f, 5.8f, Color{ 36, 34, 33, 255 });

                        // Raised floor plating (Left & Right walkways)

                        DrawCube({ cx, cy + 0.02f, 138.4f }, dx * 0.95f, 0.06f, 2.2f, Color{ 48, 45, 43, 255 });

                        DrawCube({ cx, cy + 0.02f, 141.6f }, dx * 0.95f, 0.06f, 2.2f, Color{ 48, 45, 43, 255 });

                        // Recessed Center Drainage Trench with Rusted Iron Grate

                        DrawCube({ cx, cy - 0.08f, 140.0f }, dx, 0.14f, 1.1f, Color{ 16, 15, 14, 255 });

                        DrawCube({ cx, cy - 0.02f, 140.0f }, dx * 0.9f, 0.02f, 1.0f, Color{ 30, 28, 26, 255 }); // Grating

                        // Stagnant black fluid / dried blood in trench

                        if (s % 2 == 0) {

                            DrawCube({ cx, cy - 0.04f, 140.0f }, dx * 0.7f, 0.01f, 0.85f, Color{ 35, 10, 12, 230 });

                        }



                        // 2. Dark Rusted Corrugated Steel Walls

                        DrawCube({ cx, cy + wallH * 0.5f, 137.1f }, dx, wallH, 0.35f, Color{ 42, 40, 38, 255 }); // South wall

                        DrawCube({ cx, cy + wallH * 0.5f, 142.9f }, dx, wallH, 0.35f, Color{ 42, 40, 38, 255 }); // North wall

                        // Heavy Rusted Corrugated Arched Steel Ceiling

                        DrawCube({ cx, ceilY, 140.0f }, dx, 0.32f, 6.0f, Color{ 32, 30, 28, 255 });



                        // 3. Heavy Industrial Steel I-Beam Rib Bulkheads (Every 3 segments)

                        if (s % 3 == 0) {

                            Color ibCol = { 54, 52, 50, 255 };

                            // South & North vertical columns

                            DrawCube({ cx, cy + wallH * 0.5f, 137.35f }, 0.28f, wallH, 0.28f, ibCol);

                            DrawCube({ cx, cy + wallH * 0.5f, 142.65f }, 0.28f, wallH, 0.28f, ibCol);

                            // Overhead arched I-Beam cross-beam

                            DrawCube({ cx, ceilY - 0.16f, 140.0f }, 0.30f, 0.28f, 5.5f, ibCol);

                            // Riveted triangular corner gussets

                            DrawCube({ cx, ceilY - 0.40f, 137.75f }, 0.26f, 0.32f, 0.32f, Color{ 40, 38, 36, 255 });

                            DrawCube({ cx, ceilY - 0.40f, 142.25f }, 0.26f, 0.32f, 0.32f, Color{ 40, 38, 36, 255 });



                            // Yellow/Black Hazard Stencil on bulkheads

                            DrawCube({ cx + 0.15f, cy + 1.6f, 137.45f }, 0.02f, 0.6f, 0.18f, Color{ 190, 150, 25, 240 });

                            DrawCube({ cx + 0.15f, cy + 1.6f, 142.55f }, 0.02f, 0.6f, 0.18f, Color{ 190, 150, 25, 240 });



                            // Caged Industrial Emergency Bulkhead Lamp (At stations s=3 and s=9)

                            if (s == 3 || s == 9) {

                                Vector3 lampP = { cx, ceilY - 0.42f, 140.0f };

                                float flicker = 0.75f + 0.25f * sinf((float)GetTime() * 8.0f + (float)s * 3.0f);

                                Color glowCol = { (unsigned char)(255 * flicker), (unsigned char)(170 * flicker), 45, 255 };

                                DrawSphere(lampP, 0.10f, glowCol);

                                // Protective steel wire cage

                                DrawCubeWires(lampP, 0.24f, 0.28f, 0.24f, Color{ 60, 58, 55, 240 });

                                // Sickly pool of light on the diamond-plate floor

                                DrawCircle3D(Vector3{ cx, cy + 0.05f, 140.0f }, 4.4f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 200, 120, 25, (unsigned char)(40 * flicker) });

                            }

                        }



                        // 4. Overhead Industrial Utility Pipes & Sagging Black Electrical Cables

                        // High-pressure steam/water utility pipes along North wall

                        DrawCylinderEx({ x0, cy + 2.8f, 142.45f }, { x1, cy + 2.8f, 142.45f }, 0.07f, 0.07f, 6, Color{ 75, 45, 35, 255 });

                        DrawCylinderEx({ x0, cy + 2.6f, 142.45f }, { x1, cy + 2.6f, 142.45f }, 0.05f, 0.05f, 6, Color{ 65, 40, 30, 255 });

                        // Sagging black rubber electrical cable bundle along ceiling

                        float cableSag = 0.14f * sinf(frac0 * PI * 12.0f);

                        DrawLine3D({ x0, ceilY - 0.25f, 138.8f }, { x1, ceilY - 0.25f - cableSag, 138.8f }, Color{ 20, 20, 22, 255 });

                        DrawLine3D({ x0, ceilY - 0.28f, 139.0f }, { x1, ceilY - 0.28f - cableSag, 139.0f }, Color{ 22, 22, 24, 255 });

                    }



                    // --- CRUSHED BULKHEAD CAVE-IN OBSTRUCTION (~38% DEPTH: X = 62.0, Y = 1.2) ---

                    if (!g_tunnelDug) {

                        // Catastrophic structural collapse: buckled corrugated steel plates, sheared I-beams & rock

                        DrawCube({ 62.0f, 2.8f, 140.0f }, 2.6f, 4.2f, 5.8f, Color{ 34, 32, 30, 255 }); // Core impassable rubble

                        // Buckled, torn corrugated steel siding crushed inward

                        DrawCube({ 62.6f, 2.4f, 138.6f }, 0.9f, 2.2f, 2.0f, Color{ 52, 48, 44, 255 });

                        DrawCube({ 62.5f, 3.2f, 141.4f }, 1.1f, 1.8f, 1.8f, Color{ 46, 44, 42, 255 });

                        // Sheared twisted structural I-Beams protruding out at angles

                        DrawCube({ 62.8f, 2.6f, 139.5f }, 0.22f, 3.0f, 0.22f, Color{ 68, 64, 60, 255 });

                        DrawCube({ 62.7f, 3.5f, 140.6f }, 0.24f, 0.24f, 2.6f, Color{ 62, 58, 54, 255 });

                        // Fractured rebar mesh and sharp bedrock slabs

                        DrawCube({ 62.9f, 1.7f, 140.2f }, 0.8f, 1.3f, 2.2f, Color{ 38, 36, 34, 255 });

                        DrawLine3D({ 62.9f, 3.2f, 139.0f }, { 63.3f, 1.8f, 141.0f }, Color{ 90, 85, 80, 255 });

                        DrawLine3D({ 62.9f, 2.2f, 141.5f }, { 63.4f, 3.6f, 139.5f }, Color{ 90, 85, 80, 255 });

                    } else {

                        // Excavated passage breach: sheared steel and rubble cleared to the flanks

                        DrawCube({ 62.0f, 1.8f, 137.6f }, 2.4f, 2.4f, 1.2f, Color{ 42, 40, 38, 255 });

                        DrawCube({ 62.0f, 1.8f, 142.4f }, 2.4f, 2.4f, 1.2f, Color{ 42, 40, 38, 255 });

                        // Excavated trench through the buckled diamond-plate floor

                        DrawCube({ 62.0f, 1.12f, 140.0f }, 2.6f, 0.06f, 3.4f, Color{ 22, 20, 18, 255 });

                        // Cold draft & blue haze drifting from the lower abyss

                        DrawCube({ 61.2f, 2.5f, 140.0f }, 2.0f, 3.2f, 3.4f, Color{ 130, 185, 210, 32 });

                    }



                    // --- LOWER DEEP METALLIC CONDUIT (X: 62.0 -> 32.0, Y: 1.2 -> -16.0) ---

                    if (g_tunnelDug || camera.position.x < 62.0f) {

                        for (int ds = 0; ds <= 16; ds++) {

                            float frac0 = (float)ds / 16.0f;

                            float frac1 = (float)(ds + 1) / 16.0f;

                            float x0 = 62.0f - frac0 * 30.0f;

                            float x1 = 62.0f - frac1 * 30.0f;

                            float y0 = 1.2f - frac0 * 17.2f;

                            float y1 = 1.2f - frac1 * 17.2f;

                            float cx = (x0 + x1) * 0.5f;

                            float cy = (y0 + y1) * 0.5f;

                            float dx = fabsf(x0 - x1) + 0.15f;



                            // 1. Massive Circular Ribbed Subterranean Conduit Plating

                            DrawCube({ cx, cy - 0.14f, 140.0f }, dx, 0.28f, 5.8f, Color{ 28, 26, 26, 255 });

                            // Murky sludgy water pooling over the steel plates

                            DrawCube({ cx, cy + 0.02f, 140.0f }, dx * 0.95f, 0.05f, 5.4f, Color{ 20, 22, 25, 255 });

                            if (ds % 3 == 0) {

                                DrawCube({ cx, cy + 0.05f, 140.0f }, dx * 0.8f, 0.01f, 3.8f, Color{ 10, 14, 18, 235 });

                            }



                            // Curved corrugated steel conduit walls & ceiling

                            DrawCube({ cx, cy + 2.0f, 137.1f }, dx, 4.2f, 0.40f, Color{ 32, 30, 30, 255 });

                            DrawCube({ cx, cy + 2.0f, 142.9f }, dx, 4.2f, 0.40f, Color{ 32, 30, 30, 255 });

                            DrawCube({ cx, cy + 4.1f, 140.0f }, dx, 0.40f, 6.0f, Color{ 26, 25, 26, 255 });



                            // Heavy Circular Submarine Pressure Bulkhead Rings every 4 steps

                            if (ds % 4 == 0) {

                                Color ringCol = { 46, 44, 46, 255 };

                                DrawCube({ cx, cy + 2.0f, 137.35f }, 0.38f, 4.2f, 0.38f, ringCol);

                                DrawCube({ cx, cy + 2.0f, 142.65f }, 0.38f, 4.2f, 0.38f, ringCol);

                                DrawCube({ cx, cy + 3.9f, 140.0f }, 0.38f, 0.38f, 5.6f, ringCol);



                                // Severed conduit cables with periodic electric blue spark

                                if (ds == 8 || ds == 12) {

                                    DrawLine3D({ cx, cy + 3.8f, 142.5f }, { cx + 0.3f, cy + 2.6f, 142.1f }, Color{ 25, 25, 28, 255 });

                                    if ((int)(GetTime() * 5.0f) % 3 == 0) {

                                        DrawSphere({ cx + 0.3f, cy + 2.6f, 142.1f }, 0.08f, Color{ 140, 220, 255, 255 });

                                    }

                                }

                            }

                        }



                        // Massive Blown-Out Blast Door Breach at Cliffside (X = 32.0, Y = -16.0)

                        // Opening out of the mountain bedrock into The Abandoned Village

                        DrawCube({ 32.0f, -13.8f, 140.0f }, 0.8f, 4.8f, 5.8f, Color{ 24, 22, 24, 255 });

                        // Heavy sheared pressure door propped against rock wall

                        DrawCube({ 31.6f, -14.2f, 137.8f }, 0.16f, 3.8f, 1.8f, Color{ 48, 45, 44, 255 });

                    }

                }



                // =================================================================

                // 2. THE CREEPY ABANDONED VILLAGE WORLD (Elevation Y = -16.0)

                // Spanning X: -45 to 32, Z: 105 to 175

                // =================================================================

                bool inVillage = (camera.position.x <= 40.0f && camera.position.y <= -2.0f);

                if (inVillage) {

                    // A. Vast Sunken Valley Ground Plane

                    DrawCube({ -6.5f, -16.12f, 140.0f }, 78.0f, 0.24f, 70.0f, Color{ 18, 22, 17, 255 });

                    // Muddy cart tracks and cobblestone path winding through village

                    DrawCube({ 3.5f, -16.00f, 140.0f }, 58.0f, 0.02f, 3.2f, Color{ 28, 25, 20, 255 });

                    // Rolling ground mist sheets

                    DrawCube({ -6.5f, -15.55f, 140.0f }, 76.0f, 0.65f, 68.0f, Color{ 175, 195, 210, 32 });



                    // Surrounding Dark Mountain Cliff Backdrops

                    DrawCube({ -45.5f, -8.0f, 140.0f }, 2.0f, 18.0f, 72.0f, Color{ 14, 16, 18, 255 }); // West cliff

                    DrawCube({ 32.5f, -8.0f, 122.0f }, 2.0f, 18.0f, 34.0f, Color{ 14, 16, 18, 255 });  // East cliff South

                    DrawCube({ 32.5f, -8.0f, 158.0f }, 2.0f, 18.0f, 34.0f, Color{ 14, 16, 18, 255 });  // East cliff North

                    DrawCube({ -6.5f, -8.0f, 104.5f }, 80.0f, 18.0f, 2.0f, Color{ 14, 16, 18, 255 });  // South cliff

                    DrawCube({ -6.5f, -8.0f, 175.5f }, 80.0f, 18.0f, 2.0f, Color{ 14, 16, 18, 255 });  // North cliff



                    // B. THE ANCIENT STONE WELL (Village Center X = 8.0, Z = 140.0)

                    {

                        Vector3 wellP = { 8.0f, -15.45f, 140.0f };

                        // Circular stone cylinder well lip

                        DrawCylinder(wellP, 1.25f, 1.25f, 1.1f, 14, Color{ 62, 65, 70, 255 });

                        DrawCylinder(Vector3{ wellP.x, wellP.y + 0.05f, wellP.z }, 0.95f, 0.95f, 1.2f, 14, Color{ 8, 9, 11, 255 }); // Dark interior void

                        // Weathered timber upright canopy posts

                        DrawCube({ 8.0f, -13.8f, 138.9f }, 0.16f, 2.4f, 0.16f, Color{ 68, 50, 32, 255 });

                        DrawCube({ 8.0f, -13.8f, 141.1f }, 0.16f, 2.4f, 0.16f, Color{ 68, 50, 32, 255 });

                        // Crank spindle axle and wooden drum

                        DrawCylinderEx({ 8.0f, -13.6f, 138.9f }, { 8.0f, -13.6f, 141.1f }, 0.07f, 0.07f, 8, Color{ 52, 38, 24, 255 });

                        // A-frame shingled roof canopy

                        DrawCube({ 8.0f, -12.5f, 140.0f }, 1.8f, 0.22f, 2.6f, Color{ 44, 34, 24, 255 });

                        DrawLine3D({ 7.1f, -12.7f, 140.0f }, { 8.0f, -12.2f, 140.0f }, Color{ 75, 55, 35, 255 });

                        // Frayed rope & wooden bucket

                        DrawLine3D({ 8.0f, -13.6f, 140.0f }, { 8.0f, -15.2f, 140.0f }, Color{ 160, 145, 115, 255 });

                        DrawCylinder({ 8.0f, -15.35f, 140.0f }, 0.14f, 0.12f, 0.22f, 8, Color{ 75, 52, 32, 255 });

                    }



                    // C. THE ELDER'S COTTAGE (North, X = 6.0, Z = 160.0)

                    {

                        Vector3 cabP = { 6.0f, -16.0f, 160.0f };

                        // Wooden floor foundation

                        DrawCube({ cabP.x, cabP.y + 0.15f, cabP.z }, 8.0f, 0.30f, 6.4f, Color{ 48, 36, 24, 255 });

                        // Weathered horizontal log walls

                        DrawCube({ cabP.x, cabP.y + 2.0f, cabP.z + 3.1f }, 8.0f, 3.5f, 0.22f, Color{ 58, 44, 30, 255 }); // Back (North)

                        DrawCube({ cabP.x - 3.9f, cabP.y + 2.0f, cabP.z }, 0.22f, 3.5f, 6.4f, Color{ 58, 44, 30, 255 }); // West

                        DrawCube({ cabP.x + 3.9f, cabP.y + 2.0f, cabP.z }, 0.22f, 3.5f, 6.4f, Color{ 58, 44, 30, 255 }); // East

                        // Front South wall with open doorway at X = 6.0

                        DrawCube({ cabP.x - 2.4f, cabP.y + 2.0f, cabP.z - 3.1f }, 3.2f, 3.5f, 0.22f, Color{ 58, 44, 30, 255 });

                        DrawCube({ cabP.x + 2.4f, cabP.y + 2.0f, cabP.z - 3.1f }, 3.2f, 3.5f, 0.22f, Color{ 58, 44, 30, 255 });

                        DrawCube({ cabP.x, cabP.y + 3.3f, cabP.z - 3.1f }, 1.8f, 0.9f, 0.22f, Color{ 58, 44, 30, 255 }); // Door header

                        // Covered front porch & steps

                        DrawCube({ cabP.x, cabP.y + 0.10f, cabP.z - 4.1f }, 6.2f, 0.20f, 1.8f, Color{ 42, 32, 20, 255 });

                        DrawCube({ cabP.x - 2.8f, cabP.y + 1.8f, cabP.z - 4.9f }, 0.16f, 3.4f, 0.16f, Color{ 55, 40, 26, 255 });

                        DrawCube({ cabP.x + 2.8f, cabP.y + 1.8f, cabP.z - 4.9f }, 0.16f, 3.4f, 0.16f, Color{ 55, 40, 26, 255 });

                        // Sloping cottage roof

                        DrawCube({ cabP.x, cabP.y + 4.1f, cabP.z }, 8.4f, 0.25f, 7.2f, Color{ 38, 28, 18, 255 });

                        // Hearth fireplace with stone chimney

                        DrawCube({ cabP.x + 3.4f, cabP.y + 2.4f, cabP.z + 1.5f }, 1.2f, 4.6f, 1.2f, Color{ 52, 54, 58, 255 });

                        // Solitary glowing candle in front window pane casting creepy warmth

                        DrawSphere({ cabP.x + 2.2f, cabP.y + 1.6f, cabP.z - 3.0f }, 0.05f, Color{ 255, 190, 80, 255 });

                        DrawCircle3D(Vector3{ cabP.x + 2.2f, cabP.y + 0.2f, cabP.z - 3.2f }, 2.4f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 210, 140, 40, 45 });

                    }



                    // D. THE SUNKEN CHAPEL & BELFRY (West, X = -24.0, Z = 140.0)

                    {

                        Vector3 chP = { -24.0f, -16.0f, 140.0f };

                        // Fieldstone chapel foundation

                        DrawCube({ chP.x, chP.y + 0.20f, chP.z }, 11.5f, 0.40f, 8.2f, Color{ 45, 48, 52, 255 });

                        // Weathered timber walls (height 5.2m)

                        DrawCube({ chP.x - 5.5f, chP.y + 2.8f, chP.z }, 0.30f, 5.2f, 8.0f, Color{ 42, 36, 30, 255 }); // West wall

                        DrawCube({ chP.x, chP.y + 2.8f, chP.z - 3.9f }, 11.0f, 5.2f, 0.30f, Color{ 42, 36, 30, 255 }); // South wall

                        DrawCube({ chP.x, chP.y + 2.8f, chP.z + 3.9f }, 11.0f, 5.2f, 0.30f, Color{ 42, 36, 30, 255 }); // North wall

                        // East facade with grand double-door entrance

                        DrawCube({ chP.x + 5.5f, chP.y + 2.8f, chP.z - 2.5f }, 0.30f, 5.2f, 3.0f, Color{ 42, 36, 30, 255 });

                        DrawCube({ chP.x + 5.5f, chP.y + 2.8f, chP.z + 2.5f }, 0.30f, 5.2f, 3.0f, Color{ 42, 36, 30, 255 });

                        DrawCube({ chP.x + 5.5f, chP.y + 4.4f, chP.z }, 0.30f, 2.0f, 2.2f, Color{ 42, 36, 30, 255 });

                        // Crooked Belfry Tower above entrance

                        Vector3 belfryP = { chP.x + 4.8f, chP.y + 6.8f, chP.z };

                        DrawCube(belfryP, 2.2f, 3.4f, 2.2f, Color{ 36, 30, 24, 255 });

                        DrawCubeWires(belfryP, 2.25f, 3.45f, 2.25f, Color{ 65, 55, 45, 255 });

                        // Bronze church bell hanging in belfry

                        DrawCylinder({ belfryP.x, belfryP.y - 0.2f, belfryP.z }, 0.32f, 0.20f, 0.55f, 8, Color{ 165, 125, 45, 255 });

                        // Shattered cross lying on stone steps

                        DrawCube({ chP.x + 6.8f, chP.y + 0.15f, chP.z - 0.5f }, 1.4f, 0.08f, 0.12f, Color{ 85, 65, 42, 255 });

                        DrawCube({ chP.x + 6.4f, chP.y + 0.17f, chP.z - 0.5f }, 0.12f, 0.08f, 0.75f, Color{ 85, 65, 42, 255 });

                        // Broken church pews inside

                        for (int pw = 0; pw < 4; pw++) {

                            float px = chP.x - 3.2f + (float)pw * 2.0f;

                            DrawCube({ px, chP.y + 0.65f, chP.z - 1.8f }, 0.45f, 0.55f, 2.0f, Color{ 52, 40, 28, 255 });

                            DrawCube({ px, chP.y + 0.65f, chP.z + 1.8f }, 0.45f, 0.55f, 2.0f, Color{ 52, 40, 28, 255 });

                        }

                    }



                    // E. THE BLACKSMITH FORGE & WORKSHOP (South, X = 4.0, Z = 120.0)

                    {

                        Vector3 smP = { 4.0f, -16.0f, 120.0f };

                        // Heavy timber corner columns

                        DrawCube({ smP.x - 3.2f, smP.y + 1.8f, smP.z - 2.4f }, 0.22f, 3.6f, 0.22f, Color{ 55, 42, 28, 255 });

                        DrawCube({ smP.x + 3.2f, smP.y + 1.8f, smP.z - 2.4f }, 0.22f, 3.6f, 0.22f, Color{ 55, 42, 28, 255 });

                        DrawCube({ smP.x - 3.2f, smP.y + 1.8f, smP.z + 2.4f }, 0.22f, 3.6f, 0.22f, Color{ 55, 42, 28, 255 });

                        DrawCube({ smP.x + 3.2f, smP.y + 1.8f, smP.z + 2.4f }, 0.22f, 3.6f, 0.22f, Color{ 55, 42, 28, 255 });

                        // Sloping timber roof

                        DrawCube({ smP.x, smP.y + 3.8f, smP.z }, 7.2f, 0.20f, 5.6f, Color{ 36, 28, 20, 255 });

                        // Stone forge hearth

                        DrawCube({ smP.x - 1.8f, smP.y + 0.75f, smP.z - 1.2f }, 1.4f, 1.5f, 1.4f, Color{ 44, 46, 50, 255 });

                        DrawCube({ smP.x - 1.8f, smP.y + 2.6f, smP.z - 1.2f }, 0.7f, 2.4f, 0.7f, Color{ 36, 38, 42, 255 }); // Chimney

                        // Blacksmith's Anvil on wooden stump

                        DrawCylinder({ smP.x + 0.6f, smP.y + 0.35f, smP.z }, 0.30f, 0.34f, 0.70f, 8, Color{ 72, 54, 34, 255 }); // Stump

                        DrawCube({ smP.x + 0.6f, smP.y + 0.85f, smP.z }, 0.55f, 0.24f, 0.28f, Color{ 28, 30, 32, 255 });  // Anvil horn & body

                        // Quenching trough with murky stagnant water

                        DrawCube({ smP.x + 1.8f, smP.y + 0.45f, smP.z - 1.0f }, 0.75f, 0.55f, 1.4f, Color{ 52, 42, 30, 255 });

                        DrawCircle3D(Vector3{ smP.x + 1.8f, smP.y + 0.68f, smP.z - 1.0f }, 0.55f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 35, 55, 65, 230 });

                    }



                    // F. THE COLLAPSED HOMESTEAD & DETRITUS (East, X = 18.0, Z = 118.0)

                    {

                        Vector3 colP = { 18.0f, -16.0f, 118.0f };

                        // Solitary standing fieldstone chimney

                        DrawCube({ colP.x, colP.y + 2.8f, colP.z }, 1.1f, 5.6f, 1.1f, Color{ 50, 52, 56, 255 });

                        // Charred tumbled wall logs & collapsed rafters

                        DrawCube({ colP.x - 1.8f, colP.y + 0.4f, colP.z + 1.2f }, 3.8f, 0.24f, 2.4f, Color{ 30, 24, 18, 255 });

                        DrawCube({ colP.x + 1.2f, colP.y + 0.3f, colP.z - 1.4f }, 2.6f, 0.22f, 3.2f, Color{ 24, 20, 16, 255 });

                    }



                    // G. ABANDONED WOODEN WAGON & RUSTED SICKLES (Pathside X = 18.0, Z = 146.0)

                    {

                        Vector3 wagP = { 18.0f, -15.4f, 146.0f };

                        // Wagon bed

                        DrawCube(wagP, 3.2f, 0.45f, 1.6f, Color{ 62, 44, 28, 255 });

                        // Broken spoked wheels tilted in the mud

                        DrawCylinderEx({ wagP.x - 1.2f, -15.8f, wagP.z - 0.9f }, { wagP.x - 1.2f, -15.8f, wagP.z - 0.8f }, 0.55f, 0.55f, 12, Color{ 48, 34, 20, 255 });

                        DrawCylinderEx({ wagP.x + 1.2f, -15.8f, wagP.z + 0.8f }, { wagP.x + 1.2f, -15.8f, wagP.z + 0.9f }, 0.55f, 0.55f, 12, Color{ 48, 34, 20, 255 });

                    }



                    // H. CROOKED SPLIT-RAIL CEDAR FENCES ALONG PATH

                    for (int f = 0; f < 7; f++) {

                        float fx = 26.0f - (float)f * 7.5f;

                        DrawCube({ fx, -15.35f, 137.4f }, 0.12f, 1.3f, 0.12f, Color{ 58, 44, 28, 255 });

                        DrawCube({ fx, -15.35f, 142.6f }, 0.12f, 1.3f, 0.12f, Color{ 58, 44, 28, 255 });

                        DrawLine3D({ fx, -15.1f, 137.4f }, { fx - 7.0f, -15.2f, 137.4f }, Color{ 58, 44, 28, 255 });

                        DrawLine3D({ fx, -15.6f, 137.4f }, { fx - 7.0f, -15.7f, 137.4f }, Color{ 58, 44, 28, 255 });

                        DrawLine3D({ fx, -15.1f, 142.6f }, { fx - 7.0f, -15.2f, 142.6f }, Color{ 58, 44, 28, 255 });

                        DrawLine3D({ fx, -15.6f, 142.6f }, { fx - 7.0f, -15.7f, 142.6f }, Color{ 58, 44, 28, 255 });

                    }



                    // I. CREEPY TWISTED DEAD TREES

                    Vector2 treeCoords[6] = {

                        { 14.0f, 132.0f }, { -4.0f, 128.0f }, { -14.0f, 154.0f },

                        { 12.0f, 168.0f }, { -32.0f, 130.0f }, { -34.0f, 152.0f }

                    };

                    for (int t = 0; t < 6; t++) {

                        Vector3 trP = { treeCoords[t].x, -16.0f, treeCoords[t].y };

                        // Gnarled gallow trunk

                        DrawCylinderEx(trP, { trP.x + 0.4f, trP.y + 6.2f, trP.z - 0.3f }, 0.32f, 0.12f, 6, Color{ 28, 24, 20, 255 });

                        // Claw-like branches

                        DrawLine3D({ trP.x + 0.4f, trP.y + 4.8f, trP.z - 0.3f }, { trP.x + 2.2f, trP.y + 6.5f, trP.z + 1.2f }, Color{ 28, 24, 20, 255 });

                        DrawLine3D({ trP.x + 0.4f, trP.y + 5.2f, trP.z - 0.3f }, { trP.x - 1.8f, trP.y + 6.8f, trP.z - 1.4f }, Color{ 28, 24, 20, 255 });

                    }

                }

            }

            // South Exterior Wall (Z = 125.85)

            DrawCube({ 97.0f, 12.6f, 125.85f }, 22.0f, 5.2f, 0.35f, wallExtCol);

            // North Exterior Wall (Z = 154.15) - Carved out for Haunted Washroom entrance doorway at X in [88.0, 90.5]
            DrawCube({ 87.0f, 12.6f, 154.15f }, 2.0f, 5.2f, 0.35f, wallExtCol);   // Left Flank: X in [86.0 .. 88.0]
            DrawCube({ 89.25f, 14.1f, 154.15f }, 2.5f, 2.2f, 0.35f, wallExtCol);  // Doorway Header: Y in [13.0 .. 15.2]
            DrawCube({ 99.25f, 12.6f, 154.15f }, 17.5f, 5.2f, 0.35f, wallExtCol); // Right Flank: X in [90.5 .. 108.0]

            // Haunted Washroom Annex Exterior Shell (X in [85.5 .. 93.0], Z in [154.0 .. 164.65])
            DrawCube({ 85.85f, 12.6f, 159.25f }, 0.35f, 5.2f, 10.5f, wallExtCol); // West Exterior Wall
            DrawCube({ 93.0f, 12.6f, 159.25f }, 0.35f, 5.2f, 10.5f, wallExtCol);  // East Exterior Wall
            DrawCube({ 89.42f, 12.6f, 164.65f }, 7.5f, 5.2f, 0.35f, wallExtCol); // North Back Wall
            DrawCube({ 89.42f, 15.15f, 159.25f }, 7.6f, 0.25f, 10.6f, { 28, 26, 24, 255 }); // Washroom Roof

            // East Exterior Facade Wall (X = 108.15, accurately cut out for 2.90m sliding glass entrance door)
            DrawCube({ 108.15f, 12.6f, 132.275f }, 0.35f, 5.2f, 12.55f, { 44, 40, 35, 255 });
            DrawCube({ 108.15f, 12.6f, 147.725f }, 0.35f, 5.2f, 12.55f, { 44, 40, 35, 255 });
            DrawCube({ 108.15f, 14.30f, 140.0f }, 0.35f, 1.80f, 2.90f, { 44, 40, 35, 255 }); // Door lintel header

            // Corner Pilaster Columns
            Color pillarCol = { 28, 26, 24, 255 };
            DrawCube({ 85.8f, 12.6f, 125.8f }, 0.55f, 5.2f, 0.55f, pillarCol);
            DrawCube({ 85.8f, 12.6f, 154.2f }, 0.55f, 5.2f, 0.55f, pillarCol);
            DrawCube({ 108.2f, 12.6f, 125.8f }, 0.55f, 5.2f, 0.55f, pillarCol);
            DrawCube({ 108.2f, 12.6f, 154.2f }, 0.55f, 5.2f, 0.55f, pillarCol);

            // Gutter Downspout Pipes
            DrawCylinder({ 108.35f, 10.0f, 126.1f }, 0.06f, 0.06f, 5.2f, 6, { 32, 32, 36, 255 });
            DrawCylinder({ 108.35f, 10.0f, 153.9f }, 0.06f, 0.06f, 5.2f, 6, { 32, 32, 36, 255 });

            // Commercial Parapet Roof Slab & Fascia Lip
            DrawCube({ 97.0f, 15.25f, 140.0f }, 22.8f, 0.35f, 28.8f, { 28, 26, 24, 255 });
            DrawCubeWires({ 97.0f, 15.25f, 140.0f }, 22.85f, 0.37f, 28.85f, { 45, 42, 38, 255 });



            // Glazed Display Windows on East Facade

            Color winCol = { 22, 35, 55, 175 };

            Color winFrame = { 55, 45, 35, 255 };

            // Left Window

            DrawCube({ 108.2f, 12.2f, 134.0f }, 0.06f, 2.6f, 4.5f, winCol);

            DrawCubeWires({ 108.2f, 12.2f, 134.0f }, 0.08f, 2.62f, 4.52f, winFrame);

            // Right Window

            DrawCube({ 108.2f, 12.2f, 146.0f }, 0.06f, 2.6f, 4.5f, winCol);

            DrawCubeWires({ 108.2f, 12.2f, 146.0f }, 0.08f, 2.62f, 4.52f, winFrame);



            // Porch Entrance Light above Door

            DrawSphere({ 108.45f, 13.5f, 140.0f }, 0.15f, { 255, 190, 85, 240 });

            DrawCube({ 108.40f, 13.5f, 140.0f }, 0.18f, 0.24f, 0.18f, { 30, 24, 18, 255 });



            // =========================================================================

            // COMMERCIAL AUTOMATIC SLIDING GLASS DOORS (X = 108.0, Z = 140.0)

            // Anodized architectural aluminum framing, high-visibility white frosted safety decals,

            // polished stainless steel tubular handles, and smooth pneumatic spring kinematics.

            // Guaranteed 100% visible from BOTH inside and outside the supermarket!

            // =========================================================================

            {

                // Dynamic ambient baseline ensures door framing never turns into invisible black mud

                float doorAmb = 0.55f * g_curExtDayFactor + 0.32f * g_curExtNightFactor + (g_shopLightsOn ? 0.35f : 0.12f);

                auto ModDoorCol = [&](Color baseCol, float mult = 1.0f) -> Color {

                    float f = Clamp(doorAmb * mult, 0.28f, 1.40f);

                    return Color{

                        (unsigned char)Clamp((int)(baseCol.r * f), 0, 255),

                        (unsigned char)Clamp((int)(baseCol.g * f), 0, 255),

                        (unsigned char)Clamp((int)(baseCol.b * f), 0, 255),

                        baseCol.a

                    };

                };



                Color frameAlum    = ModDoorCol(Color{ 175, 180, 190, 255 }, 1.15f); // Crisp satin commercial aluminum

                Color frameTrim    = ModDoorCol(Color{ 215, 220, 230, 255 }, 1.25f); // Aluminum highlight edge wires

                Color trackCol     = ModDoorCol(Color{ 145, 150, 158, 255 }, 1.05f); // Recessed stainless steel floor track

                Color glassTint    = Color{ 175, 218, 248, 95 };                     // Realistic architectural glass tint

                Color glassSheen   = Color{ 235, 248, 255, 180 };                    // Vibrant specular reflection highlight

                Color safetyFrost  = Color{ 248, 252, 255, 215 };                    // High-contrast white frosted safety stripe (never invisible!)

                Color rubberGasket = Color{ 32, 34, 38, 255 };                       // Black EPDM rubber perimeter seal

                Color handleCol    = ModDoorCol(Color{ 235, 240, 245, 255 }, 1.35f); // Polished chrome full-height grab handles



                // 1. Heavy outer entrance framing (spans exactly 2.90m across opening Z: 138.55 .. 141.45)

                // Overhead transom motor header box (containing drive belt, rollers, sensor)

                DrawCube({ 108.0f, 13.25f, 140.0f }, 0.40f, 0.28f, 2.90f, frameAlum);

                DrawCubeWires({ 108.0f, 13.25f, 140.0f }, 0.405f, 0.285f, 2.905f, frameTrim);



                // Left & Right vertical jamb structural posts

                DrawCube({ 108.0f, 11.55f, 138.55f }, 0.38f, 3.12f, 0.14f, frameAlum);

                DrawCubeWires({ 108.0f, 11.55f, 138.55f }, 0.385f, 3.125f, 0.145f, frameTrim);

                DrawCube({ 108.0f, 11.55f, 141.45f }, 0.38f, 3.12f, 0.14f, frameAlum);

                DrawCubeWires({ 108.0f, 11.55f, 141.45f }, 0.385f, 3.125f, 0.145f, frameTrim);



                // Recessed floor stainless guide track rail

                DrawCube({ 108.0f, 10.022f, 140.0f }, 0.28f, 0.016f, 2.80f, trackCol);



                // 2. Motion Sensor Pods (Overhead center, interior and exterior faces)

                Color ledCol = doorSensorActive ? Color{ 55, 255, 100, 255 } : Color{ 245, 45, 35, 255 };

                // Exterior sensor pod

                DrawCube({ 108.21f, 13.24f, 140.0f }, 0.05f, 0.09f, 0.26f, Color{ 30, 32, 35, 255 });

                DrawSphere({ 108.24f, 13.24f, 140.0f }, 0.026f, ledCol);

                // Interior sensor pod (Facing inside store: clearly visible to player approaching from inside!)

                DrawCube({ 107.79f, 13.24f, 140.0f }, 0.05f, 0.09f, 0.26f, Color{ 30, 32, 35, 255 });

                DrawSphere({ 107.76f, 13.24f, 140.0f }, 0.026f, ledCol);



                // 3. Fixed Outer Sidelite Glass Panes (Where sliding doors retract behind)

                rlDisableDepthMask();

                // South sidelite pane (Z: 138.62 .. 139.12)

                DrawCube({ 108.0f, 11.55f, 138.87f }, 0.03f, 2.76f, 0.50f, glassTint);

                // North sidelite pane (Z: 140.88 .. 141.38)

                DrawCube({ 108.0f, 11.55f, 141.13f }, 0.03f, 2.76f, 0.50f, glassTint);

                rlEnableDepthMask();



                DrawCubeWires({ 108.0f, 11.55f, 138.87f }, 0.035f, 2.765f, 0.505f, frameTrim);

                DrawCubeWires({ 108.0f, 11.55f, 141.13f }, 0.035f, 2.765f, 0.505f, frameTrim);



                // 4. Dual Biparting Sliding Glass Leaves

                float slideDist = doorSlideProgress * 0.88f; // Max opening clearance 1.76m



                // --- South Sliding Glass Leaf (Slides toward -Z) ---

                float leftZ = 139.52f - slideDist;

                // Aluminum leaf perimeter frame (Top rail, bottom rail, vertical stiles - 100% solid & visible)

                DrawCube({ 108.0f, 12.92f, leftZ }, 0.06f, 0.08f, 0.94f, frameAlum); // Top rail

                DrawCube({ 108.0f, 10.18f, leftZ }, 0.06f, 0.08f, 0.94f, frameAlum); // Bottom rail

                DrawCube({ 108.0f, 11.55f, leftZ + 0.44f }, 0.06f, 2.78f, 0.06f, frameAlum); // Leading vertical stile

                DrawCube({ 108.0f, 11.55f, leftZ - 0.44f }, 0.06f, 2.78f, 0.06f, frameAlum); // Trailing vertical stile

                DrawCubeWires({ 108.0f, 11.55f, leftZ }, 0.062f, 2.785f, 0.945f, frameTrim);



                // Transparent glass panel (drawn with depth mask disabled so background is never blocked)

                rlDisableDepthMask();

                DrawCube({ 108.0f, 11.55f, leftZ }, 0.025f, 2.68f, 0.82f, glassTint);

                rlEnableDepthMask();



                // Diagonal reflection highlights

                DrawLine3D({ 108.015f, 10.45f, leftZ - 0.35f }, { 108.015f, 12.65f, leftZ + 0.25f }, glassSheen);

                DrawLine3D({ 107.985f, 10.45f, leftZ - 0.35f }, { 107.985f, 12.65f, leftZ + 0.25f }, glassSheen);

                // High-contrast white frosted safety horizontal stripes across middle of glass

                DrawCube({ 108.0f, 11.40f, leftZ }, 0.028f, 0.045f, 0.82f, safetyFrost);

                DrawCube({ 108.0f, 12.15f, leftZ }, 0.028f, 0.045f, 0.82f, safetyFrost);

                // Leading edge rubber gasket seal

                DrawCube({ 108.0f, 11.55f, leftZ + 0.46f }, 0.05f, 2.78f, 0.022f, rubberGasket);

                // Stainless steel full-height vertical tubular handle (both interior and exterior sides)

                DrawCylinderEx({ 107.93f, 10.60f, leftZ + 0.36f }, { 107.93f, 12.45f, leftZ + 0.36f }, 0.016f, 0.016f, 8, handleCol);

                DrawCylinderEx({ 108.07f, 10.60f, leftZ + 0.36f }, { 108.07f, 12.45f, leftZ + 0.36f }, 0.016f, 0.016f, 8, handleCol);



                // --- North Sliding Glass Leaf (Slides toward +Z) ---

                float rightZ = 140.48f + slideDist;

                // Aluminum leaf perimeter frame (Top rail, bottom rail, vertical stiles - 100% solid & visible)

                DrawCube({ 108.0f, 12.92f, rightZ }, 0.06f, 0.08f, 0.94f, frameAlum); // Top rail

                DrawCube({ 108.0f, 10.18f, rightZ }, 0.06f, 0.08f, 0.94f, frameAlum); // Bottom rail

                DrawCube({ 108.0f, 11.55f, rightZ - 0.44f }, 0.06f, 2.78f, 0.06f, frameAlum); // Leading vertical stile

                DrawCube({ 108.0f, 11.55f, rightZ + 0.44f }, 0.06f, 2.78f, 0.06f, frameAlum); // Trailing vertical stile

                DrawCubeWires({ 108.0f, 11.55f, rightZ }, 0.062f, 2.785f, 0.945f, frameTrim);



                // Transparent glass panel (drawn with depth mask disabled so background is never blocked)

                rlDisableDepthMask();

                DrawCube({ 108.0f, 11.55f, rightZ }, 0.025f, 2.68f, 0.82f, glassTint);

                rlEnableDepthMask();



                // Diagonal reflection highlights

                DrawLine3D({ 108.015f, 10.45f, rightZ - 0.25f }, { 108.015f, 12.65f, rightZ + 0.35f }, glassSheen);

                DrawLine3D({ 107.985f, 10.45f, rightZ - 0.25f }, { 107.985f, 12.65f, rightZ + 0.35f }, glassSheen);

                // High-contrast white frosted safety horizontal stripes across middle of glass

                DrawCube({ 108.0f, 11.40f, rightZ }, 0.028f, 0.045f, 0.90f, safetyFrost);

                DrawCube({ 108.0f, 12.15f, rightZ }, 0.028f, 0.045f, 0.90f, safetyFrost);

                // Leading edge rubber gasket seal

                DrawCube({ 108.0f, 11.55f, rightZ - 0.46f }, 0.05f, 2.78f, 0.022f, rubberGasket);

                // Stainless steel full-height vertical tubular handle (both interior and exterior sides)

                DrawCylinderEx({ 107.93f, 10.60f, rightZ - 0.36f }, { 107.93f, 12.45f, rightZ - 0.36f }, 0.016f, 0.016f, 8, handleCol);

                DrawCylinderEx({ 108.07f, 10.60f, rightZ - 0.36f }, { 108.07f, 12.45f, rightZ - 0.36f }, 0.016f, 0.016f, 8, handleCol);

            }



            // ---------------------------------------------------------------------

            // 3. SHOP INTERIOR: CHECKERED SLATE FLOOR & WALLS (ZERO SELF-ILLUMINATION)

            // Light comes EXCLUSIVELY from the swinging bulb and ceiling tubelights!

            // ---------------------------------------------------------------------

            Vector3 camPosForShop = (g_lookAtShop || g_lookAtWashroom || g_lookAtAtm || isThirdPerson || isRoofCamActive) ? renderCam.position : camera.position;
            bool canSeeShopInterior = (camPosForShop.x <= 135.0f && camPosForShop.x >= 70.0f &&
                                       camPosForShop.z >= 110.0f && camPosForShop.z <= 175.0f);

            if (canSeeShopInterior) {

            // A. Dark subfloor base slab (Absolute zero self-illumination)

            DrawCube({ 97.0f, 10.005f, 140.0f }, 21.8f, 0.01f, 27.8f, { 8, 8, 10, 255 });



            // Phase 3: High-Fidelity Textured Commercial Checkered Vinyl Floor
            DrawShopAtmosphereFloor(ApplyShopLighting, g_shopLightsOn, timeVal);



            // Dynamic Physical Floor Light Pools & Ground Contact Ambient

            // Completely disabled when master power switch is turned OFF (absolute pitch darkness)

            if (g_shopLightsOn && shopLightIntensity > 0.01f) {

                // 1. Moving Floor Light Pool from the Swaying Tungsten Bulb

                Vector3 bulbFPos = { g_shopLighting.lights[0].pos.x, 10.022f, g_shopLighting.lights[0].pos.z };

                const int bulbSegs = 24;

                float bulbPoolR = 4.8f;

                unsigned char bulbPoolA = (unsigned char)Clamp(85.0f * shopLightIntensity, 0.0f, 255.0f);

                rlBegin(RL_TRIANGLES);

                for (int i = 0; i < bulbSegs; i++) {

                    float a0 = (float)i * (2.0f * PI / (float)bulbSegs);

                    float a1 = (float)(i + 1) * (2.0f * PI / (float)bulbSegs);

                    rlColor4ub(255, 225, 150, bulbPoolA);

                    rlVertex3f(bulbFPos.x, 10.022f, bulbFPos.z);

                    rlColor4ub(255, 205, 120, 0);

                    rlVertex3f(bulbFPos.x + cosf(a0) * bulbPoolR, 10.022f, bulbFPos.z + sinf(a0) * bulbPoolR);

                    rlColor4ub(255, 205, 120, 0);

                    rlVertex3f(bulbFPos.x + cosf(a1) * bulbPoolR, 10.022f, bulbFPos.z + sinf(a1) * bulbPoolR);

                }

                rlEnd();



                // Clean floor contact shadows: soft subtle contact discs beneath heavy fixtures without barrier polygons

                DrawCircle3D(Vector3{ 104.5f, 10.016f, 133.5f }, 2.4f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 8, 10, 12, 55 });

                DrawCircle3D(Vector3{ 94.5f, 10.016f, 133.5f }, 2.2f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 8, 10, 12, 55 });



                // 3. Cold Arctic Cyan Pool around Reach-in Island Freezer

                Vector3 frzFPos = { 94.5f, 10.022f, 133.5f };

                DrawCircle3D(frzFPos, 4.5f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 70, 185, 255, 34 });

                DrawCircle3D(frzFPos, 2.4f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 120, 220, 255, 52 });



                // 4. Warm Task Spotlight Pool on Floor at Checkout Register & Counter

                DrawCircle3D(Vector3{ 104.5f, 10.022f, 134.0f }, 3.2f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 255, 220, 140, 42 });

                DrawCircle3D(Vector3{ 104.5f, 10.022f, 134.0f }, 1.6f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 255, 235, 170, 60 });

            }



            // B. Spacious Empty Entrance Floor (wet sheen reflecting outside rain draft)

            DrawCube({ 104.5f, 10.026f, 140.0f }, 6.0f, 0.005f, 5.0f, ApplyShopLighting({ 104.5f, 10.026f, 140.0f }, { 28, 35, 46, 160 }));



            // Phase 3: High-Fidelity Textured Timber Walls, Water Leaks, Grunge & Storytelling Posters
            DrawShopAtmosphereWalls(ApplyShopLighting, g_shopLightsOn, timeVal);
            DrawShopAtmosphereDetails(ApplyShopLighting, g_shopLightsOn, timeVal);
            DrawShopInteriorProps(ApplyShopLighting, g_shopLightsOn, timeVal);
            DrawShopHauntedWashroom(renderCam, ApplyShopLighting, g_shopLightsOn, timeVal);
            DrawATM3D(ApplyShopLighting, g_shopLightsOn, timeVal);



            // Physical Industrial Wall Light Switch (Mounted on East Interior Wall at X = 107.75, Z = 138.2)

            {

                Vector3 swP = { 107.75f, 11.5f, 138.2f };

                // EMT metallic conduit pipe from ceiling down to switch box

                DrawCylinderEx(Vector3{ swP.x, swP.y + 0.08f, swP.z }, Vector3{ swP.x, 15.18f, swP.z }, 0.012f, 0.012f, 8, Color{ 140, 145, 150, 255 });

                // Cast-aluminum 2-gang electrical junction box

                DrawCube(swP, 0.06f, 0.16f, 0.12f, Color{ 85, 90, 95, 255 });

                // Brushed stainless steel faceplate

                DrawCube(Vector3{ swP.x - 0.015f, swP.y, swP.z }, 0.02f, 0.14f, 0.10f, Color{ 195, 200, 205, 255 });

                // Mechanical toggle switch lever (tilted up for ON, down for OFF)

                float togOffY = g_shopLightsOn ? 0.025f : -0.025f;

                DrawCube(Vector3{ swP.x - 0.035f, swP.y + togOffY, swP.z }, 0.04f, 0.025f, 0.018f, Color{ 30, 32, 35, 255 });

                // Status Indicator LED

                if (g_shopLightsOn) {

                    DrawSphere(Vector3{ swP.x - 0.026f, swP.y + 0.045f, swP.z }, 0.010f, Color{ 35, 245, 80, 255 });

                    DrawSphere(Vector3{ swP.x - 0.026f, swP.y + 0.045f, swP.z }, 0.025f, Color{ 35, 245, 80, 60 });

                } else {

                    float pulse = 0.55f + 0.45f * sinf(timeVal * 3.8f);

                    DrawSphere(Vector3{ swP.x - 0.026f, swP.y + 0.045f, swP.z }, 0.012f, Color{ 255, 35, 25, (unsigned char)(255 * pulse) });

                    DrawSphere(Vector3{ swP.x - 0.026f, swP.y + 0.045f, swP.z }, 0.040f, Color{ 255, 45, 30, (unsigned char)(80 * pulse) });

                }

            }



            // Phase 3: Weathered Acoustic Ceiling Tiles with Water Rings and Rusted Grid Tracks
            DrawShopAtmosphereCeiling(ApplyShopLighting, g_shopLightsOn);
            DrawShopAtmosphereParticles(renderCam, g_shopLightsOn, timeVal);

            float trussZ[3] = { 132.0f, 140.0f, 148.0f };

            for (int t = 0; t < 3; t++) {

                for (float rx = 88.0f; rx <= 106.0f; rx += 4.0f) {

                    DrawCube({ rx, 15.05f, trussZ[t] }, 3.96f, 0.22f, 0.35f, ApplyShopLighting({ rx, 15.05f, trussZ[t] }, { 34, 36, 40, 255 }));

                }

            }



            // ---------------------------------------------------------------------

            // 4. SUSPENDED PENDANT CEILING LIGHT WITH DRAMATIC 2-AXIS SWAY

            // Suspended at X = 95.0, Z = 143.5 directly above & alongside the sculpted meat carcass!

            // ---------------------------------------------------------------------

            {

                Vector3 lightBase = { 95.0f, 15.22f, 143.5f };

                float cordLen = 2.05f;

                Vector3 lightHead = {

                    lightBase.x + sinf(shopLightSwayX) * cordLen,

                    lightBase.y - cosf(shopLightSwayX) * cosf(shopLightSwayZ) * cordLen,

                    lightBase.z + sinf(shopLightSwayZ) * cordLen

                };



                // Heavy ceiling junction box & ceiling canopy

                DrawCube(lightBase, 0.28f, 0.14f, 0.28f, ApplyShopLighting(lightBase, { 30, 32, 36, 255 }));

                DrawCubeWires(lightBase, 0.285f, 0.145f, 0.285f, { 60, 65, 72, 255 });



                // Industrial drop cord

                DrawLine3D(lightBase, lightHead, { 20, 20, 22, 255 });

                DrawCylinderEx(lightBase, lightHead, 0.016f, 0.016f, 6, { 25, 25, 28, 255 });



                // Large spun-metal industrial dome reflector shade (dark olive enamel)

                DrawCylinder(lightHead, 0.38f, 0.14f, 0.24f, 14, { 32, 38, 30, 255 });

                DrawCylinder({ lightHead.x, lightHead.y - 0.05f, lightHead.z }, 0.36f, 0.12f, 0.10f, 14, { 225, 230, 235, 255 }); // Reflective inner cone

                DrawCylinder({ lightHead.x, lightHead.y - 0.12f, lightHead.z }, 0.39f, 0.39f, 0.02f, 14, { 185, 145, 55, 255 }); // Polished brass lip rim



                // Large exposed tungsten filament bulb & brass protective cage

                Color bulbCol = g_shopLightsOn ? 

                    Color{ (unsigned char)Clamp(255.0f * shopLightIntensity, 0.0f, 255.0f), (unsigned char)Clamp(230.0f * shopLightIntensity, 0.0f, 255.0f), (unsigned char)Clamp(140.0f * shopLightIntensity, 0.0f, 255.0f), 255 } :

                    Color{ 30, 32, 36, 255 };

                DrawSphere({ lightHead.x, lightHead.y - 0.10f, lightHead.z }, 0.14f, bulbCol);

                DrawCubeWires({ lightHead.x, lightHead.y - 0.12f, lightHead.z }, 0.32f, 0.32f, 0.32f, { 140, 110, 50, 220 });



                // Warm radiant bloom halo around bulb

                if (g_shopLightsOn) {

                    DrawSphere({ lightHead.x, lightHead.y - 0.10f, lightHead.z }, 0.50f, { 255, 215, 120, (unsigned char)Clamp(45.0f * shopLightIntensity, 0.0f, 255.0f) });

                }



                // Smooth floor spotlight pool swinging across floor in sync with bulb

                Vector3 floorLightCenter = { lightHead.x + sinf(shopLightSwayX) * 2.8f, 10.026f, lightHead.z + sinf(shopLightSwayZ) * 2.8f };

                const int pndSegs = 24;

                float pndR = 5.6f;

                unsigned char pndA = (unsigned char)Clamp(85.0f * shopLightIntensity, 0.0f, 255.0f);

                rlBegin(RL_TRIANGLES);

                for (int i = 0; i < pndSegs; i++) {

                    float a0 = (float)i * (2.0f * PI / (float)pndSegs);

                    float a1 = (float)(i + 1) * (2.0f * PI / (float)pndSegs);

                    rlColor4ub(255, 235, 175, g_shopLightsOn ? pndA : 0);

                    rlVertex3f(floorLightCenter.x, 10.026f, floorLightCenter.z);

                    rlColor4ub(255, 210, 130, 0);

                    rlVertex3f(floorLightCenter.x + cosf(a0) * pndR, 10.026f, floorLightCenter.z + sinf(a0) * pndR);

                    rlColor4ub(255, 210, 130, 0);

                    rlVertex3f(floorLightCenter.x + cosf(a1) * pndR, 10.026f, floorLightCenter.z + sinf(a1) * pndR);

                }

                rlEnd();

            }



            // ---------------------------------------------------------------------

            // 4B. 6-TROFFER INDUSTRIAL CEILING TUBELIGHT GRID

            // All fixtures respond dynamically to the master wall light switch!

            // ---------------------------------------------------------------------

            auto DrawCeilingTroffer = [&](Vector3 tPos, float tLen, Color tTubeCol, bool arcing) {

                DrawCube(tPos, tLen, 0.08f, 0.32f, ApplyShopLighting(tPos, { 48, 50, 55, 255 }));

                DrawCube(Vector3{ tPos.x, tPos.y - 0.02f, tPos.z }, tLen - 0.06f, 0.04f, 0.26f, Color{ 180, 185, 190, 255 });



                Color actualTubeCol = g_shopLightsOn ? tTubeCol : Color{ 28, 30, 34, 255 };

                float halfL = (tLen - 0.24f) * 0.5f;

                DrawCylinderEx(Vector3{ tPos.x - halfL, tPos.y - 0.04f, tPos.z - 0.07f }, Vector3{ tPos.x + halfL, tPos.y - 0.04f, tPos.z - 0.07f }, 0.022f, 0.022f, 8, actualTubeCol);

                DrawCylinderEx(Vector3{ tPos.x - halfL, tPos.y - 0.04f, tPos.z + 0.07f }, Vector3{ tPos.x + halfL, tPos.y - 0.04f, tPos.z + 0.07f }, 0.022f, 0.022f, 8, actualTubeCol);



                if (g_shopLightsOn && arcing) {

                    DrawSphere(Vector3{ tPos.x - halfL, tPos.y - 0.04f, tPos.z }, 0.16f, Color{ 245, 250, 255, 255 });

                    DrawSphere(Vector3{ tPos.x - halfL, tPos.y - 0.04f, tPos.z }, 0.40f, Color{ 160, 210, 255, 160 });

                }

            };



            // Troffer 1: North Aisle (Aisle 3, Z = 150.5, X = 95.0, Length = 3.4m)

            DrawCeilingTroffer(Vector3{ 95.0f, 15.05f, 150.5f }, 3.4f, Color{ 215, 235, 255, 255 }, tube1IsArcing);



            // Troffer 2: Center Aisle (Aisle 2, Z = 143.5, X = 91.5, Length = 2.8m)

            DrawCeilingTroffer(Vector3{ 91.5f, 15.05f, 143.5f }, 2.8f, Color{ 220, 235, 250, 255 }, false);



            // Troffer 3: South Aisle (Aisle 1, Z = 136.5, X = 95.0, Length = 3.4m)

            DrawCeilingTroffer(Vector3{ 95.0f, 15.05f, 136.5f }, 3.4f, Color{ 225, 235, 245, 255 }, false);



            // Troffer 4: Checkout Counter Task Light (Z = 133.5, X = 104.5, Length = 2.8m)

            DrawCeilingTroffer(Vector3{ 104.5f, 15.05f, 133.5f }, 2.8f, Color{ 255, 220, 150, 255 }, false);



            // Troffer 5: Entrance Corridor & Shopping Cart Bay (Z = 143.5, X = 104.5, Length = 2.8m)

            DrawCeilingTroffer(Vector3{ 104.5f, 15.05f, 143.5f }, 2.8f, Color{ 220, 230, 240, 255 }, false);



            // Troffer 6: Rear Storage Corner (Z = 133.5, X = 89.0, Length = 2.4m)

            DrawCeilingTroffer(Vector3{ 89.0f, 15.05f, 133.5f }, 2.4f, Color{ 200, 225, 245, 255 }, false);



            // ---------------------------------------------------------------------

            // 4E. 3D ELECTRICAL SPARK PARTICLES (EMITTED FROM FAULTY TUBELIGHT 1)

            // ---------------------------------------------------------------------

            for (const auto& sp : shopSparks) {

                Vector3 trailStart = Vector3Subtract(sp.pos, Vector3Scale(sp.vel, dt * 1.8f));

                DrawLine3D(trailStart, sp.pos, sp.color);

                DrawSphere(sp.pos, sp.size, sp.color);

                DrawSphere(sp.pos, sp.size * 2.2f, { sp.color.r, sp.color.g, sp.color.b, (unsigned char)(sp.color.a * 0.35f) });

            }



            // ---------------------------------------------------------------------

            // 5. PERFECTLY DESIGNED INDUSTRIAL CEILING FAN

            // Mounted at X = 90.0, Z = 143.5, Y = 14.85

            // ---------------------------------------------------------------------

            {

                Vector3 fanBase = { 90.0f, 15.22f, 143.5f };

                Vector3 motorPos = { 90.0f, 14.72f, 143.5f };



                // Ceiling mount canopy & downrod

                DrawCylinder(fanBase, 0.22f, 0.22f, 0.06f, 12, ApplyShopLighting(fanBase, { 30, 32, 36, 255 }));

                DrawCylinder({ 90.0f, 14.95f, 143.5f }, 0.045f, 0.045f, 0.50f, 8, ApplyShopLighting({ 90.0f, 14.95f, 143.5f }, { 38, 42, 46, 255 }));



                // Cast-iron cylindrical motor housing with brass band

                DrawCylinder(motorPos, 0.28f, 0.28f, 0.18f, 14, ApplyShopLighting(motorPos, { 32, 34, 38, 255 }));

                DrawCylinder({ 90.0f, 14.68f, 143.5f }, 0.29f, 0.29f, 0.03f, 14, ApplyShopLighting(motorPos, { 190, 145, 55, 255 }));

                DrawCylinder({ 90.0f, 14.58f, 143.5f }, 0.18f, 0.18f, 0.08f, 10, ApplyShopLighting(motorPos, { 45, 48, 54, 255 }));



                // 4 Aerodynamic Stamped-Metal Blades

                float rad = shopFanAngle * DEG2RAD;

                for (int b = 0; b < 4; b++) {

                    float ba = rad + (b * 90.0f) * DEG2RAD;

                    Vector3 bDir = { cosf(ba), 0.0f, sinf(ba) };

                    Vector3 bCross = { -sinf(ba), 0.0f, cosf(ba) };



                    Vector3 bArm = Vector3Add(motorPos, Vector3Scale(bDir, 0.35f));

                    DrawLine3D(motorPos, bArm, ApplyShopLighting(bArm, { 65, 70, 78, 255 }));



                    Vector3 bCenter = Vector3Add(motorPos, Vector3Scale(bDir, 0.95f));

                    bCenter.y -= 0.02f;

                    Color bladeCol = ApplyShopLighting(bCenter, { 25, 28, 32, 255 });

                    DrawCube(bCenter, 0.22f * fabsf(bCross.x) + 1.15f * fabsf(bDir.x),

                                      0.015f,

                                      0.22f * fabsf(bCross.z) + 1.15f * fabsf(bDir.z),

                                      bladeCol);

                }



                // Rotating shadow cast on floor

                DrawCircle3D({ 90.0f, 10.026f, 143.5f }, 1.4f, { 1, 0, 0 }, 90.0f, { 10, 10, 14, 45 });

            }



            // ---------------------------------------------------------------------

            // 6. SCULPTED HANGING MEAT CARCASS & HYPER-REALISTIC BLOOD FLUID DYNAMICS

            // Suspended from central ceiling girder at X = 95.0, Z = 143.5, Y = 12.28 .. 14.85

            // ---------------------------------------------------------------------

            {

                Vector3 meatMount = { 95.0f, 15.05f, 143.5f };



                // Overhead steel I-beam rafter & ceiling trolley

                DrawCube(meatMount, 2.4f, 0.18f, 0.32f, ApplyShopLighting(meatMount, { 35, 38, 42, 255 }));

                DrawCube({ 95.0f, 14.94f, 143.5f }, 0.28f, 0.08f, 0.25f, ApplyShopLighting(meatMount, { 25, 28, 30, 255 }));



                // Blackened forged iron chain links

                DrawLine3D({ 95.0f, 14.90f, 143.5f }, { 95.0f, 14.20f, 143.5f }, { 35, 36, 40, 255 });

                for (float cy = 14.85f; cy >= 14.25f; cy -= 0.12f) {

                    DrawCube({ 95.0f, cy, 143.5f }, 0.06f, 0.09f, 0.04f, ApplyShopLighting({ 95.0f, cy, 143.5f }, { 30, 32, 36, 255 }));

                }



                // Forged iron meat hook (penetrating carcass top knuckle)

                DrawCylinderEx({ 95.0f, 14.20f, 143.5f }, { 95.0f, 13.95f, 143.5f }, 0.028f, 0.020f, 8, { 68, 72, 78, 255 });

                DrawCylinderEx({ 95.0f, 13.95f, 143.5f }, { 95.08f, 13.88f, 143.5f }, 0.020f, 0.015f, 8, { 68, 72, 78, 255 });



                // Upper tendon knuckle & exposed bone joint

                DrawCylinder({ 95.0f, 13.88f, 143.5f }, 0.095f, 0.14f, 0.25f, 8, ApplyShopLighting({ 95.0f, 13.88f, 143.5f }, { 225, 220, 205, 255 }));

                DrawCube({ 95.0f, 13.82f, 143.5f }, 0.32f, 0.12f, 0.22f, ApplyShopLighting({ 95.0f, 13.82f, 143.5f }, { 210, 205, 195, 255 }));



                // Sculpted Carcass Muscle Mass & Rib Curvature

                // Receives intense dynamic illumination from the swaying tungsten bulb right beside it

                Vector3 meatPos = { 95.0f, 13.45f, 143.5f };

                Color meatCore  = ApplyShopLighting(meatPos, { 220, 36, 44, 255 });

                Color meatDark  = ApplyShopLighting(meatPos, { 150, 20, 26, 255 });

                Color fatStripe = ApplyShopLighting(meatPos, { 245, 238, 220, 255 });



                // Upper chest & flank volume

                DrawCube({ 95.0f, 13.45f, 143.5f }, 0.62f, 0.65f, 0.38f, meatCore);

                DrawCube({ 94.98f, 13.45f, 143.5f }, 0.35f, 0.60f, 0.40f, meatDark);

                DrawCube({ 95.12f, 13.42f, 143.4f }, 0.08f, 0.55f, 0.18f, fatStripe);

                DrawCube({ 94.88f, 13.48f, 143.6f }, 0.07f, 0.50f, 0.18f, fatStripe);



                // Protruding rib bone arches

                for (int r = 0; r < 4; r++) {

                    float ry = 13.60f - r * 0.12f;

                    DrawCylinderEx({ 95.18f + r * 0.08f, ry, 143.35f }, { 95.22f + r * 0.08f, ry - 0.04f, 143.25f }, 0.020f, 0.015f, 6, ApplyShopLighting({ 95.2f, ry, 143.3f }, { 215, 210, 195, 255 }));

                }



                // Mid-to-lower tapered flank

                DrawCube({ 95.0f, 12.85f, 143.5f }, 0.48f, 0.58f, 0.30f, meatCore);

                DrawCube({ 95.05f, 12.82f, 143.45f }, 0.22f, 0.50f, 0.24f, fatStripe);



                // Severed lower muscle fibers & tapering tips

                DrawCylinderEx({ 95.0f, 12.56f, 143.5f }, { 95.0f, 12.28f, 143.5f }, 0.12f, 0.02f, 8, meatCore);

                DrawCylinderEx({ 95.14f, 12.65f, 143.25f }, { 95.14f, 12.38f, 143.25f }, 0.08f, 0.015f, 8, meatDark);



                // Glistening wet specular highlights catching overhead lights

                float meatLitFactor = GetShopLightFactorAt({ 95.0f, 13.0f, 143.5f });

                DrawSphere({ 95.18f, 13.35f, 143.58f }, 0.08f, { 190, 45, 55, (unsigned char)Clamp(160.0f * meatLitFactor, 0.0f, 255.0f) });

                DrawSphere({ 94.92f, 12.85f, 143.38f }, 0.07f, { 190, 45, 55, (unsigned char)Clamp(160.0f * meatLitFactor, 0.0f, 255.0f) });



                // --- FORMING BLOOD BEADS AT DRIP TIPS ---

                float prog1 = Clamp(bloodDripTimer1 / bloodDripThreshold1, 0.0f, 1.0f);

                float beadR1 = 0.014f + prog1 * 0.032f;

                float beadY1 = 12.28f - beadR1 * (1.0f + prog1 * 1.5f);

                DrawSphere({ 95.0f, beadY1, 143.5f }, beadR1, { 130, 6, 12, 255 });

                DrawCylinderEx({ 95.0f, 12.28f, 143.5f }, { 95.0f, beadY1, 143.5f }, 0.007f, 0.002f, 6, { 110, 4, 8, 230 });



                float prog2 = Clamp(bloodDripTimer2 / bloodDripThreshold2, 0.0f, 1.0f);

                float beadR2 = 0.012f + prog2 * 0.026f;

                float beadY2 = 12.38f - beadR2 * (1.0f + prog2 * 1.4f);

                DrawSphere({ 95.14f, beadY2, 143.25f }, beadR2, { 120, 5, 10, 255 });

                DrawCylinderEx({ 95.14f, 12.38f, 143.25f }, { 95.14f, beadY2, 143.25f }, 0.006f, 0.002f, 6, { 100, 4, 8, 220 });



                // Falling blood drops

                for (const auto& d : bloodDrops) {

                    if (!d.active) continue;

                    Vector3 topPt = { d.pos.x, d.pos.y + d.length, d.pos.z };

                    DrawCapsule(d.pos, topPt, d.size, 6, 6, { 135, 6, 14, 255 });

                    DrawSphere(d.pos, d.size * 0.45f, { 240, 70, 80, 220 });

                }



                // Micro splatters

                for (const auto& s : bloodSplatters) {

                    float sFrac = s.life / s.maxLife;

                    DrawSphere(s.pos, s.size * sFrac, { 125, 5, 12, (unsigned char)(255 * sFrac) });

                }



                // Floor blood pool with expanding ripples

                Vector3 poolCenter = { 95.0f, 10.024f, 143.5f };

                DrawCircle3D(poolCenter, 1.55f, { 1, 0, 0 }, 90.0f, { 85, 6, 8, 170 });

                DrawCircle3D(poolCenter, 1.15f, { 1, 0, 0 }, 90.0f, { 115, 6, 12, 240 });

                DrawCircle3D(poolCenter, 0.75f, { 1, 0, 0 }, 90.0f, { 50, 2, 4, 255 });



                for (const auto& rip : bloodRipples) {

                    DrawCircle3D(rip.center, rip.radius, { 1, 0, 0 }, 90.0f, { 165, 25, 35, (unsigned char)(rip.alpha * 190) });

                }



                DrawCircle3D({ 96.2f, 10.025f, 144.1f }, 0.18f, { 1, 0, 0 }, 90.0f, { 95, 6, 10, 220 });

                DrawCircle3D({ 94.1f, 10.025f, 142.8f }, 0.22f, { 1, 0, 0 }, 90.0f, { 95, 6, 10, 220 });

                DrawCircle3D({ 95.6f, 10.025f, 144.9f }, 0.14f, { 1, 0, 0 }, 90.0f, { 95, 6, 10, 220 });

                DrawCircle3D({ 94.4f, 10.025f, 144.6f }, 0.16f, { 1, 0, 0 }, 90.0f, { 95, 6, 10, 220 });

            }





            // ---------------------------------------------------------------------

            // 7. EXACTLY TWO VERTICAL COMMERCIAL SHELVING RACKS (RACK 1 & RACK 2)

            // Running vertically along X from 89.0 to 101.0 (Length: 12m)

            // Lit dynamically in 4m bays using ApplyShopLighting

            // ---------------------------------------------------------------------

            {

                // RACK 1 (Z = 140.0, DOUBLE-SIDED: shelves on North +Z and South -Z)

                {

                    float rz = 140.0f;

                    for (float px = 89.0f; px <= 101.0f; px += 4.0f) {

                        Vector3 postPos = { px, 12.0f, rz };

                        DrawCube(postPos, 0.12f, 4.0f, 0.12f, ApplyShopLighting(postPos, { 32, 34, 38, 255 }));

                        DrawCubeWires(postPos, 0.125f, 4.02f, 0.125f, { 70, 75, 82, 255 });

                    }



                    bool drawNorth1 = (camera.position.z >= 139.8f || camera.position.x < 89.0f || camera.position.x > 101.0f);

                    bool drawSouth1 = (camera.position.z <= 140.2f || camera.position.x < 89.0f || camera.position.x > 101.0f);



                    // 3 bays along X: [89..93], [93..97], [97..101]

                    for (float bx = 91.0f; bx <= 99.0f; bx += 4.0f) {

                        Vector3 bayCenter = { bx, 12.0f, rz };

                        Color bayBacking = ApplyShopLighting(bayCenter, { 25, 27, 30, 255 });



                        // Central spine / divider wall

                        DrawCube({ bx, 12.0f, rz }, 4.0f, 3.8f, 0.04f, bayBacking);



                        // --- North Side Shelves (+Z: Aisle 2, facing swaying bulb) ---

                        if (drawNorth1) {

                            Vector3 pN = { bx, 12.0f, rz + 0.45f };

                            Color shelfN = ApplyShopLighting(pN, { 210, 206, 198, 255 }, Vector3{ 0, 1, 0 });

                            Color trimN  = ApplyShopLighting(pN, { 58, 62, 70, 255 }, Vector3{ 0, 0, 1 });

                            Color dustN  = ApplyShopLighting(pN, { 145, 138, 125, 180 }, Vector3{ 0, 1, 0 });

                            DrawCube({ bx, 10.35f, rz + 0.45f }, 4.0f, 0.08f, 0.90f, shelfN);

                            DrawCube({ bx, 10.35f, rz + 0.89f }, 4.0f, 0.09f, 0.02f, trimN);

                            DrawCube({ bx, 11.40f, rz + 0.42f }, 3.96f, 0.05f, 0.84f, shelfN);

                            DrawCube({ bx, 11.40f, rz + 0.83f }, 3.96f, 0.06f, 0.02f, trimN);

                            DrawCube({ bx, 12.45f, rz + 0.42f }, 3.96f, 0.05f, 0.84f, shelfN);

                            DrawCube({ bx, 12.45f, rz + 0.83f }, 3.96f, 0.06f, 0.02f, trimN);

                            DrawCube({ bx, 13.50f, rz + 0.42f }, 3.96f, 0.05f, 0.84f, shelfN);

                            DrawCube({ bx, 13.50f, rz + 0.83f }, 3.96f, 0.06f, 0.02f, trimN);

                            DrawCube({ bx, 13.53f, rz + 0.42f }, 3.92f, 0.005f, 0.82f, dustN);

                        }



                        // --- South Side Shelves (-Z: Aisle 1, occluded from central bulb by spine) ---

                        if (drawSouth1) {

                            Vector3 pS = { bx, 12.0f, rz - 0.45f };

                            Color shelfS = ApplyShopLighting(pS, { 210, 206, 198, 255 }, Vector3{ 0, 1, 0 }, 1);

                            Color trimS  = ApplyShopLighting(pS, { 58, 62, 70, 255 }, Vector3{ 0, 0, -1 }, 1);

                            Color dustS  = ApplyShopLighting(pS, { 145, 138, 125, 180 }, Vector3{ 0, 1, 0 }, 1);

                            DrawCube({ bx, 10.35f, rz - 0.45f }, 4.0f, 0.08f, 0.90f, shelfS);

                            DrawCube({ bx, 10.35f, rz - 0.89f }, 4.0f, 0.09f, 0.02f, trimS);

                            DrawCube({ bx, 11.40f, rz - 0.42f }, 3.96f, 0.05f, 0.84f, shelfS);

                            DrawCube({ bx, 11.40f, rz - 0.83f }, 3.96f, 0.06f, 0.02f, trimS);

                            DrawCube({ bx, 12.45f, rz - 0.42f }, 3.96f, 0.05f, 0.84f, shelfS);

                            DrawCube({ bx, 12.45f, rz - 0.83f }, 3.96f, 0.06f, 0.02f, trimS);

                            DrawCube({ bx, 13.50f, rz - 0.42f }, 3.96f, 0.05f, 0.84f, shelfS);

                            DrawCube({ bx, 13.50f, rz - 0.83f }, 3.96f, 0.06f, 0.02f, trimS);

                            DrawCube({ bx, 13.53f, rz - 0.42f }, 3.92f, 0.005f, 0.82f, dustS);

                        }

                    }

                    // Endcaps (Spanning both sides, width 1.82m)

                    DrawCube({ 88.95f, 12.0f, rz }, 0.06f, 3.9f, 1.82f, ApplyShopLighting({ 88.95f, 12.0f, rz }, { 32, 34, 38, 255 }));

                    DrawCube({ 101.05f, 12.0f, rz }, 0.06f, 3.9f, 1.82f, ApplyShopLighting({ 101.05f, 12.0f, rz }, { 32, 34, 38, 255 }));

                }



                // RACK 2 (Z = 147.0, DOUBLE-SIDED: shelves on North +Z and South -Z)

                {

                    float rz = 147.0f;

                    for (float px = 89.0f; px <= 101.0f; px += 4.0f) {

                        Vector3 postPos = { px, 12.0f, rz };

                        DrawCube(postPos, 0.12f, 4.0f, 0.12f, ApplyShopLighting(postPos, { 32, 34, 38, 255 }));

                        DrawCubeWires(postPos, 0.125f, 4.02f, 0.125f, { 70, 75, 82, 255 });

                    }



                    bool drawNorth2 = (camera.position.z >= 146.8f || camera.position.x < 89.0f || camera.position.x > 101.0f);

                    bool drawSouth2 = (camera.position.z <= 147.2f || camera.position.x < 89.0f || camera.position.x > 101.0f);



                    // 3 bays along X: [89..93], [93..97], [97..101]

                    for (float bx = 91.0f; bx <= 99.0f; bx += 4.0f) {

                        Vector3 bayCenter = { bx, 12.0f, rz };

                        Color bayBacking = ApplyShopLighting(bayCenter, { 25, 27, 30, 255 });



                        // Central spine / divider wall

                        DrawCube({ bx, 12.0f, rz }, 4.0f, 3.8f, 0.04f, bayBacking);



                        // --- South Side Shelves (-Z: Aisle 2, facing swaying bulb) ---

                        if (drawSouth2) {

                            Vector3 pS = { bx, 12.0f, rz - 0.45f };

                            Color shelfS = ApplyShopLighting(pS, { 210, 206, 198, 255 }, Vector3{ 0, 1, 0 });

                            Color trimS  = ApplyShopLighting(pS, { 58, 62, 70, 255 }, Vector3{ 0, 0, -1 });

                            Color dustS  = ApplyShopLighting(pS, { 145, 138, 125, 180 }, Vector3{ 0, 1, 0 });

                            DrawCube({ bx, 10.35f, rz - 0.45f }, 4.0f, 0.08f, 0.90f, shelfS);

                            DrawCube({ bx, 10.35f, rz - 0.89f }, 4.0f, 0.09f, 0.02f, trimS);

                            DrawCube({ bx, 11.40f, rz - 0.42f }, 3.96f, 0.05f, 0.84f, shelfS);

                            DrawCube({ bx, 11.40f, rz - 0.83f }, 3.96f, 0.06f, 0.02f, trimS);

                            DrawCube({ bx, 12.45f, rz - 0.42f }, 3.96f, 0.05f, 0.84f, shelfS);

                            DrawCube({ bx, 12.45f, rz - 0.83f }, 3.96f, 0.06f, 0.02f, trimS);

                            DrawCube({ bx, 13.50f, rz - 0.42f }, 3.96f, 0.05f, 0.84f, shelfS);

                            DrawCube({ bx, 13.50f, rz - 0.83f }, 3.96f, 0.06f, 0.02f, trimS);

                            DrawCube({ bx, 13.53f, rz - 0.42f }, 3.92f, 0.005f, 0.82f, dustS);

                        }



                        // --- North Side Shelves (+Z: Aisle 3, occluded from central bulb by spine) ---

                        if (drawNorth2) {

                            Vector3 pN = { bx, 12.0f, rz + 0.45f };

                            Color shelfN = ApplyShopLighting(pN, { 210, 206, 198, 255 }, Vector3{ 0, 1, 0 }, 2);

                            Color trimN  = ApplyShopLighting(pN, { 58, 62, 70, 255 }, Vector3{ 0, 0, 1 }, 2);

                            Color dustN  = ApplyShopLighting(pN, { 145, 138, 125, 180 }, Vector3{ 0, 1, 0 }, 2);

                            DrawCube({ bx, 10.35f, rz + 0.45f }, 4.0f, 0.08f, 0.90f, shelfN);

                            DrawCube({ bx, 10.35f, rz + 0.89f }, 4.0f, 0.09f, 0.02f, trimN);

                            DrawCube({ bx, 11.40f, rz + 0.42f }, 3.96f, 0.05f, 0.84f, shelfN);

                            DrawCube({ bx, 11.40f, rz + 0.83f }, 3.96f, 0.06f, 0.02f, trimN);

                            DrawCube({ bx, 12.45f, rz + 0.42f }, 3.96f, 0.05f, 0.84f, shelfN);

                            DrawCube({ bx, 12.45f, rz + 0.83f }, 3.96f, 0.06f, 0.02f, trimN);

                            DrawCube({ bx, 13.50f, rz + 0.42f }, 3.96f, 0.05f, 0.84f, shelfN);

                            DrawCube({ bx, 13.50f, rz + 0.83f }, 3.96f, 0.06f, 0.02f, trimN);

                            DrawCube({ bx, 13.53f, rz + 0.42f }, 3.92f, 0.005f, 0.82f, dustN);

                        }

                    }

                    // Endcaps (Spanning both sides, width 1.82m)

                    DrawCube({ 88.95f, 12.0f, rz }, 0.06f, 3.9f, 1.82f, ApplyShopLighting({ 88.95f, 12.0f, rz }, { 32, 34, 38, 255 }));

                    DrawCube({ 101.05f, 12.0f, rz }, 0.06f, 3.9f, 1.82f, ApplyShopLighting({ 101.05f, 12.0f, rz }, { 32, 34, 38, 255 }));

                }

            }



// (Shovel relocated to outside near carpet)



            // ---------------------------------------------------------------------

            // 7B. COMMERCIAL HORIZONTAL ISLAND REFRIGERATOR / FREEZER

            // Centered at X = 94.5, Z = 133.5, Y = 10.015

            // ---------------------------------------------------------------------

            DrawHorizontalRefrigerator(Vector3{ 94.5f, 10.015f, 133.5f }, camera);



            // (Products rendered globally outside culling block to prevent disappearing)



            // ---------------------------------------------------------------------

            // 9. CHECKOUT COUNTER & MR. GRETHNAR WOULE

            // Start Left Corner (X = 104.5, Z = 133.5), Table Facing Right (+Z)

            // Illuminated by Dim Tubelight 2 directly overhead

            // ---------------------------------------------------------------------

            {

                // Counter Base Cabinet

                Vector3 counterCenter = { 104.5f, 10.75f, 133.5f };

                DrawCube(counterCenter, 5.0f, 1.5f, 0.80f, ApplyShopLighting(counterCenter, { 42, 30, 22, 255 }));

                DrawCubeWires(counterCenter, 5.02f, 1.51f, 0.81f, ApplyShopLighting(counterCenter, { 62, 44, 32, 255 }));



                // Countertop Slab (Facing Right +Z toward entrance walkway & store)

                Vector3 slabCenter = { 104.5f, 11.52f, 133.5f };

                DrawCube(slabCenter, 5.2f, 0.08f, 0.90f, ApplyShopLighting(slabCenter, { 78, 54, 38, 255 }));

                DrawCubeWires(slabCenter, 5.21f, 0.09f, 0.91f, ApplyShopLighting(slabCenter, { 105, 75, 52, 255 }));



                // L-Section Side Wing

                Vector3 wingCenter = { 101.9f, 11.52f, 132.2f };

                DrawCube(wingCenter, 0.80f, 0.08f, 1.8f, ApplyShopLighting(wingCenter, { 78, 54, 38, 255 }));

                DrawCubeWires(wingCenter, 0.81f, 0.09f, 1.81f, ApplyShopLighting(wingCenter, { 105, 75, 52, 255 }));



                // Vintage Brass Cash Register (Mounted facing Right +Z)

                Vector3 regPos = { 105.8f, 11.62f, 133.5f };

                DrawCube(regPos, 0.46f, 0.14f, 0.42f, ApplyShopLighting(regPos, { 115, 90, 48, 255 }));

                DrawCube({ 105.8f, 11.76f, 133.46f }, 0.42f, 0.20f, 0.32f, ApplyShopLighting(regPos, { 140, 110, 58, 255 }));

                DrawCube({ 105.8f, 11.92f, 133.44f }, 0.22f, 0.12f, 0.04f, ApplyShopLighting(regPos, { 225, 215, 185, 255 }));

                DrawCubeWires({ 105.8f, 11.76f, 133.46f }, 0.43f, 0.21f, 0.33f, ApplyShopLighting(regPos, { 195, 155, 80, 255 }));



                // Midnight Security Monitor (CCTV CRT on Checkout Counter)
                Vector3 crtPos = { 104.2f, 11.78f, 133.5f };
                DrawCounterSecurityMonitor(crtPos, ApplyShopLighting, g_shopLightsOn, timeVal);



                // 3D Countertop Horror Receipt Printer & Curling Paper

                Vector3 printerPos = { 106.3f, 11.56f, 133.5f };

                float ledGlow = 0.6f + 0.4f * sinf(timeVal * 3.0f);

                DrawPrinter(printerPos, ledGlow, timeVal, [](Vector3 p, Color c) { return ApplyShopLighting(p, c); });



                Vector3 slotPos = { printerPos.x, printerPos.y + 0.22f, printerPos.z + 0.18f };

                if (g_printerState == PRINTER_PRINTING || g_printerState == PRINTER_DONE) {

                    DrawReceiptChain(slotPos, g_receiptTex, g_printerProgress, timeVal);

                }



                // Thrown receipt lying on floor

                if (g_receiptThrown) {

                    DrawReceiptChain(g_thrownReceiptPos, g_receiptTex, 16.0f, 0.0f);

                }



                // Nocturnal Autonomous Ghost Shopping Cart

                if (g_ghostCart.alpha > 0.01f) {

                    // Eerie glowing phantom floor circles under cart

                    DrawCircle3D(g_ghostCart.pos, 1.4f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 30, 220, 200, (unsigned char)(65 * g_ghostCart.alpha) });

                    DrawCircle3D(g_ghostCart.pos, 0.8f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 90, 255, 235, (unsigned char)(95 * g_ghostCart.alpha) });



                    // Floating ectoplasm particles drifting up from the cart basket

                    for (int p = 0; p < 3; p++) {

                        float pTime = timeVal * 2.2f + (float)p * 1.35f;

                        float spkX = g_ghostCart.pos.x + sinf(pTime * 2.8f) * 0.22f;
                        float spkY = g_ghostCart.pos.y + 0.6f + fmodf(pTime * 0.8f, 0.9f);
                        float spkZ = g_ghostCart.pos.z + cosf(pTime * 3.3f) * 0.22f;
                        float pAlpha = (1.0f - fmodf(pTime * 0.8f, 0.9f) / 0.9f) * g_ghostCart.alpha;
                        DrawSphere(Vector3{ spkX, spkY, spkZ }, 0.032f, Color{ 120, 255, 235, (unsigned char)(210 * pAlpha) });

                    }



                    rlPushMatrix();

                        rlTranslatef(g_ghostCart.pos.x, g_ghostCart.pos.y, g_ghostCart.pos.z);

                        rlRotatef(g_ghostCart.yaw, 0, 1, 0);

                        DrawGhostShoppingCartLocal(g_ghostCart.wheelSpin, g_ghostCart.pos, g_ghostCart.alpha, timeVal);

                        if (g_ghostCart.itemsInCart >= 1) {

                            DrawCube(Vector3{ -0.10f, 0.62f, 0.0f }, 0.18f, 0.12f, 0.22f, Color{ 100, 255, 220, (unsigned char)(200 * g_ghostCart.alpha) });

                        }

                        if (g_ghostCart.itemsInCart >= 2) {

                            DrawCube(Vector3{  0.10f, 0.62f, 0.05f }, 0.12f, 0.22f, 0.12f, Color{ 80, 220, 255, (unsigned char)(200 * g_ghostCart.alpha) });

                        }

                    rlPopMatrix();

                }



                // Shopkeeper: Mr. Grethnar Woule behind counter at X: 104.5, Y: 10.0, Z: 131.8 (Facing Right +Z)

                // When vanished or jumpscaring, the counter is completely empty under the warm spotlight!

                if (grethnarState != GRETHNAR_VANISHED && grethnarState != GRETHNAR_JUMPSCARE) {

                    Vector3 grethnarPos = { 104.5f, 10.0f, 131.8f };

                    DrawCylinder(grethnarPos, 0.22f, 0.27f, 1.95f, 14, ApplyShopLighting(grethnarPos, { 16, 16, 18, 255 }));

                    DrawCube({ 104.5f, 11.85f, 131.8f }, 0.78f, 0.18f, 0.36f, ApplyShopLighting({ 104.5f, 11.85f, 131.8f }, { 20, 20, 24, 255 }));



                    // Drooping long arms

                    DrawCylinderEx({ 104.12f, 11.75f, 131.8f }, { 104.12f, 10.60f, 131.8f }, 0.055f, 0.045f, 8, ApplyShopLighting({ 104.12f, 11.2f, 131.8f }, { 18, 18, 20, 255 }));

                    DrawSphere({ 104.12f, 10.55f, 131.8f }, 0.05f, ApplyShopLighting({ 104.12f, 10.55f, 131.8f }, { 220, 220, 215, 255 }));

                    DrawCylinderEx({ 104.88f, 11.75f, 131.8f }, { 104.88f, 10.60f, 131.8f }, 0.055f, 0.045f, 8, ApplyShopLighting({ 104.88f, 11.2f, 131.8f }, { 18, 18, 20, 255 }));

                    DrawSphere({ 104.88f, 10.55f, 131.8f }, 0.05f, ApplyShopLighting({ 104.88f, 10.55f, 131.8f }, { 220, 220, 215, 255 }));



                    float tiltRoll  = 0.0f;

                    float tiltPitch = 0.0f;

                    float tiltYaw   = 0.0f;



                    if (playerInShop) {

                        // As soon as the player enters the shop: locked-in unblinking death stare tracking the player!

                        float dx = camera.position.x - 104.5f;

                        float dy = camera.position.y - 12.19f;

                        float dz = camera.position.z - 131.8f;

                        float distXZ = sqrtf(dx * dx + dz * dz);

                        tiltYaw = atan2f(dx, dz) * RAD2DEG;

                        tiltPitch = -atan2f(dy, distXZ) * RAD2DEG;

                        tiltRoll = 0.0f;

                    } else {

                        // Outside shop: subtle idle posture

                        float snapCycle = fmodf(timeVal, 5.5f);

                        if (snapCycle < 2.2f) {

                            tiltRoll = sinf(timeVal * 1.6f) * 6.0f;

                            tiltPitch = sinf(timeVal * 1.2f) * 2.5f;

                        } else if (snapCycle < 2.45f) {

                            tiltRoll = 18.0f;

                        } else if (snapCycle < 4.4f) {

                            tiltRoll = 18.0f + sinf(timeVal * 10.0f) * 0.8f;

                            tiltPitch = -3.0f;

                        } else {

                            tiltRoll = sinf(timeVal * 2.2f) * 3.0f;

                        }

                    }



                    rlPushMatrix();

                    rlTranslatef(104.5f, 11.95f, 131.8f);

                    rlRotatef(tiltYaw, 0.0f, 1.0f, 0.0f);

                    rlRotatef(tiltRoll, 0.0f, 0.0f, 1.0f);

                    rlRotatef(tiltPitch, 1.0f, 0.0f, 0.0f);



                    // Pale spherical head (The warm tubelight illuminates the counter, BUT face stays pure stark white!)

                    Color headCol = { 250, 250, 248, 255 }; // Stark corpse-white (NOT yellowed!)

                    Color neckCol = { 230, 230, 226, 255 };

                    DrawCylinder({ 0.0f, -0.04f, 0.0f }, 0.10f, 0.10f, 0.14f, 10, neckCol);

                    DrawSphere({ 0.0f, 0.24f, 0.0f }, 0.24f, headCol);

                    DrawSphereWires({ 0.0f, 0.24f, 0.0f }, 0.242f, 12, 12, { 180, 180, 178, 110 });



                    // Stretched horizontal void mouth (facing +Z)

                    DrawCube({ 0.0f, 0.155f, 0.225f }, 0.22f, 0.035f, 0.03f, { 8, 8, 10, 255 });



                    // Eyeballs & Pupils (Normal pale/dark by default; ONLY turn red and bloody when stared at >= 5s!)

                    float curEyeRad = 0.042f * grethnarEyeScale; // Normal 0.042m, max 0.0495m (subtle!)

                    bool isBloody = (grethnarBloodIntensity > 0.01f);



                    Color eyeballCol = isBloody ? Color{ 255, 18, 22, 255 } : Color{ 225, 225, 220, 255 };

                    Color pupilCol   = isBloody ? Color{ 20, 0, 0, 255 }    : Color{ 22, 24, 28, 255 };



                    // "Outer sclerae become engorged with dark crimson throbbing veins." (ONLY when bloody / >= 5s)

                    if (isBloody) {

                        float throb = sinf(timeVal * 16.0f) * 0.15f + 0.85f;

                        Color engorgedSclera = { (unsigned char)(145 * throb), 8, 12, 255 };

                        Color veinCol = { (unsigned char)(85 * throb), 4, 6, 255 };



                        // Bulging outer sclera spheres & vein lattices

                        DrawSphere({ -0.09f, 0.27f, 0.222f }, curEyeRad * 1.08f, engorgedSclera);

                        DrawSphere({  0.09f, 0.27f, 0.222f }, curEyeRad * 1.08f, engorgedSclera);

                        DrawSphereWires({ -0.09f, 0.27f, 0.222f }, curEyeRad * 1.10f, 8, 8, veinCol);

                        DrawSphereWires({  0.09f, 0.27f, 0.222f }, curEyeRad * 1.10f, 8, 8, veinCol);



                        // Branching micro-vein lines across eyes

                        for (int v = 0; v < 6; v++) {

                            float ang = v * 60.0f * DEG2RAD;

                            float vx = cosf(ang) * curEyeRad * 1.10f;

                            float vy = sinf(ang) * curEyeRad * 1.10f;

                            DrawLine3D({ -0.09f, 0.27f, 0.22f }, { -0.09f + vx, 0.27f + vy, 0.222f }, veinCol);

                            DrawLine3D({  0.09f, 0.27f, 0.22f }, {  0.09f + vx, 0.27f + vy, 0.222f }, veinCol);

                        }

                    }



                    // Irises & Pupils

                    DrawSphere({ -0.09f, 0.27f, 0.222f }, curEyeRad, eyeballCol);

                    DrawSphere({  0.09f, 0.27f, 0.222f }, curEyeRad, eyeballCol);

                    DrawSphere({ -0.09f, 0.27f, 0.222f + curEyeRad * 0.72f }, curEyeRad * 0.38f, pupilCol);

                    DrawSphere({  0.09f, 0.27f, 0.222f + curEyeRad * 0.72f }, curEyeRad * 0.38f, pupilCol);



                    // Arterial blood weeping from eyes (ONLY when bloody)

                    if (isBloody) {

                        float blLen = grethnarBloodIntensity * 0.16f;

                        Color bCol = { 135, 8, 14, 255 };

                        DrawCylinderEx({ -0.09f, 0.27f, 0.235f }, { -0.09f, 0.27f - blLen, 0.230f }, 0.012f * grethnarBloodIntensity, 0.006f, 6, bCol);

                        DrawCylinderEx({  0.09f, 0.27f, 0.235f }, {  0.09f, 0.27f - blLen, 0.230f }, 0.012f * grethnarBloodIntensity, 0.006f, 6, bCol);

                    }



                    rlPopMatrix();

                }



                // Render hyper-realistic fluid trails connecting the falling '~' blood droplets down to floor

                for (const auto& bd : grethnarBloodDrops) {

                    float a = Clamp(bd.life / 0.5f, 0.0f, 1.0f);

                    if (bd.pos.y > 10.035f) {

                        DrawLine3D(bd.prevPos, bd.pos, { 145, 6, 12, (unsigned char)(220 * a) });

                    } else {

                        // Micro splatter puddle on the floor

                        DrawCube({ bd.pos.x, 10.022f, bd.pos.z }, 0.075f, 0.002f, 0.075f, { 115, 4, 8, (unsigned char)(200 * a) });

                    }

                }

            }

            } // end if (canSeeShopInterior)



            // ---------------------------------------------------------------------

            // 8. RENDER SUPERSTORE PRODUCTS, SHOPPING CART & PARTICLES

            // Rendered globally so held viewmodels (gun, bottles, tin) never vanish outside!

            // ---------------------------------------------------------------------

            DrawShopProductsAndParticles(camera, walkTime, bobAmplitude, dt);



            // Render physically simulated dirt clods

            DrawDirtClods();



            // Render First-Person Shovel Viewmodel (Tucked low-right, zero crosshair blockage)

            if (holdingShovel && !isRoofCamActive && g_gameState == STATE_GAMEPLAY) {

                Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

                Vector3 camRight = Vector3Normalize(Vector3CrossProduct(fwd, camera.up));

                Vector3 camUp = Vector3Normalize(Vector3CrossProduct(camRight, fwd));



                float swayX = Clamp(g_vmSwayX * 0.05f, -0.05f, 0.05f);

                float swayY = Clamp(g_vmSwayY * 0.05f, -0.04f, 0.04f);

                float stepBobY = (walkTime > 0.0f) ? sinf(walkTime * 2.0f) * 0.012f : 0.0f;



                // Primary rear hand anchor holding the shovel grip

                Vector3 handAnchor = Vector3Add(camera.position,
                    Vector3Add(Vector3Scale(camRight, 0.25f + swayX),
                               Vector3Add(Vector3Scale(camUp, -0.24f + swayY + stepBobY),
                                          Vector3Scale(fwd, 0.36f))));

                ShovelPose pose = GetAnimatedShovelPose(g_shovelAnimState, g_shovelAnimTime, g_shovelIdleClock);

                Matrix shovelWorld = ShovelPoseToWorldMatrix(pose, handAnchor, camRight, camUp, fwd);



                // Lighting direction

                Vector3 lDir = g_flashlightActive ? fwd : (g_curExtDayFactor > 0.2f ? g_curSunDir : Vector3{ -0.3f, -1.0f, -0.2f });

                DrawShovel(g_shovelRig, shovelWorld, camera.position, lDir);



                // Blade tip in world coordinates (Local Z = g_shovelTipLocalZ = 0.98m)

                Vector3 tipLocal = { 0, 0, g_shovelTipLocalZ };

                Vector3 tipWorld = Vector3Transform(tipLocal, shovelWorld);



                if (g_shovelAnimState == SHOVEL_ANIM_DIG) {

                    float t = Clamp(g_shovelAnimTime / SHOVEL_DIG_DURATION, 0.0f, 1.0f);

                    // 1. Initial downward blade penetration into soil

                    if (!g_shovelDigImpactDone && t >= SHOVEL_DIG_IMPACT_T) {

                        g_shovelDigImpactDone = true;

                        PlaySound(g_sndShovelDig);

                        g_camLandingDip = -0.16f; // Solid tactile crunch kick

                        for (int c = 0; c < 10; c++) SpawnDirtClod(tipWorld, Vector3{ 0, -0.4f, 0 });

                    }

                    // 2. Flinging scooped dirt forward in an arc

                    if (!g_shovelDigThrowDone && t >= SHOVEL_DIG_THROW_T) {

                        g_shovelDigThrowDone = true;

                        Vector3 tossDir = Vector3Normalize(Vector3{ fwd.x * 0.45f, 0.85f, fwd.z * 0.45f });

                        for (int c = 0; c < 12; c++) SpawnDirtClod(Vector3Add(tipWorld, Vector3{0, 0.15f, 0}), tossDir);

                    }

                }

                if (g_shovelAnimState == SHOVEL_ANIM_ATTACK) {

                    float t = Clamp(g_shovelAnimTime / SHOVEL_ATTACK_DURATION, 0.0f, 1.0f);

                    if (!g_shovelAttackImpactDone && t >= SHOVEL_ATTACK_IMPACT_T) {

                        g_shovelAttackImpactDone = true;

                        float gY = GetTerrainGroundHeight(tipWorld.x, tipWorld.z);

                        if (tipWorld.y <= gY + 0.9f) {

                            PlaySound(g_sndShovelDig);

                            g_camLandingDip = -0.10f;

                            for (int c = 0; c < 8; c++) SpawnDirtClod(tipWorld, Vector3{ 0, 0.8f, 0 });

                        }

                    }

                }

            }

        }

        

        // ---- SOLID OUTSIDE GROUND LAYER (Very, very dark brown, like the road material) ----

        matGround.maps[MATERIAL_MAP_ALBEDO].color = ApplyExteriorDaylight(darkBrownBase, 1.0f);

        DrawMesh(mGround, matGround, MatrixTranslate(128.0f, 10.00f, 128.0f));



        // ---- ROAD (Main Game) ----

        matRoad.maps[MATERIAL_MAP_ALBEDO].color = { 0, 0, 0, 255 }; // Pure pitch black asphalt road

        DrawMesh(mRoad, matRoad, MatrixTranslate(128.0f, 10.01f, 250.0f));

        // 3D Cloud Ground Shadows on Highway, Apron, and Terrain
        if (!isUnderwaterScene) {
            DrawCloudGroundShadows(renderCam, sunDir, sunElev, timeVal);
        }

        // Draw dual-lane stripes (Left Lane at X=119.5, Right Lane at X=136.5)

        int startStripe = (int)(camera.position.z / 6.0f) - 15;

        if (startStripe < -50) startStripe = -50;

        int endStripe = startStripe + 30;

        for (int i = startStripe; i < endStripe; i++) {

            float rz = i * 6.0f;

            // Left Lane stripe

            DrawLine3D({ 119.5f, 10.05f, rz }, { 119.5f, 10.05f, rz - 2.5f }, { 200, 200, 200, 180 });

            // Right Lane stripe

            DrawLine3D({ 136.5f, 10.05f, rz }, { 136.5f, 10.05f, rz - 2.5f }, { 200, 200, 200, 180 });

        }

        

        // ---- PROCEDURAL HORROR BOVINE SKELETON NPCS ----

        {

            Color boneCol = ApplyExteriorDaylight(Color{ 210, 205, 185, 255 }, 1.0f);

            Color hornCol = ApplyExteriorDaylight(Color{  45,  40,  35, 255 }, 1.0f);

            // Nocturnal glowing crimson void eye sockets during night, dark void during day

            Color voidCol = (sunElev < 0.0f) ? Color{ 220, 35, 25, 255 } : Color{ 10, 8, 8, 255 };



            Vector3 bovineCamFwd = Vector3Normalize(Vector3Subtract(renderCam.target, renderCam.position));

            for (int b = 0; b < MAX_BOVINE_NPCS; b++) {

                const BovineSkeleton &cow = g_bovineNPCs[b];

                float cdx = cow.pos.x - renderCam.position.x;

                float cdy = cow.pos.y - renderCam.position.y;

                float cdz = cow.pos.z - renderCam.position.z;

                float cDistSq = cdx * cdx + cdz * cdz;

                if (cDistSq > 160.0f * 160.0f) continue;

                if (cDistSq > 16.0f * 16.0f) {

                    float cFullDist = sqrtf(cdx * cdx + cdy * cdy + cdz * cdz);

                    float cDot = (bovineCamFwd.x * cdx + bovineCamFwd.y * cdy + bovineCamFwd.z * cdz) / cFullDist;

                    if (cDot < -0.35f) continue; // Behind player's head

                }

                DrawBovineSkeleton(cow, g_bovineCyl, g_bovineSphere, g_bovineCube, g_bovineCone,

                                   &g_bovineMat, boneCol, hornCol, voidCol);

            }

        }

        

        // ---- PROCEDURAL HORROR HOUND (DOG NPC) ----

        if (g_houndResourcesLoaded) {
            // 9. HOUND ENTITY RENDERING
            // (Spawn and pathfinding logic executes elsewhere)
            float distHoundSq = Vector3DistanceSqr(renderCam.position, g_houndNPC.pos);
            if (distHoundSq < 150.0f * 150.0f && g_gameState != STATE_MAIN_MENU) {

                // Pass dynamic camera and time uniforms

                float dogTime = g_houndNPC.animTime;

                SetShaderValue(g_houndShader, g_houndTimeLoc, &dogTime, SHADER_UNIFORM_FLOAT);

                float camPosArr[3] = { renderCam.position.x, renderCam.position.y, renderCam.position.z };

                SetShaderValue(g_houndShader, g_houndViewPosLoc, camPosArr, SHADER_UNIFORM_VEC3);



                // Dynamic Day/Night Ambient Light

                float ambLvl = 0.05f + (1.0f - nightFactor) * 0.42f;

                float ambArr[4] = { ambLvl * 1.05f, ambLvl * 1.0f, ambLvl * 0.95f, 1.0f };

                SetShaderValue(g_houndShader, g_houndAmbientLoc, ambArr, SHADER_UNIFORM_VEC4);



                // Dynamic Atmospheric Horizon Fog

                Color fogColC = ApplyExteriorDaylight(Color{ 20, 22, 28, 255 }, 0.5f);

                float fogColArr[4] = { fogColC.r / 255.0f, fogColC.g / 255.0f, fogColC.b / 255.0f, 1.0f };

                float fogD = 0.007f;

                SetShaderValue(g_houndShader, g_houndFogColorLoc, fogColArr, SHADER_UNIFORM_VEC4);

                SetShaderValue(g_houndShader, g_houndFogDensityLoc, &fogD, SHADER_UNIFORM_FLOAT);



                // Light 0: Celestial Sun / Moon Directional Light

                Vector3 l0Pos = Vector3Add(g_houndNPC.pos, Vector3Scale(sunDir, 40.0f));

                Color l0Col = (nightFactor > 0.4f) ? Color{ 110, 130, 185, 255 } : Color{ 255, 245, 220, 255 };

                HoundUpdateLight(0, 1, 0, l0Pos, g_houndNPC.pos, l0Col);



                // Light 1: Gas Station Overhead Canopy Fluorescent Light

                Vector3 l1Pos = { 128.0f, 14.2f, 140.0f };

                Color l1Col = fluorLightOn ? Color{ 200, 235, 210, 255 } : Color{ 0, 0, 0, 0 };

                HoundUpdateLight(1, fluorLightOn ? 1 : 0, 1, l1Pos, g_houndNPC.pos, l1Col);



                // Light 2 & 3: Disabled by default

                HoundUpdateLight(2, 0, 0, Vector3{0,0,0}, Vector3{0,0,0}, Color{0,0,0,0});

                HoundUpdateLight(3, 0, 0, Vector3{0,0,0}, Vector3{0,0,0}, Color{0,0,0,0});



                bool isNightHound = (nightFactor > 0.40f);

                if (isNightHound) {

                    // Update Skeleton Shader Uniforms

                    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonTimeLoc, &dogTime, SHADER_UNIFORM_FLOAT);

                    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonViewPosLoc, camPosArr, SHADER_UNIFORM_VEC3);

                    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonAmbientLoc, ambArr, SHADER_UNIFORM_VEC4);

                    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonFogColorLoc, fogColArr, SHADER_UNIFORM_VEC4);

                    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonFogDensityLoc, &fogD, SHADER_UNIFORM_FLOAT);

                    HoundSkeletonUpdateLight(0, 1, 0, l0Pos, g_houndNPC.pos, l0Col);

                    HoundSkeletonUpdateLight(1, fluorLightOn ? 1 : 0, 1, l1Pos, g_houndNPC.pos, l1Col);

                    HoundSkeletonUpdateLight(2, 0, 0, Vector3{0,0,0}, Vector3{0,0,0}, Color{0,0,0,0});

                    HoundSkeletonUpdateLight(3, 0, 0, Vector3{0,0,0}, Vector3{0,0,0}, Color{0,0,0,0});



                    // At night: The hound appears as the terrifying canine skeleton

                    DrawSkeletonHound(g_houndNPC, g_houndCyl, g_houndSphere, g_houndCube, g_houndCone, &g_houndSkeletonMat);

                } else {

                    // During day: The hound appears in its spotted fur coat

                    DrawHound(g_houndNPC, g_houndCyl, g_houndSphere, g_houndCube, g_houndCone, &g_houndMat);

                }

            }

        }

        

        // ---------------------------------------------------------------------

        // 10. MR. GRETHNAR WOULE: JUMPSCARE MANIFESTATION

        // Manifests abruptly directly in front of the player on the floor,

        // FULLY VISIBLE, with the EXACT SAME MODEL, SHAPE, SIZE, AND PROPORTIONS!

        // Visible for just a glimpse (0.45s) and disappears!

        // ---------------------------------------------------------------------

        if (grethnarState == GRETHNAR_JUMPSCARE) {

            Vector3 pFwdH = Vector3Normalize(Vector3{ forwardBob.x, 0.0f, forwardBob.z });

            if (Vector3Length(pFwdH) < 0.1f) pFwdH = Vector3{ 0.0f, 0.0f, 1.0f };



            // Standing right on the shop floor 1.6m in front of player (fully in camera frame in both 1st and 3rd person)

            Vector3 jsPos = Vector3Add(camera.position, Vector3Scale(pFwdH, 1.60f));

            jsPos.y = 10.0f; // Exact floor level



            float faceYaw = -atan2f(camera.position.x - jsPos.x, camera.position.z - jsPos.z) * RAD2DEG;



            rlPushMatrix();

            rlTranslatef(jsPos.x, jsPos.y, jsPos.z);

            rlRotatef(faceYaw, 0.0f, 1.0f, 0.0f);



            // EXACT SAME FULL BODY MODEL, SHAPE, AND SIZE AS BEHIND THE COUNTER:

            // 1. Trenchcoat body cylinder (same 1.95m height, 0.27m base, 0.22m top)

            DrawCylinder({ 0.0f, 0.0f, 0.0f }, 0.22f, 0.27f, 1.95f, 14, { 16, 16, 18, 255 });

            // 2. Coat shoulder yoke

            DrawCube({ 0.0f, 1.85f, 0.0f }, 0.78f, 0.18f, 0.36f, { 20, 20, 24, 255 });



            // 3. Drooping long arms and pale hands

            DrawCylinderEx({ -0.38f, 1.75f, 0.0f }, { -0.38f, 0.60f, 0.0f }, 0.055f, 0.045f, 8, { 18, 18, 20, 255 });

            DrawSphere({ -0.38f, 0.55f, 0.0f }, 0.05f, { 220, 220, 215, 255 });

            DrawCylinderEx({  0.38f, 1.75f, 0.0f }, {  0.38f, 0.60f, 0.0f }, 0.055f, 0.045f, 8, { 18, 18, 20, 255 });

            DrawSphere({  0.38f, 0.55f, 0.0f }, 0.05f, { 220, 220, 215, 255 });



            // 4. Neck & Pale Head (exact same corpse-white 0.24m sphere)

            DrawCylinder({ 0.0f, 1.91f, 0.0f }, 0.10f, 0.10f, 0.14f, 10, { 230, 230, 226, 255 });

            DrawSphere({ 0.0f, 2.19f, 0.0f }, 0.24f, { 250, 250, 248, 255 });

            DrawSphereWires({ 0.0f, 2.19f, 0.0f }, 0.242f, 12, 12, { 180, 180, 178, 110 });



            // 5. Stretched horizontal mouth

            DrawCube({ 0.0f, 2.105f, 0.225f }, 0.22f, 0.035f, 0.03f, { 8, 8, 10, 255 });



            // 6. Eyes: Bloody crimson death stare (same subtle size!)

            float jsRad = 0.042f * 1.18f;

            DrawSphere({ -0.09f, 2.22f, 0.222f }, jsRad, { 255, 18, 22, 255 });

            DrawSphere({  0.09f, 2.22f, 0.222f }, jsRad, { 255, 18, 22, 255 });

            DrawSphere({ -0.09f, 2.22f, 0.222f + jsRad * 0.72f }, jsRad * 0.38f, { 15, 0, 0, 255 });

            DrawSphere({  0.09f, 2.22f, 0.222f + jsRad * 0.72f }, jsRad * 0.38f, { 15, 0, 0, 255 });



            // Throbbing sclerae and weeping blood streaks

            DrawSphereWires({ -0.09f, 2.22f, 0.222f }, jsRad * 1.10f, 8, 8, { 120, 4, 8, 255 });

            DrawSphereWires({  0.09f, 2.22f, 0.222f }, jsRad * 1.10f, 8, 8, { 120, 4, 8, 255 });

            DrawCylinderEx({ -0.09f, 2.22f, 0.235f }, { -0.09f, 2.06f, 0.230f }, 0.012f, 0.006f, 6, { 135, 8, 14, 255 });

            DrawCylinderEx({  0.09f, 2.22f, 0.235f }, {  0.09f, 2.06f, 0.230f }, 0.012f, 0.006f, 6, { 135, 8, 14, 255 });



            rlPopMatrix();

        }

        

        // 11. ABANDONED BLACKWOOD COLLEGE (Classrooms, Whiteboard, Blood Splashes, Hallway & Anatomy Lab)
        DrawAbandonedCollege(renderCam, timeVal, dt, extDayFactor, extNightFactor, sunDir);

        // 12. REALISTIC OCEAN WITH GERSTNER WAVES & COASTAL ENVIRONMENT
        DrawCoastalEnvironment(renderCam, timeVal, extDayFactor, extNightFactor, sunDir);
        DrawOceanSurface(renderCam, timeVal, extDayFactor, extNightFactor, sunDir, sunElev, lightningFlashTimer > 0.0f ? 1.0f : 0.0f);

        // 12. DYNAMIC DISSOLVING PLAYER FOOTPRINTS
        DrawFootprints();

        // ---------------------------------------------------------------------
        // MAIN MENU 3D ATMOSPHERE: SHADOW LURKER, MIST & APPALACHIAN HORNED SKULL
        // ---------------------------------------------------------------------
        if (g_gameState == STATE_MAIN_MENU) {
            // 1. Supernatural tall silhouette entity standing under the portico archway
            float stepFwd = (1.0f - g_menuAwakeIntensity) * 0.95f;
            Vector3 colFigure = { 152.6f - stepFwd, 11.75f, 140.0f };
            DrawCube(colFigure, 0.44f, 2.35f, 0.50f, Color{ 2, 3, 5, 250 });
            DrawSphere(Vector3{ colFigure.x, 12.85f, colFigure.z }, 0.20f, Color{ 2, 3, 5, 250 });

            // Glowing tapetum-lucidum predator eyes in deep darkness
            float colEyeAlpha = Clamp((1.0f - g_menuAwakeIntensity) * 255.0f, 0.0f, 255.0f);
            if (colEyeAlpha > 8.0f) {
                DrawSphere(Vector3{ colFigure.x - 0.14f, 12.88f, colFigure.z - 0.06f }, 0.018f, Color{ 245, 190, 45, (unsigned char)colEyeAlpha });
                DrawSphere(Vector3{ colFigure.x - 0.14f, 12.88f, colFigure.z + 0.06f }, 0.018f, Color{ 245, 190, 45, (unsigned char)colEyeAlpha });
            }

            // 2. Swirling ground mist caught in the portico lantern's light
            for (int mi = 0; mi < 18; mi++) {
                float mSeed = (float)mi * 1.618f;
                float mx = 145.0f + sinf(timeVal * 0.28f + mSeed) * 5.0f;
                float my = 10.3f + fmodf(timeVal * 0.14f + mSeed * 1.8f, 3.2f);
                float mz = 140.0f + cosf(timeVal * 0.32f + mSeed * 1.4f) * 4.0f;
                float mistAlpha = sinf((my - 10.3f) / 3.2f * PI) * 110.0f * g_menuAwakeIntensity;
                if (mistAlpha > 0.0f) {
                    DrawSphere(Vector3{ mx, my, mz }, 0.040f, Color{ 185, 200, 220, (unsigned char)mistAlpha });
                }
            }

            // 3. APPALACHIAN FOLK-HORROR HORNED BOVINE SKULL (REAL-TIME GAZE TRACKING)
            // Mounted in the open right-hand foreground of CAM 01
            Vector3 skullPos = { 143.6f, 13.40f, 137.5f };

            // Weathered rustic timber fence post
            Color postWood   = { 52, 45, 38, 255 };
            Color postRing   = { 30, 26, 22, 255 };
            Color rustWire   = { 95, 55, 35, 240 };
            DrawCylinder(Vector3{ skullPos.x, 11.5f, skullPos.z }, 0.10f, 0.11f, 1.70f, 8, postWood);
            DrawCubeWires(Vector3{ skullPos.x, 13.18f, skullPos.z }, 0.22f, 0.14f, 0.22f, postRing);
            // Wrapped rusted bailing wire
            DrawCircle3D(Vector3{ skullPos.x, 13.12f, skullPos.z }, 0.115f, Vector3{ 1, 0, 0 }, 90.0f, rustWire);
            DrawCircle3D(Vector3{ skullPos.x, 13.15f, skullPos.z }, 0.115f, Vector3{ 1, 0, 0 }, 90.0f, rustWire);

            // Ritual tallow candle / sconce lighting the skull from below
            Vector3 sconcePos = { skullPos.x - 0.14f, 13.02f, skullPos.z - 0.12f };
            DrawCube(sconcePos, 0.05f, 0.025f, 0.08f, postRing);
            DrawCylinder(Vector3{ sconcePos.x, 13.03f, sconcePos.z }, 0.022f, 0.020f, 0.08f, 6, Color{ 215, 205, 185, 255 }); // Tallow candle stub
            float candleFlicker = sinf(timeVal * 11.0f) * 0.06f + cosf(timeVal * 17.0f) * 0.04f;
            Vector3 candleFlame = { sconcePos.x, 13.12f, sconcePos.z };
            DrawSphere(candleFlame, 0.032f + candleFlicker * 0.008f, Color{ 255, 225, 95, 255 });
            DrawSphere(candleFlame, 0.16f + candleFlicker * 0.025f, Color{ 255, 150, 40, (unsigned char)(65 + (int)(candleFlicker * 20.0f)) });

            // Organic micro-head tilt tracking the player's cursor
            float headYaw   = -132.0f - g_skullEyeSmoothX * 12.0f;
            float headPitch = 3.5f - g_skullEyeSmoothY * 7.5f;
            float headRoll  = g_skullEyeSmoothX * 4.0f;

            rlPushMatrix();
            rlTranslatef(skullPos.x, skullPos.y, skullPos.z);
            rlRotatef(headYaw, 0.0f, 1.0f, 0.0f);
            rlRotatef(headPitch, 1.0f, 0.0f, 0.0f);
            rlRotatef(headRoll, 0.0f, 0.0f, 1.0f);

            Color boneIvory  = { 235, 228, 212, 255 };
            Color boneDark   = { 152, 142, 126, 255 };
            Color socketHole = { 12, 10, 8, 255 };
            Color hornDark   = { 42, 36, 30, 255 };
            Color hornMid    = { 85, 74, 62, 255 };
            Color hornPale   = { 170, 160, 145, 255 };

            // A. Braincase / Cranium (Imposing scale)
            DrawSphere(Vector3{ 0.0f, 0.08f, -0.05f }, 0.22f, boneIvory);
            DrawCube(Vector3{ 0.0f, 0.12f, 0.03f }, 0.36f, 0.055f, 0.10f, boneIvory); // Supraorbital brow ridge

            // B. Snout & Nasal Bridge (Tapering forward towards viewer)
            DrawCylinderEx(Vector3{ 0.0f, 0.07f, -0.02f }, Vector3{ 0.0f, -0.08f, 0.36f }, 0.13f, 0.08f, 8, boneIvory);
            DrawCube(Vector3{ 0.0f, -0.09f, 0.36f }, 0.13f, 0.07f, 0.12f, boneIvory); // Maxilla tip
            DrawCube(Vector3{ 0.0f, -0.07f, 0.34f }, 0.045f, 0.055f, 0.10f, socketHole); // Nasal cavity aperture
            DrawCube(Vector3{ 0.0f, -0.13f, 0.30f }, 0.11f, 0.03f, 0.18f, boneDark); // Upper jaw teeth ridge

            // C. Deep Hollow Eye Sockets
            Vector3 leftEyeSocket  = { -0.13f, 0.06f, 0.08f };
            Vector3 rightEyeSocket = {  0.13f, 0.06f, 0.08f };
            DrawSphere(leftEyeSocket, 0.058f, socketHole);
            DrawSphere(rightEyeSocket, 0.058f, socketHole);
            DrawCircle3D(leftEyeSocket, 0.064f, Vector3{ 0, 0, 1 }, 0.0f, boneDark);
            DrawCircle3D(rightEyeSocket, 0.064f, Vector3{ 0, 0, 1 }, 0.0f, boneDark);

            // D. Majestic Sweeping Curved Horns (1.4m span, arching out, up, curling forward)
            // Left Horn
            DrawCylinderEx(Vector3{ -0.15f, 0.13f, -0.07f }, Vector3{ -0.34f, 0.20f, -0.08f }, 0.065f, 0.052f, 8, hornDark);
            DrawCylinderEx(Vector3{ -0.34f, 0.20f, -0.08f }, Vector3{ -0.52f, 0.32f, -0.04f }, 0.052f, 0.038f, 8, hornMid);
            DrawCylinderEx(Vector3{ -0.52f, 0.32f, -0.04f }, Vector3{ -0.56f, 0.48f,  0.06f }, 0.038f, 0.024f, 8, hornPale);
            DrawCylinderEx(Vector3{ -0.56f, 0.48f,  0.06f }, Vector3{ -0.48f, 0.58f,  0.14f }, 0.024f, 0.006f, 7, boneIvory);

            // Right Horn
            DrawCylinderEx(Vector3{  0.15f, 0.13f, -0.07f }, Vector3{  0.34f, 0.20f, -0.08f }, 0.065f, 0.052f, 8, hornDark);
            DrawCylinderEx(Vector3{  0.34f, 0.20f, -0.08f }, Vector3{  0.52f, 0.32f, -0.04f }, 0.052f, 0.038f, 8, hornMid);
            DrawCylinderEx(Vector3{  0.52f, 0.32f, -0.04f }, Vector3{  0.56f, 0.48f,  0.06f }, 0.038f, 0.024f, 8, hornPale);
            DrawCylinderEx(Vector3{  0.56f, 0.48f,  0.06f }, Vector3{  0.48f, 0.58f,  0.14f }, 0.024f, 0.006f, 7, boneIvory);

            // Occult carved rune on the forehead
            DrawLine3D(Vector3{  0.0f,  0.04f, 0.11f }, Vector3{ 0.0f, 0.16f, 0.03f }, Color{ 85, 25, 20, 230 });
            DrawLine3D(Vector3{ -0.04f, 0.11f, 0.07f }, Vector3{ 0.04f, 0.11f, 0.07f }, Color{ 85, 25, 20, 230 });

            // E. THE GAZE-TRACKING GLOWING EYES
            float pupilOffX = -g_skullEyeSmoothX * 0.028f;
            float pupilOffY = -g_skullEyeSmoothY * 0.022f;
            float pupilOffZ = 0.032f;

            Vector3 leftPupil  = { leftEyeSocket.x  + pupilOffX, leftEyeSocket.y  + pupilOffY, leftEyeSocket.z  + pupilOffZ };
            Vector3 rightPupil = { rightEyeSocket.x + pupilOffX, rightEyeSocket.y + pupilOffY, rightEyeSocket.z + pupilOffZ };

            float eyePulse = 0.90f + 0.10f * sinf(timeVal * 3.0f);
            float eyeR     = (0.020f + g_skullGazeFlare * 0.007f) * eyePulse;

            Color eyeCoreCol  = { 255, 250, 210, 255 };
            Color eyeAmberCol = g_skullGazeFlare > 0.3f ? Color{ 255, 120, 35, 255 } : Color{ 255, 195, 45, 255 };
            Color eyeHaloCol  = { 255, 160, 35, (unsigned char)(55 + (int)(g_skullGazeFlare * 75.0f)) };

            // Glowing tapetum-lucidum predatory pupils
            DrawSphere(leftPupil, eyeR, eyeAmberCol);
            DrawSphere(rightPupil, eyeR, eyeAmberCol);
            // Pinpoint white-hot core retinas
            DrawSphere(Vector3{ leftPupil.x, leftPupil.y, leftPupil.z + 0.006f }, eyeR * 0.45f, eyeCoreCol);
            DrawSphere(Vector3{ rightPupil.x, rightPupil.y, rightPupil.z + 0.006f }, eyeR * 0.45f, eyeCoreCol);
            // Volumetric micro-halo
            DrawSphere(leftPupil, eyeR * 2.8f, eyeHaloCol);
            DrawSphere(rightPupil, eyeR * 2.8f, eyeHaloCol);

            rlPopMatrix();
        }
        
        EndMode3D();

        

        // ---------------------------------------------------------------------

        // 2D POST-PROCESS & HUD DRAWING PASS (PIXEL-PERFECT VIRTUAL CAMERA)

        // ---------------------------------------------------------------------

        float uiScaleX = (float)target.texture.width / (float)LOGICAL_W;

        float uiScaleY = (float)target.texture.height / (float)LOGICAL_H;

        float uiScale = fminf(uiScaleX, uiScaleY);

        Camera2D uiCamera = { 0 };

        uiCamera.zoom = uiScale;

        uiCamera.offset = Vector2{

            ((float)target.texture.width - (float)LOGICAL_W * uiScale) * 0.5f,

            ((float)target.texture.height - (float)LOGICAL_H * uiScale) * 0.5f

        };

        BeginMode2D(uiCamera);

        // Underwater Post-Processing, Hypoxia Vignette, Oxygen Gauge, Waterline Meniscus & Screen Water Droplets
        DrawUnderwaterPostFXAndHUD(renderCam, timeVal, dt, LOGICAL_W, LOGICAL_H);

        // Flashlight Volumetric Center Halo & Atmospheric Beam in 2D View

        if (g_flashlightActive && g_gameState == STATE_GAMEPLAY && !isRoofCamActive) {

            DrawCircleGradient(LOGICAL_W / 2, LOGICAL_H / 2 + 25, 340.0f, Color{ 255, 245, 210, 22 }, Color{ 0, 0, 0, 0 });

            DrawCircleGradient(LOGICAL_W / 2, LOGICAL_H / 2 + 25, 170.0f, Color{ 255, 250, 230, 18 }, Color{ 0, 0, 0, 0 });

        }



        // (Player hand items removed from first-person perspective)



        // ---------------------------------------------------------------------

        // SUPERSTORE PRODUCT HUD INSPECTION & HOLD PROMPTS (BOTTOM-CENTER)
        // ---------------------------------------------------------------------
        if (!isShopOpen && !isRoofCamActive && !showQuitConfirm) {
            DrawHeldProductHUD(g_heldProductIndex, hudFocusIdx, nearCounter, g_hasShovel, g_isHoldingCart, g_phoneActive, g_shovelEquipped);
        }

        // ---------------------------------------------------------------------
        // AAA TOP-LEFT PLAYER HUD & TELEMETRY (CASH CARD, LOCATION, POPUPS)
        // ---------------------------------------------------------------------
        if (g_gameState == STATE_GAMEPLAY && !isRoofCamActive && !showQuitConfirm) {
            DrawPlayerHUD(camera);
        }

        // ---------------------------------------------------------------------
        // AAA DYNAMIC BOTTOM NAVIGATION DOCK (AUTO-FLOWING CONTROLS RIBBON)
        // ---------------------------------------------------------------------
        if (g_gameState == STATE_GAMEPLAY && !isShopOpen && !isRoofCamActive && !showQuitConfirm) {
            DrawBottomNavigationDock(g_hasReceipt, g_inspectingReceipt, g_flashlightActive, g_phoneActive, g_hasShovel, g_shovelEquipped);
        }

        // ---------------------------------------------------------------------
        // AAA CENTRALIZED INTERACTION MANAGER (PRIORITY QUEUE - ZERO COLLISIONS)
        // ---------------------------------------------------------------------
        if (g_gameState == STATE_GAMEPLAY && !isShopOpen && !isRoofCamActive && !showQuitConfirm) {
            DrawInteractionManager(camera, dt, timeVal, g_camLandingDip);
        }

        // Subterranean Notification Banner
        DrawSubterraneanBanner(g_tunnelBannerTimer, g_tunnelBannerText);

        // =========================================================================
        // AAA DAY / NIGHT SURVIVAL CLOCK & CELESTIAL TELEMETRY HUD (TOP-RIGHT)
        // =========================================================================
        if (g_gameState == STATE_GAMEPLAY && !isRoofCamActive && !showQuitConfirm) {
            DrawSurvivalClockHUD(dayCycleTime, dayCycleDuration);
        }

        // =========================================================================
        // AAA FULL-SCREEN HORROR RECEIPT READING / INSPECTION OVERLAY
        // =========================================================================
        DrawReceiptInspectionOverlay();

        // =========================================================================
        // AAA ROOF CCTV SURVEILLANCE OVERLAY (ZERO DUPLICATE TEXT & CLEAN CRT TELEMETRY)
        // =========================================================================
        DrawCCTVSurveillanceOverlay(timeVal);

        // =========================================================================
        // AAA SURREAL SHOPKEEPER STORE UI OVERLAY
        // =========================================================================
        DrawShopkeeperStoreUI(isShopOpen, shopFeedbackMsg);

        // =========================================================================
        // AAA DIEBOLD/NCR 24-HR CASHPOINT ATM TERMINAL INTERFACE OVERLAY
        // =========================================================================
        DrawATMOverlay2D();

        // =========================================================================
        // AAA TACTICAL CROSSHAIR RETICLE
        // =========================================================================
        if (g_gameState == STATE_GAMEPLAY && !isShopOpen && !isRoofCamActive) {
            DrawTacticalCrosshair(hudFocusIdx, nearCounter, g_heldProductIndex);
            if (IsPlayerNearWashroomSink(camera.position)) {
                const char* faucetPrompt = IsWashroomSinkRunning() ? "[E] TURN FAUCET OFF" : "[E] TURN FAUCET ON";
                int textW = MeasureText(faucetPrompt, 16);
                DrawRectangle(LOGICAL_W / 2 - textW / 2 - 8, LOGICAL_H / 2 + 32, textW + 16, 24, Color{ 15, 18, 22, 210 });
                DrawRectangleLines(LOGICAL_W / 2 - textW / 2 - 8, LOGICAL_H / 2 + 32, textW + 16, 24, Color{ 85, 165, 235, 240 });
                DrawText(faucetPrompt, LOGICAL_W / 2 - textW / 2, LOGICAL_H / 2 + 36, 16, Color{ 225, 240, 255, 255 });
            }
            if (IsPlayerNearWashroomDoor(camera.position)) {
                const char* doorPrompt = IsWashroomDoorOpen() ? "[E] CLOSE WASHROOM DOOR" : "[E] OPEN WASHROOM DOOR";
                int textW = MeasureText(doorPrompt, 16);
                DrawRectangle(LOGICAL_W / 2 - textW / 2 - 8, LOGICAL_H / 2 + 32, textW + 16, 24, Color{ 15, 18, 22, 210 });
                DrawRectangleLines(LOGICAL_W / 2 - textW / 2 - 8, LOGICAL_H / 2 + 32, textW + 16, 24, Color{ 180, 145, 55, 240 });
                DrawText(doorPrompt, LOGICAL_W / 2 - textW / 2, LOGICAL_H / 2 + 36, 16, Color{ 245, 230, 195, 255 });
            }
            if (g_ghostCart.active && g_ghostCart.alpha > 0.35f && g_ghostCart.itemsInCart > 0 && !g_isHoldingCart) {
                float dGhost = Vector3Distance(camera.position, g_ghostCart.pos);
                if (dGhost < 2.4f && g_heldProductIndex == -1) {
                    const char* ghostPrompt = "[E] TAKE PHANTOM ITEM FROM CART";
                    int textW = MeasureText(ghostPrompt, 16);
                    DrawRectangle(LOGICAL_W / 2 - textW / 2 - 8, LOGICAL_H / 2 + 60, textW + 16, 24, Color{ 24, 10, 10, 215 });
                    DrawRectangleLines(LOGICAL_W / 2 - textW / 2 - 8, LOGICAL_H / 2 + 60, textW + 16, 24, Color{ 220, 65, 50, 240 });
                    DrawText(ghostPrompt, LOGICAL_W / 2 - textW / 2, LOGICAL_H / 2 + 64, 16, Color{ 255, 185, 175, 255 });
                }
            }
        }

        // HYPER-REALISTIC SMARTPHONE & LIVE "MIRE-NAV" GPS MAP SYSTEM
        phoneSystem.Draw(target, camera, g_vmSwayX, g_vmSwayY, dayCycleTime, dayCycleDuration, g_waterState);



        EndMode2D(); // Close pixel-perfect virtual UI camera



        

        EndTextureMode();

        

        // ----------------------------------------------------

        // POST-PROCESSING PASS TO SCREEN

        // ----------------------------------------------------

        BeginDrawing();

        ClearBackground(BLACK);

        

        int screenW = GetScreenWidth();

        int screenH = GetScreenHeight();



        // Direct 1:1 Pixel-Perfect Direct Blit to Screen Buffer (Zero Bilinear Blur)

        DrawTexturePro(target.texture, 

            Rectangle{ 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },

            Rectangle{ 0.0f, 0.0f, (float)screenW, (float)screenH },

            Vector2{ 0, 0 }, 0.0f, WHITE);

            

        // ---------------------------------------------------------------------

        // HORROR SCREEN FLASH & VIGNETTE (During Grethnar Jumpscare Glimpse)

        // ---------------------------------------------------------------------

        if (grethnarState == GRETHNAR_JUMPSCARE) {

            float flashAlpha = Clamp(grethnarJumpscareTimer / 0.45f, 0.0f, 1.0f) * 115.0f;

            DrawRectangle(0, 0, screenW, screenH, Color{ 180, 0, 0, (unsigned char)flashAlpha });

            

            // Blood vignette borders framing screen during glimpse

            for (int b = 0; b < 24; b += 2) {

                unsigned char vigA = (unsigned char)(130 * (1.0f - (float)b / 24.0f) * (grethnarJumpscareTimer / 0.45f));

                DrawRectangleLines(b, b, screenW - b*2, screenH - b*2, Color{ 120, 0, 0, vigA });

            }

        }

            

        // =========================================================================

        // QUIT GAME CONFIRMATION MODAL DIALOG (Screen Pass)
        // =========================================================================
        if (showQuitConfirm) {
            DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 195 });

            float qbw = 500.0f, qbh = 220.0f;
            float qbx = ((float)screenW - qbw) * 0.5f;
            float qby = ((float)screenH - qbh) * 0.5f;

            DrawAAAPanel(Rectangle{ qbx, qby, qbw, qbh }, Color{ 14, 16, 20, 252 }, Color{ 215, 55, 55, 230 }, 8.0f, true);

            DrawTextSharpCentered(g_fontHeadSub, "QUIT GAME?", (float)screenW * 0.5f, qby + 28.0f, 26.0f, Color{ 255, 225, 225, 255 });
            DrawTextSharpCentered(g_fontBody, "Are you sure you want to exit to desktop?", (float)screenW * 0.5f, qby + 68.0f, 15.0f, Color{ 185, 190, 195, 240 });

            Vector2 mPos = GetMousePosition();
            Rectangle btnQuitRec   = { qbx + 35.0f, qby + 130.0f, 125.0f, 46.0f };
            Rectangle btnMenuRec   = { qbx + qbw * 0.5f - 70.0f, qby + 130.0f, 140.0f, 46.0f };
            Rectangle btnResumeRec = { qbx + qbw - 160.0f, qby + 130.0f, 125.0f, 46.0f };

            bool hoverQuit   = CheckCollisionPointRec(mPos, btnQuitRec);
            bool hoverMenu   = CheckCollisionPointRec(mPos, btnMenuRec);
            bool hoverResume = CheckCollisionPointRec(mPos, btnResumeRec);

            // Button [QUIT]
            DrawAAAPanel(btnQuitRec, hoverQuit ? Color{ 195, 40, 40, 255 } : Color{ 135, 28, 28, 230 }, hoverQuit ? WHITE : Color{ 245, 80, 80, 255 }, 5.0f, false);
            DrawTextSharpCentered(g_fontMenu, "QUIT", btnQuitRec.x + btnQuitRec.width * 0.5f, btnQuitRec.y + 14.0f, 16.0f, WHITE);

            // Button [MAIN MENU]
            DrawAAAPanel(btnMenuRec, hoverMenu ? Color{ 55, 85, 125, 255 } : Color{ 32, 50, 75, 230 }, hoverMenu ? WHITE : Color{ 95, 155, 235, 255 }, 5.0f, false);
            DrawTextSharpCentered(g_fontMenu, "MAIN MENU", btnMenuRec.x + btnMenuRec.width * 0.5f, btnMenuRec.y + 14.0f, 15.0f, WHITE);

            // Button [RESUME]
            DrawAAAPanel(btnResumeRec, hoverResume ? Color{ 45, 125, 65, 255 } : Color{ 28, 75, 42, 230 }, hoverResume ? WHITE : Color{ 80, 195, 105, 255 }, 5.0f, false);
            DrawTextSharpCentered(g_fontMenu, "RESUME", btnResumeRec.x + btnResumeRec.width * 0.5f, btnResumeRec.y + 14.0f, 16.0f, WHITE);

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (hoverQuit) shouldQuitGame = true;
                if (hoverMenu) {
                    showQuitConfirm = false;
                    g_gameState = STATE_MAIN_MENU;
                    isCursorCaptured = false;
                    EnableCursor();
                }
                if (hoverResume) {
                    showQuitConfirm = false;
                    isCursorCaptured = true;
                    DisableCursor();
                }
            }
        }

        // =========================================================================

        // FULL-FEATURED PSYCHOLOGICAL HORROR 3D MAIN MENU (CCTV, LORE, CALIBRATION)

        // =========================================================================

        if (g_gameState == STATE_MAIN_MENU) {

            Vector2 mPos = GetMousePosition();
            Vector2 menuMDelta = GetMouseDelta();
            float mouseMoveDist = sqrtf(menuMDelta.x * menuMDelta.x + menuMDelta.y * menuMDelta.y);
            float wheelMove = GetMouseWheelMove();
            bool anyUserInput = (mouseMoveDist > 0.6f) || (fabsf(wheelMove) > 0.05f) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || (GetKeyPressed() != 0);

            // Awakening from deep slumber
            if (anyUserInput) {
                if (g_menuAwakeIntensity < 0.45f) {
                    PlaySound(g_sndFlashlightToggle);
                    g_cctvSwitchGlitch = 0.12f;
                    g_lurkerEyeFlee = 1.0f; // startled lurkers flee
                }
                g_menuIdleTimer = 0.0f;
            } else {
                g_menuIdleTimer += dt;
            }

            // Smooth target awake intensity (1.0 -> 0.10 when idle > 2.2s)
            float targetAwake = 1.0f;
            if (g_menuIdleTimer > 2.2f) {
                float slumberProg = Clamp((g_menuIdleTimer - 2.2f) / 2.8f, 0.0f, 1.0f);
                targetAwake = Lerp(1.0f, 0.10f, slumberProg);
                if (slumberProg > 0.65f && fmodf(timeVal * 8.5f, 1.0f) < 0.10f) {
                    targetAwake *= 0.55f; // battery flicker
                }
            }
            g_menuAwakeIntensity += (targetAwake - g_menuAwakeIntensity) * Clamp(dt * 3.8f, 0.0f, 1.0f);

            // Smooth volumetric flashlight beam tracking
            g_menuLightPos.x += (mPos.x - g_menuLightPos.x) * Clamp(dt * 18.0f, 0.0f, 1.0f);
            g_menuLightPos.y += (mPos.y - g_menuLightPos.y) * Clamp(dt * 18.0f, 0.0f, 1.0f);

            // Decay flee animation
            if (g_lurkerEyeFlee > 0.0f) {
                g_lurkerEyeFlee = fmaxf(0.0f, g_lurkerEyeFlee - dt * 3.0f);
            }

            // Organic Left Horizon Shadow (providing sharp contrast for typography while keeping center, top, and right open)
            int leftShadW = (int)(screenW * 0.44f);
            DrawRectangleGradientH(0, 0, leftShadW, screenH, Color{ 3, 4, 6, 210 }, BLANK);

            // Subtle Ground Shadow for Bottom Telemetry & Vista Selectors
            DrawRectangleGradientV(0, screenH - 60, screenW, 60, BLANK, Color{ 2, 3, 5, 175 });

            // Distant Appalachian Lightning Flash across Blackwood College
            static float s_menuLightningTimer = 14.0f;
            static float s_menuFlashAlpha = 0.0f;
            s_menuLightningTimer -= dt;
            if (s_menuLightningTimer <= 0.0f) {
                s_menuLightningTimer = (float)GetRandomValue(16, 28);
                s_menuFlashAlpha = 0.38f;
                SetSoundVolume(sndThunder, 0.28f);
                PlaySound(sndThunder);
            }
            if (s_menuFlashAlpha > 0.0f) {
                s_menuFlashAlpha -= dt * 2.6f;
                if (s_menuFlashAlpha < 0.0f) s_menuFlashAlpha = 0.0f;
                DrawRectangle(0, 0, screenW, screenH, Color{ 200, 220, 250, (unsigned char)(s_menuFlashAlpha * 95.0f) });
            }

            // Darkness veil when dormant
            float slumberAlpha = (1.0f - g_menuAwakeIntensity) * 235.0f;
            if (slumberAlpha > 2.0f) {
                DrawRectangle(0, 0, screenW, screenH, Color{ 2, 3, 5, (unsigned char)slumberAlpha });
            }

            // Volumetric Flashlight Radial Beam (multi-tier luminous cone)
            DrawCircleGradient((int)g_menuLightPos.x, (int)g_menuLightPos.y, 480.0f, Color{ 210, 195, 160, (unsigned char)(22 * g_menuAwakeIntensity) }, Color{ 0, 0, 0, 0 });
            DrawCircleGradient((int)g_menuLightPos.x, (int)g_menuLightPos.y, 250.0f, Color{ 235, 215, 180, (unsigned char)(40 * g_menuAwakeIntensity) }, Color{ 0, 0, 0, 0 });
            DrawCircleGradient((int)g_menuLightPos.x, (int)g_menuLightPos.y, 95.0f,  Color{ 255, 245, 220, (unsigned char)(72 * g_menuAwakeIntensity) }, Color{ 0, 0, 0, 0 });

            // Floating dust motes catching the flashlight beam
            for (int d = 0; d < 22; d++) {
                float seed = (float)d * 137.5f;
                float dx = fmodf(seed * 43.0f + timeVal * 16.0f * (1.0f + fmodf(seed, 0.4f)), (float)screenW);
                float dy = fmodf(seed * 67.0f + sinf(timeVal * 0.7f + seed) * 35.0f, (float)screenH);
                float dDist = Vector2Distance(Vector2{ dx, dy }, g_menuLightPos);
                if (dDist < 250.0f) {
                    float alpha = (1.0f - dDist / 250.0f) * 190.0f * g_menuAwakeIntensity;
                    DrawCircle((int)dx, (int)dy, 1.2f + fmodf(seed, 1.8f), Color{ 255, 235, 195, (unsigned char)alpha });
                }
            }

            UpdateMenuCCTV(dt, wheelMove, g_showSettingsModal || g_showCaseFilesModal || g_showSurvivalModal || g_isMenuStartingGame, g_sndRadioStatic);
            DrawMenuCCTVOverlay(screenW, screenH, mPos, dt, g_sndRadioStatic);




            // -----------------------------------------------------------------

            // MODAL 1: CASE FILES & CLASSIFIED EVIDENCE DOSSIER

            // -----------------------------------------------------------------

            if (g_showCaseFilesModal) {

                int dw = (int)(screenW * 0.74f);

                if (dw < 780) dw = 780;

                if (dw > 980) dw = 980;

                int dh = (int)(screenH * 0.76f);

                if (dh < 500) dh = 500;

                if (dh > 620) dh = 620;

                int dx = screenW/2 - dw/2, dy = screenH/2 - dh/2;



                DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 215 });

                DrawRectangle(dx, dy, dw, dh, Color{ 10, 11, 14, 252 });

                DrawRectangleLines(dx, dy, dw, dh, Color{ 140, 35, 25, 255 });



                // Header (Distinct Gothic Horror Heading Font)

                DrawTextSharp(g_fontHeadSub, "STATE POLICE // CLASSIFIED EVIDENCE DOSSIER", dx + 28, dy + 18, 25.0f, Color{ 235, 225, 215, 255 });

                DrawTextSharp(g_fontSmall, "CASE #89-094 // ROUTE 9 SERVICE STATION & BORDER MIRE", dx + 28, dy + 48, 14.0f, Color{ 180, 60, 50, 240 });

                DrawLine(dx + 25, dy + 70, dx + dw - 25, dy + 70, Color{ 90, 30, 25, 220 });



                // Left Column: 4 Selectable Case Files

                int listW = 230;

                const char* caseTitles[4] = {

                    "01. DISPATCH TAPE",

                    "02. SIBLING INCIDENT",

                    "03. THE MIRE HOUND",

                    "04. SUB-SURFACE SOIL"

                };



                for (int f = 0; f < 4; f++) {

                    Rectangle fRec = { (float)(dx + 25), (float)(dy + 86 + f * 58), (float)listW, 46.0f };

                    bool fHover = CheckCollisionPointRec(mPos, fRec);

                    bool fActive = (g_caseFileSelected == f);



                    DrawRectangleRec(fRec, fActive ? Color{ 65, 26, 22, 255 } : (fHover ? Color{ 30, 22, 20, 220 } : Color{ 16, 17, 21, 210 }));

                    DrawRectangleLinesEx(fRec, 1.0f, fActive ? Color{ 230, 65, 55, 255 } : (fHover ? WHITE : Color{ 75, 50, 45, 190 }));

                    DrawTextSharp(g_fontMenu, caseTitles[f], (int)fRec.x + 14, (int)fRec.y + 12, 16.0f, fActive ? WHITE : Color{ 195, 190, 180, 230 });



                    if (fHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_caseFileSelected != f) {

                        g_caseFileSelected = f;

                        PlaySound(g_sndMenuNav);

                    }

                }



                // Right Column: Text Report

                int docX = dx + 275;

                int docY = dy + 86;

                int docW = dw - 302;

                int docH = dh - 156;



                DrawRectangle(docX, docY, docW, docH, Color{ 14, 16, 20, 255 });

                DrawRectangleLines(docX, docY, docW, docH, Color{ 55, 42, 38, 220 });



                // Content based on selected case file (Strong Heading Font for title, Large Legible Body for text)

                if (g_caseFileSelected == 0) {

                    DrawTextSharp(g_fontHeadSub, "TRANSCRIPT: 911 LOG // CALL REC 22:14:08", docX + 20, docY + 16, 19.0f, Color{ 235, 70, 60, 255 });

                    DrawTextSharp(g_fontSmall, "LOCATION: Mile Marker 14, Route 9 Northern Pass", docX + 20, docY + 42, 14.0f, Color{ 160, 155, 150, 220 });

                    DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, Color{ 70, 35, 30, 220 });



                    const char* l1 = "\"Patrol, our vehicle radiator blew near the abandoned";

                    const char* l2 = "gas stop. It's pouring rain. There are no lights out here";

                    const char* l3 = "except the pumps. My sister says she heard clicking sounds";

                    const char* l4 = "in the drainage ditch... Wait, something is watching us";

                    const char* l5 = "from the spruce tree line. Send someone out here now--\"";

                    const char* l6 = "[TRANSMISSION CUT - SIGNAL LOST // 00:01:24 RECORDED]";



                    DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l4, docX + 20, docY + 160, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l5, docX + 20, docY + 188, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l6, docX + 20, docY + 228, 17.0f, Color{ 235, 60, 50, 255 });

                } else if (g_caseFileSelected == 1) {

                    DrawTextSharp(g_fontHeadSub, "INCIDENT LOG: MISSING PERSON // SIBLING DOSSIER", docX + 20, docY + 16, 19.0f, Color{ 235, 70, 60, 255 });

                    DrawTextSharp(g_fontSmall, "STATUS: Unresolved / Active Search Warrant", docX + 20, docY + 42, 14.0f, Color{ 160, 155, 150, 220 });

                    DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, Color{ 70, 35, 30, 220 });



                    const char* l1 = "When state troopers inspected the stalled sedan at dawn,";

                    const char* l2 = "the driver's door was swung open into the mud.";

                    const char* l3 = "The passenger side was empty. A pair of footprints led";

                    const char* l4 = "from the road toward the mire. The footprints stopped";

                    const char* l5 = "abruptly 40 feet into the dark mud with no return trail.";

                    const char* l6 = "Only deep claw indentations were pressed into the peat.";



                    DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l4, docX + 20, docY + 160, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l5, docX + 20, docY + 188, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l6, docX + 20, docY + 224, 17.0f, Color{ 215, 210, 200, 245 });

                } else if (g_caseFileSelected == 2) {

                    DrawTextSharp(g_fontHeadSub, "ANOMALOUS ENTITY // CLASSIFICATION: SKELETON HOUND", docX + 20, docY + 16, 19.0f, Color{ 235, 70, 60, 255 });

                    DrawTextSharp(g_fontSmall, "OBSERVER: Station Attendant Security Cam #02", docX + 20, docY + 42, 14.0f, Color{ 160, 155, 150, 220 });

                    DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, Color{ 70, 35, 30, 220 });



                    const char* l1 = "Entity displays the anatomy of a massive canine, but with";

                    const char* l2 = "externalized skeletal structure and exposed vertebral ribs.";

                    const char* l3 = "Exhibits luminescence in ocular cavities when in shadows.";

                    const char* l4 = "Does not consume flesh conventionally; appears drawn to";

                    const char* l5 = "sub-surface mineral deposits and fresh blood pooling";

                    const char* l6 = "around the store's cold storage meat hook.";



                    DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l4, docX + 20, docY + 160, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l5, docX + 20, docY + 188, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l6, docX + 20, docY + 224, 17.0f, Color{ 215, 210, 200, 245 });

                } else {

                    DrawTextSharp(g_fontHeadSub, "FORENSIC GEOLOGY // ANOMALOUS MIRE PRESERVATION", docX + 20, docY + 16, 19.0f, Color{ 235, 70, 60, 255 });

                    DrawTextSharp(g_fontSmall, "SAMPLE ANALYSIS: Route 9 Bog Core 12-F", docX + 20, docY + 42, 14.0f, Color{ 160, 155, 150, 220 });

                    DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, Color{ 70, 35, 30, 220 });



                    const char* l1 = "Core drilling 8 meters into the mire revealed biological";

                    const char* l2 = "specimens buried decades ago with zero cellular decay.";

                    const char* l3 = "Tissues retain hydration and microscopic muscle twitching.";

                    const char* l4 = "Local saying carved into the gas station counter:";

                    const char* l5 = "\"WHAT THE GROUND KEEPS, IT NEVER RELEASES.\"";

                    const char* l6 = "Excavation without proper protective tools is lethal.";



                    DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, Color{ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l4, docX + 20, docY + 162, 15.0f, Color{ 175, 170, 165, 210 });

                    DrawTextSharp(g_fontHeadSub, l5, docX + 20, docY + 186, 17.0f, Color{ 240, 60, 50, 255 });

                    DrawTextSharp(g_fontBody, l6, docX + 20, docY + 224, 15.0f, Color{ 195, 190, 180, 230 });

                }



                // Close Button

                Rectangle btnCloseDossier = { (float)(dx + dw/2 - 85), (float)(dy + dh - 48), 170.0f, 36.0f };

                bool hClose = CheckCollisionPointRec(mPos, btnCloseDossier);

                DrawRectangleRec(btnCloseDossier, hClose ? Color{ 95, 30, 25, 255 } : Color{ 35, 18, 16, 240 });

                DrawRectangleLinesEx(btnCloseDossier, 1.0f, hClose ? WHITE : Color{ 160, 50, 40, 255 });

                DrawTextSharpCentered(g_fontMenu, "CLOSE [ESC]", dx + dw/2, dy + dh - 40, 16.0f, WHITE);

                if (hClose && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    g_showCaseFilesModal = false;

                }

            }

            // =========================================================================

            // ATTENDANT'S CLANDESTINE MONITORING DOSSIER MODAL

            // =========================================================================

            if (g_showDossierModal) {

                int dw = (int)(screenW * 0.70f);

                if (dw < 720) dw = 720;

                if (dw > 920) dw = 920;

                int dh = (int)(screenH * 0.76f);

                if (dh < 500) dw = 500;

                if (dh > 620) dh = 620;

                int dx = screenW/2 - dw/2, dy = screenH/2 - dh/2;



                DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 225 });

                DrawRectangle(dx, dy, dw, dh, Color{ 12, 14, 18, 252 });

                DrawRectangleLines(dx, dy, dw, dh, Color{ 175, 145, 65, 255 });



                DrawTextSharp(g_fontHeadSub, "TOP SECRET // CIVIL DEFENSE & DEEP MONITORING DOSSIER", dx + 28, dy + 22, 17.0f, Color{ 240, 220, 180, 255 });

                DrawTextSharp(g_fontSmall, "LOCATION: ROUTE 9 SERVICE STATION SUB-TERRAIN // AUTH: LEVEL-4 DISPATCH", dx + 28, dy + 46, 12.0f, Color{ 200, 75, 60, 255 });

                DrawLine(dx + 25, dy + 68, dx + dw - 25, dy + 68, Color{ 120, 95, 45, 200 });



                int listW = 230;

                const char* dTitles[4] = {

                    "01. 1984 EXCAVATION",

                    "02. SPECIMEN 07-B",

                    "03. MEAT LOCKER VENT",

                    "04. EMERGENCY CACHE"

                };

                for (int t = 0; t < 4; t++) {

                    Rectangle tRec = { (float)(dx + 25), (float)(dy + 82 + t * 54), (float)listW, 44.0f };

                    bool tHover = CheckCollisionPointRec(mPos, tRec);

                    bool tActive = (g_dossierFileSelected == t);



                    DrawRectangleRec(tRec, tActive ? Color{ 55, 42, 22, 255 } : (tHover ? Color{ 28, 24, 18, 220 } : Color{ 16, 17, 20, 200 }));

                    DrawRectangleLinesEx(tRec, 1.0f, tActive ? Color{ 225, 180, 70, 255 } : (tHover ? WHITE : Color{ 80, 65, 45, 180 }));

                    DrawTextSharp(g_fontBody, dTitles[t], (int)tRec.x + 12, (int)tRec.y + 13, 13.0f, tActive ? WHITE : Color{ 200, 190, 175, 220 });



                    if (tHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_dossierFileSelected != t) {

                        g_dossierFileSelected = t;

                        PlaySound(g_sndMenuNav);

                    }

                }



                int docX = dx + 275, docY = dy + 82, docW = dw - 300, docH = dh - 150;

                DrawRectangle(docX, docY, docW, docH, Color{ 16, 18, 22, 255 });

                DrawRectangleLines(docX, docY, docW, docH, Color{ 65, 55, 42, 220 });



                if (g_dossierFileSelected == 0) {

                    DrawTextSharp(g_fontMenu, "OPERATION SUB-STRATA: COLD WAR EXCAVATION LOG", docX + 20, docY + 16, 14.0f, Color{ 230, 180, 70, 255 });

                    DrawTextSharp(g_fontSmall, "ARCHIVE: Station Foundation Survey (August 1984)", docX + 20, docY + 38, 11.0f, Color{ 160, 155, 145, 220 });

                    DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, Color{ 80, 65, 40, 200 });

                    DrawTextSharp(g_fontBody, "The secret underground corridor was initially excavated", docX + 20, docY + 68, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "during the Cold War as a civilian fallout monitor bunker.", docX + 20, docY + 90, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "However, miners struck hollow fissures 18 feet below.", docX + 20, docY + 112, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "The limestone walls bore massive parallel scrape furrows", docX + 20, docY + 134, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "resembling claw paths. Work was halted indefinitely.", docX + 20, docY + 156, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "Access was sealed beneath the exterior worn rug.", docX + 20, docY + 188, 13.0f, Color{ 200, 150, 60, 255 });

                } else if (g_dossierFileSelected == 1) {

                    DrawTextSharp(g_fontMenu, "ANOMALOUS SPECIMEN 07-B: FORMALIN SUSPENSION", docX + 20, docY + 16, 14.0f, Color{ 230, 180, 70, 255 });

                    DrawTextSharp(g_fontSmall, "CONTAINMENT: Hermetic Glass Jar on Chamber Worktable", docX + 20, docY + 38, 11.0f, Color{ 160, 155, 145, 220 });

                    DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, Color{ 80, 65, 40, 200 });

                    DrawTextSharp(g_fontBody, "A severed juvenile forelimb was recovered from the bog", docX + 20, docY + 68, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "culvert and preserved in formalin. The tissue exhibits", docX + 20, docY + 90, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "external calcified plating and bioluminescent nodes.", docX + 20, docY + 112, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "Microscopic twitching persists even when submerged.", docX + 20, docY + 134, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "Do not break the glass under any circumstance.", docX + 20, docY + 164, 13.0f, Color{ 220, 50, 45, 255 });

                } else if (g_dossierFileSelected == 2) {

                    DrawTextSharp(g_fontMenu, "COLD STORAGE INTERFACE: OVERHEAD VENTILATION GRATE", docX + 20, docY + 16, 14.0f, Color{ 230, 180, 70, 255 });

                    DrawTextSharp(g_fontSmall, "CORRELATION: Supermarket Meat Room & Underground Pit", docX + 20, docY + 38, 11.0f, Color{ 160, 155, 145, 220 });

                    DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, Color{ 80, 65, 40, 200 });

                    DrawTextSharp(g_fontBody, "The overhead ceiling grate connects directly to the floor", docX + 20, docY + 68, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "of the superstore's cold-storage walk-in meat locker.", docX + 20, docY + 90, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "Condensed brine and blood drip into the floor pail.", docX + 20, docY + 112, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "The scent carries deep into subterranean strata.", docX + 20, docY + 134, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "It acts as a scent lure. They gather beneath us at night.", docX + 20, docY + 164, 13.0f, Color{ 225, 45, 40, 255 });

                } else {

                    DrawTextSharp(g_fontMenu, "EMERGENCY SUPPLY CACHE: COMBINATION CODE RECORD", docX + 20, docY + 16, 14.0f, Color{ 230, 180, 70, 255 });

                    DrawTextSharp(g_fontSmall, "CONTAINER: Heavy Cast-Iron Padlocked Crate in Corner", docX + 20, docY + 38, 11.0f, Color{ 160, 155, 145, 220 });

                    DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, Color{ 80, 65, 40, 200 });

                    DrawTextSharp(g_fontBody, "Under the exterior carpet lies a deep mining descent", docX + 20, docY + 68, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "leading to the forgotten 19th-century abandoned village.", docX + 20, docY + 90, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "The midsection is blocked by a massive cave-in.", docX + 20, docY + 112, 13.0f, Color{ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontTitle, "[ TRENCH SHOVEL REQUIRED ]", docX + 20, docY + 138, 17.0f, Color{ 245, 195, 60, 255 });

                    DrawTextSharp(g_fontBody, "Recover the shovel from the supermarket shelves to dig through.", docX + 20, docY + 185, 13.0f, Color{ 200, 195, 180, 240 });

                }



                Rectangle btnCloseDossier = { (float)(dx + dw/2 - 80), (float)(dy + dh - 48), 160.0f, 32.0f };

                bool hClose = CheckCollisionPointRec(mPos, btnCloseDossier);

                DrawRectangleRec(btnCloseDossier, hClose ? Color{ 95, 35, 25, 255 } : Color{ 36, 22, 18, 240 });

                DrawRectangleLinesEx(btnCloseDossier, 1.0f, hClose ? WHITE : Color{ 175, 140, 65, 255 });

                DrawTextSharpCentered(g_fontMenu, "CLOSE DOSSIER [ESC]", dx + dw/2, dy + dh - 40, 13.0f, WHITE);



                if (hClose && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    g_showDossierModal = false;

                    isCursorCaptured = true;

                    DisableCursor();

                }

            }

            // -----------------------------------------------------------------

            // MODAL 2: SURVIVAL SHIFT LOG

            // -----------------------------------------------------------------

            else if (g_showSurvivalModal) {

                int sw = 640, sh = 490;

                int sx = screenW/2 - sw/2, sy = screenH/2 - sh/2;



                DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 215 });

                DrawRectangle(sx, sy, sw, sh, Color{ 10, 11, 14, 252 });

                DrawRectangleLines(sx, sy, sw, sh, Color{ 140, 35, 25, 255 });



                DrawTextSharpCentered(g_fontHeadSub, "S U R V I V A L   R E C O R D", sx + sw/2, sy + 18, 26.0f, Color{ 245, 235, 225, 255 }, 2.0f);

                DrawLine(sx + 35, sy + 50, sx + sw - 35, sy + 50, Color{ 90, 30, 25, 220 });



                DrawTextSharp(g_fontSmall, "ROUTE 9 SERVICE STATION // ATTENDANT SHIFT DOSSIER", sx + 45, sy + 66, 14.0f, Color{ 190, 65, 55, 240 });



                DrawTextSharp(g_fontBody, "SURVEILLANCE ENGINE: ROCK-SOLID 144+ FPS ACTIVE", sx + 45, sy + 98, 16.0f, Color{ 215, 210, 200, 245 });

                DrawTextSharp(g_fontBody, "LOCATION: 44.9184° N, 71.3820° W (MILE 14)", sx + 45, sy + 126, 16.0f, Color{ 195, 190, 185, 230 });

                DrawTextSharp(g_fontBody, "WEATHER TELEMETRY: NIGHT TIME PRECIPITATION (TORRENTIAL)", sx + 45, sy + 154, 16.0f, Color{ 195, 190, 185, 230 });

                DrawTextSharp(g_fontBody, "ANOMALY THREAT LEVEL: HIGH (NOCTURNAL ENTITY ACTIVE)", sx + 45, sy + 182, 16.0f, Color{ 240, 60, 50, 255 });



                DrawLine(sx + 35, sy + 216, sx + sw - 35, sy + 216, Color{ 70, 25, 22, 190 });



                DrawTextSharp(g_fontHeadSub, "INVESTIGATION MILESTONES:", sx + 45, sy + 232, 19.0f, Color{ 235, 225, 215, 255 });

                DrawTextSharp(g_fontBody, "[+] STALLED SEDAN LOCATED ON HIGHWAY", sx + 55, sy + 262, 16.0f, Color{ 150, 215, 160, 245 });

                DrawTextSharp(g_fontBody, "[+] GRETHNAR'S 24/7 STATION ACCESSED", sx + 55, sy + 290, 16.0f, Color{ 150, 215, 160, 245 });

                DrawTextSharp(g_fontBody, "[+] ROOF SURVEILLANCE OPTICS CALIBRATED", sx + 55, sy + 318, 16.0f, Color{ 150, 215, 160, 245 });

                DrawTextSharp(g_fontBody, "[!] MEAT LOCKER BLOOD ANOMALY UNRESOLVED", sx + 55, sy + 346, 16.0f, Color{ 245, 75, 65, 255 });



                // Back Button

                Rectangle btnBackSurv = { (float)(sx + sw/2 - 85), (float)(sy + sh - 48), 170.0f, 36.0f };

                bool hBackS = CheckCollisionPointRec(mPos, btnBackSurv);

                DrawRectangleRec(btnBackSurv, hBackS ? Color{ 95, 30, 25, 255 } : Color{ 35, 18, 16, 240 });

                DrawRectangleLinesEx(btnBackSurv, 1.0f, hBackS ? WHITE : Color{ 160, 50, 40, 255 });

                DrawTextSharpCentered(g_fontMenu, "BACK [ESC]", sx + sw/2, sy + sh - 40, 16.0f, WHITE);

                if (hBackS && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    g_showSurvivalModal = false;

                }

            }

            // -----------------------------------------------------------------

            // MODAL 3: OPTIONS & HORROR CALIBRATION

            // -----------------------------------------------------------------

            else if (g_showSettingsModal) {

                int sw = 660, sh = 520;

                int sx = screenW/2 - sw/2, sy = screenH/2 - sh/2;



                DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 200 });

                DrawRectangle(sx, sy, sw, sh, Color{ 8, 10, 12, 252 });

                DrawRectangleLines(sx, sy, sw, sh, Color{ 140, 35, 25, 255 });



                DrawTextSharpCentered(g_fontHeadSub, "O P T I O N S", sx + sw/2, sy + 18, 26.0f, Color{ 245, 235, 225, 255 }, 2.0f);

                DrawLine(sx + 35, sy + 48, sx + sw - 35, sy + 48, Color{ 90, 30, 25, 220 });



                // 1. Audio Volume

                DrawTextSharp(g_fontBody, "MASTER AUDIO", sx + 45, sy + 60, 16.0f, Color{ 215, 210, 205, 255 });

                Rectangle volTrack = { (float)(sx + 45), (float)(sy + 84), 280.0f, 14.0f };

                DrawRectangleRec(volTrack, Color{ 20, 22, 25, 255 });

                DrawRectangleLinesEx(volTrack, 1.0f, Color{ 75, 38, 32, 240 });

                DrawRectangle((int)volTrack.x, (int)volTrack.y, (int)(volTrack.width * g_userMasterVolume), (int)volTrack.height, Color{ 220, 55, 45, 255 });

                char volStr[32]; snprintf(volStr, sizeof(volStr), "%d%%", (int)(g_userMasterVolume * 100.0f));

                DrawTextSharp(g_fontBody, volStr, sx + 340, sy + 80, 16.0f, Color{ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, Rectangle{ volTrack.x - 10, volTrack.y - 6, volTrack.width + 20, volTrack.height + 16 })) {

                    g_userMasterVolume = Clamp((mPos.x - volTrack.x) / volTrack.width, 0.0f, 1.0f);

                    SetMasterVolume(g_userMasterVolume);

                }



                // 2. Mouse Sensitivity

                DrawTextSharp(g_fontBody, "MOUSE SENSITIVITY", sx + 45, sy + 110, 16.0f, Color{ 215, 210, 205, 255 });

                Rectangle sensTrack = { (float)(sx + 45), (float)(sy + 134), 280.0f, 14.0f };

                DrawRectangleRec(sensTrack, Color{ 20, 22, 25, 255 });

                DrawRectangleLinesEx(sensTrack, 1.0f, Color{ 75, 38, 32, 240 });

                float sensNorm = Clamp((g_userMouseSensitivity - 0.5f) / 2.0f, 0.0f, 1.0f);

                DrawRectangle((int)sensTrack.x, (int)sensTrack.y, (int)(sensTrack.width * sensNorm), (int)sensTrack.height, Color{ 220, 55, 45, 255 });

                char sensStr[32]; snprintf(sensStr, sizeof(sensStr), "%.1fx", g_userMouseSensitivity);

                DrawTextSharp(g_fontBody, sensStr, sx + 340, sy + 130, 16.0f, Color{ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, Rectangle{ sensTrack.x - 10, sensTrack.y - 6, sensTrack.width + 20, sensTrack.height + 16 })) {

                    g_userMouseSensitivity = 0.5f + Clamp((mPos.x - sensTrack.x) / sensTrack.width, 0.0f, 1.0f) * 2.0f;

                }



                // 3. FOV Slider

                DrawTextSharp(g_fontBody, "FIELD OF VIEW", sx + 45, sy + 160, 16.0f, Color{ 215, 210, 205, 255 });

                Rectangle fovTrack = { (float)(sx + 45), (float)(sy + 184), 280.0f, 14.0f };

                DrawRectangleRec(fovTrack, Color{ 20, 22, 25, 255 });

                DrawRectangleLinesEx(fovTrack, 1.0f, Color{ 75, 38, 32, 240 });

                float fovNorm = Clamp((g_userFov - 50.0f) / 40.0f, 0.0f, 1.0f);

                DrawRectangle((int)fovTrack.x, (int)fovTrack.y, (int)(fovTrack.width * fovNorm), (int)fovTrack.height, Color{ 220, 55, 45, 255 });

                char fovStr[32]; snprintf(fovStr, sizeof(fovStr), "%d°", (int)g_userFov);

                DrawTextSharp(g_fontBody, fovStr, sx + 340, sy + 180, 16.0f, Color{ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, Rectangle{ fovTrack.x - 10, fovTrack.y - 6, fovTrack.width + 20, fovTrack.height + 16 })) {

                    g_userFov = 50.0f + Clamp((mPos.x - fovTrack.x) / fovTrack.width, 0.0f, 1.0f) * 40.0f;

                }



                // Horror Gamma Calibration Box

                Rectangle calibBox = { (float)(sx + 420), (float)(sy + 60), 200.0f, 142.0f };

                DrawRectangleRec(calibBox, Color{ 4, 5, 7, 255 });

                DrawRectangleLinesEx(calibBox, 1.0f, Color{ 65, 40, 35, 210 });

                Color faceCol = { (unsigned char)(28 * g_userHorrorGamma), (unsigned char)(28 * g_userHorrorGamma), (unsigned char)(34 * g_userHorrorGamma), 255 };

                DrawCircle((int)calibBox.x + 100, (int)calibBox.y + 48, 26.0f, faceCol);

                DrawCircle((int)calibBox.x + 91, (int)calibBox.y + 43, 3.5f, Color{ 0, 0, 0, 255 });

                DrawCircle((int)calibBox.x + 109, (int)calibBox.y + 43, 3.5f, Color{ 0, 0, 0, 255 });

                DrawTextSharp(g_fontSmall, "CALIBRATION:", (int)calibBox.x + 14, (int)calibBox.y + 88, 13.0f, Color{ 170, 165, 160, 230 });

                DrawTextSharp(g_fontSmall, "Adjust display so", (int)calibBox.x + 14, (int)calibBox.y + 104, 12.0f, Color{ 140, 135, 130, 200 });

                DrawTextSharp(g_fontSmall, "entity is barely visible.", (int)calibBox.x + 14, (int)calibBox.y + 120, 12.0f, Color{ 140, 135, 130, 200 });



                // Fullscreen Toggle

                Rectangle btnFs = { (float)(sx + 45), (float)(sy + 214), 250.0f, 32.0f };

                bool hFs = CheckCollisionPointRec(mPos, btnFs);

                DrawRectangleRec(btnFs, hFs ? Color{ 70, 28, 22, 255 } : Color{ 28, 20, 18, 240 });

                DrawRectangleLinesEx(btnFs, 1.0f, hFs ? Color{ 230, 75, 55, 255 } : Color{ 110, 45, 35, 220 });

                DrawTextSharp(g_fontBody, IsWindowFullscreen() ? "[ F11 ] FULLSCREEN: ON" : "[ F11 ] FULLSCREEN: OFF", sx + 55, sy + 221, 15.0f, WHITE);

                if (hFs && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    ToggleGameFullscreen();

                }



                // Divider

                DrawLine(sx + 35, sy + 258, sx + sw - 35, sy + 258, Color{ 70, 25, 22, 190 });



                // Keybindings Cheatsheet

                DrawTextSharp(g_fontHeadSub, "C O N T R O L S", sx + 45, sy + 270, 17.0f, Color{ 215, 90, 80, 255 });

                const char* ctrlList[7] = {

                    "W / A / S / D     Move / Steer",

                    "LEFT SHIFT        Sprint / Accelerate",

                    "E                 Interact / Examine / Grab",

                    "F                 Toggle Flashlight",

                    "C                 Roof Surveillance Vantage",

                    "TAB / ALT         Free / Lock Mouse",

                    "Q / E             Switch CCTV Cameras"

                };

                for (int c = 0; c < 7; c++) {

                    int cy = sy + 294 + c * 22;

                    DrawTextSharp(g_fontBody, ctrlList[c], sx + 55, cy, 14.0f, Color{ 180, 175, 170, 240 });

                }



                // Back Button

                Rectangle btnBack = { (float)(sx + sw/2 - 85), (float)(sy + sh - 46), 170.0f, 34.0f };

                bool hBack = CheckCollisionPointRec(mPos, btnBack);

                DrawRectangleRec(btnBack, hBack ? Color{ 95, 30, 25, 255 } : Color{ 35, 18, 16, 240 });

                DrawRectangleLinesEx(btnBack, 1.0f, hBack ? WHITE : Color{ 160, 50, 40, 255 });

                DrawTextSharpCentered(g_fontMenu, "BACK [ESC]", sx + sw/2, sy + sh - 38, 16.0f, WHITE);

                if (hBack && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    g_showSettingsModal = false;

                }

            }

            // -----------------------------------------------------------------

            // CORE 5 CHOICES (P L A Y, C A S E   F I L E S, O P T I O N S, S U R V I V A L, Q U I T)

            // -----------------------------------------------------------------

            else {
                float uiAlpha = g_isMenuStartingGame ? Clamp(1.0f - (g_menuPlayTransitionTimer / 0.70f) * 2.5f, 0.0f, 1.0f) : 1.0f;
                float menuDormancyAlpha = uiAlpha * Clamp(g_menuAwakeIntensity, 0.0f, 1.0f);

                if (menuDormancyAlpha > 0.02f) {
                    int menuX = (int)(screenW * 0.085f);
                    int titleY = (int)(screenH * 0.17f);

                    int jx = 0, jy = 0;
                    if (fmodf(timeVal, 8.0f) < 0.05f) {
                        jx = GetRandomValue(-1, 1);
                        jy = GetRandomValue(-1, 1);
                    }

                    // Proximity of beam to title for incandescent gleam
                    float titleDist = Vector2Distance(Vector2{ (float)menuX, (float)titleY }, g_menuLightPos);
                    float titleGleam = Clamp(1.0f - titleDist / 400.0f, 0.0f, 1.0f) * g_menuAwakeIntensity;

                    const char* mainTitle = "WHAT THE GROUND KEEPS";
                    Color shadowCol = { 135, 20, 16, (unsigned char)(210 * menuDormancyAlpha) };
                    Color textCol   = { (unsigned char)(238 + (int)(17 * titleGleam)), (unsigned char)(232 + (int)(23 * titleGleam)), (unsigned char)(224 + (int)(23 * titleGleam)), (unsigned char)(255 * menuDormancyAlpha) };

                    DrawTextSharp(g_fontTitle, mainTitle, menuX + 2 + jx, titleY + 2 + jy, 46.0f, shadowCol, 2.5f);
                    DrawTextSharp(g_fontTitle, mainTitle, menuX + jx, titleY + jy, 46.0f, textCol, 2.5f);

                    // Delicate crimson accent line & poetic atmospheric tagline
                    DrawLine(menuX, titleY + 54, menuX + 360, titleY + 54, Color{ 180, 40, 32, (unsigned char)(200 * menuDormancyAlpha) });
                    DrawTextSharp(g_fontHeadSub, "Some graves were never meant to be opened.", menuX, titleY + 64, 16.0f, Color{ 190, 75, 65, (unsigned char)(210 * menuDormancyAlpha) }, 1.2f);

                    // 5 Prestige Cinematic Survival Horror Menu Options
                    static int s_menuSelection = 0;

                    const char* kMenuLabels[5] = {
                        "Play",
                        "Case Files",
                        "Settings",
                        "Controls",
                        "Quit"
                    };

                    int menuStartY = (int)(screenH * 0.38f);
                    int itemSpacing = 54;

                    Vector2 navMDelta = GetMouseDelta();
                    bool mouseMoved = (fabsf(navMDelta.x) > 0.2f || fabsf(navMDelta.y) > 0.2f);

                    if (!g_isMenuStartingGame) {
                        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                            s_menuSelection = (s_menuSelection + 4) % 5;
                        }
                        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                            s_menuSelection = (s_menuSelection + 1) % 5;
                        }
                    }

                    bool mouseOverAnyItem = false;

                    for (int i = 0; i < 5; i++) {
                        int itemY = menuStartY + i * itemSpacing;
                        float textLen = MeasureTextSharp(g_fontTitle, kMenuLabels[i], 36.0f, 1.5f);
                        Rectangle hitRec = { (float)(menuX - 25), (float)(itemY - 4), textLen + 55.0f, 44.0f };
                        bool hover = CheckCollisionPointRec(mPos, hitRec);
                        if (hover) mouseOverAnyItem = true;

                        if (hover && mouseMoved && !g_isMenuStartingGame) {
                            s_menuSelection = i;
                        }

                        bool isSelected = (s_menuSelection == i);

                        // Smooth hover factor
                        g_menuOptionHover[i] += ((isSelected ? 1.0f : 0.0f) - g_menuOptionHover[i]) * Clamp(dt * 14.0f, 0.0f, 1.0f);
                        float slideX = g_menuOptionHover[i] * 14.0f;

                        // Refined gothic horror font
                        Color labelCol = isSelected ? Color{ 255, 250, 242, (unsigned char)(255 * menuDormancyAlpha) } : Color{ 150, 155, 165, (unsigned char)(190 * menuDormancyAlpha) };
                        if (isSelected) {
                            // Vertical blood-crimson needle
                            DrawRectangle(menuX - 14 + (int)slideX, itemY + 10, 3, 24, Color{ 235, 45, 35, (unsigned char)(255 * menuDormancyAlpha) });
                            // Crimson pointer caret >
                            DrawTextSharp(g_fontTitle, ">", menuX - 4 + (int)slideX, itemY + 6, 28.0f, Color{ 235, 45, 35, (unsigned char)(255 * menuDormancyAlpha) });
                            // Subtle red drop shadow for selected item
                            DrawTextSharp(g_fontTitle, kMenuLabels[i], menuX + 18 + (int)slideX, itemY + 2, 36.0f, Color{ 160, 30, 25, (unsigned char)(170 * menuDormancyAlpha) }, 1.5f);
                            // Underline trace in blood-crimson
                            DrawLine(menuX + 16 + (int)slideX, itemY + 40, menuX + 16 + (int)slideX + (int)textLen, itemY + 40, Color{ 235, 45, 35, (unsigned char)(180 * menuDormancyAlpha) });
                            
                            // Add some eerie randomized symbols that jitter on hover
                            if (!g_isMenuStartingGame && GetRandomValue(0, 100) > 90) {
                                const char* glitched[] = { "+", "x", "-", "|", ".", ":" };
                                DrawTextSharp(g_fontSmall, glitched[GetRandomValue(0, 5)], menuX + 20 + (int)slideX + textLen + GetRandomValue(-4, 4), itemY + 12 + GetRandomValue(-4, 4), 16.0f, Color{ 220, 50, 40, (unsigned char)(120 * menuDormancyAlpha) });
                            }
                        }
                        DrawTextSharp(g_fontTitle, kMenuLabels[i], menuX + 16 + (int)slideX, itemY, 36.0f, labelCol, 1.5f);

                        // Input activation
                        bool clicked = hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
                        bool enterPressed = isSelected && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE));

                        if (!g_isMenuStartingGame) {
                            if (i == 0 && (clicked || enterPressed || IsKeyPressed(KEY_ONE))) {
                                PlaySound(g_sndMenuBoom);
                                g_isMenuStartingGame = true;
                                g_menuPlayTransitionTimer = 0.0f;
                            } else if (i == 1 && (clicked || enterPressed || IsKeyPressed(KEY_TWO))) {
                                PlaySound(g_sndMenuBoom);
                                g_showCaseFilesModal = true;
                            } else if (i == 2 && (clicked || enterPressed || IsKeyPressed(KEY_THREE))) {
                                PlaySound(g_sndMenuBoom);
                                g_showSettingsModal = true;
                            } else if (i == 3 && (clicked || enterPressed || IsKeyPressed(KEY_FOUR))) {
                                PlaySound(g_sndMenuBoom);
                                g_showSurvivalModal = true;
                            } else if (i == 4 && (clicked || enterPressed || IsKeyPressed(KEY_FIVE))) {
                                PlaySound(g_sndMenuBoom);
                                shouldQuitGame = true;
                            }
                        }
                    }

                    // Skull gaze flares with intensity when hovering over interactive options
                    if (mouseOverAnyItem) {
                        g_skullGazeFlare += (1.0f - g_skullGazeFlare) * Clamp(dt * 5.0f, 0.0f, 1.0f);
                    } else {
                        g_skullGazeFlare += (0.0f - g_skullGazeFlare) * Clamp(dt * 3.0f, 0.0f, 1.0f);
                    }

                    if (s_menuSelection != g_prevMenuSelection) {
                        PlaySound(g_sndMenuNav);
                        g_prevMenuSelection = s_menuSelection;
                    }

                    // Custom In-Engine Surveillance Crosshair Reticle
                    if (g_menuAwakeIntensity > 0.05f) {
                        int cx = (int)mPos.x, cy = (int)mPos.y;
                        if (mouseOverAnyItem) {
                            Color retCol = { 235, 55, 45, (unsigned char)(235 * g_menuAwakeIntensity * uiAlpha) };
                            DrawCircle(cx, cy, 2.5f, retCol);
                            DrawLine(cx - 8, cy, cx + 8, cy, retCol);
                            DrawLine(cx, cy - 8, cx, cy + 8, retCol);
                        } else {
                            Color retCol = { 250, 235, 200, (unsigned char)(210 * g_menuAwakeIntensity * uiAlpha) };
                            DrawCircle(cx, cy, 2.0f, retCol);
                            DrawLine(cx - 7, cy, cx - 3, cy, retCol);
                            DrawLine(cx + 3, cy, cx + 7, cy, retCol);
                            DrawLine(cx, cy - 7, cx, cy - 3, retCol);
                            DrawLine(cx, cy + 3, cx, cy + 7, retCol);
                        }
                    }
                }




                // Cinematic transition to gameplay: progressive screen dissolve into pure black

                if (g_isMenuStartingGame) {

                    g_menuPlayTransitionTimer += dt;

                    float pProg = Clamp(g_menuPlayTransitionTimer / 0.70f, 0.0f, 1.0f);

                    DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, pProg));



                    if (pProg >= 1.0f) {

                        g_isMenuStartingGame = false;

                        if (!g_hasPlayedIntro) {

                            introCinematic.Run();

                            g_hasPlayedIntro = true;

                        }

                        g_gameState = STATE_GAMEPLAY;
                        if (!isStormActive) {
                            StopSound(sndRain);
                            rainParticles.clear();
                        }

                        isCursorCaptured = true;

                        DisableCursor();

                        gameIntroFade = 0.8f;

                    }

                }

            }

        }

        // =========================================================================

        // FULL DIEGETIC PSYCHOLOGICAL HORROR PAUSE MENU (STATE_PAUSED)

        // =========================================================================

        else if (g_gameState == STATE_PAUSED) {

            Vector2 mPos = GetMousePosition();



            // Dark horror dimming overlay

            DrawRectangle(0, 0, screenW, screenH, Color{ 6, 8, 12, 215 });



            // Feather-light CRT scanlines

            for (int y = 0; y < screenH; y += 4) {

                DrawLine(0, y, screenW, y, Color{ 0, 0, 0, 24 });

            }



            // Heavy horror vignette

            int vH = screenH / 3;

            int vW = screenW / 4;

            DrawRectangleGradientV(0, 0, screenW, vH, Color{ 0, 0, 0, 240 }, BLANK);

            DrawRectangleGradientV(0, screenH - vH, screenW, vH, BLANK, Color{ 0, 0, 0, 255 });

            DrawRectangleGradientH(0, 0, vW, screenH, Color{ 0, 0, 0, 230 }, BLANK);

            DrawRectangleGradientH(screenW - vW, 0, vW, screenH, BLANK, Color{ 0, 0, 0, 230 });



            // If submodals are open inside pause menu, render them

            if (g_showSettingsModal) {

                int sw = 660, sh = 520;

                int sx = screenW/2 - sw/2, sy = screenH/2 - sh/2;



                DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 200 });

                DrawRectangle(sx, sy, sw, sh, Color{ 8, 10, 12, 252 });

                DrawRectangleLines(sx, sy, sw, sh, Color{ 140, 35, 25, 255 });



                DrawTextSharpCentered(g_fontHeadSub, "O P T I O N S", sx + sw/2, sy + 18, 26.0f, Color{ 245, 235, 225, 255 }, 2.0f);

                DrawLine(sx + 35, sy + 48, sx + sw - 35, sy + 48, Color{ 90, 30, 25, 220 });



                // 1. Audio Volume

                DrawTextSharp(g_fontBody, "MASTER AUDIO", sx + 45, sy + 60, 16.0f, Color{ 215, 210, 205, 255 });

                Rectangle volTrack = { (float)(sx + 45), (float)(sy + 84), 280.0f, 14.0f };

                DrawRectangleRec(volTrack, Color{ 20, 22, 25, 255 });

                DrawRectangleLinesEx(volTrack, 1.0f, Color{ 75, 38, 32, 240 });

                DrawRectangle((int)volTrack.x, (int)volTrack.y, (int)(volTrack.width * g_userMasterVolume), (int)volTrack.height, Color{ 220, 55, 45, 255 });

                char volStr[32]; snprintf(volStr, sizeof(volStr), "%d%%", (int)(g_userMasterVolume * 100.0f));

                DrawTextSharp(g_fontBody, volStr, sx + 340, sy + 80, 16.0f, Color{ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, Rectangle{ volTrack.x - 10, volTrack.y - 6, volTrack.width + 20, volTrack.height + 16 })) {

                    g_userMasterVolume = Clamp((mPos.x - volTrack.x) / volTrack.width, 0.0f, 1.0f);

                    SetMasterVolume(g_userMasterVolume);

                }



                // 2. Mouse Sensitivity

                DrawTextSharp(g_fontBody, "MOUSE SENSITIVITY", sx + 45, sy + 110, 16.0f, Color{ 215, 210, 205, 255 });

                Rectangle sensTrack = { (float)(sx + 45), (float)(sy + 134), 280.0f, 14.0f };

                DrawRectangleRec(sensTrack, Color{ 20, 22, 25, 255 });

                DrawRectangleLinesEx(sensTrack, 1.0f, Color{ 75, 38, 32, 240 });

                float sensNorm = Clamp((g_userMouseSensitivity - 0.5f) / 2.0f, 0.0f, 1.0f);

                DrawRectangle((int)sensTrack.x, (int)sensTrack.y, (int)(sensTrack.width * sensNorm), (int)sensTrack.height, Color{ 220, 55, 45, 255 });

                char sensStr[32]; snprintf(sensStr, sizeof(sensStr), "%.1fx", g_userMouseSensitivity);

                DrawTextSharp(g_fontBody, sensStr, sx + 340, sy + 130, 16.0f, Color{ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, Rectangle{ sensTrack.x - 10, sensTrack.y - 6, sensTrack.width + 20, sensTrack.height + 16 })) {

                    g_userMouseSensitivity = 0.5f + Clamp((mPos.x - sensTrack.x) / sensTrack.width, 0.0f, 1.0f) * 2.0f;

                }



                // 3. FOV Slider

                DrawTextSharp(g_fontBody, "FIELD OF VIEW", sx + 45, sy + 160, 16.0f, Color{ 215, 210, 205, 255 });

                Rectangle fovTrack = { (float)(sx + 45), (float)(sy + 184), 280.0f, 14.0f };

                DrawRectangleRec(fovTrack, Color{ 20, 22, 25, 255 });

                DrawRectangleLinesEx(fovTrack, 1.0f, Color{ 75, 38, 32, 240 });

                float fovNorm = Clamp((g_userFov - 50.0f) / 40.0f, 0.0f, 1.0f);

                DrawRectangle((int)fovTrack.x, (int)fovTrack.y, (int)(fovTrack.width * fovNorm), (int)fovTrack.height, Color{ 220, 55, 45, 255 });

                char fovStr[32]; snprintf(fovStr, sizeof(fovStr), "%d°", (int)g_userFov);

                DrawTextSharp(g_fontBody, fovStr, sx + 340, sy + 180, 16.0f, Color{ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, Rectangle{ fovTrack.x - 10, fovTrack.y - 6, fovTrack.width + 20, fovTrack.height + 16 })) {

                    g_userFov = 50.0f + Clamp((mPos.x - fovTrack.x) / fovTrack.width, 0.0f, 1.0f) * 40.0f;

                }



                // Fullscreen Toggle

                Rectangle btnFs = { (float)(sx + 45), (float)(sy + 214), 250.0f, 32.0f };

                bool hFs = CheckCollisionPointRec(mPos, btnFs);

                DrawRectangleRec(btnFs, hFs ? Color{ 70, 28, 22, 255 } : Color{ 28, 20, 18, 240 });

                DrawRectangleLinesEx(btnFs, 1.0f, hFs ? Color{ 230, 75, 55, 255 } : Color{ 110, 45, 35, 220 });

                DrawTextSharp(g_fontBody, IsWindowFullscreen() ? "[ F11 ] FULLSCREEN: ON" : "[ F11 ] FULLSCREEN: OFF", sx + 55, sy + 221, 15.0f, WHITE);

                if (hFs && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    ToggleGameFullscreen();

                }



                // Divider

                DrawLine(sx + 35, sy + 258, sx + sw - 35, sy + 258, Color{ 70, 25, 22, 190 });



                // Keybindings Cheatsheet

                DrawTextSharp(g_fontHeadSub, "C O N T R O L S", sx + 45, sy + 270, 17.0f, Color{ 215, 90, 80, 255 });

                const char* ctrlList[7] = {

                    "W / A / S / D     Move / Steer",

                    "LEFT SHIFT        Sprint / Accelerate",

                    "E                 Interact / Examine / Grab",

                    "F                 Toggle Flashlight",

                    "C                 Roof Surveillance Vantage",

                    "TAB / ALT         Free / Lock Mouse",

                    "ESC               Pause / Resume Game"

                };

                for (int c = 0; c < 7; c++) {

                    int cy = sy + 294 + c * 22;

                    DrawTextSharp(g_fontBody, ctrlList[c], sx + 55, cy, 14.0f, Color{ 180, 175, 170, 240 });

                }



                // Back Button

                Rectangle btnBack = { (float)(sx + sw/2 - 85), (float)(sy + sh - 46), 170.0f, 34.0f };

                bool hBack = CheckCollisionPointRec(mPos, btnBack);

                DrawRectangleRec(btnBack, hBack ? Color{ 95, 30, 25, 255 } : Color{ 35, 18, 16, 240 });

                DrawRectangleLinesEx(btnBack, 1.0f, hBack ? WHITE : Color{ 160, 50, 40, 255 });

                DrawTextSharpCentered(g_fontMenu, "BACK [ESC]", sx + sw/2, sy + sh - 38, 16.0f, WHITE);

                if (hBack && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    g_showSettingsModal = false;

                }

            } else if (g_showCaseFilesModal) {

                // (Same Case Files modal content as above)

                int dw = (int)(screenW * 0.65f);

                if (dw < 680) dw = 680;

                if (dw > 860) dw = 860;

                int dh = (int)(screenH * 0.72f);

                if (dh < 460) dh = 460;

                if (dh > 560) dh = 560;

                int dx = screenW/2 - dw/2, dy = screenH/2 - dh/2;



                DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 215 });

                DrawRectangle(dx, dy, dw, dh, Color{ 10, 11, 14, 252 });

                DrawRectangleLines(dx, dy, dw, dh, Color{ 140, 35, 25, 255 });



                DrawTextSharp(g_fontMenu, "STATE POLICE DEPARTMENT // CLASSIFIED EVIDENCE DOSSIER", dx + 28, dy + 20, 16.0f, Color{ 220, 215, 205, 255 });

                DrawTextSharp(g_fontSmall, "CASE #89-094 // ROUTE 9 SERVICE STATION & BORDER MIRE", dx + 28, dy + 42, 12.0f, Color{ 160, 50, 45, 240 });

                DrawLine(dx + 25, dy + 62, dx + dw - 25, dy + 62, Color{ 80, 25, 20, 200 });



                int listW = 210;

                const char* caseTitles[4] = { "01. DISPATCH TAPE", "02. SIBLING INCIDENT", "03. THE MIRE HOUND", "04. SUB-SURFACE SOIL" };

                for (int f = 0; f < 4; f++) {

                    Rectangle fRec = { (float)(dx + 25), (float)(dy + 76 + f * 52), (float)listW, 42.0f };

                    bool fHover = CheckCollisionPointRec(mPos, fRec);

                    bool fActive = (g_caseFileSelected == f);



                    DrawRectangleRec(fRec, fActive ? Color{ 50, 22, 18, 255 } : (fHover ? Color{ 24, 18, 16, 220 } : Color{ 14, 15, 18, 200 }));

                    DrawRectangleLinesEx(fRec, 1.0f, fActive ? Color{ 200, 50, 40, 255 } : (fHover ? WHITE : Color{ 60, 40, 35, 180 }));

                    DrawTextSharp(g_fontBody, caseTitles[f], (int)fRec.x + 12, (int)fRec.y + 12, 13.0f, fActive ? WHITE : Color{ 180, 175, 170, 220 });



                    if (fHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_caseFileSelected != f) {

                        g_caseFileSelected = f;

                        PlaySound(g_sndMenuNav);

                    }

                }



                int docX = dx + 255, docY = dy + 76, docW = dw - 280, docH = dh - 140;

                DrawRectangle(docX, docY, docW, docH, Color{ 14, 16, 20, 255 });

                DrawRectangleLines(docX, docY, docW, docH, Color{ 45, 35, 32, 220 });



                if (g_caseFileSelected == 0) {

                    DrawTextSharp(g_fontMenu, "TRANSCRIPT: 911 LOG // CALL REC 22:14:08", docX + 18, docY + 16, 14.0f, Color{ 220, 60, 50, 255 });

                    DrawTextSharp(g_fontSmall, "LOCATION: Mile Marker 14, Route 9 Northern Pass", docX + 18, docY + 38, 11.0f, Color{ 150, 145, 140, 220 });

                    DrawLine(docX + 18, docY + 54, docX + docW - 18, docY + 54, Color{ 55, 30, 25, 200 });

                    DrawTextSharp(g_fontBody, "\"Patrol, our vehicle radiator blew near the abandoned", docX + 18, docY + 68, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "gas stop. It's pouring rain. There are no lights out here", docX + 18, docY + 90, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "except the pumps. My sister says she heard clicking sounds", docX + 18, docY + 112, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "in the drainage ditch... Wait, something is watching us", docX + 18, docY + 134, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "from the spruce tree line. Send someone out here now--\"", docX + 18, docY + 156, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "[TRANSMISSION CUT - SIGNAL LOST // 00:01:24 RECORDED]", docX + 18, docY + 192, 13.0f, Color{ 210, 45, 40, 255 });

                } else if (g_caseFileSelected == 1) {

                    DrawTextSharp(g_fontMenu, "INCIDENT LOG: MISSING PERSON // SIBLING DOSSIER", docX + 18, docY + 16, 14.0f, Color{ 220, 60, 50, 255 });

                    DrawTextSharp(g_fontSmall, "STATUS: Unresolved / Active Search Warrant", docX + 18, docY + 38, 11.0f, Color{ 150, 145, 140, 220 });

                    DrawLine(docX + 18, docY + 54, docX + docW - 18, docY + 54, Color{ 55, 30, 25, 200 });

                    DrawTextSharp(g_fontBody, "When state troopers inspected the stalled sedan at dawn,", docX + 18, docY + 68, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "the driver's door was swung open into the mud.", docX + 18, docY + 90, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "The passenger side was empty. A pair of footprints led", docX + 18, docY + 112, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "from the road toward the mire. The footprints stopped", docX + 18, docY + 134, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "abruptly 40 feet into the dark mud with no return trail.", docX + 18, docY + 156, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Only deep claw indentations were pressed into the peat.", docX + 18, docY + 184, 13.0f, Color{ 200, 195, 185, 240 });

                } else if (g_caseFileSelected == 2) {

                    DrawTextSharp(g_fontMenu, "ANOMALOUS ENTITY // CLASSIFICATION: SKELETON HOUND", docX + 18, docY + 16, 14.0f, Color{ 220, 60, 50, 255 });

                    DrawTextSharp(g_fontSmall, "OBSERVER: Station Attendant Security Cam #02", docX + 18, docY + 38, 11.0f, Color{ 150, 145, 140, 220 });

                    DrawLine(docX + 18, docY + 54, docX + docW - 18, docY + 54, Color{ 55, 30, 25, 200 });

                    DrawTextSharp(g_fontBody, "Entity displays the anatomy of a massive canine, but with", docX + 18, docY + 68, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "externalized skeletal structure and exposed vertebral ribs.", docX + 18, docY + 90, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Exhibits luminescence in ocular cavities when in shadows.", docX + 18, docY + 112, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Does not consume flesh conventionally; appears drawn to", docX + 18, docY + 134, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "sub-surface mineral deposits and fresh blood pooling", docX + 18, docY + 156, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "around the store's cold storage meat hook.", docX + 18, docY + 184, 13.0f, Color{ 200, 195, 185, 240 });

                } else {

                    DrawTextSharp(g_fontMenu, "FORENSIC GEOLOGY // ANOMALOUS MIRE PRESERVATION", docX + 18, docY + 16, 14.0f, Color{ 220, 60, 50, 255 });

                    DrawTextSharp(g_fontSmall, "SAMPLE ANALYSIS: Route 9 Bog Core 12-F", docX + 18, docY + 38, 11.0f, Color{ 150, 145, 140, 220 });

                    DrawLine(docX + 18, docY + 54, docX + docW - 18, docY + 54, Color{ 55, 30, 25, 200 });

                    DrawTextSharp(g_fontBody, "Core drilling 8 meters into the mire revealed biological", docX + 18, docY + 68, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "specimens buried decades ago with zero cellular decay.", docX + 18, docY + 90, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Tissues retain hydration and microscopic muscle twitching.", docX + 18, docY + 112, 13.0f, Color{ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Local saying carved into the gas station counter:", docX + 18, docY + 140, 12.0f, Color{ 160, 155, 150, 200 });

                    DrawTextSharp(g_fontMenu, "\"WHAT THE GROUND KEEPS, IT NEVER RELEASES.\"", docX + 18, docY + 160, 13.0f, Color{ 230, 45, 40, 255 });

                    DrawTextSharp(g_fontBody, "Excavation without proper protective tools is lethal.", docX + 18, docY + 188, 12.0f, Color{ 180, 175, 170, 220 });

                }



                Rectangle btnCloseDossier = { (float)(dx + dw/2 - 70), (float)(dy + dh - 44), 140.0f, 30.0f };

                bool hClose = CheckCollisionPointRec(mPos, btnCloseDossier);

                DrawRectangleRec(btnCloseDossier, hClose ? Color{ 80, 25, 20, 255 } : Color{ 30, 16, 14, 240 });

                DrawRectangleLinesEx(btnCloseDossier, 1.0f, hClose ? WHITE : Color{ 140, 45, 35, 255 });

                DrawTextSharpCentered(g_fontBody, "CLOSE [ESC]", dx + dw/2, dy + dh - 36, 13.0f, WHITE);

                if (hClose && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    g_showCaseFilesModal = false;

                }

            } else {

                // Pause Menu Main Display (Strong Gothic Heading Font)

                const char* pauseTitle = "INVESTIGATION SUSPENDED";

                int pauseY = (int)(screenH * 0.18f);

                DrawTextSharpCentered(g_fontTitle, pauseTitle, screenW/2 + 3, pauseY + 3, 44.0f, Color{ 120, 18, 14, 210 }, 3.0f);

                DrawTextSharpCentered(g_fontTitle, pauseTitle, screenW/2, pauseY, 44.0f, Color{ 245, 240, 232, 255 }, 3.0f);



                DrawTextSharpCentered(g_fontSmall, "ROUTE 9 SERVICE STATION // SIMULATION FROZEN", screenW/2, pauseY + 50, 14.0f, Color{ 190, 65, 55, 240 });



                static int s_pauseSelection = 0;

                static int s_prevPauseSelection = 0;

                const char* pauseOpts[5] = {

                    "RESUME INVESTIGATION",

                    "SYSTEM OPTIONS",

                    "CASE FILES",

                    "MAIN MENU",

                    "QUIT TO DESKTOP"

                };



                int startY = (int)(screenH * 0.40f);

                int spacing = 56;



                Vector2 mDelta = GetMouseDelta();

                bool mouseMoved = (fabsf(mDelta.x) > 0.2f || fabsf(mDelta.y) > 0.2f);



                if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {

                    s_pauseSelection = (s_pauseSelection + 4) % 5;

                }

                if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {

                    s_pauseSelection = (s_pauseSelection + 1) % 5;

                }



                for (int i = 0; i < 5; i++) {

                    float ow = MeasureTextSharp(g_fontMenu, pauseOpts[i], 28.0f);

                    int ox = (int)(screenW/2 - ow/2);

                    int oy = startY + i * spacing;



                    Rectangle hitRec = { (float)(ox - 45), (float)(oy - 6), (float)(ow + 90), 42.0f };

                    bool hover = CheckCollisionPointRec(mPos, hitRec);

                    if (hover && mouseMoved) s_pauseSelection = i;



                    bool isSel = (s_pauseSelection == i);



                    if (isSel) {

                        float pulse = 0.5f + 0.5f * sinf(timeVal * 4.5f);

                        Color emberCol = { (unsigned char)(210 + 40 * pulse), (unsigned char)(35 + 20 * pulse), 25, 255 };

                        DrawLine(ox - 30, oy + 36, (int)(ox + ow + 30), oy + 36, emberCol);

                        DrawLine(ox - 20, oy + 37, (int)(ox + ow + 20), oy + 37, Color{ emberCol.r, emberCol.g, emberCol.b, 150 });

                        DrawTextSharp(g_fontMenu, ">", ox - 35, oy, 28.0f, emberCol);

                        DrawTextSharp(g_fontMenu, "<", ox + ow + 18, oy, 28.0f, emberCol);

                    }



                    Color optCol = isSel ? WHITE : Color{ 175, 170, 160, 220 };

                    DrawTextSharp(g_fontMenu, pauseOpts[i], ox, oy, 28.0f, optCol);



                    bool clicked = hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

                    bool enterPressed = isSel && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE));



                    if (i == 0 && (clicked || enterPressed || IsKeyPressed(KEY_ONE))) {

                        // RESUME

                        g_gameState = STATE_GAMEPLAY;

                        isCursorCaptured = true;

                        DisableCursor();

                        PlaySound(g_sndMenuNav);

                        escCooldown = 0.25f;

                    } else if (i == 1 && (clicked || enterPressed || IsKeyPressed(KEY_TWO))) {

                        // OPTIONS

                        PlaySound(g_sndMenuBoom);

                        g_showSettingsModal = true;

                    } else if (i == 2 && (clicked || enterPressed || IsKeyPressed(KEY_THREE))) {

                        // CASE FILES

                        PlaySound(g_sndMenuBoom);

                        g_showCaseFilesModal = true;

                    } else if (i == 3 && (clicked || enterPressed || IsKeyPressed(KEY_FOUR))) {

                        // MAIN MENU

                        PlaySound(g_sndMenuBoom);

                        g_gameState = STATE_MAIN_MENU;

                        isCursorCaptured = false;

                        EnableCursor();

                    } else if (i == 4 && (clicked || enterPressed || IsKeyPressed(KEY_FIVE))) {

                        // QUIT

                        PlaySound(g_sndMenuBoom);

                        shouldQuitGame = true;

                    }

                }



                if (s_pauseSelection != s_prevPauseSelection) {

                    PlaySound(g_sndMenuNav);

                    s_prevPauseSelection = s_pauseSelection;

                }

            }

        }



        // ---------------------------------------------------------------------
        // Silky-smooth post-intro cinematic fade-in from black
        if (gameIntroFade > 0.0f) {
            gameIntroFade = fmaxf(0.0f, gameIntroFade - dt * 1.5f);
            DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, gameIntroFade));
        }

        // CURSOR UNLOCKED / RESIZE WINDOW BANNER (Clean UI notification)
        // Clean AAA HUD: FPS counter gated behind F3 debug toggle
        if (IsKeyPressed(KEY_F3)) g_showDebugFPS = !g_showDebugFPS;
        if (g_showDebugFPS && g_gameState == STATE_GAMEPLAY) DrawFPS(10, 10);

        EndDrawing();

        if (g_testFrames > 0) {
            g_testFrameCount++;
            if (g_testFrameCount >= g_testFrames) {
                CloudProfilingMetrics m = GetCloudProfilingMetrics();
                printf("\n--- VOLUMETRIC CLOUD & ATMOSPHERE PROFILING METRICS ---\n");
                printf("  Cloud Raymarch Pass (Half-Res):   %.3f ms\n", m.cloudPassMs);
                printf("  Temporal Accumulation & Reproj:   %.3f ms\n", m.accumPassMs);
                printf("  Bilateral Upscale Reconstruction: %.3f ms\n", m.upscalePassMs);
                printf("  World Shadow Map Pass (512x512):  %.3f ms\n", m.shadowPassMs);
                printf("  Total System Frame Cost:          %.3f ms\n", m.totalSystemMs);
                printf("------------------------------------------------------\n\n");
                TakeScreenshot(g_testScreenshot);
                break;
            }
        }

    }

    

    UnloadSound(sndFootstep);

    UnloadSound(sndCrickets);

    UnloadSound(sndWind);

    UnloadSound(sndSpark);

    UnloadSound(sndJumpscare);

    UnloadSound(g_sndGunshot);

    UnloadSound(g_sndStoreFootstep);

    UnloadSound(g_sndFoil);

    UnloadSound(g_sndMenuNav);

    UnloadSound(g_sndMenuBoom);

    UnloadSound(g_sndRadioStatic);

    UnloadSound(g_sndWaterDrip);

    UnloadSound(g_sndChestOpen);

    UnloadSound(g_sndShovelDig);

    UnloadSound(g_sndPhoneSlide);

    UnloadSound(g_sndPhoneTap);

    UnloadSound(g_sndFlashlightToggle);
    UnloadSound(g_sndLightSwitch);
    UnloadSound(g_sndNozzleLatch);
    UnloadSound(g_sndNozzleShutoff);
    UnloadSound(g_sndStepperMotor);

    CloseAudioDevice();

    if (g_fontTitle.texture.id != GetFontDefault().texture.id)   UnloadFont(g_fontTitle);
    if (g_fontHeadSub.texture.id != GetFontDefault().texture.id) UnloadFont(g_fontHeadSub);
    if (g_fontMenu.texture.id != GetFontDefault().texture.id)    UnloadFont(g_fontMenu);
    if (g_fontBody.texture.id != GetFontDefault().texture.id)    UnloadFont(g_fontBody);
    if (g_fontSmall.texture.id != GetFontDefault().texture.id)   UnloadFont(g_fontSmall);

    if (g_proceduralAssetsLoaded) {
        UnloadProceduralShopAssets();
    }

    if (g_pumpScreenRTLoaded) {
        UnloadRenderTexture(g_pumpScreenRT);
        g_pumpScreenRTLoaded = false;
    }

    UnloadRenderTexture(target);

    UnloadTexture(g_receiptTex);

    UnloadShader(instancedShader);

    UnloadTexture(atlas);

    UnloadMesh(mGround);

    UnloadMaterial(matGround);

    UnloadMesh(mRoad);

    UnloadMaterial(matRoad);

    UnloadMesh(quad);

    UnloadBovineMeshes();

    UnloadHoundResources();

    UnloadShovel(g_shovelRig);

    delete chunk;

    UnloadCollegeShaderAndTextures();

    UnloadOceanSystem();

    UnloadShopAtmosphere();
    UnloadATMSystem();

    CleanupCloudSystem();

    CloseWindow();

    return 0;
    exit(0);

}