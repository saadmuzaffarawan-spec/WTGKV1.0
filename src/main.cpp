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



// ============================================================

// WHAT THE GROUND KEEPS - 3D ASCII ENGINE

// Features: Voxel Grid, Physics Debris, Foliage Sway, Skybox & Fog

// ============================================================










































// --- KINEMATIC HORROR CAMERA & VIEWMODEL INERTIA CONTROLLER ---

static float g_vmSwayX        = 0.0f; // Viewmodel mouse yaw inertial lag

static float g_vmSwayY        = 0.0f; // Viewmodel mouse pitch inertial lag

static float g_camIdleTimer   = 0.0f; // Stationary breathing sway timer

static float g_camRoll        = 0.0f; // Dynamic camera roll / banking angle (radians)

static float g_camStepOffset  = 0.0f; // Damped step-up vertical smoothing offset

static float g_camLandingDip  = 0.0f; // Downward kinetic impact dip upon landing

static float g_lastPlayerVelY = 0.0f; // Previous frame vertical velocity for impact check

static bool  g_wasOnGround    = true; // Previous frame grounded state

static float g_camDynamicFov  = 60.0f;// Dynamic FOV expanded during sprint

static bool  g_isSprinting    = false;// Sprint active flag ([Left Shift])



// --- DYNAMIC FLASHLIGHT & WORLD LIGHTING CONTEXT ---

static bool    g_flashlightActive       = false; // [F] Key flashlight active state


// --- FUNCTIONAL GAS STATION & FUEL SELLING ECONOMY ---
static Sound g_sndCashRegister;
static Sound g_sndDrivewayBell;
static Sound g_sndPumpFlow;
static Sound g_sndNozzleLatch;
static Sound g_sndNozzleShutoff;

// 3D In-World Fuel Pump CRT Monitor Texture
static RenderTexture2D g_pumpScreenRT;
static bool            g_pumpScreenRTLoaded = false;

static float g_stationFuelGallons  = 450.0f; // 10,000-Gal Underground Tank (Starts at 450.0 Gal)
static float g_fuelPricePerGallon = 3.89f;  // Fuel price per gallon
static float g_playerCash         = 45.00f; // Starting wallet
static float g_cashPopupAmount    = 0.0f;
static float g_cashPopupTimer     = 0.0f;
static char  g_cashPopupText[80]  = { 0 };

static bool  g_holdingFuelNozzle   = false;
static int   g_activePumpIndex     = -1;    // 0 = Pump 1, 1 = Pump 2
static bool  g_nozzleInCar         = false;
static float g_fuelSoundCooldown   = 0.0f;
static bool  g_autoDispenseMode    = false; // Store counter console toggle

static float g_stationBellBannerTimer = 0.0f;
static char  g_stationBellBanner[128] = { 0 };

#include "systems/gas_station_system.h"

static CustomerCar g_customerCar = { CAR_INACTIVE };
static float g_customerCarCooldown = 35.0f; // Seconds until next car arrives

static inline void SpawnCustomerCar() {
    SpawnCustomerCar(g_customerCar, g_fuelPricePerGallon);
}


static Sound   g_sndFlashlightToggle;            // Tactile switch click sound

static bool    g_shopLightsOn           = true;  // Store master light switch state

static Sound   g_sndLightSwitch;                 // Master relay toggle switch sound

static float   g_curExtDayFactor        = 0.5f;

static float   g_curExtNightFactor      = 0.5f;

static float   g_curLightningFlash      = 0.0f;

static Vector3 g_curSunDir              = { 0.0f, 1.0f, 0.0f };

static Vector3 g_playerCamPos           = { 100.0f, 12.2f, 140.0f };

static Vector3 g_playerCamFwd           = { 0.0f, 0.0f, 1.0f };



static Sound g_sndGunshot;

static Sound g_sndStoreFootstep;

static Sound g_sndFoil;

static Sound g_sndMenuNav;

static Sound g_sndMenuBoom;

static Sound g_sndRadioStatic;

static Sound g_sndWaterDrip;

static Sound g_sndChestOpen;

static Sound g_sndShovelDig;

static Sound g_sndPhoneSlide;

static Sound g_sndPhoneTap;



// --- DISTINCT STRONG HEADING & HIGH-LEGIBILITY MENU FONTS FROM ASSETS ---

static Font g_fontTitle;    // assets/alagard.ttf @ 52px - Strong, gothic horror display font for main titles

static Font g_fontHeadSub;  // assets/alagard.ttf @ 30px - Distinct horror display font for headers & banners

Font g_fontMenu;            // assets/IBMPlexMono-Bold.ttf @ 28px - Prominent, high-legibility monospace for menus & buttons

static Font g_fontBody;     // assets/IBMPlexMono-Medium.ttf @ 20px - Crisp monospace for lore transcripts & settings

static Font g_fontSmall;    // assets/IBMPlexMono-Regular.ttf @ 16px - Telemetry, timestamps & status stamps

#include "systems/ui_helpers.h"

// =========================================================================
// AAA POLISHED UI PRIMITIVES & DESIGN SYSTEM
// =========================================================================



static int g_heldProductIndex = -1;



static RenderTexture2D g_milkTexRT;

static RenderTexture2D g_bloodTexRT;

static RenderTexture2D g_crustTexRT;

static Model g_milkLiquidModel;

static Model g_bloodLiquidModel;

static Model g_breadModel;

static bool g_proceduralAssetsLoaded = false;

static Vector3 g_cartPos = { 104.5f, 10.02f, 142.5f };

static Vector3 g_cartVel = { 0.0f, 0.0f, 0.0f };

static float   g_cartYaw = -90.0f; // Body orientation (facing -X down aisle)

static float   g_cartWheelSpin = 0.0f;

static bool    g_isHoldingCart = false;

static std::vector<int> g_cartProductIndices;



// --- HORROR THERMAL RECEIPT PRINTER & RECEIPT GLOBALS ---

enum PrinterState { PRINTER_IDLE = 0, PRINTER_PRINTING, PRINTER_DONE };

static PrinterState g_printerState = PRINTER_IDLE;

static float g_printerProgress = 0.0f; // 0 to 16

static Texture2D g_receiptTex;

static bool  g_hasReceipt = false;

static bool  g_inspectingReceipt = false;

static bool  g_receiptThrown = false;

static Vector3 g_thrownReceiptPos = { 0.0f, 0.0f, 0.0f };



// --- NOCTURNAL AUTONOMOUS GHOST TROLLEY ---

struct GhostCart {

    Vector3 pos;

    float yaw;

    float wheelSpin;

    float alpha;

    int waypoint;

    float waitTimer;

    int itemsInCart;

    bool active;

};

static GhostCart g_ghostCart = { { 97.0f, 10.02f, 142.5f }, 180.0f, 0.0f, 0.0f, 0, 5.0f, 0, false };



static float LerpAngleDeg(float a, float b, float t) {

    float diff = fmodf(b - a + 180.0f, 360.0f);

    if (diff < 0) diff += 360.0f;

    diff -= 180.0f;

    return a + diff * t;

}



static float Frand(float lo, float hi) {

    return lo + (float)GetRandomValue(0, 10000) / 10000.0f * (hi - lo);

}



#include "systems/texture_factories.h"



void InitProceduralShopAssets() {

    if (g_proceduralAssetsLoaded) return;



    g_milkTexRT  = GenerateGlyphTexture("~", (Color){ 245, 245, 250, 255 },

                                             (Color){ 220, 225, 235, 255 }, 256, 26, 3);

    g_bloodTexRT = GenerateGlyphTexture("~", (Color){ 70, 4, 6, 255 },

                                             (Color){ 150, 10, 14, 255 }, 256, 26, 3);

    g_crustTexRT = GenerateCrustTexture(256);



    g_milkLiquidModel  = MakeTexturedCylinder(0.5f, 1.0f, 20, g_milkTexRT.texture);

    g_bloodLiquidModel = MakeTexturedCylinder(0.5f, 1.0f, 20, g_bloodTexRT.texture);

    g_breadModel       = MakeTexturedSphere(0.14f, 14, 18, g_crustTexRT.texture);



    g_shopProducts.clear();

    g_shopProducts.reserve(160);

    g_shopParticles.reserve(256);



    // Shelf spacing across 3 bays in X: [89..93], [93..97], [97..101]

    // 3 items per bay = 9 items per shelf tier

    const float xOffsets[9] = {

        90.1f, 91.0f, 91.9f,

        94.1f, 95.0f, 95.9f,

        98.1f, 99.0f, 99.9f

    };



    // =========================================================================

    // RACK 1 (Gondola centered at Z = 140.0) - DOUBLE SIDED

    // North face at Z = 140.42f (facing central aisle +Z)

    // South face at Z = 139.58f (facing south aisle -Z)

    // =========================================================================

    float z1_north = 140.42f;

    float z1_south = 139.58f;



    // --- Rack 1 North Face (+Z) ---

    // Tier 4 (Top, Y = 13.53): Popcorn Tins

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_TIN; it.homePos = { xOffsets[i], 13.53f, z1_north };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 1.0f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 45;

        it.label = "Popcorn Tin"; it.price = "$4.50"; g_shopProducts.push_back(it);

    }

    // Tier 3 (Middle, Y = 12.48): Sparrow's milk

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_MILK; it.homePos = { xOffsets[i], 12.48f, z1_north };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 0.5f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 50;

        it.label = "Sparrow's milk"; it.price = "$3.25"; g_shopProducts.push_back(it);

    }

    // Tier 2 (Lower, Y = 11.43): B+ Blood

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_BLOOD; it.homePos = { xOffsets[i], 11.43f, z1_north };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 0.5f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 50;

        it.label = "B+ Blood"; it.price = "$6.66"; g_shopProducts.push_back(it);

    }

    // Tier 1 (Base, Y = 10.39): Bread

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_BREAD; it.homePos = { xOffsets[i], 10.39f, z1_north };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 1.0f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 0;

        it.label = "Bread"; it.price = "$2.00"; g_shopProducts.push_back(it);

    }



    // --- Rack 1 South Face (-Z) ---

    // Tier 4 (Top, Y = 13.53): B+ Blood

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_BLOOD; it.homePos = { xOffsets[i], 13.53f, z1_south };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 0.5f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 50;

        it.label = "B+ Blood"; it.price = "$6.66"; g_shopProducts.push_back(it);

    }

    // Tier 3 (Middle, Y = 12.48): Bread

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_BREAD; it.homePos = { xOffsets[i], 12.48f, z1_south };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 1.0f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 0;

        it.label = "Bread"; it.price = "$2.00"; g_shopProducts.push_back(it);

    }

    // Tier 2 (Lower, Y = 11.43): Sparrow's milk

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_MILK; it.homePos = { xOffsets[i], 11.43f, z1_south };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 0.5f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 50;

        it.label = "Sparrow's milk"; it.price = "$3.25"; g_shopProducts.push_back(it);

    }

    // Tier 1 (Base, Y = 10.39): Popcorn Tin

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_TIN; it.homePos = { xOffsets[i], 10.39f, z1_south };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 1.0f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 45;

        it.label = "Popcorn Tin"; it.price = "$4.50"; g_shopProducts.push_back(it);

    }



    // =========================================================================

    // RACK 2 (Gondola centered at Z = 147.0) - DOUBLE SIDED

    // South face at Z = 146.58f (facing central aisle -Z)

    // North face at Z = 147.42f (facing north aisle +Z)

    // =========================================================================

    float z2_south = 146.58f;

    float z2_north = 147.42f;



    // --- Rack 2 South Face (-Z) ---

    // Tier 4 (Top, Y = 13.53): B+ Blood

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_BLOOD; it.homePos = { xOffsets[i], 13.53f, z2_south };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 0.5f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 50;

        it.label = "B+ Blood"; it.price = "$6.66"; g_shopProducts.push_back(it);

    }

    // Tier 3 (Middle, Y = 12.48): Popcorn Tin

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_TIN; it.homePos = { xOffsets[i], 12.48f, z2_south };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 1.0f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 45;

        it.label = "Popcorn Tin"; it.price = "$4.50"; g_shopProducts.push_back(it);

    }

    // Tier 2 (Lower, Y = 11.43): Bread

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_BREAD; it.homePos = { xOffsets[i], 11.43f, z2_south };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 1.0f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 0;

        it.label = "Bread"; it.price = "$2.00"; g_shopProducts.push_back(it);

    }

    // Tier 1 (Base, Y = 10.39): Sparrow's milk

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_MILK; it.homePos = { xOffsets[i], 10.39f, z2_south };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 0.5f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 50;

        it.label = "Sparrow's milk"; it.price = "$3.25"; g_shopProducts.push_back(it);

    }



    // --- Rack 2 North Face (+Z) ---

    // Tier 4 (Top, Y = 13.53): Sparrow's milk

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_MILK; it.homePos = { xOffsets[i], 13.53f, z2_north };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 0.5f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 50;

        it.label = "Sparrow's milk"; it.price = "$3.25"; g_shopProducts.push_back(it);

    }

    // Tier 3 (Middle, Y = 12.48): B+ Blood

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_BLOOD; it.homePos = { xOffsets[i], 12.48f, z2_north };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 0.5f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 50;

        it.label = "B+ Blood"; it.price = "$6.66"; g_shopProducts.push_back(it);

    }

    // Tier 2 (Lower, Y = 11.43): Popcorn Tin

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_TIN; it.homePos = { xOffsets[i], 11.43f, z2_north };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 1.0f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 45;

        it.label = "Popcorn Tin"; it.price = "$4.50"; g_shopProducts.push_back(it);

    }

    // Tier 1 (Base, Y = 10.39): Bread

    for (int i = 0; i < 9; i++) {

        ShopProduct it; it.type = PROD_BREAD; it.homePos = { xOffsets[i], 10.39f, z2_north };

        it.originalPos = it.homePos; it.vel = {0,0,0}; it.held = false; it.opened = false;

        it.fill = 1.0f; it.spawnTimer = 0.0f; it.spawnedCount = 0; it.maxSpawn = 0;

        it.label = "Bread"; it.price = "$2.00"; g_shopProducts.push_back(it);

    }



    // =========================================================================

    // SHOPPING CART (Placed in superstore aisle near entrance)

    // =========================================================================

    {

        ShopProduct cart;

        cart.type = PROD_CART;

        cart.homePos = g_cartPos;

        cart.originalPos = cart.homePos;

        cart.vel = { 0, 0, 0 };

        cart.held = false; cart.opened = false;

        cart.fill = 1.0f; cart.spawnTimer = 0.0f; cart.spawnedCount = 0; cart.maxSpawn = 0;

        cart.label = "Shopping Cart";

        cart.price = "N/A";

        g_shopProducts.push_back(cart);

    }



    // =========================================================================

    // HORIZONTAL REFRIGERATOR / FREEZER CHOCOLATES

    // Centered at X = 94.5, Z = 133.5, Y = 10.52 (inside cooler baskets)

    // 4 Baskets along X: 93.15, 94.05, 94.95, 95.85

    // =========================================================================

    const float chocoX[4] = { 93.15f, 94.05f, 94.95f, 95.85f };

    const float chocoZ[3] = { 133.25f, 133.50f, 133.75f };

    for (int b = 0; b < 4; b++) {

        for (int k = 0; k < 3; k++) {

            ShopProduct ch;

            ch.type = PROD_CHOCOLATE;

            ch.homePos = { chocoX[b], 10.52f, chocoZ[k] };

            ch.originalPos = ch.homePos;

            ch.vel = { 0, 0, 0 };

            ch.held = false;

            ch.opened = false;

            ch.fill = 1.0f;

            ch.spawnTimer = 0.0f;

            ch.spawnedCount = 0;

            ch.maxSpawn = 0;

            ch.subType = (b < 3) ? b : k; // 0: Dark Noir, 1: Alpine Milk, 2: Sea Salt Caramel

            if (ch.subType == 0) {

                ch.label = "Dark Noir 85% Cacao";

                ch.price = "$3.50";

            } else if (ch.subType == 1) {

                ch.label = "Alpine Milk Chocolate";

                ch.price = "$2.75";

            } else {

                ch.label = "Sea Salt Caramel";

                ch.price = "$3.25";

            }

            g_shopProducts.push_back(ch);

        }

    }



    // =========================================================================

    // TACTICAL 9MM PISTOL (On presentation tray at checkout counter)

    // Countertop at X = 103.2, Y = 11.58, Z = 133.5

    // =========================================================================

    {

        ShopProduct gun;

        gun.type = PROD_GUN;

        gun.homePos = { 103.2f, 11.58f, 133.5f };

        gun.originalPos = gun.homePos;

        gun.vel = { 0, 0, 0 };

        gun.held = false;

        gun.opened = false;

        gun.fill = 1.0f;

        gun.spawnTimer = 0.0f;

        gun.spawnedCount = 0;

        gun.maxSpawn = 0;

        gun.subType = 0;

        gun.label = "Tactical 9mm Pistol";

        gun.price = "$450.00";

        g_shopProducts.push_back(gun);

    }



    g_proceduralAssetsLoaded = true;

}



// =========================================================================

// SHOP LIGHTING CONTEXT & PHYSICAL MULTI-LIGHT ILLUMINATION ENGINE

// Evaluates light contributions from the central swaying bulb and ceiling tubelights

// =========================================================================

struct ShopLightSource {

    Vector3 pos;

    Color color;

    float intensity;

    float radius;

    float radiusSq;

    float invRadius;

    float colR;

    float colG;

    float colB;

};



struct ShopLightingContext {

    ShopLightSource lights[8]; // 0: Central Bulb, 1: North Tube, 2: Counter Spotlight, 3: South Tube, 4: Freezer LEDs, 5: Entrance Tube, 6: Storage Tube, 7: Washroom Fixture

};

static ShopLightingContext g_shopLighting;



inline void SetShopLight(int idx, Vector3 pos, Color color, float intensity, float radius) {

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

inline Color ApplyShopLighting(Vector3 pos, Color baseAlbedo, Vector3 normal = (Vector3){ 0.0f, 1.0f, 0.0f }, int occludeAisle = 0) {

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

            // Rich indirect bounce ambient when store lights are active: fills entire store with warm, vibrant radiance
            rAcc += 0.28f * 1.02f;
            gAcc += 0.28f * 1.00f;
            bAcc += 0.28f * 0.94f;

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



    return (Color){

        (unsigned char)Clamp(outR, 0, 255),

        (unsigned char)Clamp(outG, 0, 255),

        (unsigned char)Clamp(outB, 0, 255),

        baseAlbedo.a

    };

}



inline float GetShopLightFactorAt(Vector3 pos) {

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



static void DrawPopcornTin(Vector3 pos, bool opened, float wobble, bool held, float distSq = 0.0f)

{

    float r = 0.13f, h = 0.22f;

    Color tinRed = ApplyShopLighting(pos, (Color){ 210, 40, 40, 255 });

    // Body cylinder

    DrawCylinder(pos, r, r, h, 14, tinRed);



    // High-detail stripes and wireframe outlines only when up close (< 3.5m) or held

    if (distSq < 12.0f || held) {

        DrawCylinderWires(pos, r, r, h, 14, ApplyShopLighting(pos, Fade(BLACK, 0.45f)));

        for (int i = 0; i < 8; i++) {

            float a0 = (float)i / 8.0f * 2.0f * PI;

            float a1 = (float)(i + 1) / 8.0f * 2.0f * PI;

            Color stripe = (i % 2 == 0) ? WHITE : (Color){20, 40, 160, 255};

            Vector3 p0 = { pos.x + cosf(a0) * (r + 0.002f), pos.y + h * 0.55f, pos.z + sinf(a0) * (r + 0.002f) };

            Vector3 p1 = { pos.x + cosf(a1) * (r + 0.002f), pos.y + h * 0.55f, pos.z + sinf(a1) * (r + 0.002f) };

            DrawCylinderEx(p0, p1, 0.012f, 0.012f, 4, ApplyShopLighting(p0, stripe));

        }

    }



    // Lid

    Vector3 lidBase = { pos.x, pos.y + h + (opened ? (0.05f + wobble) : 0.0f), pos.z };

    Color lidCol = ApplyShopLighting(lidBase, (Color){ 230, 230, 230, 255 });

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

            DrawSphere(popPos, 0.026f, (i % 2 == 0) ? (Color){ 255, 248, 205, 255 } : (Color){ 245, 215, 110, 255 });

        }

    }

}



static void DrawBottle(Vector3 pos, float bottleH, float bottleR,

                       Model &liquidModel, float fill, Color glassColor,

                       bool opened, bool held, Vector3 fwdDir, float distSq = 0.0f)

{

    rlPushMatrix();

    rlTranslatef(pos.x, pos.y, pos.z);



    // If held and pouring, tilt the bottle forward to pour!

    if (held && opened) {

        Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwdDir, (Vector3){0, 1, 0}));

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

            DrawModel(liquidModel, (Vector3){ 0, 0, 0 }, 1.0f, liquidTint);

        rlPopMatrix();

    }



    rlPopMatrix();

}



static void DrawBread(Vector3 pos, Model &crustModel, bool held, float distSq = 0.0f)

{

    Color litTint = ApplyShopLighting(pos, WHITE);

    // Main oblong loaf body

    rlPushMatrix();

        rlTranslatef(pos.x, pos.y + 0.09f, pos.z);

        rlScalef(1.7f, 0.75f, 1.0f);

        DrawModel(crustModel, (Vector3){ 0, 0, 0 }, 1.0f, litTint);

    rlPopMatrix();



    // Baked domed top hump only when close (< 3.5m) or held

    if (distSq < 12.0f || held) {

        rlPushMatrix();

            rlTranslatef(pos.x, pos.y + 0.155f, pos.z);

            rlScalef(1.25f, 0.55f, 0.8f);

            DrawModel(crustModel, (Vector3){ 0, 0, 0 }, 1.0f, ApplyShopLighting(pos, Fade(WHITE, 0.95f)));

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



static void DrawShoppingCartLocal(float wheelSpinDeg, Vector3 worldPos, float distSq = 0.0f)

{

    Color steel       = ApplyShopLighting(worldPos, (Color){ 205, 210, 215, 255 });

    Color steelDark   = ApplyShopLighting(worldPos, (Color){ 140, 145, 150, 255 });

    Color rubber      = ApplyShopLighting(worldPos, (Color){ 35, 35, 38, 255 });

    Color seatPlastic = ApplyShopLighting(worldPos, (Color){ 210, 60, 50, 255 });



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

            DrawCylinderEx((Vector3){0,0,0}, (Vector3){0, radius*0.9f, 0}, 0.004f, 0.004f, 6, spokeCol);

        rlPopMatrix();

    };

    drawWheel(rearAxleL,  rearR,  wheelSpinDeg);

    drawWheel(rearAxleR,  rearR,  wheelSpinDeg);

    drawWheel(frontAxleL, frontR, wheelSpinDeg);

    drawWheel(frontAxleR, frontR, wheelSpinDeg);



    // little plastic corner bumpers at the bottom corners

    Color bumperCol = ApplyShopLighting(worldPos, (Color){60,60,65,255});

    for (int i = 0; i < 4; i++)

        DrawSphere(bottomCorners[i], 0.018f, bumperCol);

}



// Specialized Ethereal Renderer for Nocturnal Ghost Shopping Cart

static void DrawGhostShoppingCartLocal(float wheelSpinDeg, Vector3 worldPos, float alpha, float timeVal)

{

    float pulse = 0.82f + 0.18f * sinf(timeVal * 4.5f);

    Color ghostSteel     = (Color){  80, 240, 215, (unsigned char)(195 * alpha * pulse) };

    Color ghostSteelDark = (Color){  45, 175, 160, (unsigned char)(210 * alpha) };

    Color ghostGlow      = (Color){ 130, 255, 235, (unsigned char)(245 * alpha * pulse) };

    Color ghostWheel     = (Color){  90, 255, 230, (unsigned char)(255 * alpha) };



    float baseW = 0.42f, baseL = 0.62f;

    float topW  = 0.56f, topL  = 0.70f;

    float basketBottomY = 0.55f;

    float basketTopY    = 1.00f;



    Vector3 bottomCorners[4] = {

        (Vector3){ -baseW/2, basketBottomY, -baseL/2 },

        (Vector3){  baseW/2, basketBottomY, -baseL/2 },

        (Vector3){  baseW/2, basketBottomY,  baseL/2 },

        (Vector3){ -baseW/2, basketBottomY,  baseL/2 }

    };

    Vector3 topCorners[4] = {

        (Vector3){ -topW/2, basketTopY, -topL/2 },

        (Vector3){  topW/2, basketTopY, -topL/2 },

        (Vector3){  topW/2, basketTopY,  topL/2 },

        (Vector3){ -topW/2, basketTopY,  topL/2 }

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

        DrawCubeWires((Vector3){ 0.0f, y, 0.0f }, w, 0.01f, l, ghostSteel);

    }

    for (int k = -2; k <= 2; k++) {

        float x = (float)k * 0.09f;

        DrawLine3D((Vector3){ x, basketBottomY, -baseL/2 }, (Vector3){ x, basketTopY, -topL/2 }, ghostSteelDark);

        DrawLine3D((Vector3){ x, basketBottomY,  baseL/2 }, (Vector3){ x, basketTopY,  topL/2 }, ghostSteelDark);

    }



    // Handle bar

    Vector3 handleLeft  = (Vector3){ -topW/2 * 1.06f, 1.05f, -topL/2 - 0.16f };

    Vector3 handleRight = (Vector3){  topW/2 * 1.06f, 1.05f, -topL/2 - 0.16f };

    DrawCylinderEx(handleLeft, handleRight, 0.024f, 0.024f, 8, ghostGlow);

    DrawLine3D(topCorners[0], handleLeft, ghostSteel);

    DrawLine3D(topCorners[1], handleRight, ghostSteel);



    // Lower chassis tubular struts

    DrawCubeWires((Vector3){ 0.0f, 0.16f, 0.0f }, 0.44f, 0.12f, 0.66f, ghostSteelDark);



    // 4 Glowing Caster Wheels

    Vector3 wheelOffsets[4] = {

        (Vector3){ -0.21f, 0.08f, -0.28f },

        (Vector3){  0.21f, 0.08f, -0.28f },

        (Vector3){ -0.21f, 0.08f,  0.28f },

        (Vector3){  0.21f, 0.08f,  0.28f }

    };

    for (int i = 0; i < 4; i++) {

        DrawSphere(wheelOffsets[i], 0.052f, ghostWheel);

        DrawSphereWires(wheelOffsets[i], 0.054f, 6, 6, ghostGlow);

    }

}



#include "systems/receipt_printer.h"



static void UpdateGhostCart(float dt, float nightFactor) {

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

        { (Vector3){  97.0f, 10.02f, 142.5f }, 1.2f, 0 }, // WP 0: Materializes right in central walkway in plain sight!

        { (Vector3){  91.5f, 10.02f, 142.5f }, 2.0f, 1 }, // WP 1: Rolls down aisle to shelf, pauses & takes phantom item

        { (Vector3){  97.0f, 10.02f, 137.0f }, 1.5f, 2 }, // WP 2: Turns and rolls toward checkout walkway

        { (Vector3){ 104.5f, 10.02f, 135.0f }, 3.5f, 2 }, // WP 3: Pulls right up to counter & printer in front of player

        { (Vector3){ 104.5f, 10.02f, 135.0f }, 2.0f, 2 }  // WP 4: Dissolves into floor with printer ringing up

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



static void UpdateShopParticle(ShopParticle &p, float dt, float floorY)

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

            p.vel = (Vector3){ 0, 0, 0 };

            p.landed = true;

            p.settleTimer = 1.8f; // Settled particles smoothly fade out after 1.8s

        }

    }

}



static void DrawShopParticle(const ShopParticle &p)

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



// ==================================================================================================

// SKELETON_COW_ANIMATED -- Procedural Horror Bovine Skeleton NPC System

// Features full anatomical osteology with an advanced kinematic FSM and autonomous AI

// ==================================================================================================

typedef enum {

    BOVINE_IDLE = 0,

    BOVINE_WALK,

    BOVINE_RUN,

    BOVINE_EAT,

    BOVINE_SIT,

    BOVINE_LIE,

    BOVINE_SHAKE

} BovineState;



struct BovineSkeleton {

    Vector3 pos;

    float yaw;

    float animTime;

    

    BovineState state;

    float walkPhase, runPhase, eatPhase, shakePhase;

    float walkBlend, runBlend, eatBlend, sitBlend, lieBlend, shakeBlend;



    Vector3 spineNodes[24]; 

    Vector3 tailNodes[8];

    Vector3 shoulderPos[2];

    Vector3 hipPos[2];



    // Autonomous AI state

    float stateTimer;

    float targetYaw;

    Vector3 homePos;

    float wanderRadius;

    bool isSpooked;

    float spookTimer;

    float rainShakeCooldown;

};



#define MAX_BOVINE_NPCS 4

static BovineSkeleton g_bovineNPCs[MAX_BOVINE_NPCS];



static Mesh g_bovineCyl = { 0 };

static Mesh g_bovineSphere = { 0 };

static Mesh g_bovineCube = { 0 };

static Mesh g_bovineCone = { 0 };

static Material g_bovineMat = { 0 };

static bool g_bovineMeshesLoaded = false;



static Vector3 BovineRotateAroundAxis(Vector3 v, Vector3 axis, float angleRad) {

    axis = Vector3Normalize(axis);

    float c = cosf(angleRad), s = sinf(angleRad);

    Vector3 t1 = Vector3Scale(v, c);

    Vector3 t2 = Vector3Scale(Vector3CrossProduct(axis, v), s);

    Vector3 t3 = Vector3Scale(axis, Vector3DotProduct(axis, v) * (1.0f - c));

    return Vector3Add(Vector3Add(t1, t2), t3);

}



static void BovineDrawBoneSegment(Mesh mesh, Material *mat, Vector3 from, Vector3 to, float radiusX, float radiusZ, Color tint) {

    Vector3 diff = Vector3Subtract(to, from);

    float len = Vector3Length(diff);

    if (len < 0.0001f) return;

    Vector3 dir = Vector3Scale(diff, 1.0f / len);

    Quaternion q = QuaternionFromVector3ToVector3((Vector3){ 0, 1, 0 }, dir);

    Matrix rot = QuaternionToMatrix(q);

    Matrix scale = MatrixScale(radiusX, len, radiusZ);

    Matrix translate = MatrixTranslate(from.x, from.y, from.z);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, MatrixMultiply(MatrixMultiply(scale, rot), translate));

}



static void BovineDrawForm(Mesh mesh, Material *mat, Vector3 center, Vector3 radii, Vector3 eulerDeg, Color tint) {

    Matrix s = MatrixScale(radii.x, radii.y, radii.z);

    Matrix r = MatrixRotateXYZ((Vector3){ DEG2RAD * eulerDeg.x, DEG2RAD * eulerDeg.y, DEG2RAD * eulerDeg.z });

    Matrix t = MatrixTranslate(center.x, center.y, center.z);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, MatrixMultiply(MatrixMultiply(s, r), t));

}



static void InitBovine(BovineSkeleton &cow, Vector3 startPos, float startYaw) {

    cow = (BovineSkeleton){ 0 };

    cow.pos = startPos;

    cow.yaw = startYaw;

    cow.targetYaw = startYaw;

    cow.state = BOVINE_IDLE;

    cow.homePos = startPos;

    cow.wanderRadius = 16.0f;

    cow.stateTimer = Frand(3.0f, 7.0f);

    cow.rainShakeCooldown = Frand(10.0f, 25.0f);

}



static void InitBovineMeshes() {

    if (!g_bovineMeshesLoaded) {

        g_bovineCyl = GenMeshCylinder(1.0f, 1.0f, 12);

        g_bovineSphere = GenMeshSphere(1.0f, 10, 10);

        g_bovineCube = GenMeshCube(1.0f, 1.0f, 1.0f);

        g_bovineCone = GenMeshCone(1.0f, 1.0f, 12);

        g_bovineMat = LoadMaterialDefault();

        g_bovineMeshesLoaded = true;

    }

}



static void UnloadBovineMeshes() {

    if (g_bovineMeshesLoaded) {

        UnloadMesh(g_bovineCyl);

        UnloadMesh(g_bovineSphere);

        UnloadMesh(g_bovineCube);

        UnloadMesh(g_bovineCone);

        g_bovineMeshesLoaded = false;

    }

}



static void InitBovineNPCs() {

    InitBovineMeshes();



    // Cow 0: East roadside verge right across from gas station (grazing, immediately visible from car!)

    InitBovine(g_bovineNPCs[0], (Vector3){ 142.0f, 10.0f, 138.0f }, 1.4f);

    g_bovineNPCs[0].homePos = (Vector3){ 143.0f, 10.0f, 140.0f };

    g_bovineNPCs[0].wanderRadius = 12.0f;

    g_bovineNPCs[0].state = BOVINE_EAT;

    g_bovineNPCs[0].eatBlend = 1.0f;

    g_bovineNPCs[0].stateTimer = Frand(6.0f, 12.0f);



    // Cow 1: North roadside meadow along highway (standing & chewing cud, easily seen looking forward)

    InitBovine(g_bovineNPCs[1], (Vector3){ 144.0f, 10.0f, 162.0f }, -0.7f);

    g_bovineNPCs[1].homePos = (Vector3){ 146.0f, 10.0f, 160.0f };

    g_bovineNPCs[1].wanderRadius = 14.0f;

    g_bovineNPCs[1].state = BOVINE_IDLE;

    g_bovineNPCs[1].stateTimer = Frand(4.0f, 8.0f);



    // Cow 2: West roadside meadow across from the shop (resting, lying on side)

    InitBovine(g_bovineNPCs[2], (Vector3){ 112.0f, 10.0f, 142.0f }, 2.4f);

    g_bovineNPCs[2].homePos = (Vector3){ 112.0f, 10.0f, 142.0f };

    g_bovineNPCs[2].wanderRadius = 12.0f;

    g_bovineNPCs[2].state = BOVINE_LIE;

    g_bovineNPCs[2].sitBlend = 1.0f;

    g_bovineNPCs[2].lieBlend = 1.0f;

    g_bovineNPCs[2].stateTimer = Frand(15.0f, 25.0f);



    // Cow 3: East meadow pasture near highway entrance (walking / grazing)

    InitBovine(g_bovineNPCs[3], (Vector3){ 146.0f, 10.0f, 122.0f }, 0.5f);

    g_bovineNPCs[3].homePos = (Vector3){ 148.0f, 10.0f, 124.0f };

    g_bovineNPCs[3].wanderRadius = 15.0f;

    g_bovineNPCs[3].state = BOVINE_WALK;

    g_bovineNPCs[3].walkBlend = 1.0f;

    g_bovineNPCs[3].targetYaw = 0.5f;

    g_bovineNPCs[3].stateTimer = Frand(5.0f, 9.0f);

}



static void UpdateBovineKinematics(BovineSkeleton &cow, float dt) {

    cow.animTime += dt;



    // FSM State Blending Targets

    float tWalk  = (cow.state == BOVINE_WALK) ? 1.0f : 0.0f;

    float tRun   = (cow.state == BOVINE_RUN) ? 1.0f : 0.0f;

    float tEat   = (cow.state == BOVINE_EAT) ? 1.0f : 0.0f;

    float tSit   = (cow.state == BOVINE_SIT || cow.state == BOVINE_LIE) ? 1.0f : 0.0f; // Lie passes through Sit

    float tLie   = (cow.state == BOVINE_LIE) ? 1.0f : 0.0f;

    float tShake = (cow.state == BOVINE_SHAKE) ? 1.0f : 0.0f;



    // Apply linear lerp interpolation for natural, smooth procedural transitions

    float blendSpd = dt * 4.0f;

    cow.walkBlend  = Lerp(cow.walkBlend,  tWalk,  blendSpd);

    cow.runBlend   = Lerp(cow.runBlend,   tRun,   blendSpd);

    cow.eatBlend   = Lerp(cow.eatBlend,   tEat,   blendSpd);

    cow.sitBlend   = Lerp(cow.sitBlend,   tSit,   blendSpd);

    cow.lieBlend   = Lerp(cow.lieBlend,   tLie,   blendSpd);

    cow.shakeBlend = Lerp(cow.shakeBlend, tShake, blendSpd * 1.5f);



    // Root Motion Update

    float curSpeed = (0.8f * cow.walkBlend) + (2.6f * cow.runBlend);

    if (curSpeed > 0.01f) {

        cow.walkPhase += dt * 4.5f * cow.walkBlend;

        cow.runPhase  += dt * 8.5f * cow.runBlend;

        cow.pos.x += sinf(cow.yaw) * curSpeed * dt;

        cow.pos.z += cosf(cow.yaw) * curSpeed * dt;

    }



    if (cow.eatBlend > 0.01f) cow.eatPhase += dt * 3.0f;

    

    // Violent Shake Oscillation 

    if (cow.shakeBlend > 0.01f) cow.shakePhase += dt * 50.0f; 



    float breathe = sinf(cow.animTime * 1.5f) * 0.015f * (1.0f - cow.lieBlend * 0.5f);

    

    // Base Verticality & Drops

    float basePelvisHeight = 1.45f;

    float locomotionBob = sinf(cow.walkPhase * 2.0f) * 0.04f * cow.walkBlend + 

                          fabsf(sinf(cow.runPhase)) * 0.12f * cow.runBlend;

    

    // Deep sit/lie offset lowers the root body to the floor

    float heightOffset = -(cow.sitBlend * 0.85f) - (cow.lieBlend * 0.10f); 

    

    Vector3 root = { cow.pos.x, cow.pos.y + basePelvisHeight + locomotionBob + heightOffset, cow.pos.z };

    

    // Body Roll applied when lying down (rotates the entire coordinate frame!)

    Vector3 fwd = { sinf(cow.yaw), 0, cosf(cow.yaw) };

    float bodyRoll = cow.lieBlend * 1.35f; // Rolls over ~77 degrees onto the side

    Vector3 right = BovineRotateAroundAxis((Vector3){ cosf(cow.yaw), 0, -sinf(cow.yaw) }, fwd, bodyRoll);

    Vector3 up = BovineRotateAroundAxis((Vector3){ 0, 1, 0 }, fwd, bodyRoll);



    // Procedural Spine Generation (Index 0 = Tail Base -> Index 23 = Skull Joint)

    for (int i = 0; i < 24; i++) {

        float t = (float)i / 23.0f; 

        float localZ = -1.2f + (t * 2.3f);

        float localY = 0.0f;



        // Base Curvature

        if (t < 0.25f) localY = 0.0f; 

        else if (t < 0.65f) localY = -sinf((t - 0.25f) / 0.40f * PI) * 0.12f; 

        else { 

            float nt = (t - 0.65f) / 0.35f;

            localY = -0.35f + cosf(nt * PI) * 0.35f; 

        }



        // Eating Modifier: Limits drop to exactly ground level without burying the skull

        if (t > 0.65f) {

            float neckT = (t - 0.65f) / 0.35f;

            localY -= neckT * 0.70f * cow.eatBlend; 

        }

        

        // Shaking Modifier: Rapid lateral centrifugal twist to cast water off

        float shakeT = sinf(cow.shakePhase - t * 3.0f) * 0.35f * cow.shakeBlend;

        float shakeX = shakeT * sinf(t * PI); 



        localY += breathe * sinf(t * PI);

        

        Vector3 node = Vector3Add(root, Vector3Scale(fwd, localZ));

        node = Vector3Add(node, Vector3Scale(right, shakeX));

        node = Vector3Add(node, Vector3Scale(up, localY));

        cow.spineNodes[i] = node;

    }



    // Tail Kinematics

    for (int i = 0; i < 8; i++) {

        float drop = (float)i * 0.12f;

        float swRate = (cow.shakeBlend > 0.0f) ? 40.0f : 1.5f;

        float sway = sinf(cow.animTime * swRate - drop * 2.0f) * (0.08f + cow.shakeBlend * 0.4f);

        Vector3 node = Vector3Add(cow.spineNodes[0], Vector3Scale(fwd, -drop * 0.3f));

        node = Vector3Add(node, Vector3Scale(up, -drop * 0.8f * (1.0f - cow.lieBlend)));

        node = Vector3Add(node, Vector3Scale(right, sway));

        cow.tailNodes[i] = node;

    }



    cow.shoulderPos[0] = Vector3Add(cow.spineNodes[17], Vector3Scale(right, -0.28f));

    cow.shoulderPos[1] = Vector3Add(cow.spineNodes[17], Vector3Scale(right,  0.28f));

    cow.shoulderPos[0] = Vector3Add(cow.shoulderPos[0], Vector3Scale(up, -0.15f));

    cow.shoulderPos[1] = Vector3Add(cow.shoulderPos[1], Vector3Scale(up, -0.15f));



    cow.hipPos[0] = Vector3Add(cow.spineNodes[2], Vector3Scale(right, -0.26f));

    cow.hipPos[1] = Vector3Add(cow.spineNodes[2], Vector3Scale(right,  0.26f));

}



static void UpdateBovineAI(BovineSkeleton &cow, Vector3 playerPos, float dt, bool isRaining) {

    cow.pos.y = 10.0f;



    // Smooth heading rotation towards targetYaw

    float diffYaw = fmodf(cow.targetYaw - cow.yaw + PI, 2.0f * PI);

    if (diffYaw < 0.0f) diffYaw += 2.0f * PI;

    diffYaw -= PI;

    cow.yaw += diffYaw * dt * 2.5f;



    // 1. Player Proximity & Spook Mechanics

    float pDx = cow.pos.x - playerPos.x;

    float pDz = cow.pos.z - playerPos.z;

    float distToPlayer = sqrtf(pDx * pDx + pDz * pDz);



    if (distToPlayer < 3.8f) {

        cow.isSpooked = true;

        cow.spookTimer = 5.0f;

        // Turn directly away from player

        cow.targetYaw = atan2f(pDx, pDz);

        if (distToPlayer < 2.2f) {

            cow.state = BOVINE_RUN;

        } else if (cow.state != BOVINE_RUN) {

            cow.state = BOVINE_WALK;

        }

    }



    if (cow.isSpooked) {

        cow.spookTimer -= dt;

        if (cow.spookTimer <= 0.0f) {

            cow.isSpooked = false;

            cow.state = BOVINE_IDLE;

            cow.stateTimer = Frand(3.0f, 6.0f);

        }

        // Steer away from highway edge (X: 115..141)

        if (cow.homePos.x > 130.0f && cow.pos.x < 144.0f) {

            cow.targetYaw = atan2f(1.0f, 0.0f); // Head East

        } else if (cow.homePos.x < 120.0f && cow.pos.x > 108.0f) {

            cow.targetYaw = atan2f(-1.0f, 0.0f); // Head West

        }

        return;

    }



    // 2. Rain Shake Reaction

    if (isRaining && cow.state != BOVINE_SIT && cow.state != BOVINE_LIE) {

        cow.rainShakeCooldown -= dt;

        if (cow.rainShakeCooldown <= 0.0f) {

            cow.state = BOVINE_SHAKE;

            cow.stateTimer = 1.35f;

            cow.rainShakeCooldown = Frand(15.0f, 30.0f);

        }

    }



    // 3. Autonomous Pasture Routine

    cow.stateTimer -= dt;

    if (cow.stateTimer <= 0.0f) {

        // Prevent wandering too far from home pasture

        float hDx = cow.pos.x - cow.homePos.x;

        float hDz = cow.pos.z - cow.homePos.z;

        float distFromHome = sqrtf(hDx * hDx + hDz * hDz);



        int roll = GetRandomValue(0, 100);

        if (distFromHome > cow.wanderRadius) {

            // Wander back home

            cow.state = BOVINE_WALK;

            cow.targetYaw = atan2f(-hDx, -hDz);

            cow.stateTimer = Frand(5.0f, 9.0f);

        } else if (roll < 42) {

            // Grazing grass

            cow.state = BOVINE_EAT;

            cow.stateTimer = Frand(8.0f, 16.0f);

        } else if (roll < 68) {

            // Idle chewing cud & looking around

            cow.state = BOVINE_IDLE;

            cow.stateTimer = Frand(4.0f, 8.0f);

        } else if (roll < 88) {

            // Gentle wander to new pasture patch

            cow.state = BOVINE_WALK;

            cow.targetYaw = Frand(-PI, PI);

            cow.stateTimer = Frand(5.0f, 10.0f);

        } else {

            // Resting on the ground (sitting or lying down)

            cow.state = (GetRandomValue(0, 1) == 0) ? BOVINE_SIT : BOVINE_LIE;

            cow.stateTimer = Frand(12.0f, 22.0f);

        }

    }

}



static void DrawBovineSkeleton(const BovineSkeleton &cow, Mesh cyl, Mesh sphere, Mesh cube, Mesh cone,

                               Material *mat, Color boneCol, Color hornCol, Color voidCol)

{

    Vector3 fwd = { sinf(cow.yaw), 0, cosf(cow.yaw) };

    float bodyRoll = cow.lieBlend * 1.35f;

    Vector3 right = BovineRotateAroundAxis((Vector3){ cosf(cow.yaw), 0, -sinf(cow.yaw) }, fwd, bodyRoll);

    Vector3 up = BovineRotateAroundAxis((Vector3){ 0, 1, 0 }, fwd, bodyRoll);



    // 1. SPINAL COLUMN

    for (int i = 0; i < 23; i++) {

        BovineDrawBoneSegment(cyl, mat, cow.spineNodes[i], cow.spineNodes[i+1], 0.035f, 0.040f, boneCol);

        if (i >= 8 && i <= 17) {

            float humpHeight = sinf(((float)(i - 8) / 9.0f) * PI) * 0.35f;

            Vector3 processTop = Vector3Add(cow.spineNodes[i], Vector3Scale(up, humpHeight));

            BovineDrawBoneSegment(cyl, mat, cow.spineNodes[i], processTop, 0.015f, 0.025f, boneCol);

        } else if (i >= 2 && i < 8) {

            Vector3 procL = Vector3Add(cow.spineNodes[i], Vector3Scale(right, -0.15f));

            Vector3 procR = Vector3Add(cow.spineNodes[i], Vector3Scale(right,  0.15f));

            BovineDrawBoneSegment(cyl, mat, cow.spineNodes[i], procL, 0.015f, 0.01f, boneCol);

            BovineDrawBoneSegment(cyl, mat, cow.spineNodes[i], procR, 0.015f, 0.01f, boneCol);

        }

    }

    for (int i = 0; i < 7; i++) {

        BovineDrawBoneSegment(cyl, mat, cow.tailNodes[i], cow.tailNodes[i+1], 0.025f - (i*0.003f), 0.025f - (i*0.003f), boneCol);

    }



    // 2. RIBCAGE & STERNUM

    for (int r = 0; r < 13; r++) {

        int spineIdx = 5 + r; 

        Vector3 rootNode = cow.spineNodes[spineIdx];

        float width = 0.42f - fabsf((float)r - 6.0f) * 0.018f; 

        float drop = 0.75f - fabsf((float)r - 6.0f) * 0.015f;

        float backwardSweep = (float)r * 0.025f;



        for (int side = -1; side <= 1; side += 2) {

            Vector3 ribMid = Vector3Add(rootNode, Vector3Scale(right, side * width));

            ribMid = Vector3Add(ribMid, Vector3Scale(up, -drop * 0.3f));

            ribMid = Vector3Add(ribMid, Vector3Scale(fwd, -backwardSweep * 0.5f));

            

            Vector3 ribBot = Vector3Add(rootNode, Vector3Scale(right, side * width * 0.3f));

            ribBot = Vector3Add(ribBot, Vector3Scale(up, -drop));

            ribBot = Vector3Add(ribBot, Vector3Scale(fwd, -backwardSweep));



            BovineDrawBoneSegment(cyl, mat, rootNode, ribMid, 0.030f, 0.010f, boneCol);

            BovineDrawBoneSegment(cyl, mat, ribMid, ribBot, 0.025f, 0.008f, boneCol);

        }

    }

    Vector3 sternumFront = Vector3Add(cow.spineNodes[17], Vector3Scale(up, -0.75f));

    Vector3 sternumBack  = Vector3Add(cow.spineNodes[5],  Vector3Scale(up, -0.75f));

    BovineDrawBoneSegment(cyl, mat, sternumBack, sternumFront, 0.04f, 0.08f, boneCol);



    // 3. PELVIS

    Vector3 sacrum = cow.spineNodes[2];

    BovineDrawBoneSegment(cyl, mat, sacrum, cow.hipPos[0], 0.05f, 0.02f, boneCol);

    BovineDrawBoneSegment(cyl, mat, sacrum, cow.hipPos[1], 0.05f, 0.02f, boneCol);

    Vector3 pinL = Vector3Add(sacrum, Vector3Add(Vector3Scale(fwd, -0.35f), Vector3Scale(right, -0.1f)));

    Vector3 pinR = Vector3Add(sacrum, Vector3Add(Vector3Scale(fwd, -0.35f), Vector3Scale(right,  0.1f)));

    BovineDrawBoneSegment(cyl, mat, cow.hipPos[0], pinL, 0.03f, 0.03f, boneCol);

    BovineDrawBoneSegment(cyl, mat, cow.hipPos[1], pinR, 0.03f, 0.03f, boneCol);

    BovineDrawBoneSegment(cyl, mat, sacrum, pinL, 0.04f, 0.02f, boneCol);

    BovineDrawBoneSegment(cyl, mat, sacrum, pinR, 0.04f, 0.02f, boneCol);



    // 4. SKULL & MANDIBLE

    Vector3 atlas = cow.spineNodes[23];

    Vector3 headDir = Vector3Normalize(Vector3Add(Vector3Scale(fwd, 1.0f), Vector3Scale(up, 0.3f - cow.eatBlend * 1.5f)));

    Vector3 snoutEnd = Vector3Add(atlas, Vector3Scale(headDir, 0.55f));

    

    BovineDrawForm(sphere, mat, Vector3Add(atlas, Vector3Scale(headDir, 0.15f)), (Vector3){0.18f, 0.12f, 0.22f}, (Vector3){15, cow.yaw*RAD2DEG, 0}, boneCol);

    BovineDrawBoneSegment(cyl, mat, Vector3Add(atlas, Vector3Scale(headDir, 0.15f)), snoutEnd, 0.08f, 0.10f, boneCol);

    BovineDrawForm(sphere, mat, snoutEnd, (Vector3){0.12f, 0.06f, 0.08f}, (Vector3){15, cow.yaw*RAD2DEG, 0}, boneCol);



    float jawDrop = cow.eatBlend * fabsf(sinf(cow.eatPhase)) * 0.10f;

    Vector3 jawPivot = Vector3Add(atlas, Vector3Scale(up, -0.12f));

    Vector3 jawTip = Vector3Add(snoutEnd, Vector3Add(Vector3Scale(up, -0.06f - jawDrop), Vector3Scale(fwd, -0.05f)));

    BovineDrawBoneSegment(cyl, mat, jawPivot, jawTip, 0.08f, 0.06f, boneCol);



    Vector3 eyeL = Vector3Add(atlas, Vector3Add(Vector3Scale(headDir, 0.25f), Vector3Scale(right, -0.16f)));

    Vector3 eyeR = Vector3Add(atlas, Vector3Add(Vector3Scale(headDir, 0.25f), Vector3Scale(right,  0.16f)));

    BovineDrawForm(sphere, mat, eyeL, (Vector3){0.05f, 0.06f, 0.05f}, (Vector3){0,0,0}, voidCol);

    BovineDrawForm(sphere, mat, eyeR, (Vector3){0.05f, 0.06f, 0.05f}, (Vector3){0,0,0}, voidCol);



    Vector3 hornL = Vector3Add(atlas, Vector3Add(Vector3Scale(fwd, -0.05f), Vector3Scale(right, -0.15f))); hornL = Vector3Add(hornL, Vector3Scale(up, 0.12f));

    Vector3 hornR = Vector3Add(atlas, Vector3Add(Vector3Scale(fwd, -0.05f), Vector3Scale(right,  0.15f))); hornR = Vector3Add(hornR, Vector3Scale(up, 0.12f));

    Vector3 hornTipL = Vector3Add(hornL, Vector3Add(Vector3Scale(fwd, 0.15f), Vector3Add(Vector3Scale(right, -0.25f), Vector3Scale(up, 0.35f))));

    Vector3 hornTipR = Vector3Add(hornR, Vector3Add(Vector3Scale(fwd, 0.15f), Vector3Add(Vector3Scale(right,  0.25f), Vector3Scale(up, 0.35f))));

    BovineDrawBoneSegment(cone, mat, hornL, hornTipL, 0.045f, 0.045f, hornCol);

    BovineDrawBoneSegment(cone, mat, hornR, hornTipR, 0.045f, 0.045f, hornCol);



    // 5. FORELIMBS (Anatomical Sit & Lie Folding)

    for (int side = -1; side <= 1; side += 2) {

        float off = (side == -1) ? 0.0f : PI;

        float swing = sinf(cow.walkPhase + off) * 0.35f * cow.walkBlend + sinf(cow.runPhase + off) * 0.70f * cow.runBlend;

        float carpalFlex = fmaxf(0.0f, sinf(cow.walkPhase + off)) * 0.5f * cow.walkBlend + fmaxf(0.0f, sinf(cow.runPhase + off)) * 0.8f * cow.runBlend;



        // Front Limbs Absolute Sit Folding Angles (Tuck backwards parallel to ground)

        float foldFrontHumerus = cow.sitBlend * -0.6f + cow.lieBlend * -0.6f;

        float foldFrontRadius  = cow.sitBlend * -1.0f + cow.lieBlend * -1.0f;

        float foldFrontCannon  = cow.sitBlend *  1.5f + cow.lieBlend *  1.5f; // Flat forward on floor



        Vector3 shoulderJoint = cow.shoulderPos[(side==-1)?0:1]; 

        Vector3 scapulaTop = Vector3Add(shoulderJoint, Vector3Add(Vector3Scale(up, 0.45f), Vector3Scale(fwd, -0.2f)));

        BovineDrawBoneSegment(cyl, mat, shoulderJoint, scapulaTop, 0.12f, 0.02f, boneCol);



        Vector3 elbowDir   = BovineRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.4f))), right, swing + foldFrontHumerus);

        Vector3 carpalDir  = BovineRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd,  0.2f))), right, swing - carpalFlex + foldFrontRadius); 

        Vector3 fetlockDir = BovineRotateAroundAxis((Vector3){0, -1, 0}, right, swing - carpalFlex + foldFrontCannon);



        Vector3 elbow   = Vector3Add(shoulderJoint, Vector3Scale(elbowDir, 0.35f));

        Vector3 carpal  = Vector3Add(elbow, Vector3Scale(carpalDir, 0.35f)); 

        Vector3 fetlock = Vector3Add(carpal, Vector3Scale(fetlockDir, 0.25f));

        

        Vector3 hoof = Vector3Add(fetlock, Vector3Add(Vector3Scale(up, -0.1f), Vector3Scale(fwd, 0.05f)));

        hoof.y = fmaxf(cow.pos.y + 0.02f, hoof.y);



        BovineDrawBoneSegment(cyl, mat, shoulderJoint, elbow, 0.05f, 0.05f, boneCol);

        BovineDrawForm(sphere, mat, elbow, (Vector3){0.055f, 0.055f, 0.055f}, (Vector3){0,0,0}, boneCol);

        BovineDrawBoneSegment(cyl, mat, elbow, carpal, 0.04f, 0.035f, boneCol);

        BovineDrawForm(sphere, mat, carpal, (Vector3){0.045f, 0.045f, 0.045f}, (Vector3){0,0,0}, boneCol);

        BovineDrawBoneSegment(cyl, mat, carpal, fetlock, 0.03f, 0.025f, boneCol);

        BovineDrawForm(cube, mat, Vector3Add(hoof, Vector3Scale(right, -0.02f)), (Vector3){0.035f, 0.06f, 0.08f}, (Vector3){0, cow.yaw*RAD2DEG, 0}, hornCol);

        BovineDrawForm(cube, mat, Vector3Add(hoof, Vector3Scale(right,  0.02f)), (Vector3){0.035f, 0.06f, 0.08f}, (Vector3){0, cow.yaw*RAD2DEG, 0}, hornCol);

    }



    // 6. HINDLIMBS

    for (int side = -1; side <= 1; side += 2) {

        float off = (side == -1) ? PI : 0.0f; 

        float swing = sinf(cow.walkPhase + off) * 0.40f * cow.walkBlend + sinf(cow.runPhase + off) * 0.80f * cow.runBlend;

        float hockFlex = fmaxf(0.0f, -sinf(cow.walkPhase + off)) * 0.6f * cow.walkBlend + fmaxf(0.0f, -sinf(cow.runPhase + off)) * 1.0f * cow.runBlend;



        // Hind Limbs Absolute Sit Folding Angles (Tuck forward parallel to ground)

        float foldHindFemur  = cow.sitBlend * 0.6f + cow.lieBlend * 0.6f;

        float foldHindTibia  = cow.sitBlend * 1.0f + cow.lieBlend * 1.0f;

        float foldHindCannon = cow.sitBlend * 1.5f + cow.lieBlend * 1.5f;



        Vector3 hipJoint = cow.hipPos[(side==-1)?0:1];



        Vector3 stifleDir  = BovineRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd,  0.4f))), right, swing + foldHindFemur);

        Vector3 hockDir    = BovineRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.3f))), right, swing + hockFlex + foldHindTibia);

        Vector3 fetlockDir = BovineRotateAroundAxis((Vector3){0, -1, 0}, right, swing + hockFlex + foldHindCannon);



        Vector3 stifle  = Vector3Add(hipJoint, Vector3Scale(stifleDir, 0.4f)); 

        Vector3 hock    = Vector3Add(stifle, Vector3Scale(hockDir, 0.45f));

        Vector3 fetlock = Vector3Add(hock, Vector3Scale(fetlockDir, 0.30f));

        

        Vector3 hoof = Vector3Add(fetlock, Vector3Add(Vector3Scale(up, -0.1f), Vector3Scale(fwd, 0.05f)));

        hoof.y = fmaxf(cow.pos.y + 0.02f, hoof.y);



        BovineDrawBoneSegment(cyl, mat, hipJoint, stifle, 0.06f, 0.06f, boneCol);

        BovineDrawForm(sphere, mat, stifle, (Vector3){0.055f, 0.055f, 0.055f}, (Vector3){0,0,0}, boneCol);

        BovineDrawBoneSegment(cyl, mat, stifle, hock, 0.045f, 0.045f, boneCol);

        BovineDrawBoneSegment(cyl, mat, hock, Vector3Add(hock, Vector3Add(Vector3Scale(up, 0.1f), Vector3Scale(fwd, -0.1f))), 0.02f, 0.02f, boneCol);

        BovineDrawForm(sphere, mat, hock, (Vector3){0.045f, 0.055f, 0.045f}, (Vector3){0,0,0}, boneCol);

        BovineDrawBoneSegment(cyl, mat, hock, fetlock, 0.03f, 0.025f, boneCol);

        

        BovineDrawForm(cube, mat, Vector3Add(hoof, Vector3Scale(right, -0.02f)), (Vector3){0.035f, 0.06f, 0.08f}, (Vector3){0, cow.yaw*RAD2DEG, 0}, hornCol);

        BovineDrawForm(cube, mat, Vector3Add(hoof, Vector3Scale(right,  0.02f)), (Vector3){0.035f, 0.06f, 0.08f}, (Vector3){0, cow.yaw*RAD2DEG, 0}, hornCol);

    }



    // 7. VIOLENT SHAKE WATER DROPLETS (Flinging physics)

    if (cow.shakeBlend > 0.25f) {

        for (int d = 0; d < 6; d++) {

            float fPhase = cow.shakePhase * 1.4f + (float)d * 1.15f;

            float fRadius = 0.45f + fmodf(fPhase * 0.7f, 1.5f);

            float fAngle = (float)d * (2.0f * PI / 6.0f) + sinf(fPhase) * 0.6f;

            float fDir = (sinf(cow.shakePhase) > 0.0f) ? 1.0f : -1.0f;

            Vector3 dropPos = {

                cow.pos.x + cosf(fAngle) * fRadius * fDir,

                cow.pos.y + 0.9f + sinf(fPhase * 2.0f) * 0.35f,

                cow.pos.z + sinf(fAngle) * fRadius

            };

            DrawSphere(dropPos, 0.022f, (Color){ 175, 220, 255, (unsigned char)(210 * cow.shakeBlend) });

        }

    }

}



// ==================================================================================================

// HOUND -- Procedural Horror Dog NPC System

// Features full anatomical kinematics, autonomous companion AI, and domain-warped FBM fur shader

// ==================================================================================================



static const char *VS_HOUND_SOURCE =

"#version 330\n"

"in vec3 vertexPosition;\n"

"in vec2 vertexTexCoord;\n"

"in vec3 vertexNormal;\n"

"in vec4 vertexColor;\n"

"uniform mat4 mvp;\n"

"uniform mat4 matModel;\n"

"uniform mat4 matNormal;\n"

"out vec3 fragPosition;\n"

"out vec3 fragLocalPos;\n"

"out vec2 fragTexCoord;\n"

"out vec4 fragColor;\n"

"out vec3 fragNormal;\n"

"void main()\n"

"{\n"

"    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"

"    fragLocalPos = vertexPosition;\n"

"    fragTexCoord = vertexTexCoord;\n"

"    fragColor = vertexColor;\n"

"    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"

"    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"

"}\n";



static const char *FS_HOUND_SOURCE =

"#version 330\n"

"in vec3 fragPosition;\n"

"in vec3 fragLocalPos;\n"

"in vec2 fragTexCoord;\n"

"in vec4 fragColor;\n"

"in vec3 fragNormal;\n"

"uniform sampler2D texture0;\n"

"uniform vec4 colDiffuse;\n"

"#define MAX_LIGHTS 4\n"

"struct Light { int enabled; int type; vec3 position; vec3 target; vec4 color; };\n"

"uniform Light lights[MAX_LIGHTS];\n"

"uniform vec4 ambient;\n"

"uniform vec3 viewPos;\n"

"uniform vec4 fogColor;\n"

"uniform float fogDensity;\n"

"uniform int matType;\n" // 0=Fur, 1=Skin/Leather, 2=Glowing Eyes, 3=Environment

"uniform float time;\n"

"out vec4 finalColor;\n"

"\n"

"float hash13(vec3 p) {\n"

"    p = fract(p * 0.3183099 + 0.1);\n"

"    p *= 17.0;\n"

"    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));\n"

"}\n"

"float vnoise(vec3 p) {\n"

"    vec3 i = floor(p);\n"

"    vec3 f = fract(p);\n"

"    vec3 u = f * f * (3.0 - 2.0 * f);\n"

"    return mix(mix(mix(hash13(i+vec3(0,0,0)), hash13(i+vec3(1,0,0)), u.x),\n"

"                   mix(hash13(i+vec3(0,1,0)), hash13(i+vec3(1,1,0)), u.x), u.y),\n"

"               mix(mix(hash13(i+vec3(0,0,1)), hash13(i+vec3(1,0,1)), u.x),\n"

"                   mix(hash13(i+vec3(0,1,1)), hash13(i+vec3(1,1,1)), u.x), u.y), u.z);\n"

"}\n"

"float fbm(vec3 p) {\n"

"    float v = 0.0, a = 0.5;\n"

"    for (int i = 0; i < 4; i++) { v += a * vnoise(p); p *= 2.02; a *= 0.5; }\n"

"    return v;\n"

"}\n"

"void main()\n"

"{\n"

"    vec4 tint = colDiffuse * fragColor;\n"

"    vec3 normal = normalize(fragNormal);\n"

"    vec3 viewD = normalize(viewPos - fragPosition);\n"

"    vec3 emission = vec3(0.0);\n"

"\n"

"    if (matType == 0) // FUR WITH PROCEDURAL SPOTS\n"

"    {\n"

"        vec3 lp = fragLocalPos * 14.0;\n"

"        vec3 warp = vec3(fbm(lp), fbm(lp + 13.0), fbm(lp + 27.0));\n"

"        float spotNoise = fbm(lp + warp * 1.8);\n"

"        float spotMask = smoothstep(0.40, 0.52, spotNoise);\n"

"        vec3 spotColor = vec3(0.08, 0.07, 0.07);\n"

"        tint.rgb = mix(tint.rgb, spotColor, spotMask);\n"

"\n"

"        float furNoise = fbm(fragLocalPos * 150.0);\n"

"        float clump = fbm(fragLocalPos * 30.0);\n"

"        tint.rgb *= mix(0.75, 1.15, furNoise) * mix(0.85, 1.1, clump);\n"

"        float dirAO = clamp(normal.y * 0.4 + 0.6, 0.2, 1.0);\n"

"        tint.rgb *= dirAO;\n"

"        vec3 furBump = normalize(vec3(vnoise(fragLocalPos*180.0)-0.5, vnoise(fragLocalPos*180.0+20.0)-0.5, 1.0));\n"

"        normal = normalize(normal + furBump * 0.15);\n"

"    }\n"

"    else if (matType == 1) // LEATHERY SKIN (Nose/Paws/Teeth)\n"

"    {\n"

"        float grain = fbm(fragLocalPos * 50.0);\n"

"        tint.rgb *= mix(0.85, 1.10, grain);\n"

"    }\n"

"    else if (matType == 2) // GLOWING EYES\n"

"    {\n"

"        float pulse = sin(time * 5.0) * 0.2 + 0.8;\n"

"        emission = tint.rgb * pulse * 2.5;\n"

"    }\n"

"\n"

"    vec3 lightDot = vec3(0.0);\n"

"    vec3 specular = vec3(0.0);\n"

"    float specPower = (matType == 1) ? 24.0 : 4.0;\n"

"    float specStrength = (matType == 1) ? 0.2 : 0.02;\n"

"    if (matType == 2) specStrength = 0.5;\n"

"\n"

"    for (int i = 0; i < MAX_LIGHTS; i++)\n"

"    {\n"

"        if (lights[i].enabled == 1)\n"

"        {\n"

"            vec3 lightDir = normalize(lights[i].position - fragPosition);\n"

"            float dist = length(lights[i].position - fragPosition);\n"

"            float atten = 1.0 / (1.0 + 0.05 * dist + 0.01 * dist * dist);\n"

"            float NdotL = max(dot(normal, lightDir), 0.0);\n"

"            lightDot += lights[i].color.rgb * NdotL * atten;\n"

"            if (NdotL > 0.0)\n"

"            {\n"

"                float specCo = pow(max(0.0, dot(viewD, reflect(-lightDir, normal))), specPower);\n"

"                specular += specCo * atten * specStrength * lights[i].color.rgb;\n"

"            }\n"

"        }\n"

"    }\n"

"\n"

"    vec3 lit = tint.rgb * (lightDot + ambient.rgb) + specular + emission;\n"

"    \n"

"    if (matType == 0) {\n"

"        float rim = 1.0 - max(dot(viewD, normal), 0.0);\n"

"        lit += vec3(0.4, 0.45, 0.5) * pow(rim, 4.0) * 0.25;\n"

"    }\n"

"\n"

"    float d = length(viewPos - fragPosition);\n"

"    float fogFactor = clamp(1.0 - exp(-fogDensity * d), 0.0, 1.0);\n"

"    vec3 finalRGB = mix(lit, fogColor.rgb, fogFactor);\n"

"    finalColor = vec4(pow(finalRGB, vec3(1.0/2.15)), tint.a);\n"

"}\n";



typedef struct HoundSceneLight {

    int enabledLoc, typeLoc, posLoc, targetLoc, colorLoc;

} HoundSceneLight;



static const char *VS_SKELETON_HOUND_SOURCE =

"#version 330\n"

"in vec3 vertexPosition;\n"

"in vec2 vertexTexCoord;\n"

"in vec3 vertexNormal;\n"

"in vec4 vertexColor;\n"

"uniform mat4 mvp;\n"

"uniform mat4 matModel;\n"

"uniform mat4 matNormal;\n"

"out vec3 fragPosition;\n"

"out vec2 fragTexCoord;\n"

"out vec4 fragColor;\n"

"out vec3 fragNormal;\n"

"void main()\n"

"{\n"

"    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"

"    fragTexCoord = vertexTexCoord;\n"

"    fragColor = vertexColor;\n"

"    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"

"    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"

"}\n";



static const char *FS_SKELETON_HOUND_SOURCE =

"#version 330\n"

"in vec3 fragPosition;\n"

"in vec2 fragTexCoord;\n"

"in vec4 fragColor;\n"

"in vec3 fragNormal;\n"

"uniform sampler2D texture0;\n"

"uniform vec4 colDiffuse;\n"

"#define MAX_LIGHTS 4\n"

"struct Light { int enabled; int type; vec3 position; vec3 target; vec4 color; };\n"

"uniform Light lights[MAX_LIGHTS];\n"

"uniform vec4 ambient;\n"

"uniform vec3 viewPos;\n"

"uniform vec4 fogColor;\n"

"uniform float fogDensity;\n"

"uniform int matType;\n" // 0=Bone, 1=Dark Cavity, 2=Glowing Eyes, 3=Teeth

"uniform float time;\n"

"out vec4 finalColor;\n"

"\n"

"float hash13(vec3 p) {\n"

"    p = fract(p * 0.3183099 + 0.1);\n"

"    p *= 17.0;\n"

"    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));\n"

"}\n"

"float vnoise(vec3 p) {\n"

"    vec3 i = floor(p);\n"

"    vec3 f = fract(p);\n"

"    vec3 u = f * f * (3.0 - 2.0 * f);\n"

"    return mix(mix(mix(hash13(i+vec3(0,0,0)), hash13(i+vec3(1,0,0)), u.x),\n"

"                   mix(hash13(i+vec3(0,1,0)), hash13(i+vec3(1,1,0)), u.x), u.y),\n"

"               mix(mix(hash13(i+vec3(0,0,1)), hash13(i+vec3(1,0,1)), u.x),\n"

"                   mix(hash13(i+vec3(0,1,1)), hash13(i+vec3(1,1,1)), u.x), u.y), u.z);\n"

"}\n"

"float fbm(vec3 p) {\n"

"    float v = 0.0, a = 0.5;\n"

"    for (int i = 0; i < 4; i++) { v += a * vnoise(p); p *= 2.02; a *= 0.5; }\n"

"    return v;\n"

"}\n"

"void main()\n"

"{\n"

"    vec4 tint = colDiffuse * fragColor;\n"

"    vec3 normal = normalize(fragNormal);\n"

"    vec3 viewD = normalize(viewPos - fragPosition);\n"

"    vec3 emission = vec3(0.0);\n"

"\n"

"    if (matType == 0 || matType == 3) // BONE OR TEETH\n"

"    {\n"

"        float grain = fbm(fragPosition * 30.0);\n"

"        tint.rgb *= mix(0.85, 1.10, grain);\n"

"        float pits = vnoise(fragPosition * 250.0);\n"

"        tint.rgb *= mix(0.88, 1.0, pits);\n"

"        float dirAO = clamp(normal.y * 0.4 + 0.6, 0.25, 1.0);\n"

"        tint.rgb *= dirAO;\n"

"        vec3 boneBump = normalize(vec3(vnoise(fragPosition*180.0)-0.5, vnoise(fragPosition*180.0+20.0)-0.5, 1.0));\n"

"        normal = normalize(normal + boneBump * 0.05);\n"

"    }\n"

"    else if (matType == 2) // GLOWING EYES\n"

"    {\n"

"        float pulse = sin(time * 6.0) * 0.15 + 0.85;\n"

"        emission = tint.rgb * pulse * 3.0;\n"

"    }\n"

"\n"

"    vec3 lightDot = vec3(0.0);\n"

"    vec3 specular = vec3(0.0);\n"

"    float specPower = (matType == 0 || matType == 3) ? 12.0 : 4.0;\n"

"    float specStrength = (matType == 0 || matType == 3) ? 0.08 : 0.02;\n"

"    if (matType == 2) specStrength = 0.5;\n"

"\n"

"    for (int i = 0; i < MAX_LIGHTS; i++)\n"

"    {\n"

"        if (lights[i].enabled == 1)\n"

"        {\n"

"            vec3 lightDir = normalize(lights[i].position - fragPosition);\n"

"            float dist = length(lights[i].position - fragPosition);\n"

"            float atten = 1.0 / (1.0 + 0.05 * dist + 0.01 * dist * dist);\n"

"            float NdotL = max(dot(normal, lightDir), 0.0);\n"

"            lightDot += lights[i].color.rgb * NdotL * atten;\n"

"            if (NdotL > 0.0)\n"

"            {\n"

"                float specCo = pow(max(0.0, dot(viewD, reflect(-lightDir, normal))), specPower);\n"

"                specular += specCo * atten * specStrength * lights[i].color.rgb;\n"

"            }\n"

"        }\n"

"    }\n"

"\n"

"    vec3 lit = tint.rgb * (lightDot + ambient.rgb) + specular + emission;\n"

"    \n"

"    if (matType == 0 || matType == 3) {\n"

"        float rim = 1.0 - max(dot(viewD, normal), 0.0);\n"

"        lit += vec3(0.45, 0.50, 0.55) * pow(rim, 3.5) * 0.20;\n"

"    }\n"

"\n"

"    float d = length(viewPos - fragPosition);\n"

"    float fogFactor = clamp(1.0 - exp(-fogDensity * d), 0.0, 1.0);\n"

"    vec3 finalRGB = mix(lit, fogColor.rgb, fogFactor);\n"

"    finalColor = vec4(pow(finalRGB, vec3(1.0/2.15)), tint.a);\n"

"}\n";



static Shader g_houndShader = { 0 };

static int g_houndMatTypeLoc = -1;

static int g_houndTimeLoc = -1;

static int g_houndViewPosLoc = -1;

static int g_houndAmbientLoc = -1;

static int g_houndFogColorLoc = -1;

static int g_houndFogDensityLoc = -1;

static HoundSceneLight g_houndLights[4];



static Shader g_houndSkeletonShader = { 0 };

static int g_houndSkeletonMatTypeLoc = -1;

static int g_houndSkeletonTimeLoc = -1;

static int g_houndSkeletonViewPosLoc = -1;

static int g_houndSkeletonAmbientLoc = -1;

static int g_houndSkeletonFogColorLoc = -1;

static int g_houndSkeletonFogDensityLoc = -1;

static HoundSceneLight g_houndSkeletonLights[4];



static Mesh g_houndCyl = { 0 };

static Mesh g_houndSphere = { 0 };

static Mesh g_houndCube = { 0 };

static Mesh g_houndCone = { 0 };

static Material g_houndMat = { 0 };

static Material g_houndSkeletonMat = { 0 };

static bool g_houndResourcesLoaded = false;



static inline void HoundSetMaterialType(Shader shader, int matType) {

    if (shader.id == g_houndSkeletonShader.id) {

        if (g_houndSkeletonMatTypeLoc >= 0) SetShaderValue(shader, g_houndSkeletonMatTypeLoc, &matType, SHADER_UNIFORM_INT);

    } else {

        if (g_houndMatTypeLoc >= 0) SetShaderValue(shader, g_houndMatTypeLoc, &matType, SHADER_UNIFORM_INT);

    }

}



static void HoundUpdateLight(int index, int enabled, int type, Vector3 pos, Vector3 target, Color col) {

    if (index < 0 || index >= 4 || !g_houndResourcesLoaded) return;

    SetShaderValue(g_houndShader, g_houndLights[index].enabledLoc, &enabled, SHADER_UNIFORM_INT);

    SetShaderValue(g_houndShader, g_houndLights[index].typeLoc, &type, SHADER_UNIFORM_INT);

    float p[3] = { pos.x, pos.y, pos.z };

    SetShaderValue(g_houndShader, g_houndLights[index].posLoc, p, SHADER_UNIFORM_VEC3);

    float t[3] = { target.x, target.y, target.z };

    SetShaderValue(g_houndShader, g_houndLights[index].targetLoc, t, SHADER_UNIFORM_VEC3);

    float c[4] = { col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f };

    SetShaderValue(g_houndShader, g_houndLights[index].colorLoc, c, SHADER_UNIFORM_VEC4);

}



static void HoundSkeletonUpdateLight(int index, int enabled, int type, Vector3 pos, Vector3 target, Color col) {

    if (index < 0 || index >= 4 || !g_houndResourcesLoaded) return;

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].enabledLoc, &enabled, SHADER_UNIFORM_INT);

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].typeLoc, &type, SHADER_UNIFORM_INT);

    float p[3] = { pos.x, pos.y, pos.z };

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].posLoc, p, SHADER_UNIFORM_VEC3);

    float t[3] = { target.x, target.y, target.z };

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].targetLoc, t, SHADER_UNIFORM_VEC3);

    float c[4] = { col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f };

    SetShaderValue(g_houndSkeletonShader, g_houndSkeletonLights[index].colorLoc, c, SHADER_UNIFORM_VEC4);

}



static Vector3 HoundRotateAroundAxis(Vector3 v, Vector3 axis, float angleRad) {

    axis = Vector3Normalize(axis);

    float c = cosf(angleRad), s = sinf(angleRad);

    Vector3 t1 = Vector3Scale(v, c);

    Vector3 t2 = Vector3Scale(Vector3CrossProduct(axis, v), s);

    Vector3 t3 = Vector3Scale(axis, Vector3DotProduct(axis, v) * (1.0f - c));

    return Vector3Add(Vector3Add(t1, t2), t3);

}



static void HoundDrawBoneSegment(Mesh mesh, Material *mat, Vector3 from, Vector3 to, float radiusX, float radiusZ, Color tint, int mType = 0) {

    Vector3 diff = Vector3Subtract(to, from);

    float len = Vector3Length(diff);

    if (len < 0.0001f) return;

    Vector3 dir = Vector3Scale(diff, 1.0f / len);

    Quaternion q = QuaternionFromVector3ToVector3((Vector3){ 0, 1, 0 }, dir);

    Matrix rot = QuaternionToMatrix(q);

    Matrix scale = MatrixScale(radiusX, len, radiusZ);

    Matrix translate = MatrixTranslate(from.x, from.y, from.z);

    HoundSetMaterialType(mat->shader, mType);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, MatrixMultiply(MatrixMultiply(scale, rot), translate));

}



static void HoundDrawForm(Mesh mesh, Material *mat, Vector3 center, Vector3 radii, Vector3 eulerDeg, Color tint, int mType = 0) {

    Matrix s = MatrixScale(radii.x, radii.y, radii.z);

    Matrix r = MatrixRotateXYZ((Vector3){ DEG2RAD * eulerDeg.x, DEG2RAD * eulerDeg.y, DEG2RAD * eulerDeg.z });

    Matrix t = MatrixTranslate(center.x, center.y, center.z);

    HoundSetMaterialType(mat->shader, mType);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, MatrixMultiply(MatrixMultiply(s, r), t));

}



static void HoundDrawPartLocal(Mesh mesh, Material *mat, Matrix parentWorld, Vector3 localOffset, Vector3 localScale, Vector3 localEulerDeg, Color tint, int mType = 0) {

    Matrix s = MatrixScale(localScale.x, localScale.y, localScale.z);

    Matrix r = MatrixRotateXYZ((Vector3){ DEG2RAD * localEulerDeg.x, DEG2RAD * localEulerDeg.y, DEG2RAD * localEulerDeg.z });

    Matrix t = MatrixTranslate(localOffset.x, localOffset.y, localOffset.z);

    Matrix local = MatrixMultiply(MatrixMultiply(s, r), t);

    Matrix world = MatrixMultiply(local, parentWorld);

    HoundSetMaterialType(mat->shader, mType);

    mat->maps[MATERIAL_MAP_ALBEDO].color = tint;

    DrawMesh(mesh, *mat, world);

}



typedef enum { DOG_STATE_IDLE = 0, DOG_STATE_WALK, DOG_STATE_RUN, DOG_STATE_SIT, DOG_STATE_BARK } DogState;



struct DogNPC {

    Vector3 pos;

    float yaw;

    float animTime;

    

    DogState state;

    float walkPhase, runPhase, barkPhase;

    float walkBlend, runBlend, sitBlend, barkBlend;

    float barkTimer;



    Vector3 pelvis, chest, neckBase, headPivot;

    Vector3 shoulder[2], hip[2];

    Vector3 tailNodes[6];

    

    Matrix headWorld, jawWorld;



    // AI & companion state

    float targetYaw;

    Vector3 homePos;

    float wanderRadius;

    float stateTimer;

    float petTimer;

    float barkCooldown;

    bool isPet;

    float drinkTimer;

};



static DogNPC g_houndNPC;



static void InitDog(DogNPC &dog, Vector3 startPos, float startYaw) {

    dog = (DogNPC){ 0 };

    dog.pos = startPos;

    dog.yaw = startYaw;

    dog.targetYaw = startYaw;

    dog.homePos = startPos;

    dog.wanderRadius = 10.0f;

    dog.state = DOG_STATE_SIT;

    dog.sitBlend = 1.0f;

    dog.stateTimer = 8.0f;

    dog.barkCooldown = 12.0f;

}



static void InitHoundResources() {

    if (!g_houndResourcesLoaded) {

        g_houndCyl    = GenMeshCylinder(1.0f, 1.0f, 16);

        g_houndSphere = GenMeshSphere(1.0f, 16, 16);

        g_houndCube   = GenMeshCube(1.0f, 1.0f, 1.0f);

        g_houndCone   = GenMeshCone(1.0f, 1.0f, 16);



        g_houndShader = LoadShaderFromMemory(VS_HOUND_SOURCE, FS_HOUND_SOURCE);

        g_houndShader.locs[SHADER_LOC_MATRIX_MVP]   = GetShaderLocation(g_houndShader, "mvp");

        g_houndShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(g_houndShader, "matModel");

        g_houndShader.locs[SHADER_LOC_MATRIX_NORMAL]= GetShaderLocation(g_houndShader, "matNormal");

        g_houndShader.locs[SHADER_LOC_COLOR_DIFFUSE]= GetShaderLocation(g_houndShader, "colDiffuse");



        g_houndMatTypeLoc    = GetShaderLocation(g_houndShader, "matType");

        g_houndTimeLoc       = GetShaderLocation(g_houndShader, "time");

        g_houndViewPosLoc    = GetShaderLocation(g_houndShader, "viewPos");

        g_houndAmbientLoc    = GetShaderLocation(g_houndShader, "ambient");

        g_houndFogColorLoc   = GetShaderLocation(g_houndShader, "fogColor");

        g_houndFogDensityLoc = GetShaderLocation(g_houndShader, "fogDensity");



        for (int i = 0; i < 4; i++) {

            g_houndLights[i].enabledLoc = GetShaderLocation(g_houndShader, TextFormat("lights[%i].enabled", i));

            g_houndLights[i].typeLoc    = GetShaderLocation(g_houndShader, TextFormat("lights[%i].type", i));

            g_houndLights[i].posLoc     = GetShaderLocation(g_houndShader, TextFormat("lights[%i].position", i));

            g_houndLights[i].targetLoc  = GetShaderLocation(g_houndShader, TextFormat("lights[%i].target", i));

            g_houndLights[i].colorLoc   = GetShaderLocation(g_houndShader, TextFormat("lights[%i].color", i));

        }



        g_houndMat = LoadMaterialDefault();

        g_houndMat.shader = g_houndShader;



        // Compile and initialize nighttime canine skeleton shader

        g_houndSkeletonShader = LoadShaderFromMemory(VS_SKELETON_HOUND_SOURCE, FS_SKELETON_HOUND_SOURCE);

        g_houndSkeletonShader.locs[SHADER_LOC_MATRIX_MVP]   = GetShaderLocation(g_houndSkeletonShader, "mvp");

        g_houndSkeletonShader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(g_houndSkeletonShader, "matModel");

        g_houndSkeletonShader.locs[SHADER_LOC_MATRIX_NORMAL]= GetShaderLocation(g_houndSkeletonShader, "matNormal");

        g_houndSkeletonShader.locs[SHADER_LOC_COLOR_DIFFUSE]= GetShaderLocation(g_houndSkeletonShader, "colDiffuse");



        g_houndSkeletonMatTypeLoc    = GetShaderLocation(g_houndSkeletonShader, "matType");

        g_houndSkeletonTimeLoc       = GetShaderLocation(g_houndSkeletonShader, "time");

        g_houndSkeletonViewPosLoc    = GetShaderLocation(g_houndSkeletonShader, "viewPos");

        g_houndSkeletonAmbientLoc    = GetShaderLocation(g_houndSkeletonShader, "ambient");

        g_houndSkeletonFogColorLoc   = GetShaderLocation(g_houndSkeletonShader, "fogColor");

        g_houndSkeletonFogDensityLoc = GetShaderLocation(g_houndSkeletonShader, "fogDensity");



        for (int i = 0; i < 4; i++) {

            g_houndSkeletonLights[i].enabledLoc = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].enabled", i));

            g_houndSkeletonLights[i].typeLoc    = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].type", i));

            g_houndSkeletonLights[i].posLoc     = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].position", i));

            g_houndSkeletonLights[i].targetLoc  = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].target", i));

            g_houndSkeletonLights[i].colorLoc   = GetShaderLocation(g_houndSkeletonShader, TextFormat("lights[%i].color", i));

        }



        g_houndSkeletonMat = LoadMaterialDefault();

        g_houndSkeletonMat.shader = g_houndSkeletonShader;



        g_houndResourcesLoaded = true;



        // Spawn faithful Hound at gas station pump apron facing incoming car

        InitDog(g_houndNPC, (Vector3){ 124.0f, 10.0f, 135.0f }, 0.0f);

    }

}



static void UnloadHoundResources() {

    if (g_houndResourcesLoaded) {

        UnloadMesh(g_houndCyl);

        UnloadMesh(g_houndSphere);

        UnloadMesh(g_houndCube);

        UnloadMesh(g_houndCone);

        UnloadShader(g_houndShader);

        UnloadShader(g_houndSkeletonShader);

        g_houndResourcesLoaded = false;

    }

}



static void UpdateDog(DogNPC &dog, float dt) {

    dog.animTime += dt;



    // FSM State Blending Targets

    float tWalk = (dog.state == DOG_STATE_WALK) ? 1.0f : 0.0f;

    float tRun  = (dog.state == DOG_STATE_RUN)  ? 1.0f : 0.0f;

    float tSit  = (dog.state == DOG_STATE_SIT)  ? 1.0f : 0.0f;

    float tBark = (dog.state == DOG_STATE_BARK) ? 1.0f : 0.0f;



    // Smooth Transitions

    float blendSpd = dt * 6.0f;

    dog.walkBlend = Lerp(dog.walkBlend, tWalk, blendSpd);

    dog.runBlend  = Lerp(dog.runBlend,  tRun,  blendSpd);

    dog.sitBlend  = Lerp(dog.sitBlend,  tSit,  blendSpd);

    dog.barkBlend = Lerp(dog.barkBlend, tBark, blendSpd * 1.5f);



    // Barking Logic

    if (dog.state == DOG_STATE_BARK) {

        dog.barkTimer -= dt;

        dog.barkPhase += dt * 35.0f; // Rapid jaw chattering

        if (dog.barkTimer <= 0.0f) dog.state = DOG_STATE_IDLE;

    }



    // Root Motion (Sprints smoothly to keep pace with player when companion)

    float runSpd = dog.isPet ? 5.2f : 3.8f;

    float curSpeed = (1.3f * dog.walkBlend) + (runSpd * dog.runBlend);

    if (curSpeed > 0.01f) {

        dog.walkPhase += dt * 6.0f * dog.walkBlend;

        dog.runPhase  += dt * 12.0f * dog.runBlend;

        dog.pos.x += sinf(dog.yaw) * curSpeed * dt;

        dog.pos.z += cosf(dog.yaw) * curSpeed * dt;

    }



    float breathe = sinf(dog.animTime * 2.5f) * 0.01f;

    

    // Core Anatomy Heights (grounded at 10.0f)

    float basePelvis = 0.55f;

    float bob = sinf(dog.walkPhase * 2.0f) * 0.03f * dog.walkBlend + 

                fabsf(sinf(dog.runPhase)) * 0.08f * dog.runBlend;

    

    // Sit mechanics: Drop pelvis to floor, but keep chest elevated

    float sitDrop = dog.sitBlend * 0.38f; 

    

    dog.pelvis = (Vector3){ dog.pos.x, dog.pos.y + basePelvis + bob - sitDrop + breathe, dog.pos.z };

    

    Vector3 fwd = { sinf(dog.yaw), 0, cosf(dog.yaw) };

    Vector3 right = { cosf(dog.yaw), 0, -sinf(dog.yaw) };

    Vector3 up = { 0, 1, 0 };



    // Spine angle dictates posture. NEGATIVE pitches UP, pushing the chest higher than pelvis

    float spinePitch = dog.sitBlend * -40.0f; 

    Vector3 spineDir = HoundRotateAroundAxis(fwd, right, DEG2RAD * spinePitch);

    

    dog.chest = Vector3Add(dog.pelvis, Vector3Scale(spineDir, 0.45f));

    dog.neckBase = Vector3Add(dog.chest, Vector3Scale(spineDir, 0.1f));

    dog.neckBase.y += 0.05f; // Slight natural arch



    // Neck & Head Kinematics

    float barkPitch = dog.barkBlend * -25.0f; // Head throws UP/BACK when barking

    float lookPitch = 10.0f + barkPitch + (dog.sitBlend * 35.0f); 

    Vector3 neckDir = HoundRotateAroundAxis(spineDir, right, DEG2RAD * lookPitch);

    

    dog.headPivot = Vector3Add(dog.neckBase, Vector3Scale(neckDir, 0.25f));



    // Head Orientation

    float headVisualPitch = -15.0f + (dog.barkBlend * -30.0f) + (dog.sitBlend * -10.0f);

    Matrix headRot = MatrixRotateXYZ((Vector3){ DEG2RAD * headVisualPitch, 0, 0 });

    Matrix bodyRot = MatrixRotateY(dog.yaw);

    Matrix headTrans = MatrixTranslate(dog.headPivot.x, dog.headPivot.y, dog.headPivot.z);

    dog.headWorld = MatrixMultiply(MatrixMultiply(headRot, bodyRot), headTrans);



    // Jaw Pivot (TMJ)

    float jawAngle = dog.barkBlend * fmaxf(0.0f, sinf(dog.barkPhase) * 25.0f) + (dog.runBlend * 10.0f);

    Vector3 jawPivot = { 0.0f, -0.04f, 0.02f };

    Matrix tNeg = MatrixTranslate(-jawPivot.x, -jawPivot.y, -jawPivot.z);

    Matrix rJaw = MatrixRotateX(DEG2RAD * jawAngle);

    Matrix tPos = MatrixTranslate(jawPivot.x, jawPivot.y, jawPivot.z);

    dog.jawWorld = MatrixMultiply(MatrixMultiply(MatrixMultiply(tNeg, rJaw), tPos), dog.headWorld);



    // Shoulders and Hips

    dog.shoulder[0] = Vector3Add(dog.chest, Vector3Scale(right, -0.12f));

    dog.shoulder[1] = Vector3Add(dog.chest, Vector3Scale(right,  0.12f));

    dog.hip[0] = Vector3Add(dog.pelvis, Vector3Scale(right, -0.1f));

    dog.hip[1] = Vector3Add(dog.pelvis, Vector3Scale(right,  0.1f));



    // Tail Kinematics

    float wagRate = (dog.petTimer > 0.0f) ? 22.0f : 8.0f;

    float wagAmp  = (dog.petTimer > 0.0f) ? 0.32f : 0.15f;

    for (int i = 0; i < 6; i++) {

        float drop = (float)i * 0.1f;

        float wag = sinf(dog.animTime * wagRate - drop * 2.0f) * wagAmp * (1.0f - dog.sitBlend * 0.4f);

        float tuck = dog.sitBlend * 0.2f * i * (dog.petTimer > 0.0f ? 0.3f : 1.0f); // Tail wags free when pet

        Vector3 node = Vector3Add(dog.pelvis, Vector3Scale(fwd, -drop * 0.8f + tuck));

        node = Vector3Add(node, Vector3Scale(up, -drop * 0.6f - tuck));

        node = Vector3Add(node, Vector3Scale(right, wag));

        dog.tailNodes[i] = node;

    }

}



static void UpdateDogAI(DogNPC &dog, Vector3 playerPos, float dt, float lightningTimer, bool isNight) {

    dog.pos.y = 10.0f;



    // Smooth heading rotation towards targetYaw

    float diffYaw = fmodf(dog.targetYaw - dog.yaw + PI, 2.0f * PI);

    if (diffYaw < 0.0f) diffYaw += 2.0f * PI;

    diffYaw -= PI;

    dog.yaw += diffYaw * dt * 4.0f;



    // 1. Loyal Pet Companion Following Behavior

    if (dog.isPet) {

        float dx = playerPos.x - dog.pos.x;

        float dz = playerPos.z - dog.pos.z;

        float distToPlayer = sqrtf(dx * dx + dz * dz);



        if (distToPlayer > 0.15f) {

            dog.targetYaw = atan2f(dx, dz);

        }



        // Lapping blood timer

        if (dog.drinkTimer > 0.0f) {

            dog.drinkTimer -= dt;

            dog.state = DOG_STATE_BARK;

            dog.barkPhase += dt * 16.0f;

            return;

        }



        // Being petted

        if (dog.petTimer > 0.0f) {

            dog.petTimer -= dt;

            dog.state = DOG_STATE_SIT;

            return;

        }



        // Spooky alert even when pet: bark at thunder or darkness

        if (lightningTimer > 0.4f && dog.barkCooldown <= 0.0f) {

            dog.state = DOG_STATE_BARK;

            dog.barkTimer = 1.0f;

            dog.barkCooldown = 12.0f;

            return;

        }

        if (dog.barkCooldown > 0.0f) dog.barkCooldown -= dt;



        // Dynamic Following: Sprint if player is far, walk if close, sit at player's feet

        if (distToPlayer > 7.5f) {

            dog.state = DOG_STATE_RUN;  // Sprint to catch up with player!

        } else if (distToPlayer > 2.2f) {

            dog.state = DOG_STATE_WALK; // Trot alongside player

        } else {

            dog.state = DOG_STATE_SIT;  // Sit down faithfully at player's feet

        }

        return;

    }



    if (dog.petTimer > 0.0f) {

        dog.petTimer -= dt;

        dog.state = DOG_STATE_SIT;

    }



    // 2. Spooky Reaction: Thunder/Lightning or Grethnar

    if (lightningTimer > 0.4f && dog.barkCooldown <= 0.0f) {

        dog.state = DOG_STATE_BARK;

        dog.barkTimer = 1.2f;

        dog.barkCooldown = 10.0f;

        dog.targetYaw = atan2f(playerPos.x - dog.pos.x, playerPos.z - dog.pos.z);

        return;

    }

    if (dog.barkCooldown > 0.0f) dog.barkCooldown -= dt;



    // 3. Player Proximity Dynamics (Pre-pet)

    float dx = playerPos.x - dog.pos.x;

    float dz = playerPos.z - dog.pos.z;

    float distToPlayer = sqrtf(dx * dx + dz * dz);



    if (distToPlayer < 3.5f) {

        // Look towards player

        dog.targetYaw = atan2f(dx, dz);

        if (dog.state == DOG_STATE_WALK) {

            dog.state = DOG_STATE_SIT;

            dog.stateTimer = Frand(4.0f, 8.0f);

        }

        return;

    }



    // 4. Autonomous Patrol & Sitting Routine around gas station apron

    dog.stateTimer -= dt;

    if (dog.stateTimer <= 0.0f) {

        float hDx = dog.pos.x - dog.homePos.x;

        float hDz = dog.pos.z - dog.homePos.z;

        float distFromHome = sqrtf(hDx * hDx + hDz * hDz);



        int roll = GetRandomValue(0, 100);

        if (distFromHome > dog.wanderRadius) {

            dog.state = DOG_STATE_WALK;

            dog.targetYaw = atan2f(-hDx, -hDz);

            dog.stateTimer = Frand(4.0f, 7.0f);

        } else if (roll < 45) {

            // Sit and guard the apron

            dog.state = DOG_STATE_SIT;

            dog.stateTimer = Frand(6.0f, 14.0f);

        } else if (roll < 75) {

            // Stand idle, breathing, looking around

            dog.state = DOG_STATE_IDLE;

            dog.stateTimer = Frand(3.0f, 6.0f);

        } else {

            // Patrol to a new waypoint near the pumps

            dog.state = DOG_STATE_WALK;

            dog.targetYaw = Frand(-PI, PI);

            dog.stateTimer = Frand(3.0f, 6.0f);

        }

    }

}



static void DrawHound(const DogNPC &dog, Mesh cyl, Mesh sphere, Mesh cube, Mesh cone, Material *mat)

{

    Color furCol   = { 170, 160, 145, 255 }; 

    Color skinCol  = {  12,  10,  10, 255 }; 

    Color eyeCol   = { 255,  30,  10, 255 }; 

    Color toothCol = { 200, 190, 180, 255 }; 

    Color voidCol  = {   5,   0,   0, 255 }; 



    Vector3 fwd = { sinf(dog.yaw), 0, cosf(dog.yaw) };

    Vector3 right = { cosf(dog.yaw), 0, -sinf(dog.yaw) };

    Vector3 up = { 0, 1, 0 };



    // 1. TORSO & SPINE

    float chestScale = 1.0f + (dog.barkBlend * 0.15f);

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, dog.chest, 0.14f, 0.16f, furCol, 0);

    HoundDrawForm(sphere, mat, dog.chest, (Vector3){0.18f * chestScale, 0.22f * chestScale, 0.22f * chestScale}, (Vector3){0, dog.yaw*RAD2DEG, 0}, furCol, 0);

    HoundDrawForm(sphere, mat, dog.pelvis, (Vector3){0.14f, 0.14f, 0.16f}, (Vector3){0, dog.yaw*RAD2DEG, 0}, furCol, 0);

    

    HoundDrawBoneSegment(cyl, mat, dog.chest, dog.headPivot, 0.08f, 0.10f, furCol, 0);



    for (int i = 0; i < 5; i++) {

        HoundDrawBoneSegment(cyl, mat, dog.tailNodes[i], dog.tailNodes[i+1], 0.035f - (i*0.005f), 0.035f - (i*0.005f), furCol, 0);

    }



    // 2. FORELIMBS (Scapula -> Humerus -> Radius -> Paw)

    for (int side = 0; side < 2; side++) {

        float off = (side == 0) ? 0.0f : PI;

        float walkS = sinf(dog.walkPhase + off) * 0.4f * dog.walkBlend;

        float runS  = sinf(dog.runPhase + ((side==0)?0.0f:-0.4f)) * 0.8f * dog.runBlend;

        float swing = walkS + runS;

        

        float carpalFlex = fmaxf(0.0f, sinf(dog.walkPhase + off)) * 0.6f * dog.walkBlend + 

                           fmaxf(0.0f, sinf(dog.runPhase + ((side==0)?0.0f:-0.4f))) * 1.0f * dog.runBlend;



        float foldHumerus = dog.sitBlend * -0.2f; 

        float foldRadius  = 0.0f; 



        Vector3 shoulderJoint = dog.shoulder[side]; 

        

        Vector3 elbowDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.2f))), right, swing + foldHumerus);

        Vector3 elbow = Vector3Add(shoulderJoint, Vector3Scale(elbowDir, 0.25f));

        

        Vector3 carpalDir = HoundRotateAroundAxis(Vector3Scale(up, -1.0f), right, swing - carpalFlex + foldRadius); 

        Vector3 carpal = Vector3Add(elbow, Vector3Scale(carpalDir, 0.22f)); 

        

        Vector3 paw = Vector3Add(carpal, (Vector3){0, -0.05f, 0.05f});

        paw.y = fmaxf(dog.pos.y + 0.02f, paw.y);



        HoundDrawBoneSegment(cyl, mat, shoulderJoint, elbow, 0.05f, 0.06f, furCol, 0);

        HoundDrawForm(sphere, mat, elbow, (Vector3){0.055f, 0.055f, 0.055f}, (Vector3){0,0,0}, furCol, 0);

        HoundDrawBoneSegment(cyl, mat, elbow, carpal, 0.04f, 0.04f, furCol, 0);

        HoundDrawForm(sphere, mat, carpal, (Vector3){0.045f, 0.045f, 0.045f}, (Vector3){0,0,0}, furCol, 0);

        HoundDrawForm(cube, mat, paw, (Vector3){0.08f, 0.06f, 0.10f}, (Vector3){0, dog.yaw*RAD2DEG, 0}, furCol, 0);

    }



    // 3. HINDLIMBS (Femur -> Tibia -> Metatarsus -> Paw)

    for (int side = 0; side < 2; side++) {

        float off = (side == 0) ? PI/2.0f : 3.0f*PI/2.0f; 

        float walkS = sinf(dog.walkPhase + off) * 0.45f * dog.walkBlend;

        float runS  = sinf(dog.runPhase + PI + ((side==0)?0.0f:-0.4f)) * 0.9f * dog.runBlend;

        float swing = walkS + runS;

        

        float hockFlex = fmaxf(0.0f, -sinf(dog.walkPhase + off)) * 0.8f * dog.walkBlend + 

                         fmaxf(0.0f, -sinf(dog.runPhase + PI + ((side==0)?0.0f:-0.4f))) * 1.2f * dog.runBlend;



        float foldFemur  = dog.sitBlend * -1.0f; 

        float foldTibia  = dog.sitBlend *  1.2f; 

        float foldCannon = dog.sitBlend * -1.5f; 



        Vector3 hipJoint = dog.hip[side];



        Vector3 stifleDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd,  0.4f))), right, swing + foldFemur);

        Vector3 stifle = Vector3Add(hipJoint, Vector3Scale(stifleDir, 0.28f)); 



        Vector3 hockDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.4f))), right, swing + hockFlex + foldTibia);

        Vector3 hock = Vector3Add(stifle, Vector3Scale(hockDir, 0.26f));



        Vector3 metaDir = HoundRotateAroundAxis(Vector3Scale(up, -1.0f), right, swing + hockFlex + foldCannon);

        Vector3 fetlock = Vector3Add(hock, Vector3Scale(metaDir, 0.18f));

        

        Vector3 paw = Vector3Add(fetlock, (Vector3){0, -0.04f, 0.06f});

        paw.y = fmaxf(dog.pos.y + 0.02f, paw.y);



        HoundDrawBoneSegment(cyl, mat, hipJoint, stifle, 0.08f, 0.09f, furCol, 0);

        HoundDrawForm(sphere, mat, stifle, (Vector3){0.065f, 0.065f, 0.065f}, (Vector3){0,0,0}, furCol, 0);

        HoundDrawBoneSegment(cyl, mat, stifle, hock, 0.05f, 0.05f, furCol, 0);

        HoundDrawForm(sphere, mat, hock, (Vector3){0.055f, 0.065f, 0.055f}, (Vector3){0,0,0}, furCol, 0);

        HoundDrawBoneSegment(cyl, mat, hock, fetlock, 0.035f, 0.035f, furCol, 0);

        HoundDrawForm(cube, mat, paw, (Vector3){0.07f, 0.06f, 0.10f}, (Vector3){0, dog.yaw*RAD2DEG, 0}, furCol, 0);

    }



    // 4. HEAD & SNOUT

    Matrix hw = dog.headWorld;



    HoundDrawPartLocal(sphere, mat, hw, (Vector3){ 0, 0, 0 }, (Vector3){0.09f, 0.09f, 0.10f}, (Vector3){0,0,0}, furCol, 0);

    HoundDrawPartLocal(cube, mat, hw, (Vector3){ 0, -0.02f, 0.12f }, (Vector3){0.06f, 0.06f, 0.14f}, (Vector3){0,0,0}, furCol, 0);

    HoundDrawPartLocal(sphere, mat, hw, (Vector3){ 0, -0.01f, 0.19f }, (Vector3){0.025f, 0.02f, 0.02f}, (Vector3){0,0,0}, skinCol, 1);

    

    HoundDrawPartLocal(sphere, mat, hw, (Vector3){ -0.045f, 0.02f, 0.08f }, (Vector3){0.015f, 0.015f, 0.015f}, (Vector3){0,0,0}, eyeCol, 2);

    HoundDrawPartLocal(sphere, mat, hw, (Vector3){  0.045f, 0.02f, 0.08f }, (Vector3){0.015f, 0.015f, 0.015f}, (Vector3){0,0,0}, eyeCol, 2);



    HoundDrawPartLocal(cone, mat, hw, (Vector3){ -0.07f, 0.08f, -0.02f }, (Vector3){0.03f, 0.10f, 0.04f}, (Vector3){-15, 0, -25}, furCol, 0);

    HoundDrawPartLocal(cone, mat, hw, (Vector3){  0.07f, 0.08f, -0.02f }, (Vector3){0.03f, 0.10f, 0.04f}, (Vector3){-15, 0,  25}, furCol, 0);



    HoundDrawPartLocal(cube, mat, hw, (Vector3){ 0, -0.055f, 0.14f }, (Vector3){0.045f, 0.01f, 0.08f}, (Vector3){0,0,0}, toothCol, 1);



    // 5. JAW

    Matrix jw = dog.jawWorld;

    HoundDrawPartLocal(cube, mat, jw, (Vector3){ 0, -0.03f, 0.11f }, (Vector3){0.05f, 0.04f, 0.13f}, (Vector3){0,0,0}, furCol, 0);

    HoundDrawPartLocal(cube, mat, jw, (Vector3){ 0, -0.005f, 0.13f }, (Vector3){0.04f, 0.01f, 0.07f}, (Vector3){0,0,0}, toothCol, 1);

    HoundDrawPartLocal(cube, mat, jw, (Vector3){ 0, -0.01f, 0.08f }, (Vector3){0.04f, 0.02f, 0.08f}, (Vector3){0,0,0}, voidCol, 1);

}



// ----------------------------------------------------------------------------------

// PROCEDURAL CANINE OSTEOLOGY (Nighttime Skeleton Hound)

// ----------------------------------------------------------------------------------

static void DrawSkeletonHound(const DogNPC &dog, Mesh cyl, Mesh sphere, Mesh cube, Mesh cone, Material *mat)

{

    Color boneCol = { 218, 212, 198, 255 }; // Aged yellowed calcium

    Color voidCol = {  12,   8,   6, 255 }; // Deep dark internal cavity

    Color eyeCol  = { 255,  30,  10, 255 }; // Glowing demonic red

    Color toothCol= { 200, 190, 180, 255 }; // Discolored teeth



    Vector3 fwd = { sinf(dog.yaw), 0, cosf(dog.yaw) };

    Vector3 right = { cosf(dog.yaw), 0, -sinf(dog.yaw) };

    Vector3 up = { 0, 1, 0 };



    // ==========================================================

    // 1. AXIAL SKELETON: SPINE & PELVIS

    // ==========================================================

    for (int i = 0; i < 12; i++) {

        float t = (float)i / 11.0f;

        Vector3 vPos = Vector3Lerp(dog.pelvis, dog.chest, t);

        vPos = Vector3Add(vPos, Vector3Scale(up, sinf(t * PI) * 0.04f));

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), vPos, (Vector3){0.025f, 0.025f, 0.028f}, (Vector3){0,0,0}, boneCol, 0);

    }

    

    for (int i = 0; i < 6; i++) {

        float t = (float)i / 5.0f;

        Vector3 vPos = Vector3Lerp(dog.chest, dog.neckBase, t);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), vPos, (Vector3){0.022f, 0.022f, 0.025f}, (Vector3){0,0,0}, boneCol, 0);

    }



    for (int i = 0; i < 5; i++) {

        HoundDrawBoneSegment(cyl, mat, dog.tailNodes[i], dog.tailNodes[i+1], 0.020f - (i*0.003f), 0.020f - (i*0.003f), boneCol, 0);

    }



    Vector3 iliumL = Vector3Add(dog.pelvis, Vector3Add(Vector3Scale(fwd, 0.1f), Vector3Scale(right, -0.08f)));

    Vector3 iliumR = Vector3Add(dog.pelvis, Vector3Add(Vector3Scale(fwd, 0.1f), Vector3Scale(right,  0.08f)));

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, iliumL, 0.02f, 0.02f, boneCol, 0);

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, iliumR, 0.02f, 0.02f, boneCol, 0);

    

    Vector3 ischiumL = Vector3Add(dog.pelvis, Vector3Add(Vector3Scale(fwd, -0.12f), Vector3Scale(right, -0.05f)));

    Vector3 ischiumR = Vector3Add(dog.pelvis, Vector3Add(Vector3Scale(fwd, -0.12f), Vector3Scale(right,  0.05f)));

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, ischiumL, 0.02f, 0.02f, boneCol, 0);

    HoundDrawBoneSegment(cyl, mat, dog.pelvis, ischiumR, 0.02f, 0.02f, boneCol, 0);



    // ==========================================================

    // 2. AXIAL SKELETON: RIBCAGE (13 Ribs)

    // ==========================================================

    for (int r = 0; r < 9; r++) {

        float t = 0.3f + (r / 8.0f) * 0.7f;

        Vector3 rootNode = Vector3Lerp(dog.pelvis, dog.chest, t);

        rootNode = Vector3Add(rootNode, Vector3Scale(up, sinf(t * PI) * 0.04f));

        

        float width = 0.12f - fabsf((float)r - 4.0f) * 0.01f; 

        float drop = 0.22f - fabsf((float)r - 4.0f) * 0.015f;

        float backwardSweep = (float)r * 0.015f;



        for (int side = -1; side <= 1; side += 2) {

            Vector3 ribMid = Vector3Add(rootNode, Vector3Scale(right, side * width));

            ribMid = Vector3Add(ribMid, Vector3Scale(up, -drop * 0.4f));

            ribMid = Vector3Add(ribMid, Vector3Scale(fwd, -backwardSweep * 0.5f));

            

            Vector3 ribBot = Vector3Add(rootNode, Vector3Scale(right, side * width * 0.4f));

            ribBot = Vector3Add(ribBot, Vector3Scale(up, -drop));

            ribBot = Vector3Add(ribBot, Vector3Scale(fwd, -backwardSweep));



            HoundDrawBoneSegment(cyl, mat, rootNode, ribMid, 0.012f, 0.006f, boneCol, 0);

            HoundDrawBoneSegment(cyl, mat, ribMid, ribBot, 0.010f, 0.005f, boneCol, 0);

        }

    }

    Vector3 sternumFront = Vector3Add(dog.chest, Vector3Scale(up, -0.22f));

    Vector3 sternumBack  = Vector3Add(Vector3Lerp(dog.pelvis, dog.chest, 0.3f), Vector3Scale(up, -0.20f));

    HoundDrawBoneSegment(cyl, mat, sternumBack, sternumFront, 0.015f, 0.02f, boneCol, 0);



    // ==========================================================

    // 3. APPENDICULAR SKELETON: THORACIC LIMBS (Front)

    // ==========================================================

    for (int side = 0; side < 2; side++) {

        float off = (side == 0) ? 0.0f : PI;

        float walkS = sinf(dog.walkPhase + off) * 0.4f * dog.walkBlend;

        float runS  = sinf(dog.runPhase + ((side==0)?0.0f:-0.4f)) * 0.8f * dog.runBlend;

        float swing = walkS + runS;

        

        float carpalFlex = fmaxf(0.0f, sinf(dog.walkPhase + off)) * 0.6f * dog.walkBlend + 

                           fmaxf(0.0f, sinf(dog.runPhase + ((side==0)?0.0f:-0.4f))) * 1.0f * dog.runBlend;



        float foldHumerus = dog.sitBlend * -0.2f; 

        float foldRadius  = 0.0f; 



        Vector3 shoulderJoint = dog.shoulder[side]; 

        

        Vector3 scapulaTop = Vector3Add(shoulderJoint, Vector3Add(Vector3Scale(up, 0.15f), Vector3Scale(fwd, -0.08f)));

        HoundDrawBoneSegment(cyl, mat, shoulderJoint, scapulaTop, 0.035f, 0.01f, boneCol, 0);



        Vector3 elbowDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.2f))), right, swing + foldHumerus);

        Vector3 elbow = Vector3Add(shoulderJoint, Vector3Scale(elbowDir, 0.25f));

        

        Vector3 carpalDir = HoundRotateAroundAxis(Vector3Scale(up, -1.0f), right, swing - carpalFlex + foldRadius); 

        Vector3 carpal = Vector3Add(elbow, Vector3Scale(carpalDir, 0.22f)); 

        

        Vector3 pawCenter = Vector3Add(carpal, (Vector3){0, -0.05f, 0.05f});

        pawCenter.y = fmaxf(dog.pos.y + 0.01f, pawCenter.y);



        HoundDrawBoneSegment(cyl, mat, shoulderJoint, elbow, 0.02f, 0.02f, boneCol, 0);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), elbow, (Vector3){0.025f, 0.025f, 0.025f}, (Vector3){0,0,0}, boneCol, 0);

        HoundDrawBoneSegment(cyl, mat, elbow, carpal, 0.015f, 0.015f, boneCol, 0);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), carpal, (Vector3){0.02f, 0.02f, 0.02f}, (Vector3){0,0,0}, boneCol, 0);

        

        for(int t = -1; t <= 1; t++) {

            Vector3 toe = Vector3Add(pawCenter, Vector3Scale(right, t * 0.02f));

            HoundDrawBoneSegment(cyl, mat, carpal, toe, 0.008f, 0.008f, boneCol, 0);

        }

    }



    // ==========================================================

    // 4. APPENDICULAR SKELETON: PELVIC LIMBS (Hind)

    // ==========================================================

    for (int side = 0; side < 2; side++) {

        float off = (side == 0) ? PI/2.0f : 3.0f*PI/2.0f;

        float walkS = sinf(dog.walkPhase + off) * 0.45f * dog.walkBlend;

        float runS  = sinf(dog.runPhase + PI + ((side==0)?0.0f:-0.4f)) * 0.9f * dog.runBlend;

        float swing = walkS + runS;

        

        float hockFlex = fmaxf(0.0f, -sinf(dog.walkPhase + off)) * 0.8f * dog.walkBlend + 

                         fmaxf(0.0f, -sinf(dog.runPhase + PI + ((side==0)?0.0f:-0.4f))) * 1.2f * dog.runBlend;



        float foldFemur  = dog.sitBlend * 1.2f;

        float foldTibia  = dog.sitBlend * 2.2f;

        float foldCannon = dog.sitBlend * 1.5f;



        Vector3 hipJoint = dog.hip[side];



        Vector3 stifleDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd,  0.4f))), right, swing + foldFemur);

        Vector3 stifle = Vector3Add(hipJoint, Vector3Scale(stifleDir, 0.28f)); 



        Vector3 hockDir = HoundRotateAroundAxis(Vector3Normalize(Vector3Add(Vector3Scale(up, -1), Vector3Scale(fwd, -0.4f))), right, swing + hockFlex - foldTibia);

        Vector3 hock = Vector3Add(stifle, Vector3Scale(hockDir, 0.26f));



        Vector3 metaDir = HoundRotateAroundAxis(Vector3Scale(up, -1.0f), right, swing + hockFlex - foldCannon);

        Vector3 fetlock = Vector3Add(hock, Vector3Scale(metaDir, 0.18f));

        

        Vector3 pawCenter = Vector3Add(fetlock, (Vector3){0, -0.04f, 0.06f});

        pawCenter.y = fmaxf(dog.pos.y + 0.01f, pawCenter.y);



        HoundDrawBoneSegment(cyl, mat, hipJoint, stifle, 0.025f, 0.025f, boneCol, 0);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), stifle, (Vector3){0.028f, 0.028f, 0.028f}, (Vector3){0,0,0}, boneCol, 0);

        HoundDrawBoneSegment(cyl, mat, stifle, hock, 0.02f, 0.02f, boneCol, 0);

        HoundDrawPartLocal(sphere, mat, MatrixIdentity(), hock, (Vector3){0.022f, 0.022f, 0.022f}, (Vector3){0,0,0}, boneCol, 0);

        HoundDrawBoneSegment(cyl, mat, hock, fetlock, 0.015f, 0.015f, boneCol, 0);

        

        for(int t = -1; t <= 1; t++) {

            Vector3 toe = Vector3Add(pawCenter, Vector3Scale(right, t * 0.02f));

            HoundDrawBoneSegment(cyl, mat, fetlock, toe, 0.008f, 0.008f, boneCol, 0);

        }

    }



    // ==========================================================

    // 5. AXIAL SKELETON: SKULL & MAXILLA

    // ==========================================================

    Matrix hw = dog.headWorld;



    HoundDrawPartLocal(sphere, mat, hw, (Vector3){ 0, 0, 0 }, (Vector3){0.07f, 0.07f, 0.08f}, (Vector3){0,0,0}, boneCol, 0);

    HoundDrawPartLocal(cube, mat, hw, (Vector3){ 0, 0.07f, -0.02f }, (Vector3){0.01f, 0.02f, 0.08f}, (Vector3){0,0,0}, boneCol, 0);

    

    HoundDrawPartLocal(cyl, mat, hw, (Vector3){ -0.06f, -0.02f, 0.05f }, (Vector3){0.01f, 0.08f, 0.01f}, (Vector3){0,0,-85}, boneCol, 0);

    HoundDrawPartLocal(cyl, mat, hw, (Vector3){  0.06f, -0.02f, 0.05f }, (Vector3){0.01f, 0.08f, 0.01f}, (Vector3){0,0, 85}, boneCol, 0);



    HoundDrawPartLocal(cyl, mat, hw, (Vector3){ 0, -0.02f, 0.12f }, (Vector3){0.035f, 0.08f, 0.035f}, (Vector3){85,0,0}, boneCol, 0);

    

    HoundDrawPartLocal(sphere, mat, hw, (Vector3){ 0, -0.01f, 0.17f }, (Vector3){0.02f, 0.015f, 0.02f}, (Vector3){0,0,0}, voidCol, 1);

    

    HoundDrawPartLocal(sphere, mat, hw, (Vector3){ -0.04f, 0.02f, 0.07f }, (Vector3){0.025f, 0.025f, 0.02f}, (Vector3){0,0,0}, voidCol, 1);

    HoundDrawPartLocal(sphere, mat, hw, (Vector3){  0.04f, 0.02f, 0.07f }, (Vector3){0.025f, 0.025f, 0.02f}, (Vector3){0,0,0}, voidCol, 1);



    HoundDrawPartLocal(sphere, mat, hw, (Vector3){ -0.04f, 0.02f, 0.075f }, (Vector3){0.010f, 0.010f, 0.010f}, (Vector3){0,0,0}, eyeCol, 2);

    HoundDrawPartLocal(sphere, mat, hw, (Vector3){  0.04f, 0.02f, 0.075f }, (Vector3){0.010f, 0.010f, 0.010f}, (Vector3){0,0,0}, eyeCol, 2);



    HoundDrawPartLocal(cube, mat, hw, (Vector3){ 0, -0.05f, 0.14f }, (Vector3){0.045f, 0.01f, 0.08f}, (Vector3){0,0,0}, boneCol, 0);

    for(int t=-2; t<=2; t+=4) {

        HoundDrawPartLocal(cone, mat, hw, (Vector3){ t*0.01f, -0.065f, 0.17f }, (Vector3){0.008f, 0.02f, 0.008f}, (Vector3){0,0,0}, toothCol, 3);

    }



    // ==========================================================

    // 6. AXIAL SKELETON: MANDIBLE

    // ==========================================================

    Matrix jw = dog.jawWorld;

    

    HoundDrawPartLocal(cyl, mat, jw, (Vector3){ -0.035f, -0.02f, 0.08f }, (Vector3){0.015f, 0.12f, 0.015f}, (Vector3){85,-15,0}, boneCol, 0);

    HoundDrawPartLocal(cyl, mat, jw, (Vector3){  0.035f, -0.02f, 0.08f }, (Vector3){0.015f, 0.12f, 0.015f}, (Vector3){85, 15,0}, boneCol, 0);

    

    HoundDrawPartLocal(cube, mat, jw, (Vector3){ 0, -0.01f, 0.13f }, (Vector3){0.035f, 0.01f, 0.07f}, (Vector3){0,0,0}, boneCol, 0);

    for(int t=-2; t<=2; t+=4) {

        HoundDrawPartLocal(cone, mat, jw, (Vector3){ t*0.008f, 0.005f, 0.16f }, (Vector3){0.006f, 0.015f, 0.006f}, (Vector3){180,0,0}, toothCol, 3);

    }

}





// ----------------------------------------------------------------------

// ATMOSPHERIC VOLUMETRIC DUST & FOOTSTEP TRAILS SYSTEM (ZERO-HEAP FRAGMENTATION)

// ----------------------------------------------------------------------

static void SpawnPickupDust(Vector3 center, int count = 20)

{

    int spawnCount = count;

    if (g_dustParticles.size() + spawnCount > 64) {

        int overflow = (int)(g_dustParticles.size() + spawnCount) - 64;

        if (overflow > (int)g_dustParticles.size()) overflow = (int)g_dustParticles.size();

        g_dustParticles.erase(g_dustParticles.begin(), g_dustParticles.begin() + overflow);

    }

    for (int i = 0; i < spawnCount; i++) {

        DustParticle p;

        float r = Frand(0.02f, 0.18f);

        float a = Frand(0.0f, 2.0f * PI);

        p.pos = (Vector3){ center.x + cosf(a) * r, center.y + Frand(-0.01f, 0.04f), center.z + sinf(a) * r };

        p.vel = (Vector3){ cosf(a) * Frand(0.06f, 0.28f), Frand(0.12f, 0.38f), sinf(a) * Frand(0.06f, 0.28f) };

        p.size = Frand(0.008f, 0.018f);

        p.maxLife = Frand(1.4f, 2.4f);

        p.life = p.maxLife;

        unsigned char shade = (unsigned char)GetRandomValue(195, 240);

        p.color = (Color){ shade, (unsigned char)(shade * 0.96f), (unsigned char)(shade * 0.88f), 220 };

        p.spin = Frand(0.0f, 360.0f);

        p.spinSpeed = Frand(-120.0f, 120.0f);

        g_dustParticles.push_back(p);

    }

}



static void UpdateDustParticles(float dt)

{

    for (size_t i = 0; i < g_dustParticles.size(); ) {

        DustParticle &p = g_dustParticles[i];

        p.life -= dt;

        if (p.life <= 0.0f) {

            // O(1) swap-and-pop: zero memory shifting or heap churn

            g_dustParticles[i] = g_dustParticles.back();

            g_dustParticles.pop_back();

            continue;

        }

        p.vel.x *= (1.0f - 1.5f * dt);

        p.vel.z *= (1.0f - 1.5f * dt);

        p.vel.y -= 0.06f * dt;

        p.pos.x += p.vel.x * dt;

        p.pos.y += p.vel.y * dt;

        p.pos.z += p.vel.z * dt;

        p.spin += p.spinSpeed * dt;

        i++;

    }

}



// Single-batch hardware billboard rendering: replaces 300 DrawCube calls with 1 batch pass

static void DrawDustParticles(const Camera3D &camera)

{

    if (g_dustParticles.empty()) return;

    Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwd, camera.up));

    Vector3 up  = Vector3CrossProduct(rgt, fwd);



    rlBegin(RL_QUADS);

    for (const auto &p : g_dustParticles) {

        float alphaNorm = Clamp(p.life / p.maxLife, 0.0f, 1.0f);

        unsigned char a = (unsigned char)(p.color.a * alphaNorm);

        if (a < 4) continue;

        Color c = ApplyShopLighting(p.pos, p.color);

        rlColor4ub(c.r, c.g, c.b, a);



        float hs = p.size * 0.85f;

        Vector3 rx = Vector3Scale(rgt, hs);

        Vector3 uy = Vector3Scale(up, hs);



        rlVertex3f(p.pos.x - rx.x - uy.x, p.pos.y - rx.y - uy.y, p.pos.z - rx.z - uy.z);

        rlVertex3f(p.pos.x - rx.x + uy.x, p.pos.y - rx.y + uy.y, p.pos.z - rx.z - uy.z);

        rlVertex3f(p.pos.x + rx.x + uy.x, p.pos.y + rx.y + uy.y, p.pos.z + rx.z + uy.z);

        rlVertex3f(p.pos.x + rx.x - uy.x, p.pos.y + rx.y - uy.y, p.pos.z + rx.z - uy.z);

    }

    rlEnd();

}



static void AddFootstepTrail(Vector3 pos, float yaw, bool isLeft)

{

    if (g_footstepTrails.size() >= 32) {

        g_footstepTrails.erase(g_footstepTrails.begin());

    }

    FootstepTrail t;

    t.pos = pos;

    t.yaw = yaw;

    t.maxLife = 10.0f;

    t.life = t.maxLife;

    t.isLeft = isLeft;

    g_footstepTrails.push_back(t);

}



static void UpdateFootstepTrails(float dt)

{

    for (size_t i = 0; i < g_footstepTrails.size(); ) {

        g_footstepTrails[i].life -= dt;

        if (g_footstepTrails[i].life <= 0.0f) {

            // O(1) swap-and-pop

            g_footstepTrails[i] = g_footstepTrails.back();

            g_footstepTrails.pop_back();

        } else {

            i++;

        }

    }

}



static void DrawFootstepTrails()

{

    if (g_footstepTrails.empty()) return;

    rlBegin(RL_QUADS);

    rlNormal3f(0.0f, 1.0f, 0.0f);

    for (const auto &t : g_footstepTrails) {

        float alphaNorm = Clamp(t.life / t.maxLife, 0.0f, 1.0f);

        unsigned char a = (unsigned char)(115.0f * alphaNorm);

        if (a < 2) continue;

        Color col = ApplyShopLighting(t.pos, (Color){ 16, 18, 22, a });

        rlColor4ub(col.r, col.g, col.b, a);



        float rad = t.yaw * DEG2RAD;

        float fx = sinf(rad), fz = cosf(rad);

        float rx = cosf(rad), rz = -sinf(rad);

        float y = 10.021f;



        // Front sole pad

        float fcx = t.pos.x + fx * 0.055f;

        float fcz = t.pos.z + fz * 0.055f;

        float sw = 0.046f, sl = 0.070f;

        rlVertex3f(fcx - rx * sw - fx * sl, y, fcz - rz * sw - fz * sl);

        rlVertex3f(fcx - rx * sw + fx * sl, y, fcz - rz * sw + fz * sl);

        rlVertex3f(fcx + rx * sw + fx * sl, y, fcz + rz * sw + fz * sl);

        rlVertex3f(fcx + rx * sw - fx * sl, y, fcz + rz * sw - fz * sl);



        // Rear heel pad

        float hcx = t.pos.x - fx * 0.065f;

        float hcz = t.pos.z - fz * 0.065f;

        float hw = 0.040f, hl = 0.038f;

        rlVertex3f(hcx - rx * hw - fx * hl, y, hcz - rz * hw - fz * hl);

        rlVertex3f(hcx - rx * hw + fx * hl, y, hcz - rz * hw + fz * hl);

        rlVertex3f(hcx + rx * hw + fx * hl, y, hcz + rz * hw + fz * hl);

        rlVertex3f(hcx + rx * hw - fx * hl, y, hcz + rz * hw - fz * hl);

    }

    rlEnd();

}



static void UpdateShellCasingsAndSparks(float dt)

{

    for (size_t i = 0; i < g_shellCasings.size(); ) {

        ShellCasing &sc = g_shellCasings[i];

        sc.life -= dt;

        if (sc.life <= 0.0f) {

            g_shellCasings[i] = g_shellCasings.back();

            g_shellCasings.pop_back();

            continue;

        }

        if (!sc.landed) {

            sc.vel.y -= 9.8f * dt;

            sc.pos.x += sc.vel.x * dt;

            sc.pos.y += sc.vel.y * dt;

            sc.pos.z += sc.vel.z * dt;

            sc.rot.x += sc.rotVel.x * dt;

            sc.rot.y += sc.rotVel.y * dt;

            sc.rot.z += sc.rotVel.z * dt;

            if (sc.pos.y <= 10.024f) {

                sc.pos.y = 10.024f;

                sc.vel.y *= -0.28f;

                sc.vel.x *= 0.55f;

                sc.vel.z *= 0.55f;

                sc.rotVel = Vector3Scale(sc.rotVel, 0.35f);

                if (fabsf(sc.vel.y) < 0.25f) {

                    sc.landed = true;

                    sc.vel = (Vector3){0, 0, 0};

                }

            }

        }

        i++;

    }



    for (size_t i = 0; i < g_gunSparks.size(); ) {

        GunSpark &gs = g_gunSparks[i];

        gs.life -= dt;

        if (gs.life <= 0.0f) {

            g_gunSparks[i] = g_gunSparks.back();

            g_gunSparks.pop_back();

            continue;

        }

        gs.vel.y -= 9.8f * dt;

        gs.pos.x += gs.vel.x * dt;

        gs.pos.y += gs.vel.y * dt;

        gs.pos.z += gs.vel.z * dt;

        i++;

    }

}



static void DrawShellCasingsAndSparks()

{

    for (const auto &sc : g_shellCasings) {

        rlPushMatrix();

            rlTranslatef(sc.pos.x, sc.pos.y, sc.pos.z);

            rlRotatef(sc.rot.y, 0, 1, 0);

            rlRotatef(sc.rot.x, 1, 0, 0);

            rlRotatef(sc.rot.z, 0, 0, 1);

            Color brass = ApplyShopLighting(sc.pos, (Color){ 225, 185, 65, 255 });

            DrawCylinder((Vector3){0,0,0}, 0.005f, 0.005f, 0.019f, 6, brass);

        rlPopMatrix();

    }

    for (const auto &gs : g_gunSparks) {

        DrawSphere(gs.pos, 0.012f, gs.color);

        DrawSphere(gs.pos, 0.024f, Fade(gs.color, 0.4f));

    }

}



static void DrawChocolateBar(Vector3 pos, int subType, bool opened, bool held, Vector3 fwdDir, float distSq = 0.0f)

{

    Color wrapCol;

    Color foilCol;

    Color accentCol;

    if (subType == 0) { // Dark Noir 85% Cacao

        wrapCol   = ApplyShopLighting(pos, (Color){ 24, 24, 26, 255 });

        foilCol   = ApplyShopLighting(pos, (Color){ 235, 195, 75, 255 }); // Gold foil

        accentCol = (Color){ 245, 215, 110, 255 };

    } else if (subType == 1) { // Alpine Milk Chocolate

        wrapCol   = ApplyShopLighting(pos, (Color){ 28, 65, 155, 255 }); // Royal Blue

        foilCol   = ApplyShopLighting(pos, (Color){ 215, 222, 230, 255 }); // Silver foil

        accentCol = (Color){ 240, 245, 255, 255 };

    } else { // Sea Salt Caramel

        wrapCol   = ApplyShopLighting(pos, (Color){ 180, 95, 32, 255 }); // Amber bronze

        foilCol   = ApplyShopLighting(pos, (Color){ 225, 165, 95, 255 }); // Copper foil

        accentCol = (Color){ 255, 225, 160, 255 };

    }



    float w = 0.088f;  // Width

    float h = 0.016f;  // Height/thickness

    float l = 0.180f;  // Length



    // Fast distant LOD when observing the whole cooler: single clean slab (1 draw call)

    if (!held && distSq > 5.5f * 5.5f) {

        DrawCube(pos, w, h, l, wrapCol);

        return;

    }



    rlPushMatrix();

    rlTranslatef(pos.x, pos.y, pos.z);



    if (held) {

        Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwdDir, (Vector3){0, 1, 0}));

        rlRotatef(25.0f, rgt.x, rgt.y, rgt.z);

        rlRotatef(-15.0f, 0, 1, 0);

    }



    Color chocDark = ApplyShopLighting(pos, (Color){ 52, 28, 18, 255 });



    // Inner chocolate bar body (dark rich chocolate)

    DrawCube((Vector3){ 0, 0, 0 }, w, h, l, chocDark);



    if (opened) {

        // Exposed breakable chocolate grid on top half (Z > 0)

        for (int row = 0; row < 2; row++) {

            for (int col = 0; col < 2; col++) {

                float sx = -w * 0.25f + col * (w * 0.5f);

                float sz = 0.020f + row * 0.045f;

                DrawCube((Vector3){ sx, h * 0.52f, sz }, w * 0.40f, 0.004f, 0.038f, chocDark);

                DrawCubeWires((Vector3){ sx, h * 0.52f, sz }, w * 0.40f, 0.004f, 0.038f, ApplyShopLighting(pos, (Color){ 35, 18, 10, 255 }));

            }

        }

        // Crinkled peeled foil boundary

        DrawCube((Vector3){ 0, 0.002f, -0.005f }, w * 1.03f, h * 1.06f, 0.015f, foilCol);

        // Bottom sleeve wrapper (Z <= 0)

        DrawCube((Vector3){ 0, 0, -l * 0.25f }, w * 1.02f, h * 1.04f, l * 0.50f, wrapCol);

        // Gold/silver brand accent band

        DrawCube((Vector3){ 0, 0, -l * 0.25f }, w * 1.025f, h * 1.05f, 0.035f, foilCol);

    } else {

        // Fully wrapped bar

        DrawCube((Vector3){ 0, 0, 0 }, w * 1.02f, h * 1.04f, l * 0.94f, wrapCol);

        // Shiny foil ends peeking out

        DrawCube((Vector3){ 0, 0,  l * 0.49f }, w * 0.98f, h * 0.90f, 0.025f, foilCol);

        DrawCube((Vector3){ 0, 0, -l * 0.49f }, w * 0.98f, h * 0.90f, 0.025f, foilCol);

        // Center printed label band & brand accent stripe

        DrawCube((Vector3){ 0, 0, 0 }, w * 1.025f, h * 1.05f, 0.070f, accentCol);

        DrawCube((Vector3){ 0, 0, 0 }, w * 1.030f, h * 1.06f, 0.045f, wrapCol);

    }



    rlPopMatrix();

}



static void DrawGunWorld(Vector3 pos, bool held)

{

    if (held) return;



    // Luxurious executive presentation velvet tray on checkout counter

    Vector3 trayPos = { pos.x, pos.y - 0.03f, pos.z };

    Color velvetDark = ApplyShopLighting(trayPos, (Color){ 75, 12, 20, 255 });

    Color brassTrim  = ApplyShopLighting(trayPos, (Color){ 175, 140, 60, 255 });

    DrawCube(trayPos, 0.46f, 0.022f, 0.32f, velvetDark);

    DrawCubeWires(trayPos, 0.462f, 0.024f, 0.322f, brassTrim);

    DrawCube((Vector3){ trayPos.x, trayPos.y + 0.012f, trayPos.z - 0.15f }, 0.46f, 0.015f, 0.02f, brassTrim);

    DrawCube((Vector3){ trayPos.x, trayPos.y + 0.012f, trayPos.z + 0.15f }, 0.46f, 0.015f, 0.02f, brassTrim);

    DrawCube((Vector3){ trayPos.x - 0.22f, trayPos.y + 0.012f, trayPos.z }, 0.02f, 0.015f, 0.32f, brassTrim);

    DrawCube((Vector3){ trayPos.x + 0.22f, trayPos.y + 0.012f, trayPos.z }, 0.02f, 0.015f, 0.32f, brassTrim);



    // Box of 9mm Ammunition beside the gun

    Vector3 boxPos = { pos.x - 0.13f, pos.y - 0.005f, pos.z + 0.06f };

    Color ammoBoxCol = ApplyShopLighting(boxPos, (Color){ 42, 65, 45, 255 });

    DrawCube(boxPos, 0.10f, 0.045f, 0.075f, ammoBoxCol);

    DrawCubeWires(boxPos, 0.102f, 0.046f, 0.076f, ApplyShopLighting(boxPos, (Color){ 180, 195, 120, 255 }));

    // Loose brass cartridges resting on tray

    Color brass = ApplyShopLighting(boxPos, (Color){ 225, 185, 65, 255 });

    DrawCylinderEx((Vector3){ pos.x - 0.12f, pos.y - 0.015f, pos.z - 0.06f }, (Vector3){ pos.x - 0.10f, pos.y - 0.015f, pos.z - 0.06f }, 0.005f, 0.005f, 6, brass);

    DrawCylinderEx((Vector3){ pos.x - 0.12f, pos.y - 0.015f, pos.z - 0.04f }, (Vector3){ pos.x - 0.10f, pos.y - 0.015f, pos.z - 0.04f }, 0.005f, 0.005f, 6, brass);



    // The Pistol Model resting on the tray

    rlPushMatrix();

    rlTranslatef(pos.x + 0.05f, pos.y, pos.z);

    rlRotatef(-25.0f, 0, 1, 0); // Angled presentation

    rlRotatef(90.0f, 0, 0, 1);  // Resting on its side on the velvet

    rlScalef(0.85f, 0.85f, 0.85f);



    Color frameCol = ApplyShopLighting(pos, (Color){ 30, 32, 35, 255 });

    Color slideCol = ApplyShopLighting(pos, (Color){ 44, 46, 50, 255 });

    Color metalCol = ApplyShopLighting(pos, (Color){ 160, 165, 170, 255 });

    Color sightDot = (Color){ 90, 255, 100, 255 };



    // Slide

    DrawCube((Vector3){ 0, 0.038f, -0.04f }, 0.030f, 0.034f, 0.180f, slideCol);

    DrawCubeWires((Vector3){ 0, 0.038f, -0.04f }, 0.031f, 0.035f, 0.181f, ApplyShopLighting(pos, Fade(BLACK, 0.4f)));

    // Barrel chamber

    DrawCube((Vector3){ 0.008f, 0.040f, -0.02f }, 0.016f, 0.018f, 0.040f, metalCol);

    // Sights

    DrawCube((Vector3){ 0, 0.058f, -0.12f }, 0.008f, 0.010f, 0.012f, slideCol); // Front sight

    DrawSphere((Vector3){ 0, 0.060f, -0.12f }, 0.003f, sightDot);

    DrawCube((Vector3){ 0, 0.058f, 0.045f }, 0.020f, 0.010f, 0.012f, slideCol); // Rear sight

    // Frame & Grip

    DrawCube((Vector3){ 0, 0.018f, -0.04f }, 0.028f, 0.016f, 0.170f, frameCol); // Picatinny frame

    rlPushMatrix();

        rlTranslatef(0, -0.035f, 0.020f);

        rlRotatef(-16.0f, 1, 0, 0); // Grip rake angle

        DrawCube((Vector3){ 0, 0, 0 }, 0.027f, 0.105f, 0.048f, frameCol);

        // Stippled grip panels

        DrawCube((Vector3){ 0, 0, 0 }, 0.029f, 0.080f, 0.036f, ApplyShopLighting(pos, (Color){ 20, 20, 22, 255 }));

    rlPopMatrix();

    // Trigger guard & skeleton trigger

    DrawCylinderEx((Vector3){ 0, 0.010f, -0.010f }, (Vector3){ 0, -0.025f, -0.010f }, 0.004f, 0.004f, 6, frameCol);

    DrawCylinderEx((Vector3){ 0, -0.025f, -0.010f }, (Vector3){ 0, -0.020f, 0.022f }, 0.004f, 0.004f, 6, frameCol);

    DrawCube((Vector3){ 0, -0.008f, 0.006f }, 0.006f, 0.018f, 0.008f, metalCol);



    rlPopMatrix();

}



static void DrawGunViewModel(const Camera3D &camera, float walkTime, float bobAmplitude, float dt)

{

    Vector3 fwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwd, camera.up));

    Vector3 up  = Vector3CrossProduct(rgt, fwd);



    // Standing Idle Breathing Animation

    float t = (float)GetTime();

    float breathY = sinf(t * 1.8f) * 0.0035f;

    float breathX = cosf(t * 0.9f) * 0.0020f;

    float breathPitch = sinf(t * 1.8f) * 0.6f;

    float breathRoll  = cosf(t * 1.1f) * 0.4f;



    // Walking Idle Sway & Bobbing Animation

    float walkSwayX = sinf(walkTime * 0.5f) * 0.018f * bobAmplitude;

    float walkDipY  = (cosf(walkTime) - 0.5f) * 0.014f * bobAmplitude;

    float walkRoll  = sinf(walkTime * 0.5f) * 2.4f * bobAmplitude;

    float walkYaw   = -cosf(walkTime * 0.5f) * 1.8f * bobAmplitude;



    // Shooting Recoil & Blowback Animation

    float recNorm = Clamp(g_gunRecoilTimer / 0.16f, 0.0f, 1.0f);

    float recCurve = powf(recNorm, 0.65f);

    float recPitch = recCurve * 14.5f;

    float recBack  = recCurve * 0.034f;

    float recUp    = recCurve * 0.012f;

    float slideBlowback = (recNorm > 0.28f) ? (0.038f * (recNorm - 0.28f) / 0.72f) : 0.0f;



    // Hand origin position with inertial sway and locomotion lag

    Vector3 gunPos = Vector3Add(camera.position, Vector3Scale(fwd, 0.44f - recBack));

    gunPos = Vector3Add(gunPos, Vector3Scale(rgt, 0.17f + breathX + walkSwayX + g_vmSwayX));

    gunPos = Vector3Subtract(gunPos, Vector3Scale(up, 0.14f - (breathY + walkDipY + recUp - g_vmSwayY)));



    rlPushMatrix();

    rlTranslatef(gunPos.x, gunPos.y, gunPos.z);



    // Orientation aligned to camera look direction

    float yawDeg = atan2f(fwd.x, fwd.z) * RAD2DEG;

    float pitchDeg = asinf(-fwd.y) * RAD2DEG;

    rlRotatef(yawDeg, 0, 1, 0);

    rlRotatef(pitchDeg, 1, 0, 0);



    // Viewmodel inertial angular lag when turning

    float vmRotYaw   = g_vmSwayX * 125.0f;

    float vmRotPitch = -g_vmSwayY * 105.0f;

    float vmRotRoll  = g_vmSwayX * 85.0f;



    // Weapon Animation rotations:

    rlRotatef(walkYaw + vmRotYaw, 0, 1, 0);

    rlRotatef(-(breathPitch + recPitch + vmRotPitch), 1, 0, 0); // Muzzle kicks up with inertia

    rlRotatef(breathRoll + walkRoll + vmRotRoll, 0, 0, 1);



    Color frameCol = ApplyShopLighting(gunPos, (Color){ 28, 30, 34, 255 });

    Color slideCol = ApplyShopLighting(gunPos, (Color){ 46, 48, 52, 255 });

    Color metalCol = ApplyShopLighting(gunPos, (Color){ 175, 180, 185, 255 });

    Color sightDot = (Color){ 80, 255, 95, 255 };



    // Slide (reciprocating backward by slideBlowback)

    rlPushMatrix();

        rlTranslatef(0, 0.038f, -0.04f + slideBlowback);

        DrawCube((Vector3){ 0, 0, 0 }, 0.030f, 0.034f, 0.180f, slideCol);

        DrawCubeWires((Vector3){ 0, 0, 0 }, 0.031f, 0.035f, 0.181f, ApplyShopLighting(gunPos, Fade(BLACK, 0.45f)));



        // Rear cocking serrations

        for (int s = 0; s < 5; s++) {

            float sz = 0.055f + s * 0.007f;

            DrawLine3D((Vector3){ -0.0155f, -0.012f, sz }, (Vector3){ -0.0155f, 0.012f, sz }, ApplyShopLighting(gunPos, (Color){ 18, 18, 20, 255 }));

            DrawLine3D((Vector3){  0.0155f, -0.012f, sz }, (Vector3){  0.0155f, 0.012f, sz }, ApplyShopLighting(gunPos, (Color){ 18, 18, 20, 255 }));

        }



        // High-contrast tactical sights

        DrawCube((Vector3){ 0, 0.021f, -0.080f }, 0.007f, 0.009f, 0.012f, slideCol);

        DrawSphere((Vector3){ 0, 0.021f, -0.074f }, 0.0028f, sightDot);

        DrawCube((Vector3){ -0.008f, 0.021f, 0.082f }, 0.007f, 0.009f, 0.010f, slideCol);

        DrawCube((Vector3){  0.008f, 0.021f, 0.082f }, 0.007f, 0.009f, 0.010f, slideCol);

        DrawSphere((Vector3){ -0.008f, 0.021f, 0.082f }, 0.0025f, sightDot);

        DrawSphere((Vector3){  0.008f, 0.021f, 0.082f }, 0.0025f, sightDot);

    rlPopMatrix();



    // Fixed Barrel & Chamber (exposed when slide cycles back!)

    DrawCube((Vector3){ 0, 0.038f, -0.020f }, 0.022f, 0.022f, 0.050f, metalCol);

    DrawCylinderEx((Vector3){ 0, 0.038f, -0.020f }, (Vector3){ 0, 0.038f, -0.145f }, 0.008f, 0.008f, 10, metalCol);

    DrawCylinder((Vector3){ 0, 0.038f, -0.146f }, 0.005f, 0.005f, 0.002f, 8, BLACK);



    // Frame, Picatinny accessory rail & trigger guard

    DrawCube((Vector3){ 0, 0.018f, -0.04f }, 0.028f, 0.016f, 0.170f, frameCol);

    for (int r = 0; r < 3; r++) {

        DrawCube((Vector3){ 0, 0.008f, -0.085f - r * 0.016f }, 0.029f, 0.004f, 0.008f, ApplyShopLighting(gunPos, (Color){ 20, 20, 22, 255 }));

    }



    // Ergonomic stippled pistol grip

    rlPushMatrix();

        rlTranslatef(0, -0.035f, 0.020f);

        rlRotatef(-16.0f, 1, 0, 0);

        DrawCube((Vector3){ 0, 0, 0 }, 0.027f, 0.105f, 0.048f, frameCol);

        DrawCube((Vector3){ 0, 0, 0 }, 0.029f, 0.080f, 0.036f, ApplyShopLighting(gunPos, (Color){ 18, 18, 20, 255 }));

        DrawCube((Vector3){ 0, -0.054f, 0.004f }, 0.031f, 0.012f, 0.054f, ApplyShopLighting(gunPos, (Color){ 36, 38, 42, 255 }));

    rlPopMatrix();



    // Trigger guard and trigger

    DrawCylinderEx((Vector3){ 0, 0.010f, -0.010f }, (Vector3){ 0, -0.025f, -0.010f }, 0.004f, 0.004f, 6, frameCol);

    DrawCylinderEx((Vector3){ 0, -0.025f, -0.010f }, (Vector3){ 0, -0.020f, 0.022f }, 0.004f, 0.004f, 6, frameCol);

    DrawCube((Vector3){ 0, -0.008f, 0.006f }, 0.006f, 0.018f, 0.008f, metalCol);



    // Muzzle Flash Effect!

    if (g_gunMuzzleFlashTimer > 0.0f) {

        Vector3 mPos = { 0, 0.038f, -0.150f };

        DrawLine3D((Vector3){ mPos.x - 0.07f, mPos.y, mPos.z }, (Vector3){ mPos.x + 0.07f, mPos.y, mPos.z }, (Color){ 255, 235, 160, 255 });

        DrawLine3D((Vector3){ mPos.x, mPos.y - 0.07f, mPos.z }, (Vector3){ mPos.x, mPos.y + 0.07f, mPos.z }, (Color){ 255, 235, 160, 255 });

        DrawLine3D((Vector3){ mPos.x - 0.05f, mPos.y - 0.05f, mPos.z }, (Vector3){ mPos.x + 0.05f, mPos.y + 0.05f, mPos.z }, (Color){ 255, 200, 100, 255 });

        DrawLine3D((Vector3){ mPos.x - 0.05f, mPos.y + 0.05f, mPos.z }, (Vector3){ mPos.x + 0.05f, mPos.y - 0.05f, mPos.z }, (Color){ 255, 200, 100, 255 });

        DrawSphere(mPos, 0.032f, (Color){ 255, 255, 240, 255 });

        DrawSphere(mPos, 0.085f, (Color){ 255, 185, 45, 180 });

        DrawSphere(mPos, 0.220f, (Color){ 255, 130, 20, 75 });

    }



    rlPopMatrix();

}



static void DrawHorizontalRefrigerator(Vector3 center, const Camera3D &camera)

{

    // Center at X = 94.5, Y = 10.015, Z = 133.5

    // Footprint: Length X = 3.60m, Width Z = 1.28m, Height Y = 0.94m

    float dx = center.x - camera.position.x;

    float dy = (center.y + 0.5f) - camera.position.y;

    float dz = center.z - camera.position.z;

    float distSq = dx * dx + dy * dy + dz * dz;



    // 1. Distance culling (skip completely beyond 32 meters)

    if (distSq > 32.0f * 32.0f) return;



    // 2. Frustum culling (if > 3.0m away, skip if behind camera)

    if (distSq > 3.0f * 3.0f) {

        Vector3 fwdDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

        Vector3 toObj = Vector3Normalize((Vector3){ dx, dy, dz });

        if (Vector3DotProduct(fwdDir, toObj) < -0.25f) return;

    }



    // 1. Recessed dark matte compressor kickplate base

    Vector3 basePos = { center.x, 10.09f, center.z };

    Color kickplateCol = ApplyShopLighting(basePos, (Color){ 26, 28, 32, 255 });

    DrawCube(basePos, 3.44f, 0.15f, 1.16f, kickplateCol);



    // 2. Commercial Appliance White Powder-Coated Chest Body

    Vector3 bodyPos = { center.x, 10.53f, center.z };

    Color whiteEnamel = ApplyShopLighting(bodyPos, (Color){ 238, 240, 244, 255 });

    DrawCube(bodyPos, 3.60f, 0.72f, 1.26f, whiteEnamel);



    // Heavy-duty perimeter cart bumper rubber rail at mid-height

    Vector3 bumperPos = { center.x, 10.52f, center.z };

    Color bumperRubber = ApplyShopLighting(bumperPos, (Color){ 36, 38, 42, 255 });

    DrawCube(bumperPos, 3.65f, 0.065f, 1.31f, bumperRubber);



    // 3. Top Deck Stainless Steel Trim Frame (perimeter lip)

    Color stainless = ApplyShopLighting((Vector3){ center.x, 10.90f, center.z }, (Color){ 215, 220, 226, 255 });

    DrawCube((Vector3){ center.x, 10.90f, center.z - 0.58f }, 3.62f, 0.035f, 0.11f, stainless);

    DrawCube((Vector3){ center.x, 10.90f, center.z + 0.58f }, 3.62f, 0.035f, 0.11f, stainless);

    DrawCube((Vector3){ center.x - 1.76f, 10.90f, center.z }, 0.12f, 0.035f, 1.28f, stainless);

    DrawCube((Vector3){ center.x + 1.76f, 10.90f, center.z }, 0.12f, 0.035f, 1.28f, stainless);



    // 4. Interior Refrigerated Compartment & Cold Arctic LED Glow

    Color ledStripCol = (Color){ 175, 240, 255, 255 };

    DrawLine3D((Vector3){ center.x - 1.66f, 10.87f, center.z - 0.50f }, (Vector3){ center.x + 1.66f, 10.87f, center.z - 0.50f }, ledStripCol);

    DrawLine3D((Vector3){ center.x - 1.66f, 10.87f, center.z + 0.50f }, (Vector3){ center.x + 1.66f, 10.87f, center.z + 0.50f }, ledStripCol);

    DrawCube((Vector3){ center.x, 10.60f, center.z }, 3.32f, 0.44f, 0.98f, (Color){ 160, 230, 255, 28 });



    // Dual Sliding Glass Lids (Translucent Cyan-Tinted Tempered Glass)

    Color glassTint = (Color){ 190, 235, 255, 68 };

    DrawCube((Vector3){ center.x, 10.925f, center.z }, 3.44f, 0.012f, 1.06f, glassTint);



    // DISTANT LOD: If camera is more than 6.5m away, simplified clean model is sufficient

    if (distSq > 6.5f * 6.5f) return;



    // HIGH DETAIL LOD (within 6.5m):

    // Compressor ventilation louvers

    Color louverSlot = ApplyShopLighting(basePos, (Color){ 12, 14, 16, 255 });

    for (int v = 0; v < 3; v++) {

        float vy = 10.06f + v * 0.030f;

        DrawCube((Vector3){ center.x - 0.90f, vy, center.z - 0.582f }, 0.85f, 0.012f, 0.015f, louverSlot);

        DrawCube((Vector3){ center.x + 0.90f, vy, center.z - 0.582f }, 0.85f, 0.012f, 0.015f, louverSlot);

    }



    // 4 Corner Protector Moldings

    float halfX = 1.80f, halfZ = 0.63f;

    Vector3 cCorners[4] = {

        { center.x - halfX, 10.53f, center.z - halfZ },

        { center.x + halfX, 10.53f, center.z - halfZ },

        { center.x - halfX, 10.53f, center.z + halfZ },

        { center.x + halfX, 10.53f, center.z + halfZ }

    };

    Color cornerCol = ApplyShopLighting(bodyPos, (Color){ 55, 58, 65, 255 });

    for (int c = 0; c < 4; c++) {

        DrawCylinder(cCorners[c], 0.025f, 0.025f, 0.72f, 8, cornerCol);

    }



    // Chrome Wire Divider Baskets

    Color wireCol = ApplyShopLighting((Vector3){ center.x, 10.60f, center.z }, (Color){ 200, 205, 215, 255 });

    const float dividersX[3] = { center.x - 0.85f, center.x, center.x + 0.85f };

    for (int d = 0; d < 3; d++) {

        DrawCube((Vector3){ dividersX[d], 10.60f, center.z }, 0.015f, 0.45f, 1.02f, Fade(wireCol, 0.65f));

        DrawCylinderEx((Vector3){ dividersX[d], 10.82f, center.z - 0.51f }, (Vector3){ dividersX[d], 10.82f, center.z + 0.51f }, 0.007f, 0.007f, 6, wireCol);

    }



    // Aluminum Handles & Diagonal Glass Highlights

    Color handleAlum = ApplyShopLighting((Vector3){ center.x, 10.93f, center.z }, (Color){ 220, 225, 232, 255 });

    DrawCube((Vector3){ center.x - 0.86f, 10.932f, center.z - 0.49f }, 1.50f, 0.018f, 0.035f, handleAlum);

    DrawCube((Vector3){ center.x + 0.86f, 10.944f, center.z - 0.49f }, 1.50f, 0.018f, 0.035f, handleAlum);

    DrawLine3D((Vector3){ center.x - 1.56f, 10.926f, center.z - 0.35f }, (Vector3){ center.x - 0.31f, 10.926f, center.z + 0.35f }, (Color){ 255, 255, 255, 110 });

    DrawLine3D((Vector3){ center.x + 0.16f, 10.938f, center.z - 0.35f }, (Vector3){ center.x + 1.41f, 10.938f, center.z + 0.35f }, (Color){ 255, 255, 255, 110 });



    // Digital Microprocessor Temperature Display (Front South Face, Z = 132.86)

    Vector3 dispPos = { center.x, 10.66f, center.z - 0.635f };

    DrawCube(dispPos, 0.28f, 0.09f, 0.015f, (Color){ 14, 16, 18, 255 });



    // Digital readout: "-18°C" in glowing arctic cyan

    Color ledText = (Color){ 50, 240, 255, 255 };

    float textZ = dispPos.z - 0.010f;

    DrawLine3D((Vector3){ center.x - 0.095f, 10.66f, textZ }, (Vector3){ center.x - 0.075f, 10.66f, textZ }, ledText);

    DrawLine3D((Vector3){ center.x - 0.055f, 10.635f, textZ }, (Vector3){ center.x - 0.055f, 10.685f, textZ }, ledText);

    DrawCube((Vector3){ center.x - 0.025f, 10.66f, textZ }, 0.024f, 0.050f, 0.002f, ledText);

    DrawCube((Vector3){ center.x + 0.005f, 10.680f, textZ }, 0.008f, 0.008f, 0.002f, ledText);

    DrawCube((Vector3){ center.x + 0.032f, 10.66f, textZ }, 0.024f, 0.050f, 0.002f, ledText);

    // Green operational status LED dot

    DrawSphere((Vector3){ center.x + 0.095f, 10.66f, textZ }, 0.005f, (Color){ 45, 255, 95, 255 });

}



static void DrawShopProductsAndParticles(const Camera3D &camera, float walkTime = 0.0f, float bobAmplitude = 0.0f, float dt = 0.016f)

{

    float wobble = sinf((float)GetTime() * 6.0f) * 0.004f;

    Vector3 fwdDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));



    bool canSeeShopInterior = (camera.position.x <= 110.0f) || 

        (camera.position.x <= 116.0f && camera.position.z >= 135.0f && camera.position.z <= 145.0f);



    for (size_t i = 0; i < g_shopProducts.size(); i++) {

        const ShopProduct &it = g_shopProducts[i];

        bool isHeld = ((int)i == g_heldProductIndex);



        if (!isHeld && it.type != PROD_CART && !canSeeShopInterior) continue;



        float dx = it.homePos.x - camera.position.x;

        float dy = it.homePos.y - camera.position.y;

        float dz = it.homePos.z - camera.position.z;

        float distSq = dx * dx + dy * dy + dz * dz;



        if (!isHeld) {

            if (it.type == PROD_CART) {

                // Cart visible up to 55m

                if (distSq > 55.0f * 55.0f) continue;

                if (distSq > 4.0f * 4.0f) {

                    Vector3 toCart = Vector3Normalize((Vector3){ dx, dy, dz });

                    if (Vector3DotProduct(fwdDir, toCart) < -0.35f) continue;

                }

            } else {

                // Shelf backplate occlusion culling:

                if (fabsf(it.originalPos.z - 140.0f) < 1.0f) {

                    if (it.homePos.z > 140.0f && camera.position.z < 140.0f) continue;

                    if (it.homePos.z < 140.0f && camera.position.z > 140.0f) continue;

                } else if (fabsf(it.originalPos.z - 147.0f) < 1.0f) {

                    if (it.homePos.z > 147.0f && camera.position.z < 147.0f) continue;

                    if (it.homePos.z < 147.0f && camera.position.z > 147.0f) continue;

                }



                // Tight distance culling for shelf items: 11 meters

                if (distSq > 11.0f * 11.0f) continue;



                // Camera frustum culling

                if (distSq > 1.2f * 1.2f) {

                    Vector3 toProd = Vector3Normalize((Vector3){ dx, dy, dz });

                    if (Vector3DotProduct(fwdDir, toProd) < 0.35f) continue;

                }

            }

        }



        switch (it.type) {

            case PROD_TIN:

                DrawPopcornTin(it.homePos, it.opened, wobble, isHeld, distSq);

                break;

            case PROD_MILK:

                DrawBottle(it.homePos, 0.34f, 0.055f, g_milkLiquidModel, it.fill, (Color){ 200, 230, 255, 255 }, it.opened, isHeld, fwdDir, distSq);

                break;

            case PROD_BLOOD:

                DrawBottle(it.homePos, 0.34f, 0.055f, g_bloodLiquidModel, it.fill, (Color){ 180, 200, 190, 255 }, it.opened, isHeld, fwdDir, distSq);

                break;

            case PROD_BREAD:

                DrawBread(it.homePos, g_breadModel, isHeld, distSq);

                break;

            case PROD_CART:

                rlPushMatrix();

                    rlTranslatef(g_cartPos.x, g_cartPos.y, g_cartPos.z);

                    rlRotatef(g_cartYaw, 0, 1, 0);

                    DrawShoppingCartLocal(g_cartWheelSpin, g_cartPos, distSq);

                rlPopMatrix();

                break;

            case PROD_CHOCOLATE:

                DrawChocolateBar(it.homePos, it.subType, it.opened, isHeld, fwdDir, distSq);

                break;

            case PROD_GUN:

                if (!isHeld) {

                    DrawGunWorld(it.homePos, false);

                } else {

                    DrawGunViewModel(camera, walkTime, bobAmplitude, dt);

                }

                break;

        }

    }



    if (canSeeShopInterior) {

        for (const auto &p : g_shopParticles) {

            DrawShopParticle(p);

        }

        DrawDustParticles(camera);

        DrawFootstepTrails();

        DrawShellCasingsAndSparks();

    }

}



static int GetCrosshairFocusedProduct(const Camera3D &camera, float maxReach = 2.8f, bool allowCart = false)

{

    Vector3 rayOrigin = camera.position;

    Vector3 rayDir = Vector3Normalize(Vector3Subtract(camera.target, camera.position));



    int bestIdx = -1;

    float bestScore = 1e9f;



    for (size_t i = 0; i < g_shopProducts.size(); i++) {

        const ShopProduct &p = g_shopProducts[i];

        if (p.held) continue;

        if (p.type == PROD_CART && !allowCart) continue;



        // Divider wall occlusion check for double-sided racks:

        // If an item is on the opposite face of the rack divider wall relative to player, skip!

        if (p.homePos.x >= 88.8f && p.homePos.x <= 101.2f) {

            if (fabsf(p.originalPos.z - 140.0f) < 1.0f) {

                if (p.homePos.z > 140.0f && rayOrigin.z < 140.0f) continue;

                if (p.homePos.z < 140.0f && rayOrigin.z > 140.0f) continue;

            } else if (fabsf(p.originalPos.z - 147.0f) < 1.0f) {

                if (p.homePos.z > 147.0f && rayOrigin.z < 147.0f) continue;

                if (p.homePos.z < 147.0f && rayOrigin.z > 147.0f) continue;

            }

        }



        Vector3 v = Vector3Subtract(p.homePos, rayOrigin);

        float t = Vector3DotProduct(v, rayDir);

        if (t <= 0.2f || t > maxReach) continue;



        // Closest point on ray to product center

        Vector3 rayPt = Vector3Add(rayOrigin, Vector3Scale(rayDir, t));

        float perpDist = Vector3Distance(p.homePos, rayPt);



        // Shopping cart has a larger bounding reach than small shelf bottles

        float maxPerp = (p.type == PROD_CART) ? 0.55f : 0.24f;

        if (perpDist <= maxPerp) {

            float score = perpDist * 2.0f + t * 0.08f;

            if (score < bestScore) {

                bestScore = score;

                bestIdx = (int)i;

            }

        }

    }

    return bestIdx;

}



static void ToggleGameFullscreen() {

    int mon = GetCurrentMonitor();

    int monW = GetMonitorWidth(mon);

    int monH = GetMonitorHeight(mon);

    if (!IsWindowFullscreen()) {

        SetWindowSize(monW, monH);

        ToggleFullscreen();

    } else {

        ToggleFullscreen();

        SetWindowSize(LOGICAL_W, LOGICAL_H);

        SetWindowPosition((monW - LOGICAL_W) / 2, (monH - LOGICAL_H) / 2);

    }

}



// ==================================================================

// HYPER-REALISTIC SHOVEL SYSTEM (Custom Curved Spade Mesh, Lit Shader, Rig & Physics)

// ==================================================================

static const char *VS_SHOVEL = R"(

#version 330

in vec3 vertexPosition;

in vec3 vertexNormal;

in vec2 vertexTexCoord;

uniform mat4 mvp;

uniform mat4 matModel;

out vec3 fragNormal;

out vec3 fragPos;

out vec2 fragTexCoord;

void main() {

    fragPos = vec3(matModel * vec4(vertexPosition, 1.0));

    fragNormal = normalize(mat3(matModel) * vertexNormal);

    fragTexCoord = vertexTexCoord;

    gl_Position = mvp * vec4(vertexPosition, 1.0);

}

)";



static const char *FS_SHOVEL = R"(

#version 330

in vec3 fragNormal;

in vec3 fragPos;

in vec2 fragTexCoord;

out vec4 finalColor;



uniform vec3 lightDir;

uniform vec3 viewPos;

uniform vec4 baseColor;

uniform float roughness;

uniform float grainAmount;

uniform float metallic;



float hash(vec2 p) { return fract(sin(dot(p, vec2(41.3, 289.1))) * 43758.5453); }



void main() {

    vec3 N = normalize(fragNormal);

    vec3 L = normalize(-lightDir);

    vec3 V = normalize(viewPos - fragPos);

    vec3 H = normalize(L + V);



    float ambient = 0.32;

    float diff = max(dot(N, L), 0.0);

    float shininess = mix(8.0, 128.0, 1.0 - roughness);

    float spec = pow(max(dot(N, H), 0.0), shininess) * mix(0.15, 1.0, metallic);



    float rim = pow(1.0 - max(dot(N, V), 0.0), 2.5) * 0.35;



    vec3 color = baseColor.rgb;



    if (grainAmount > 0.0) {

        float grain = hash(vec2(fragTexCoord.y * 60.0, floor(fragTexCoord.x * 8.0)));

        float stripes = sin(fragTexCoord.y * 90.0 + grain * 6.0) * 0.5 + 0.5;

        color = mix(color, color * 0.75, stripes * grainAmount);

    }



    vec3 lit = color * (ambient + diff * 0.85) + vec3(spec) + vec3(rim) * 0.6;

    finalColor = vec4(lit, baseColor.a);

}

)";



static Mesh GenMeshShovelBlade(int segments, float zStart, float length, float maxWidth, float curveDepth)

{

    int cols = 6;

    int rows = segments + 1;

    int vertCount = rows * cols;

    int triCount = segments * (cols - 1) * 4;



    Mesh mesh = { 0 };

    mesh.vertexCount = vertCount;

    mesh.triangleCount = triCount;

    mesh.vertices  = (float *)MemAlloc(vertCount * 3 * sizeof(float));

    mesh.normals   = (float *)MemAlloc(vertCount * 3 * sizeof(float));

    mesh.texcoords = (float *)MemAlloc(vertCount * 2 * sizeof(float));

    mesh.indices   = (unsigned short *)MemAlloc(triCount * 3 * sizeof(unsigned short));



    for (int r = 0; r < rows; r++) {

        float t = (float)r / (float)segments;

        // Blade tapers cleanly towards the pointed spade tip

        float width = maxWidth * powf(1.0f - t, 0.60f);

        float z = zStart + t * length;

        // Scoop dish curve along the spine

        float scoopLong = curveDepth * sinf(PI * t);



        for (int c = 0; c < cols; c++) {

            float s = (float)c / (float)(cols - 1);

            float x = (s - 0.5f) * 2.0f * width;

            float crossCurve = curveDepth * 0.45f * (1.0f - powf(2.0f * s - 1.0f, 2.0f));

            float y = scoopLong - crossCurve;



            int idx = r * cols + c;

            mesh.vertices[idx*3+0] = x;

            mesh.vertices[idx*3+1] = y;

            mesh.vertices[idx*3+2] = z;

            mesh.texcoords[idx*2+0] = s;

            mesh.texcoords[idx*2+1] = t;

        }

    }



    int ii = 0;

    for (int r = 0; r < segments; r++) {

        for (int c = 0; c < cols - 1; c++) {

            int a = r * cols + c;

            int b = a + 1;

            int cN = (r + 1) * cols + c;

            int d = cN + 1;

            // Front face

            mesh.indices[ii++] = (unsigned short)a; mesh.indices[ii++] = (unsigned short)cN; mesh.indices[ii++] = (unsigned short)b;

            mesh.indices[ii++] = (unsigned short)b; mesh.indices[ii++] = (unsigned short)cN; mesh.indices[ii++] = (unsigned short)d;

            // Back face

            mesh.indices[ii++] = (unsigned short)a; mesh.indices[ii++] = (unsigned short)b; mesh.indices[ii++] = (unsigned short)cN;

            mesh.indices[ii++] = (unsigned short)b; mesh.indices[ii++] = (unsigned short)d; mesh.indices[ii++] = (unsigned short)cN;

        }

    }



    for (int i = 0; i < vertCount * 3; i++) mesh.normals[i] = 0.0f;

    for (int i = 0; i < triCount; i++) {

        int i0 = mesh.indices[i*3+0], i1 = mesh.indices[i*3+1], i2 = mesh.indices[i*3+2];

        Vector3 v0 = { mesh.vertices[i0*3+0], mesh.vertices[i0*3+1], mesh.vertices[i0*3+2] };

        Vector3 v1 = { mesh.vertices[i1*3+0], mesh.vertices[i1*3+1], mesh.vertices[i1*3+2] };

        Vector3 v2 = { mesh.vertices[i2*3+0], mesh.vertices[i2*3+1], mesh.vertices[i2*3+2] };

        Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(v1, v0), Vector3Subtract(v2, v0)));

        int idxs[3] = { i0, i1, i2 };

        for (int k = 0; k < 3; k++) {

            mesh.normals[idxs[k]*3+0] += n.x;

            mesh.normals[idxs[k]*3+1] += n.y;

            mesh.normals[idxs[k]*3+2] += n.z;

        }

    }

    for (int v = 0; v < vertCount; v++) {

        Vector3 n = { mesh.normals[v*3+0], mesh.normals[v*3+1], mesh.normals[v*3+2] };

        float len = Vector3Length(n);

        if (len > 0.0001f) n = Vector3Scale(n, 1.0f / len);

        else n = (Vector3){ 0.0f, 1.0f, 0.0f };

        mesh.normals[v*3+0] = n.x; mesh.normals[v*3+1] = n.y; mesh.normals[v*3+2] = n.z;

    }



    UploadMesh(&mesh, false);

    return mesh;

}



struct ShovelPart {

    Model model;

    Matrix localOffset;

};



struct Shovel {

    ShovelPart pommel;

    ShovelPart grip;

    ShovelPart shaft;

    ShovelPart collar;

    ShovelPart tabL;

    ShovelPart tabR;

    ShovelPart blade;

    Shader shader;

    int locLightDir, locViewPos, locBaseColor, locRough, locGrain, locMetal;

};



static float g_shovelTipLocalZ = 0.98f;



static void SetPartUniforms(Shovel &sv, Color baseColor, float roughness, float grain, float metallic)

{

    Vector4 c = ColorNormalize(baseColor);

    SetShaderValue(sv.shader, sv.locBaseColor, &c, SHADER_UNIFORM_VEC4);

    SetShaderValue(sv.shader, sv.locRough, &roughness, SHADER_UNIFORM_FLOAT);

    SetShaderValue(sv.shader, sv.locGrain, &grain, SHADER_UNIFORM_FLOAT);

    SetShaderValue(sv.shader, sv.locMetal, &metallic, SHADER_UNIFORM_FLOAT);

}



static Shovel BuildShovel()

{

    Shovel sv = { 0 };

    sv.shader = LoadShaderFromMemory(VS_SHOVEL, FS_SHOVEL);

    sv.shader.locs[SHADER_LOC_MATRIX_MVP]   = GetShaderLocation(sv.shader, "mvp");

    sv.shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(sv.shader, "matModel");

    sv.locLightDir  = GetShaderLocation(sv.shader, "lightDir");

    sv.locViewPos   = GetShaderLocation(sv.shader, "viewPos");

    sv.locBaseColor = GetShaderLocation(sv.shader, "baseColor");

    sv.locRough     = GetShaderLocation(sv.shader, "roughness");

    sv.locGrain     = GetShaderLocation(sv.shader, "grainAmount");

    sv.locMetal     = GetShaderLocation(sv.shader, "metallic");



    // UNIFIED FORWARD-ORIENTED SHOVEL ASSEMBLY (Origin Z=0.0 at Hand Grip, ZERO GAPS):

    // 1. Pommel End Cap: Sphere at Z = -0.04m (radius 0.026m)

    // 2. Grip Sleeve:    Z = -0.04m to Z = +0.18m (radius 0.024m)

    // 3. Ash Wood Shaft: Z =  0.00m to Z = +0.68m (radius 0.018m, 18cm overlap with grip)

    // 4. Steel Collar:   Z = +0.62m to Z = +0.74m (radius 0.025m, 6cm overlap with shaft)

    // 5. Foot Tabs:      Z = +0.73m at blade shoulders

    // 6. Curved Blade:   Z = +0.70m to Z = +0.98m (tip at 0.98m, 4cm overlap with collar)



    float gripLen   = 0.22f;

    float shaftLen  = 0.68f;

    float collarLen = 0.12f;

    float bladeLen  = 0.28f;

    float bladeWidth= 0.20f;

    float bladeCurve= 0.038f;



    // 1. Pommel: Steel spherical end cap at handle butt

    Mesh pommelMesh = GenMeshSphere(0.026f, 12, 12);

    sv.pommel.model = LoadModelFromMesh(pommelMesh);

    sv.pommel.model.materials[0].shader = sv.shader;

    sv.pommel.localOffset = MatrixTranslate(0, 0, -0.04f);



    // 2. Grip Sleeve: Heavy textured rubber grip sleeve

    Mesh gripMesh = GenMeshCylinder(0.024f, gripLen, 14);

    sv.grip.model = LoadModelFromMesh(gripMesh);

    sv.grip.model.materials[0].shader = sv.shader;

    sv.grip.localOffset = MatrixMultiply(MatrixRotateX(PI/2.0f), MatrixTranslate(0, 0, -0.04f));



    // 3. Handle Shaft: Weathered ash wood shaft

    Mesh shaftMesh = GenMeshCylinder(0.018f, shaftLen, 12);

    sv.shaft.model = LoadModelFromMesh(shaftMesh);

    sv.shaft.model.materials[0].shader = sv.shader;

    sv.shaft.localOffset = MatrixMultiply(MatrixRotateX(PI/2.0f), MatrixTranslate(0, 0, 0.00f));



    // 4. Socket Collar: Gunmetal steel socket clamping blade to shaft

    Mesh collarMesh = GenMeshCylinder(0.025f, collarLen, 12);

    sv.collar.model = LoadModelFromMesh(collarMesh);

    sv.collar.model.materials[0].shader = sv.shader;

    sv.collar.localOffset = MatrixMultiply(MatrixRotateX(PI/2.0f), MatrixTranslate(0, 0, 0.62f));



    // 5. Left & Right Foot Tabs: Welded steel steps at collar shoulders

    Mesh tabMeshL = GenMeshCube(0.075f, 0.012f, 0.038f);

    sv.tabL.model = LoadModelFromMesh(tabMeshL);

    sv.tabL.model.materials[0].shader = sv.shader;

    sv.tabL.localOffset = MatrixTranslate(-0.088f, 0.008f, 0.728f);



    Mesh tabMeshR = GenMeshCube(0.075f, 0.012f, 0.038f);

    sv.tabR.model = LoadModelFromMesh(tabMeshR);

    sv.tabR.model.materials[0].shader = sv.shader;

    sv.tabR.localOffset = MatrixTranslate( 0.088f, 0.008f, 0.728f);



    // 6. Blade: Curved tool steel spade starting at Z=0.70m, tip at Z=0.98m

    Mesh bladeMesh = GenMeshShovelBlade(10, 0.70f, bladeLen, bladeWidth, bladeCurve);

    sv.blade.model = LoadModelFromMesh(bladeMesh);

    sv.blade.model.materials[0].shader = sv.shader;

    sv.blade.localOffset = MatrixIdentity();

    g_shovelTipLocalZ = 0.70f + bladeLen;



    return sv;

}



static void UnloadShovel(Shovel &sv)

{

    UnloadModel(sv.pommel.model);

    UnloadModel(sv.grip.model);

    UnloadModel(sv.shaft.model);

    UnloadModel(sv.collar.model);

    UnloadModel(sv.tabL.model);

    UnloadModel(sv.tabR.model);

    UnloadModel(sv.blade.model);

    UnloadShader(sv.shader);

}



static void DrawShovel(Shovel &sv, Matrix rootTransform, Vector3 viewPos, Vector3 lightDir)

{

    SetShaderValue(sv.shader, sv.locViewPos, &viewPos, SHADER_UNIFORM_VEC3);

    SetShaderValue(sv.shader, sv.locLightDir, &lightDir, SHADER_UNIFORM_VEC3);



    ShovelPart *parts[7] = { &sv.pommel, &sv.grip, &sv.shaft, &sv.collar, &sv.tabL, &sv.tabR, &sv.blade };

    Color colors[7]   = {

        (Color){ 88, 90, 96, 255 },     // pommel: steel

        (Color){ 34, 34, 36, 255 },     // grip: dark rubber

        (Color){ 145, 102, 60, 255 },   // shaft: warm ash wood

        (Color){ 160, 162, 168, 255 },  // collar: stamped steel

        (Color){ 145, 148, 154, 255 },  // tabL: steel step

        (Color){ 145, 148, 154, 255 },  // tabR: steel step

        (Color){ 195, 198, 204, 255 }   // blade: tool steel

    };

    float roughness[7]= { 0.45f, 0.90f, 0.72f, 0.30f, 0.35f, 0.35f, 0.22f };

    float grain[7]    = { 0.0f,  0.0f,  0.70f, 0.0f,  0.0f,  0.0f,  0.0f  };

    float metallic[7] = { 0.80f, 0.0f,  0.0f,  0.85f, 0.85f, 0.85f, 0.92f };



    for (int i = 0; i < 7; i++) {

        SetPartUniforms(sv, colors[i], roughness[i], grain[i], metallic[i]);

        Matrix world = MatrixMultiply(parts[i]->localOffset, rootTransform);

        parts[i]->model.transform = world;

        DrawModel(parts[i]->model, (Vector3){0,0,0}, 1.0f, WHITE);

    }

}



// Keyframe animation system

struct ShovelPose { Vector3 pos; Vector3 rot; };

struct ShovelKeyframe { float t; ShovelPose pose; };



static ShovelPose LerpShovelPose(ShovelPose a, ShovelPose b, float f)

{

    float sf = f*f*(3.0f - 2.0f*f);

    ShovelPose r;

    r.pos = Vector3Lerp(a.pos, b.pos, sf);

    r.rot = Vector3Lerp(a.rot, b.rot, sf);

    return r;

}



static ShovelPose SampleShovelKeyframes(const ShovelKeyframe *kf, int count, float t)

{

    if (t <= kf[0].t) return kf[0].pose;

    if (t >= kf[count-1].t) return kf[count-1].pose;

    for (int i = 0; i < count - 1; i++) {

        if (t >= kf[i].t && t <= kf[i+1].t) {

            float span = kf[i+1].t - kf[i].t;

            float f = (span > 0.0001f) ? (t - kf[i].t) / span : 0.0f;

            return LerpShovelPose(kf[i].pose, kf[i+1].pose, f);

        }

    }

    return kf[count-1].pose;

}



enum ShovelAnimState { SHOVEL_ANIM_IDLE, SHOVEL_ANIM_DIG, SHOVEL_ANIM_ATTACK };



// Shovel Ready / Idle Pose: Tucked low-right, pointing naturally forward-down toward the earth

// Pitch=22 deg, Yaw=-12 deg, Roll=8 deg: blade rests visible in lower right, crosshair unblocked

static const ShovelPose SHOVEL_HOLD_POSE = { {0.0f, 0.0f, 0.0f}, {-8.0f, -14.0f, 10.0f} };

// Authentic "Digging From Bottom" Motion:
// 1. (0.00-0.12s) Raise & brace back for downward plunge
// 2. (0.12-0.24s) Forceful downward plunge driving blade into the soil (Impact + camera kick)
// 3. (0.24-0.46s) Hands pull BACK and DOWN on handle to LEVER / PRY the dirt clump loose from underneath
// 4. (0.46-0.70s) Upward scoop and FLING dirt forward in an arc (dirt clod shower)
// 5. (0.70-1.00s) Settle back into upward-tilted ready stance
static const ShovelKeyframe SHOVEL_DIG_KF[] = {
    { 0.00f, { { 0.00f,  0.00f,  0.00f}, { -8.0f, -14.0f,  10.0f} } },
    { 0.12f, { { 0.02f,  0.06f, -0.06f}, {-14.0f,  -8.0f,  14.0f} } }, // Raise & brace
    { 0.24f, { {-0.04f, -0.26f,  0.20f}, { 46.0f,  -6.0f,   4.0f} } }, // Plunge into earth (IMPACT!)
    { 0.46f, { {-0.02f, -0.16f,  0.04f}, {  6.0f, -14.0f,  16.0f} } }, // Levering / prying soil from bottom
    { 0.70f, { {-0.06f,  0.12f,  0.10f}, {-18.0f,  -2.0f, -12.0f} } }, // Scooping & flinging dirt forward!
    { 0.86f, { { 0.01f, -0.04f,  0.02f}, { -2.0f, -12.0f,   8.0f} } }, // Settling
    { 1.00f, { { 0.00f,  0.00f,  0.00f}, { -8.0f, -14.0f,  10.0f} } }, // Return to upward-tilted stance
};

// Attack Swing: Fast horizontal clearing sweep across ground / lower screen
static const ShovelKeyframe SHOVEL_ATTACK_KF[] = {
    { 0.00f, { { 0.00f,  0.00f,  0.00f}, { -8.0f, -14.0f,  10.0f} } },
    { 0.14f, { { 0.10f,  0.05f, -0.10f}, {  6.0f, -36.0f,  24.0f} } }, // Windup back right
    { 0.28f, { {-0.14f, -0.08f,  0.16f}, { 24.0f,  38.0f, -28.0f} } }, // Swift slash through center
    { 0.40f, { {-0.06f, -0.04f,  0.06f}, {  2.0f,  12.0f,  -6.0f} } }, // Follow-through
    { 0.50f, { { 0.00f,  0.00f,  0.00f}, { -8.0f, -14.0f,  10.0f} } }, // Return to upward-tilted stance
};

static const float SHOVEL_DIG_DURATION = 0.95f;

static const float SHOVEL_ATTACK_DURATION = 0.50f;

static const float SHOVEL_DIG_IMPACT_T = 0.24f;

static const float SHOVEL_DIG_THROW_T  = 0.70f;

static const float SHOVEL_ATTACK_IMPACT_T = 0.28f;



static ShovelPose GetAnimatedShovelPose(ShovelAnimState state, float stateTime, float idleClock)

{

    if (state == SHOVEL_ANIM_DIG) {

        float t = Clamp(stateTime / SHOVEL_DIG_DURATION, 0.0f, 1.0f);

        return SampleShovelKeyframes(SHOVEL_DIG_KF, sizeof(SHOVEL_DIG_KF)/sizeof(ShovelKeyframe), t);

    }

    if (state == SHOVEL_ANIM_ATTACK) {

        float t = Clamp(stateTime / SHOVEL_ATTACK_DURATION, 0.0f, 1.0f);

        return SampleShovelKeyframes(SHOVEL_ATTACK_KF, sizeof(SHOVEL_ATTACK_KF)/sizeof(ShovelKeyframe), t);

    }

    ShovelPose p = SHOVEL_HOLD_POSE;

    p.pos.y += sinf(idleClock * 1.4f) * 0.006f;

    p.rot.z += sinf(idleClock * 1.0f) * 1.2f;

    p.rot.x += sinf(idleClock * 0.7f) * 0.6f;

    return p;

}



static Matrix ShovelPoseToWorldMatrix(ShovelPose pose, Vector3 basePos, Vector3 right, Vector3 up, Vector3 fwd)

{

    Matrix rot = MatrixRotateXYZ((Vector3){ DEG2RAD*pose.rot.x, DEG2RAD*pose.rot.y, DEG2RAD*pose.rot.z });

    Vector3 offset = Vector3Add(Vector3Add(Vector3Scale(right, pose.pos.x), Vector3Scale(up, pose.pos.y)),

                                 Vector3Scale(fwd, pose.pos.z));

    Vector3 finalPos = Vector3Add(basePos, offset);



    Matrix basis = { right.x, up.x, fwd.x, 0,

                      right.y, up.y, fwd.y, 0,

                      right.z, up.z, fwd.z, 0,

                      0,0,0,1 };

    Matrix world = MatrixMultiply(rot, basis);

    world.m12 = finalPos.x; world.m13 = finalPos.y; world.m14 = finalPos.z;

    return world;

}



// Physically simulated flying dirt particles

struct DirtClod {

    Vector3 pos, vel;

    float life;

    bool active;

};

static const int MAX_DIRT_CLODS = 60;

static DirtClod g_dirtClods[MAX_DIRT_CLODS];



static void SpawnDirtClod(Vector3 origin, Vector3 dir)

{

    for (int i = 0; i < MAX_DIRT_CLODS; i++) {

        if (!g_dirtClods[i].active) {

            g_dirtClods[i].active = true;

            g_dirtClods[i].pos = origin;

            float spread = 1.2f;

            g_dirtClods[i].vel = Vector3Add(Vector3Scale(dir, 2.2f + (float)GetRandomValue(0, 150)/100.0f),

                (Vector3){ (float)GetRandomValue(-100, 100)/100.0f * spread, 1.6f + (float)GetRandomValue(0, 150)/100.0f, (float)GetRandomValue(-100, 100)/100.0f * spread });

            g_dirtClods[i].life = 1.4f;

            return;

        }

    }

}



static float GetTerrainGroundHeight(float x, float z) {

    if (x <= 84.0f && x >= 62.0f && z >= 136.0f && z <= 144.0f) {

        return 1.2f + ((x - 62.0f) / 22.0f) * 8.8f;

    }

    if (x < 62.0f && x >= 32.0f && z >= 136.0f && z <= 144.0f) {

        return -16.0f + ((x - 32.0f) / 30.0f) * 17.2f;

    }

    if (x < 32.0f && z >= 105.0f && z <= 175.0f) {

        return -16.0f;

    }

    return 10.0f;

}



static void UpdateDirtClods(float dt)

{

    for (int i = 0; i < MAX_DIRT_CLODS; i++) {

        if (!g_dirtClods[i].active) continue;

        g_dirtClods[i].vel.y += -9.8f * dt;

        g_dirtClods[i].pos = Vector3Add(g_dirtClods[i].pos, Vector3Scale(g_dirtClods[i].vel, dt));



        float gY = GetTerrainGroundHeight(g_dirtClods[i].pos.x, g_dirtClods[i].pos.z);

        if (g_dirtClods[i].pos.y <= gY + 0.03f) {

            g_dirtClods[i].pos.y = gY + 0.03f;

            g_dirtClods[i].vel.y *= -0.35f;

            g_dirtClods[i].vel.x *= 0.6f;

            g_dirtClods[i].vel.z *= 0.6f;

            if (fabsf(g_dirtClods[i].vel.y) < 0.4f) g_dirtClods[i].vel.y = 0;

        }



        g_dirtClods[i].life -= dt;

        if (g_dirtClods[i].life <= 0.0f) g_dirtClods[i].active = false;

    }

}



static void DrawDirtClods()

{

    for (int i = 0; i < MAX_DIRT_CLODS; i++) {

        if (!g_dirtClods[i].active) continue;

        float a = Clamp(g_dirtClods[i].life / 1.4f, 0.0f, 1.0f);

        Color c = (Color){ 68, 48, 28, (unsigned char)(255 * a) };

        DrawCube(g_dirtClods[i].pos, 0.06f, 0.06f, 0.06f, c);

    }

}



#include "systems/footprint_decals.h"
static float g_collegeFlickerTimer = 0.0f;
static float g_collegeCreakTimer = 12.0f;
static bool g_collegeLightOn = true;

// =========================================================================
// ABANDONED BLACKWOOD COLLEGE: ADVANCED DYNAMIC LIGHTING & PROCEDURAL SHADER
// Features: Embedded multi-point-light + distance-fog shader, procedural grime
// and water streak textures, institutional checkered tile & acid-etched soapstone,
// 6 dynamic flickering horror point lights, and textured 3D architectural boxes.
// =========================================================================

#define COLLEGE_MAX_LIGHTS 6

static const char *COLLEGE_LIGHTING_VS = R"glsl(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

void main()
{
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 1.0)));
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)glsl";

static const char *COLLEGE_LIGHTING_FS = R"glsl(
#version 330
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform int useTexture;

#define COLLEGE_MAX_LIGHTS 6
uniform vec3  lightPos[COLLEGE_MAX_LIGHTS];
uniform vec3  lightColor[COLLEGE_MAX_LIGHTS];
uniform float lightIntensity[COLLEGE_MAX_LIGHTS];
uniform int   lightsCount;

uniform vec3  ambient;
uniform vec3  viewPos;
uniform vec3  fogColor;
uniform float fogDensity;

out vec4 finalColor;

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 baseColor = fragColor.rgb;
    if (useTexture == 1) baseColor *= texture(texture0, fragTexCoord).rgb;

    vec3 lighting = ambient;
    for (int i = 0; i < lightsCount; i++)
    {
        vec3 toLight  = lightPos[i] - fragPosition;
        float dist    = length(toLight);
        vec3 lightDir = toLight / max(dist, 0.0001);

        float diff = max(dot(normal, lightDir), 0.0);
        float wrap = max(dot(normal, lightDir) * 0.5 + 0.5, 0.0) * 0.15;
        float atten = lightIntensity[i] / (1.0 + 0.14 * dist + 0.07 * dist * dist);

        lighting += lightColor[i] * (diff + wrap) * atten;
    }

    vec3 color = baseColor * lighting;

    float distToCam = length(viewPos - fragPosition);
    float fogFactor  = 1.0 - exp(-fogDensity * fogDensity * distToCam * distToCam);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    finalColor = vec4(mix(color, fogColor, fogFactor), fragColor.a);
}
)glsl";

static Shader    g_collegeShader;
static Model     g_collegeUnitCube;
static int       g_collegeLocUseTexture       = -1;
static int       g_collegeLocAmbient          = -1;
static int       g_collegeLocViewPos          = -1;
static int       g_collegeLocFogColor         = -1;
static int       g_collegeLocFogDensity       = -1;
static int       g_collegeLocLightsCount      = -1;
static int       g_collegeLocLightPos[COLLEGE_MAX_LIGHTS];
static int       g_collegeLocLightColor[COLLEGE_MAX_LIGHTS];
static int       g_collegeLocLightIntensity[COLLEGE_MAX_LIGHTS];

static Texture2D g_texCollegeBrick;
static Texture2D g_texCollegeConcrete;
static Texture2D g_texCollegeWallInt;
static Texture2D g_texCollegeTileHall;
static Texture2D g_texCollegeTileLab;
static Texture2D g_texCollegeWood;
static Texture2D g_texCollegeRoof;
static bool      g_collegeAssetsLoaded = false;

static inline int CollegeClampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Procedural grimy texture generator with vertical dark water streaks
static Texture2D MakeCollegeGrimeTexture(Color base, int variance, int streaks, int seed, int size = 128)
{
    Image img = GenImageColor(size, size, base);
    SetRandomSeed(seed);

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int d = GetRandomValue(-variance, variance);
            Color c = {
                (unsigned char)CollegeClampi(base.r + d, 0, 255),
                (unsigned char)CollegeClampi(base.g + d, 0, 255),
                (unsigned char)CollegeClampi(base.b + d, 0, 255),
                255
            };
            ImageDrawPixel(&img, x, y, c);
        }
    }

    // Vertical water / grime runoff streaks
    Color dark = { (unsigned char)(base.r * 0.42f), (unsigned char)(base.g * 0.42f), (unsigned char)(base.b * 0.42f), 255 };
    for (int i = 0; i < streaks; i++) {
        int x = GetRandomValue(0, size - 1);
        int startY = GetRandomValue(0, size / 3);
        int len = GetRandomValue(size / 3, size - startY);
        int w = GetRandomValue(1, 3);
        for (int y = startY; y < startY + len && y < size; y++) {
            for (int wx = -w; wx <= w; wx++) {
                if (x + wx >= 0 && x + wx < size) ImageDrawPixel(&img, x + wx, y, dark);
            }
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&tex);
    SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    return tex;
}

// Procedural tile texture generator with dark grout and per-tile shade jitter
static Texture2D MakeCollegeTileTexture(Color grout, Color tile, int tilesPerSide, int seed, int size = 128)
{
    Image img = GenImageColor(size, size, grout);
    SetRandomSeed(seed);
    int cell = size / tilesPerSide;
    for (int ty = 0; ty < tilesPerSide; ty++) {
        for (int tx = 0; tx < tilesPerSide; tx++) {
            int shade = GetRandomValue(-9, 9);
            Color c = {
                (unsigned char)CollegeClampi(tile.r + shade, 0, 255),
                (unsigned char)CollegeClampi(tile.g + shade, 0, 255),
                (unsigned char)CollegeClampi(tile.b + shade, 0, 255),
                255
            };
            Rectangle r = { (float)(tx * cell + 1), (float)(ty * cell + 1), (float)(cell - 2), (float)(cell - 2) };
            ImageDrawRectangleRec(&img, r, c);
        }
    }
    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&tex);
    SetTextureFilter(tex, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    return tex;
}

static void InitCollegeShaderAndTextures()
{
    if (g_collegeAssetsLoaded) return;

    g_collegeShader = LoadShaderFromMemory(COLLEGE_LIGHTING_VS, COLLEGE_LIGHTING_FS);
    if (g_collegeShader.id == 0) {
        TraceLog(LOG_ERROR, "College lighting shader failed to compile!");
    }

    Mesh mesh = GenMeshCube(1.0f, 1.0f, 1.0f);
    g_collegeUnitCube = LoadModelFromMesh(mesh);
    g_collegeUnitCube.materials[0].shader = g_collegeShader;

    g_collegeLocUseTexture       = GetShaderLocation(g_collegeShader, "useTexture");
    g_collegeLocAmbient          = GetShaderLocation(g_collegeShader, "ambient");
    g_collegeLocViewPos          = GetShaderLocation(g_collegeShader, "viewPos");
    g_collegeLocFogColor         = GetShaderLocation(g_collegeShader, "fogColor");
    g_collegeLocFogDensity       = GetShaderLocation(g_collegeShader, "fogDensity");
    g_collegeLocLightsCount      = GetShaderLocation(g_collegeShader, "lightsCount");

    for (int i = 0; i < COLLEGE_MAX_LIGHTS; i++) {
        g_collegeLocLightPos[i]       = GetShaderLocation(g_collegeShader, TextFormat("lightPos[%d]", i));
        g_collegeLocLightColor[i]     = GetShaderLocation(g_collegeShader, TextFormat("lightColor[%d]", i));
        g_collegeLocLightIntensity[i] = GetShaderLocation(g_collegeShader, TextFormat("lightIntensity[%d]", i));
    }

    g_texCollegeBrick    = MakeCollegeGrimeTexture({ 78, 42, 34, 255 }, 14, 26, 101);
    g_texCollegeConcrete = MakeCollegeGrimeTexture({ 58, 56, 52, 255 }, 12, 18, 202);
    g_texCollegeWallInt  = MakeCollegeGrimeTexture({ 64, 62, 54, 255 }, 12, 22, 303);
    g_texCollegeTileHall = MakeCollegeTileTexture({ 28, 30, 28, 255 }, { 68, 72, 64, 255 }, 8, 404);
    g_texCollegeTileLab  = MakeCollegeTileTexture({ 20, 22, 24, 255 }, { 36, 40, 44, 255 }, 8, 505);
    g_texCollegeWood     = MakeCollegeGrimeTexture({ 72, 52, 36, 255 }, 16, 12, 606);
    g_texCollegeRoof     = MakeCollegeGrimeTexture({ 28, 26, 24, 255 }, 10, 20, 707);

    g_collegeAssetsLoaded = true;
    TraceLog(LOG_INFO, "College lighting shader & procedural textures initialized successfully!");
}

static void UnloadCollegeShaderAndTextures()
{
    if (!g_collegeAssetsLoaded) return;
    UnloadShader(g_collegeShader);
    UnloadModel(g_collegeUnitCube);
    UnloadTexture(g_texCollegeBrick);
    UnloadTexture(g_texCollegeConcrete);
    UnloadTexture(g_texCollegeWallInt);
    UnloadTexture(g_texCollegeTileHall);
    UnloadTexture(g_texCollegeTileLab);
    UnloadTexture(g_texCollegeWood);
    UnloadTexture(g_texCollegeRoof);
    g_collegeAssetsLoaded = false;
}

static void DrawCollegeTexturedBox(Vector3 center, Vector3 size, Texture2D tex, Color tint = WHITE)
{
    int on = 1;
    SetShaderValue(g_collegeShader, g_collegeLocUseTexture, &on, SHADER_UNIFORM_INT);
    SetMaterialTexture(&g_collegeUnitCube.materials[0], MATERIAL_MAP_ALBEDO, tex);
    DrawModelEx(g_collegeUnitCube, center, (Vector3){ 0.0f, 1.0f, 0.0f }, 0.0f, size, tint);
    int off = 0;
    SetShaderValue(g_collegeShader, g_collegeLocUseTexture, &off, SHADER_UNIFORM_INT);
}

static void DrawAbandonedCollege(Camera3D camera, float timeVal, float dt, float extDayFactor, float extNightFactor, Vector3 sunDir) {
    Vector3 collegeCenter = { 170.0f, 12.0f, 142.0f };
    float distToCollege = Vector3Distance(camera.position, collegeCenter);
    if (distToCollege > 140.0f) return; // Far-distance cull

    // Ensure assets are loaded
    if (!g_collegeAssetsLoaded) {
        InitCollegeShaderAndTextures();
    }

    // Dynamic Fluorescent Corridor Flicker & Creak Timers
    g_collegeFlickerTimer -= dt;
    if (g_collegeFlickerTimer <= 0.0f) {
        if (g_collegeLightOn) {
            g_collegeLightOn = (GetRandomValue(0, 10) > 3);
            g_collegeFlickerTimer = g_collegeLightOn ? ((float)GetRandomValue(15, 60) / 10.0f) : ((float)GetRandomValue(1, 8) / 20.0f);
        } else {
            g_collegeLightOn = true;
            g_collegeFlickerTimer = (float)GetRandomValue(5, 30) / 10.0f;
        }
    }

    g_collegeCreakTimer -= dt;
    if (g_collegeCreakTimer <= 0.0f) {
        g_collegeCreakTimer = (float)GetRandomValue(7, 16);
        if (distToCollege < 38.0f) {
            PlaySound(g_sndWaterDrip);
        }
    }

    // ---------------------------------------------------------------------
    // DYNAMIC POINT LIGHTS & SHADER UNIFORM SETUP
    // ---------------------------------------------------------------------
    bool isInsideCollege = (camera.position.x >= 152.0f && camera.position.x <= 188.0f &&
                            camera.position.z >= 124.0f && camera.position.z <= 160.0f &&
                            camera.position.y >= 9.8f  && camera.position.y <= 16.0f);

    Vector3 amb;
    if (isInsideCollege) {
        amb = (Vector3){ 0.028f, 0.030f, 0.035f }; // Deep claustrophobic darkness inside
    } else {
        amb = (Vector3){ 0.035f + 0.22f * extDayFactor, 0.035f + 0.22f * extDayFactor, 0.045f + 0.18f * extDayFactor };
    }
    Vector3 fogColor = (Vector3){ 0.035f + 0.04f * extDayFactor, 0.040f + 0.04f * extDayFactor, 0.045f + 0.05f * extDayFactor };
    float   fogDensity = isInsideCollege ? 0.038f : 0.022f;

    SetShaderValue(g_collegeShader, g_collegeLocAmbient, &amb, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_collegeShader, g_collegeLocViewPos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_collegeShader, g_collegeLocFogColor, &fogColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_collegeShader, g_collegeLocFogDensity, &fogDensity, SHADER_UNIFORM_FLOAT);

    int lightsCount = COLLEGE_MAX_LIGHTS;
    SetShaderValue(g_collegeShader, g_collegeLocLightsCount, &lightsCount, SHADER_UNIFORM_INT);

    // Light 0: Portico Swinging Lantern
    float lanternSway = sinf(timeVal * 1.6f) * 0.06f;
    Vector3 l0Pos = { 149.2f, 15.10f + lanternSway * 0.5f, 140.0f + lanternSway };
    Vector3 l0Col = { 1.0f, 0.65f, 0.25f };
    float   l0Int = 3.8f * (0.85f + 0.15f * sinf(timeVal * 7.0f));

    // Light 1: Hallway Flickering Fluorescent Tube
    Vector3 l1Pos = { 163.0f, 14.85f, 140.0f };
    Vector3 l1Col = { 0.88f, 1.0f, 0.82f };
    float   l1Int = g_collegeLightOn ? (5.8f + 0.6f * sinf(timeVal * 45.0f)) : 0.0f;

    // Light 2: Corridor Blood-Red Emergency Exit Sign
    Vector3 l2Pos = { 173.8f, 14.85f, 140.0f };
    Vector3 l2Col = { 1.0f, 0.08f, 0.10f };
    float   l2Int = 3.6f;

    // Light 3: Classroom 101 Volumetric Window Light
    Vector3 l3Pos = { 163.0f, 12.8f, 128.5f };
    Vector3 l3Col = { 0.78f, 0.85f, 0.95f };
    float   l3Int = 3.2f;

    // Light 4: Science Lab 102 Bioluminescent Formalin Jars
    Vector3 l4Pos = { 162.0f, 12.8f, 156.5f };
    Vector3 l4Col = { 0.20f, 1.0f, 0.35f };
    float   l4Int = 4.6f;

    // Light 5: Archive 103 Faculty Desk & Safe
    Vector3 l5Pos = { 181.0f, 11.2f, 135.0f };
    Vector3 l5Col = { 0.90f, 0.75f, 0.45f };
    float   l5Int = 2.4f;

    Vector3 lPositions[6]  = { l0Pos, l1Pos, l2Pos, l3Pos, l4Pos, l5Pos };
    Vector3 lColors[6]     = { l0Col, l1Col, l2Col, l3Col, l4Col, l5Col };
    float   lIntensities[6] = { l0Int, l1Int, l2Int, l3Int, l4Int, l5Int };

    for (int i = 0; i < COLLEGE_MAX_LIGHTS; i++) {
        SetShaderValue(g_collegeShader, g_collegeLocLightPos[i], &lPositions[i], SHADER_UNIFORM_VEC3);
        SetShaderValue(g_collegeShader, g_collegeLocLightColor[i], &lColors[i], SHADER_UNIFORM_VEC3);
        SetShaderValue(g_collegeShader, g_collegeLocLightIntensity[i], &lIntensities[i], SHADER_UNIFORM_FLOAT);
    }

    // Material Colors
    Color concreteCol     = { 68, 66, 62, 255 };
    Color ironCol         = { 22, 20, 18, 255 };
    Color rustCol         = { 95, 48, 26, 255 };

    // ---------------------------------------------------------------------
    // BEGIN SHADER MODE: Render all textured boxes and illuminated props!
    // ---------------------------------------------------------------------
    BeginShaderMode(g_collegeShader);

    // 1. GROUND APRON & OVERGROWTH (X: 147..193, Z: 119..165)
    DrawCollegeTexturedBox((Vector3){ 170.0f, 10.008f, 142.0f }, (Vector3){ 46.0f, 0.02f, 46.0f }, g_texCollegeConcrete, { 110, 110, 105, 255 });
    // Overgrown weed patches & perimeter cracks
    DrawCollegeTexturedBox((Vector3){ 150.5f, 10.02f, 134.0f }, (Vector3){ 3.0f, 0.012f, 2.2f }, g_texCollegeConcrete, { 60, 85, 45, 255 });
    DrawCollegeTexturedBox((Vector3){ 150.5f, 10.02f, 146.0f }, (Vector3){ 2.6f, 0.012f, 2.5f }, g_texCollegeConcrete, { 60, 85, 45, 255 });
    DrawCollegeTexturedBox((Vector3){ 188.5f, 10.02f, 130.0f }, (Vector3){ 2.8f, 0.012f, 3.2f }, g_texCollegeConcrete, { 55, 80, 42, 255 });
    DrawCollegeTexturedBox((Vector3){ 188.5f, 10.02f, 154.0f }, (Vector3){ 3.1f, 0.012f, 2.8f }, g_texCollegeConcrete, { 55, 80, 42, 255 });
    // Asphalt cracks
    DrawCube((Vector3){ 149.8f, 10.025f, 137.0f }, 1.8f, 0.005f, 0.05f, { 14, 12, 10, 255 });
    DrawCube((Vector3){ 150.2f, 10.025f, 143.0f }, 2.1f, 0.005f, 0.06f, { 14, 12, 10, 255 });

    // 2. ENTRANCE PORTICO, STEPS, FLUTED COLUMNS & ARCH
    DrawCollegeTexturedBox((Vector3){ 148.4f, 10.12f, 140.0f }, (Vector3){ 1.4f, 0.24f, 6.8f }, g_texCollegeConcrete);
    DrawCollegeTexturedBox((Vector3){ 149.6f, 10.24f, 140.0f }, (Vector3){ 1.2f, 0.48f, 6.4f }, g_texCollegeConcrete);
    DrawCollegeTexturedBox((Vector3){ 150.8f, 10.36f, 140.0f }, (Vector3){ 1.2f, 0.72f, 6.0f }, g_texCollegeConcrete);

    // Classical Fluted Columns (Pillars at Z = 137.4 and Z = 142.6)
    float colZs[2] = { 137.4f, 142.6f };
    for (int ci = 0; ci < 2; ci++) {
        float cz = colZs[ci];
        DrawCollegeTexturedBox((Vector3){ 149.2f, 10.55f, cz }, (Vector3){ 0.85f, 0.90f, 0.85f }, g_texCollegeConcrete);
        DrawCylinder((Vector3){ 149.2f, 11.0f, cz }, 0.28f, 0.32f, 4.2f, 12, concreteCol);
        for (int r = 0; r < 6; r++) {
            float ribAngle = (float)r * (PI / 3.0f);
            float rx = 149.2f + cosf(ribAngle) * 0.30f;
            float rz = cz + sinf(ribAngle) * 0.30f;
            DrawCylinder((Vector3){ rx, 11.0f, rz }, 0.018f, 0.018f, 4.2f, 4, { 44, 42, 40, 255 });
        }
        DrawCollegeTexturedBox((Vector3){ 149.2f, 15.25f, cz }, (Vector3){ 0.80f, 0.30f, 0.80f }, g_texCollegeConcrete);
        DrawCollegeTexturedBox((Vector3){ 149.2f, 15.42f, cz }, (Vector3){ 0.92f, 0.15f, 0.92f }, g_texCollegeConcrete);
    }

    // Portico Roof Entablature with Classical Dentils
    DrawCollegeTexturedBox((Vector3){ 150.2f, 15.60f, 140.0f }, (Vector3){ 3.8f, 0.45f, 7.2f }, g_texCollegeConcrete);
    for (float dz = 136.8f; dz <= 143.2f; dz += 0.45f) {
        DrawCube((Vector3){ 148.26f, 15.48f, dz }, 0.12f, 0.12f, 0.22f, concreteCol);
    }
    DrawCollegeTexturedBox((Vector3){ 149.2f, 16.05f, 140.0f }, (Vector3){ 0.4f, 0.50f, 6.2f }, g_texCollegeConcrete);

    // Wrought-iron Arch Sign: "BLACKWOOD VALLEY COLLEGE - EST. 1948"
    DrawCylinder((Vector3){ 149.2f, 15.6f, colZs[0] }, 0.04f, 0.04f, 1.4f, 8, ironCol);
    DrawCylinder((Vector3){ 149.2f, 15.6f, colZs[1] }, 0.04f, 0.04f, 1.4f, 8, ironCol);
    DrawCube((Vector3){ 149.2f, 16.85f, 140.0f }, 0.06f, 0.12f, 5.4f, ironCol);
    DrawCollegeTexturedBox((Vector3){ 149.2f, 16.50f, 140.0f }, (Vector3){ 0.05f, 0.52f, 4.6f }, g_texCollegeWood, { 60, 50, 45, 255 });
    DrawCubeWires((Vector3){ 149.2f, 16.50f, 140.0f }, 0.06f, 0.54f, 4.62f, rustCol);
    DrawCube((Vector3){ 149.17f, 16.55f, 140.0f }, 0.02f, 0.08f, 3.8f, { 185, 170, 140, 255 });
    DrawCube((Vector3){ 149.17f, 16.38f, 140.0f }, 0.02f, 0.06f, 2.2f, { 150, 135, 110, 255 });
    DrawSphere((Vector3){ 149.2f, 17.05f, colZs[0] }, 0.09f, ironCol);
    DrawSphere((Vector3){ 149.2f, 17.05f, colZs[1] }, 0.09f, ironCol);
    DrawSphere((Vector3){ 149.2f, 17.05f, 140.0f }, 0.11f, ironCol);

    // Hanging Rusted Lantern swinging under portico
    DrawCylinder((Vector3){ 149.2f, 15.35f, 140.0f }, 0.008f, 0.008f, 0.40f, 4, ironCol);
    DrawCube(l0Pos, 0.22f, 0.32f, 0.22f, ironCol);
    DrawCubeWires(l0Pos, 0.23f, 0.33f, 0.23f, rustCol);
    DrawSphere(l0Pos, 0.08f, (Color){ 255, 175, 70, 255 });

    // 3. EXTERIOR WALLS & FLAT TAR ROOF (X: 152..188, Z: 124..160)
    // Concrete Foundation Plinth (Y = 10.0..10.8)
    DrawCollegeTexturedBox((Vector3){ 170.0f, 10.4f, 142.0f }, (Vector3){ 36.2f, 0.8f, 36.2f }, g_texCollegeConcrete);

    // Weathered Red Brick Facades with vertical rain streaks
    DrawCollegeTexturedBox((Vector3){ 152.0f, 13.2f, 131.4f }, (Vector3){ 0.45f, 4.8f, 14.8f }, g_texCollegeBrick);
    DrawCollegeTexturedBox((Vector3){ 152.0f, 13.2f, 150.6f }, (Vector3){ 0.45f, 4.8f, 18.8f }, g_texCollegeBrick);
    DrawCollegeTexturedBox((Vector3){ 152.0f, 14.6f, 140.0f }, (Vector3){ 0.48f, 2.0f, 3.2f }, g_texCollegeBrick); // Entrance lintel
    DrawCollegeTexturedBox((Vector3){ 188.0f, 13.2f, 142.0f }, (Vector3){ 0.45f, 4.8f, 36.0f }, g_texCollegeBrick); // Back wall
    DrawCollegeTexturedBox((Vector3){ 170.0f, 13.2f, 124.0f }, (Vector3){ 36.0f, 4.8f, 0.45f }, g_texCollegeBrick); // South wall
    DrawCollegeTexturedBox((Vector3){ 170.0f, 13.2f, 160.0f }, (Vector3){ 36.0f, 4.8f, 0.45f }, g_texCollegeBrick); // North wall

    // Flat Tar Roof & Parapet Coping
    DrawCollegeTexturedBox((Vector3){ 170.0f, 15.65f, 142.0f }, (Vector3){ 36.6f, 0.35f, 36.6f }, g_texCollegeRoof);
    DrawCollegeTexturedBox((Vector3){ 170.0f, 15.95f, 142.0f }, (Vector3){ 36.8f, 0.25f, 36.8f }, g_texCollegeConcrete);
    // Rooftop AC Chiller Unit
    DrawCube((Vector3){ 175.0f, 16.4f, 134.0f }, 2.8f, 1.2f, 2.0f, { 48, 52, 54, 255 });
    DrawCubeWires((Vector3){ 175.0f, 16.4f, 134.0f }, 2.85f, 1.22f, 2.05f, ironCol);
    DrawCylinder((Vector3){ 175.0f, 17.0f, 134.0f }, 0.55f, 0.55f, 0.35f, 10, { 36, 38, 40, 255 });
    rlPushMatrix();
    rlTranslatef(175.0f, 17.15f, 134.0f);
    rlRotatef(timeVal * 180.0f, 0.0f, 1.0f, 0.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 0.90f, 0.02f, 0.12f, ironCol);
    rlPopMatrix();
    DrawCylinder((Vector3){ 162.0f, 15.8f, 152.0f }, 0.18f, 0.18f, 1.8f, 8, { 60, 56, 50, 255 });

    // 4. BOARDED WINDOWS WITH SPECULAR BROKEN GLASS
    for (float wz = 127.0f; wz <= 157.0f; wz += 6.0f) {
        if (wz >= 138.0f && wz <= 142.0f) continue;
        DrawCollegeTexturedBox((Vector3){ 151.75f, 11.85f, wz }, (Vector3){ 0.28f, 0.14f, 1.85f }, g_texCollegeConcrete);
        DrawCube((Vector3){ 151.76f, 11.25f, wz }, 0.03f, 1.05f, 1.4f, { 28, 24, 20, 210 });
        DrawCube((Vector3){ 151.85f, 12.85f, wz }, 0.08f, 1.85f, 1.65f, { 38, 28, 20, 255 });
        DrawCubeWires((Vector3){ 151.85f, 12.85f, wz }, 0.09f, 1.87f, 1.67f, ironCol);
        // Window mullion bars
        DrawCube((Vector3){ 151.83f, 12.85f, wz }, 0.04f, 1.80f, 0.05f, { 32, 24, 18, 255 });
        DrawCube((Vector3){ 151.83f, 12.85f, wz }, 0.04f, 0.05f, 1.60f, { 32, 24, 18, 255 });
        // Glass shards glinting
        DrawCube((Vector3){ 151.84f, 12.25f, wz - 0.35f }, 0.015f, 0.50f, 0.45f, { 170, 215, 225, 160 });
        DrawCube((Vector3){ 151.84f, 13.35f, wz + 0.30f }, 0.015f, 0.40f, 0.40f, { 185, 230, 240, 180 });
        DrawCube((Vector3){ 151.83f, 12.45f, wz - 0.25f }, 0.02f, 0.04f, 0.20f, { 245, 255, 255, 220 });
        // Rough wood barricade boards
        DrawCollegeTexturedBox((Vector3){ 151.74f, 12.55f, wz }, (Vector3){ 0.05f, 0.22f, 2.05f }, g_texCollegeWood);
        DrawCollegeTexturedBox((Vector3){ 151.72f, 13.15f, wz }, (Vector3){ 0.05f, 0.20f, 1.95f }, g_texCollegeWood);
        rlPushMatrix();
        rlTranslatef(151.70f, 12.85f, wz);
        rlRotatef(28.0f, 1.0f, 0.0f, 0.0f);
        DrawCollegeTexturedBox((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.05f, 0.18f, 2.15f }, g_texCollegeWood);
        rlPopMatrix();
        DrawSphere((Vector3){ 151.68f, 12.55f, wz - 0.85f }, 0.018f, rustCol);
        DrawSphere((Vector3){ 151.68f, 12.55f, wz + 0.85f }, 0.018f, rustCol);
        DrawSphere((Vector3){ 151.68f, 13.15f, wz - 0.75f }, 0.018f, rustCol);
    }

    // Main Double Entrance Doors
    rlPushMatrix();
    rlTranslatef(152.0f, 10.0f, 139.1f);
    rlRotatef(-32.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(4.0f, 0.0f, 0.0f, 1.0f);
    DrawCollegeTexturedBox((Vector3){ 0.0f, 1.5f, 0.45f }, (Vector3){ 0.08f, 3.0f, 0.90f }, g_texCollegeWood);
    DrawCube((Vector3){ 0.0f, 0.25f, 0.45f }, 0.09f, 0.45f, 0.88f, { 140, 115, 60, 255 }); // Brass kickplate
    rlPopMatrix();
    DrawCollegeTexturedBox((Vector3){ 152.6f, 11.5f, 140.9f }, (Vector3){ 0.90f, 3.0f, 0.08f }, g_texCollegeWood);
    // Bloody hand smear on doorframe
    DrawCube((Vector3){ 151.96f, 11.85f, 138.95f }, 0.02f, 0.65f, 0.16f, { 115, 8, 14, 240 });
    DrawCube((Vector3){ 152.02f, 11.75f, 139.0f }, 0.08f, 0.025f, 0.02f, { 85, 5, 10, 255 });
    DrawCube((Vector3){ 152.02f, 11.65f, 139.0f }, 0.08f, 0.025f, 0.02f, { 85, 5, 10, 255 });

    // Cull fine interior details if player is far away
    if (distToCollege > 78.0f && camera.position.x < 151.0f) {
        EndShaderMode();
        return;
    }

    // 5. CENTRAL CORRIDOR (X: 152..188, Z: 138.5..141.5, Y: 10.0..15.5)
    // High-contrast checkered linoleum tile floor
    DrawCollegeTexturedBox((Vector3){ 169.5f, 10.015f, 140.0f }, (Vector3){ 35.0f, 0.01f, 3.0f }, g_texCollegeTileHall);

    // Acoustic ceiling grid with missing tiles
    DrawCollegeTexturedBox((Vector3){ 170.0f, 15.25f, 140.0f }, (Vector3){ 35.0f, 0.05f, 3.0f }, g_texCollegeConcrete, { 80, 78, 72, 255 });
    DrawCube((Vector3){ 161.5f, 15.55f, 140.0f }, 2.4f, 0.55f, 2.0f, { 12, 12, 14, 255 }); // Dark void
    DrawCube((Vector3){ 161.5f, 15.55f, 140.0f }, 2.4f, 0.40f, 0.85f, { 75, 78, 80, 255 }); // AC duct
    DrawCubeWires((Vector3){ 161.5f, 15.55f, 140.0f }, 2.42f, 0.42f, 0.87f, ironCol);
    rlPushMatrix();
    rlTranslatef(161.2f, 14.85f, 140.2f);
    rlRotatef(25.0f, 0.0f, 0.0f, 1.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 0.65f, 0.035f, 0.65f, { 45, 42, 38, 255 });
    rlPopMatrix();
    DrawCylinder((Vector3){ 161.0f, 15.05f, 140.2f }, 0.004f, 0.004f, 0.45f, 4, ironCol);
    DrawCylinder((Vector3){ 167.2f, 14.75f, 139.8f }, 0.008f, 0.008f, 0.70f, 4, { 18, 18, 18, 255 });
    DrawSphere((Vector3){ 167.2f, 14.40f, 139.8f }, 0.015f, { 220, 140, 50, 255 });

    // Ceiling Leak Bucket
    Vector3 bucketPos = { 165.5f, 10.0f, 139.4f };
    DrawCylinder(bucketPos, 0.18f, 0.15f, 0.32f, 8, { 110, 115, 120, 255 });
    DrawCylinder((Vector3){ bucketPos.x, bucketPos.y + 0.22f, bucketPos.z }, 0.17f, 0.17f, 0.04f, 8, { 35, 45, 50, 200 });

    // Metal Student Lockers
    Color lockerCol  = { 38, 44, 42, 255 };
    Color lockerTrim = { 22, 26, 24, 255 };
    for (float lx = 154.5f; lx <= 172.5f; lx += 1.2f) {
        if (lx >= 160.8f && lx <= 163.6f) continue;
        DrawCube((Vector3){ lx, 11.35f, 138.8f }, 1.15f, 2.7f, 0.50f, lockerCol);
        DrawCubeWires((Vector3){ lx, 11.35f, 138.8f }, 1.16f, 2.72f, 0.52f, lockerTrim);
        DrawCube((Vector3){ lx, 12.3f, 139.06f }, 0.8f, 0.12f, 0.02f, lockerTrim);
        DrawCube((Vector3){ lx, 10.4f, 139.06f }, 0.8f, 0.12f, 0.02f, lockerTrim);

        DrawCube((Vector3){ lx, 11.35f, 141.2f }, 1.15f, 2.7f, 0.50f, lockerCol);
        DrawCubeWires((Vector3){ lx, 11.35f, 141.2f }, 1.16f, 2.72f, 0.52f, lockerTrim);
        DrawCube((Vector3){ lx, 12.3f, 140.94f }, 0.8f, 0.12f, 0.02f, lockerTrim);
        DrawCube((Vector3){ lx, 10.4f, 140.94f }, 0.8f, 0.12f, 0.02f, lockerTrim);
    }
    DrawCube((Vector3){ 168.0f, 10.04f, 139.4f }, 0.35f, 0.04f, 0.45f, { 165, 155, 120, 255 });
    DrawCube((Vector3){ 168.3f, 10.03f, 139.6f }, 0.40f, 0.03f, 0.30f, { 45, 55, 75, 255 });

    // Cork Noticeboard
    DrawCollegeTexturedBox((Vector3){ 165.5f, 12.2f, 141.42f }, (Vector3){ 2.2f, 1.2f, 0.05f }, g_texCollegeWood);
    DrawCube((Vector3){ 165.5f, 12.2f, 141.40f }, 2.0f, 1.0f, 0.02f, { 140, 120, 90, 255 });
    DrawCube((Vector3){ 165.0f, 12.3f, 141.38f }, 0.4f, 0.55f, 0.01f, { 220, 215, 195, 255 });
    DrawCube((Vector3){ 166.0f, 12.1f, 141.38f }, 0.5f, 0.35f, 0.01f, { 210, 190, 150, 255 });

    // Tilted Fluorescent Fixture
    rlPushMatrix();
    rlTranslatef(163.0f, 15.15f, 140.0f);
    rlRotatef(-12.0f, 0.0f, 0.0f, 1.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 2.4f, 0.08f, 0.38f, { 38, 40, 42, 255 });
    Color tubeCol = g_collegeLightOn ? (Color){ 245, 255, 225, 255 } : (Color){ 45, 48, 46, 255 };
    DrawCylinder((Vector3){ -1.05f, -0.04f, -0.08f }, 0.025f, 0.025f, 2.1f, 6, tubeCol);
    DrawCylinder((Vector3){ -1.05f, -0.04f,  0.08f }, 0.025f, 0.025f, 2.1f, 6, tubeCol);
    rlPopMatrix();

    // Glowing Blood-Red Exit Sign
    DrawCube((Vector3){ 173.8f, 14.85f, 140.0f }, 0.12f, 0.30f, 0.58f, { 24, 24, 26, 255 });
    DrawCube((Vector3){ 173.8f, 14.85f, 140.0f }, 0.13f, 0.25f, 0.52f, { 245, 20, 25, 250 });

    // 18-Meter Arterial Blood Drag Mark
    for (float bx = 162.0f; bx <= 180.0f; bx += 1.5f) {
        float offZ = 140.0f + sinf(bx * 0.7f) * 0.28f;
        float smearW = 0.58f + 0.28f * cosf(bx * 1.1f);
        DrawCube((Vector3){ bx, 10.022f, offZ }, 1.55f, 0.002f, smearW, { 115, 8, 14, 235 });
        DrawCube((Vector3){ bx, 10.024f, offZ + 0.18f }, 1.4f, 0.002f, 0.022f, { 65, 4, 8, 255 });
        DrawCube((Vector3){ bx, 10.024f, offZ + 0.24f }, 1.4f, 0.002f, 0.022f, { 65, 4, 8, 255 });
        DrawCube((Vector3){ bx, 10.024f, offZ + 0.30f }, 1.4f, 0.002f, 0.022f, { 65, 4, 8, 255 });
    }

    // 6. CLASSROOM 101: LECTURE HALL (North Wing, X: 153..173, Z: 125..138.5)
    // Partition walls with peeling plaster texture
    DrawCollegeTexturedBox((Vector3){ 157.0f, 12.8f, 138.5f }, (Vector3){ 8.0f, 5.4f, 0.35f }, g_texCollegeWallInt);
    DrawCollegeTexturedBox((Vector3){ 168.0f, 12.8f, 138.5f }, (Vector3){ 10.0f, 5.4f, 0.35f }, g_texCollegeWallInt);
    // Doorway
    rlPushMatrix();
    rlTranslatef(161.2f, 10.0f, 138.5f);
    rlRotatef(48.0f, 0.0f, 1.0f, 0.0f);
    DrawCollegeTexturedBox((Vector3){ 0.45f, 1.4f, 0.0f }, (Vector3){ 0.90f, 2.8f, 0.06f }, g_texCollegeWood);
    rlPopMatrix();
    DrawCube((Vector3){ 162.0f, 13.0f, 138.7f }, 1.2f, 0.25f, 0.03f, { 180, 170, 150, 255 });

    // Classroom floor (dusty grimy wood)
    DrawCollegeTexturedBox((Vector3){ 163.0f, 10.015f, 131.5f }, (Vector3){ 20.0f, 0.01f, 13.0f }, g_texCollegeWood, { 140, 135, 125, 255 });

    // 12 Student Desks
    Color steelLegCol = { 32, 34, 38, 255 };
    for (int row = 0; row < 4; row++) {
        float rz = 136.0f - (float)row * 2.6f;
        for (int col = 0; col < 3; col++) {
            float rx = 157.0f + (float)col * 5.0f;
            int deskIdx = row * 3 + col;
            if (deskIdx == 7) {
                rlPushMatrix();
                rlTranslatef(rx, 10.25f, rz);
                rlRotatef(78.0f, 0.0f, 0.0f, 1.0f);
                DrawCollegeTexturedBox((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector3){ 0.65f, 0.03f, 0.48f }, g_texCollegeWood);
                DrawCylinder((Vector3){ -0.28f, -0.7f, -0.2f }, 0.02f, 0.02f, 0.7f, 6, steelLegCol);
                DrawCylinder((Vector3){ 0.28f, -0.7f, 0.2f }, 0.02f, 0.02f, 0.7f, 6, steelLegCol);
                rlPopMatrix();
                DrawCube((Vector3){ rx + 0.4f, 10.02f, rz + 0.3f }, 0.28f, 0.005f, 0.22f, { 220, 215, 200, 255 });
                DrawCube((Vector3){ rx - 0.5f, 10.02f, rz - 0.2f }, 0.26f, 0.005f, 0.20f, { 205, 195, 175, 255 });
                continue;
            }

            float yawTilt = (deskIdx == 3 || deskIdx == 8) ? 18.0f : ((deskIdx == 5) ? -22.0f : 0.0f);
            rlPushMatrix();
            rlTranslatef(rx, 10.0f, rz);
            if (yawTilt != 0.0f) rlRotatef(yawTilt, 0.0f, 1.0f, 0.0f);
            DrawCylinder((Vector3){ -0.30f, 0.0f, -0.20f }, 0.018f, 0.018f, 0.72f, 6, steelLegCol);
            DrawCylinder((Vector3){  0.30f, 0.0f, -0.20f }, 0.018f, 0.018f, 0.72f, 6, steelLegCol);
            DrawCylinder((Vector3){ -0.30f, 0.0f,  0.20f }, 0.018f, 0.018f, 0.72f, 6, steelLegCol);
            DrawCylinder((Vector3){  0.30f, 0.0f,  0.20f }, 0.018f, 0.018f, 0.72f, 6, steelLegCol);
            DrawCollegeTexturedBox((Vector3){ 0.0f, 0.72f, 0.0f }, (Vector3){ 0.72f, 0.035f, 0.50f }, g_texCollegeWood);
            DrawCube((Vector3){ 0.0f, 0.22f, 0.40f }, 0.04f, 0.04f, 0.40f, steelLegCol);
            DrawCollegeTexturedBox((Vector3){ 0.0f, 0.44f, 0.48f }, (Vector3){ 0.42f, 0.03f, 0.38f }, g_texCollegeWood, { 115, 85, 55, 255 });
            DrawCollegeTexturedBox((Vector3){ 0.0f, 0.76f, 0.65f }, (Vector3){ 0.40f, 0.24f, 0.025f }, g_texCollegeWood, { 115, 85, 55, 255 });
            if (deskIdx == 1 || deskIdx == 6) {
                DrawCube((Vector3){ 0.10f, 0.75f, 0.0f }, 0.24f, 0.02f, 0.18f, { 180, 50, 45, 255 });
            }
            rlPopMatrix();
        }
    }

    // Teacher's Podium & Instructor Desk
    DrawCollegeTexturedBox((Vector3){ 163.0f, 10.45f, 126.8f }, (Vector3){ 2.0f, 0.88f, 0.95f }, g_texCollegeWood);
    DrawCube((Vector3){ 163.4f, 11.15f, 126.8f }, 0.65f, 0.50f, 0.50f, { 50, 34, 22, 255 });

    // Psychotic Chalkboard
    DrawCollegeTexturedBox((Vector3){ 163.0f, 12.80f, 125.08f }, (Vector3){ 10.2f, 2.85f, 0.06f }, g_texCollegeWood);
    DrawCube((Vector3){ 163.0f, 12.80f, 125.11f }, 9.9f, 2.55f, 0.02f, { 22, 30, 24, 255 });
    DrawCube((Vector3){ 163.0f, 11.50f, 125.16f }, 9.9f, 0.05f, 0.12f, { 45, 32, 20, 255 });
    DrawCube((Vector3){ 160.5f, 11.54f, 125.16f }, 0.16f, 0.03f, 0.06f, { 70, 50, 35, 255 });
    DrawCube((Vector3){ 161.2f, 11.54f, 125.16f }, 0.08f, 0.02f, 0.02f, { 240, 240, 235, 255 });

    Color chalkCol = { 235, 235, 230, 240 };
    DrawCube((Vector3){ 160.0f, 13.55f, 125.13f }, 2.8f, 0.06f, 0.01f, chalkCol);
    DrawCube((Vector3){ 160.0f, 13.25f, 125.13f }, 2.2f, 0.05f, 0.01f, chalkCol);
    DrawCube((Vector3){ 165.5f, 13.60f, 125.13f }, 2.4f, 0.06f, 0.01f, chalkCol);
    DrawCube((Vector3){ 165.5f, 13.30f, 125.13f }, 2.6f, 0.05f, 0.01f, chalkCol);
    for (int t = 0; t < 8; t++) {
        float tx = 158.4f + (float)t * 0.70f;
        DrawCube((Vector3){ tx + 0.00f, 12.5f, 125.13f }, 0.02f, 0.25f, 0.01f, chalkCol);
        DrawCube((Vector3){ tx + 0.08f, 12.5f, 125.13f }, 0.02f, 0.25f, 0.01f, chalkCol);
        DrawCube((Vector3){ tx + 0.16f, 12.5f, 125.13f }, 0.02f, 0.25f, 0.01f, chalkCol);
        DrawCube((Vector3){ tx + 0.24f, 12.5f, 125.13f }, 0.02f, 0.25f, 0.01f, chalkCol);
        DrawCube((Vector3){ tx + 0.12f, 12.5f, 125.13f }, 0.32f, 0.025f, 0.01f, chalkCol);
    }
    DrawSphereWires((Vector3){ 163.0f, 12.5f, 125.13f }, 0.38f, 8, 8, chalkCol);
    DrawSphere((Vector3){ 163.0f, 12.5f, 125.14f }, 0.09f, { 18, 22, 20, 255 });

    Color bloodDeep = { 125, 8, 14, 245 };
    Color bloodDark = { 80, 5, 8, 250 };
    DrawCube((Vector3){ 164.2f, 12.9f, 125.14f }, 1.4f, 0.85f, 0.015f, bloodDeep);
    DrawCube((Vector3){ 163.8f, 13.2f, 125.14f }, 0.8f, 0.60f, 0.015f, bloodDeep);
    DrawCube((Vector3){ 163.8f, 12.2f, 125.14f }, 0.05f, 1.2f, 0.015f, bloodDark);
    DrawCube((Vector3){ 164.3f, 12.0f, 125.14f }, 0.06f, 1.4f, 0.015f, bloodDark);
    DrawCube((Vector3){ 164.7f, 12.3f, 125.14f }, 0.04f, 0.9f, 0.015f, bloodDark);
    DrawCube((Vector3){ 164.3f, 11.51f, 125.18f }, 1.2f, 0.02f, 0.14f, bloodDeep);
    DrawCube((Vector3){ 164.0f, 10.024f, 128.0f }, 3.8f, 0.003f, 2.8f, bloodDark);
    DrawCube((Vector3){ 164.8f, 10.025f, 128.5f }, 2.2f, 0.003f, 2.0f, bloodDeep);

    // 7. CLASSROOM 102: ANATOMY LAB (South Wing, X: 153..173, Z: 141.5..159)
    DrawCollegeTexturedBox((Vector3){ 157.0f, 12.8f, 141.5f }, (Vector3){ 8.0f, 5.4f, 0.35f }, g_texCollegeWallInt);
    DrawCollegeTexturedBox((Vector3){ 168.0f, 12.8f, 141.5f }, (Vector3){ 10.0f, 5.4f, 0.35f }, g_texCollegeWallInt);
    rlPushMatrix();
    rlTranslatef(161.2f, 10.0f, 141.5f);
    rlRotatef(-42.0f, 0.0f, 1.0f, 0.0f);
    DrawCollegeTexturedBox((Vector3){ 0.45f, 1.4f, 0.0f }, (Vector3){ 0.90f, 2.8f, 0.06f }, g_texCollegeWood);
    rlPopMatrix();
    DrawCube((Vector3){ 162.0f, 13.0f, 141.3f }, 1.2f, 0.25f, 0.03f, { 180, 170, 150, 255 });

    // Acid-etched dark soapstone slate floor
    DrawCollegeTexturedBox((Vector3){ 163.0f, 10.015f, 150.5f }, (Vector3){ 20.0f, 0.01f, 17.0f }, g_texCollegeTileLab);

    // 3 Heavy Soapstone Workbenches
    Color benchCol = { 24, 26, 28, 255 };
    Color benchTop = { 18, 20, 22, 255 };
    Color sinkCol = { 210, 215, 215, 255 };
    Color brassCol = { 180, 145, 55, 255 };
    float benchZs[3] = { 145.0f, 149.0f, 155.5f };
    for (int b = 0; b < 3; b++) {
        float bz = benchZs[b];
        DrawCube((Vector3){ 162.0f, 10.45f, bz }, 10.0f, 0.90f, 1.10f, benchCol);
        DrawCube((Vector3){ 162.0f, 10.92f, bz }, 10.3f, 0.06f, 1.25f, benchTop);
        DrawCube((Vector3){ 166.2f, 10.85f, bz }, 0.75f, 0.35f, 0.65f, sinkCol);
        DrawCylinder((Vector3){ 166.5f, 10.95f, bz }, 0.015f, 0.015f, 0.35f, 6, brassCol);
        if (b == 0) {
            DrawCube((Vector3){ 160.5f, 10.96f, bz }, 0.18f, 0.02f, 0.15f, { 190, 225, 230, 180 });
        }
    }

    // 8 Glowing Formalin Specimen Jars
    DrawCollegeTexturedBox((Vector3){ 162.0f, 12.5f, 158.8f }, (Vector3){ 8.0f, 0.05f, 0.40f }, g_texCollegeWood);
    DrawCollegeTexturedBox((Vector3){ 162.0f, 13.5f, 158.8f }, (Vector3){ 8.0f, 0.05f, 0.40f }, g_texCollegeWood);
    for (int j = 0; j < 8; j++) {
        float jx = 158.5f + (float)j * 1.0f;
        DrawCylinder((Vector3){ jx, 12.55f, 158.8f }, 0.085f, 0.085f, 0.28f, 8, { 180, 210, 200, 120 });
        DrawCylinder((Vector3){ jx, 12.56f, 158.8f }, 0.078f, 0.078f, 0.24f, 8, { 55, 140, 50, 190 });
        DrawSphere((Vector3){ jx, 12.68f, 158.8f }, 0.045f, { 115, 55, 50, 230 });
        DrawCylinder((Vector3){ jx, 12.83f, 158.8f }, 0.09f, 0.07f, 0.04f, 8, { 25, 25, 25, 255 });
    }

    // Central Stainless Steel Dissection Table
    Color steelCol = { 140, 145, 150, 255 };
    DrawCylinder((Vector3){ 161.8f, 10.0f, 151.4f }, 0.035f, 0.035f, 0.85f, 6, steelCol);
    DrawCylinder((Vector3){ 164.2f, 10.0f, 151.4f }, 0.035f, 0.035f, 0.85f, 6, steelCol);
    DrawCylinder((Vector3){ 161.8f, 10.0f, 152.6f }, 0.035f, 0.035f, 0.85f, 6, steelCol);
    DrawCylinder((Vector3){ 164.2f, 10.0f, 152.6f }, 0.035f, 0.035f, 0.85f, 6, steelCol);
    DrawCube((Vector3){ 163.0f, 10.85f, 152.0f }, 2.6f, 0.08f, 1.30f, steelCol);
    DrawCubeWires((Vector3){ 163.0f, 10.87f, 152.0f }, 2.62f, 0.10f, 1.32f, { 90, 95, 100, 255 });
    DrawCube((Vector3){ 163.0f, 10.90f, 152.0f }, 2.2f, 0.005f, 0.95f, bloodDeep);
    DrawCube((Vector3){ 163.6f, 10.90f, 152.0f }, 1.2f, 0.005f, 0.80f, bloodDark);
    DrawCylinder((Vector3){ 164.0f, 10.0f, 152.0f }, 0.16f, 0.14f, 0.32f, 8, { 100, 105, 110, 255 });
    DrawCylinder((Vector3){ 164.0f, 10.22f, 152.0f }, 0.15f, 0.15f, 0.08f, 8, bloodDark);
    DrawCube((Vector3){ 161.75f, 10.75f, 151.4f }, 0.03f, 0.22f, 0.05f, { 65, 45, 30, 255 });
    DrawCube((Vector3){ 164.25f, 10.75f, 152.6f }, 0.03f, 0.22f, 0.05f, { 65, 45, 30, 255 });

    // Full Anatomical Skeleton staring at the doorway
    Color boneCol = { 215, 210, 192, 255 };
    Color boneDark = { 170, 165, 148, 255 };
    DrawSphere((Vector3){ 168.3f, 10.03f, 146.8f }, 0.035f, steelCol);
    DrawSphere((Vector3){ 168.7f, 10.03f, 146.8f }, 0.035f, steelCol);
    DrawSphere((Vector3){ 168.3f, 10.03f, 147.2f }, 0.035f, steelCol);
    DrawSphere((Vector3){ 168.7f, 10.03f, 147.2f }, 0.035f, steelCol);
    DrawCube((Vector3){ 168.5f, 10.06f, 147.0f }, 0.55f, 0.03f, 0.55f, steelCol);
    DrawCylinder((Vector3){ 168.5f, 10.0f, 147.0f }, 0.015f, 0.015f, 3.4f, 6, steelCol);

    DrawCube((Vector3){ 168.5f, 11.95f, 147.0f }, 0.28f, 0.16f, 0.18f, boneCol);
    DrawCubeWires((Vector3){ 168.5f, 11.95f, 147.0f }, 0.29f, 0.17f, 0.19f, boneDark);
    DrawCylinder((Vector3){ 168.4f, 11.35f, 147.0f }, 0.03f, 0.025f, 0.55f, 6, boneCol);
    DrawCylinder((Vector3){ 168.6f, 11.35f, 147.0f }, 0.03f, 0.025f, 0.55f, 6, boneCol);
    DrawSphere((Vector3){ 168.4f, 11.33f, 147.0f }, 0.035f, boneCol);
    DrawSphere((Vector3){ 168.6f, 11.33f, 147.0f }, 0.035f, boneCol);
    DrawCylinder((Vector3){ 168.4f, 10.65f, 147.0f }, 0.025f, 0.022f, 0.65f, 6, boneCol);
    DrawCylinder((Vector3){ 168.6f, 10.65f, 147.0f }, 0.025f, 0.022f, 0.65f, 6, boneCol);
    DrawCube((Vector3){ 168.4f, 10.08f, 146.92f }, 0.07f, 0.04f, 0.16f, boneCol);
    DrawCube((Vector3){ 168.6f, 10.08f, 146.92f }, 0.07f, 0.04f, 0.16f, boneCol);

    for (float sy = 12.05f; sy <= 12.75f; sy += 0.08f) {
        DrawSphere((Vector3){ 168.5f, sy, 147.0f }, 0.035f, boneCol);
    }
    DrawCube((Vector3){ 168.5f, 12.45f, 147.0f }, 0.32f, 0.42f, 0.22f, boneCol);
    DrawCubeWires((Vector3){ 168.5f, 12.45f, 147.0f }, 0.34f, 0.44f, 0.24f, boneDark);
    DrawCylinder((Vector3){ 168.32f, 12.10f, 147.0f }, 0.024f, 0.020f, 0.55f, 6, boneCol);
    DrawCylinder((Vector3){ 168.68f, 12.10f, 147.0f }, 0.024f, 0.020f, 0.55f, 6, boneCol);
    DrawCylinder((Vector3){ 168.32f, 11.55f, 147.0f }, 0.020f, 0.016f, 0.52f, 6, boneCol);
    DrawCylinder((Vector3){ 168.68f, 11.55f, 147.0f }, 0.020f, 0.016f, 0.52f, 6, boneCol);

    rlPushMatrix();
    rlTranslatef(168.5f, 12.95f, 147.0f);
    rlRotatef(-48.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(14.0f, 1.0f, 0.0f, 0.0f);
    DrawSphere((Vector3){ 0.0f, 0.12f, 0.0f }, 0.11f, boneCol);
    DrawSphereWires((Vector3){ 0.0f, 0.12f, 0.0f }, 0.115f, 8, 8, boneDark);
    DrawSphere((Vector3){ -0.042f, 0.11f, -0.095f }, 0.022f, { 14, 10, 8, 255 });
    DrawSphere((Vector3){  0.042f, 0.11f, -0.095f }, 0.022f, { 14, 10, 8, 255 });
    DrawCube((Vector3){ 0.0f, 0.07f, -0.098f }, 0.020f, 0.035f, 0.015f, { 14, 10, 8, 255 });
    DrawCube((Vector3){ 0.0f, 0.02f, -0.065f }, 0.085f, 0.05f, 0.09f, boneCol);
    DrawCube((Vector3){ 0.0f, 0.025f, -0.095f }, 0.065f, 0.015f, 0.01f, { 235, 235, 230, 255 });
    rlPopMatrix();

    // 8. ROOM 103: ARCHIVE & OFFICE (East Wing, X: 174..187)
    DrawCollegeTexturedBox((Vector3){ 174.0f, 12.8f, 131.5f }, (Vector3){ 0.40f, 5.4f, 15.0f }, g_texCollegeWallInt);
    DrawCollegeTexturedBox((Vector3){ 174.0f, 12.8f, 150.5f }, (Vector3){ 0.40f, 5.4f, 19.0f }, g_texCollegeWallInt);
    rlPushMatrix();
    rlTranslatef(174.0f, 10.0f, 139.2f);
    rlRotatef(62.0f, 0.0f, 1.0f, 0.0f);
    DrawCollegeTexturedBox((Vector3){ 0.0f, 1.4f, 0.45f }, (Vector3){ 0.06f, 2.8f, 0.90f }, g_texCollegeWood);
    rlPopMatrix();

    // Parquet Floor
    DrawCollegeTexturedBox((Vector3){ 180.5f, 10.015f, 142.0f }, (Vector3){ 14.0f, 0.01f, 34.0f }, g_texCollegeWood, { 120, 100, 80, 255 });

    // Faculty Desk with Banker's Lamp & Off-hook Rotary Phone
    DrawCollegeTexturedBox((Vector3){ 181.0f, 10.45f, 135.0f }, (Vector3){ 2.4f, 0.90f, 1.20f }, g_texCollegeWood);
    DrawCube((Vector3){ 181.0f, 10.92f, 135.0f }, 1.2f, 0.01f, 0.70f, { 35, 55, 40, 255 });
    DrawCube((Vector3){ 181.6f, 10.96f, 134.8f }, 0.22f, 0.08f, 0.22f, { 18, 18, 20, 255 });
    DrawCylinder((Vector3){ 181.6f, 11.02f, 134.8f }, 0.045f, 0.045f, 0.02f, 8, { 220, 220, 220, 255 });
    DrawCube((Vector3){ 181.3f, 10.94f, 134.9f }, 0.24f, 0.04f, 0.07f, { 18, 18, 20, 255 });
    DrawCylinder((Vector3){ 180.4f, 10.92f, 135.2f }, 0.06f, 0.06f, 0.02f, 8, brassCol);
    DrawCylinder((Vector3){ 180.4f, 10.94f, 135.2f }, 0.012f, 0.012f, 0.35f, 6, brassCol);
    DrawCube((Vector3){ 180.4f, 11.30f, 135.2f }, 0.16f, 0.08f, 0.28f, { 25, 85, 45, 230 });

    // 4 Metal Filing Cabinets
    Color fileCabCol = { 38, 44, 40, 255 };
    DrawCube((Vector3){ 186.0f, 11.15f, 142.0f }, 0.65f, 2.3f, 0.90f, fileCabCol);
    DrawCubeWires((Vector3){ 186.0f, 11.15f, 142.0f }, 0.66f, 2.32f, 0.92f, { 22, 28, 24, 255 });
    DrawCube((Vector3){ 186.0f, 11.15f, 143.2f }, 0.65f, 2.3f, 0.90f, fileCabCol);
    rlPushMatrix();
    rlTranslatef(184.5f, 10.35f, 145.5f);
    rlRotatef(82.0f, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 0.65f, 2.3f, 0.90f, fileCabCol);
    rlPopMatrix();
    DrawCube((Vector3){ 184.2f, 10.02f, 147.2f }, 0.35f, 0.01f, 0.28f, { 205, 185, 140, 255 });
    DrawCube((Vector3){ 184.8f, 10.02f, 146.8f }, 0.32f, 0.01f, 0.26f, { 190, 170, 130, 255 });

    // Heavy Cast-Iron Safe with Claw Gouges
    DrawCube((Vector3){ 186.0f, 10.80f, 127.0f }, 1.3f, 1.6f, 1.3f, { 28, 30, 32, 255 });
    DrawCubeWires((Vector3){ 186.0f, 10.80f, 127.0f }, 1.32f, 1.62f, 1.32f, { 16, 18, 20, 255 });
    DrawCube((Vector3){ 185.35f, 10.80f, 127.0f }, 0.10f, 1.4f, 1.1f, { 35, 38, 40, 255 });
    DrawCylinder((Vector3){ 185.28f, 10.80f, 127.0f }, 0.08f, 0.08f, 0.04f, 12, brassCol);
    DrawCube((Vector3){ 185.28f, 11.0f, 126.9f }, 0.02f, 0.30f, 0.015f, { 80, 12, 16, 240 });
    DrawCube((Vector3){ 185.28f, 11.0f, 127.0f }, 0.02f, 0.30f, 0.015f, { 80, 12, 16, 240 });
    DrawCube((Vector3){ 185.28f, 11.0f, 127.1f }, 0.02f, 0.30f, 0.015f, { 80, 12, 16, 240 });

    EndShaderMode();
    // ---------------------------------------------------------------------
    // END SHADER MODE: Render alpha overlays, light pools & dust motes
    // ---------------------------------------------------------------------

    // Volumetric Halos & Light Pools
    DrawSphere(l0Pos, 0.45f, { 255, 170, 50, 25 }); // Portico lantern warm halo

    if (g_collegeLightOn) {
        DrawSphere(l1Pos, 0.55f, { 220, 245, 200, 55 }); // Fluorescent tube green-white halo
        DrawCircle3D((Vector3){ 163.0f, 10.02f, 140.0f }, 2.5f, (Vector3){ 1, 0, 0 }, 90.0f, { 75, 88, 65, 45 });
        DrawCircle3D((Vector3){ 163.0f, 10.02f, 140.0f }, 4.2f, (Vector3){ 1, 0, 0 }, 90.0f, { 45, 55, 38, 25 });
    }

    DrawSphere(l2Pos, 0.40f, { 220, 15, 20, 60 }); // Red exit sign halo
    DrawCircle3D((Vector3){ 173.8f, 10.02f, 140.0f }, 3.2f, (Vector3){ 1, 0, 0 }, 90.0f, { 85, 8, 12, 50 });

    // Formalin Jars Green Glow
    for (int j = 0; j < 8; j++) {
        float jx = 158.5f + (float)j * 1.0f;
        DrawSphere((Vector3){ jx, 12.68f, 158.8f }, 0.28f, { 60, 230, 80, 30 });
    }

    // Classroom 101 Volumetric Light Shaft through broken boards
    Color lightShaftCol = { 235, 225, 180, (unsigned char)(20 + (int)(sinf(timeVal * 1.5f) * 6)) };
    rlPushMatrix();
    rlTranslatef(163.0f, 12.0f, 128.5f);
    rlRotatef(-32.0f, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 3.5f, 0.6f, 6.0f, lightShaftCol);
    rlPopMatrix();

    // Ceiling Leak Expanding Ripple on Puddle
    float rippleRadius = 0.4f + fmodf(timeVal * 0.8f, 1.0f) * 0.65f;
    unsigned char rippleAlpha = (unsigned char)(Clamp(1.0f - (rippleRadius - 0.4f) / 0.65f, 0.0f, 1.0f) * 110.0f);
    DrawCircle3D((Vector3){ bucketPos.x, 10.022f, bucketPos.z }, rippleRadius, (Vector3){ 1, 0, 0 }, 90.0f, { 60, 80, 95, rippleAlpha });
    DrawCircle3D((Vector3){ bucketPos.x, 10.020f, bucketPos.z }, 0.95f, (Vector3){ 1, 0, 0 }, 90.0f, { 30, 42, 50, 90 });

    // 24 Floating Atmospheric Dust Motes
    for (int d = 0; d < 24; d++) {
        float seed = (float)d * 3.71f;
        float mx = 155.0f + fmodf(seed * 19.3f, 26.0f);
        float my = 10.4f  + fmodf(seed * 11.7f + timeVal * 0.15f, 3.8f);
        float mz = 127.0f + fmodf(seed * 23.1f, 28.0f);
        mx += sinf(timeVal * 0.8f + seed) * 0.18f;
        mz += cosf(timeVal * 0.6f + seed) * 0.18f;
        float moteAlpha = 65.0f + sinf(timeVal * 2.0f + seed) * 35.0f;
        Color moteCol = { 225, 220, 205, (unsigned char)Clamp(moteAlpha, 20.0f, 110.0f) };
        DrawCube((Vector3){ mx, my, mz }, 0.028f, 0.028f, 0.028f, moteCol);
    }
}







// =========================================================================
// CRASHED SILVER SEDAN & ROADSIDE IMPACT SITE (Route 9 Mile Marker 14)
// Tilted into ditch verge at X = 143.8, Z = 136.5 with crumpled accordion
// hood, steaming cracked radiator, spiderweb fractured windshield,
// deflated airbag, sprung ajar driver door, sheared utility pole, and skid ruts.
// =========================================================================
static const Vector3 g_crashedCarPos = { 143.8f, 9.92f, 136.5f };

static inline Color CrashCarTint(Color base, float dayF, float nightF, float vertBias = 1.0f) {
    float amb = 0.28f + 0.62f * dayF + 0.15f * nightF;
    float r = ((float)base.r / 255.0f) * amb * vertBias;
    float g = ((float)base.g / 255.0f) * amb * vertBias;
    float b = ((float)base.b / 255.0f) * amb * vertBias;
    return (Color){
        (unsigned char)Clamp(r * 255.0f, 0.0f, 255.0f),
        (unsigned char)Clamp(g * 255.0f, 0.0f, 255.0f),
        (unsigned char)Clamp(b * 255.0f, 0.0f, 255.0f),
        base.a
    };
}

static void DrawCrashedSedan(Vector3 carPos, float timeVal, float extDayFactor, float extNightFactor, Camera3D camera) {
    float distSq = (carPos.x - camera.position.x)*(carPos.x - camera.position.x) + (carPos.z - camera.position.z)*(carPos.z - camera.position.z);
    if (distSq > 130.0f * 130.0f) return;

    // 1. Skid marks curving from Route 9 asphalt into roadside ditch verge
    for (int t = 0; t < 18; t++) {
        float f = (float)t / 17.0f;
        float sx = Lerp(138.5f, 142.8f, f * f);
        float sz = Lerp(126.0f, 134.5f, f);
        float sy = 10.015f - f * 0.05f;
        DrawCube((Vector3){ sx - 0.75f, sy, sz }, 0.22f, 0.005f, 0.55f, (Color){ 12, 12, 14, (unsigned char)(180 - f * 60) });
        DrawCube((Vector3){ sx + 0.75f, sy, sz }, 0.24f, 0.005f, 0.55f, (Color){ 10, 10, 12, (unsigned char)(210 - f * 40) });
        if (t > 10) {
            DrawCube((Vector3){ sx + 0.95f, sy + 0.03f, sz }, 0.18f, 0.06f, 0.35f, (Color){ 36, 28, 20, 240 });
        }
    }

    // 2. Splintered wooden utility pole sheared off at impact (Stump center: X = 145.3f, Z = 138.8f)
    Vector3 poleBase = { 145.3f, 9.90f, 138.8f };
    Color woodPost = CrashCarTint((Color){ 62, 52, 40, 255 }, extDayFactor, extNightFactor);
    Color woodSplinter = CrashCarTint((Color){ 110, 95, 70, 255 }, extDayFactor, extNightFactor);

    // Sheared stump in mud
    DrawCylinder(poleBase, 0.22f, 0.24f, 0.85f, 8, woodPost);
    DrawCube((Vector3){ poleBase.x - 0.08f, 10.80f, poleBase.z }, 0.08f, 0.35f, 0.08f, woodSplinter);
    DrawCube((Vector3){ poleBase.x + 0.06f, 10.75f, poleBase.z - 0.05f }, 0.07f, 0.28f, 0.07f, woodSplinter);

    // Tilted upper pole resting across car hood
    rlPushMatrix();
    rlTranslatef(poleBase.x, 10.75f, poleBase.z);
    rlRotatef(22.0f, 0.0f, 0.0f, 1.0f);
    rlRotatef(-14.0f, 1.0f, 0.0f, 0.0f);
    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.20f, 0.17f, 6.5f, 8, woodPost);
    DrawCube((Vector3){ 0.0f, 5.8f, 0.0f }, 1.8f, 0.12f, 0.12f, woodPost);
    DrawCylinder((Vector3){ -0.7f, 6.0f, 0.0f }, 0.06f, 0.06f, 0.18f, 6, (Color){ 200, 210, 215, 255 });
    DrawCylinder((Vector3){  0.7f, 6.0f, 0.0f }, 0.06f, 0.06f, 0.18f, 6, (Color){ 200, 210, 215, 255 });
    rlPopMatrix();

    // Snapped low-voltage wires dangling down into the weeds
    DrawLine3D((Vector3){ poleBase.x + 1.2f, 15.5f, poleBase.z }, (Vector3){ poleBase.x + 0.6f, 12.0f, poleBase.z + 0.8f }, (Color){ 25, 25, 28, 220 });
    DrawLine3D((Vector3){ poleBase.x + 0.6f, 12.0f, poleBase.z + 0.8f }, (Vector3){ poleBase.x - 0.2f, 10.1f, poleBase.z + 1.4f }, (Color){ 25, 25, 28, 220 });

    // Dented "MILE 14" roadside marker post knocked askew into the mud
    DrawCube((Vector3){ 142.2f, 10.08f, 134.8f }, 0.08f, 0.16f, 0.85f, (Color){ 90, 95, 100, 255 });
    DrawCube((Vector3){ 142.2f, 10.15f, 134.8f }, 0.22f, 0.02f, 0.35f, (Color){ 215, 220, 210, 255 });

    // Buckled steel guardrail segment bent around front bumper
    DrawCube((Vector3){ 144.2f, 10.45f, 139.2f }, 1.8f, 0.32f, 0.08f, (Color){ 130, 135, 140, 255 });
    DrawCubeWires((Vector3){ 144.2f, 10.45f, 139.2f }, 1.82f, 0.33f, 0.09f, (Color){ 70, 75, 80, 255 });

    // Dark oil / coolant spill beneath crushed engine
    DrawCircle3D((Vector3){ 144.6f, 9.97f, 138.2f }, 1.15f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 8, 8, 10, 235 });
    DrawCircle3D((Vector3){ 144.3f, 9.97f, 137.8f }, 0.65f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 15, 20, 25, 190 });

    // Scattered safety glass shards glinting on ground
    for (int g = 0; g < 14; g++) {
        float gx = 144.0f + sinf((float)g * 1.8f) * 1.2f;
        float gz = 137.5f + cosf((float)g * 2.3f) * 1.4f;
        float shimmer = 0.6f + 0.4f * sinf(timeVal * 4.0f + (float)g);
        DrawCube((Vector3){ gx, 9.98f, gz }, 0.04f, 0.015f, 0.04f, (Color){ 210, 235, 255, (unsigned char)(140 * shimmer) });
    }

    // 3. MAIN CAR BODY (Transformed with Yaw: -24 deg, Roll: -6 deg, Pitch: 3.5 deg)
    rlPushMatrix();
    rlTranslatef(carPos.x, carPos.y, carPos.z);
    rlRotatef(-24.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(-6.0f, 0.0f, 0.0f, 1.0f);
    rlRotatef(3.5f, 1.0f, 0.0f, 0.0f);

    Color carPaint      = CrashCarTint((Color){ 135, 140, 150, 255 }, extDayFactor, extNightFactor, 0.95f);
    Color carPaintDark  = CrashCarTint((Color){ 110, 115, 125, 255 }, extDayFactor, extNightFactor, 0.85f);
    Color crumpledMetal = CrashCarTint((Color){ 88, 92, 102, 255 }, extDayFactor, extNightFactor, 0.80f);

    // Chassis drop shadow in local space
    DrawCube((Vector3){ 0.0f, 0.02f, 0.0f }, 2.6f, 0.005f, 5.4f, (Color){ 6, 6, 8, 200 });

    // Lower chassis body frame
    DrawCube((Vector3){ 0.0f, 0.62f, -0.2f }, 2.30f, 0.65f, 4.6f, carPaint);
    // Mud splatter along lower sills & undercarriage
    DrawCube((Vector3){ 0.0f, 0.35f, -0.2f }, 2.34f, 0.22f, 4.62f, (Color){ 42, 34, 24, 230 });

    // Crumpled front end / engine compartment (accordion crushed inwards)
    DrawCube((Vector3){ -0.55f, 0.68f, 2.15f }, 1.10f, 0.58f, 0.85f, crumpledMetal);

    // Right side crushed heavily against pole impact
    rlPushMatrix();
    rlTranslatef(0.50f, 0.65f, 2.05f);
    rlRotatef(-16.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(12.0f, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 1.15f, 0.52f, 0.95f, (Color){ 75, 80, 88, 255 });
    rlPopMatrix();

    // Smashed radiator & engine block exposed inside
    DrawCube((Vector3){ 0.0f, 0.70f, 2.10f }, 1.30f, 0.50f, 0.65f, (Color){ 28, 30, 32, 255 });
    DrawCube((Vector3){ 0.15f, 0.72f, 2.25f }, 0.65f, 0.40f, 0.12f, (Color){ 55, 58, 62, 255 });
    DrawCubeWires((Vector3){ 0.15f, 0.72f, 2.25f }, 0.66f, 0.41f, 0.13f, (Color){ 140, 60, 30, 255 });

    // Front Bumper (accordion bent inwards at center)
    DrawCube((Vector3){ -0.65f, 0.38f, 2.58f }, 1.15f, 0.18f, 0.18f, (Color){ 180, 185, 190, 255 });
    rlPushMatrix();
    rlTranslatef(0.55f, 0.38f, 2.50f);
    rlRotatef(-28.0f, 0.0f, 1.0f, 0.0f);
    rlRotatef(-10.0f, 0.0f, 0.0f, 1.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 1.15f, 0.18f, 0.18f, (Color){ 140, 145, 150, 255 });
    rlPopMatrix();

    // Popped V-buckled hood (two bent panels meeting in a tented crease)
    rlPushMatrix();
    rlTranslatef(0.0f, 0.96f, 1.45f);
    rlRotatef(24.0f, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 2.08f, 0.06f, 0.95f, carPaint);
    rlPopMatrix();

    rlPushMatrix();
    rlTranslatef(0.08f, 1.12f, 2.08f);
    rlRotatef(-32.0f, 1.0f, 0.0f, 0.0f);
    rlRotatef(8.0f, 0.0f, 0.0f, 1.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 2.05f, 0.06f, 0.75f, carPaintDark);
    rlPopMatrix();

    // Trunk & Rear Bumper
    DrawCube((Vector3){ 0.0f, 0.82f, -1.80f }, 2.15f, 0.20f, 1.35f, carPaint);
    DrawCube((Vector3){ 0.0f, 0.38f, -2.65f }, 2.25f, 0.18f, 0.18f, (Color){ 200, 205, 210, 255 });

    // Cabin Greenhouse & Roof
    DrawCube((Vector3){ 0.0f, 1.48f, -0.30f }, 1.95f, 0.06f, 2.45f, carPaint);
    DrawCube((Vector3){ -0.96f, 1.25f, -1.45f }, 0.08f, 0.55f, 0.08f, carPaint);
    DrawCube((Vector3){  0.96f, 1.25f, -1.45f }, 0.08f, 0.55f, 0.08f, carPaint);
    DrawCube((Vector3){ -0.96f, 1.25f,  0.95f }, 0.08f, 0.55f, 0.08f, carPaint);
    DrawCube((Vector3){  0.96f, 1.25f,  0.95f }, 0.08f, 0.55f, 0.08f, carPaint);

    // Dark interior cavity & seats
    DrawCube((Vector3){ 0.0f, 1.15f, -0.30f }, 1.88f, 0.58f, 2.40f, (Color){ 16, 18, 22, 255 });
    DrawCube((Vector3){ -0.48f, 1.02f, -0.45f }, 0.55f, 0.45f, 0.55f, (Color){ 35, 38, 42, 255 });
    DrawCube((Vector3){  0.48f, 1.02f, -0.45f }, 0.55f, 0.45f, 0.55f, (Color){ 35, 38, 42, 255 });

    // Deflated white airbag draped over driver steering wheel
    DrawSphere((Vector3){ -0.48f, 1.18f, 0.22f }, 0.22f, (Color){ 220, 220, 215, 240 });
    DrawCube((Vector3){ -0.48f, 1.10f, 0.24f }, 0.32f, 0.12f, 0.25f, (Color){ 195, 195, 190, 255 });

    // Driver's door sprung ajar ~20 degrees
    rlPushMatrix();
    rlTranslatef(-1.12f, 0.72f, 0.75f);
    rlRotatef(-20.0f, 0.0f, 1.0f, 0.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, -0.65f }, 0.08f, 0.72f, 1.25f, carPaint);
    DrawCube((Vector3){ 0.04f, 0.0f, -0.65f }, 0.03f, 0.65f, 1.15f, (Color){ 32, 34, 38, 255 });
    rlPopMatrix();

    // Passenger door (dented shut)
    DrawCube((Vector3){ 1.14f, 0.72f, 0.10f }, 0.06f, 0.72f, 1.30f, carPaint);

    // Spiderweb Cracked Windshield
    rlPushMatrix();
    rlTranslatef(0.0f, 1.26f, 0.72f);
    rlRotatef(-34.0f, 1.0f, 0.0f, 0.0f);
    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 1.90f, 0.02f, 1.15f, (Color){ 190, 215, 235, 110 });
    DrawLine3D((Vector3){ -0.45f, 0.02f, 0.10f }, (Vector3){ -0.15f, 0.02f, 0.45f }, (Color){ 255, 255, 255, 200 });
    DrawLine3D((Vector3){ -0.45f, 0.02f, 0.10f }, (Vector3){ -0.75f, 0.02f, 0.35f }, (Color){ 255, 255, 255, 200 });
    DrawLine3D((Vector3){ -0.45f, 0.02f, 0.10f }, (Vector3){ -0.55f, 0.02f, -0.35f }, (Color){ 255, 255, 255, 200 });
    DrawLine3D((Vector3){ -0.45f, 0.02f, 0.10f }, (Vector3){  0.25f, 0.02f, -0.15f }, (Color){ 255, 255, 255, 180 });
    DrawCircle3D((Vector3){ -0.45f, 0.025f, 0.10f }, 0.28f, (Vector3){ 0, 1, 0 }, 0.0f, (Color){ 240, 250, 255, 130 });
    rlPopMatrix();

    // 4 WHEELS (With damage & mud)
    DrawCylinder((Vector3){ -1.15f, 0.35f, -1.60f }, 0.35f, 0.35f, 0.22f, 10, (Color){ 20, 20, 22, 255 });
    DrawCylinder((Vector3){  1.15f, 0.30f, -1.60f }, 0.35f, 0.35f, 0.22f, 10, (Color){ 18, 18, 20, 255 });

    // Front Left Wheel (tilted outward)
    rlPushMatrix();
    rlTranslatef(-1.18f, 0.35f, 1.60f);
    rlRotatef(-14.0f, 0.0f, 1.0f, 0.0f);
    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.35f, 0.35f, 0.22f, 10, (Color){ 22, 22, 24, 255 });
    rlPopMatrix();

    // Front Right Wheel (heavily bent camber, popped tire, jammed in mud)
    rlPushMatrix();
    rlTranslatef(1.12f, 0.22f, 1.55f);
    rlRotatef(28.0f, 0.0f, 0.0f, 1.0f);
    rlRotatef(18.0f, 0.0f, 1.0f, 0.0f);
    DrawCylinder((Vector3){ 0.0f, 0.0f, 0.0f }, 0.32f, 0.28f, 0.25f, 10, (Color){ 28, 24, 20, 255 });
    rlPopMatrix();

    // Smashed Right Headlight (empty crushed dark socket with dangling copper wires)
    DrawCube((Vector3){ 0.85f, 0.68f, 2.50f }, 0.28f, 0.16f, 0.12f, (Color){ 18, 18, 20, 255 });
    DrawLine3D((Vector3){ 0.80f, 0.65f, 2.55f }, (Vector3){ 0.88f, 0.52f, 2.62f }, (Color){ 180, 100, 40, 255 });
    DrawLine3D((Vector3){ 0.86f, 0.65f, 2.55f }, (Vector3){ 0.82f, 0.48f, 2.60f }, (Color){ 60, 120, 210, 255 });

    // Left Headlight (cracked lens, flickering weakly)
    bool lightFlicker = (fmodf(timeVal * 7.5f, 1.0f) > 0.25f);
    Color headlitCol = lightFlicker ? (Color){ 255, 240, 180, 210 } : (Color){ 65, 60, 45, 255 };
    DrawCube((Vector3){ -0.85f, 0.68f, 2.55f }, 0.26f, 0.15f, 0.08f, headlitCol);

    // Hazard Flashers (Only rear left still pulses amber, front smashed)
    bool hazFlash = (fmodf(timeVal, 1.1f) < 0.55f);
    Color hazAmber = hazFlash ? (Color){ 255, 135, 15, 255 } : (Color){ 45, 20, 5, 255 };
    DrawCube((Vector3){ -0.95f, 0.70f, -2.62f }, 0.24f, 0.12f, 0.08f, hazAmber);
    DrawCube((Vector3){  0.95f, 0.70f, -2.62f }, 0.24f, 0.12f, 0.08f, (Color){ 30, 15, 10, 255 });

    rlPopMatrix(); // End car local transform

    // 4. Steam / Smoke Motes rising from the punctured radiator (world space)
    Vector3 radPos = { carPos.x + 0.35f, carPos.y + 1.15f, carPos.z + 1.8f };
    for (int sm = 0; sm < 5; sm++) {
        float sSeed = (float)sm * 1.85f;
        float sAge = fmodf(timeVal * 0.85f + sSeed, 2.2f);
        float sAlpha = (1.0f - (sAge / 2.2f));
        float sx = radPos.x + sinf(timeVal * 1.2f + sSeed) * 0.18f;
        float sy = radPos.y + sAge * 0.75f;
        float sz = radPos.z + cosf(timeVal * 1.0f + sSeed) * 0.18f;
        float sSize = 0.06f + sAge * 0.08f;
        DrawCube((Vector3){ sx, sy, sz }, sSize, sSize, sSize, (Color){ 230, 235, 240, (unsigned char)(sAlpha * 95.0f) });
    }
}

// -----------------------------------------------------------------------------
// PHASE 2: IN-WORLD 3D FUEL PUMP CRT & CATENARY HOSE RENDERING SYSTEM
// -----------------------------------------------------------------------------

static void UpdatePumpCrtTexture(int pumpNum, float gallons, float salePrice, bool isFlowing, float flk) {
    if (!g_pumpScreenRTLoaded) return;

    static int lastPumpNum = -1;
    static float lastGallons = -1.0f;
    static bool lastIsFlowing = false;
    static float lastFlk = -1.0f;
    static double lastFlowTime = 0.0;

    // Only update if state changes, or if flowing (for the bargraph animation) at 15 FPS
    bool needsUpdate = (pumpNum != lastPumpNum || gallons != lastGallons || isFlowing != lastIsFlowing || flk != lastFlk);
    if (isFlowing && (GetTime() - lastFlowTime > 0.06)) needsUpdate = true;

    if (!needsUpdate) return;

    lastPumpNum = pumpNum;
    lastGallons = gallons;
    lastIsFlowing = isFlowing;
    lastFlk = flk;
    if (isFlowing) lastFlowTime = GetTime();

    BeginTextureMode(g_pumpScreenRT);
    ClearBackground((Color){ 4, 18, 8, 255 }); // Dark retro emerald phosphorescent glass

    // Horizontal phosphor scanline raster grid
    for (int y = 0; y < 240; y += 4) {
        DrawRectangle(0, y, 320, 2, (Color){ 2, 10, 4, 115 });
    }

    // Header Bar with border
    DrawRectangle(10, 8, 300, 26, (Color){ 8, 36, 16, 235 });
    DrawRectangleLines(10, 8, 300, 26, (Color){ 35, 175, 75, 255 });
    DrawTextSharp(g_fontSmall, TextFormat("ROUTE 9 COOP // DISPENSER 0%d", pumpNum), 18, 14, 12.5f, (Color){ 140, 255, 170, 255 }, 1.2f);

    // Fuel Grade & Octane Badge
    DrawRectangle(10, 38, 140, 20, (Color){ 6, 26, 12, 225 });
    DrawRectangleLines(10, 38, 140, 20, (Color){ 25, 120, 50, 255 });
    DrawTextSharp(g_fontSmall, "OCTANE 87 REGULAR", 16, 42, 10.0f, (Color){ 100, 220, 130, 240 }, 1.1f);

    // Status Indicator Badge
    Color statusBg = isFlowing ? (Color){ 12, 65, 24, 255 } : (Color){ 45, 38, 12, 255 };
    Color statusFg = isFlowing ? (Color){ 80, 255, 120, 255 } : (Color){ 245, 200, 60, 255 };
    DrawRectangle(210, 38, 100, 20, statusBg);
    DrawRectangleLines(210, 38, 100, 20, statusFg);
    DrawTextSharp(g_fontSmall, isFlowing ? ">> FLOWING <<" : "[ STANDBY ]", 216, 42, 10.0f, statusFg, 1.1f);

    // Large Phosphor Digital Meter Displays
    // Box 1: THIS SALE ($)
    DrawRectangle(10, 64, 300, 44, (Color){ 6, 24, 12, 240 });
    DrawRectangleLines(10, 64, 300, 44, (Color){ 30, 150, 65, 255 });
    DrawTextSharp(g_fontSmall, "THIS SALE", 18, 70, 9.5f, (Color){ 90, 190, 115, 220 }, 1.1f);
    DrawTextSharp(g_fontTitle, TextFormat("$ %.2f", salePrice), 18, 83, 21.0f, (Color){ 50, (unsigned char)(245 * flk), 90, 255 }, 1.4f);

    // Box 2: GALLONS
    DrawRectangle(10, 114, 300, 44, (Color){ 6, 24, 12, 240 });
    DrawRectangleLines(10, 114, 300, 44, (Color){ 30, 150, 65, 255 });
    DrawTextSharp(g_fontSmall, "GALLONS", 18, 120, 9.5f, (Color){ 90, 190, 115, 220 }, 1.1f);
    DrawTextSharp(g_fontTitle, TextFormat("%.2f GAL", gallons), 18, 133, 21.0f, (Color){ 50, (unsigned char)(245 * flk), 90, 255 }, 1.4f);

    // Telemetry Footer
    DrawTextSharp(g_fontSmall, TextFormat("UNIT PRICE: $%.3f/GAL", g_fuelPricePerGallon), 14, 166, 11.0f, (Color){ 100, 210, 130, 230 }, 1.1f);
    DrawTextSharp(g_fontSmall, TextFormat("UNDERGROUND TANK: %.1f GAL", g_stationFuelGallons), 14, 182, 11.0f, (Color){ 85, 175, 110, 200 }, 1.1f);

    // Dynamic 16-Segment Flow Bargraph
    int barSegments = 16;
    int litSegments = isFlowing ? ((int)(GetTime() * 14.0f) % (barSegments + 1)) : 0;
    for (int b = 0; b < barSegments; b++) {
        Color bCol = (b < litSegments) ? (Color){ 65, 255, 110, 255 } : (Color){ 16, 52, 26, 210 };
        DrawRectangle(14 + b * 18, 206, 14, 14, bCol);
    }

    // CRT Edge Vignette & Corner Glass Glint
    DrawRectangleLinesEx((Rectangle){ 0, 0, 320, 240 }, 4.0f, (Color){ 2, 8, 3, 255 });
    DrawLine(10, 10, 65, 10, (Color){ 180, 255, 200, 75 });

    EndTextureMode();
}

static void DrawPumpCrtScreen3D(Vector3 center, float width, float height, Texture2D tex, bool faceWest, Color tint = WHITE) {
    float hw = width * 0.5f;
    float hh = height * 0.5f;

    rlDisableBackfaceCulling();
    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    rlColor4ub(tint.r, tint.g, tint.b, tint.a);

    if (faceWest) {
        // Quad facing -X (towards Left Lane)
        rlNormal3f(-1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(center.x, center.y - hh, center.z + hw); // Bottom-left
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(center.x, center.y - hh, center.z - hw); // Bottom-right
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(center.x, center.y + hh, center.z - hw); // Top-right
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(center.x, center.y + hh, center.z + hw); // Top-left
    } else {
        // Quad facing +X (towards Right Lane)
        rlNormal3f(1.0f, 0.0f, 0.0f);
        rlTexCoord2f(0.0f, 0.0f); rlVertex3f(center.x, center.y - hh, center.z - hw); // Bottom-left
        rlTexCoord2f(1.0f, 0.0f); rlVertex3f(center.x, center.y - hh, center.z + hw); // Bottom-right
        rlTexCoord2f(1.0f, 1.0f); rlVertex3f(center.x, center.y + hh, center.z + hw); // Top-right
        rlTexCoord2f(0.0f, 1.0f); rlVertex3f(center.x, center.y + hh, center.z - hw); // Top-left
    }

    rlEnd();
    rlSetTexture(0);
    rlEnableBackfaceCulling();
}

// (DrawCatenaryHose & DrawFirstPersonFuelNozzle extracted to systems/gas_station_system.h/.cpp)

#include "systems/ocean_system.h"
#include "systems/shop_atmosphere.h"

int main() {

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

        ClearBackground((Color){ 6, 7, 9, 255 });



        int scrW = GetScreenWidth();

        int scrH = GetScreenHeight();

        if (scrW < 960) scrW = LOGICAL_W;

        if (scrH < 540) scrH = LOGICAL_H;



        // Cinematic deep horror vignette framing the screen

        int vH = scrH / 3;

        int vW = scrW / 4;

        DrawRectangleGradientV(0, 0, scrW, vH, (Color){ 0, 0, 0, 245 }, BLANK);

        DrawRectangleGradientV(0, scrH - vH, scrW, vH, BLANK, (Color){ 0, 0, 0, 255 });

        DrawRectangleGradientH(0, 0, vW, scrH, (Color){ 0, 0, 0, 235 }, BLANK);

        DrawRectangleGradientH(scrW - vW, 0, vW, scrH, BLANK, (Color){ 0, 0, 0, 235 });



        // Feather-light CRT scanlines for analog found-footage texture

        for (int y = 0; y < scrH; y += 4) {

            DrawLine(0, y, scrW, y, (Color){ 0, 0, 0, 28 });

        }



        float timeSec = (float)GetTime();



        // Flashing surveillance recording indicator at top left

        bool recBlink = (fmodf(timeSec * 2.0f, 1.0f) < 0.65f);

        if (recBlink) {

            DrawCircle(45, 38, 5.0f, (Color){ 225, 35, 30, 255 });

            DrawTextSharp(g_fontBody, "REC [DISPATCH EVIDENCE REEL #09]", 58, 28, 17.0f, (Color){ 235, 55, 45, 240 });

        } else {

            DrawTextSharp(g_fontBody, "    [DISPATCH EVIDENCE REEL #09]", 58, 28, 17.0f, (Color){ 140, 40, 35, 180 });

        }

        DrawTextSharp(g_fontSmall, "STATE POLICE CRIME LAB // UNRESOLVED CASE ARCHIVE", 45, 54, 14.0f, (Color){ 140, 135, 130, 200 });





        // Forensic watermark stamp at top right

        DrawTextSharp(g_fontSmall, "DECLASSIFIED // EVIDENCE VAULT B-9", scrW - 320, 28, 14.0f, (Color){ 170, 50, 45, 210 });

        DrawTextSharp(g_fontSmall, "SEC LOG: 142.85 MHz [CALIBRATED]", scrW - 320, 48, 13.0f, (Color){ 120, 115, 110, 170 });



        // Ominous cryptic case header (Strong Gothic Heading Font)

        const char* titleHead = "WHAT THE GROUND KEEPS";

        DrawTextSharpCentered(g_fontTitle, titleHead, scrW/2 + 3, scrH/2 - 118, 46.0f, (Color){ 120, 18, 14, 210 }, 3.0f);

        DrawTextSharpCentered(g_fontTitle, titleHead, scrW/2, scrH/2 - 120, 46.0f, (Color){ 245, 240, 230, 255 }, 3.0f);



        // Cryptic warning transcript

        const char* quote = "\"Whatever falls into the Route 9 mire... does not decay.\"";

        DrawTextSharpCentered(g_fontBody, quote, scrW/2, scrH/2 - 62, 18.0f, (Color){ 190, 180, 170, 230 });



        // Sleek blood-ember forensic gauge track

        int barW = (int)(scrW * 0.44f);

        if (barW < 400) barW = 400;

        if (barW > 600) barW = 600;

        int barH = 6;

        int bx = scrW / 2 - barW / 2;

        int by = scrH / 2 - 10;



        // Forensic scale tick marks

        DrawLine(bx, by - 8, bx, by + barH + 8, (Color){ 90, 85, 80, 200 });

        DrawLine(bx + barW/4, by - 4, bx + barW/4, by + barH + 4, (Color){ 60, 55, 50, 150 });

        DrawLine(bx + barW/2, by - 6, bx + barW/2, by + barH + 6, (Color){ 90, 85, 80, 200 });

        DrawLine(bx + barW*3/4, by - 4, bx + barW*3/4, by + barH + 4, (Color){ 60, 55, 50, 150 });

        DrawLine(bx + barW, by - 8, bx + barW, by + barH + 8, (Color){ 90, 85, 80, 200 });



        // Dark track background

        DrawRectangle(bx, by, barW, barH, (Color){ 14, 16, 18, 255 });

        DrawRectangleLines(bx - 1, by - 1, barW + 2, barH + 2, (Color){ 45, 30, 28, 240 });



        // Glowing blood-ember fill

        int fillW = (int)(Clamp(progress, 0.0f, 1.0f) * barW);

        if (fillW > 0) {

            DrawRectangle(bx, by, fillW, barH, (Color){ 200, 35, 25, 255 });

            DrawRectangle(bx + fillW - 3, by - 2, 4, barH + 4, (Color){ 255, 90, 70, 255 });

            DrawCircle(bx + fillW, by + barH/2, 5.0f, (Color){ 255, 120, 80, 110 });

        }



        // Active forensic step description

        DrawTextSharpCentered(g_fontBody, statusText, scrW / 2, by + 24, 18.0f, (Color){ 215, 210, 200, 250 });



        // Percentage indicator

        const char* pct = TextFormat("%02d%%", (int)(progress * 100.0f));

        DrawTextSharp(g_fontMenu, pct, bx + barW + 18, by - 11, 22.0f, (Color){ 235, 55, 45, 255 });



        // Telemetry readout

        DrawTextSharp(g_fontSmall, "ARCHIVE COORD: 44.9184° N, 71.3820° W", bx, by + 56, 14.0f, (Color){ 130, 125, 120, 190 });

        const char* incTag = "INCIDENT FILE: #09-B // EVIDENCE LOGGED";

        float incW = MeasureTextSharp(g_fontSmall, incTag, 14.0f);

        DrawTextSharp(g_fontSmall, incTag, bx + barW - incW, by + 56, 14.0f, (Color){ 130, 125, 120, 190 });



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

    SetSoundVolume(g_sndNozzleLatch, 0.70f);
    SetSoundVolume(g_sndNozzleShutoff, 0.85f);

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

    camera.position = (Vector3){ CHUNK_W/2.0f, 11.65f, CHUNK_D/2.0f };

    camera.target = (Vector3){ CHUNK_W/2.0f, 12.2f, CHUNK_D/2.0f + 1.0f };

    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };

    camera.fovy = 60.0f;

    camera.projection = CAMERA_PERSPECTIVE;

    

    Shader instancedShader = LoadShaderFromMemory(instancedVS, instancedFS);

    instancedShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(instancedShader, "mvp");

    instancedShader.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(instancedShader, "matView");

    instancedShader.locs[SHADER_LOC_MATRIX_PROJECTION] = GetShaderLocation(instancedShader, "matProjection");

    

    int uvOffsetLoc = GetShaderLocation(instancedShader, "uvOffset");

    int uvScaleLoc = GetShaderLocation(instancedShader, "uvScale");

    int timeLoc = GetShaderLocation(instancedShader, "time");

    int lightPosLoc = GetShaderLocation(instancedShader, "lightPos");

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

                    v.fgColor = (Color){10, 7, 5, 255}; 

                } else if (y == surfaceY) {

                    v.isSolid = true;

                    if (x <= 34) {

                        v.glyphIndex = 0; // ZERO ASCII characters in ocean seabed (smooth floor mesh rendered instead)

                    } else if (isRoad) {

                        v.glyphIndex = '#';

                        v.fgColor = (Color){20, 20, 22, 255}; 

                    } else if (x <= 42) {

                        v.glyphIndex = ',';

                        v.fgColor = (Color){ 48, 46, 42, 255 }; // Coastal pebble beach

                    } else {

                        v.glyphIndex = (GetRandomValue(0, 1) == 0) ? '|' : '/';

                        v.fgColor = (Color){50, 200, 50, 255}; 

                    }

                } else {

                    v.isSolid = false;

                    v.glyphIndex = 0;

                }

            }

        }

    }



    drawLoadingBar(0.48f, "GROWING DENSE SWAMP SPRUCE FORESTS...");

    // Generate trees across the larger map

    for(int x = 5; x < CHUNK_W - 5; x += GetRandomValue(10, 25)) {

        int z = GetRandomValue(5, CHUNK_D - 5);

        

        float dx = (float)x - (CHUNK_W / 2.0f);

        float dz = (float)z - (CHUNK_D / 2.0f);

        if (sqrtf(dx*dx + dz*dz) < 15.0f) continue;

        if (fabs(dx) < 14.0f) continue; // Keep dual-lane road and central station clear of trees

        if (x >= 75 && x <= 118 && z >= 118 && z <= 162) continue; // Keep trees off shop lot

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

                    v.fgColor = (i == 0) ? (Color){80, 50, 30, 255} : (Color){60, 35, 20, 255}; 

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

                            v.fgColor = (Color){30, (unsigned char)GetRandomValue(150, 220), 50, 255};

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

            // Keep the exact center clearing relatively clear

            float dx = (float)x - (CHUNK_W / 2.0f);

            float dz = (float)z - (CHUNK_D / 2.0f);

            if (sqrtf(dx*dx + dz*dz) < 18.0f) continue;

            if (fabs(dx) < 14.0f) continue; // Keep grass off dual-lane road

            if (x >= 75 && x <= 127 && z >= 118 && z <= 162) continue; // Keep grass off gas station and shop lot

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

                                v.fgColor = (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 254};

                            } else {

                                // Lower blade body (or normal 1-block extra grass): vibrant green

                                v.glyphIndex = grassGlyphs[GetRandomValue(0, 7)];

                                int r = GetRandomValue(30, 70);

                                int g = GetRandomValue(160, 220);

                                int b = GetRandomValue(30, 70);

                                v.fgColor = (Color){(unsigned char)r, (unsigned char)g, (unsigned char)b, 255};

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

    

    float swingTimer = 0.0f;

    float walkTime = 0.0f;

    float bobAmplitude = 0.0f;

    float digShake = 0.0f;

    float hitStopTimer = 0.0f;

    bool isThirdPerson = false;

    Camera3D renderCam = camera;

    Vector3 playerVel = {0.0f, 0.0f, 0.0f};



    // Gas Station, Shopkeeper & Front Roof Wall CCTV State

    bool isRoofCamActive = false;

    float roofCamSlideX  = 0.0f;   // Sliding along front roof wall rail (-3.2 to +3.2)

    float roofCamPitch   = -14.0f; // Tilt angle (-65 to +28 deg)

    float roofCamYaw     = 0.0f;   // Pan angle (-85 to +85 deg)

    float roofCamFOV     = 65.0f;  // Optical zoom (15 to 80 deg)

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

    float grethnarCooldownTimer = 0.0f;

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

    bool isShopOpen = false;

    bool playerHasDuplicateKey = false;

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



// INTRO CINEMATIC - Wrapped into callable lambda for seamless 3D Menu integration

auto RunIntroCinematic = [&]() {

    // ----------------------------------------------------------

    // INTRO PHASE ENUM

    // ----------------------------------------------------------

    

    const char* glassShaderCode = R"(

    #version 330

    in vec2 fragTexCoord;

    in vec4 fragColor;

    out vec4 finalColor;

    uniform sampler2D texture0;

    uniform float iTime;

    void main() {

        vec2 uv = fragTexCoord;

        // Subtle lens distortion

        vec2 cc = uv - vec2(0.5);

        float dist = dot(cc, cc);

        vec2 dUV = uv + cc * (dist * 0.12);

        

        vec4 base = texture(texture0, dUV);

        

        // Tree shadows (scrolling noise fake)

        float shadows = sin(dUV.x * 20.0 + iTime * 2.0) * sin(dUV.y * 15.0 - iTime * 1.5);

        shadows = smoothstep(0.4, 1.0, shadows);

        base.rgb -= vec3(0.05, 0.08, 0.05) * shadows * 0.4;

        

        // Edge darkening (vignette)

        base.rgb *= (1.0 - dist * 0.3);

        

        finalColor = base;

    }

    )";

    Shader glassShader = LoadShaderFromMemory(nullptr, glassShaderCode);

    int glassTimeLoc = GetShaderLocation(glassShader, "iTime");



    enum IntroPhase {

        INTRO_AERIAL = 0,       // Bird's eye — car rolling on road

        INTRO_DRIVER_POV,       // Driver's FP — road ahead, rain

        INTRO_JERK_1,           // First engine jolt

        INTRO_JERK_2,           // Second jolt (stronger)

        INTRO_JERK_3_STOP,      // Third jolt — car stops dead

        INTRO_DOOR_OPEN,        // Driver opens door (FP→3P transition)

        INTRO_EXIT_CAR,         // Driver walks to car front (3P)

        INTRO_SISTER_EXIT,      // Sister exits (3P)

        INTRO_LOOK_AROUND,      // FP look left→right vignette

        INTRO_FADE_TO_GAME,     // Fade to black → game begins

        INTRO_DONE

    } iPhase = INTRO_AERIAL;



    // ----------------------------------------------------------

    // INTRO STATE VARIABLES

    // ----------------------------------------------------------

    float iTimer      = 0.0f;

    float iCarZ       = 20.0f;       // Car advances along Z

    float iCarX       = 128.0f;      // Road center X

    float iCarY       = 10.0f;       // Road surface Y (matches terrain base)

    float iCarSpd     = 12.0f;       // Car speed (units/sec along Z)

    float iWheelAng   = 0.0f;        // Wheel rotation accumulator

    float iWiperT     = 0.0f;        // Wiper angle (from sinf)

    float iShakeX     = 0.0f;        // Camera shake X

    float iShakeY     = 0.0f;        // Camera shake Y

    float iShakeDec   = 0.0f;        // Shake decay

    int   iJerkCount  = 0;           // How many jerks fired (0,1,2,3)

    float iNextJerkT  = 3.2f;        // Time until next jerk fires

    float iDoorAng    = 0.0f;        // Door open angle (0-90°)

    float iPlayerT    = 0.0f;        // Player walk progress (0→1)

    float iSisterT    = 0.0f;        // Sister walk progress (0→1)

    bool  iSisterOut  = false;

    float iLookYaw    = 0.0f;        // Look around yaw angle

    int   iLookStep   = 0;           // Sub-step in look sequence

    float iEyeT       = 0.0f;        // Eyelid blink amount (0=open, 1=shut)

    float iFadeAlpha  = 0.0f;        // Black fade overlay

    float iCarSpdLerp = 12.0f;       // Smoothed car speed for jerk feel

    float iFOVExtra   = 0.0f;        // Extra FOV for jerk zoom punch

    float iJerkPushZ  = 0.0f;        // Forward lurch offset for camera

    float iMirrorFlip = 1.0f;        // Mirror lateral flip (always -1 for real mirror)



    // Windshield water drops

    struct WDrop { Vector3 worldPos; float life; float slideSpd; };

    std::vector<WDrop> wDrops;



    // ----------------------------------------------------------

    // CAMERA SETUP

    // ----------------------------------------------------------

    Camera3D iCam = { 0 };

    iCam.up         = { 0.0f, 1.0f, 0.0f };

    iCam.fovy       = 55.0f;

    iCam.projection = CAMERA_PERSPECTIVE;

    // Start above and behind the car

    iCam.position   = { iCarX, iCarY + 16.0f, iCarZ - 14.0f };

    iCam.target     = { iCarX, iCarY + 1.0f,  iCarZ + 2.0f  };



    // Smooth camera interpolation targets

    Vector3 iCamPosTarget = iCam.position;

    Vector3 iCamTgtTarget = iCam.target;

    float   iCamFOVTarget = 55.0f;

    float   iCamLerpSpd   = 8.0f;   // How fast camera lerps to target



    // ----------------------------------------------------------

    // CAR GEOMETRY — Ford-like proportions

    // Coordinate system: car center at (iCarX, iCarY + carBodyCY, iCarZ)

    // Road surface = iCarY, wheel radius = 0.38

    // Body bottom = iCarY + 0.38 + 0.05 (clearance) = iCarY + 0.43

    // Body half-height = 0.50  →  body center Y = iCarY + 0.93

    // ----------------------------------------------------------

    const float kWheelR     = 0.38f;

    const float kClearance  = 0.08f;

    const float kBodyHalfH  = 0.52f;

    const float kBodyCY     = kWheelR + kClearance + kBodyHalfH; // ~0.98



    // Helper: carBodyCenterY absolute

    auto carCY = [&]() -> float { return iCarY + kBodyCY; };



    // Main body — wide & boxy (old Ford)

    Mesh mBody      = GenMeshCube(2.4f, 1.04f, 5.2f);

    // Cabin / greenhouse (narrower, shorter, set back)

    Mesh mCabin     = GenMeshCube(2.0f, 0.72f, 2.8f);

    // Trunk hump

    Mesh mTrunk     = GenMeshCube(2.1f, 0.36f, 1.4f);

    // Hood slope

    Mesh mHood      = GenMeshCube(2.1f, 0.18f, 1.6f);

    // Front grille

    Mesh mGrille    = GenMeshCube(2.2f, 0.62f, 0.18f);

    // Wheels (cylinder along X axis — will be rotated)

    Mesh mWheel     = GenMeshCylinder(kWheelR, 0.28f, 18);

    // Left door panel (hinge-pivot animation)

    Mesh mDoor      = GenMeshCube(0.12f, 0.90f, 2.0f);

    // Dashboard inside

    Mesh mDash      = GenMeshCube(2.1f, 0.30f, 0.65f);

    // Seats

    Mesh mSeat      = GenMeshCube(0.62f, 0.75f, 0.68f);

    Mesh mSeatBack  = GenMeshCube(0.62f, 0.75f, 0.14f);

    // Road plane (long, centered on road path)



    // Side mirrors (small boxes)

    Mesh mMirror    = GenMeshCube(0.10f, 0.10f, 0.22f);

    // Bumper front

    Mesh mBumper    = GenMeshCube(2.5f, 0.22f, 0.22f);

    // Fender arches (flattened sphere approximated by scaled sphere... use cube)

    Mesh mFender    = GenMeshCube(0.30f, 0.18f, 0.90f);

    // Steering wheel approximated with torus (use cylinder disc)

    Mesh mSteerRim  = GenMeshCylinder(0.26f, 0.03f, 20);

    // Rear-view mirror bar inside

    Mesh mRVMFrame  = GenMeshCube(0.60f, 0.08f, 0.04f);

    // Real car front details: Dual headlights & amber indicators

    Mesh mHeadlight = GenMeshCube(0.34f, 0.22f, 0.12f);

    Mesh mIndicator = GenMeshCube(0.18f, 0.18f, 0.12f);



    // Materials

    auto makeMat = [](Color c) -> Material {

        Material m = LoadMaterialDefault();

        m.maps[MATERIAL_MAP_DIFFUSE].color = c;

        return m;

    };

        // Car Exterior — Sleek Automotive Metallic Silver Palette

    Material matBody    = makeMat({138, 144, 154, 255}); // Liquid metallic silver body

    Material matCabin   = makeMat({118, 124, 134, 255}); // Dark metallic silver greenhouse & pillars

    Material matHood    = makeMat({150, 156, 166, 255}); // Metallic silver sculpted hood

    Material matDoor    = makeMat({132, 138, 148, 255}); // Metallic silver door panels

    Material matTrunk   = makeMat({126, 132, 142, 255}); // Metallic silver rear trunk

    Material matGlass   = makeMat({60,   85, 115, 175}); // Deep automotive tinted glass

    Material matWheel   = makeMat({20,   20,  22, 255}); // Matte tire rubber

    Material matHub     = makeMat({185, 192, 204, 255}); // Bright machined aluminum hubcap

    Material matInterior= makeMat({28,   30,  34, 255}); // Charcoal cabin interior

    Material matSeat    = makeMat({46,   44,  42, 255}); // Dark leather seats

    Material matChrome  = makeMat({225, 230, 240, 255}); // Chrome front/rear bumper

    Material matGrille  = makeMat({22,   24,  28, 255}); // Deep black honeycomb grille

    Material matHeadlit = makeMat({255, 252, 220, 255}); // Warm halogen headlight

    Material matBezel   = makeMat({160, 168, 178, 255}); // Chrome headlight housing

    Material matAmber   = makeMat({235, 150,  30, 255}); // Amber indicator lights

    Material matMirrorF = makeMat({45,   48,  55, 255}); // Mirror frame

    Material matBrake   = makeMat({205,  35,  35, 255}); // Ruby red brake light



    RenderTexture2D mirrorRT = LoadRenderTexture(512, 192);

    SetTextureFilter(mirrorRT.texture, TEXTURE_FILTER_BILINEAR);



    // ----------------------------------------------------------

    // FORCE RAIN ACTIVE FOR ENTIRE INTRO

    // ----------------------------------------------------------

    isStormActive  = true;

    stormDuration  = 99999.0f;

    PlaySound(sndRain);



    // ----------------------------------------------------------

    // HELPER LAMBDAS

    // ----------------------------------------------------------



    // Absolute driver eye position (Driver bucket seat, sitting back from dash)

    auto driverEyePos = [&]() -> Vector3 {

        return { iCarX - 0.50f, carCY() + 0.35f, iCarZ + 0.15f };

    };

    // Absolute sister eye position (Passenger bucket seat, identical height and depth)

    auto sisterEyePos = [&]() -> Vector3 {

        return { iCarX + 0.50f, carCY() + 0.35f, iCarZ + 0.15f };

    };

    // Driver standing position (outside, front-left of car)

    auto driverStandPos = [&](float t) -> Vector3 {

        Vector3 inSeat  = { iCarX - 0.50f, iCarY + 1.45f, iCarZ + 0.20f };

        Vector3 outside = { iCarX - 1.65f, iCarY + 1.65f, iCarZ + 0.40f };

        Vector3 atFront = { iCarX - 0.75f, iCarY + 1.65f, iCarZ + 3.80f };

        if (t < 0.35f) {

            float subT = t / 0.35f;

            return Vector3Lerp(inSeat, outside, subT);

        } else {

            float subT = (t - 0.35f) / 0.65f;

            return Vector3Lerp(outside, atFront, subT);

        }

    };

    // Sister standing position (outside, front-right of car)

    auto sisterStandPos = [&](float t) -> Vector3 {

        Vector3 inSeat  = { iCarX + 0.50f, iCarY + 1.45f, iCarZ + 0.20f };

        Vector3 outside = { iCarX + 1.65f, iCarY + 1.65f, iCarZ + 0.40f };

        Vector3 atFront = { iCarX + 0.75f, iCarY + 1.65f, iCarZ + 3.80f };

        if (t < 0.35f) {

            float subT = t / 0.35f;

            return Vector3Lerp(inSeat, outside, subT);

        } else {

            float subT = (t - 0.35f) / 0.65f;

            return Vector3Lerp(outside, atFront, subT);

        }

    };



    // Apply camera shake (additive displacement)

    auto applyShake = [&](Vector3& pos, Vector3& tgt) {

        if (iShakeDec > 0.001f) {

            float sx = ((GetRandomValue(-100,100)/100.0f)) * iShakeDec * 0.12f;

            float sy = ((GetRandomValue(-100,100)/100.0f)) * iShakeDec * 0.08f;

            float sz = ((GetRandomValue(-100,100)/100.0f)) * iShakeDec * 0.05f;

            pos.x += sx; pos.y += sy; pos.z += sz;

            tgt.x += sx * 0.4f; tgt.y += sy * 0.4f;

        }

    };



    // ----------------------------------------------------------

    // INTRO MAIN LOOP

    // ----------------------------------------------------------

    while (!WindowShouldClose() && iPhase != INTRO_DONE) {

        if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {

            iPhase = INTRO_DONE;

            break;

        }

        if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER))) {

            ToggleGameFullscreen();

        }

        float dt      = GetFrameTime();

        float timeVal = (float)GetTime();





        iTimer       += dt;

        iWheelAng    -= (iCarSpd / kWheelR) * dt;   // Wheel roll

        iWiperT       = sinf(timeVal * 4.2f);         // Wiper oscillation

        iShakeDec     = Lerp(iShakeDec, 0.0f, dt * 5.5f);

        iFOVExtra     = Lerp(iFOVExtra, 0.0f, dt * 6.0f);

        iJerkPushZ    = Lerp(iJerkPushZ, 0.0f, dt * 7.0f);

        iCarSpdLerp   = Lerp(iCarSpdLerp, iCarSpd, dt * 3.0f);



        bool driverFP = (iPhase == INTRO_DRIVER_POV ||

                         iPhase == INTRO_JERK_1     ||

                         iPhase == INTRO_JERK_2     ||

                         iPhase == INTRO_JERK_3_STOP);

        bool interiorVisible = driverFP;



        // ---- STATE MACHINE ----

        switch (iPhase) {



        // ---- AERIAL — Third person above, car drives forward ----

        case INTRO_AERIAL:

            iCarZ += iCarSpd * dt;

            iCamPosTarget = { iCarX,         iCarY + 17.0f, iCarZ - 15.0f };

            iCamTgtTarget = { iCarX,         iCarY + 1.2f,  iCarZ +  2.0f };

            iCamFOVTarget = 50.0f;

            iCamLerpSpd   = 3.0f; // Slow graceful follow

            if (iTimer >= 4.0f) { 

                iPhase = INTRO_DRIVER_POV; 

                iTimer = 0.0f; 

                iNextJerkT = 3.0f; 

            }

            break;



        // ---- SISTER POV — right seat looking left at driver ----

        case INTRO_DRIVER_POV: {

            iCarZ += iCarSpd * dt;

            Vector3 eye = driverEyePos();

            iCamPosTarget = { eye.x + iShakeX, eye.y + iShakeY, eye.z + iJerkPushZ };

            iCamTgtTarget = { eye.x + iShakeX * 0.3f,

                               eye.y + iShakeY * 0.3f - 0.06f,

                               iCarZ + 8.0f };

            iCamFOVTarget = 68.0f + iFOVExtra;

            iCamLerpSpd   = 20.0f;



            iNextJerkT -= dt;

            if (iNextJerkT <= 0.0f) {

                // First jerk

                iJerkCount = 1;

                iShakeDec  = 3.8f;

                iFOVExtra  = 22.0f;

                iJerkPushZ = 0.35f;

                iCarSpd    = 4.0f; // Car briefly slows

                PlaySound(sndThunder);

                lightningFlashTimer = 0.9f;

                lightningSegments.clear();

                Vector3 ls = { iCarX + (float)GetRandomValue(-20,20), iCarY + 90.0f, iCarZ + (float)GetRandomValue(-10,10) };

                Vector3 le = { ls.x, iCarY + 0.5f, ls.z };

                GenerateLightningBolt(ls, le, 4, lightningSegments);

                iPhase = INTRO_JERK_1;

                iTimer = 0.0f;

            }

            break;

        }



        // ---- JERK 1 — car recovers briefly ----

        case INTRO_JERK_1:

            iCarZ += iCarSpd * dt;

            iCarSpd = Lerp(iCarSpd, 10.0f, dt * 1.5f); // Recover speed

            {

                Vector3 eye = driverEyePos();

                iCamPosTarget = { eye.x + iShakeX, eye.y + iShakeY, eye.z + iJerkPushZ };

                iCamTgtTarget = { eye.x + iShakeX * 0.3f, eye.y - 0.06f, iCarZ + 8.0f };

                iCamFOVTarget = 68.0f + iFOVExtra;

                iCamLerpSpd   = 20.0f;

            }

            if (iTimer >= 2.8f) {

                // Second jerk — stronger

                iJerkCount = 2;

                iShakeDec  = 5.5f;

                iFOVExtra  = 30.0f;

                iJerkPushZ = 0.55f;

                iCarSpd    = 2.5f;

                PlaySound(sndThunder);

                lightningFlashTimer = 1.2f;

                lightningSegments.clear();

                Vector3 ls = { iCarX + (float)GetRandomValue(-15,15), iCarY + 85.0f, iCarZ + (float)GetRandomValue(-5,15) };

                Vector3 le = { ls.x, iCarY + 0.5f, ls.z };

                GenerateLightningBolt(ls, le, 5, lightningSegments);

                iPhase = INTRO_JERK_2;

                iTimer = 0.0f;

            }

            break;



        // ---- JERK 2 — car struggles, nearly stops ----

        case INTRO_JERK_2:

            iCarSpd = Lerp(iCarSpd, 7.0f, dt * 1.2f);

            iCarZ += iCarSpd * dt;

            {

                Vector3 eye = driverEyePos();

                iCamPosTarget = { eye.x + iShakeX, eye.y + iShakeY, eye.z + iJerkPushZ };

                iCamTgtTarget = { eye.x + iShakeX * 0.3f, eye.y - 0.06f, iCarZ + 8.0f };

                iCamFOVTarget = 68.0f + iFOVExtra;

                iCamLerpSpd   = 20.0f;

            }

            if (iTimer >= 3.0f) {

                // Third jerk — car STOPS

                iJerkCount = 3;

                iShakeDec  = 8.0f;

                iFOVExtra  = 38.0f;

                iJerkPushZ = 0.80f;   // Strong forward lurch

                iCarSpd    = 0.0f;    // CAR STOPS

                PlaySound(sndThunder);

                lightningFlashTimer = 1.6f;

                lightningSegments.clear();

                Vector3 ls = { iCarX + (float)GetRandomValue(-10,10), iCarY + 80.0f, iCarZ + (float)GetRandomValue(0,20) };

                Vector3 le = { ls.x, iCarY + 0.5f, ls.z };

                GenerateLightningBolt(ls, le, 5, lightningSegments);

                iPhase = INTRO_JERK_3_STOP;

                iTimer = 0.0f;

            }

            break;



        // ---- JERK 3 STOP — car dead, camera settles ----

        case INTRO_JERK_3_STOP: {

            // Car does NOT move

            Vector3 eye = driverEyePos();

            iCamPosTarget = { eye.x + iShakeX * 0.5f, eye.y + iShakeY * 0.5f, eye.z + iJerkPushZ };

            iCamTgtTarget = { eye.x, eye.y - 0.05f, iCarZ + 8.0f };

            iCamFOVTarget = 68.0f + iFOVExtra;

            iCamLerpSpd   = 16.0f;

            if (iTimer >= 2.8f) {

                iPhase = INTRO_DOOR_OPEN;

                iTimer = 0.0f;

                iDoorAng = 0.0f;

            }

            break;

        }



        // ---- 1ST PERSON CAR EXIT PHASES ----

        case INTRO_DOOR_OPEN: {

            float targetDoor = 78.0f;

            iDoorAng = Lerp(iDoorAng, targetDoor, dt * 2.4f);



            // 1st Person: Driver turns head left and watches door swing open into the rain

            float t = Clamp(iTimer / 2.0f, 0.0f, 1.0f);

            float s = t * t * (3.0f - 2.0f * t);



            Vector3 eye = driverEyePos();

            Vector3 forwardTgt = { eye.x, eye.y - 0.05f, iCarZ + 8.0f };

            Vector3 doorTgt    = { iCarX - 2.5f, eye.y - 0.10f, iCarZ + 0.5f };



            // Lean slightly toward the door as it pushes open

            iCamPosTarget = { eye.x - s * 0.20f, eye.y, eye.z + s * 0.10f };

            iCamTgtTarget = Vector3Lerp(forwardTgt, doorTgt, s);

            iCamFOVTarget = 70.0f;

            iCamLerpSpd   = 16.0f;



            if (iTimer >= 2.2f) {

                iPhase = INTRO_EXIT_CAR;

                iTimer = 0.0f;

                iPlayerT = 0.0f;

            }

            break;

        }



        // ---- EXIT CAR — driver physically steps out and walks to front (1st Person) ----

        case INTRO_EXIT_CAR: {

            iPlayerT += dt * 0.30f;

            if (iPlayerT > 1.0f) iPlayerT = 1.0f;



            // Continuous 1st Person camera moving out of door and walking to front

            Vector3 inSeat   = { iCarX - 0.70f, carCY() + 0.35f, iCarZ + 0.25f };

            Vector3 outside  = { iCarX - 1.55f, iCarY + 1.75f,   iCarZ + 0.40f };

            Vector3 frontBmp = { iCarX - 0.40f, iCarY + 1.75f,   iCarZ + 4.20f };



            Vector3 eyePos;

            Vector3 eyeTgt;



            if (iPlayerT < 0.35f) {

                // Step 1: Swing out through open door onto the ground

                float subT = iPlayerT / 0.35f;

                float s = subT * subT * (3.0f - 2.0f * subT);

                eyePos = Vector3Lerp(inSeat, outside, s);

                // Head dips slightly then rises to standing height

                eyePos.y += sinf(subT * PI) * 0.06f;



                Vector3 lookGround = { iCarX - 1.8f, iCarY + 0.3f, iCarZ + 1.5f };

                Vector3 lookAhead  = { iCarX - 1.0f, iCarY + 1.5f, iCarZ + 5.0f };

                eyeTgt = Vector3Lerp(lookGround, lookAhead, s);

            } else {

                // Step 2: Walk forward along the front fender to the front bumper

                float subT = (iPlayerT - 0.35f) / 0.65f;

                float s = subT * subT * (3.0f - 2.0f * subT);

                eyePos = Vector3Lerp(outside, frontBmp, s);

                // Subtle rhythmic walking head-bob

                eyePos.y += sinf(subT * 16.0f) * 0.04f;



                eyeTgt = { iCarX - 0.30f, iCarY + 1.60f, iCarZ + 12.0f };

            }



            iCamPosTarget = eyePos;

            iCamTgtTarget = eyeTgt;

            iCamFOVTarget = 70.0f;

            iCamLerpSpd   = 18.0f;



            if (iTimer >= 3.6f) {

                iPhase = INTRO_SISTER_EXIT;

                iTimer = 0.0f;

                iSisterT = 0.0f;

            }

            break;

        }



        // ---- SISTER EXIT — player in 1st person watches sister exit and join at front ----

        case INTRO_SISTER_EXIT: {

            iSisterT += dt * 0.28f;

            if (iSisterT > 1.0f) iSisterT = 1.0f;

            if (iTimer >= 0.6f) iSisterOut = true;



            // Player stands at front bumper, looking back towards passenger side

            Vector3 standPos = { iCarX - 0.35f, iCarY + 1.76f, iCarZ + 4.20f };

            Vector3 sPos     = sisterStandPos(iSisterT);



            // Camera is focused right on sister as she gets out and walks to the front

            iCamPosTarget = standPos;

            iCamTgtTarget = { sPos.x, sPos.y, sPos.z };

            iCamFOVTarget = 68.0f;

            iCamLerpSpd   = 14.0f;



            if (iTimer >= 3.6f) {

                iPhase = INTRO_LOOK_AROUND;

                iTimer = 0.0f;

                iLookStep = 0;

                iLookYaw  = 0.0f;

            }

            break;

        }



        // ---- LOOK AROUND — FP, vignette blink ----

        case INTRO_LOOK_AROUND: {

            // Standing in front of car at eye height

            float eyeX = iCarX - 0.30f;

            float eyeY = iCarY + 1.78f;

            float eyeZ = iCarZ + 4.2f;

            iCamPosTarget = { eyeX, eyeY, eyeZ };

            iCamTgtTarget = { eyeX + sinf(iLookYaw) * 4.0f,

                               eyeY - 0.04f,

                               eyeZ + cosf(iLookYaw) * 4.0f };

            iCamFOVTarget = 68.0f;

            iCamLerpSpd   = 18.0f;



            switch (iLookStep) {

            case 0: // Sweep right

                iLookYaw = Lerp(iLookYaw, 0.90f, dt * 2.0f);

                if (fabsf(iLookYaw - 0.90f) < 0.05f) { iLookStep = 1; iTimer = 0.0f; }

                break;

            case 1: // Blink right

                iEyeT = (iTimer < 0.35f) ? Lerp(iEyeT, 1.0f, dt * 12.0f)

                                           : Lerp(iEyeT, 0.0f, dt * 10.0f);

                if (iTimer >= 0.85f) { iLookStep = 2; iTimer = 0.0f; }

                break;

            case 2: // Sweep left

                iLookYaw = Lerp(iLookYaw, -0.90f, dt * 2.0f);

                if (fabsf(iLookYaw - (-0.90f)) < 0.05f) { iLookStep = 3; iTimer = 0.0f; }

                break;

            case 3: // Blink left

                iEyeT = (iTimer < 0.35f) ? Lerp(iEyeT, 1.0f, dt * 12.0f)

                                           : Lerp(iEyeT, 0.0f, dt * 10.0f);

                if (iTimer >= 0.85f) { iLookStep = 4; iTimer = 0.0f; }

                break;

            default: // Return to center, then fade

                iLookYaw = Lerp(iLookYaw, 0.0f, dt * 2.5f);

                if (iTimer >= 1.8f) { iPhase = INTRO_FADE_TO_GAME; iTimer = 0.0f; }

                break;

            }

            break;

        }



        // ---- FADE TO GAME ----

        case INTRO_FADE_TO_GAME: {

            iFadeAlpha += dt * 0.65f;

            if (iFadeAlpha >= 1.0f) { iFadeAlpha = 1.0f; iPhase = INTRO_DONE; }

            float eyeX = iCarX - 0.30f, eyeY = iCarY + 1.78f, eyeZ = iCarZ + 4.2f;

            iCamPosTarget = { eyeX, eyeY, eyeZ };

            iCamTgtTarget = { eyeX, eyeY - 0.04f, eyeZ + 3.5f };

            iCamFOVTarget = 68.0f;

            iCamLerpSpd   = 10.0f;

            break;

        }

        default: break;

        }



        // ---- SMOOTH CAMERA LERP ----

        float ls = iCamLerpSpd * dt;

        if (ls > 1.0f) ls = 1.0f;

        iCam.position = Vector3Lerp(iCam.position, iCamPosTarget, ls);

        iCam.target   = Vector3Lerp(iCam.target,   iCamTgtTarget, ls);

        iCam.fovy     = Lerp(iCam.fovy, iCamFOVTarget, ls);



        // Apply random shake displacement on top

        Vector3 shakeCamPos = iCam.position;

        Vector3 shakeCamTgt = iCam.target;

        applyShake(shakeCamPos, shakeCamTgt);



        // ---- RAIN UPDATE (use center camera for rain source) ----

        camera.position = { iCarX, iCarY + 2.0f, iCarZ };



        if (lightningFlashTimer > 0.0f) {

            lightningFlashTimer -= dt * 2.2f;

            if (lightningFlashTimer < 0.0f) lightningFlashTimer = 0.0f;

        }



        // Spawn rain around car

        for (int p = 0; p < 14; p++) {

            RainParticle rp;

            float ang = GetRandomValue(-180, 180) * DEG2RAD;

            float dst = GetRandomValue(40, 1800) / 100.0f;

            rp.pos.x = iCarX + cosf(ang) * dst;

            rp.pos.z = iCarZ + sinf(ang) * dst;

            rp.pos.y = iCarY + 18.0f + GetRandomValue(0, 50) / 10.0f;

            rp.life  = 1.0f;

            rp.glyph = (GetRandomValue(0,1) == 0) ? '|' : ',';

            rainParticles.push_back(rp);

        }

        for (int i = 0; i < (int)rainParticles.size(); ) {

            rainParticles[i].pos.y -= dt * 44.0f;

            rainParticles[i].life  -= dt * 1.1f;

            if (rainParticles[i].pos.y < iCarY || rainParticles[i].life <= 0.0f) {

                rainParticles[i] = rainParticles.back();

                rainParticles.pop_back();

            } else { i++; }

        }



        // Build rain instances

        for (int i = 0; i < 256; i++) rainInstances[i].clear();

        for (const auto& rp : rainParticles) {

            Matrix m = MatrixIdentity();

            m.m0 = 140/255.0f; m.m1 = 170/255.0f; m.m2 = 1.0f; m.m3 = rp.life * 0.60f;

            m.m4 = 0.45f; m.m5 = 0.45f;

            m.m8 = 0.0f; m.m9 = 0.0f; m.m10 = 0.0f; m.m11 = 1.0f;

            m.m12 = rp.pos.x; m.m13 = rp.pos.y; m.m14 = rp.pos.z;

            rainInstances[(uint8_t)rp.glyph].push_back(m);

        }



        // ---- SOUND LOOP ----

        if (!IsSoundPlaying(sndRain))    PlaySound(sndRain);

        if (!IsSoundPlaying(sndWind))    PlaySound(sndWind);

        if (!IsSoundPlaying(sndCrickets)) PlaySound(sndCrickets);



        // ===========================================================

        // REARVIEW MIRROR PRE-PASS (Secondary Camera looking backward)

        // ===========================================================

        if (driverFP) {

            Camera3D mCam = { 0 };

            // Mirror camera: placed at windshield center rearview mirror mount

            // looking backward (180° opposite from car forward +Z)

            mCam.position   = { iCarX, carCY() + 0.52f, iCarZ + 0.90f };

            mCam.target     = { iCarX, carCY() + 0.46f, iCarZ - 20.0f };

            mCam.up         = { 0.0f, 1.0f, 0.0f };

            mCam.fovy       = 58.0f;

            mCam.projection = CAMERA_PERSPECTIVE;



            BeginTextureMode(mirrorRT);

            ClearBackground({ 14, 18, 26, 255 });

            BeginMode3D(mCam);



            // 1. Instanced terrain behind car (Memory Optimization: static bucket reuse)

            BeginShaderMode(instancedShader);

            Vector2 uvScl = { 1.0f / 16.0f, 1.0f / 16.0f };

            SetShaderValue(instancedShader, uvScaleLoc,       &uvScl,                SHADER_UNIFORM_VEC2);

            SetShaderValue(instancedShader, timeLoc,          &timeVal,              SHADER_UNIFORM_FLOAT);

            SetShaderValue(instancedShader, playerPosLoc,     &camera.position,      SHADER_UNIFORM_VEC3);

        float introDayFactor = 0.0f;

        Vector3 introSunDir = { 0.0f, -1.0f, 0.0f };

        Vector3 introSunColor = { 1.0f, 0.96f, 0.85f };

        SetShaderValue(instancedShader, sunDirLoc, &introSunDir, SHADER_UNIFORM_VEC3);

        SetShaderValue(instancedShader, dayFactorLoc, &introDayFactor, SHADER_UNIFORM_FLOAT);

        SetShaderValue(instancedShader, sunColorLoc, &introSunColor, SHADER_UNIFORM_VEC3);

        Vector3 introMoonDir = { 0.0f, 1.0f, 0.0f };

        float introNightFactor = 1.0f;

        SetShaderValue(instancedShader, moonDirLoc, &introMoonDir, SHADER_UNIFORM_VEC3);

        SetShaderValue(instancedShader, nightFactorLoc, &introNightFactor, SHADER_UNIFORM_FLOAT);

            

            static std::vector<RenderBucket*> mirrorBuckets;

            mirrorBuckets.clear();

            for (int bx = 0; bx < BUCKETS_X; bx++) {

                for (int bz = 0; bz < BUCKETS_Z; bz++) {

                    float centerX = bx * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);

                    float centerZ = bz * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);

                    float dx = centerX - mCam.position.x;

                    float dz = centerZ - mCam.position.z;

                    // dz < 10.0f means behind the car

                    if (dz < 10.0f && dx*dx + dz*dz < 75.0f*75.0f) {

                        mirrorBuckets.push_back(&chunk->buckets[bx][bz]);

                    }

                }

            }

            

            static std::vector<Matrix> mirrorBatchTransforms;

            static bool s_mirrorBatchInit = false;

            if (!s_mirrorBatchInit) {

                mirrorBatchTransforms.reserve(8192);

                s_mirrorBatchInit = true;

            }

            static bool s_mirrorGlyphUsed[256];

            memset(s_mirrorGlyphUsed, 0, sizeof(s_mirrorGlyphUsed));

            for (RenderBucket* b : mirrorBuckets) {

                for (uint8_t g : b->activeGlyphs) {

                    s_mirrorGlyphUsed[g] = true;

                }

            }

            for(int i=0; i<256; i++) {

                if (!s_mirrorGlyphUsed[i]) continue;

                mirrorBatchTransforms.clear();

                for (RenderBucket* b : mirrorBuckets) {

                    if (!b->instances[i].empty()) {

                        mirrorBatchTransforms.insert(mirrorBatchTransforms.end(), b->instances[i].begin(), b->instances[i].end());

                    }

                }

                if (mirrorBatchTransforms.empty()) continue;

                

                int col = i % 16; int row = i / 16;

                Vector2 uvOff = { col * uvScl.x, row * uvScl.y };

                SetShaderValue(instancedShader, uvOffsetLoc, &uvOff, SHADER_UNIFORM_VEC2);

                DrawMeshInstanced(quad, material, mirrorBatchTransforms.data(), (int)mirrorBatchTransforms.size());

            }

            EndShaderMode();



            // 2. Solid ground and road surface behind car (mirrored view sees it receding into distance)

            DrawMesh(mGround, matGround, MatrixTranslate(iCarX, iCarY + 0.00f, 250.0f));

            DrawMesh(mRoad, matRoad, MatrixTranslate(iCarX, iCarY + 0.01f, 250.0f));



            // Dashed center stripes receding behind car

            float stripeStart = floorf((iCarZ - 200.0f) / 8.0f) * 8.0f;

            for (float sz = stripeStart; sz < iCarZ - 1.0f; sz += 8.0f) {

                DrawLine3D({ iCarX, iCarY + 0.06f, sz }, { iCarX, iCarY + 0.06f, sz + 4.0f }, WHITE);

            }

            // Road white edges

            for (float sz = stripeStart; sz < iCarZ; sz += 2.0f) {

                DrawLine3D({ iCarX - 6.0f, iCarY + 0.06f, sz }, { iCarX - 6.0f, iCarY + 0.06f, sz + 1.5f }, { 220, 220, 220, 200 });

                DrawLine3D({ iCarX + 6.0f, iCarY + 0.06f, sz }, { iCarX + 6.0f, iCarY + 0.06f, sz + 1.5f }, { 220, 220, 220, 200 });

            }



            // 3. Cabin interior reflected in the mirror:

            // Passenger seat

            DrawMesh(mSeat, matSeat, MatrixTranslate(iCarX + 0.52f, carCY() - 0.15f, iCarZ + 0.15f));

            DrawMesh(mSeatBack, matSeat, MatrixTranslate(iCarX + 0.52f, carCY() + 0.15f, iCarZ + 0.05f));

            // Driver seat back behind driver

            DrawMesh(mSeatBack, matSeat, MatrixTranslate(iCarX - 0.50f, carCY() + 0.15f, iCarZ + 0.05f));

            // Rear cabin frame

            DrawMesh(mCabin, matCabin, MatrixMultiply(MatrixScale(0.95f, 0.95f, 0.95f), MatrixTranslate(iCarX, carCY() + kBodyHalfH + 0.36f, iCarZ - 0.25f)));



            EndMode3D();



            // 4. Reflection of characters inside the car (proportional sizing for 512x192 super-sampled RT):

            // Sister "i" in the passenger seat (reflected on passenger side of mirror)

            Vector3 sisHeadWorld = { iCarX + 0.52f, carCY() + 0.38f, iCarZ + 0.15f };

            Vector2 sisMirrorSS = GetWorldToScreenEx(sisHeadWorld, mCam, mirrorRT.texture.width, mirrorRT.texture.height);

            if (sisMirrorSS.x >= 0 && sisMirrorSS.x < mirrorRT.texture.width &&

                sisMirrorSS.y >= 0 && sisMirrorSS.y < mirrorRT.texture.height) {

                DrawText("i", (int)sisMirrorSS.x - 6, (int)sisMirrorSS.y - 14, 28, { 255, 220, 160, 245 });

            }

            // Driver "@" in driver seat (reflected on driver side of mirror)

            Vector3 drvHeadWorld = { iCarX - 0.50f, carCY() + 0.36f, iCarZ + 0.15f };

            Vector2 drvMirrorSS = GetWorldToScreenEx(drvHeadWorld, mCam, mirrorRT.texture.width, mirrorRT.texture.height);

            if (drvMirrorSS.x >= 0 && drvMirrorSS.x < mirrorRT.texture.width &&

                drvMirrorSS.y >= 0 && drvMirrorSS.y < mirrorRT.texture.height) {

                DrawText("@", (int)drvMirrorSS.x - 9, (int)drvMirrorSS.y - 16, 30, { 200, 215, 235, 245 });

            }



            EndTextureMode();

        }



        // ===========================================================

        // MAIN 3D RENDER PASS

        // ===========================================================

        // Build shake camera from lerped iCam + per-frame random shake

        Camera3D shakeCam = iCam;

        shakeCam.position = shakeCamPos;

        shakeCam.target   = shakeCamTgt;



        BeginTextureMode(target);

        ClearBackground(BLACK);

        BeginMode3D(shakeCam);



        // Shader setup for instanced rain

        Vector2 uvScl = { 1.0f / 16.0f, 1.0f / 16.0f };

        SetShaderValue(instancedShader, uvScaleLoc,       &uvScl,                SHADER_UNIFORM_VEC2);

        SetShaderValue(instancedShader, timeLoc,          &timeVal,              SHADER_UNIFORM_FLOAT);

        SetShaderValue(instancedShader, playerPosLoc,     &camera.position,      SHADER_UNIFORM_VEC3);

        float zTL[16] = { 0 }; Vector3 zTP[16] = { { 0,0,0 } };

        SetShaderValueV(instancedShader, trailPosLoc,  zTP, SHADER_UNIFORM_VEC3,  16);

        SetShaderValueV(instancedShader, trailLifeLoc, zTL, SHADER_UNIFORM_FLOAT, 16);

        SetShaderValue(instancedShader, lightningFlashLoc, &lightningFlashTimer,  SHADER_UNIFORM_FLOAT);

        float introDayFactor = 0.0f;

        Vector3 introSunDir = { 0.0f, -1.0f, 0.0f };

        Vector3 introSunColor = { 1.0f, 0.96f, 0.85f };

        SetShaderValue(instancedShader, sunDirLoc, &introSunDir, SHADER_UNIFORM_VEC3);

        SetShaderValue(instancedShader, dayFactorLoc, &introDayFactor, SHADER_UNIFORM_FLOAT);

        SetShaderValue(instancedShader, sunColorLoc, &introSunColor, SHADER_UNIFORM_VEC3);

        Vector3 introMoonDir = { 0.0f, 1.0f, 0.0f };

        float introNightFactor = 1.0f;

        SetShaderValue(instancedShader, moonDirLoc, &introMoonDir, SHADER_UNIFORM_VEC3);

        SetShaderValue(instancedShader, nightFactorLoc, &introNightFactor, SHADER_UNIFORM_FLOAT);



        // ---- ROAD ----

        // ---- TERRAIN ----

        {

            static std::vector<RenderBucket*> visibleBuckets;

            static bool s_intro1BucketsInit = false;

            if (!s_intro1BucketsInit) {

                visibleBuckets.reserve(BUCKETS_X * BUCKETS_Z);

                s_intro1BucketsInit = true;

            }

            visibleBuckets.clear();

            float cullDistSq = 65.0f * 65.0f; // 65 block render distance

            

            Vector3 camForward = Vector3Normalize(Vector3Subtract(shakeCam.target, shakeCam.position));

            camForward.y = 0.0f;

            camForward = Vector3Normalize(camForward);

            

            for (int bx = 0; bx < BUCKETS_X; bx++) {

                for (int bz = 0; bz < BUCKETS_Z; bz++) {

                    float centerX = bx * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);

                    float centerZ = bz * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);

                    float dx = centerX - shakeCam.position.x;

                    float dz = centerZ - shakeCam.position.z;

                    float distSq = dx*dx + dz*dz;

                    

                    if (distSq < cullDistSq) {

                        if (distSq < (BUCKET_SIZE * BUCKET_SIZE)) {

                            visibleBuckets.push_back(&chunk->buckets[bx][bz]);

                        } else {

                            Vector3 dirToBucket = Vector3Normalize({dx, 0.0f, dz});

                            float dotProd = Vector3DotProduct(camForward, dirToBucket);

                            if (dotProd > -0.4f) { 

                                visibleBuckets.push_back(&chunk->buckets[bx][bz]);

                            }

                        }

                    }

                }

            }

            

            // Note: uvScale is available from global scope? Wait, in Intro it might be called uvScl.

            // Let's check uvScale. In Intro it's uvScl.

            Vector2 curUvScale = { 1.0f / 16.0f, 1.0f / 16.0f };

            

            static std::vector<Matrix> intro1BatchTransforms;

            static bool s_intro1BatchInit = false;

            if (!s_intro1BatchInit) {

                intro1BatchTransforms.reserve(16384);

                s_intro1BatchInit = true;

            }

            static bool s_intro1GlyphUsed[256];

            memset(s_intro1GlyphUsed, 0, sizeof(s_intro1GlyphUsed));

            for (RenderBucket* b : visibleBuckets) {

                for (uint8_t g : b->activeGlyphs) {

                    s_intro1GlyphUsed[g] = true;

                }

            }

            for(int i=0; i<256; i++) {

                if (!s_intro1GlyphUsed[i]) continue;

                intro1BatchTransforms.clear();

                for (RenderBucket* b : visibleBuckets) {

                    if (!b->instances[i].empty()) {

                        intro1BatchTransforms.insert(intro1BatchTransforms.end(), b->instances[i].begin(), b->instances[i].end());

                    }

                }

                if (intro1BatchTransforms.empty()) continue;

                

                int col = i % 16;

                int row = i / 16;

                Vector2 uvOffset = {col * curUvScale.x, row * curUvScale.y};

                SetShaderValue(instancedShader, uvOffsetLoc, &uvOffset, SHADER_UNIFORM_VEC2);

                DrawMeshInstanced(quad, material, intro1BatchTransforms.data(), (int)intro1BatchTransforms.size());

            }

            

            // Restore uvScl for rain if needed

            SetShaderValue(instancedShader, uvScaleLoc, &curUvScale, SHADER_UNIFORM_VEC2);

        }



        // Solid ground and road extend far in both directions centered on iCarX

        DrawMesh(mGround, matGround, MatrixTranslate(iCarX, iCarY + 0.00f, 250.0f));

        DrawMesh(mRoad, matRoad, MatrixTranslate(iCarX, iCarY + 0.01f, 250.0f));



        // Center dashed stripes (moving effect — snap to grid)

        float stripeBase = floorf((iCarZ - 220.0f) / 8.0f) * 8.0f;

        for (float sz = stripeBase; sz < iCarZ + 220.0f; sz += 8.0f)

            DrawLine3D({ iCarX, iCarY + 0.06f, sz }, { iCarX, iCarY + 0.06f, sz + 4.0f }, { 255,255,255,200 });



        // Road boundaries (solid white lines)

        for (float sz = stripeBase; sz < iCarZ + 220.0f; sz += 1.5f) {

            DrawLine3D({ iCarX - 6.0f, iCarY + 0.06f, sz }, { iCarX - 6.0f, iCarY + 0.06f, sz + 1.0f }, { 240,240,240,230 });

            DrawLine3D({ iCarX + 6.0f, iCarY + 0.06f, sz }, { iCarX + 6.0f, iCarY + 0.06f, sz + 1.0f }, { 240,240,240,230 });

        }



                // ---- CAR 3D RENDERING (Interior/Exterior Switching & Realistic Front) ----

        {

            float ccy = carCY();

            // Flag: when inside first-person view, only render windshield, wipers, and the forward hood

            bool drawExt = (iPhase == INTRO_AERIAL || iPhase == INTRO_DOOR_OPEN || iPhase == INTRO_EXIT_CAR || iPhase == INTRO_SISTER_EXIT || iPhase == INTRO_LOOK_AROUND);

            bool drawInt = drawExt; // Draw full 3D interior meshes when viewed from outside



            if (drawExt) {

                // Silver Gradient Body Panels

                DrawMesh(mBody,  matBody,  MatrixTranslate(iCarX, ccy, iCarZ));

                DrawMesh(mCabin, matCabin, MatrixMultiply(MatrixScale(1.0f, 1.0f, 1.0f), MatrixTranslate(iCarX, ccy + kBodyHalfH + 0.36f, iCarZ - 0.25f)));

                DrawMesh(mHood,  matHood,  MatrixTranslate(iCarX, ccy + kBodyHalfH + 0.09f, iCarZ + 2.1f));

                DrawMesh(mTrunk, matTrunk, MatrixTranslate(iCarX, ccy + kBodyHalfH + 0.18f, iCarZ - 2.0f));

                

                // Front Radiator Grille & Chrome Bumper

                DrawMesh(mGrille, matGrille, MatrixTranslate(iCarX, ccy + 0.02f, iCarZ + 2.68f));

                DrawMesh(mBumper, matChrome, MatrixTranslate(iCarX, iCarY + 0.38f, iCarZ + 2.70f)); // Front chrome bumper

                DrawMesh(mBumper, matChrome, MatrixTranslate(iCarX, iCarY + 0.38f, iCarZ - 2.68f)); // Rear chrome bumper

                

                // Real Car Front Details: Dual Headlights with Chrome Bezels & Amber Indicators

                // Left Headlight & Bezel

                DrawMesh(mHeadlight, matHeadlit, MatrixTranslate(iCarX - 0.82f, ccy + 0.10f, iCarZ + 2.67f));

                DrawMesh(mHeadlight, matBezel,   MatrixTranslate(iCarX - 0.82f, ccy + 0.10f, iCarZ + 2.63f));

                // Right Headlight & Bezel

                DrawMesh(mHeadlight, matHeadlit, MatrixTranslate(iCarX + 0.82f, ccy + 0.10f, iCarZ + 2.67f));

                DrawMesh(mHeadlight, matBezel,   MatrixTranslate(iCarX + 0.82f, ccy + 0.10f, iCarZ + 2.63f));

                // Amber side turn signals

                DrawMesh(mIndicator, matAmber, MatrixTranslate(iCarX - 1.08f, ccy + 0.10f, iCarZ + 2.65f));

                DrawMesh(mIndicator, matAmber, MatrixTranslate(iCarX + 1.08f, ccy + 0.10f, iCarZ + 2.65f));

// Replaced with full 3D volumetric beams and road illumination below

                

                // Rear Ruby Red Taillights

                DrawMesh(mIndicator, matBrake, MatrixTranslate(iCarX - 0.90f, ccy + 0.15f, iCarZ - 2.63f));

                DrawMesh(mIndicator, matBrake, MatrixTranslate(iCarX + 0.90f, ccy + 0.15f, iCarZ - 2.63f));



                // Fenders

                float fwx[4] = { -1.1f, 1.1f, -1.1f, 1.1f }; float fwz[4] = {  1.7f, 1.7f, -1.7f,-1.7f };

                for (int fi = 0; fi < 4; fi++) DrawMesh(mFender, matBody, MatrixTranslate(iCarX + fwx[fi], iCarY + kWheelR + 0.08f, iCarZ + fwz[fi]));



                // Wheels with alloy hubs

                struct WP { float ox, oz; };

                WP wps[4] = { {-1.15f, 1.75f}, { 1.15f, 1.75f}, {-1.15f,-1.75f}, { 1.15f,-1.75f} };

                for (int wi = 0; wi < 4; wi++) {

                    Matrix wm = MatrixRotateZ(90.0f * DEG2RAD);

                    wm = MatrixMultiply(MatrixRotateX(iWheelAng), wm);

                    wm = MatrixMultiply(wm, MatrixTranslate(iCarX + wps[wi].ox, iCarY + kWheelR, iCarZ + wps[wi].oz));

                    DrawMesh(mWheel, matWheel, wm);

                }

                

                // Left driver door with open angle hinge pivot

                {

                    float hx = iCarX - 1.21f;

                    float hz = iCarZ + 1.10f;

                    float dAng = iDoorAng * DEG2RAD;

                    Matrix dm = MatrixTranslate(0.0f, 0.0f, -1.0f);

                    dm = MatrixMultiply(dm, MatrixRotateY(dAng));

                    dm = MatrixMultiply(dm, MatrixTranslate(hx, ccy, hz));

                    DrawMesh(mDoor, matDoor, dm);

                }

                // Right passenger door

                {

                    float rDoorAng = 0.0f;

                    if (iPhase >= INTRO_SISTER_EXIT) {

                        rDoorAng = Clamp(iSisterT * 2.2f, 0.0f, 1.0f) * 68.0f * DEG2RAD;

                    }

                    Matrix rdm = MatrixTranslate(0.0f, 0.0f, -1.0f);

                    rdm = MatrixMultiply(rdm, MatrixRotateY(-rDoorAng));

                    rdm = MatrixMultiply(rdm, MatrixTranslate(iCarX + 1.21f, ccy, iCarZ + 1.10f));

                    DrawMesh(mDoor, matDoor, rdm);

                }

            }



            // Draw full 3D interior (Dashboard, Cowl, Gauges, Switches, Console, Seats, Steering Wheel)

            if (drawInt) {

                Draw3DCarInterior(iCarX, iCarY, iCarZ, ccy, timeVal, iCarSpd, iShakeDec, iJerkCount,

                                   mSeat, mSeatBack, mSteerRim, matSeat, matCabin);

            }

        }



        // ---- WIPERS (Realistic Cowl Mount & Tandem Sweep) ----

        {

            float u = (sinf(timeVal * 4.2f) + 1.0f) * 0.5f; // 0 (rest at cowl) to 1 (full upright sweep)

            float wiperAng = (12.0f + u * 74.0f) * DEG2RAD; // 12 deg to 86 deg

            float wLen = 0.68f;



            // Base sits properly at the bottom cowl of the windshield (top of hood)

            float wBaseY = carCY() + 0.38f;

            float wBaseZ = iCarZ + 1.18f;



            // Left Wiper (Driver side)

            Vector3 wLBase = { iCarX - 0.48f, wBaseY, wBaseZ };

            Vector3 wLTip  = {

                wLBase.x - cosf(wiperAng) * wLen,

                wLBase.y + sinf(wiperAng) * wLen * 0.85f,

                wLBase.z - sinf(wiperAng) * wLen * 0.30f  // Leans back with the slope of the windshield glass

            };

            DrawLine3D(wLBase, wLTip, { 85, 92, 105, 255 });

            DrawLine3D({ wLBase.x, wLBase.y + 0.008f, wLBase.z }, { wLTip.x, wLTip.y + 0.008f, wLTip.z }, { 35, 38, 45, 255 });



            // Right Wiper (Passenger side - sweeps tandem in the same direction)

            Vector3 wRBase = { iCarX + 0.22f, wBaseY, wBaseZ };

            Vector3 wRTip  = {

                wRBase.x - cosf(wiperAng) * wLen,

                wRBase.y + sinf(wiperAng) * wLen * 0.85f,

                wRBase.z - sinf(wiperAng) * wLen * 0.30f

            };

            DrawLine3D(wRBase, wRTip, { 85, 92, 105, 255 });

            DrawLine3D({ wRBase.x, wRBase.y + 0.008f, wRBase.z }, { wRTip.x, wRTip.y + 0.008f, wRTip.z }, { 35, 38, 45, 255 });

        }



        // ---- REAL HEADLIGHT LIGHT BEAMS & HIGHWAY ILLUMINATION ----

        // Active in BOTH First-Person Driving (driverFP) and Exterior Views

        {

            float ccy = carCY();

            Vector3 hL = { iCarX - 0.82f, ccy + 0.10f, iCarZ + 2.70f }; // Left headlight

            Vector3 hR = { iCarX + 0.82f, ccy + 0.10f, iCarZ + 2.70f }; // Right headlight



            // 1. Wet Highway Surface Illumination (Spotlight cone hitting asphalt)

            struct LightPatch { float z0, z1, w0, w1; unsigned char a0, a1; };

            LightPatch patches[4] = {

                { iCarZ + 2.8f,  iCarZ + 8.0f,  1.8f, 2.6f, 95, 75 }, // Hotspot directly in front of bumper

                { iCarZ + 8.0f,  iCarZ + 16.0f, 2.6f, 3.6f, 75, 50 }, // Main driving illumination pool

                { iCarZ + 16.0f, iCarZ + 26.0f, 3.6f, 4.6f, 50, 24 }, // Mid-range throw

                { iCarZ + 26.0f, iCarZ + 38.0f, 4.6f, 5.5f, 24,  0 }  // Distance fade

            };



            for (int p = 0; p < 4; p++) {

                float y = iCarY + 0.025f; // Hover slightly above road to avoid Z-fighting

                Vector3 p0 = { iCarX - patches[p].w0, y, patches[p].z0 };

                Vector3 p1 = { iCarX + patches[p].w0, y, patches[p].z0 };

                Vector3 p2 = { iCarX + patches[p].w1, y, patches[p].z1 };

                Vector3 p3 = { iCarX - patches[p].w1, y, patches[p].z1 };

                Color cNear = { 255, 252, 220, patches[p].a0 };

                Color cFar  = { 245, 248, 235, patches[p].a1 };

                DrawTriangle3D(p0, p1, p2, cNear);

                DrawTriangle3D(p0, p2, p3, cFar);

            }



            // 2. 3D Volumetric Light Shafts (Golden beams cutting through dark rain & mist)

            auto drawLightBeam = [&](Vector3 apex, float dirX) {

                float beamLen = 24.0f;

                float zEnd = apex.z + beamLen;

                float yEnd = iCarY + 0.06f;

                

                // Bottom fan across road

                Vector3 bL = { apex.x + dirX - 1.4f, yEnd, zEnd };

                Vector3 bR = { apex.x + dirX + 1.4f, yEnd, zEnd };

                // Top fan through mist

                Vector3 tL = { apex.x + dirX - 1.0f, apex.y + 0.55f, zEnd };

                Vector3 tR = { apex.x + dirX + 1.0f, apex.y + 0.55f, zEnd };



                Color beamMid  = { 255, 252, 225, 30 };

                Color beamEnd  = { 240, 245, 255, 8 };



                // Bottom shaft

                DrawTriangle3D(apex, bL, bR, beamMid);

                // Left & Right sides

                DrawTriangle3D(apex, bL, tL, beamEnd);

                DrawTriangle3D(apex, tR, bR, beamEnd);

                // Top shaft

                DrawTriangle3D(apex, tL, tR, beamEnd);

            };



            drawLightBeam(hL, -0.30f);

            drawLightBeam(hR,  0.30f);

        }





        // --- SPATIAL & FRUSTUM BUCKET CULLING FOR INTRO ---

        static std::vector<RenderBucket*> visibleBuckets;

        static bool s_intro2BucketsInit = false;

        if (!s_intro2BucketsInit) {

            visibleBuckets.reserve(BUCKETS_X * BUCKETS_Z);

            s_intro2BucketsInit = true;

        }

        visibleBuckets.clear();

        float cullDistSq = 80.0f * 80.0f; // 80 block render distance

        

        Vector3 camForward = Vector3Normalize(Vector3Subtract(shakeCam.target, shakeCam.position));

        camForward.y = 0.0f;

        camForward = Vector3Normalize(camForward);

        

        for (int bx = 0; bx < BUCKETS_X; bx++) {

            for (int bz = 0; bz < BUCKETS_Z; bz++) {

                float centerX = bx * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);

                float centerZ = bz * BUCKET_SIZE + (BUCKET_SIZE / 2.0f);

                float dx = centerX - shakeCam.position.x;

                float dz = centerZ - shakeCam.position.z;

                float distSq = dx*dx + dz*dz;

                

                if (distSq < cullDistSq) {

                    if (distSq < (BUCKET_SIZE * BUCKET_SIZE)) {

                        visibleBuckets.push_back(&chunk->buckets[bx][bz]);

                    } else {

                        Vector3 dirToBucket = Vector3Normalize({dx, 0.0f, dz});

                        float dotProd = Vector3DotProduct(camForward, dirToBucket);

                        if (dotProd > -0.5f) { // Wide FOV to prevent popping

                            visibleBuckets.push_back(&chunk->buckets[bx][bz]);

                        }

                    }

                }

            }

        }



        // ---- TERRAIN & RAIN (instanced shader pass - batched) ----

        static std::vector<Matrix> intro2BatchTransforms;

        static bool s_intro2BatchInit = false;

        if (!s_intro2BatchInit) {

            intro2BatchTransforms.reserve(16384);

            s_intro2BatchInit = true;

        }

        static bool s_intro2GlyphUsed[256];

        memset(s_intro2GlyphUsed, 0, sizeof(s_intro2GlyphUsed));

        for (RenderBucket* b : visibleBuckets) {

            for (uint8_t g : b->activeGlyphs) {

                s_intro2GlyphUsed[g] = true;

            }

        }

        for (int i = 0; i < 256; i++) {

            if (!rainInstances[i].empty()) s_intro2GlyphUsed[i] = true;

        }

        for(int i=0; i<256; i++) {

            if (!s_intro2GlyphUsed[i]) continue;

            bool hasRain = !rainInstances[i].empty();

            intro2BatchTransforms.clear();

            for (RenderBucket* b : visibleBuckets) {

                if (!b->instances[i].empty()) {

                    intro2BatchTransforms.insert(intro2BatchTransforms.end(), b->instances[i].begin(), b->instances[i].end());

                }

            }

            if (hasRain) {

                intro2BatchTransforms.insert(intro2BatchTransforms.end(), rainInstances[i].begin(), rainInstances[i].end());

            }

            if (intro2BatchTransforms.empty()) continue;

            

            int col = i % 16;

            int row = i / 16;

            Vector2 uvOff = { col * uvScl.x, row * uvScl.y };

            SetShaderValue(instancedShader, uvOffsetLoc, &uvOff, SHADER_UNIFORM_VEC2);

            DrawMeshInstanced(quad, material, intro2BatchTransforms.data(), (int)intro2BatchTransforms.size());

        }

            

            if (false) { // disabled old driver hack

                Matrix m = MatrixIdentity();

                m.m0 = 1.0f; m.m1 = 215/255.0f; m.m2 = 150/255.0f; m.m3 = 1.0f; // text color

                m.m4 = 1.0f; m.m5 = 1.0f; // scale

                m.m8 = 3.0f; m.m9 = 0.0f; m.m10 = 0.0f; m.m11 = 1.0f; // isEntity flag for billboard shader

                Vector3 deye = driverEyePos();

                m.m12 = deye.x; m.m13 = deye.y; m.m14 = deye.z; // position

                DrawMeshInstanced(quad, material, &m, 1);

            }

        



        // Multi-tier Ionized Plasma Lightning Bolt

        DrawPlasmaLightningBolt(lightningSegments, lightningFlashTimer, groundImpactPos, groundImpactTimer);



        EndMode3D();



        // ===========================================================

        // 2D OVERLAYS — all DrawText / DrawRectangle after EndMode3D

        // ===========================================================



        // ---- CHARACTER BILLBOARDS (@, i) projected to screen ----

        // Compute world positions of characters (always, even if not shown)

        {

            Vector3 pPos = (iPhase >= INTRO_EXIT_CAR) ? driverStandPos(iPlayerT) : driverEyePos();

            Vector3 sPos = (iPhase >= INTRO_SISTER_EXIT) ? sisterStandPos(iSisterT) : sisterEyePos();



            bool show3PChars = (iPhase == INTRO_AERIAL);



            // Driver @ is visible in third person AND in Sister POV looking at him

            bool showPlayer = show3PChars;



            if (showPlayer) {

                Vector3 driverHead = false ? (Vector3){ iCarX - 0.50f, carCY() + 0.35f, iCarZ + 0.15f } : pPos;

                Vector2 pSS = GetWorldToScreen(driverHead, shakeCam);

                DrawText("@", (int)pSS.x - 9, (int)pSS.y - 14, 24, { 255, 215, 150, 245 });



                if (iSisterOut || iPhase == INTRO_AERIAL || iPhase == INTRO_SISTER_EXIT) {

                    Vector2 sSS = GetWorldToScreen(sPos, shakeCam);

                    DrawText("i", (int)sSS.x - 4, (int)sSS.y - 14, 22, { 255, 215, 150, 245 });



                    // "!" exclamation above both when outside car

                    if (iSisterOut && iPhase >= INTRO_SISTER_EXIT) {

                        float fA = sinf(timeVal * 6.2f) * 0.5f + 0.5f;

                        unsigned char fa = (unsigned char)(fA * 255.0f);

                        Vector2 pe = GetWorldToScreen({ pPos.x, pPos.y + 0.72f, pPos.z }, shakeCam);

                        Vector2 se = GetWorldToScreen({ sPos.x, sPos.y + 0.72f, sPos.z }, shakeCam);

                        DrawText("!", (int)pe.x - 5, (int)pe.y - 13, 28, { 255, 255, 55, fa });

                        DrawText("!", (int)se.x - 5, (int)se.y - 13, 28, { 255, 255, 55, fa });

                    }

                }

            }

        }



        // ---- DRIVER FP OVERLAYS ----

        if (driverFP) {



            // 1. ASCII Steering wheel HUD overlay removed per user request



            // 2. Windshield water drop simulation

            // Spawn new drops

            if (GetRandomValue(0, 100) < 45) {

                WDrop d;

                d.worldPos = {

                    iCarX + (GetRandomValue(-85, 85) / 100.0f),

                    carCY() + 0.44f + (GetRandomValue(0, 45) / 100.0f),

                    iCarZ + 1.22f

                };

                d.life     = 1.0f;

                d.slideSpd = GetRandomValue(12, 35) / 100.0f;

                wDrops.push_back(d);

            }



            // Wiper sweep clears drops

            float wSweep = iWiperT * 52.0f * DEG2RAD;

            for (int i = 0; i < (int)wDrops.size(); ) {

                wDrops[i].life     -= dt * 0.08f;

                wDrops[i].worldPos.y -= dt * wDrops[i].slideSpd; // slide down

                wDrops[i].worldPos.z  = iCarZ + 1.22f;           // stay on glass



                // Check if wiped (within wiper sweep arc — simplified: left/right arcs)

                float dxL = wDrops[i].worldPos.x - (iCarX - 0.48f);

                float dyL = wDrops[i].worldPos.y - (carCY() + 0.44f);

                float distL = sqrtf(dxL*dxL + dyL*dyL);

                float wLtipX = (iCarX - 0.48f) + sinf(wSweep) * 0.72f;

                float wLtipY = (carCY() + 0.44f) + cosf(wSweep) * 0.72f * 0.9f;

                float wLang  = atan2f(wLtipX - (iCarX - 0.48f), wLtipY - (carCY() + 0.44f));

                float dAng   = atan2f(dxL, dyL);

                bool wipedL  = (distL < 0.72f && fabsf(dAng - wLang) < 0.15f);



                float dxR = wDrops[i].worldPos.x - (iCarX + 0.48f);

                float dyR = wDrops[i].worldPos.y - (carCY() + 0.44f);

                float distR = sqrtf(dxR*dxR + dyR*dyR);

                float wRtipX = (iCarX + 0.48f) + sinf(-wSweep) * 0.72f;

                float wRtipY = (carCY() + 0.44f) + cosf(-wSweep) * 0.72f * 0.9f;

                float wRang  = atan2f(wRtipX - (iCarX + 0.48f), wRtipY - (carCY() + 0.44f));

                float dRang  = atan2f(dxR, dyR);

                bool wipedR  = (distR < 0.72f && fabsf(dRang - wRang) < 0.15f);



                if (wDrops[i].life <= 0.0f || wipedL || wipedR) {

                    wDrops[i] = wDrops.back();

                    wDrops.pop_back();

                } else {

                    Vector2 dSS = GetWorldToScreen(wDrops[i].worldPos, shakeCam);

                    unsigned char da = (unsigned char)(wDrops[i].life * 190.0f);

                    // Draw a water droplet (white inner with darker refraction edge)

                    DrawCircleV(dSS, 4.0f, { 255, 255, 255, (unsigned char)(da * 0.4f) });

                    DrawCircleLines(dSS.x, dSS.y, 4.5f, { 100, 120, 150, (unsigned char)(da * 0.6f) });

                    i++;

                }

            }



            float dashPanX = 0.0f; // Centered cockpit dashboard for driver POV



            // Draw the curved dashboard with speedometer, fuel gauge, switches, and steering wheel

            DrawFirstPersonDashboard(iCarSpd, iShakeDec, iJerkCount, timeVal, dashPanX, false);

        }



        // ---- REARVIEW MIRROR HUD REMOVED ----





        // ---- VIGNETTE (look-around and fade phases) ----

        if (iPhase == INTRO_LOOK_AROUND || iPhase == INTRO_FADE_TO_GAME) {

            int vH = LOGICAL_H / 3;

            int vW = LOGICAL_W / 4;

            DrawRectangleGradientV(0, 0,          LOGICAL_W, vH, { 0,0,0,210 }, BLANK);

            DrawRectangleGradientV(0, LOGICAL_H-vH, LOGICAL_W, vH, BLANK, { 0,0,0,210 });

            DrawRectangleGradientH(0, 0,          vW, LOGICAL_H, { 0,0,0,170 }, BLANK);

            DrawRectangleGradientH(LOGICAL_W-vW, 0, vW, LOGICAL_H, BLANK, { 0,0,0,170 });



            // Eyelid blink

            if (iEyeT > 0.01f) {

                int eH = (int)(iEyeT * (LOGICAL_H / 2) + 6);

                DrawRectangle(0, 0,           LOGICAL_W, eH, BLACK);

                DrawRectangle(0, LOGICAL_H - eH, LOGICAL_W, eH, BLACK);

            }

        }



        // ---- LIGHTNING FLASH SCREEN OVERLAY ----

        if (lightningFlashTimer > 0.0f) {

            unsigned char fa = (unsigned char)(lightningFlashTimer * 60.0f);

            DrawRectangle(0, 0, LOGICAL_W, LOGICAL_H, { 220, 230, 255, fa });

        }



// Rearview mirror moved to UI pass on top of 3D view



        // ---- FINAL FADE TO BLACK ----

        if (iFadeAlpha > 0.0f) {

            DrawRectangle(0, 0, LOGICAL_W, LOGICAL_H, Fade(BLACK, iFadeAlpha));

        }



        // Title text and FPS overlay removed per user request



        EndTextureMode();



        // ---- BLIT TO SCREEN ----

        BeginDrawing();

        ClearBackground(BLACK);

        

        int screenW = GetScreenWidth();

        int screenH = GetScreenHeight();

        float scale = fminf((float)screenW / (float)LOGICAL_W, (float)screenH / (float)LOGICAL_H);

        float destW = (float)LOGICAL_W * scale;

        float destH = (float)LOGICAL_H * scale;

        float destX = ((float)screenW - destW) * 0.5f;

        float destY = ((float)screenH - destH) * 0.5f;

        Rectangle destRec = { destX, destY, destW, destH };



        if (driverFP) {

            BeginShaderMode(glassShader);

            SetShaderValue(glassShader, glassTimeLoc, &timeVal, SHADER_UNIFORM_FLOAT);

        }

        

        DrawTexturePro(target.texture,

            { 0.0f, 0.0f, (float)target.texture.width, -(float)target.texture.height },

            destRec,

            { 0, 0 }, 0.0f, WHITE);

            

        if (driverFP) EndShaderMode();



        // ---- 2D UI REARVIEW MIRROR (Crisp UI on top of 3D view) ----

        if (driverFP) {

            int mW = (int)(224.0f * scale);

            int mH = (int)(80.0f * scale);

            int mX = (int)(destX + (destW - mW) * 0.5f);

            int mY = (int)(destY + 12.0f * scale);



            // Windshield roof mounting bracket

            DrawRectangle((int)(destX + destW * 0.5f - 3.0f * scale), (int)destY, (int)(6.0f * scale), (int)(12.0f * scale), { 20, 24, 30, 255 });

            // Outer casing with metallic bezel trim

            DrawRectangleRounded({ (float)mX - 5.0f * scale, (float)mY - 5.0f * scale, (float)mW + 10.0f * scale, (float)mH + 10.0f * scale }, 0.20f, 8, { 22, 26, 32, 255 });

            DrawRectangleRoundedLines({ (float)mX - 5.0f * scale, (float)mY - 5.0f * scale, (float)mW + 10.0f * scale, (float)mH + 10.0f * scale }, 0.20f, 8, { 60, 70, 85, 255 });



            // Live reflection texture (flipped horizontally for true mirror physics)

            Rectangle src = { 0, 0, -(float)mirrorRT.texture.width, -(float)mirrorRT.texture.height };

            DrawTexturePro(mirrorRT.texture, src, { (float)mX, (float)mY, (float)mW, (float)mH }, { 0, 0 }, 0.0f, WHITE);



            // Anti-glare tint & glass reflection sheen line

            DrawRectangle(mX, mY, mW, mH, { 15, 25, 45, 25 });

            DrawLineEx({ (float)mX + 10.0f * scale, (float)mY + 6.0f * scale }, { (float)mX + (float)mW - 30.0f * scale, (float)mY + 6.0f * scale }, 1.5f * scale, { 220, 235, 255, 75 });

        }

        

        int escFontSize = (int)(14.0f * scale);

        if (escFontSize < 10) escFontSize = 10;

        Rectangle skipRec = { (float)(destX + destW - 150.0f * scale), (float)(destY + destH - 30.0f * scale), 142.0f * scale, 24.0f * scale };

        Vector2 mousePos = GetMousePosition();

        bool hoverSkip = CheckCollisionPointRec(mousePos, skipRec);

        if (hoverSkip) {

            DrawRectangleRec(skipRec, (Color){ 35, 45, 60, 200 });

            DrawRectangleLinesEx(skipRec, 1.0f, (Color){ 100, 140, 190, 240 });

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                iPhase = INTRO_DONE;

                break;

            }

        }

        Color skipCol = hoverSkip ? WHITE : (Color){ 180, 180, 180, 180 };

        DrawText("[ESC] SKIP INTRO", (int)(destX + destW - 140.0f * scale), (int)(destY + destH - 24.0f * scale), escFontSize, skipCol);



        // Window resize hint during intro

        DrawText("Resize / Maximize window or press [F11] for Fullscreen", (int)(destX + 16.0f * scale), (int)(destY + 12.0f * scale), (int)(13.0f * scale), (Color){ 140, 165, 190, 180 });

        EndDrawing();



    } // end intro while



    // ===========================================================

    // CLEANUP INTRO RESOURCES

    // ===========================================================

    UnloadMesh(mBody);   UnloadMesh(mCabin);  UnloadMesh(mTrunk);

    UnloadMesh(mHood);   UnloadMesh(mGrille); UnloadMesh(mWheel);

    UnloadMesh(mDoor);   UnloadMesh(mDash);   UnloadMesh(mSeat);

    UnloadMesh(mSeatBack); UnloadMesh(mMirror);

    UnloadMesh(mBumper); UnloadMesh(mFender); UnloadMesh(mSteerRim);

    UnloadMesh(mRVMFrame); UnloadMesh(mHeadlight); UnloadMesh(mIndicator);



    UnloadMaterial(matBody);   UnloadMaterial(matCabin);  UnloadMaterial(matHood);

    UnloadMaterial(matDoor);   UnloadMaterial(matTrunk);  UnloadMaterial(matGlass);

    UnloadMaterial(matWheel);  UnloadMaterial(matHub);    UnloadMaterial(matInterior);

    UnloadMaterial(matSeat);   UnloadMaterial(matChrome); UnloadMaterial(matGrille);

    UnloadMaterial(matHeadlit);UnloadMaterial(matBezel);  UnloadMaterial(matAmber);

    UnloadMaterial(matMirrorF);UnloadMaterial(matBrake);



    UnloadRenderTexture(mirrorRT);

    UnloadShader(glassShader);



    // ===========================================================

    // RESET GAME STATE — transition into gameplay

    // ===========================================================

    isStormActive       = false;

    stormDuration       = 0.0f;

    lightningFlashTimer = 0.0f;

    groundImpactTimer   = 0.0f;

    nextStormEventTimer = (float)GetRandomValue(8, 20);

    rainParticles.clear();

    for (int i = 0; i < 256; i++) rainInstances[i].clear();



    camera.position = (Vector3){ CHUNK_W / 2.0f, 12.2f, CHUNK_D / 2.0f };

    camera.target   = (Vector3){ CHUNK_W / 2.0f, 12.2f, CHUNK_D / 2.0f + 1.0f };

    camera.up       = (Vector3){ 0.0f, 1.0f, 0.0f };

    camera.fovy     = 60.0f;

    renderCam       = camera;



    StopSound(sndRain);

    StopSound(sndWind);

    StopSound(sndCrickets);

    PlaySound(sndCrickets);

    PlaySound(sndWind);



}; // end RunIntroCinematic lambda



    // =========================================================================

    // GAME STATE MACHINE & 3D MAIN MENU STATE

    // =========================================================================

    enum GameState {

        STATE_MAIN_MENU = 0,

        STATE_GAMEPLAY,

        STATE_PAUSED

    };

    GameState g_gameState = STATE_MAIN_MENU;

    bool g_hasPlayedIntro = false;

    bool g_showSettingsModal = false;

    bool g_showCaseFilesModal = false;

    bool g_showManifestModal = false;

    bool g_showSurvivalModal = false;

    int  g_caseFileSelected = 0;

    int  g_menuCCTVFeed = 0; // 0: Cold Storage Aisle, 1: Forecourt & Pumps, 2: Blackwood College Portico

    float g_cctvSwitchGlitch = 0.0f;

    // --- Phase 1: Ultimate Interactive Horror Menu State ---
    static float   g_menuIdleTimer          = 0.0f;   // Seconds mouse has remained stationary
    static float   g_menuAwakeIntensity     = 1.0f;   // 1.0 = fully awake/bright, 0.12 = dark slumber
    static Vector2 g_menuLightPos           = { 640.0f, 360.0f }; // Smoothed volumetric beam tracking pos
    static float   g_lurkerEyeFlee          = 0.0f;   // Flee/vanish animation timer for lurking eyes (0.0 to 1.0)
    static float   g_menuCamSmoothX         = 0.0f;   // Damped cursor parallax X for 3D camera
    static float   g_menuCamSmoothY         = 0.0f;   // Damped cursor parallax Y for 3D camera
    static float   g_skullEyeSmoothX        = 0.0f;   // Smoothed pupil tracking X for horned bovine skull
    static float   g_skullEyeSmoothY        = 0.0f;   // Smoothed pupil tracking Y for horned bovine skull
    static float   g_skullGazeFlare         = 0.0f;   // Pupil flare / intensity on menu interaction
    static int     g_prevMenuSelection      = -1;     // Previous hover selection for audio feedback
    static float   g_menuOptionHover[5]     = { 0 };  // Per-option smooth hover transition factor (0.0 -> 1.0)



    // Exterior Carpet & Secret Underground Tunnel & Bunker State

    static bool  g_carpetMoved        = false; // false = flat covering hatch, true = pulled back

    static float g_carpetAnim         = 0.0f;  // 0.0 (flat) -> 1.0 (pulled / folded)

    static bool  g_tunnelHatchOpen    = false; // false = closed, true = open

    static float g_hatchAnim          = 0.0f;  // 0.0 (closed) -> 1.0 (open)

    static bool  g_radioPower         = true;  // Clandestine vacuum tube radio power

    static float g_radioAnim          = 0.0f;  // Tuning dial / magic eye pulse

    static float g_radioMsgTimer      = 0.0f;  // Periodic transmission timer

    static bool  g_showDossierModal   = false; // Attendant's clandestine dossier modal

    static bool  g_dossierReadOnce    = false; // Unlocked chest code knowledge (0-8-4-2)

    static int   g_dossierFileSelected = 0;    // Selected tab in dossier modal

    static bool  g_chestUnlocked      = false; // True when chest padlock opened

    static float g_chestLidAnim       = 0.0f;  // 0.0 (closed) -> 1.0 (open)

    static bool  g_chestLooted        = false; // True once survival items collected

    static float g_tunnelDripTimer    = 0.0f;  // Pail water drip timer

    static float g_workLightFlicker   = 1.0f;  // Caged light intensity multiplier

    static float g_workLightBuzzTimer = 0.0f;  // Light flicker spark timer

    static bool  g_hasShovel          = false; // Acquired from supermarket hardware shelf
    static bool  g_shovelEquipped      = true;  // [1] or [X] toggles equipped in hand

    static bool  g_tunnelDug          = false; // True once cave-in obstruction is excavated

    static float g_digProgress        = 0.0f;  // Excavation progress 0.0f to 1.0f

    static float g_digAnimTimer       = 0.0f;  // Shovel strike feedback timer

    static ShovelAnimState g_shovelAnimState   = SHOVEL_ANIM_IDLE;

    static float g_shovelAnimTime              = 0.0f;

    static float g_shovelIdleClock             = 0.0f;

    static bool  g_shovelDigImpactDone         = false;

    static bool  g_shovelDigThrowDone          = false;

    static bool  g_shovelAttackImpactDone      = false;

    static float g_tunnelBannerTimer  = 0.0f;  // Top notification banner timer

    static char  g_tunnelBannerText[160] = { 0 };

    // Hyper-Realistic Phone & Live GPS Map State

    static bool  g_phoneActive        = false; // false = in pocket, true = held in hand

    static float g_phoneAnim          = 0.0f;  // 0.0 (pocket) -> 1.0 (raised in hand)

    static int   g_phoneZoomMode      = 0;     // 0: Local Sector Tactical (1x), 1: Full Region Overview (2x)

    static float g_phoneRadarPulse    = 0.0f;  // Radar beacon pulse animation

    static float g_phoneSignalFlicker = 0.0f;  // Carrier bar flicker timer

    bool g_isMenuStartingGame = false;

    float g_menuPlayTransitionTimer = 0.0f;



    // User settings & calibration

    float g_userMasterVolume = 1.0f;

    float g_userMouseSensitivity = 1.0f;

    float g_userFov = 60.0f;

    float g_userHorrorGamma = 1.0f;



    bool isCursorCaptured = false;

    EnableCursor();

    showQuitConfirm = false;

    float escCooldown = 0.6f;

    float gameIntroFade = 0.0f;



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

            if (IsKeyPressed(KEY_M)) {

                g_phoneActive = !g_phoneActive;

                PlaySound(g_sndPhoneSlide);

            }

            if (g_phoneActive) {

                if (IsKeyPressed(KEY_Z)) {

                    g_phoneZoomMode = (g_phoneZoomMode == 0) ? 1 : 0;

                    PlaySound(g_sndPhoneTap);

                }

            }

        }



        // Smooth Phone Raise / Lower Animation

        if (g_phoneActive && g_gameState == STATE_GAMEPLAY) {

            g_phoneAnim = Clamp(g_phoneAnim + dt * 5.2f, 0.0f, 1.0f);

        } else {

            g_phoneAnim = Clamp(g_phoneAnim - dt * 6.2f, 0.0f, 1.0f);

        }

        g_phoneRadarPulse = fmodf(g_phoneRadarPulse + dt * 1.35f, 1.0f);

        g_phoneSignalFlicker += dt;



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



            if (IsKeyPressed(KEY_ESCAPE) && !showQuitConfirm && !isShopOpen && !g_showSettingsModal && !g_showManifestModal) {
                isCursorCaptured = !isCursorCaptured;
                if (isCursorCaptured) DisableCursor();
                else EnableCursor();
            }

            // Modals automatically free cursor so user can click buttons or resize window

            if (showQuitConfirm || isShopOpen || g_showSettingsModal || g_showManifestModal) {

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

        if (IsKeyPressed(KEY_C) && !isShopOpen && !showQuitConfirm) isRoofCamActive = !isRoofCamActive;



        // FRONT ROOF WALL CCTV CONTROLS: Sliders along the front roof edge wall

        if (isRoofCamActive && !showQuitConfirm) {

            // Slide along the front roof edge rail with A / D

            if (IsKeyDown(KEY_A)) roofCamSlideX = Clamp(roofCamSlideX - dt * 4.5f, -3.2f, 3.2f);

            if (IsKeyDown(KEY_D)) roofCamSlideX = Clamp(roofCamSlideX + dt * 4.5f, -3.2f, 3.2f);



            // Strong Optical Zooming: Mouse Wheel or W / S

            float wheel = GetMouseWheelMove();

            if (wheel != 0.0f) roofCamFOV = Clamp(roofCamFOV - wheel * 4.5f, 15.0f, 80.0f);

            if (IsKeyDown(KEY_W)) roofCamFOV = Clamp(roofCamFOV - dt * 28.0f, 15.0f, 80.0f);

            if (IsKeyDown(KEY_S)) roofCamFOV = Clamp(roofCamFOV + dt * 28.0f, 15.0f, 80.0f);



            // Mouse Look: Pan & Tilt (Only when cursor is captured)

            if (isCursorCaptured) {

                Vector2 mDelta = GetMouseDelta();

                roofCamYaw   = Clamp(roofCamYaw - mDelta.x * 0.22f, -85.0f, 85.0f);

                roofCamPitch = Clamp(roofCamPitch - mDelta.y * 0.22f, -65.0f, 28.0f);

            }



            if (IsKeyDown(KEY_LEFT))  roofCamYaw   = Clamp(roofCamYaw + dt * 45.0f, -85.0f, 85.0f);

            if (IsKeyDown(KEY_RIGHT)) roofCamYaw   = Clamp(roofCamYaw - dt * 45.0f, -85.0f, 85.0f);

            if (IsKeyDown(KEY_UP))    roofCamPitch = Clamp(roofCamPitch + dt * 35.0f, -65.0f, 28.0f);

            if (IsKeyDown(KEY_DOWN))  roofCamPitch = Clamp(roofCamPitch - dt * 35.0f, -65.0f, 28.0f);

        } else {

            // Strange Autonomous Horror Behavior: Robotic CCTV box slowly creeps along the front roof wall!

            roofCamSlideX = sinf(timeVal * 0.8f) * 2.8f;

            roofCamPitch  = -15.0f + sinf(timeVal * 0.5f) * 4.0f;

            roofCamYaw    = sinf(timeVal * 0.9f) * 24.0f;

        }



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

            float distToSpark = Vector3Distance(camera.position, (Vector3){ 90.25f, 15.02f, 148.0f });

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

        g_curSunDir         = Vector3Normalize((Vector3){ cosf(sunTheta_loop), sunElev_loop, cosf(sunTheta_loop) * 0.28f });

        g_curExtNightFactor = Clamp((-sunElev_loop + 0.08f) / 0.28f, 0.0f, 1.0f);

        g_curExtDayFactor   = Clamp((sunElev_loop + 0.08f) / 0.28f, 0.0f, 1.0f);

        g_curLightningFlash = lightningFlashTimer;

        g_playerCamPos      = camera.position;

        g_playerCamFwd      = Vector3Normalize(Vector3Subtract(camera.target, camera.position));



        // Toggle Flashlight ([F])

        if (IsKeyPressed(KEY_F) && !isShopOpen && !showQuitConfirm && !g_showSettingsModal && !g_showManifestModal && g_gameState == STATE_GAMEPLAY) {

            float dToCartF = Vector2Distance((Vector2){ camera.position.x, camera.position.z }, (Vector2){ g_cartPos.x, g_cartPos.z });

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
            SetShopLight(0, bulbHeadPos, (Color){ 255, 215, 135, 255 }, shopLightIntensity * 3.60f, 18.0f);

            // Light 1: Tubelight 1 - North Aisle (Aisle 3 above north shelves)
            SetShopLight(1, (Vector3){ 95.0f, 15.00f, 150.5f }, (Color){ 215, 235, 255, 255 }, 2.80f, 12.0f);

            // Light 2: Tubelight 2 - Checkout Counter Task Spotlight
            SetShopLight(2, (Vector3){ 104.5f, 14.20f, 133.5f }, (Color){ 255, 220, 150, 255 }, 3.00f, 9.5f);

            // Light 3: Tubelight 3 - South Aisle (Aisle 1 above grocery shelves)
            SetShopLight(3, (Vector3){ 95.0f, 15.00f, 136.5f }, (Color){ 225, 235, 245, 255 }, 2.80f, 12.0f);

            // Light 4: Commercial Island Freezer LEDs (Icy cyan basin glow)
            SetShopLight(4, (Vector3){ 94.5f, 10.75f, 133.5f }, (Color){ 100, 205, 255, 255 }, 1.50f, 5.5f);

            // Light 5: Tubelight 5 - Entrance Corridor & Shopping Cart Bay
            SetShopLight(5, (Vector3){ 104.5f, 15.00f, 143.5f }, (Color){ 220, 230, 240, 255 }, 2.80f, 11.0f);

            // Light 6: Tubelight 6 - Rear Storage Corner
            SetShopLight(6, (Vector3){ 89.0f, 15.00f, 133.5f }, (Color){ 210, 230, 250, 255 }, 2.60f, 10.5f);

            // Light 7: Haunted Washroom Overhead Flickering Fixture
            SetShopLight(7, (Vector3){ 89.0f, 13.85f, 157.5f }, (Color){ 230, 248, 205, 255 }, 2.40f, 8.5f);

        } else {

            // Master store power switch OFF: blackout fixtures
            for (int li = 0; li < 8; li++) {
                SetShopLight(li, (Vector3){ 95.0f, 15.0f, 140.0f }, (Color){ 0, 0, 0, 0 }, 0.0f, 0.0f);
            }

        }

        bool isPlayerMoving = (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D));
        UpdateShopAtmosphere(dt, timeVal, camera.position, isPlayerMoving, g_shopLightsOn, sinf(shopLightSwayX) * 2.05f, sinf(shopLightSwayZ) * 2.05f);

        // --- MR. GRETHNAR WOULE: GAZE DETECTION, EYE SWELL & JUMPSCARE ENGINE ---

        bool playerInShop = ((camera.position.x >= 86.0f && camera.position.x <= 109.2f &&
                              camera.position.z >= 125.5f && camera.position.z <= 154.5f) ||
                             (camera.position.x >= 85.5f && camera.position.x <= 92.8f &&
                              camera.position.z >= 153.5f && camera.position.z <= 161.5f));



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

                    sc.rot = (Vector3){ Frand(0, 360), Frand(0, 360), Frand(0, 360) };

                    sc.rotVel = (Vector3){ Frand(450, 950), Frand(350, 850), Frand(350, 850) };

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

                        gs.vel = (Vector3){ Frand(-2.5f, 2.5f) - fwd.x * 1.5f, Frand(0.8f, 3.2f), Frand(-2.5f, 2.5f) - fwd.z * 1.5f };

                        gs.life = Frand(0.2f, 0.45f);

                        gs.color = (Color){ 255, (unsigned char)GetRandomValue(180, 240), 70, 255 };

                        g_gunSparks.push_back(gs);

                    }

                }

            }



            if (IsKeyPressed(KEY_Q)) {

                hp.held = false;

                hp.opened = false;

                hp.vel = (Vector3){ 0.0f, -0.5f, 0.0f };

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

                shopFeedbackMsg = "You purchase Bottled Whispers ($6.66). Faint trapped voices murmur from the corked glass.";

                shopFeedbackTimer = 5.0f;

            } else if (IsKeyPressed(KEY_TWO)) {

                shopFeedbackMsg = "You purchase Canned Silence ($4.44). Total acoustic deadness wraps your fingers. Nothing rattles.";

                shopFeedbackTimer = 5.0f;

            } else if (IsKeyPressed(KEY_THREE)) {

                shopFeedbackMsg = "You take Expired Sunlight ($0.00). It flares faintly warm and smells of old asphalt and dusk.";

                shopFeedbackTimer = 5.0f;

            } else if (IsKeyPressed(KEY_FOUR)) {

                shopFeedbackMsg = "You purchase Your Old Wallet ($9). Inside is an expired driver's license with your name and face.";

                shopFeedbackTimer = 5.0f;

            } else if (IsKeyPressed(KEY_FIVE)) {

                playerHasDuplicateKey = true;

                shopFeedbackMsg = "You slip the duplicate key into your pocket beside your original.\nThey are identical down to the scratch on the brass.\nThe new one feels freezing cold.";

                shopFeedbackTimer = 7.0f;

            } else if (IsKeyPressed(KEY_SIX)) {

                shopFeedbackMsg = "You purchase the Jar of Loose Teeth ($13.13). They clatter with a dry ceramic snap when you look away.";

                shopFeedbackTimer = 5.0f;

            } else if (IsKeyPressed(KEY_SEVEN)) {

                shopFeedbackMsg = "You take the Donkey Milk ($7.77). The glass is warm.\nThe thick bone-white cream turns slowly against the glass of its own volition.";

                shopFeedbackTimer = 6.0f;

            } else if (IsKeyPressed(KEY_ESCAPE) || (IsKeyPressed(KEY_E) && !nearCounter)) {

                isShopOpen = false;

                shopFeedbackMsg = nullptr;

            }

        }



        Vector3 oldPos = camera.position;

        if (g_gameState == STATE_GAMEPLAY && hitStopTimer <= 0.0f && !isShopOpen && !isRoofCamActive && !showQuitConfirm && !g_showSettingsModal && !g_showManifestModal) {

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

            Vector3 rgt = Vector3Normalize(Vector3CrossProduct(fwd, (Vector3){0, 1, 0}));

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

                        p.pos = (Vector3){ hp.homePos.x + Frand(-0.03f, 0.03f), hp.homePos.y + 0.22f, hp.homePos.z + Frand(-0.03f, 0.03f) };

                        p.vel = (Vector3){ Frand(-0.4f, 0.4f) + fwd.x * 0.4f, Frand(0.35f, 0.75f), Frand(-0.4f, 0.4f) + fwd.z * 0.4f };

                        p.radius = 0.013f;

                        p.color = (Color){ (unsigned char)GetRandomValue(245, 255), (unsigned char)GetRandomValue(225, 245), (unsigned char)GetRandomValue(160, 195), 255 };

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

                        Vector3 mouth = (Vector3){ hp.homePos.x + fwd.x * 0.20f, hp.homePos.y + 0.12f, hp.homePos.z + fwd.z * 0.20f };

                        p.pos = mouth;

                        p.vel = (Vector3){ Frand(-0.1f, 0.1f) + fwd.x * 0.25f, -0.35f, Frand(-0.1f, 0.1f) + fwd.z * 0.25f };

                        p.radius = 0.010f;

                        p.color = (hp.type == PROD_MILK) ? (Color){ 245, 245, 252, 255 } : (Color){ 160, 10, 16, 255 };

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

                    p.vel = (Vector3){ 0, 0, 0 };

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

            // Western Ocean Boundary (X <= 35): Open infinite sea for swimming & diving!
            if (vx <= 35) {
                // Submarine floor support: seabed slopes from Y=7.5m down to Y=3.8m in open sea
                float bedY = (vx >= 0) ? (3.8f + ((float)vx / 35.0f) * 3.7f) : 3.8f;
                if ((float)vy <= bedY) return true; // Solid seabed floor
                return false; // Water volume is 100% open for infinite swimming westward!
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

            // Shop lot & surrounding grounds subfloor support (flush flat Y <= 10 across entire clearing)

            if (vx >= 60 && vx <= 127 && vz >= 108 && vz <= 172 && vy <= 10) return true;

            // Real geometric shop walls collision:

            if (vy >= 11 && vy <= 15) {

                // West Wall (X = 86, Z: 126..154)

                if (vx == 86 && vz >= 126 && vz <= 154) return true;

                // South Wall (Z = 126, X: 86..108)

                if (vz == 126 && vx >= 86 && vx <= 108) return true;

                // North Wall (Z = 154, X: 86..108) with doorway into Haunted Washroom (Generous threshold X: 87..91)
                if (vz == 154 && vx >= 86 && vx <= 108) {
                    bool inWashroomDoor = (vx >= 87 && vx <= 91 && vy <= 14);
                    if (!inWashroomDoor) return true;
                }

                // Haunted Washroom Solid Exterior Walls (X in [85..93], Z in [154..161])
                if (vz >= 154 && vz <= 161 && (vx <= 85 || vx >= 92)) return true; // West & East washroom walls
                if (vz >= 161 && vx >= 85 && vx <= 92) return true; // North washroom back wall

                // Washroom Fixtures (Porcelain Toilet & Wall Sink - compact to allow free movement)
                if (vx >= 86 && vx <= 87 && vz >= 159 && vz <= 160 && vy <= 12) return true; // Toilet
                if (vx >= 89 && vx <= 90 && vz >= 160 && vz <= 161 && vy <= 12) return true; // Sink

                // East Facade Wall (X = 108, Z: 126..154, with doorway at Z: 139..141, Y: 11..13)

                if (vx == 108 && vz >= 126 && vz <= 154) {

                    bool inDoorway = (vz >= 139 && vz <= 141 && vy <= 13);

                    if (inDoorway) {

                        if (doorSlideProgress < 0.55f) return true; // Closed / mostly closed glass door is solid

                    } else {

                        return true; // Wall is solid

                    }

                }

                // Exactly Two Shelving Racks (Vertical along X: Rack 1 at Z = 140, Rack 2 at Z = 147, X: 89..101, Y: 11..13)

                if ((vz >= 139 && vz <= 140 && vx >= 89 && vx <= 101 && vy <= 13) ||

                    (vz >= 146 && vz <= 147 && vx >= 89 && vx <= 101 && vy <= 13)) return true;

                // Checkout Counter in Start Left Corner (X: 102..107, Z: 132..134, Y = 11)

                if (vx >= 102 && vx <= 107 && vz >= 132 && vz <= 134 && vy == 11) return true;

                // Commercial Horizontal Refrigerator Island (X: 93..96, Z: 133..134, Y = 11)

                if (vx >= 93 && vx <= 96 && vz >= 133 && vz <= 134 && vy == 11) return true;

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

                Vector3 camFwd = Vector3Normalize((Vector3){ camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z });

                Vector3 targetCartPos = { camera.position.x + camFwd.x * 1.15f, 10.02f, camera.position.z + camFwd.z * 1.15f };

                float moveDist = Vector2Distance((Vector2){ g_cartPos.x, g_cartPos.z }, (Vector2){ targetCartPos.x, targetCartPos.z });

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

                    g_cartVel = (Vector3){ 0, 0, 0 };

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



            // Sync items placed inside the cart

            for (size_t k = 0; k < g_cartProductIndices.size(); k++) {

                int pIdx = g_cartProductIndices[k];

                float lx = (k % 2 == 0) ? -0.12f : 0.12f;

                float lz = ((int)(k / 2) * 0.15f) - 0.12f;

                float cosY = cosf(g_cartYaw * DEG2RAD);

                float sinY = sinf(g_cartYaw * DEG2RAD);

                float wx = g_cartPos.x + lx * cosY + lz * sinY;

                float wz = g_cartPos.z - lx * sinY + lz * cosY;

                g_shopProducts[pIdx].homePos = (Vector3){ wx, 10.64f, wz };

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



            if (distToCounter < 2.6f && (cartDistToCounter < 2.8f || g_cartProductIndices.size() > 0)) {

                if (IsKeyPressed(KEY_E) && g_printerState == PRINTER_IDLE) {

                    g_printerState = PRINTER_PRINTING;

                    g_printerProgress = 0.0f;

                    g_cartProductIndices.clear();

                    // (Footstep audio call removed)

                }

            }



            // Update printer animation

            if (g_printerState == PRINTER_PRINTING) {

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

                        Vector3 camFwd = Vector3Normalize((Vector3){ camera.target.x - camera.position.x, 0.0f, camera.target.z - camera.position.z });

                        g_thrownReceiptPos = (Vector3){ camera.position.x + camFwd.x * 1.1f, 10.03f, camera.position.z + camFwd.z * 1.1f };

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

                float dToCaveIn = Vector3Distance(camera.position, (Vector3){ 63.2f, 2.4f, 140.0f });

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

                    float dToBucket = Vector3Distance(camera.position, (Vector3){ 98.5f, 5.0f, 137.6f });

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

                        float dToRadio = Vector3Distance(camera.position, (Vector3){ 100.5f, 6.2f, 140.6f });

                        if (dToRadio < 11.0f) {

                            float vol = Clamp((1.0f - (dToRadio / 11.0f)) * 0.45f, 0.05f, 0.45f);

                            SetSoundVolume(g_sndRadioStatic, vol);

                            PlaySound(g_sndRadioStatic);

                        }

                    }

                }



                float dToTable = Vector3Distance(camera.position, (Vector3){ 100.2f, 6.2f, 140.0f });

                float dToChest = Vector3Distance(camera.position, (Vector3){ 100.4f, 5.5f, 137.8f });



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

        Vector3 sunDir = Vector3Normalize((Vector3){ cosf(sunTheta), sunElev, cosf(sunTheta) * 0.28f });

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

            Vector3 upRefMoon = (fabsf(moonForward.y) > 0.88f) ? (Vector3){0, 0, 1} : (Vector3){0, 1, 0};

            Vector3 moonRight = Vector3Normalize(Vector3CrossProduct(upRefMoon, moonForward));

            

            for(int row=0; row<5; row++) {

                for(int col=0; col<11; col++) {

                    char c = moonArt[row][col];

                    if (c == ' ') continue;

                    float dx = (col - 5) * 5.0f;

                    float dy = (2 - row) * 8.0f; 

                    Vector3 pos = Vector3Add(moonCenter, Vector3Scale(moonRight, dx));

                    pos = Vector3Add(pos, Vector3Scale((Vector3){0,1,0}, dy));

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

                Vector3 stepRgt = Vector3Normalize(Vector3CrossProduct(stepFwd, (Vector3){0, 1, 0}));

                float footOffset = g_isLeftFootStep ? -0.16f : 0.16f;

                Vector3 footPos = { camera.position.x + stepRgt.x * footOffset, 10.019f, camera.position.z + stepRgt.z * footOffset };

                float footYaw = atan2f(stepFwd.x, stepFwd.z) * RAD2DEG;



                if (inShop) {

                    // Footstep audio silenced per user request

                    AddFootstepTrail(footPos, footYaw, g_isLeftFootStep);

                    // Kick up subtle micro dust speck at shoe impact if under particle limit

                    if (g_dustParticles.size() < 48) {

                        DustParticle p;

                        p.pos = (Vector3){ footPos.x + Frand(-0.02f, 0.02f), 10.03f, footPos.z + Frand(-0.02f, 0.02f) };

                        p.vel = (Vector3){ Frand(-0.06f, 0.06f), Frand(0.06f, 0.14f), Frand(-0.06f, 0.06f) };

                        p.size = Frand(0.006f, 0.012f);

                        p.maxLife = Frand(0.6f, 1.0f);

                        p.life = p.maxLife;

                        p.color = (Color){ 165, 160, 150, 150 };

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

        Vector3 rightBob   = Vector3Normalize(Vector3CrossProduct(forwardBob, (Vector3){ 0.0f, 1.0f, 0.0f }));

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

            // CCTV SURVEILLANCE CAMERA VIEW: Clean viewpoint placed forward of roof edge so you see the pure world (no camera parts clip into screen!)

            Vector3 camPos = { 128.0f + roofCamSlideX, 15.35f, 133.05f };

            renderCam.position = camPos;



            // Compute look target facing South toward oncoming road + user pan/tilt

            float radPitch = roofCamPitch * DEG2RAD;

            float radYaw   = (180.0f + roofCamYaw) * DEG2RAD;

            Vector3 forwardDir = { sinf(radYaw) * cosf(radPitch), sinf(radPitch), cosf(radYaw) * cosf(radPitch) };

            renderCam.target = Vector3Add(camPos, forwardDir);

            renderCam.up = (Vector3){ 0.0f, 1.0f, 0.0f };

            renderCam.fovy = roofCamFOV;



            // Draw player '@' model visible from roof CCTV perspective (Bigger & 3D)

            Matrix mp = MatrixIdentity();

            mp.m0 = 0.2f; mp.m1 = 0.88f; mp.m2 = 0.35f; mp.m3 = 1.0f; // Terminal green

            mp.m4 = 1.45f; mp.m5 = 1.45f;

            mp.m8 = 0.0f; mp.m9 = 0.0f; mp.m10 = 0.0f; mp.m11 = 1.0f; // Billboard

            mp.m12 = camera.position.x;

            mp.m13 = camera.position.y - 0.7f;

            mp.m14 = camera.position.z;



            Vector3 camToCctvP = Vector3Normalize(Vector3Subtract(camera.position, renderCam.position));

            for (int slice = -2; slice <= 2; slice++) {

                Matrix sm = mp;

                sm.m12 += camToCctvP.x * (slice * 0.035f);

                sm.m13 += camToCctvP.y * (slice * 0.035f);

                sm.m14 += camToCctvP.z * (slice * 0.035f);

                float shade = (slice == 0) ? 1.0f : (1.0f - abs(slice) * 0.20f);

                sm.m0 *= shade; sm.m1 *= shade; sm.m2 *= shade;

                playerInstances['@'].push_back(sm);

            }

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

                const float SHOP_CAM_MAX_Z = 153.2f;

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

                maxT = Clamp(maxT, 0.25f, 1.0f);

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

                renderCam.target = (Vector3){ camera.position.x, camera.position.y - 0.20f, camera.position.z };

                renderCam.up = (Vector3){0, 1, 0};

            } else {

                // Outdoor elastic drone camera (5.0m distance, 1.5m height)

                Vector3 idealPos = Vector3Subtract(camera.position, Vector3Scale(forwardBob, 5.0f));

                idealPos.y += 1.5f;

                if (hitStopTimer <= 0.0f) {

                    renderCam.position = Vector3Lerp(renderCam.position, idealPos, rawDt * 8.0f);

                }

                renderCam.target = camera.position;

                renderCam.up = (Vector3){0, 1, 0};

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

                    renderCam.position = Vector3Add(renderCam.position, (Vector3){ jx, jy, jz });

                    renderCam.target   = Vector3Add(renderCam.target,   (Vector3){ jx * 1.6f, jy * 1.6f, jz * 1.6f });

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

            } else {

                m.m0 = 1.0f; m.m1 = 1.0f; m.m2 = 1.0f;

            }

            m.m3 = 1.0f;

            m.m4 = playerScale; m.m5 = playerScale;

            m.m8 = 3.0f; // Transparent entity

            m.m9 = 0.0f; 

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

            DrawCircle3D((Vector3){ camera.position.x, 10.018f, camera.position.z }, 0.45f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 8, 8, 12, 185 });

        } else {

            if (g_gameState == STATE_MAIN_MENU) {

                Vector2 mPos = GetMousePosition();
                float mNormX = Clamp((mPos.x / (float)curWinW - 0.5f) * 2.0f, -1.0f, 1.0f);
                float mNormY = Clamp((mPos.y / (float)curWinH - 0.5f) * 2.0f, -1.0f, 1.0f);

                // Smooth organic spring-damping parallax
                g_menuCamSmoothX += (mNormX - g_menuCamSmoothX) * Clamp(dt * 3.8f, 0.0f, 1.0f);
                g_menuCamSmoothY += (mNormY - g_menuCamSmoothY) * Clamp(dt * 3.8f, 0.0f, 1.0f);

                // Organic pupil tracking for the horned bovine skull
                g_skullEyeSmoothX += (mNormX - g_skullEyeSmoothX) * Clamp(dt * 7.5f, 0.0f, 1.0f);
                g_skullEyeSmoothY += (mNormY - g_skullEyeSmoothY) * Clamp(dt * 7.5f, 0.0f, 1.0f);

                float breatheY = sinf(timeVal * 0.75f) * 0.035f;
                float breatheX = cosf(timeVal * 0.45f) * 0.045f;

                Vector3 basePos, baseTgt;

                if (g_menuCCTVFeed == 0) {
                    // CAM 01: Elevated perspective showcasing college facade & grand steps
                    basePos = (Vector3){ 141.6f - g_menuCamSmoothX * 0.35f + breatheX * 0.4f, 13.85f + breatheY * 0.5f - g_menuCamSmoothY * 0.20f, 136.2f };
                    baseTgt = (Vector3){ 152.0f + g_menuCamSmoothX * 1.0f, 14.50f - g_menuCamSmoothY * 0.60f, 140.0f };
                } else if (g_menuCCTVFeed == 1) {
                    // CAM 02: Courtyard & South Facade - high crane perspective across rain-swept grounds towards entrance
                    basePos = (Vector3){ 136.5f - g_menuCamSmoothX * 0.55f + breatheX * 0.5f, 14.20f + breatheY * 0.4f - g_menuCamSmoothY * 0.30f, 122.0f };
                    baseTgt = (Vector3){ 155.0f + g_menuCamSmoothX * 1.2f, 12.00f - g_menuCamSmoothY * 0.70f, 138.0f };
                } else {
                    // CAM 03: Portico Vestibule Looking Out - under the grand portico looking out between columns into stormy night
                    basePos = (Vector3){ 152.2f - g_menuCamSmoothX * 0.25f + breatheX * 0.3f, 11.80f + breatheY * 0.5f - g_menuCamSmoothY * 0.15f, 140.0f };
                    baseTgt = (Vector3){ 136.0f + g_menuCamSmoothX * 1.1f, 11.20f - g_menuCamSmoothY * 0.50f, 137.0f };
                }

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
                renderCam.up       = (Vector3){ 0.0f, 1.0f, 0.0f };
                renderCam.fovy     = targetFov;

            } else {

                renderCam = camera;

                renderCam.fovy = g_camDynamicFov;

                renderCam.position = Vector3Add(renderCam.position, bobOffset);

                renderCam.target = Vector3Add(renderCam.target, bobOffset);

                if (g_waterState == WATER_STATE_SURFACE) {
                    renderCam.target.y += g_waterSmoothTiltPitch;
                }

                renderCam.up = rollUp;

            }



            // Apply Grethnar Jumpscare Zoom & Screen Shake

            if (grethnarState == GRETHNAR_JUMPSCARE) {

                renderCam.fovy = grethnarJumpscareFov;

                if (grethnarJumpscareShake > 0.005f) {

                    float jx = ((float)GetRandomValue(-100, 100) / 100.0f) * grethnarJumpscareShake * 0.18f;

                    float jy = ((float)GetRandomValue(-100, 100) / 100.0f) * grethnarJumpscareShake * 0.16f;

                    float jz = ((float)GetRandomValue(-100, 100) / 100.0f) * grethnarJumpscareShake * 0.12f;

                    renderCam.position = Vector3Add(renderCam.position, (Vector3){ jx, jy, jz });

                    renderCam.target   = Vector3Add(renderCam.target,   (Vector3){ jx * 1.6f, jy * 1.6f, jz * 1.6f });

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
                    if (Vector3Length(refFwd) < 0.01f) refFwd = (Vector3){ 0.0f, 0.0f, 1.0f };
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
        UpdateShopWashroomMirror(camera, ApplyShopLighting, g_shopLightsOn, timeVal);

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

        ClearBackground(baseSkyClear); 

        

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

        

        bool isDeepInStore = (camera.position.x <= 104.0f && camera.position.x >= 86.2f &&

                              camera.position.z >= 126.2f && camera.position.z <= 153.8f);

        float activeCullDistSq = isDeepInStore ? (38.0f * 38.0f) : cullDistSq;



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

        

        // 3D Cloud Ground Shadows on Highway, Apron, and Terrain
        if (!isUnderwaterScene) {
            DrawCloudGroundShadows(renderCam, sunDir, sunElev);
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



            return (Color){

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
                    DrawCylinder(mhPos, 0.44f, 0.44f, 0.015f, 14, (Color){ 32, 30, 28, 255 });
                    DrawCylinder((Vector3){ mhPos.x, mhPos.y + 0.016f, mhPos.z }, 0.12f, 0.12f, 0.02f, 10, (Color){ 180, 145, 55, 255 });
                    DrawCircle3D(mhPos, 0.46f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 220, 180, 35, 180 });
                }

                // 6. ATTENDANT SERVICE BELL ON CENTRAL PILLAR (Z = 140.0)
                Vector3 bellPos = { 127.72f, 11.45f, 140.0f };
                DrawSphere(bellPos, 0.065f, (Color){ 215, 175, 55, 255 });
                DrawCylinder((Vector3){ bellPos.x, 11.38f, bellPos.z }, 0.08f, 0.08f, 0.02f, 12, (Color){ 45, 42, 38, 255 });

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
                    DrawCatenaryHose(pumpOutlet, hoseCoupling, 0.90f, 16, 0.026f, (Color){ 16, 16, 18, 255 });

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
                    Color crtCol = fluorLightOn ? (Color){ 6, 26, 12, 255 } : (Color){ 2, 8, 4, 255 };
                    DrawCube({ sx, 11.75f, pz }, 0.040f, 0.64f, 0.54f, crtCol);
                    
                    // In-World 3D Textured CRT Screen
                    if (g_pumpScreenRTLoaded) {
                        float screenXOffset = (s == 0) ? -0.023f : 0.023f;
                        Color crtTint = fluorLightOn ? WHITE : (Color){ 85, 95, 90, 255 };
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
                    DrawCube((Vector3){ 0.0f, 0.0f, 0.0f }, 0.06f, 0.03f, 0.015f, { 220, 25, 20, 255 }); // Red flow impeller
                    rlPopMatrix();
                }

                // Left Lane Nozzle Cradle & Heavy Rubber Hose (West face, X = 127.42)
                bool leftCradleOccupied = (!g_holdingFuelNozzle && !g_nozzleInCar) || (g_activePumpIndex != pi && (g_customerCar.state == CAR_INACTIVE || g_customerCar.targetPump != pi));
                if (leftCradleOccupied) {
                    DrawCube({ 127.42f, 11.1f, pz - 0.25f }, 0.08f, 0.18f, 0.12f, { 18, 18, 18, 255 });
                    DrawCube({ 127.38f, 11.16f, pz - 0.25f }, 0.06f, 0.14f, 0.07f, { 70, 75, 82, 255 });
                    Vector3 pumpOutlet = { 127.42f, 11.2f, pz - 0.25f };
                    Vector3 cradleBase = { 127.42f, 10.3f, pz - 0.25f };
                    DrawCatenaryHose(pumpOutlet, cradleBase, 0.35f, 8, 0.024f, (Color){ 16, 16, 18, 255 });
                }
                // Right Lane Nozzle Cradle & Heavy Rubber Hose (East face, X = 128.58)
                DrawCube({ 128.58f, 11.1f, pz + 0.25f }, 0.08f, 0.18f, 0.12f, { 18, 18, 18, 255 });
                DrawCube({ 128.62f, 11.16f, pz + 0.25f }, 0.06f, 0.14f, 0.07f, { 70, 75, 82, 255 });
                Vector3 pumpOutletR = { 128.58f, 11.2f, pz + 0.25f };
                Vector3 cradleBaseR = { 128.58f, 10.3f, pz + 0.25f };
                DrawCatenaryHose(pumpOutletR, cradleBaseR, 0.35f, 8, 0.024f, (Color){ 16, 16, 18, 255 });
            }


            // Customer car fuel vapor shimmer & exhaust smoke feedback
            if (g_customerCar.state == CAR_REFUELING && (IsKeyDown(KEY_E) || IsMouseButtonDown(MOUSE_BUTTON_LEFT))) {
                Vector3 flapPos = { g_customerCar.pos.x + 0.98f, g_customerCar.pos.y + 0.85f, g_customerCar.pos.z - 0.85f };
                for (int v = 0; v < 3; v++) {
                    float vy = flapPos.y + (float)v * 0.12f + sinf(timeVal * 12.0f + (float)v) * 0.05f;
                    float vz = flapPos.z + cosf(timeVal * 8.0f + (float)v) * 0.06f;
                    DrawCube((Vector3){ flapPos.x, vy, vz }, 0.06f, 0.06f, 0.06f, (Color){ 200, 200, 190, 45 }); // Vapor shimmer
                }
            } else if (g_customerCar.state == CAR_DEPARTING) {
                Vector3 exhaustPos = { g_customerCar.pos.x - 0.70f, 10.35f, g_customerCar.pos.z - 2.2f };
                for (int ex = 0; ex < 4; ex++) {
                    float exZ = exhaustPos.z - (float)ex * 0.45f;
                    float exY = exhaustPos.y + (float)ex * 0.14f;
                    DrawSphere((Vector3){ exhaustPos.x, exY, exZ }, 0.12f + (float)ex * 0.08f, (Color){ 45, 45, 48, (unsigned char)(140 - ex * 30) });
                }
            }

            // 5. Forecourt Oil Slicks on Asphalt in both lanes

            DrawCube({ 121.5f, 10.02f, 140.0f }, 2.8f, 0.01f, 4.5f, { 8, 16, 10, 170 }); // Left Lane

            DrawCube({ 134.5f, 10.02f, 140.0f }, 2.8f, 0.01f, 4.5f, { 8, 16, 10, 170 }); // Right Lane



            // 6. Central Roadside Sign (RUSTY OIL CO.) at Median X: 128.0, Z: 131.0

            DrawCube({ 128.0f, 13.0f, 131.0f }, 0.25f, 6.0f, 0.25f, { 32, 28, 24, 255 });

            DrawCube({ 128.0f, 16.4f, 131.0f }, 3.6f, 1.8f, 0.20f, { 18, 12, 10, 255 });

            Color signNeon = fluorLightOn ? (Color){ (unsigned char)(190 * flk), 10, 20, 255 } : (Color){ 25, 0, 5, 255 };

            DrawCubeWires({ 128.0f, 16.4f, 131.0f }, 3.65f, 1.85f, 0.24f, signNeon);



            // =====================================================================

            // 7. FRONT ROOF WALL GUIDE RAIL & CREEPING CCTV BOX CAMERA

            // Mounted on the front roof edge wall. Appears fully when viewed from outside,

            // but hidden while looking through it so you see the pure world!

            // =====================================================================

            {

                // 1. Standoff Brackets anchoring the rail forward from the front canopy fascia (Y = 15.22, Z = 133.25)

                float bracketXs[4] = { 124.6f, 126.8f, 129.2f, 131.4f };

                for (int b = 0; b < 4; b++) {

                    DrawCube({ bracketXs[b], 15.22f, 133.25f }, 0.10f, 0.16f, 0.26f, { 25, 25, 28, 255 });

                }



                // 2. Continuous Front Roof Wall Steel Rail Tube (X: 124.5..131.5, length 7.0m, Y = 15.35, Z = 133.20)

                DrawCube({ 128.0f, 15.35f, 133.20f }, 7.0f, 0.08f, 0.08f, { 35, 36, 40, 255 });

                // End Stopper Caps

                DrawCube({ 124.45f, 15.35f, 133.20f }, 0.14f, 0.22f, 0.14f, { 20, 20, 24, 255 });

                DrawCube({ 131.55f, 15.35f, 133.20f }, 0.14f, 0.22f, 0.14f, { 20, 20, 24, 255 });



                // 3. PHYSICAL CCTV CAMERA ASSEMBLY (Appears fully when viewed from ground; hidden during CCTV mode so view is unobstructed!)

                if (!isRoofCamActive) {

                    float curCamX = 128.0f + roofCamSlideX;



                    // Sliding Motorized Carriage gripping the rail

                    DrawCube({ curCamX, 15.35f, 133.20f }, 0.34f, 0.12f, 0.22f, { 26, 26, 30, 255 });



                    // Articulated Swivel Gimbal Drop Bracket

                    DrawCube({ curCamX, 15.42f, 133.16f }, 0.14f, 0.16f, 0.14f, { 42, 42, 46, 255 });



                    // Dynamic rotation for camera body based on yaw/pitch

                    float radYaw   = (180.0f + roofCamYaw) * DEG2RAD;

                    float radPitch = roofCamPitch * DEG2RAD;

                    Vector3 fwd = { sinf(radYaw) * cosf(radPitch), sinf(radPitch), cosf(radYaw) * cosf(radPitch) };

                    Vector3 rgt = { fwd.z, 0.0f, -fwd.x };



                    // Weatherproof Rectangular CCTV Box Housing (Crisp security off-white/beige)

                    Vector3 boxPos = { curCamX + fwd.x * 0.08f, 15.48f + fwd.y * 0.08f, 133.10f + fwd.z * 0.08f };

                    DrawCube(boxPos, 0.36f, 0.24f, 0.46f, { 226, 226, 230, 255 });

                    DrawCubeWires(boxPos, 0.365f, 0.245f, 0.465f, { 90, 90, 95, 255 });



                    // Matte Black Protective Sunshield / Rain Visor extending over top and front

                    Vector3 visorPos = { boxPos.x + fwd.x * 0.05f, boxPos.y + 0.13f, boxPos.z + fwd.z * 0.05f };

                    DrawCube(visorPos, 0.40f, 0.04f, 0.52f, { 35, 36, 40, 255 });



                    // Protruding Dark Cylindrical Lens Barrel

                    Vector3 lensPos = { boxPos.x + fwd.x * 0.24f, boxPos.y - 0.02f + fwd.y * 0.24f, boxPos.z + fwd.z * 0.24f };

                    DrawSphere(lensPos, 0.085f, { 12, 14, 18, 255 });



                    // Active Blinking Red Recording LED beside lens (● REC)

                    bool recBlink = (fmodf(timeVal, 0.8f) < 0.4f);

                    Vector3 ledPos = { lensPos.x + rgt.x * 0.12f, lensPos.y + 0.07f, lensPos.z + rgt.z * 0.12f };

                    DrawSphere(ledPos, 0.035f, recBlink ? (Color){ 255, 12, 12, 255 } : (Color){ 70, 0, 0, 255 });

                }

            }

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

            DrawCube((Vector3){ 128.0f, 10.012f, 140.0f }, 4.6f, 0.005f, 14.6f, (Color){ 10, 10, 14, 185 });

            DrawCircle3D((Vector3){ 128.0f, 10.015f, 135.0f }, 0.85f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 8, 8, 12, 195 });

            DrawCircle3D((Vector3){ 128.0f, 10.015f, 145.0f }, 0.85f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 8, 8, 12, 195 });



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

                DrawCircle3D({ 113.5f, 10.015f, 127.0f }, 0.75f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 10, 10, 14, 175 });

                DrawCylinder({ 113.5f, 10.0f, 127.0f }, 0.09f, 0.09f, 4.8f, 8, { 35, 38, 42, 255 });

                DrawCube({ 113.0f, 14.7f, 127.0f }, 1.0f, 0.07f, 0.07f, { 35, 38, 42, 255 });

                DrawSphere({ 112.6f, 14.65f, 127.0f }, 0.18f, { 255, 215, 110, 245 });

                DrawCircle3D({ 112.6f, 10.028f, 127.0f }, 4.5f, { 1, 0, 0 }, 90.0f, { 75, 65, 25, 75 });



                // North Lot Lamp Post

                DrawCircle3D({ 113.5f, 10.015f, 153.0f }, 0.75f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 10, 10, 14, 175 });

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

                DrawCube({ 83.8f, 10.005f, 140.0f }, 2.4f, 0.03f, 2.8f, (Color){ 30, 32, 36, 255 }); // Dark iron rim

                DrawCubeWires({ 83.8f, 10.010f, 140.0f }, 2.42f, 0.035f, 2.82f, (Color){ 18, 20, 22, 255 });



                // Corner mounting anchor bolts

                DrawSphere({ 82.8f, 10.025f, 138.8f }, 0.04f, (Color){ 70, 72, 78, 255 });

                DrawSphere({ 84.8f, 10.025f, 138.8f }, 0.04f, (Color){ 70, 72, 78, 255 });

                DrawSphere({ 82.8f, 10.025f, 141.2f }, 0.04f, (Color){ 70, 72, 78, 255 });

                DrawSphere({ 84.8f, 10.025f, 141.2f }, 0.04f, (Color){ 70, 72, 78, 255 });



                // B. HEAVY REINFORCED INDUSTRIAL STEEL BLAST HATCH

                if (g_hatchAnim < 0.05f) {

                    // Closed Hatch Door Leaf

                    Vector3 hPos = { 83.8f, 10.02f, 140.0f };

                    DrawCube(hPos, 2.1f, 0.05f, 2.5f, (Color){ 48, 46, 44, 255 }); // Rusted steel diamond-plate

                    DrawCubeWires(hPos, 2.11f, 0.055f, 2.51f, (Color){ 24, 22, 20, 255 });

                    // Yellow & Black Industrial Hazard Stripes along border

                    DrawCube({ hPos.x, hPos.y + 0.01f, hPos.z - 1.15f }, 2.0f, 0.01f, 0.14f, (Color){ 195, 155, 30, 255 });

                    DrawCube({ hPos.x, hPos.y + 0.01f, hPos.z + 1.15f }, 2.0f, 0.01f, 0.14f, (Color){ 195, 155, 30, 255 });

                    DrawCube({ hPos.x - 0.95f, hPos.y + 0.01f, hPos.z }, 0.14f, 0.01f, 2.2f, (Color){ 195, 155, 30, 255 });

                    DrawCube({ hPos.x + 0.95f, hPos.y + 0.01f, hPos.z }, 0.14f, 0.01f, 2.2f, (Color){ 195, 155, 30, 255 });

                    // Heavy Rotary Latching Dog-Wheel & Center Lock Spindle

                    DrawCylinder({ hPos.x, hPos.y + 0.02f, hPos.z }, 0.22f, 0.22f, 0.06f, 12, (Color){ 32, 34, 38, 255 });

                    DrawCylinder({ hPos.x, hPos.y + 0.05f, hPos.z }, 0.06f, 0.06f, 0.08f, 8, (Color){ 65, 68, 75, 255 });

                    // Dual heavy steel slide bolts

                    DrawCube({ hPos.x, hPos.y + 0.025f, hPos.z - 0.55f }, 1.6f, 0.04f, 0.09f, (Color){ 75, 78, 85, 255 });

                    DrawCube({ hPos.x, hPos.y + 0.025f, hPos.z + 0.55f }, 1.6f, 0.04f, 0.09f, (Color){ 75, 78, 85, 255 });

                } else {

                    // Open Hatch Door Leaf angled smoothly back on heavy greased hinges

                    float hAngle = g_hatchAnim * 95.0f;

                    float radH = hAngle * DEG2RAD;

                    Vector3 hPivot = { 82.75f, 10.02f, 140.0f };

                    Vector3 hCenter = { hPivot.x - sinf(radH) * 1.05f, hPivot.y + cosf(radH) * 1.05f, 140.0f };

                    DrawCube(hCenter, 0.08f, 2.1f, 2.5f, (Color){ 48, 46, 44, 255 });

                    DrawCubeWires(hCenter, 0.085f, 2.11f, 2.51f, (Color){ 24, 22, 20, 255 });

                    // Heavy hinge brackets

                    DrawCube({ 82.75f, 10.05f, 139.2f }, 0.24f, 0.12f, 0.16f, (Color){ 32, 34, 38, 255 });

                    DrawCube({ 82.75f, 10.05f, 140.8f }, 0.24f, 0.12f, 0.16f, (Color){ 32, 34, 38, 255 });

                }



                // PROPPED HEAVY TRENCH SHOVEL NEAR CARPET (Blade on ground, handle resting on timber)

                if (!g_hasShovel) {

                    // Weathered timber resting block supporting the leaning shovel shaft

                    DrawCube((Vector3){ 84.40f, 10.20f, 138.15f }, 0.30f, 0.36f, 0.45f, (Color){ 58, 42, 28, 255 });

                    DrawCubeWires((Vector3){ 84.40f, 10.20f, 138.15f }, 0.31f, 0.37f, 0.46f, (Color){ 32, 22, 16, 255 });



                    // Shovel blade firmly on earth at Y=10.02, shaft propped up against timber at Y=10.71

                    Matrix sRot = MatrixMultiply(MatrixRotateX(45.0f * DEG2RAD), MatrixRotateY(-15.0f * DEG2RAD));

                    Matrix sWorld = MatrixMultiply(sRot, MatrixTranslate(84.40f, 10.71f, 138.20f));

                    Vector3 lDirWorld = { -0.2f, -1.0f, -0.3f };

                    DrawShovel(g_shovelRig, sWorld, camera.position, lDirWorld);



                    // Subtle atmospheric interaction glow halo where the steel blade meets the dirt

                    float shGlow = 0.5f + 0.35f * sinf(timeVal * 4.0f);

                    DrawCircle3D((Vector3){ 84.22f, 10.035f, 138.87f }, 0.45f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 255, 215, 80, (unsigned char)(45 * shGlow) });

                }



                // C. WORN BURGUNDY CARPET (Lying flat over hatch or rolled back)

                float cSlideX = g_carpetAnim * 3.2f;

                float cFoldY  = g_carpetAnim * 0.22f;

                float cScaleX = 2.6f - g_carpetAnim * 1.1f;

                Vector3 cPos = { 83.8f - cSlideX, 10.05f + cFoldY * 0.5f, 140.0f };

                // Heavy wool fabric base

                DrawCube(cPos, cScaleX, 0.03f + cFoldY, 3.4f, (Color){ 105, 18, 22, 255 });

                DrawCube({ cPos.x, cPos.y + 0.006f, cPos.z }, cScaleX * 0.88f, 0.03f, 3.0f, (Color){ 165, 125, 35, 255 });

                DrawCube({ cPos.x, cPos.y + 0.010f, cPos.z }, cScaleX * 0.74f, 0.03f, 2.6f, (Color){ 75, 12, 15, 255 });

                // Fringe & damp mud weathering

                DrawCube({ cPos.x, cPos.y + 0.003f, cPos.z - 1.72f }, cScaleX, 0.015f, 0.12f, (Color){ 175, 160, 125, 240 });

                DrawCube({ cPos.x, cPos.y + 0.003f, cPos.z + 1.72f }, cScaleX, 0.015f, 0.12f, (Color){ 175, 160, 125, 240 });



                // D. ENTRANCE SHAFT & DESCENDING STEEL STAIRS (Under the Hatch, strictly Y <= 10.0)

                if (g_tunnelHatchOpen) {

                    // Dark steel & concrete lined shaft interior walls

                    DrawCube({ 83.8f, 8.0f, 138.6f }, 2.4f, 4.0f, 0.20f, (Color){ 26, 28, 32, 255 }); // South wall

                    DrawCube({ 83.8f, 8.0f, 141.4f }, 2.4f, 4.0f, 0.20f, (Color){ 26, 28, 32, 255 }); // North wall

                    DrawCube({ 85.0f, 8.0f, 140.0f }, 0.20f, 4.0f, 2.6f, (Color){ 26, 28, 32, 255 }); // East wall



                    // Heavy welded steel diamond-plate stairs descending from Y=10.0 down to Y=6.5 at X=82.0

                    for (int step = 0; step < 8; step++) {

                        float stFrac = (float)step / 7.0f;

                        float stX = 84.6f - stFrac * 2.6f;

                        float stY = 9.8f - stFrac * 3.3f;

                        // Steel step tread

                        DrawCube({ stX, stY, 140.0f }, 0.36f, 0.08f, 2.2f, (Color){ 52, 50, 48, 255 });

                        // Safety yellow abrasive nosing strip

                        DrawCube({ stX - 0.16f, stY + 0.01f, 140.0f }, 0.05f, 0.08f, 2.18f, (Color){ 210, 175, 40, 255 });

                    }

                    // Tubular steel safety handrails inside the shaft

                    DrawLine3D({ 84.6f, 10.4f, 139.0f }, { 82.0f, 7.3f, 139.0f }, (Color){ 55, 58, 65, 255 });

                    DrawLine3D({ 84.6f, 10.4f, 141.0f }, { 82.0f, 7.3f, 141.0f }, (Color){ 55, 58, 65, 255 });

                    // Cold green-blue industrial mist wafting from shaft

                    DrawCube({ 83.4f, 7.5f, 140.0f }, 2.0f, 2.8f, 2.2f, (Color){ 120, 180, 200, 28 });

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

                        DrawCube({ cx, cy - 0.12f, 140.0f }, dx, 0.24f, 5.8f, (Color){ 36, 34, 33, 255 });

                        // Raised floor plating (Left & Right walkways)

                        DrawCube({ cx, cy + 0.02f, 138.4f }, dx * 0.95f, 0.06f, 2.2f, (Color){ 48, 45, 43, 255 });

                        DrawCube({ cx, cy + 0.02f, 141.6f }, dx * 0.95f, 0.06f, 2.2f, (Color){ 48, 45, 43, 255 });

                        // Recessed Center Drainage Trench with Rusted Iron Grate

                        DrawCube({ cx, cy - 0.08f, 140.0f }, dx, 0.14f, 1.1f, (Color){ 16, 15, 14, 255 });

                        DrawCube({ cx, cy - 0.02f, 140.0f }, dx * 0.9f, 0.02f, 1.0f, (Color){ 30, 28, 26, 255 }); // Grating

                        // Stagnant black fluid / dried blood in trench

                        if (s % 2 == 0) {

                            DrawCube({ cx, cy - 0.04f, 140.0f }, dx * 0.7f, 0.01f, 0.85f, (Color){ 35, 10, 12, 230 });

                        }



                        // 2. Dark Rusted Corrugated Steel Walls

                        DrawCube({ cx, cy + wallH * 0.5f, 137.1f }, dx, wallH, 0.35f, (Color){ 42, 40, 38, 255 }); // South wall

                        DrawCube({ cx, cy + wallH * 0.5f, 142.9f }, dx, wallH, 0.35f, (Color){ 42, 40, 38, 255 }); // North wall

                        // Heavy Rusted Corrugated Arched Steel Ceiling

                        DrawCube({ cx, ceilY, 140.0f }, dx, 0.32f, 6.0f, (Color){ 32, 30, 28, 255 });



                        // 3. Heavy Industrial Steel I-Beam Rib Bulkheads (Every 3 segments)

                        if (s % 3 == 0) {

                            Color ibCol = { 54, 52, 50, 255 };

                            // South & North vertical columns

                            DrawCube({ cx, cy + wallH * 0.5f, 137.35f }, 0.28f, wallH, 0.28f, ibCol);

                            DrawCube({ cx, cy + wallH * 0.5f, 142.65f }, 0.28f, wallH, 0.28f, ibCol);

                            // Overhead arched I-Beam cross-beam

                            DrawCube({ cx, ceilY - 0.16f, 140.0f }, 0.30f, 0.28f, 5.5f, ibCol);

                            // Riveted triangular corner gussets

                            DrawCube({ cx, ceilY - 0.40f, 137.75f }, 0.26f, 0.32f, 0.32f, (Color){ 40, 38, 36, 255 });

                            DrawCube({ cx, ceilY - 0.40f, 142.25f }, 0.26f, 0.32f, 0.32f, (Color){ 40, 38, 36, 255 });



                            // Yellow/Black Hazard Stencil on bulkheads

                            DrawCube({ cx + 0.15f, cy + 1.6f, 137.45f }, 0.02f, 0.6f, 0.18f, (Color){ 190, 150, 25, 240 });

                            DrawCube({ cx + 0.15f, cy + 1.6f, 142.55f }, 0.02f, 0.6f, 0.18f, (Color){ 190, 150, 25, 240 });



                            // Caged Industrial Emergency Bulkhead Lamp (At stations s=3 and s=9)

                            if (s == 3 || s == 9) {

                                Vector3 lampP = { cx, ceilY - 0.42f, 140.0f };

                                float flicker = 0.75f + 0.25f * sinf((float)GetTime() * 8.0f + (float)s * 3.0f);

                                Color glowCol = { (unsigned char)(255 * flicker), (unsigned char)(170 * flicker), 45, 255 };

                                DrawSphere(lampP, 0.10f, glowCol);

                                // Protective steel wire cage

                                DrawCubeWires(lampP, 0.24f, 0.28f, 0.24f, (Color){ 60, 58, 55, 240 });

                                // Sickly pool of light on the diamond-plate floor

                                DrawCircle3D((Vector3){ cx, cy + 0.05f, 140.0f }, 4.4f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 200, 120, 25, (unsigned char)(40 * flicker) });

                            }

                        }



                        // 4. Overhead Industrial Utility Pipes & Sagging Black Electrical Cables

                        // High-pressure steam/water utility pipes along North wall

                        DrawCylinderEx({ x0, cy + 2.8f, 142.45f }, { x1, cy + 2.8f, 142.45f }, 0.07f, 0.07f, 6, (Color){ 75, 45, 35, 255 });

                        DrawCylinderEx({ x0, cy + 2.6f, 142.45f }, { x1, cy + 2.6f, 142.45f }, 0.05f, 0.05f, 6, (Color){ 65, 40, 30, 255 });

                        // Sagging black rubber electrical cable bundle along ceiling

                        float cableSag = 0.14f * sinf(frac0 * PI * 12.0f);

                        DrawLine3D({ x0, ceilY - 0.25f, 138.8f }, { x1, ceilY - 0.25f - cableSag, 138.8f }, (Color){ 20, 20, 22, 255 });

                        DrawLine3D({ x0, ceilY - 0.28f, 139.0f }, { x1, ceilY - 0.28f - cableSag, 139.0f }, (Color){ 22, 22, 24, 255 });

                    }



                    // --- CRUSHED BULKHEAD CAVE-IN OBSTRUCTION (~38% DEPTH: X = 62.0, Y = 1.2) ---

                    if (!g_tunnelDug) {

                        // Catastrophic structural collapse: buckled corrugated steel plates, sheared I-beams & rock

                        DrawCube({ 62.0f, 2.8f, 140.0f }, 2.6f, 4.2f, 5.8f, (Color){ 34, 32, 30, 255 }); // Core impassable rubble

                        // Buckled, torn corrugated steel siding crushed inward

                        DrawCube({ 62.6f, 2.4f, 138.6f }, 0.9f, 2.2f, 2.0f, (Color){ 52, 48, 44, 255 });

                        DrawCube({ 62.5f, 3.2f, 141.4f }, 1.1f, 1.8f, 1.8f, (Color){ 46, 44, 42, 255 });

                        // Sheared twisted structural I-Beams protruding out at angles

                        DrawCube({ 62.8f, 2.6f, 139.5f }, 0.22f, 3.0f, 0.22f, (Color){ 68, 64, 60, 255 });

                        DrawCube({ 62.7f, 3.5f, 140.6f }, 0.24f, 0.24f, 2.6f, (Color){ 62, 58, 54, 255 });

                        // Fractured rebar mesh and sharp bedrock slabs

                        DrawCube({ 62.9f, 1.7f, 140.2f }, 0.8f, 1.3f, 2.2f, (Color){ 38, 36, 34, 255 });

                        DrawLine3D({ 62.9f, 3.2f, 139.0f }, { 63.3f, 1.8f, 141.0f }, (Color){ 90, 85, 80, 255 });

                        DrawLine3D({ 62.9f, 2.2f, 141.5f }, { 63.4f, 3.6f, 139.5f }, (Color){ 90, 85, 80, 255 });

                    } else {

                        // Excavated passage breach: sheared steel and rubble cleared to the flanks

                        DrawCube({ 62.0f, 1.8f, 137.6f }, 2.4f, 2.4f, 1.2f, (Color){ 42, 40, 38, 255 });

                        DrawCube({ 62.0f, 1.8f, 142.4f }, 2.4f, 2.4f, 1.2f, (Color){ 42, 40, 38, 255 });

                        // Excavated trench through the buckled diamond-plate floor

                        DrawCube({ 62.0f, 1.12f, 140.0f }, 2.6f, 0.06f, 3.4f, (Color){ 22, 20, 18, 255 });

                        // Cold draft & blue haze drifting from the lower abyss

                        DrawCube({ 61.2f, 2.5f, 140.0f }, 2.0f, 3.2f, 3.4f, (Color){ 130, 185, 210, 32 });

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

                            DrawCube({ cx, cy - 0.14f, 140.0f }, dx, 0.28f, 5.8f, (Color){ 28, 26, 26, 255 });

                            // Murky sludgy water pooling over the steel plates

                            DrawCube({ cx, cy + 0.02f, 140.0f }, dx * 0.95f, 0.05f, 5.4f, (Color){ 20, 22, 25, 255 });

                            if (ds % 3 == 0) {

                                DrawCube({ cx, cy + 0.05f, 140.0f }, dx * 0.8f, 0.01f, 3.8f, (Color){ 10, 14, 18, 235 });

                            }



                            // Curved corrugated steel conduit walls & ceiling

                            DrawCube({ cx, cy + 2.0f, 137.1f }, dx, 4.2f, 0.40f, (Color){ 32, 30, 30, 255 });

                            DrawCube({ cx, cy + 2.0f, 142.9f }, dx, 4.2f, 0.40f, (Color){ 32, 30, 30, 255 });

                            DrawCube({ cx, cy + 4.1f, 140.0f }, dx, 0.40f, 6.0f, (Color){ 26, 25, 26, 255 });



                            // Heavy Circular Submarine Pressure Bulkhead Rings every 4 steps

                            if (ds % 4 == 0) {

                                Color ringCol = { 46, 44, 46, 255 };

                                DrawCube({ cx, cy + 2.0f, 137.35f }, 0.38f, 4.2f, 0.38f, ringCol);

                                DrawCube({ cx, cy + 2.0f, 142.65f }, 0.38f, 4.2f, 0.38f, ringCol);

                                DrawCube({ cx, cy + 3.9f, 140.0f }, 0.38f, 0.38f, 5.6f, ringCol);



                                // Severed conduit cables with periodic electric blue spark

                                if (ds == 8 || ds == 12) {

                                    DrawLine3D({ cx, cy + 3.8f, 142.5f }, { cx + 0.3f, cy + 2.6f, 142.1f }, (Color){ 25, 25, 28, 255 });

                                    if ((int)(GetTime() * 5.0f) % 3 == 0) {

                                        DrawSphere({ cx + 0.3f, cy + 2.6f, 142.1f }, 0.08f, (Color){ 140, 220, 255, 255 });

                                    }

                                }

                            }

                        }



                        // Massive Blown-Out Blast Door Breach at Cliffside (X = 32.0, Y = -16.0)

                        // Opening out of the mountain bedrock into The Abandoned Village

                        DrawCube({ 32.0f, -13.8f, 140.0f }, 0.8f, 4.8f, 5.8f, (Color){ 24, 22, 24, 255 });

                        // Heavy sheared pressure door propped against rock wall

                        DrawCube({ 31.6f, -14.2f, 137.8f }, 0.16f, 3.8f, 1.8f, (Color){ 48, 45, 44, 255 });

                    }

                }



                // =================================================================

                // 2. THE CREEPY ABANDONED VILLAGE WORLD (Elevation Y = -16.0)

                // Spanning X: -45 to 32, Z: 105 to 175

                // =================================================================

                bool inVillage = (camera.position.x <= 40.0f && camera.position.y <= -2.0f);

                if (inVillage) {

                    // A. Vast Sunken Valley Ground Plane

                    DrawCube({ -6.5f, -16.12f, 140.0f }, 78.0f, 0.24f, 70.0f, (Color){ 18, 22, 17, 255 });

                    // Muddy cart tracks and cobblestone path winding through village

                    DrawCube({ 3.5f, -16.00f, 140.0f }, 58.0f, 0.02f, 3.2f, (Color){ 28, 25, 20, 255 });

                    // Rolling ground mist sheets

                    DrawCube({ -6.5f, -15.55f, 140.0f }, 76.0f, 0.65f, 68.0f, (Color){ 175, 195, 210, 32 });



                    // Surrounding Dark Mountain Cliff Backdrops

                    DrawCube({ -45.5f, -8.0f, 140.0f }, 2.0f, 18.0f, 72.0f, (Color){ 14, 16, 18, 255 }); // West cliff

                    DrawCube({ 32.5f, -8.0f, 122.0f }, 2.0f, 18.0f, 34.0f, (Color){ 14, 16, 18, 255 });  // East cliff South

                    DrawCube({ 32.5f, -8.0f, 158.0f }, 2.0f, 18.0f, 34.0f, (Color){ 14, 16, 18, 255 });  // East cliff North

                    DrawCube({ -6.5f, -8.0f, 104.5f }, 80.0f, 18.0f, 2.0f, (Color){ 14, 16, 18, 255 });  // South cliff

                    DrawCube({ -6.5f, -8.0f, 175.5f }, 80.0f, 18.0f, 2.0f, (Color){ 14, 16, 18, 255 });  // North cliff



                    // B. THE ANCIENT STONE WELL (Village Center X = 8.0, Z = 140.0)

                    {

                        Vector3 wellP = { 8.0f, -15.45f, 140.0f };

                        // Circular stone cylinder well lip

                        DrawCylinder(wellP, 1.25f, 1.25f, 1.1f, 14, (Color){ 62, 65, 70, 255 });

                        DrawCylinder((Vector3){ wellP.x, wellP.y + 0.05f, wellP.z }, 0.95f, 0.95f, 1.2f, 14, (Color){ 8, 9, 11, 255 }); // Dark interior void

                        // Weathered timber upright canopy posts

                        DrawCube({ 8.0f, -13.8f, 138.9f }, 0.16f, 2.4f, 0.16f, (Color){ 68, 50, 32, 255 });

                        DrawCube({ 8.0f, -13.8f, 141.1f }, 0.16f, 2.4f, 0.16f, (Color){ 68, 50, 32, 255 });

                        // Crank spindle axle and wooden drum

                        DrawCylinderEx({ 8.0f, -13.6f, 138.9f }, { 8.0f, -13.6f, 141.1f }, 0.07f, 0.07f, 8, (Color){ 52, 38, 24, 255 });

                        // A-frame shingled roof canopy

                        DrawCube({ 8.0f, -12.5f, 140.0f }, 1.8f, 0.22f, 2.6f, (Color){ 44, 34, 24, 255 });

                        DrawLine3D({ 7.1f, -12.7f, 140.0f }, { 8.0f, -12.2f, 140.0f }, (Color){ 75, 55, 35, 255 });

                        // Frayed rope & wooden bucket

                        DrawLine3D({ 8.0f, -13.6f, 140.0f }, { 8.0f, -15.2f, 140.0f }, (Color){ 160, 145, 115, 255 });

                        DrawCylinder({ 8.0f, -15.35f, 140.0f }, 0.14f, 0.12f, 0.22f, 8, (Color){ 75, 52, 32, 255 });

                    }



                    // C. THE ELDER'S COTTAGE (North, X = 6.0, Z = 160.0)

                    {

                        Vector3 cabP = { 6.0f, -16.0f, 160.0f };

                        // Wooden floor foundation

                        DrawCube({ cabP.x, cabP.y + 0.15f, cabP.z }, 8.0f, 0.30f, 6.4f, (Color){ 48, 36, 24, 255 });

                        // Weathered horizontal log walls

                        DrawCube({ cabP.x, cabP.y + 2.0f, cabP.z + 3.1f }, 8.0f, 3.5f, 0.22f, (Color){ 58, 44, 30, 255 }); // Back (North)

                        DrawCube({ cabP.x - 3.9f, cabP.y + 2.0f, cabP.z }, 0.22f, 3.5f, 6.4f, (Color){ 58, 44, 30, 255 }); // West

                        DrawCube({ cabP.x + 3.9f, cabP.y + 2.0f, cabP.z }, 0.22f, 3.5f, 6.4f, (Color){ 58, 44, 30, 255 }); // East

                        // Front South wall with open doorway at X = 6.0

                        DrawCube({ cabP.x - 2.4f, cabP.y + 2.0f, cabP.z - 3.1f }, 3.2f, 3.5f, 0.22f, (Color){ 58, 44, 30, 255 });

                        DrawCube({ cabP.x + 2.4f, cabP.y + 2.0f, cabP.z - 3.1f }, 3.2f, 3.5f, 0.22f, (Color){ 58, 44, 30, 255 });

                        DrawCube({ cabP.x, cabP.y + 3.3f, cabP.z - 3.1f }, 1.8f, 0.9f, 0.22f, (Color){ 58, 44, 30, 255 }); // Door header

                        // Covered front porch & steps

                        DrawCube({ cabP.x, cabP.y + 0.10f, cabP.z - 4.1f }, 6.2f, 0.20f, 1.8f, (Color){ 42, 32, 20, 255 });

                        DrawCube({ cabP.x - 2.8f, cabP.y + 1.8f, cabP.z - 4.9f }, 0.16f, 3.4f, 0.16f, (Color){ 55, 40, 26, 255 });

                        DrawCube({ cabP.x + 2.8f, cabP.y + 1.8f, cabP.z - 4.9f }, 0.16f, 3.4f, 0.16f, (Color){ 55, 40, 26, 255 });

                        // Sloping cottage roof

                        DrawCube({ cabP.x, cabP.y + 4.1f, cabP.z }, 8.4f, 0.25f, 7.2f, (Color){ 38, 28, 18, 255 });

                        // Hearth fireplace with stone chimney

                        DrawCube({ cabP.x + 3.4f, cabP.y + 2.4f, cabP.z + 1.5f }, 1.2f, 4.6f, 1.2f, (Color){ 52, 54, 58, 255 });

                        // Solitary glowing candle in front window pane casting creepy warmth

                        DrawSphere({ cabP.x + 2.2f, cabP.y + 1.6f, cabP.z - 3.0f }, 0.05f, (Color){ 255, 190, 80, 255 });

                        DrawCircle3D((Vector3){ cabP.x + 2.2f, cabP.y + 0.2f, cabP.z - 3.2f }, 2.4f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 210, 140, 40, 45 });

                    }



                    // D. THE SUNKEN CHAPEL & BELFRY (West, X = -24.0, Z = 140.0)

                    {

                        Vector3 chP = { -24.0f, -16.0f, 140.0f };

                        // Fieldstone chapel foundation

                        DrawCube({ chP.x, chP.y + 0.20f, chP.z }, 11.5f, 0.40f, 8.2f, (Color){ 45, 48, 52, 255 });

                        // Weathered timber walls (height 5.2m)

                        DrawCube({ chP.x - 5.5f, chP.y + 2.8f, chP.z }, 0.30f, 5.2f, 8.0f, (Color){ 42, 36, 30, 255 }); // West wall

                        DrawCube({ chP.x, chP.y + 2.8f, chP.z - 3.9f }, 11.0f, 5.2f, 0.30f, (Color){ 42, 36, 30, 255 }); // South wall

                        DrawCube({ chP.x, chP.y + 2.8f, chP.z + 3.9f }, 11.0f, 5.2f, 0.30f, (Color){ 42, 36, 30, 255 }); // North wall

                        // East facade with grand double-door entrance

                        DrawCube({ chP.x + 5.5f, chP.y + 2.8f, chP.z - 2.5f }, 0.30f, 5.2f, 3.0f, (Color){ 42, 36, 30, 255 });

                        DrawCube({ chP.x + 5.5f, chP.y + 2.8f, chP.z + 2.5f }, 0.30f, 5.2f, 3.0f, (Color){ 42, 36, 30, 255 });

                        DrawCube({ chP.x + 5.5f, chP.y + 4.4f, chP.z }, 0.30f, 2.0f, 2.2f, (Color){ 42, 36, 30, 255 });

                        // Crooked Belfry Tower above entrance

                        Vector3 belfryP = { chP.x + 4.8f, chP.y + 6.8f, chP.z };

                        DrawCube(belfryP, 2.2f, 3.4f, 2.2f, (Color){ 36, 30, 24, 255 });

                        DrawCubeWires(belfryP, 2.25f, 3.45f, 2.25f, (Color){ 65, 55, 45, 255 });

                        // Bronze church bell hanging in belfry

                        DrawCylinder({ belfryP.x, belfryP.y - 0.2f, belfryP.z }, 0.32f, 0.20f, 0.55f, 8, (Color){ 165, 125, 45, 255 });

                        // Shattered cross lying on stone steps

                        DrawCube({ chP.x + 6.8f, chP.y + 0.15f, chP.z - 0.5f }, 1.4f, 0.08f, 0.12f, (Color){ 85, 65, 42, 255 });

                        DrawCube({ chP.x + 6.4f, chP.y + 0.17f, chP.z - 0.5f }, 0.12f, 0.08f, 0.75f, (Color){ 85, 65, 42, 255 });

                        // Broken church pews inside

                        for (int pw = 0; pw < 4; pw++) {

                            float px = chP.x - 3.2f + (float)pw * 2.0f;

                            DrawCube({ px, chP.y + 0.65f, chP.z - 1.8f }, 0.45f, 0.55f, 2.0f, (Color){ 52, 40, 28, 255 });

                            DrawCube({ px, chP.y + 0.65f, chP.z + 1.8f }, 0.45f, 0.55f, 2.0f, (Color){ 52, 40, 28, 255 });

                        }

                    }



                    // E. THE BLACKSMITH FORGE & WORKSHOP (South, X = 4.0, Z = 120.0)

                    {

                        Vector3 smP = { 4.0f, -16.0f, 120.0f };

                        // Heavy timber corner columns

                        DrawCube({ smP.x - 3.2f, smP.y + 1.8f, smP.z - 2.4f }, 0.22f, 3.6f, 0.22f, (Color){ 55, 42, 28, 255 });

                        DrawCube({ smP.x + 3.2f, smP.y + 1.8f, smP.z - 2.4f }, 0.22f, 3.6f, 0.22f, (Color){ 55, 42, 28, 255 });

                        DrawCube({ smP.x - 3.2f, smP.y + 1.8f, smP.z + 2.4f }, 0.22f, 3.6f, 0.22f, (Color){ 55, 42, 28, 255 });

                        DrawCube({ smP.x + 3.2f, smP.y + 1.8f, smP.z + 2.4f }, 0.22f, 3.6f, 0.22f, (Color){ 55, 42, 28, 255 });

                        // Sloping timber roof

                        DrawCube({ smP.x, smP.y + 3.8f, smP.z }, 7.2f, 0.20f, 5.6f, (Color){ 36, 28, 20, 255 });

                        // Stone forge hearth

                        DrawCube({ smP.x - 1.8f, smP.y + 0.75f, smP.z - 1.2f }, 1.4f, 1.5f, 1.4f, (Color){ 44, 46, 50, 255 });

                        DrawCube({ smP.x - 1.8f, smP.y + 2.6f, smP.z - 1.2f }, 0.7f, 2.4f, 0.7f, (Color){ 36, 38, 42, 255 }); // Chimney

                        // Blacksmith's Anvil on wooden stump

                        DrawCylinder({ smP.x + 0.6f, smP.y + 0.35f, smP.z }, 0.30f, 0.34f, 0.70f, 8, (Color){ 72, 54, 34, 255 }); // Stump

                        DrawCube({ smP.x + 0.6f, smP.y + 0.85f, smP.z }, 0.55f, 0.24f, 0.28f, (Color){ 28, 30, 32, 255 });  // Anvil horn & body

                        // Quenching trough with murky stagnant water

                        DrawCube({ smP.x + 1.8f, smP.y + 0.45f, smP.z - 1.0f }, 0.75f, 0.55f, 1.4f, (Color){ 52, 42, 30, 255 });

                        DrawCircle3D((Vector3){ smP.x + 1.8f, smP.y + 0.68f, smP.z - 1.0f }, 0.55f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 35, 55, 65, 230 });

                    }



                    // F. THE COLLAPSED HOMESTEAD & DETRITUS (East, X = 18.0, Z = 118.0)

                    {

                        Vector3 colP = { 18.0f, -16.0f, 118.0f };

                        // Solitary standing fieldstone chimney

                        DrawCube({ colP.x, colP.y + 2.8f, colP.z }, 1.1f, 5.6f, 1.1f, (Color){ 50, 52, 56, 255 });

                        // Charred tumbled wall logs & collapsed rafters

                        DrawCube({ colP.x - 1.8f, colP.y + 0.4f, colP.z + 1.2f }, 3.8f, 0.24f, 2.4f, (Color){ 30, 24, 18, 255 });

                        DrawCube({ colP.x + 1.2f, colP.y + 0.3f, colP.z - 1.4f }, 2.6f, 0.22f, 3.2f, (Color){ 24, 20, 16, 255 });

                    }



                    // G. ABANDONED WOODEN WAGON & RUSTED SICKLES (Pathside X = 18.0, Z = 146.0)

                    {

                        Vector3 wagP = { 18.0f, -15.4f, 146.0f };

                        // Wagon bed

                        DrawCube(wagP, 3.2f, 0.45f, 1.6f, (Color){ 62, 44, 28, 255 });

                        // Broken spoked wheels tilted in the mud

                        DrawCylinderEx({ wagP.x - 1.2f, -15.8f, wagP.z - 0.9f }, { wagP.x - 1.2f, -15.8f, wagP.z - 0.8f }, 0.55f, 0.55f, 12, (Color){ 48, 34, 20, 255 });

                        DrawCylinderEx({ wagP.x + 1.2f, -15.8f, wagP.z + 0.8f }, { wagP.x + 1.2f, -15.8f, wagP.z + 0.9f }, 0.55f, 0.55f, 12, (Color){ 48, 34, 20, 255 });

                    }



                    // H. CROOKED SPLIT-RAIL CEDAR FENCES ALONG PATH

                    for (int f = 0; f < 7; f++) {

                        float fx = 26.0f - (float)f * 7.5f;

                        DrawCube({ fx, -15.35f, 137.4f }, 0.12f, 1.3f, 0.12f, (Color){ 58, 44, 28, 255 });

                        DrawCube({ fx, -15.35f, 142.6f }, 0.12f, 1.3f, 0.12f, (Color){ 58, 44, 28, 255 });

                        DrawLine3D({ fx, -15.1f, 137.4f }, { fx - 7.0f, -15.2f, 137.4f }, (Color){ 58, 44, 28, 255 });

                        DrawLine3D({ fx, -15.6f, 137.4f }, { fx - 7.0f, -15.7f, 137.4f }, (Color){ 58, 44, 28, 255 });

                        DrawLine3D({ fx, -15.1f, 142.6f }, { fx - 7.0f, -15.2f, 142.6f }, (Color){ 58, 44, 28, 255 });

                        DrawLine3D({ fx, -15.6f, 142.6f }, { fx - 7.0f, -15.7f, 142.6f }, (Color){ 58, 44, 28, 255 });

                    }



                    // I. CREEPY TWISTED DEAD TREES

                    Vector2 treeCoords[6] = {

                        { 14.0f, 132.0f }, { -4.0f, 128.0f }, { -14.0f, 154.0f },

                        { 12.0f, 168.0f }, { -32.0f, 130.0f }, { -34.0f, 152.0f }

                    };

                    for (int t = 0; t < 6; t++) {

                        Vector3 trP = { treeCoords[t].x, -16.0f, treeCoords[t].y };

                        // Gnarled gallow trunk

                        DrawCylinderEx(trP, { trP.x + 0.4f, trP.y + 6.2f, trP.z - 0.3f }, 0.32f, 0.12f, 6, (Color){ 28, 24, 20, 255 });

                        // Claw-like branches

                        DrawLine3D({ trP.x + 0.4f, trP.y + 4.8f, trP.z - 0.3f }, { trP.x + 2.2f, trP.y + 6.5f, trP.z + 1.2f }, (Color){ 28, 24, 20, 255 });

                        DrawLine3D({ trP.x + 0.4f, trP.y + 5.2f, trP.z - 0.3f }, { trP.x - 1.8f, trP.y + 6.8f, trP.z - 1.4f }, (Color){ 28, 24, 20, 255 });

                    }

                }

            }

            // South Exterior Wall (Z = 125.85)

            DrawCube({ 97.0f, 12.6f, 125.85f }, 22.0f, 5.2f, 0.35f, wallExtCol);

            // North Exterior Wall (Z = 154.15)

            DrawCube({ 97.0f, 12.6f, 154.15f }, 22.0f, 5.2f, 0.35f, wallExtCol);

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

                    return (Color){

                        (unsigned char)Clamp((int)(baseCol.r * f), 0, 255),

                        (unsigned char)Clamp((int)(baseCol.g * f), 0, 255),

                        (unsigned char)Clamp((int)(baseCol.b * f), 0, 255),

                        baseCol.a

                    };

                };



                Color frameAlum    = ModDoorCol((Color){ 175, 180, 190, 255 }, 1.15f); // Crisp satin commercial aluminum

                Color frameTrim    = ModDoorCol((Color){ 215, 220, 230, 255 }, 1.25f); // Aluminum highlight edge wires

                Color trackCol     = ModDoorCol((Color){ 145, 150, 158, 255 }, 1.05f); // Recessed stainless steel floor track

                Color glassTint    = (Color){ 175, 218, 248, 95 };                     // Realistic architectural glass tint

                Color glassSheen   = (Color){ 235, 248, 255, 180 };                    // Vibrant specular reflection highlight

                Color safetyFrost  = (Color){ 248, 252, 255, 215 };                    // High-contrast white frosted safety stripe (never invisible!)

                Color rubberGasket = (Color){ 32, 34, 38, 255 };                       // Black EPDM rubber perimeter seal

                Color handleCol    = ModDoorCol((Color){ 235, 240, 245, 255 }, 1.35f); // Polished chrome full-height grab handles



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

                Color ledCol = doorSensorActive ? (Color){ 55, 255, 100, 255 } : (Color){ 245, 45, 35, 255 };

                // Exterior sensor pod

                DrawCube({ 108.21f, 13.24f, 140.0f }, 0.05f, 0.09f, 0.26f, (Color){ 30, 32, 35, 255 });

                DrawSphere({ 108.24f, 13.24f, 140.0f }, 0.026f, ledCol);

                // Interior sensor pod (Facing inside store: clearly visible to player approaching from inside!)

                DrawCube({ 107.79f, 13.24f, 140.0f }, 0.05f, 0.09f, 0.26f, (Color){ 30, 32, 35, 255 });

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

            bool canSeeShopInterior = (camera.position.x <= 110.5f) || 

                (camera.position.x <= 118.0f && camera.position.z >= 134.0f && camera.position.z <= 146.0f);

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

                DrawCircle3D((Vector3){ 104.5f, 10.016f, 133.5f }, 2.4f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 8, 10, 12, 55 });

                DrawCircle3D((Vector3){ 94.5f, 10.016f, 133.5f }, 2.2f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 8, 10, 12, 55 });



                // 3. Cold Arctic Cyan Pool around Reach-in Island Freezer

                Vector3 frzFPos = { 94.5f, 10.022f, 133.5f };

                DrawCircle3D(frzFPos, 4.5f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 70, 185, 255, 34 });

                DrawCircle3D(frzFPos, 2.4f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 120, 220, 255, 52 });



                // 4. Warm Task Spotlight Pool on Floor at Checkout Register & Counter

                DrawCircle3D((Vector3){ 104.5f, 10.022f, 134.0f }, 3.2f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 255, 220, 140, 42 });

                DrawCircle3D((Vector3){ 104.5f, 10.022f, 134.0f }, 1.6f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 255, 235, 170, 60 });

            }



            // B. Spacious Empty Entrance Floor (wet sheen reflecting outside rain draft)

            DrawCube({ 104.5f, 10.026f, 140.0f }, 6.0f, 0.005f, 5.0f, ApplyShopLighting({ 104.5f, 10.026f, 140.0f }, { 28, 35, 46, 160 }));



            // Phase 3: High-Fidelity Textured Timber Walls, Water Leaks, Grunge & Storytelling Posters
            DrawShopAtmosphereWalls(ApplyShopLighting, g_shopLightsOn, timeVal);
            DrawShopAtmosphereDetails(ApplyShopLighting, g_shopLightsOn, timeVal);
            DrawShopInteriorProps(ApplyShopLighting, g_shopLightsOn, timeVal);
            DrawShopHauntedWashroom(renderCam, ApplyShopLighting, g_shopLightsOn, timeVal);



            // Physical Industrial Wall Light Switch (Mounted on East Interior Wall at X = 107.75, Z = 138.2)

            {

                Vector3 swP = { 107.75f, 11.5f, 138.2f };

                // EMT metallic conduit pipe from ceiling down to switch box

                DrawCylinderEx((Vector3){ swP.x, swP.y + 0.08f, swP.z }, (Vector3){ swP.x, 15.18f, swP.z }, 0.012f, 0.012f, 8, (Color){ 140, 145, 150, 255 });

                // Cast-aluminum 2-gang electrical junction box

                DrawCube(swP, 0.06f, 0.16f, 0.12f, (Color){ 85, 90, 95, 255 });

                // Brushed stainless steel faceplate

                DrawCube((Vector3){ swP.x - 0.015f, swP.y, swP.z }, 0.02f, 0.14f, 0.10f, (Color){ 195, 200, 205, 255 });

                // Mechanical toggle switch lever (tilted up for ON, down for OFF)

                float togOffY = g_shopLightsOn ? 0.025f : -0.025f;

                DrawCube((Vector3){ swP.x - 0.035f, swP.y + togOffY, swP.z }, 0.04f, 0.025f, 0.018f, (Color){ 30, 32, 35, 255 });

                // Status Indicator LED

                if (g_shopLightsOn) {

                    DrawSphere((Vector3){ swP.x - 0.026f, swP.y + 0.045f, swP.z }, 0.010f, (Color){ 35, 245, 80, 255 });

                    DrawSphere((Vector3){ swP.x - 0.026f, swP.y + 0.045f, swP.z }, 0.025f, (Color){ 35, 245, 80, 60 });

                } else {

                    float pulse = 0.55f + 0.45f * sinf(timeVal * 3.8f);

                    DrawSphere((Vector3){ swP.x - 0.026f, swP.y + 0.045f, swP.z }, 0.012f, (Color){ 255, 35, 25, (unsigned char)(255 * pulse) });

                    DrawSphere((Vector3){ swP.x - 0.026f, swP.y + 0.045f, swP.z }, 0.040f, (Color){ 255, 45, 30, (unsigned char)(80 * pulse) });

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

                    (Color){ (unsigned char)Clamp(255.0f * shopLightIntensity, 0.0f, 255.0f), (unsigned char)Clamp(230.0f * shopLightIntensity, 0.0f, 255.0f), (unsigned char)Clamp(140.0f * shopLightIntensity, 0.0f, 255.0f), 255 } :

                    (Color){ 30, 32, 36, 255 };

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

                DrawCube((Vector3){ tPos.x, tPos.y - 0.02f, tPos.z }, tLen - 0.06f, 0.04f, 0.26f, (Color){ 180, 185, 190, 255 });



                Color actualTubeCol = g_shopLightsOn ? tTubeCol : (Color){ 28, 30, 34, 255 };

                float halfL = (tLen - 0.24f) * 0.5f;

                DrawCylinderEx((Vector3){ tPos.x - halfL, tPos.y - 0.04f, tPos.z - 0.07f }, (Vector3){ tPos.x + halfL, tPos.y - 0.04f, tPos.z - 0.07f }, 0.022f, 0.022f, 8, actualTubeCol);

                DrawCylinderEx((Vector3){ tPos.x - halfL, tPos.y - 0.04f, tPos.z + 0.07f }, (Vector3){ tPos.x + halfL, tPos.y - 0.04f, tPos.z + 0.07f }, 0.022f, 0.022f, 8, actualTubeCol);



                if (g_shopLightsOn && arcing) {

                    DrawSphere((Vector3){ tPos.x - halfL, tPos.y - 0.04f, tPos.z }, 0.16f, (Color){ 245, 250, 255, 255 });

                    DrawSphere((Vector3){ tPos.x - halfL, tPos.y - 0.04f, tPos.z }, 0.40f, (Color){ 160, 210, 255, 160 });

                }

            };



            // Troffer 1: North Aisle (Aisle 3, Z = 150.5, X = 95.0, Length = 3.4m)

            DrawCeilingTroffer((Vector3){ 95.0f, 15.05f, 150.5f }, 3.4f, (Color){ 215, 235, 255, 255 }, tube1IsArcing);



            // Troffer 2: Center Aisle (Aisle 2, Z = 143.5, X = 91.5, Length = 2.8m)

            DrawCeilingTroffer((Vector3){ 91.5f, 15.05f, 143.5f }, 2.8f, (Color){ 220, 235, 250, 255 }, false);



            // Troffer 3: South Aisle (Aisle 1, Z = 136.5, X = 95.0, Length = 3.4m)

            DrawCeilingTroffer((Vector3){ 95.0f, 15.05f, 136.5f }, 3.4f, (Color){ 225, 235, 245, 255 }, false);



            // Troffer 4: Checkout Counter Task Light (Z = 133.5, X = 104.5, Length = 2.8m)

            DrawCeilingTroffer((Vector3){ 104.5f, 15.05f, 133.5f }, 2.8f, (Color){ 255, 220, 150, 255 }, false);



            // Troffer 5: Entrance Corridor & Shopping Cart Bay (Z = 143.5, X = 104.5, Length = 2.8m)

            DrawCeilingTroffer((Vector3){ 104.5f, 15.05f, 143.5f }, 2.8f, (Color){ 220, 230, 240, 255 }, false);



            // Troffer 6: Rear Storage Corner (Z = 133.5, X = 89.0, Length = 2.4m)

            DrawCeilingTroffer((Vector3){ 89.0f, 15.05f, 133.5f }, 2.4f, (Color){ 200, 225, 245, 255 }, false);



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

                            Color shelfN = ApplyShopLighting(pN, { 210, 206, 198, 255 }, (Vector3){ 0, 1, 0 });

                            Color trimN  = ApplyShopLighting(pN, { 58, 62, 70, 255 }, (Vector3){ 0, 0, 1 });

                            Color dustN  = ApplyShopLighting(pN, { 145, 138, 125, 180 }, (Vector3){ 0, 1, 0 });

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

                            Color shelfS = ApplyShopLighting(pS, { 210, 206, 198, 255 }, (Vector3){ 0, 1, 0 }, 1);

                            Color trimS  = ApplyShopLighting(pS, { 58, 62, 70, 255 }, (Vector3){ 0, 0, -1 }, 1);

                            Color dustS  = ApplyShopLighting(pS, { 145, 138, 125, 180 }, (Vector3){ 0, 1, 0 }, 1);

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

                            Color shelfS = ApplyShopLighting(pS, { 210, 206, 198, 255 }, (Vector3){ 0, 1, 0 });

                            Color trimS  = ApplyShopLighting(pS, { 58, 62, 70, 255 }, (Vector3){ 0, 0, -1 });

                            Color dustS  = ApplyShopLighting(pS, { 145, 138, 125, 180 }, (Vector3){ 0, 1, 0 });

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

                            Color shelfN = ApplyShopLighting(pN, { 210, 206, 198, 255 }, (Vector3){ 0, 1, 0 }, 2);

                            Color trimN  = ApplyShopLighting(pN, { 58, 62, 70, 255 }, (Vector3){ 0, 0, 1 }, 2);

                            Color dustN  = ApplyShopLighting(pN, { 145, 138, 125, 180 }, (Vector3){ 0, 1, 0 }, 2);

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

            DrawHorizontalRefrigerator((Vector3){ 94.5f, 10.015f, 133.5f }, camera);



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



                // Mini CRT Surveillance Monitor with phosphor scanlines (Facing Right +Z)

                Vector3 crtPos = { 104.2f, 11.78f, 133.5f };

                DrawCube(crtPos, 0.38f, 0.34f, 0.36f, ApplyShopLighting(crtPos, { 30, 32, 34, 255 }));

                DrawCube({ 104.2f, 11.78f, 133.69f }, 0.28f, 0.25f, 0.03f, { 22, 190, 65, 240 });

                float scanY = 11.78f - 0.10f + fmodf(timeVal * 0.35f, 0.20f);

                DrawLine3D({ 104.07f, scanY, 133.71f }, { 104.33f, scanY, 133.71f }, { 140, 255, 170, 220 });

                bool crtBlink = (fmodf(timeVal, 0.8f) < 0.4f);

                DrawSphere({ 104.33f, 11.91f, 133.70f }, 0.022f, crtBlink ? (Color){ 255, 20, 20, 255 } : (Color){ 80, 0, 0, 255 });



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

                    DrawCircle3D(g_ghostCart.pos, 1.4f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 30, 220, 200, (unsigned char)(65 * g_ghostCart.alpha) });

                    DrawCircle3D(g_ghostCart.pos, 0.8f, (Vector3){ 1, 0, 0 }, 90.0f, (Color){ 90, 255, 235, (unsigned char)(95 * g_ghostCart.alpha) });



                    // Floating ectoplasm particles drifting up from the cart basket

                    for (int p = 0; p < 3; p++) {

                        float pTime = timeVal * 2.2f + (float)p * 1.35f;

                        float px = g_ghostCart.pos.x + sinf(pTime * 2.8f) * 0.22f;

                        float py = g_ghostCart.pos.y + 0.6f + fmodf(pTime * 0.8f, 0.9f);

                        float pz = g_ghostCart.pos.z + cosf(pTime * 3.3f) * 0.22f;

                        float pAlpha = (1.0f - fmodf(pTime * 0.8f, 0.9f) / 0.9f) * g_ghostCart.alpha;

                        DrawSphere((Vector3){ px, py, pz }, 0.032f, (Color){ 120, 255, 235, (unsigned char)(210 * pAlpha) });

                    }



                    rlPushMatrix();

                        rlTranslatef(g_ghostCart.pos.x, g_ghostCart.pos.y, g_ghostCart.pos.z);

                        rlRotatef(g_ghostCart.yaw, 0, 1, 0);

                        DrawGhostShoppingCartLocal(g_ghostCart.wheelSpin, g_ghostCart.pos, g_ghostCart.alpha, timeVal);

                        if (g_ghostCart.itemsInCart >= 1) {

                            DrawCube((Vector3){ -0.10f, 0.62f, 0.0f }, 0.18f, 0.12f, 0.22f, (Color){ 100, 255, 220, (unsigned char)(200 * g_ghostCart.alpha) });

                        }

                        if (g_ghostCart.itemsInCart >= 2) {

                            DrawCube((Vector3){  0.10f, 0.62f, 0.05f }, 0.12f, 0.22f, 0.12f, (Color){ 80, 220, 255, (unsigned char)(200 * g_ghostCart.alpha) });

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



                    Color eyeballCol = isBloody ? (Color){ 255, 18, 22, 255 } : (Color){ 225, 225, 220, 255 };

                    Color pupilCol   = isBloody ? (Color){ 20, 0, 0, 255 }    : (Color){ 22, 24, 28, 255 };



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

                Vector3 lDir = g_flashlightActive ? fwd : (g_curExtDayFactor > 0.2f ? g_curSunDir : (Vector3){ -0.3f, -1.0f, -0.2f });

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

                        for (int c = 0; c < 10; c++) SpawnDirtClod(tipWorld, (Vector3){ 0, -0.4f, 0 });

                    }

                    // 2. Flinging scooped dirt forward in an arc

                    if (!g_shovelDigThrowDone && t >= SHOVEL_DIG_THROW_T) {

                        g_shovelDigThrowDone = true;

                        Vector3 tossDir = Vector3Normalize((Vector3){ fwd.x * 0.45f, 0.85f, fwd.z * 0.45f });

                        for (int c = 0; c < 12; c++) SpawnDirtClod(Vector3Add(tipWorld, (Vector3){0, 0.15f, 0}), tossDir);

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

                            for (int c = 0; c < 8; c++) SpawnDirtClod(tipWorld, (Vector3){ 0, 0.8f, 0 });

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

            Color boneCol = ApplyExteriorDaylight((Color){ 210, 205, 185, 255 }, 1.0f);

            Color hornCol = ApplyExteriorDaylight((Color){  45,  40,  35, 255 }, 1.0f);

            // Nocturnal glowing crimson void eye sockets during night, dark void during day

            Color voidCol = (sunElev < 0.0f) ? (Color){ 220, 35, 25, 255 } : (Color){ 10, 8, 8, 255 };



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

                Color fogColC = ApplyExteriorDaylight((Color){ 20, 22, 28, 255 }, 0.5f);

                float fogColArr[4] = { fogColC.r / 255.0f, fogColC.g / 255.0f, fogColC.b / 255.0f, 1.0f };

                float fogD = 0.007f;

                SetShaderValue(g_houndShader, g_houndFogColorLoc, fogColArr, SHADER_UNIFORM_VEC4);

                SetShaderValue(g_houndShader, g_houndFogDensityLoc, &fogD, SHADER_UNIFORM_FLOAT);



                // Light 0: Celestial Sun / Moon Directional Light

                Vector3 l0Pos = Vector3Add(g_houndNPC.pos, Vector3Scale(sunDir, 40.0f));

                Color l0Col = (nightFactor > 0.4f) ? (Color){ 110, 130, 185, 255 } : (Color){ 255, 245, 220, 255 };

                HoundUpdateLight(0, 1, 0, l0Pos, g_houndNPC.pos, l0Col);



                // Light 1: Gas Station Overhead Canopy Fluorescent Light

                Vector3 l1Pos = { 128.0f, 14.2f, 140.0f };

                Color l1Col = fluorLightOn ? (Color){ 200, 235, 210, 255 } : (Color){ 0, 0, 0, 0 };

                HoundUpdateLight(1, fluorLightOn ? 1 : 0, 1, l1Pos, g_houndNPC.pos, l1Col);



                // Light 2 & 3: Disabled by default

                HoundUpdateLight(2, 0, 0, (Vector3){0,0,0}, (Vector3){0,0,0}, (Color){0,0,0,0});

                HoundUpdateLight(3, 0, 0, (Vector3){0,0,0}, (Vector3){0,0,0}, (Color){0,0,0,0});



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

                    HoundSkeletonUpdateLight(2, 0, 0, (Vector3){0,0,0}, (Vector3){0,0,0}, (Color){0,0,0,0});

                    HoundSkeletonUpdateLight(3, 0, 0, (Vector3){0,0,0}, (Vector3){0,0,0}, (Color){0,0,0,0});



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

            Vector3 pFwdH = Vector3Normalize((Vector3){ forwardBob.x, 0.0f, forwardBob.z });

            if (Vector3Length(pFwdH) < 0.1f) pFwdH = (Vector3){ 0.0f, 0.0f, 1.0f };



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
            DrawCube(colFigure, 0.44f, 2.35f, 0.50f, (Color){ 2, 3, 5, 250 });
            DrawSphere((Vector3){ colFigure.x, 12.85f, colFigure.z }, 0.20f, (Color){ 2, 3, 5, 250 });

            // Glowing tapetum-lucidum predator eyes in deep darkness
            float colEyeAlpha = Clamp((1.0f - g_menuAwakeIntensity) * 255.0f, 0.0f, 255.0f);
            if (colEyeAlpha > 8.0f) {
                DrawSphere((Vector3){ colFigure.x - 0.14f, 12.88f, colFigure.z - 0.06f }, 0.018f, (Color){ 245, 190, 45, (unsigned char)colEyeAlpha });
                DrawSphere((Vector3){ colFigure.x - 0.14f, 12.88f, colFigure.z + 0.06f }, 0.018f, (Color){ 245, 190, 45, (unsigned char)colEyeAlpha });
            }

            // 2. Swirling ground mist caught in the portico lantern's light
            for (int mi = 0; mi < 18; mi++) {
                float mSeed = (float)mi * 1.618f;
                float mx = 145.0f + sinf(timeVal * 0.28f + mSeed) * 5.0f;
                float my = 10.3f + fmodf(timeVal * 0.14f + mSeed * 1.8f, 3.2f);
                float mz = 140.0f + cosf(timeVal * 0.32f + mSeed * 1.4f) * 4.0f;
                float mistAlpha = sinf((my - 10.3f) / 3.2f * PI) * 110.0f * g_menuAwakeIntensity;
                if (mistAlpha > 0.0f) {
                    DrawSphere((Vector3){ mx, my, mz }, 0.040f, (Color){ 185, 200, 220, (unsigned char)mistAlpha });
                }
            }

            // 3. APPALACHIAN FOLK-HORROR HORNED BOVINE SKULL (REAL-TIME GAZE TRACKING)
            // Mounted in the open right-hand foreground of CAM 01
            Vector3 skullPos = { 143.6f, 13.40f, 137.5f };

            // Weathered rustic timber fence post
            Color postWood   = { 52, 45, 38, 255 };
            Color postRing   = { 30, 26, 22, 255 };
            Color rustWire   = { 95, 55, 35, 240 };
            DrawCylinder((Vector3){ skullPos.x, 11.5f, skullPos.z }, 0.10f, 0.11f, 1.70f, 8, postWood);
            DrawCubeWires((Vector3){ skullPos.x, 13.18f, skullPos.z }, 0.22f, 0.14f, 0.22f, postRing);
            // Wrapped rusted bailing wire
            DrawCircle3D((Vector3){ skullPos.x, 13.12f, skullPos.z }, 0.115f, (Vector3){ 1, 0, 0 }, 90.0f, rustWire);
            DrawCircle3D((Vector3){ skullPos.x, 13.15f, skullPos.z }, 0.115f, (Vector3){ 1, 0, 0 }, 90.0f, rustWire);

            // Ritual tallow candle / sconce lighting the skull from below
            Vector3 sconcePos = { skullPos.x - 0.14f, 13.02f, skullPos.z - 0.12f };
            DrawCube(sconcePos, 0.05f, 0.025f, 0.08f, postRing);
            DrawCylinder((Vector3){ sconcePos.x, 13.03f, sconcePos.z }, 0.022f, 0.020f, 0.08f, 6, (Color){ 215, 205, 185, 255 }); // Tallow candle stub
            float candleFlicker = sinf(timeVal * 11.0f) * 0.06f + cosf(timeVal * 17.0f) * 0.04f;
            Vector3 candleFlame = { sconcePos.x, 13.12f, sconcePos.z };
            DrawSphere(candleFlame, 0.032f + candleFlicker * 0.008f, (Color){ 255, 225, 95, 255 });
            DrawSphere(candleFlame, 0.16f + candleFlicker * 0.025f, (Color){ 255, 150, 40, (unsigned char)(65 + (int)(candleFlicker * 20.0f)) });

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
            DrawSphere((Vector3){ 0.0f, 0.08f, -0.05f }, 0.22f, boneIvory);
            DrawCube((Vector3){ 0.0f, 0.12f, 0.03f }, 0.36f, 0.055f, 0.10f, boneIvory); // Supraorbital brow ridge

            // B. Snout & Nasal Bridge (Tapering forward towards viewer)
            DrawCylinderEx((Vector3){ 0.0f, 0.07f, -0.02f }, (Vector3){ 0.0f, -0.08f, 0.36f }, 0.13f, 0.08f, 8, boneIvory);
            DrawCube((Vector3){ 0.0f, -0.09f, 0.36f }, 0.13f, 0.07f, 0.12f, boneIvory); // Maxilla tip
            DrawCube((Vector3){ 0.0f, -0.07f, 0.34f }, 0.045f, 0.055f, 0.10f, socketHole); // Nasal cavity aperture
            DrawCube((Vector3){ 0.0f, -0.13f, 0.30f }, 0.11f, 0.03f, 0.18f, boneDark); // Upper jaw teeth ridge

            // C. Deep Hollow Eye Sockets
            Vector3 leftEyeSocket  = { -0.13f, 0.06f, 0.08f };
            Vector3 rightEyeSocket = {  0.13f, 0.06f, 0.08f };
            DrawSphere(leftEyeSocket, 0.058f, socketHole);
            DrawSphere(rightEyeSocket, 0.058f, socketHole);
            DrawCircle3D(leftEyeSocket, 0.064f, (Vector3){ 0, 0, 1 }, 0.0f, boneDark);
            DrawCircle3D(rightEyeSocket, 0.064f, (Vector3){ 0, 0, 1 }, 0.0f, boneDark);

            // D. Majestic Sweeping Curved Horns (1.4m span, arching out, up, curling forward)
            // Left Horn
            DrawCylinderEx((Vector3){ -0.15f, 0.13f, -0.07f }, (Vector3){ -0.34f, 0.20f, -0.08f }, 0.065f, 0.052f, 8, hornDark);
            DrawCylinderEx((Vector3){ -0.34f, 0.20f, -0.08f }, (Vector3){ -0.52f, 0.32f, -0.04f }, 0.052f, 0.038f, 8, hornMid);
            DrawCylinderEx((Vector3){ -0.52f, 0.32f, -0.04f }, (Vector3){ -0.56f, 0.48f,  0.06f }, 0.038f, 0.024f, 8, hornPale);
            DrawCylinderEx((Vector3){ -0.56f, 0.48f,  0.06f }, (Vector3){ -0.48f, 0.58f,  0.14f }, 0.024f, 0.006f, 7, boneIvory);

            // Right Horn
            DrawCylinderEx((Vector3){  0.15f, 0.13f, -0.07f }, (Vector3){  0.34f, 0.20f, -0.08f }, 0.065f, 0.052f, 8, hornDark);
            DrawCylinderEx((Vector3){  0.34f, 0.20f, -0.08f }, (Vector3){  0.52f, 0.32f, -0.04f }, 0.052f, 0.038f, 8, hornMid);
            DrawCylinderEx((Vector3){  0.52f, 0.32f, -0.04f }, (Vector3){  0.56f, 0.48f,  0.06f }, 0.038f, 0.024f, 8, hornPale);
            DrawCylinderEx((Vector3){  0.56f, 0.48f,  0.06f }, (Vector3){  0.48f, 0.58f,  0.14f }, 0.024f, 0.006f, 7, boneIvory);

            // Occult carved rune on the forehead
            DrawLine3D((Vector3){  0.0f,  0.04f, 0.11f }, (Vector3){ 0.0f, 0.16f, 0.03f }, (Color){ 85, 25, 20, 230 });
            DrawLine3D((Vector3){ -0.04f, 0.11f, 0.07f }, (Vector3){ 0.04f, 0.11f, 0.07f }, (Color){ 85, 25, 20, 230 });

            // E. THE GAZE-TRACKING GLOWING EYES
            float pupilOffX = -g_skullEyeSmoothX * 0.028f;
            float pupilOffY = -g_skullEyeSmoothY * 0.022f;
            float pupilOffZ = 0.032f;

            Vector3 leftPupil  = { leftEyeSocket.x  + pupilOffX, leftEyeSocket.y  + pupilOffY, leftEyeSocket.z  + pupilOffZ };
            Vector3 rightPupil = { rightEyeSocket.x + pupilOffX, rightEyeSocket.y + pupilOffY, rightEyeSocket.z + pupilOffZ };

            float eyePulse = 0.90f + 0.10f * sinf(timeVal * 3.0f);
            float eyeR     = (0.020f + g_skullGazeFlare * 0.007f) * eyePulse;

            Color eyeCoreCol  = { 255, 250, 210, 255 };
            Color eyeAmberCol = g_skullGazeFlare > 0.3f ? (Color){ 255, 120, 35, 255 } : (Color){ 255, 195, 45, 255 };
            Color eyeHaloCol  = { 255, 160, 35, (unsigned char)(55 + (int)(g_skullGazeFlare * 75.0f)) };

            // Glowing tapetum-lucidum predatory pupils
            DrawSphere(leftPupil, eyeR, eyeAmberCol);
            DrawSphere(rightPupil, eyeR, eyeAmberCol);
            // Pinpoint white-hot core retinas
            DrawSphere((Vector3){ leftPupil.x, leftPupil.y, leftPupil.z + 0.006f }, eyeR * 0.45f, eyeCoreCol);
            DrawSphere((Vector3){ rightPupil.x, rightPupil.y, rightPupil.z + 0.006f }, eyeR * 0.45f, eyeCoreCol);
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

        uiCamera.offset = (Vector2){

            ((float)target.texture.width - (float)LOGICAL_W * uiScale) * 0.5f,

            ((float)target.texture.height - (float)LOGICAL_H * uiScale) * 0.5f

        };

        BeginMode2D(uiCamera);

        // Underwater Post-Processing, Hypoxia Vignette, Oxygen Gauge, Waterline Meniscus & Screen Water Droplets
        DrawUnderwaterPostFXAndHUD(renderCam, timeVal, dt, LOGICAL_W, LOGICAL_H);

        // Flashlight Volumetric Center Halo & Atmospheric Beam in 2D View

        if (g_flashlightActive && g_gameState == STATE_GAMEPLAY && !isRoofCamActive) {

            DrawCircleGradient(LOGICAL_W / 2, LOGICAL_H / 2 + 25, 340.0f, (Color){ 255, 245, 210, 22 }, (Color){ 0, 0, 0, 0 });

            DrawCircleGradient(LOGICAL_W / 2, LOGICAL_H / 2 + 25, 170.0f, (Color){ 255, 250, 230, 18 }, (Color){ 0, 0, 0, 0 });

        }



        // (Player hand items removed from first-person perspective)



        // ---------------------------------------------------------------------

        // SUPERSTORE PRODUCT HUD INSPECTION & HOLD PROMPTS (BOTTOM-CENTER)
        // ---------------------------------------------------------------------
        if (!isShopOpen && !isRoofCamActive && !showQuitConfirm) {
            if (g_heldProductIndex != -1) {
                const ShopProduct &hp = g_shopProducts[g_heldProductIndex];
                char hintBuf[160];
                if (hp.type == PROD_TIN) {
                    snprintf(hintBuf, sizeof(hintBuf), "Holding: %s   [E] %s   [Q] Drop",
                             hp.label, hp.opened ? "Popping Kernels..." : "Open Lid");
                } else if (hp.type == PROD_MILK || hp.type == PROD_BLOOD) {
                    int fillPct = (int)(hp.fill * 100.0f);
                    snprintf(hintBuf, sizeof(hintBuf), "Holding: %s (%d%%)   [E] %s   [Q] Drop",
                             hp.label, fillPct, hp.opened ? "Stop Pouring" : "Pour");
                } else if (hp.type == PROD_CHOCOLATE) {
                    snprintf(hintBuf, sizeof(hintBuf), "Holding: %s   [E] %s   [Q] Drop",
                             hp.label, hp.opened ? "Wrap Foil" : "Unwrap Foil");
                } else if (hp.type == PROD_GUN) {
                    snprintf(hintBuf, sizeof(hintBuf), "Holding: %s   [Left Click] Fire Weapon   [Q] Drop", hp.label);
                } else {
                    snprintf(hintBuf, sizeof(hintBuf), "Holding: %s   [Q] Drop", hp.label);
                }
                float tw = MeasureTextSharp(g_fontMenu, hintBuf, 15.0f);
                float pw = tw + 32.0f;
                float px = ((float)LOGICAL_W - pw) * 0.5f;
                float py = (float)LOGICAL_H - 48.0f;
                DrawAAAPanel((Rectangle){ px, py, pw, 30.0f }, (Color){ 16, 18, 22, 235 }, (Color){ 225, 185, 60, 230 }, 5.0f, true);
                DrawTextSharpCentered(g_fontMenu, hintBuf, (float)LOGICAL_W * 0.5f, py + 7.0f, 15.0f, (Color){ 255, 235, 150, 255 });
            } else if (hudFocusIdx != -1 && !nearCounter) {
                const ShopProduct &fp = g_shopProducts[hudFocusIdx];
                char hintBuf[160];
                if (fp.type == PROD_CART) {
                    snprintf(hintBuf, sizeof(hintBuf), "[ %s  |  Walk into to Push ]", fp.label);
                } else {
                    snprintf(hintBuf, sizeof(hintBuf), "[E] Pick up %s  |  %s", fp.label, fp.price);
                }
                float tw = MeasureTextSharp(g_fontMenu, hintBuf, 15.0f);
                float pw = tw + 32.0f;
                float px = ((float)LOGICAL_W - pw) * 0.5f;
                float py = (float)LOGICAL_H - 48.0f;
                DrawAAAPanel((Rectangle){ px, py, pw, 30.0f }, (Color){ 14, 18, 20, 235 }, (Color){ 75, 195, 95, 230 }, 5.0f, true);
                DrawTextSharpCentered(g_fontMenu, hintBuf, (float)LOGICAL_W * 0.5f, py + 7.0f, 15.0f, (Color){ 220, 255, 220, 255 });
            } else if (g_hasShovel && !g_isHoldingCart && !nearCounter && hudFocusIdx == -1 && !g_phoneActive && g_shovelEquipped) {
                const char *hintBuf = "Equipped: Heavy Trench Shovel   [LMB] Swing   [RMB] Dig   [1/X] Holster";
                float tw = MeasureTextSharp(g_fontMenu, hintBuf, 14.0f);
                float pw = tw + 32.0f;
                float px = ((float)LOGICAL_W - pw) * 0.5f;
                float py = (float)LOGICAL_H - 48.0f;
                DrawAAAPanel((Rectangle){ px, py, pw, 28.0f }, (Color){ 16, 18, 22, 230 }, (Color){ 175, 145, 85, 220 }, 5.0f, true);
                DrawTextSharpCentered(g_fontMenu, hintBuf, (float)LOGICAL_W * 0.5f, py + 6.5f, 14.0f, (Color){ 245, 230, 185, 245 });
            }
        }

        // ---------------------------------------------------------------------
        // AAA TOP-LEFT PLAYER HUD & TELEMETRY (CASH CARD, LOCATION, POPUPS)
        // ---------------------------------------------------------------------
        if (g_gameState == STATE_GAMEPLAY && !isRoofCamActive && !showQuitConfirm) {
            // 1. Permanent Retro Green / Brass Cash Card
            DrawAAAPanel((Rectangle){ 24.0f, 20.0f, 200.0f, 38.0f }, (Color){ 10, 16, 12, 240 }, (Color){ 45, 185, 80, 240 }, 6.0f, true);
            DrawAAAKeycap("$", 32.0f, 28.0f, (Color){ 65, 240, 115, 255 });
            DrawTextSharp(g_fontMenu, TextFormat("CASH: $%.2f", g_playerCash), 64.0f, 29.0f, 18.0f, (Color){ 110, 255, 145, 255 });

            // 2. Abandoned College Location Tag (STRICTLY BELOW CASH CARD)
            bool inColBldg = (camera.position.x >= 152.0f && camera.position.x <= 188.0f &&
                              camera.position.z >= 124.0f && camera.position.z <= 160.0f &&
                              camera.position.y >= 9.8f && camera.position.y <= 16.2f);
            if (inColBldg) {
                const char* locTag = "LOCATION: BLACKWOOD COLLEGE // CONDEMNED ANNEX";
                float lw = MeasureTextSharp(g_fontSmall, locTag, 11.0f);
                DrawAAAPanel((Rectangle){ 24.0f, 64.0f, lw + 24.0f, 26.0f }, (Color){ 20, 14, 14, 230 }, (Color){ 200, 65, 55, 220 }, 4.0f, true);
                DrawTextSharp(g_fontSmall, locTag, 36.0f, 70.5f, 11.0f, (Color){ 245, 195, 190, 245 });
            }

            // 3. Floating Cash Gain Popup
            if (g_cashPopupTimer > 0.0f) {
                float a = Clamp(g_cashPopupTimer / 1.0f, 0.0f, 1.0f);
                float popupY = (inColBldg ? 96.0f : 64.0f) + (4.8f - g_cashPopupTimer) * 5.0f;
                float pw = MeasureTextSharp(g_fontMenu, g_cashPopupText, 16.0f);
                DrawAAAPanel((Rectangle){ 24.0f, popupY, pw + 28.0f, 30.0f }, (Color){ 22, 24, 16, (unsigned char)(240 * a) }, (Color){ 235, 190, 50, (unsigned char)(255 * a) }, 4.0f, true);
                DrawTextSharp(g_fontMenu, g_cashPopupText, 38.0f, popupY + 6.5f, 16.0f, (Color){ 255, 235, 110, (unsigned char)(255 * a) });
            }

            // 4. Driveway Bell Chime Notification Banner (Top-Center)
            if (g_stationBellBannerTimer > 0.0f) {
                float a = Clamp(g_stationBellBannerTimer / 1.0f, 0.0f, 1.0f);
                float bw = MeasureTextSharp(g_fontMenu, g_stationBellBanner, 16.0f);
                float bx = ((float)LOGICAL_W - bw) * 0.5f;
                DrawAAAPanel((Rectangle){ bx - 18.0f, 22.0f, bw + 36.0f, 34.0f }, (Color){ 14, 18, 24, (unsigned char)(240 * a) }, (Color){ 235, 185, 55, (unsigned char)(255 * a) }, 6.0f, true);
                DrawTextSharpCentered(g_fontMenu, g_stationBellBanner, (float)LOGICAL_W * 0.5f, 29.5f, 16.0f, (Color){ 255, 235, 130, (unsigned char)(255 * a) });
            }
        }

        // ---------------------------------------------------------------------
        // AAA DYNAMIC BOTTOM NAVIGATION DOCK (AUTO-FLOWING CONTROLS RIBBON)
        // ---------------------------------------------------------------------
        if (g_gameState == STATE_GAMEPLAY && !isShopOpen && !isRoofCamActive && !showQuitConfirm) {
            float dockX = 24.0f;
            float dockY = (float)LOGICAL_H - 34.0f;
            auto DrawDockItem = [&](const char* key, const char* label, Color accentCol, bool activeState) {
                float kw = DrawAAAKeycap(key, dockX, dockY, accentCol);
                dockX += kw + 7.0f;
                float lw = MeasureTextSharp(g_fontSmall, label, 12.0f);
                DrawTextSharp(g_fontSmall, label, dockX, dockY + 4.5f, 12.0f, activeState ? (Color){ 245, 248, 252, 255 } : (Color){ 160, 170, 180, 200 });
                dockX += lw + 18.0f;
            };

            if (g_hasReceipt && !g_inspectingReceipt) {
                DrawDockItem("TAB", "RECEIPT", (Color){ 255, 175, 175, 255 }, true);
            }
            DrawDockItem("F", "FLASHLIGHT", g_flashlightActive ? (Color){ 255, 235, 110, 255 } : (Color){ 140, 145, 155, 200 }, g_flashlightActive);
            DrawDockItem("M", "PHONE GPS", g_phoneActive ? (Color){ 80, 220, 255, 255 } : (Color){ 150, 195, 220, 200 }, g_phoneActive);
            DrawDockItem("C", "CCTV ROOF", (Color){ 110, 220, 140, 220 }, false);
            if (g_hasShovel && g_shovelEquipped) {
                DrawDockItem("1 / X", "HOLSTER", (Color){ 220, 180, 90, 230 }, false);
            }
        }

        // ---------------------------------------------------------------------
        // AAA CENTRALIZED INTERACTION MANAGER (PRIORITY QUEUE - ZERO COLLISIONS)
        // ---------------------------------------------------------------------
        if (g_gameState == STATE_GAMEPLAY && !isShopOpen && !isRoofCamActive && !showQuitConfirm) {
            struct QueuedPrompt {
                bool active;
                int priority;
                float dist;
                char text[192];
                Color accentCol;
            };
            QueuedPrompt activePrompt = { false, -999, 9999.0f, "", WHITE };
            auto RegisterPrompt = [&](int priority, float dist, const char* text, Color col) {
                if (!activePrompt.active || priority > activePrompt.priority || (priority == activePrompt.priority && dist < activePrompt.dist)) {
                    activePrompt.active = true;
                    activePrompt.priority = priority;
                    activePrompt.dist = dist;
                    snprintf(activePrompt.text, sizeof(activePrompt.text), "%s", text);
                    activePrompt.accentCol = col;
                }
            };

            Vector3 cPos = { 104.5f, 11.5f, 134.5f };
            float distCounter = Vector3Distance(camera.position, cPos);
            float distCart = Vector2Distance((Vector2){ camera.position.x, camera.position.z }, (Vector2){ g_cartPos.x, g_cartPos.z });
            Vector3 printerPosP = { 106.3f, 11.56f, 133.5f };
            float distPrinter = Vector3Distance(camera.position, printerPosP);

            if (g_isHoldingCart) {
                const char* cartPrompt = (distCounter < 2.8f && g_cartProductIndices.size() > 0)
                    ? "[E] CHECKOUT TROLLEY   |   [F] PLACE ITEM IN TROLLEY"
                    : "[E] RELEASE TROLLEY   |   [F] PLACE ITEM IN TROLLEY";
                RegisterPrompt(60, 0.0f, cartPrompt, (Color){ 255, 215, 60, 255 });
            } else if (distPrinter < 2.0f && g_printerState == PRINTER_DONE) {
                RegisterPrompt(70, distPrinter, "[E] TAKE RECEIPT", (Color){ 255, 90, 90, 255 });
            } else if (distCounter < 2.8f && g_printerState == PRINTER_PRINTING) {
                RegisterPrompt(50, distCounter, "* PRINTER CHURNING RECEIPT... *", (Color){ 255, 140, 60, 255 });
            } else if (distCounter < 2.8f && g_cartProductIndices.size() > 0 && g_printerState == PRINTER_IDLE) {
                RegisterPrompt(60, distCounter, "[E] CHECKOUT TROLLEY", (Color){ 255, 215, 60, 255 });
            } else if (g_receiptThrown && Vector3Distance(camera.position, g_thrownReceiptPos) < 1.8f) {
                RegisterPrompt(60, Vector3Distance(camera.position, g_thrownReceiptPos), "[E] PICK UP RECEIPT", (Color){ 255, 215, 60, 255 });
            } else if (distCart < 1.85f && g_heldProductIndex == -1) {
                RegisterPrompt(40, distCart, "[E] GRAB TROLLEY", (Color){ 255, 215, 60, 255 });
            } else if (g_heldProductIndex != -1 && distCart < 2.2f) {
                RegisterPrompt(45, distCart, "[F] PLACE ITEM IN TROLLEY", (Color){ 140, 240, 140, 255 });
            }

            // Light Switch Prompt
            Vector3 swHUDPos = { 107.75f, 11.5f, 138.2f };
            float distToSwHUD = Vector3Distance(camera.position, swHUDPos);
            if (distToSwHUD < 2.2f) {
                const char* swPrompt = g_shopLightsOn ? "[E] TURN OFF STORE LIGHTS" : "[E] TURN ON STORE LIGHTS";
                RegisterPrompt(30, distToSwHUD, swPrompt, g_shopLightsOn ? (Color){ 255, 215, 60, 255 } : (Color){ 100, 255, 140, 255 });
            }

            // Carpet & Secret Tunnel Hatch
            Vector3 carpetHUDPos = { 83.8f, 10.0f, 140.0f };
            float distToCarpetHUD = Vector3Distance(camera.position, carpetHUDPos);
            if (distToCarpetHUD < 3.2f && camera.position.y > 9.0f) {
                if (!g_carpetMoved) {
                    RegisterPrompt(40, distToCarpetHUD, "[E] PULL BACK WORN CARPET", (Color){ 215, 60, 50, 255 });
                } else if (!g_tunnelHatchOpen) {
                    RegisterPrompt(40, distToCarpetHUD, "[E] UNLOCK & OPEN HEAVY STEEL HATCH", (Color){ 240, 180, 50, 255 });
                }
            }

            // Interactive Gas Station Fuel Pumps
            float dToPump1 = Vector3Distance(camera.position, (Vector3){ 127.4f, 11.2f, 137.5f });
            float dToPump2 = Vector3Distance(camera.position, (Vector3){ 127.4f, 11.2f, 142.5f });
            int nearPumpIdx = (dToPump1 < 2.5f) ? 0 : ((dToPump2 < 2.5f) ? 1 : -1);

            if (nearPumpIdx != -1) {
                float pumpDist = (nearPumpIdx == 0) ? dToPump1 : dToPump2;
                if (g_shovelEquipped || g_isHoldingCart || g_heldProductIndex != -1) {
                    RegisterPrompt(45, pumpDist, TextFormat("[HANDS FULL] PUT DOWN ITEM TO UNHOLSTER PUMP 0%d", nearPumpIdx + 1), (Color){ 255, 180, 100, 255 });
                } else if (!g_holdingFuelNozzle) {
                    RegisterPrompt(50, pumpDist, TextFormat("[E] UNHOLSTER PUMP 0%d NOZZLE ($%.2f/GAL) [TANK: %.1f GAL]", nearPumpIdx + 1, g_fuelPricePerGallon, g_stationFuelGallons), (Color){ 140, 255, 175, 255 });
                    if (IsKeyPressed(KEY_E)) {
                        g_holdingFuelNozzle = true;
                        g_activePumpIndex = nearPumpIdx;
                        PlaySound(g_sndNozzleLatch);
                    }
                } else if (g_activePumpIndex == nearPumpIdx) {
                    RegisterPrompt(50, pumpDist, "[E] DOCK FUEL NOZZLE IN CRADLE", (Color){ 255, 205, 140, 255 });
                    if (IsKeyPressed(KEY_E)) {
                        g_holdingFuelNozzle = false;
                        g_activePumpIndex = -1;
                        g_nozzleInCar = false;
                        PlaySound(g_sndNozzleLatch);
                    }
                }
            }

            // Abandoned Blackwood College Examination Prompts
            {
                float dToCollegeSign = Vector3Distance(camera.position, (Vector3){ 149.0f, 11.5f, 140.0f });
                if (dToCollegeSign < 4.2f) {
                    RegisterPrompt(10, dToCollegeSign, "[ARCH SIGN]: BLACKWOOD VALLEY COLLEGE - EST. 1948 [CONDEMNED]", (Color){ 255, 195, 130, 255 });
                }
                float dToWhiteboard = Vector3Distance(camera.position, (Vector3){ 163.0f, 12.0f, 126.2f });
                if (dToWhiteboard < 3.8f) {
                    RegisterPrompt(10, dToWhiteboard, "[WHITEBOARD]: \"CLASS OF '84 NEVER LEFT... IT DIGS BENEATH...\"", (Color){ 255, 120, 130, 255 });
                }
                float dToDissect = Vector3Distance(camera.position, (Vector3){ 163.0f, 11.2f, 151.0f });
                if (dToDissect < 3.2f) {
                    RegisterPrompt(10, dToDissect, "[SURGICAL TRAY]: Fresh arterial coagulant... Someone was here.", (Color){ 255, 160, 170, 255 });
                }
                float dToSkel = Vector3Distance(camera.position, (Vector3){ 163.0f, 11.2f, 153.5f });
                if (dToSkel < 2.6f) {
                    RegisterPrompt(10, dToSkel, "[ANATOMICAL SKELETON]: Ribcage wired shut around animal entrails.", (Color){ 255, 130, 140, 255 });
                }
                float dToArchive = Vector3Distance(camera.position, (Vector3){ 183.5f, 11.2f, 138.2f });
                if (dToArchive < 3.4f) {
                    RegisterPrompt(10, dToArchive, "[FILE ARCHIVE]: Class rosters burned. One name remains: GRETHNAR.", (Color){ 255, 200, 120, 255 });
                }
            }

            // Customer Vehicle Fueling & Interactions
            if (g_customerCar.state != CAR_INACTIVE) {
                Vector3 carFuelFlap = { g_customerCar.pos.x + 0.98f, 10.85f, g_customerCar.pos.z - 0.85f };
                float dToFlap = Vector3Distance(camera.position, carFuelFlap);
                if (dToFlap < 2.6f) {
                    if (g_customerCar.state == CAR_PARKED) {
                        if (g_holdingFuelNozzle) {
                            RegisterPrompt(80, dToFlap, TextFormat("[E] INSERT NOZZLE (ORDER: %.1f GAL / $%.2f)", g_customerCar.requestedGallons, g_customerCar.totalSale), (Color){ 170, 230, 255, 255 });
                            if (IsKeyPressed(KEY_E)) {
                                g_nozzleInCar = true;
                                g_customerCar.state = CAR_REFUELING;
                                PlaySound(g_sndNozzleLatch);
                            }
                        } else {
                            RegisterPrompt(45, dToFlap, "[UNHOLSTER FUEL NOZZLE FROM PUMP TO BEGIN SERVICE]", (Color){ 255, 215, 140, 255 });
                        }
                    } else if (g_customerCar.state == CAR_REFUELING && g_nozzleInCar) {
                        float pct = Clamp(g_customerCar.dispensedGallons / g_customerCar.requestedGallons, 0.0f, 1.0f);
                        RegisterPrompt(100, dToFlap, TextFormat("[HOLD E / LMB] DISPENSE: %.1f / %.1f GAL ($%.2f) [%d%%]  |  [E/Q] REMOVE NOZZLE",
                            g_customerCar.dispensedGallons, g_customerCar.requestedGallons,
                            g_customerCar.dispensedGallons * g_fuelPricePerGallon, (int)(pct * 100.0f)), (Color){ 130, 255, 160, 255 });

                        if (IsKeyPressed(KEY_Q) || IsKeyPressed(KEY_E)) {
                            g_nozzleInCar = false;
                            PlaySound(g_sndNozzleLatch);
                        } else if ((IsKeyDown(KEY_E) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) && g_stationFuelGallons > 0.0f) {
                            float fFlow = dt * 3.4f;
                            g_stationFuelGallons -= fFlow;
                            g_customerCar.dispensedGallons += fFlow;
                            g_camLandingDip = -0.012f * sinf(timeVal * 55.0f);
                            if (g_fuelSoundCooldown <= 0.0f) {
                                PlaySound(g_sndPumpFlow);
                                g_fuelSoundCooldown = 0.28f;
                            }
                            if (g_customerCar.dispensedGallons >= g_customerCar.requestedGallons) {
                                g_customerCar.dispensedGallons = g_customerCar.requestedGallons;
                                g_customerCar.state = CAR_PAID;
                                float sale = g_customerCar.totalSale + g_customerCar.tipAmount;
                                g_playerCash += sale;
                                g_cashPopupAmount = sale;
                                g_cashPopupTimer = 4.8f;
                                snprintf(g_cashPopupText, sizeof(g_cashPopupText), "+$%.2f (FUEL SALE + $%.2f TIP!)", sale, g_customerCar.tipAmount);
                                PlaySound(g_sndNozzleShutoff);
                                PlaySound(g_sndCashRegister);
                                g_camLandingDip = -0.06f;
                                g_customerCar.waitTimer = 5.0f;
                            }
                        }
                    } else if (g_customerCar.state == CAR_PAID) {
                        if (g_nozzleInCar) {
                            RegisterPrompt(80, dToFlap, "[E / Q] REMOVE FUEL NOZZLE FROM VEHICLE", (Color){ 180, 230, 255, 255 });
                            if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_Q)) {
                                g_nozzleInCar = false;
                                PlaySound(g_sndNozzleLatch);
                            }
                        } else {
                            RegisterPrompt(70, dToFlap, g_customerCar.driverDialogue, (Color){ 245, 235, 190, 255 });
                        }
                    }
                }
            }

            // Service Bell Ring Prompt
            float dToBell = Vector3Distance(camera.position, (Vector3){ 127.7f, 11.45f, 140.0f });
            if (dToBell < 2.2f) {
                const char* bPrompt = (g_customerCar.state == CAR_INACTIVE) ? "[E] RING ATTENDANT BELL (CALL TRAVELER)" : "TRAVELER ALREADY AT PUMPS";
                RegisterPrompt(25, dToBell, bPrompt, (Color){ 255, 225, 130, 255 });
                if (IsKeyPressed(KEY_E) && g_customerCar.state == CAR_INACTIVE) {
                    PlaySound(g_sndDrivewayBell);
                    SpawnCustomerCar();
                }
            }

            // Pier Fog Bell Interaction
            float dToFogBell = Vector3Distance(camera.position, (Vector3){ 16.5f, 11.5f, 138.0f });
            if (dToFogBell < 2.8f) {
                RegisterPrompt(25, dToFogBell, "[E] RING MARITIME FOG BELL", (Color){ 160, 245, 215, 255 });
                if (IsKeyPressed(KEY_E)) {
                    PlaySound(g_sndFogBell);
                    snprintf(g_stationBellBanner, sizeof(g_stationBellBanner), "[MARITIME BELL] THE DULL PEAL ECHOES FAR OUT OVER THE BLACK SWELLS...");
                    g_stationBellBannerTimer = 5.5f;
                }
            }

            // Sunken Skiff Divers Salvage Box
            float dToCrate = Vector3Distance(camera.position, (Vector3){ 10.4f, 6.8f, 150.8f });
            if (dToCrate < 2.8f && !g_salvageCrateOpened) {
                RegisterPrompt(35, dToCrate, "[E] SALVAGE SUNKEN DIVE CASE", (Color){ 255, 220, 120, 255 });
                if (IsKeyPressed(KEY_E)) {
                    g_salvageCrateOpened = true;
                    PlaySound(g_sndChestOpen);
                    g_playerCash += 45.0f;
                    snprintf(g_stationBellBanner, sizeof(g_stationBellBanner), "SALVAGED WATERPROOF CASE: +$45.00 CASH & DIVER FLARE!");
                    g_stationBellBannerTimer = 6.0f;
                }
            }

            // Crashed Sedan Inspection
            float dToCarInspect = Vector3Distance(camera.position, (Vector3){ 142.8f, 10.5f, 136.5f });
            if (dToCarInspect < 3.2f) {
                RegisterPrompt(15, dToCarInspect, "[E] INSPECT CRASHED SEDAN", (Color){ 200, 230, 255, 255 });
                if (IsKeyPressed(KEY_E)) {
                    snprintf(g_stationBellBanner, sizeof(g_stationBellBanner), "CRASHED SEDAN // RADIATOR PUNCTURED. DRIVER DOOR FORCED FROM INSIDE. TRACKS LEAD AWAY.");
                    g_stationBellBannerTimer = 6.0f;
                }
            }

            // Underground Fuel Reservoir Manhole Inspection
            float dToMh1 = Vector3Distance(camera.position, (Vector3){ 130.5f, 10.02f, 137.5f });
            float dToMh2 = Vector3Distance(camera.position, (Vector3){ 130.5f, 10.02f, 142.5f });
            if (dToMh1 < 2.4f || dToMh2 < 2.4f) {
                RegisterPrompt(15, fminf(dToMh1, dToMh2), TextFormat("[UNDERGROUND FUEL TANK] RESERVES: %.1f / 1000.0 GAL (UNLEADED 87)", g_stationFuelGallons), (Color){ 255, 215, 110, 255 });
            }

            // Shovel pickup prompt near carpet
            if (!g_hasShovel) {
                float dToShovelHUD = Vector3Distance(camera.position, (Vector3){ 84.4f, 10.0f, 138.6f });
                if (dToShovelHUD < 2.8f && !g_isHoldingCart) {
                    RegisterPrompt(40, dToShovelHUD, "[E] TAKE HEAVY TRENCH SHOVEL", (Color){ 255, 235, 160, 255 });
                }
            }

            // Cave-in digging prompt inside deep tunnel
            if (!g_tunnelDug) {
                float dToCaveInHUD = Vector3Distance(camera.position, (Vector3){ 63.2f, 2.4f, 140.0f });
                if (dToCaveInHUD < 3.4f && camera.position.x > 61.5f && camera.position.y < 8.0f) {
                    char digPromptBuf[80];
                    if (!g_hasShovel) {
                        snprintf(digPromptBuf, sizeof(digPromptBuf), "[BLOCKED CAVE-IN] NEED A SHOVEL TO DIG THROUGH");
                    } else if (!g_shovelEquipped) {
                        snprintf(digPromptBuf, sizeof(digPromptBuf), "[BLOCKED CAVE-IN] EQUIP SHOVEL IN HAND TO DIG [%d%%]", (int)(g_digProgress * 100.0f));
                    } else {
                        snprintf(digPromptBuf, sizeof(digPromptBuf), "[RMB / E] DIG THROUGH CAVE-IN (SHOVEL) [%d%%]", (int)(g_digProgress * 100.0f));
                    }
                    RegisterPrompt(50, dToCaveInHUD, digPromptBuf, g_hasShovel ? (Color){ 80, 210, 120, 255 } : (Color){ 215, 60, 50, 255 });
                }
            }

            // Hound Interaction Prompt
            float dHoundPrompt = Vector3Distance(camera.position, g_houndNPC.pos);
            if (dHoundPrompt < 4.0f && !g_isHoldingCart) {
                bool hasBloodBottle = (g_heldProductIndex != -1 && g_shopProducts[g_heldProductIndex].type == PROD_BLOOD && g_shopProducts[g_heldProductIndex].fill > 0.02f);
                if (hasBloodBottle || g_heldProductIndex == -1) {
                    const char* hPrompt = hasBloodBottle ? "[E] GIVE BLOOD TO HOUND" : (g_houndNPC.isPet ? "[E] PET COMPANION" : "[E] PET HOUND");
                    RegisterPrompt(35, dHoundPrompt, hPrompt, hasBloodBottle ? (Color){ 255, 180, 180, 255 } : (Color){ 255, 235, 190, 255 });
                }
            }

            // DRAW EXACTLY ONE UNIFIED INTERACTION PROMPT
            if (activePrompt.active) {
                DrawAAAInteractionBadge(activePrompt.text, activePrompt.accentCol, (float)(LOGICAL_H / 2 + 48));
            }
        }

        // Subterranean Notification Banner
        if (g_tunnelBannerTimer > 0.0f) {
            float bannerAlpha = Clamp(g_tunnelBannerTimer, 0.0f, 1.0f);
            float bw = MeasureTextSharp(g_fontMenu, g_tunnelBannerText, 15.0f) + 40.0f;
            if (bw < 380.0f) bw = 380.0f;
            float bx = ((float)LOGICAL_W - bw) * 0.5f;
            float by = 68.0f;
            DrawAAAPanel((Rectangle){ bx, by, bw, 34.0f }, (Color){ 8, 10, 14, (unsigned char)(235 * bannerAlpha) }, (Color){ 215, 55, 45, (unsigned char)(255 * bannerAlpha) }, 5.0f, true);
            DrawTextSharpCentered(g_fontMenu, g_tunnelBannerText, (float)LOGICAL_W * 0.5f, by + 8.5f, 15.0f, (Color){ 245, 235, 220, (unsigned char)(255 * bannerAlpha) });
        }

        // =========================================================================
        // AAA DAY / NIGHT SURVIVAL CLOCK & CELESTIAL TELEMETRY HUD (TOP-RIGHT)
        // =========================================================================
        if (g_gameState == STATE_GAMEPLAY && !isRoofCamActive && !showQuitConfirm) {
            float cycleFrac = dayCycleTime / dayCycleDuration;
            float time24 = cycleFrac * 24.0f;
            int inGameHour = (int)time24 % 24;
            int inGameMin  = (int)((time24 - floorf(time24)) * 60.0f);
            int displayHour = inGameHour % 12;
            if (displayHour == 0) displayHour = 12;
            const char* ampm = (inGameHour >= 12) ? "PM" : "AM";

            const char* phaseStr = "DAYLIGHT";
            Color phaseCol = (Color){ 255, 215, 80, 255 };
            if (inGameHour >= 21 || inGameHour < 5) {
                phaseStr = "NIGHT [NOCTURNAL ACTIVE]";
                phaseCol = (Color){ 110, 220, 255, 255 };
            } else if (inGameHour >= 5 && inGameHour < 7) {
                phaseStr = "DAWN";
                phaseCol = (Color){ 255, 170, 120, 255 };
            } else if (inGameHour >= 18 && inGameHour < 21) {
                phaseStr = "DUSK / TWILIGHT";
                phaseCol = (Color){ 240, 130, 170, 255 };
            }

            float hudW = 320.0f;
            float hudH = 56.0f;
            float hudX = (float)LOGICAL_W - hudW - 16.0f;
            float hudY = 16.0f;

            DrawAAAPanel((Rectangle){ hudX, hudY, hudW, hudH }, (Color){ 12, 16, 20, 230 }, (Color){ 70, 80, 95, 200 }, 6.0f, true);

            char timeStr[96];
            snprintf(timeStr, sizeof(timeStr), "TIME: %02d:%02d %s  |  %s", displayHour, inGameMin, ampm, phaseStr);
            DrawTextSharp(g_fontMenu, timeStr, hudX + 14.0f, hudY + 11.0f, 14.0f, phaseCol);

            const char* timeControls = "[[ / ]] SCRUB TIME   |   [P] PAUSE TIME";
            DrawTextSharp(g_fontSmall, timeControls, hudX + 14.0f, hudY + 33.0f, 12.0f, (Color){ 160, 170, 185, 210 });
        }

        // =========================================================================
        // AAA FULL-SCREEN HORROR RECEIPT READING / INSPECTION OVERLAY
        // =========================================================================
        if (g_inspectingReceipt && g_hasReceipt) {
            DrawRectangle(0, 0, LOGICAL_W, LOGICAL_H, (Color){ 0, 0, 0, 200 });
            int rW = 340;
            int rH = 680;
            int rx = LOGICAL_W / 2 - rW / 2;
            int ry = LOGICAL_H / 2 - rH / 2;

            DrawRectangle(rx + 6, ry + 8, rW, rH, (Color){ 0, 0, 0, 110 });
            DrawRectangle(rx + 3, ry + 4, rW, rH, (Color){ 0, 0, 0, 150 });
            DrawTexturePro(g_receiptTex, (Rectangle){ 0, 0, (float)g_receiptTex.width, (float)g_receiptTex.height },
                           (Rectangle){ (float)rx, (float)ry, (float)rW, (float)rH }, (Vector2){ 0, 0 }, 0.0f, WHITE);
            DrawRectangleLines(rx - 1, ry - 1, rW + 2, rH + 2, (Color){ 160, 160, 160, 160 });

            const char* inspectHint = "[Q] LOWER RECEIPT   |   [G] THROW AWAY RECEIPT";
            float hw = MeasureTextSharp(g_fontMenu, inspectHint, 15.0f);
            float hpw = hw + 36.0f;
            DrawAAAPanel((Rectangle){ ((float)LOGICAL_W - hpw) * 0.5f, (float)LOGICAL_H - 46.0f, hpw, 32.0f }, (Color){ 18, 15, 14, 240 }, (Color){ 225, 175, 75, 220 }, 5.0f, true);
            DrawTextSharpCentered(g_fontMenu, inspectHint, (float)LOGICAL_W * 0.5f, (float)LOGICAL_H - 38.0f, 15.0f, (Color){ 255, 225, 160, 255 });
        }

        // =========================================================================
        // AAA ROOF CCTV SURVEILLANCE OVERLAY (ZERO DUPLICATE TEXT & CLEAN CRT TELEMETRY)
        // =========================================================================
        if (isRoofCamActive) {
            for (int sl = 0; sl < LOGICAL_H; sl += 4) {
                DrawLine(0, sl, LOGICAL_W, sl, { 0, 0, 0, 45 });
            }
            DrawRectangle(0, 0, LOGICAL_W, LOGICAL_H, { 10, 25, 15, 25 });

            bool recBlink = (fmodf(timeVal, 0.8f) < 0.4f);
            if (recBlink) {
                DrawCircle(24, 24, 6, { 240, 20, 20, 255 });
            }
            DrawTextSharp(g_fontMenu, "REC", 36, 16, 15.0f, { 240, 20, 20, 255 });
            DrawTextSharp(g_fontMenu, "CAM 01 - FRONT ROOF WALL // SURVEILLANCE FEED [LIVE]", 85, 16, 15.0f, { 190, 240, 190, 245 });

            float zoomMag = 65.0f / roofCamFOV;
            char telemBuf[160];
            snprintf(telemBuf, sizeof(telemBuf), "RAIL POS: %+.1fm  |  ZOOM: %.1fx  |  PAN: %+.0f*  |  PITCH: %+.0f*",
                     roofCamSlideX, zoomMag, roofCamYaw, roofCamPitch);
            DrawTextSharp(g_fontSmall, telemBuf, 85, 37, 12.0f, { 140, 220, 140, 230 });
            DrawTextSharp(g_fontSmall, "SURVEILLANCE ACTIVE", LOGICAL_W - 200, 17, 12.0f, { 120, 240, 120, 220 });

            int cw = 28, ch = 28, thick = 2;
            DrawRectangle(14, 14, cw, thick, { 100, 180, 100, 200 });
            DrawRectangle(14, 14, thick, ch, { 100, 180, 100, 200 });
            DrawRectangle(LOGICAL_W - 14 - cw, 14, cw, thick, { 100, 180, 100, 200 });
            DrawRectangle(LOGICAL_W - 14, 14, thick, ch, { 100, 180, 100, 200 });
            DrawRectangle(14, LOGICAL_H - 14 - thick, cw, thick, { 100, 180, 100, 200 });
            DrawRectangle(14, LOGICAL_H - 14 - ch, thick, ch, { 100, 180, 100, 200 });
            DrawRectangle(LOGICAL_W - 14 - cw, LOGICAL_H - 14 - thick, cw, thick, { 100, 180, 100, 200 });
            DrawRectangle(LOGICAL_W - 14, LOGICAL_H - 14 - ch, thick, ch, { 100, 180, 100, 200 });

            DrawLine(LOGICAL_W/2 - 14, LOGICAL_H/2, LOGICAL_W/2 + 14, LOGICAL_H/2, { 100, 180, 100, 160 });
            DrawLine(LOGICAL_W/2, LOGICAL_H/2 - 14, LOGICAL_W/2, LOGICAL_H/2 + 14, { 100, 180, 100, 160 });

            const char* cctvControls = "[A/D] SLIDE ROOF WALL   |   [MOUSE] PAN/TILT   |   [W/S/WHEEL] ZOOM   |   [C / ESC] EXIT";
            float cmw = MeasureTextSharp(g_fontSmall, cctvControls, 13.0f);
            DrawAAAPanel((Rectangle){ ((float)LOGICAL_W - cmw) * 0.5f - 16.0f, (float)LOGICAL_H - 42.0f, cmw + 32.0f, 28.0f }, (Color){ 10, 16, 12, 235 }, (Color){ 70, 165, 75, 220 }, 4.0f, true);
            DrawTextSharpCentered(g_fontSmall, cctvControls, (float)LOGICAL_W * 0.5f, (float)LOGICAL_H - 34.5f, 13.0f, { 185, 245, 185, 255 });
        }

        // =========================================================================
        // AAA SURREAL SHOPKEEPER STORE UI OVERLAY
        // =========================================================================
        if (isShopOpen) {
            float bw = 740.0f, bh = 510.0f;
            float bx = ((float)LOGICAL_W - bw) * 0.5f;
            float by = ((float)LOGICAL_H - bh) * 0.5f;

            DrawAAAPanel((Rectangle){ bx, by, bw, bh }, (Color){ 12, 15, 18, 252 }, (Color){ 215, 140, 50, 240 }, 8.0f, true);

            DrawTextSharp(g_fontHeadSub, "THE STRANGE HOUR // GENERAL SUPERSTORE", bx + 28.0f, by + 22.0f, 24.0f, (Color){ 255, 225, 140, 255 });
            DrawTextSharp(g_fontBody, "\"WE DO NOT ACCEPT RETURNS. OR REFUNDS. OR YOU.\"", bx + 28.0f, by + 52.0f, 13.0f, (Color){ 180, 170, 155, 230 });
            DrawLine((int)(bx + 20.0f), (int)(by + 74.0f), (int)(bx + bw - 20.0f), (int)(by + 74.0f), (Color){ 120, 85, 45, 190 });

            const char* itemKeys[7] = { "1", "2", "3", "4", "5", "6", "7" };
            const char* itemNames[7] = {
                "Bottled Whispers", "Canned Silence", "Expired Sunlight",
                "Your Old Wallet", "Your Current Car Key", "Jar of Loose Teeth", "Jar of Donkey Milk"
            };
            const char* itemPrices[7] = { "$6.66", "$4.44", "$0.00", "$9.00", "$0.00", "$13.13", "$7.77" };
            const char* itemDescs[7] = {
                "\"A corked flask of trapped voices from 1984.\"",
                "\"Heavier than lead. Shake it and nothing sounds.\"",
                "\"Tastes like copper and warm asphalt.\"",
                "\"Lost three years ago. Inside is your expired ID.\"",
                "\"Cold to touch. Exactly matches the one in your pocket.\"",
                "\"Rattles with a dry snap when you don't look at it.\"",
                "\"Freshly harvested. The donkey had no eyes.\""
            };

            for (int i = 0; i < 7; i++) {
                float rowY = by + 86.0f + (float)i * 35.0f;
                Color nameCol = (i == 4) ? (Color){ 255, 220, 80, 255 } : ((i == 6) ? (Color){ 255, 245, 215, 255 } : (Color){ 225, 230, 235, 255 });

                DrawAAAKeycap(itemKeys[i], bx + 28.0f, rowY, (Color){ 255, 215, 120, 255 });
                DrawTextSharp(g_fontMenu, itemNames[i], bx + 64.0f, rowY + 3.0f, 15.0f, nameCol);
                DrawTextSharp(g_fontMenu, itemPrices[i], bx + 265.0f, rowY + 3.0f, 15.0f, (Color){ 255, 200, 80, 255 });
                DrawTextSharp(g_fontBody, itemDescs[i], bx + 335.0f, rowY + 4.5f, 12.0f, (Color){ 165, 170, 175, 220 });
            }

            DrawLine((int)(bx + 20.0f), (int)(by + 338.0f), (int)(bx + bw - 20.0f), (int)(by + 338.0f), (Color){ 120, 85, 45, 190 });

            if (shopFeedbackMsg != nullptr) {
                DrawTextSharp(g_fontMenu, shopFeedbackMsg, bx + 28.0f, by + 352.0f, 15.0f, (Color){ 255, 230, 120, 255 });
            } else {
                DrawTextSharp(g_fontBody, "Mr. Grethnar Woule stands motionless. His neck occasionally snaps 76 degrees.", bx + 28.0f, by + 354.0f, 13.0f, (Color){ 165, 170, 175, 220 });
            }

            DrawAAAPanel((Rectangle){ bx + bw * 0.5f - 240.0f, by + bh - 42.0f, 480.0f, 30.0f }, (Color){ 18, 20, 24, 235 }, (Color){ 165, 120, 60, 200 }, 4.0f, false);
            DrawTextSharpCentered(g_fontSmall, "PRESS [1 - 7] TO PURCHASE   |   PRESS [E] OR [ESC] TO EXIT", bx + bw * 0.5f, by + bh - 34.0f, 13.0f, (Color){ 235, 205, 135, 240 });
        }

        // =========================================================================
        // AAA TACTICAL CROSSHAIR RETICLE
        // =========================================================================
        if (g_gameState == STATE_GAMEPLAY && !isShopOpen && !isRoofCamActive) {
            Color crosshairCol = (hudFocusIdx != -1 && !nearCounter && g_heldProductIndex == -1) ? (Color){ 90, 245, 130, 255 } : CYAN;
            int cx = LOGICAL_W / 2;
            int cy = LOGICAL_H / 2;
            DrawCircle(cx, cy, 1.8f, crosshairCol);
            DrawLine(cx - 8, cy, cx - 3, cy, crosshairCol);
            DrawLine(cx + 4, cy, cx + 9, cy, crosshairCol);
            DrawLine(cx, cy - 8, cx, cy - 3, crosshairCol);
            DrawLine(cx, cy + 4, cx, cy + 9, crosshairCol);
        }

        // HYPER-REALISTIC SMARTPHONE & LIVE "MIRE-NAV" GPS MAP SYSTEM

        // Slides up smoothly from pocket on [M], showing live player GPS & world map

        // =========================================================================

        if (g_phoneAnim > 0.005f && g_gameState == STATE_GAMEPLAY && !isRoofCamActive) {

            float pW = 295.0f;

            float pH = 570.0f;

            float phoneLagX = Clamp(g_vmSwayX * 240.0f, -20.0f, 20.0f);

            float phoneLagY = Clamp(g_vmSwayY * 180.0f, -16.0f, 16.0f);

            float pX = (float)LOGICAL_W - pW - 35.0f + phoneLagX;

            // Ease-out smooth slide from below bottom screen edge + viewmodel inertia

            float pY = (float)LOGICAL_H - (pH - 24.0f) * g_phoneAnim + phoneLagY;



            // 1. Soft Physical Ambient Drop Shadow

            DrawRectangleRounded((Rectangle){ pX + 8, pY + 12, pW, pH }, 0.08f, 16, (Color){ 0, 0, 0, (unsigned char)(110 * g_phoneAnim) });

            DrawRectangleRounded((Rectangle){ pX + 4, pY + 6, pW, pH }, 0.08f, 16, (Color){ 0, 0, 0, (unsigned char)(150 * g_phoneAnim) });



            // 2. Titanium / Matte Metal Smartphone Chassis

            DrawRectangleRounded((Rectangle){ pX, pY, pW, pH }, 0.08f, 16, (Color){ 34, 37, 42, 255 });

            DrawRectangleRoundedLinesEx((Rectangle){ pX, pY, pW, pH }, 0.08f, 16, 2.0f, (Color){ 72, 78, 86, 255 });



            // Physical Hardware Buttons on Outer Rim

            DrawRectangle((int)(pX - 3), (int)(pY + 115), 3, 26, (Color){ 48, 52, 58, 255 }); // Vol Up

            DrawRectangle((int)(pX - 3), (int)(pY + 152), 3, 26, (Color){ 48, 52, 58, 255 }); // Vol Down

            DrawRectangle((int)(pX + pW), (int)(pY + 130), 3, 38, (Color){ 48, 52, 58, 255 }); // Power



            // 3. AMOLED Glass Bezel & Screen Base

            float scrX = pX + 11.0f;

            float scrY = pY + 12.0f;

            float scrW = pW - 22.0f; // 273px

            float scrH = pH - 24.0f; // 546px

            DrawRectangleRounded((Rectangle){ scrX, scrY, scrW, scrH }, 0.06f, 12, (Color){ 12, 14, 18, 255 });



            // 4. Dynamic Island / Front Camera Pill Notch

            DrawRectangleRounded((Rectangle){ pX + pW/2 - 28, scrY + 5, 56, 13 }, 0.5f, 8, (Color){ 6, 7, 9, 255 });

            DrawCircle((int)(pX + pW/2 + 14), (int)(scrY + 11), 3, (Color){ 22, 38, 62, 255 }); // Lens reflection dot

            DrawRectangle((int)(pX + pW/2 - 14), (int)(scrY + 1), 28, 2, (Color){ 28, 30, 35, 255 }); // Ear speaker slit



            // 5. STATUS BAR (Top of Phone Screen)

            // Clock (dynamically synced to in-game day/night survival time)

            float cycleFracClock = dayCycleTime / dayCycleDuration;

            float time24Clock = cycleFracClock * 24.0f;

            int phoneHour = (int)time24Clock % 24;

            int phoneMin  = (int)((time24Clock - floorf(time24Clock)) * 60.0f);

            int displayHourPhone = phoneHour % 12;

            if (displayHourPhone == 0) displayHourPhone = 12;

            char clockBuf[16];

            snprintf(clockBuf, sizeof(clockBuf), "%02d:%02d %s", displayHourPhone, phoneMin, (phoneHour >= 12) ? "PM" : "AM");

            DrawTextSharp(g_fontSmall, clockBuf, scrX + 10, scrY + 6, 11.0f, (Color){ 230, 235, 240, 255 });



            // Cellular & Battery Status (flickering low service in horror zone)

            bool signalFlicker = (fmodf(g_phoneSignalFlicker, 4.2f) > 3.6f);

            DrawTextSharp(g_fontSmall, signalFlicker ? "NO SERVICE" : "1 BAR [E]", scrX + scrW - 105, scrY + 6, 10.0f, signalFlicker ? (Color){ 225, 65, 55, 255 } : (Color){ 160, 165, 175, 220 });

            // Battery icon & percentage

            DrawRectangleLines((int)(scrX + scrW - 28), (int)(scrY + 7), 16, 9, (Color){ 175, 180, 190, 240 });

            DrawRectangle((int)(scrX + scrW - 12), (int)(scrY + 9), 2, 5, (Color){ 175, 180, 190, 240 });

            DrawRectangle((int)(scrX + scrW - 26), (int)(scrY + 9), 4, 5, (Color){ 230, 65, 55, 255 }); // Low 18% battery in red



            // 6. "MIRE-NAV" APP BANNER

            DrawRectangle((int)scrX, (int)(scrY + 22), (int)scrW, 28, (Color){ 18, 22, 28, 255 });

            DrawLine((int)scrX, (int)(scrY + 50), (int)(scrX + scrW), (int)(scrY + 50), (Color){ 45, 55, 68, 255 });

            DrawTextSharp(g_fontSmall, "MIRE-NAV // GPS SATELLITE (OFFLINE)", scrX + 8, scrY + 26, 10.0f, (Color){ 85, 195, 245, 255 });



            // Dynamic Sector Banner

            const char* sectorStr = "ROUTE 9 NORTH PASS";

            if (camera.position.y < 8.5f) {

                sectorStr = "SUB-TERRAIN BUNKER (-18 FT)";

            } else if (camera.position.x >= 86.0f && camera.position.x <= 108.0f && camera.position.z >= 126.0f && camera.position.z <= 154.0f) {

                sectorStr = "SUPERSTORE INTERIOR";

            } else if (camera.position.x >= 110.0f && camera.position.x <= 124.0f && camera.position.z >= 132.0f && camera.position.z <= 148.0f) {

                sectorStr = "SERVICE STATION FORECOURT";

            } else if (camera.position.x > 135.0f) {

                sectorStr = "PEAT MIRE & BOG DRAINAGE";

            } else if (camera.position.x <= 36.0f) {

                sectorStr = (g_waterState == WATER_STATE_DIVING) ? "BLACKWATER OCEAN (DIVING)" : "BLACKWATER COAST & PIER";

            } else if (camera.position.x < 85.0f) {

                sectorStr = "WEST WALL ALLEY (BEHIND SHOP)";

            }

            DrawTextSharp(g_fontSmall, sectorStr, scrX + 8, scrY + 38, 9.0f, (Color){ 215, 205, 180, 220 });



            // 7. THE INTERACTIVE GPS MAP CANVAS

            float mapX = scrX + 4.0f;

            float mapY = scrY + 53.0f;

            float mapW = scrW - 8.0f;  // 265px

            float mapH = scrH - 128.0f; // 418px



            // Dark satellite topographical background

            DrawRectangle((int)mapX, (int)mapY, (int)mapW, (int)mapH, (Color){ 15, 18, 22, 255 });

            DrawRectangleLines((int)mapX, (int)mapY, (int)mapW, (int)mapH, (Color){ 35, 42, 52, 255 });



            // Map scale & center definition

            float viewCenterX = (g_phoneZoomMode == 0) ? camera.position.x : 115.0f;

            float viewCenterZ = (g_phoneZoomMode == 0) ? camera.position.z : 140.0f;

            float mapScale    = (g_phoneZoomMode == 0) ? 4.4f : 1.35f; // pixels per world meter



            auto WorldToMap = [&](float wx, float wz) -> Vector2 {

                float mx = mapX + mapW * 0.5f + (wx - viewCenterX) * mapScale;

                float my = mapY + mapH * 0.5f + (wz - viewCenterZ) * mapScale;

                return (Vector2){ mx, my };

            };



            auto InMapBounds = [&](float mx, float my, float pad = 0.0f) -> bool {

                return (mx >= mapX - pad && mx <= mapX + mapW + pad && my >= mapY - pad && my <= mapY + mapH + pad);

            };



            // Topographical Grid Lines (every 20 meters)

            for (int gx = 40; gx <= 220; gx += 20) {

                Vector2 g1 = WorldToMap((float)gx, 60.0f);

                Vector2 g2 = WorldToMap((float)gx, 220.0f);

                if ((g1.x >= mapX && g1.x <= mapX + mapW) || (g2.x >= mapX && g2.x <= mapX + mapW)) {

                    float clx = Clamp(g1.x, mapX, mapX + mapW);

                    DrawLine((int)clx, (int)fmaxf(g1.y, mapY), (int)clx, (int)fminf(g2.y, mapY + mapH), (Color){ 28, 34, 42, 160 });

                }

            }

            for (int gz = 60; gz <= 220; gz += 20) {

                Vector2 g1 = WorldToMap(40.0f, (float)gz);

                Vector2 g2 = WorldToMap(220.0f, (float)gz);

                if ((g1.y >= mapY && g1.y <= mapY + mapH) || (g2.y >= mapY && g2.y <= mapY + mapH)) {

                    float cly = Clamp(g1.y, mapY, mapY + mapH);

                    DrawLine((int)fmaxf(g1.x, mapX), (int)cly, (int)fminf(g2.x, mapX + mapW), (int)cly, (Color){ 28, 34, 42, 160 });

                }

            }



            // A. Dense Spruce Forest Zones

            Vector2 fWestTop = WorldToMap(38.0f, 60.0f);

            Vector2 fWestBot = WorldToMap(75.0f, 220.0f);

            float fwX = Clamp(fWestTop.x, mapX, mapX + mapW);

            float fwW = Clamp(fWestBot.x - fwX, 0.0f, mapX + mapW - fwX);

            if (fwW > 0.0f) {

                DrawRectangle((int)fwX, (int)fmaxf(fWestTop.y, mapY), (int)fwW, (int)fminf(fWestBot.y - fWestTop.y, mapH), (Color){ 12, 18, 14, 210 });

            }



            // A2. The Blackwater Coast & Ocean (West of X=35)
            Vector2 ocTop = WorldToMap(-50.0f, 0.0f);
            Vector2 ocBot = WorldToMap(35.0f, 256.0f);
            float ocX = Clamp(ocTop.x, mapX, mapX + mapW);
            float ocW = Clamp(ocBot.x - ocX, 0.0f, mapX + mapW - ocX);
            if (ocW > 0.0f) {
                DrawRectangle((int)ocX, (int)mapY, (int)ocW, (int)mapH, (Color){ 10, 26, 38, 235 });
                DrawLine((int)(ocX + ocW), (int)mapY, (int)(ocX + ocW), (int)(mapY + mapH), (Color){ 45, 120, 160, 240 });
                Vector2 pierStart = WorldToMap(36.0f, 138.0f);
                Vector2 pierEnd   = WorldToMap(16.0f, 138.0f);
                DrawLineEx(pierStart, pierEnd, 3.0f, (Color){ 140, 115, 75, 255 });
                if (InMapBounds(ocX + 4, mapY + 20)) {
                    DrawTextSharp(g_fontSmall, "BLACKWATER SEA", ocX + 6, mapY + 12, 9.0f, (Color){ 65, 185, 215, 230 });
                }
            }

            // B. Peat Mire & Drainage Bog (East of road, X: 136..185, Z: 95..185)

            Vector2 mireP1 = WorldToMap(136.0f, 95.0f);

            Vector2 mireP2 = WorldToMap(185.0f, 185.0f);

            float mX = Clamp(mireP1.x, mapX, mapX + mapW);

            float mY = Clamp(mireP1.y, mapY, mapY + mapH);

            float mW = Clamp(mireP2.x - mX, 0.0f, mapX + mapW - mX);

            float mH = Clamp(mireP2.y - mY, 0.0f, mapY + mapH - mY);

            if (mW > 0.0f && mH > 0.0f) {

                DrawRectangle((int)mX, (int)mY, (int)mW, (int)mH, (Color){ 16, 26, 24, 230 });

                DrawRectangleLines((int)mX, (int)mY, (int)mW, (int)mH, (Color){ 28, 48, 42, 240 });

                if (InMapBounds(mX + 6, mY + 12)) {

                    DrawTextSharp(g_fontSmall, "PEAT MIRE [HAZARD]", mX + 6, mY + 8, 9.0f, (Color){ 55, 125, 95, 220 });

                }

            }



            // C. Highway 9 (Route 9 Road Strip, centered at X=128, width 14)

            Vector2 rTop = WorldToMap(121.0f, 60.0f);

            Vector2 rBot = WorldToMap(135.0f, 220.0f);

            float rx = Clamp(rTop.x, mapX, mapX + mapW);

            float rw = Clamp(rBot.x - rTop.x, 0.0f, mapX + mapW - rx);

            float ry = fmaxf(rTop.y, mapY);

            float rh = fminf(rBot.y - rTop.y, mapY + mapH - ry);

            if (rw > 0.0f && rh > 0.0f) {

                // Asphalt Road Bed

                DrawRectangle((int)rx, (int)ry, (int)rw, (int)rh, (Color){ 38, 42, 48, 255 });

                // White outer shoulder lines

                DrawLine((int)rx, (int)ry, (int)rx, (int)(ry + rh), (Color){ 160, 165, 175, 220 });

                DrawLine((int)(rx + rw), (int)ry, (int)(rx + rw), (int)(ry + rh), (Color){ 160, 165, 175, 220 });

                // Broken Yellow Center Line

                Vector2 rMid = WorldToMap(128.0f, 60.0f);

                if (rMid.x >= mapX && rMid.x <= mapX + mapW) {

                    for (float dz = 60.0f; dz < 220.0f; dz += 8.0f) {

                        Vector2 d1 = WorldToMap(128.0f, dz);

                        Vector2 d2 = WorldToMap(128.0f, dz + 4.5f);

                        if (d1.y >= mapY && d2.y <= mapY + mapH) {

                            DrawLine((int)d1.x, (int)d1.y, (int)d2.x, (int)d2.y, (Color){ 220, 185, 45, 240 });

                        }

                    }

                }

            }



            // D. Service Station Parking Apron (X: 78..115, Z: 120..160)

            Vector2 lotP1 = WorldToMap(78.0f, 120.0f);

            Vector2 lotP2 = WorldToMap(115.0f, 160.0f);

            float lx = Clamp(lotP1.x, mapX, mapX + mapW);

            float ly = Clamp(lotP1.y, mapY, mapY + mapH);

            float lw = Clamp(lotP2.x - lx, 0.0f, mapX + mapW - lx);

            float lh = Clamp(lotP2.y - ly, 0.0f, mapY + mapH - ly);

            if (lw > 0.0f && lh > 0.0f) {

                DrawRectangle((int)lx, (int)ly, (int)lw, (int)lh, (Color){ 28, 30, 35, 255 });

                DrawRectangleLines((int)lx, (int)ly, (int)lw, (int)lh, (Color){ 52, 58, 68, 255 });

            }



            // E. Supermarket Building (X: 86..108, Z: 126..154)

            Vector2 shopP1 = WorldToMap(86.0f, 126.0f);

            Vector2 shopP2 = WorldToMap(108.0f, 154.0f);

            float sx = Clamp(shopP1.x, mapX, mapX + mapW);

            float sy = Clamp(shopP1.y, mapY, mapY + mapH);

            float sw = Clamp(shopP2.x - sx, 0.0f, mapX + mapW - sx);

            float sh = Clamp(shopP2.y - sy, 0.0f, mapY + mapH - sy);

            if (sw > 0.0f && sh > 0.0f) {

                DrawRectangle((int)sx, (int)sy, (int)sw, (int)sh, (Color){ 44, 48, 56, 255 });

                DrawRectangleLines((int)sx, (int)sy, (int)sw, (int)sh, (Color){ 165, 175, 190, 255 });

                // East Door Entrance Line (cyan)

                Vector2 doorP1 = WorldToMap(108.0f, 138.5f);

                Vector2 doorP2 = WorldToMap(108.0f, 141.5f);

                if (InMapBounds(doorP1.x, doorP1.y)) {

                    DrawLine((int)doorP1.x, (int)doorP1.y, (int)doorP2.x, (int)doorP2.y, (Color){ 65, 215, 245, 255 });

                }

                // Building Label

                if (InMapBounds(sx + 6, sy + sh/2)) {

                    DrawTextSharp(g_fontSmall, "SUPERMARKET", sx + 6, sy + sh/2 - 6, 9.0f, (Color){ 220, 225, 235, 240 });

                }

            }



            // F. Fuel Pumps Canopy (X: 113..120, Z: 135..145)

            Vector2 canP1 = WorldToMap(113.0f, 135.0f);

            Vector2 canP2 = WorldToMap(120.0f, 145.0f);

            if (canP1.x >= mapX && canP2.x <= mapX + mapW && canP1.y >= mapY && canP2.y <= mapY + mapH) {

                DrawRectangle((int)canP1.x, (int)canP1.y, (int)(canP2.x - canP1.x), (int)(canP2.y - canP1.y), (Color){ 36, 40, 48, 220 });

                DrawRectangleLines((int)canP1.x, (int)canP1.y, (int)(canP2.x - canP1.x), (int)(canP2.y - canP1.y), (Color){ 230, 110, 45, 255 });

                DrawTextSharp(g_fontSmall, "PUMPS", canP1.x + 4, canP1.y + 4, 8.0f, (Color){ 245, 140, 70, 255 });

            }



            // G. Crashed Sedan (Mile Marker 14, East Verge: X = 143.8, Z = 136.5)

            Vector2 carMap = WorldToMap(143.8f, 136.5f);

            if (InMapBounds(carMap.x, carMap.y, 10.0f)) {

                DrawRectangle((int)(carMap.x - 4), (int)(carMap.y - 7), 8, 14, (Color){ 180, 185, 195, 255 });

                DrawCircle((int)carMap.x, (int)carMap.y, 2, RED);

                DrawTextSharp(g_fontSmall, "CAR #14", carMap.x + 6, carMap.y - 5, 8.0f, (Color){ 235, 80, 70, 255 });

            }



            // H. Exterior Secret Hatch (Behind West Wall, X = 83.8, Z = 140.0)

            Vector2 hatchMap = WorldToMap(83.8f, 140.0f);

            if (InMapBounds(hatchMap.x, hatchMap.y, 12.0f)) {

                DrawRectangle((int)(hatchMap.x - 3), (int)(hatchMap.y - 3), 6, 6, (Color){ 195, 45, 40, 255 });

                DrawRectangleLines((int)(hatchMap.x - 4), (int)(hatchMap.y - 4), 8, 8, (Color){ 235, 185, 65, 255 });

                DrawTextSharp(g_fontSmall, "HATCH", hatchMap.x - 32, hatchMap.y - 5, 8.0f, (Color){ 235, 185, 65, 240 });

            }



            // I. Subterranean Bunker Blueprint (If underground or zoomed in)

            if (camera.position.y < 8.5f || g_phoneZoomMode == 0) {

                Vector2 cor1 = WorldToMap(84.0f, 139.0f);

                Vector2 cor2 = WorldToMap(95.0f, 141.0f);

                Vector2 ch1  = WorldToMap(95.0f, 137.0f);

                Vector2 ch2  = WorldToMap(101.4f, 143.0f);

                if (InMapBounds(cor1.x, cor1.y, 20.0f)) {

                    DrawRectangleLines((int)cor1.x, (int)cor1.y, (int)(cor2.x - cor1.x), (int)(cor2.y - cor1.y), (Color){ 65, 210, 180, 180 });

                    DrawRectangleLines((int)ch1.x, (int)ch1.y, (int)(ch2.x - ch1.x), (int)(ch2.y - ch1.y), (Color){ 65, 210, 180, 220 });

                    if (camera.position.y < 8.5f) {

                        DrawTextSharp(g_fontSmall, "BUNKER (-18 FT)", ch1.x + 4, ch1.y + 6, 8.0f, (Color){ 65, 240, 200, 255 });

                    }

                }

            }



            // J. LIVE PLAYER GPS BEACON & DIRECTIONAL RADAR

            Vector2 pMap = WorldToMap(camera.position.x, camera.position.z);

            if (InMapBounds(pMap.x, pMap.y, 15.0f)) {

                // Expanding Radar Pulse Wave

                float rRadius = 6.0f + g_phoneRadarPulse * 28.0f;

                unsigned char rAlpha = (unsigned char)(210.0f * (1.0f - g_phoneRadarPulse));

                DrawCircleLines((int)pMap.x, (int)pMap.y, rRadius, (Color){ 45, 185, 255, rAlpha });



                // Directional Heading Arrow & View Cone

                float fwdX = camera.target.x - camera.position.x;

                float fwdZ = camera.target.z - camera.position.z;

                float coneAngle = atan2f(fwdZ, fwdX);

                float coneDist = 20.0f;

                Vector2 tipPos = { pMap.x + cosf(coneAngle) * coneDist, pMap.y + sinf(coneAngle) * coneDist };

                Vector2 leftWing  = { pMap.x + cosf(coneAngle + 2.5f) * 9.0f, pMap.y + sinf(coneAngle + 2.5f) * 9.0f };

                Vector2 rightWing = { pMap.x + cosf(coneAngle - 2.5f) * 9.0f, pMap.y + sinf(coneAngle - 2.5f) * 9.0f };



                DrawTriangle(tipPos, leftWing, rightWing, (Color){ 45, 195, 255, 240 });

                DrawTriangleLines(tipPos, leftWing, rightWing, WHITE);



                // Central GPS Blue Beacon Dot

                DrawCircle((int)pMap.x, (int)pMap.y, 4, (Color){ 0, 140, 255, 255 });

                DrawCircle((int)pMap.x, (int)pMap.y, 2, WHITE);

            }



            // K. Minimalist Compass Rose (Upper Right of Map Canvas)

            int compX = (int)(mapX + mapW - 20);

            int compY = (int)(mapY + 20);

            DrawCircle(compX, compY, 11, (Color){ 18, 22, 28, 220 });

            DrawCircleLines(compX, compY, 11, (Color){ 55, 65, 80, 220 });

            DrawTriangle((Vector2){ (float)compX, (float)(compY - 9) }, (Vector2){ (float)(compX - 4), (float)compY }, (Vector2){ (float)(compX + 4), (float)compY }, RED);

            DrawTriangle((Vector2){ (float)compX, (float)(compY + 9) }, (Vector2){ (float)(compX + 4), (float)compY }, (Vector2){ (float)(compX - 4), (float)compY }, (Color){ 180, 185, 195, 220 });

            DrawTextSharp(g_fontSmall, "N", compX - 3, compY - 8, 8.0f, WHITE);



            // 8. APP FOOTER & TELEMETRY CONTROLS

            float footY = mapY + mapH + 4.0f;

            DrawRectangle((int)scrX, (int)footY, (int)scrW, 44, (Color){ 16, 18, 24, 255 });

            DrawLine((int)scrX, (int)footY, (int)(scrX + scrW), (int)footY, (Color){ 45, 55, 65, 255 });



            // Coordinates & Altitude

            char coordBuf[64];

            float altVal = (camera.position.y < 8.5f) ? 298.0f : 312.0f;

            snprintf(coordBuf, sizeof(coordBuf), "LAT 44.829 N  LON 71.304 W  |  ALT: %.0fm", altVal);

            DrawTextSharp(g_fontSmall, coordBuf, scrX + 8, footY + 5, 9.0f, (Color){ 175, 185, 195, 220 });



            // Interactive Zoom Mode & Pocket Hint

            const char* zoomStr = (g_phoneZoomMode == 0) ? "[Z] ZOOM: LOCAL (1x)" : "[Z] ZOOM: REGION (2x)";

            DrawTextSharp(g_fontSmall, zoomStr, scrX + 8, footY + 22, 10.0f, (Color){ 75, 215, 245, 255 });

            DrawTextSharp(g_fontSmall, "[M] POCKET", scrX + scrW - 68, footY + 22, 10.0f, (Color){ 230, 205, 140, 240 });



            // 9. HYPER-REALISTIC GLASS OVERLAYS

            // Diagonal specular glass reflection streak

            DrawTriangle((Vector2){ scrX + 15, scrY + 2 }, (Vector2){ scrX + 75, scrY + 2 }, (Vector2){ scrX + scrW - 15, scrY + scrH - 10 }, (Color){ 255, 255, 255, 10 });

            // Micro hairline glass crack in top-right corner (horror tactile atmosphere)

            DrawLine((int)(scrX + scrW - 18), (int)(scrY + 12), (int)(scrX + scrW - 48), (int)(scrY + 52), (Color){ 215, 225, 235, 80 });

            DrawLine((int)(scrX + scrW - 48), (int)(scrY + 52), (int)(scrX + scrW - 32), (int)(scrY + 84), (Color){ 215, 225, 235, 60 });

            DrawLine((int)(scrX + scrW - 48), (int)(scrY + 52), (int)(scrX + scrW - 74), (int)(scrY + 68), (Color){ 215, 225, 235, 50 });

        }



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

            (Rectangle){ 0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height },

            (Rectangle){ 0.0f, 0.0f, (float)screenW, (float)screenH },

            (Vector2){ 0, 0 }, 0.0f, WHITE);

            

        // ---------------------------------------------------------------------

        // HORROR SCREEN FLASH & VIGNETTE (During Grethnar Jumpscare Glimpse)

        // ---------------------------------------------------------------------

        if (grethnarState == GRETHNAR_JUMPSCARE) {

            float flashAlpha = Clamp(grethnarJumpscareTimer / 0.45f, 0.0f, 1.0f) * 115.0f;

            DrawRectangle(0, 0, screenW, screenH, (Color){ 180, 0, 0, (unsigned char)flashAlpha });

            

            // Blood vignette borders framing screen during glimpse

            for (int b = 0; b < 24; b += 2) {

                unsigned char vigA = (unsigned char)(130 * (1.0f - (float)b / 24.0f) * (grethnarJumpscareTimer / 0.45f));

                DrawRectangleLines(b, b, screenW - b*2, screenH - b*2, (Color){ 120, 0, 0, vigA });

            }

        }

            

        // =========================================================================

        // QUIT GAME CONFIRMATION MODAL DIALOG (Screen Pass)
        // =========================================================================
        if (showQuitConfirm) {
            DrawRectangle(0, 0, screenW, screenH, (Color){ 0, 0, 0, 195 });

            float qbw = 500.0f, qbh = 220.0f;
            float qbx = ((float)screenW - qbw) * 0.5f;
            float qby = ((float)screenH - qbh) * 0.5f;

            DrawAAAPanel((Rectangle){ qbx, qby, qbw, qbh }, (Color){ 14, 16, 20, 252 }, (Color){ 215, 55, 55, 230 }, 8.0f, true);

            DrawTextSharpCentered(g_fontHeadSub, "QUIT GAME?", (float)screenW * 0.5f, qby + 28.0f, 26.0f, (Color){ 255, 225, 225, 255 });
            DrawTextSharpCentered(g_fontBody, "Are you sure you want to exit to desktop?", (float)screenW * 0.5f, qby + 68.0f, 15.0f, (Color){ 185, 190, 195, 240 });

            Vector2 mPos = GetMousePosition();
            Rectangle btnQuitRec   = { qbx + 35.0f, qby + 130.0f, 125.0f, 46.0f };
            Rectangle btnMenuRec   = { qbx + qbw * 0.5f - 70.0f, qby + 130.0f, 140.0f, 46.0f };
            Rectangle btnResumeRec = { qbx + qbw - 160.0f, qby + 130.0f, 125.0f, 46.0f };

            bool hoverQuit   = CheckCollisionPointRec(mPos, btnQuitRec);
            bool hoverMenu   = CheckCollisionPointRec(mPos, btnMenuRec);
            bool hoverResume = CheckCollisionPointRec(mPos, btnResumeRec);

            // Button [QUIT]
            DrawAAAPanel(btnQuitRec, hoverQuit ? (Color){ 195, 40, 40, 255 } : (Color){ 135, 28, 28, 230 }, hoverQuit ? WHITE : (Color){ 245, 80, 80, 255 }, 5.0f, false);
            DrawTextSharpCentered(g_fontMenu, "QUIT", btnQuitRec.x + btnQuitRec.width * 0.5f, btnQuitRec.y + 14.0f, 16.0f, WHITE);

            // Button [MAIN MENU]
            DrawAAAPanel(btnMenuRec, hoverMenu ? (Color){ 55, 85, 125, 255 } : (Color){ 32, 50, 75, 230 }, hoverMenu ? WHITE : (Color){ 95, 155, 235, 255 }, 5.0f, false);
            DrawTextSharpCentered(g_fontMenu, "MAIN MENU", btnMenuRec.x + btnMenuRec.width * 0.5f, btnMenuRec.y + 14.0f, 15.0f, WHITE);

            // Button [RESUME]
            DrawAAAPanel(btnResumeRec, hoverResume ? (Color){ 45, 125, 65, 255 } : (Color){ 28, 75, 42, 230 }, hoverResume ? WHITE : (Color){ 80, 195, 105, 255 }, 5.0f, false);
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
            Vector2 mDelta = GetMouseDelta();
            float mouseMoveDist = sqrtf(mDelta.x * mDelta.x + mDelta.y * mDelta.y);
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
            DrawRectangleGradientH(0, 0, leftShadW, screenH, (Color){ 3, 4, 6, 210 }, BLANK);

            // Subtle Ground Shadow for Bottom Telemetry & Vista Selectors
            DrawRectangleGradientV(0, screenH - 60, screenW, 60, BLANK, (Color){ 2, 3, 5, 175 });

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
                DrawRectangle(0, 0, screenW, screenH, (Color){ 200, 220, 250, (unsigned char)(s_menuFlashAlpha * 95.0f) });
            }

            // Darkness veil when dormant
            float slumberAlpha = (1.0f - g_menuAwakeIntensity) * 235.0f;
            if (slumberAlpha > 2.0f) {
                DrawRectangle(0, 0, screenW, screenH, (Color){ 2, 3, 5, (unsigned char)slumberAlpha });
            }

            // Volumetric Flashlight Radial Beam (multi-tier luminous cone)
            DrawCircleGradient((int)g_menuLightPos.x, (int)g_menuLightPos.y, 480.0f, (Color){ 210, 195, 160, (unsigned char)(22 * g_menuAwakeIntensity) }, (Color){ 0, 0, 0, 0 });
            DrawCircleGradient((int)g_menuLightPos.x, (int)g_menuLightPos.y, 250.0f, (Color){ 235, 215, 180, (unsigned char)(40 * g_menuAwakeIntensity) }, (Color){ 0, 0, 0, 0 });
            DrawCircleGradient((int)g_menuLightPos.x, (int)g_menuLightPos.y, 95.0f,  (Color){ 255, 245, 220, (unsigned char)(72 * g_menuAwakeIntensity) }, (Color){ 0, 0, 0, 0 });

            // Floating dust motes catching the flashlight beam
            for (int d = 0; d < 22; d++) {
                float seed = (float)d * 137.5f;
                float dx = fmodf(seed * 43.0f + timeVal * 16.0f * (1.0f + fmodf(seed, 0.4f)), (float)screenW);
                float dy = fmodf(seed * 67.0f + sinf(timeVal * 0.7f + seed) * 35.0f, (float)screenH);
                float dDist = Vector2Distance((Vector2){ dx, dy }, g_menuLightPos);
                if (dDist < 250.0f) {
                    float alpha = (1.0f - dDist / 250.0f) * 190.0f * g_menuAwakeIntensity;
                    DrawCircle((int)dx, (int)dy, 1.2f + fmodf(seed, 1.8f), (Color){ 255, 235, 195, (unsigned char)alpha });
                }
            }

            // Feather-light CRT scanlines for analog texture
            for (int y = 0; y < screenH; y += 4) {
                DrawLine(0, y, screenW, y, (Color){ 0, 0, 0, 16 });
            }

            // CCTV Camera Switch Glitch Overlay (video scanline flutter)
            if (g_cctvSwitchGlitch > 0.0f) {
                g_cctvSwitchGlitch -= dt;
                for (int n = 0; n < 8; n++) {
                    int ly = GetRandomValue(10, screenH - 10);
                    DrawLine(0, ly, screenW, ly, (Color){ 200, 215, 235, (unsigned char)GetRandomValue(35, 80) });
                }
            }

            // Keyboard and Mouse Wheel cycling for CCTV Cameras
            if (!g_showSettingsModal && !g_showCaseFilesModal && !g_showSurvivalModal && !g_isMenuStartingGame) {
                int camDelta = 0;
                if (IsKeyPressed(KEY_Q) || wheelMove < -0.2f) camDelta = 2; // (current + 2) % 3 is previous
                if (IsKeyPressed(KEY_E) || wheelMove > 0.2f)  camDelta = 1; // (current + 1) % 3 is next
                if (camDelta != 0) {
                    g_menuCCTVFeed = (g_menuCCTVFeed + camDelta) % 3;
                    PlaySound(g_sndRadioStatic);
                    g_cctvSwitchGlitch = 0.16f;
                }
            }

            // Minimalist Floating Camera Switch Tabs at Screen Bottom
            DrawTextSharp(g_fontSmall, "[ Q / E ]  Vistas:", 45, screenH - 32, 13.0f, (Color){ 150, 155, 165, 190 }, 1.2f);
            const char* camTabs[3] = { "Portico Arch", "Courtyard", "Vestibule" };
            int tabSpacing = 145;
            int tabStartX  = screenW - 470;
            for (int c = 0; c < 3; c++) {
                int tx = tabStartX + c * tabSpacing;
                int ty = screenH - 32;
                Rectangle tabHit = { (float)(tx - 10), (float)(screenH - 42), 130.0f, 32.0f };
                bool tHover = CheckCollisionPointRec(mPos, tabHit);
                bool tActive = (g_menuCCTVFeed == c);

                Color tabCol = tActive ? WHITE : (tHover ? (Color){ 245, 130, 120, 255 } : (Color){ 150, 145, 140, 190 });
                DrawTextSharp(g_fontSmall, camTabs[c], tx, ty, 13.0f, tabCol, 1.2f);
                if (tActive) {
                    int tLen = (int)MeasureTextSharp(g_fontSmall, camTabs[c], 13.0f, 1.2f);
                    DrawLine(tx, ty + 18, tx + tLen, ty + 18, (Color){ 235, 45, 35, 255 });
                }

                if (tHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_menuCCTVFeed != c) {
                    g_menuCCTVFeed = c;
                    PlaySound(g_sndRadioStatic);
                    g_cctvSwitchGlitch = 0.16f;
                }
            }




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



                DrawRectangle(0, 0, screenW, screenH, (Color){ 0, 0, 0, 215 });

                DrawRectangle(dx, dy, dw, dh, (Color){ 10, 11, 14, 252 });

                DrawRectangleLines(dx, dy, dw, dh, (Color){ 140, 35, 25, 255 });



                // Header (Distinct Gothic Horror Heading Font)

                DrawTextSharp(g_fontHeadSub, "STATE POLICE // CLASSIFIED EVIDENCE DOSSIER", dx + 28, dy + 18, 25.0f, (Color){ 235, 225, 215, 255 });

                DrawTextSharp(g_fontSmall, "CASE #89-094 // ROUTE 9 SERVICE STATION & BORDER MIRE", dx + 28, dy + 48, 14.0f, (Color){ 180, 60, 50, 240 });

                DrawLine(dx + 25, dy + 70, dx + dw - 25, dy + 70, (Color){ 90, 30, 25, 220 });



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



                    DrawRectangleRec(fRec, fActive ? (Color){ 65, 26, 22, 255 } : (fHover ? (Color){ 30, 22, 20, 220 } : (Color){ 16, 17, 21, 210 }));

                    DrawRectangleLinesEx(fRec, 1.0f, fActive ? (Color){ 230, 65, 55, 255 } : (fHover ? WHITE : (Color){ 75, 50, 45, 190 }));

                    DrawTextSharp(g_fontMenu, caseTitles[f], (int)fRec.x + 14, (int)fRec.y + 12, 16.0f, fActive ? WHITE : (Color){ 195, 190, 180, 230 });



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



                DrawRectangle(docX, docY, docW, docH, (Color){ 14, 16, 20, 255 });

                DrawRectangleLines(docX, docY, docW, docH, (Color){ 55, 42, 38, 220 });



                // Content based on selected case file (Strong Heading Font for title, Large Legible Body for text)

                if (g_caseFileSelected == 0) {

                    DrawTextSharp(g_fontHeadSub, "TRANSCRIPT: 911 LOG // CALL REC 22:14:08", docX + 20, docY + 16, 19.0f, (Color){ 235, 70, 60, 255 });

                    DrawTextSharp(g_fontSmall, "LOCATION: Mile Marker 14, Route 9 Northern Pass", docX + 20, docY + 42, 14.0f, (Color){ 160, 155, 150, 220 });

                    DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, (Color){ 70, 35, 30, 220 });



                    const char* l1 = "\"Patrol, our vehicle radiator blew near the abandoned";

                    const char* l2 = "gas stop. It's pouring rain. There are no lights out here";

                    const char* l3 = "except the pumps. My sister says she heard clicking sounds";

                    const char* l4 = "in the drainage ditch... Wait, something is watching us";

                    const char* l5 = "from the spruce tree line. Send someone out here now--\"";

                    const char* l6 = "[TRANSMISSION CUT - SIGNAL LOST // 00:01:24 RECORDED]";



                    DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l4, docX + 20, docY + 160, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l5, docX + 20, docY + 188, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l6, docX + 20, docY + 228, 17.0f, (Color){ 235, 60, 50, 255 });

                } else if (g_caseFileSelected == 1) {

                    DrawTextSharp(g_fontHeadSub, "INCIDENT LOG: MISSING PERSON // SIBLING DOSSIER", docX + 20, docY + 16, 19.0f, (Color){ 235, 70, 60, 255 });

                    DrawTextSharp(g_fontSmall, "STATUS: Unresolved / Active Search Warrant", docX + 20, docY + 42, 14.0f, (Color){ 160, 155, 150, 220 });

                    DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, (Color){ 70, 35, 30, 220 });



                    const char* l1 = "When state troopers inspected the stalled sedan at dawn,";

                    const char* l2 = "the driver's door was swung open into the mud.";

                    const char* l3 = "The passenger side was empty. A pair of footprints led";

                    const char* l4 = "from the road toward the mire. The footprints stopped";

                    const char* l5 = "abruptly 40 feet into the dark mud with no return trail.";

                    const char* l6 = "Only deep claw indentations were pressed into the peat.";



                    DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l4, docX + 20, docY + 160, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l5, docX + 20, docY + 188, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l6, docX + 20, docY + 224, 17.0f, (Color){ 215, 210, 200, 245 });

                } else if (g_caseFileSelected == 2) {

                    DrawTextSharp(g_fontHeadSub, "ANOMALOUS ENTITY // CLASSIFICATION: SKELETON HOUND", docX + 20, docY + 16, 19.0f, (Color){ 235, 70, 60, 255 });

                    DrawTextSharp(g_fontSmall, "OBSERVER: Station Attendant Security Cam #02", docX + 20, docY + 42, 14.0f, (Color){ 160, 155, 150, 220 });

                    DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, (Color){ 70, 35, 30, 220 });



                    const char* l1 = "Entity displays the anatomy of a massive canine, but with";

                    const char* l2 = "externalized skeletal structure and exposed vertebral ribs.";

                    const char* l3 = "Exhibits luminescence in ocular cavities when in shadows.";

                    const char* l4 = "Does not consume flesh conventionally; appears drawn to";

                    const char* l5 = "sub-surface mineral deposits and fresh blood pooling";

                    const char* l6 = "around the store's cold storage meat hook.";



                    DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l4, docX + 20, docY + 160, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l5, docX + 20, docY + 188, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l6, docX + 20, docY + 224, 17.0f, (Color){ 215, 210, 200, 245 });

                } else {

                    DrawTextSharp(g_fontHeadSub, "FORENSIC GEOLOGY // ANOMALOUS MIRE PRESERVATION", docX + 20, docY + 16, 19.0f, (Color){ 235, 70, 60, 255 });

                    DrawTextSharp(g_fontSmall, "SAMPLE ANALYSIS: Route 9 Bog Core 12-F", docX + 20, docY + 42, 14.0f, (Color){ 160, 155, 150, 220 });

                    DrawLine(docX + 20, docY + 62, docX + docW - 20, docY + 62, (Color){ 70, 35, 30, 220 });



                    const char* l1 = "Core drilling 8 meters into the mire revealed biological";

                    const char* l2 = "specimens buried decades ago with zero cellular decay.";

                    const char* l3 = "Tissues retain hydration and microscopic muscle twitching.";

                    const char* l4 = "Local saying carved into the gas station counter:";

                    const char* l5 = "\"WHAT THE GROUND KEEPS, IT NEVER RELEASES.\"";

                    const char* l6 = "Excavation without proper protective tools is lethal.";



                    DrawTextSharp(g_fontBody, l1, docX + 20, docY + 76, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l2, docX + 20, docY + 104, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l3, docX + 20, docY + 132, 17.0f, (Color){ 215, 210, 200, 245 });

                    DrawTextSharp(g_fontBody, l4, docX + 20, docY + 162, 15.0f, (Color){ 175, 170, 165, 210 });

                    DrawTextSharp(g_fontHeadSub, l5, docX + 20, docY + 186, 17.0f, (Color){ 240, 60, 50, 255 });

                    DrawTextSharp(g_fontBody, l6, docX + 20, docY + 224, 15.0f, (Color){ 195, 190, 180, 230 });

                }



                // Close Button

                Rectangle btnCloseDossier = { (float)(dx + dw/2 - 85), (float)(dy + dh - 48), 170.0f, 36.0f };

                bool hClose = CheckCollisionPointRec(mPos, btnCloseDossier);

                DrawRectangleRec(btnCloseDossier, hClose ? (Color){ 95, 30, 25, 255 } : (Color){ 35, 18, 16, 240 });

                DrawRectangleLinesEx(btnCloseDossier, 1.0f, hClose ? WHITE : (Color){ 160, 50, 40, 255 });

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



                DrawRectangle(0, 0, screenW, screenH, (Color){ 0, 0, 0, 225 });

                DrawRectangle(dx, dy, dw, dh, (Color){ 12, 14, 18, 252 });

                DrawRectangleLines(dx, dy, dw, dh, (Color){ 175, 145, 65, 255 });



                DrawTextSharp(g_fontHeadSub, "TOP SECRET // CIVIL DEFENSE & DEEP MONITORING DOSSIER", dx + 28, dy + 22, 17.0f, (Color){ 240, 220, 180, 255 });

                DrawTextSharp(g_fontSmall, "LOCATION: ROUTE 9 SERVICE STATION SUB-TERRAIN // AUTH: LEVEL-4 DISPATCH", dx + 28, dy + 46, 12.0f, (Color){ 200, 75, 60, 255 });

                DrawLine(dx + 25, dy + 68, dx + dw - 25, dy + 68, (Color){ 120, 95, 45, 200 });



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



                    DrawRectangleRec(tRec, tActive ? (Color){ 55, 42, 22, 255 } : (tHover ? (Color){ 28, 24, 18, 220 } : (Color){ 16, 17, 20, 200 }));

                    DrawRectangleLinesEx(tRec, 1.0f, tActive ? (Color){ 225, 180, 70, 255 } : (tHover ? WHITE : (Color){ 80, 65, 45, 180 }));

                    DrawTextSharp(g_fontBody, dTitles[t], (int)tRec.x + 12, (int)tRec.y + 13, 13.0f, tActive ? WHITE : (Color){ 200, 190, 175, 220 });



                    if (tHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_dossierFileSelected != t) {

                        g_dossierFileSelected = t;

                        PlaySound(g_sndMenuNav);

                    }

                }



                int docX = dx + 275, docY = dy + 82, docW = dw - 300, docH = dh - 150;

                DrawRectangle(docX, docY, docW, docH, (Color){ 16, 18, 22, 255 });

                DrawRectangleLines(docX, docY, docW, docH, (Color){ 65, 55, 42, 220 });



                if (g_dossierFileSelected == 0) {

                    DrawTextSharp(g_fontMenu, "OPERATION SUB-STRATA: COLD WAR EXCAVATION LOG", docX + 20, docY + 16, 14.0f, (Color){ 230, 180, 70, 255 });

                    DrawTextSharp(g_fontSmall, "ARCHIVE: Station Foundation Survey (August 1984)", docX + 20, docY + 38, 11.0f, (Color){ 160, 155, 145, 220 });

                    DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, (Color){ 80, 65, 40, 200 });

                    DrawTextSharp(g_fontBody, "The secret underground corridor was initially excavated", docX + 20, docY + 68, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "during the Cold War as a civilian fallout monitor bunker.", docX + 20, docY + 90, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "However, miners struck hollow fissures 18 feet below.", docX + 20, docY + 112, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "The limestone walls bore massive parallel scrape furrows", docX + 20, docY + 134, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "resembling claw paths. Work was halted indefinitely.", docX + 20, docY + 156, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "Access was sealed beneath the exterior worn rug.", docX + 20, docY + 188, 13.0f, (Color){ 200, 150, 60, 255 });

                } else if (g_dossierFileSelected == 1) {

                    DrawTextSharp(g_fontMenu, "ANOMALOUS SPECIMEN 07-B: FORMALIN SUSPENSION", docX + 20, docY + 16, 14.0f, (Color){ 230, 180, 70, 255 });

                    DrawTextSharp(g_fontSmall, "CONTAINMENT: Hermetic Glass Jar on Chamber Worktable", docX + 20, docY + 38, 11.0f, (Color){ 160, 155, 145, 220 });

                    DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, (Color){ 80, 65, 40, 200 });

                    DrawTextSharp(g_fontBody, "A severed juvenile forelimb was recovered from the bog", docX + 20, docY + 68, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "culvert and preserved in formalin. The tissue exhibits", docX + 20, docY + 90, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "external calcified plating and bioluminescent nodes.", docX + 20, docY + 112, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "Microscopic twitching persists even when submerged.", docX + 20, docY + 134, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "Do not break the glass under any circumstance.", docX + 20, docY + 164, 13.0f, (Color){ 220, 50, 45, 255 });

                } else if (g_dossierFileSelected == 2) {

                    DrawTextSharp(g_fontMenu, "COLD STORAGE INTERFACE: OVERHEAD VENTILATION GRATE", docX + 20, docY + 16, 14.0f, (Color){ 230, 180, 70, 255 });

                    DrawTextSharp(g_fontSmall, "CORRELATION: Supermarket Meat Room & Underground Pit", docX + 20, docY + 38, 11.0f, (Color){ 160, 155, 145, 220 });

                    DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, (Color){ 80, 65, 40, 200 });

                    DrawTextSharp(g_fontBody, "The overhead ceiling grate connects directly to the floor", docX + 20, docY + 68, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "of the superstore's cold-storage walk-in meat locker.", docX + 20, docY + 90, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "Condensed brine and blood drip into the floor pail.", docX + 20, docY + 112, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "The scent carries deep into subterranean strata.", docX + 20, docY + 134, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "It acts as a scent lure. They gather beneath us at night.", docX + 20, docY + 164, 13.0f, (Color){ 225, 45, 40, 255 });

                } else {

                    DrawTextSharp(g_fontMenu, "EMERGENCY SUPPLY CACHE: COMBINATION CODE RECORD", docX + 20, docY + 16, 14.0f, (Color){ 230, 180, 70, 255 });

                    DrawTextSharp(g_fontSmall, "CONTAINER: Heavy Cast-Iron Padlocked Crate in Corner", docX + 20, docY + 38, 11.0f, (Color){ 160, 155, 145, 220 });

                    DrawLine(docX + 20, docY + 54, docX + docW - 20, docY + 54, (Color){ 80, 65, 40, 200 });

                    DrawTextSharp(g_fontBody, "Under the exterior carpet lies a deep mining descent", docX + 20, docY + 68, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "leading to the forgotten 19th-century abandoned village.", docX + 20, docY + 90, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontBody, "The midsection is blocked by a massive cave-in.", docX + 20, docY + 112, 13.0f, (Color){ 215, 210, 200, 240 });

                    DrawTextSharp(g_fontTitle, "[ TRENCH SHOVEL REQUIRED ]", docX + 20, docY + 138, 17.0f, (Color){ 245, 195, 60, 255 });

                    DrawTextSharp(g_fontBody, "Recover the shovel from the supermarket shelves to dig through.", docX + 20, docY + 185, 13.0f, (Color){ 200, 195, 180, 240 });

                }



                Rectangle btnCloseDossier = { (float)(dx + dw/2 - 80), (float)(dy + dh - 48), 160.0f, 32.0f };

                bool hClose = CheckCollisionPointRec(mPos, btnCloseDossier);

                DrawRectangleRec(btnCloseDossier, hClose ? (Color){ 95, 35, 25, 255 } : (Color){ 36, 22, 18, 240 });

                DrawRectangleLinesEx(btnCloseDossier, 1.0f, hClose ? WHITE : (Color){ 175, 140, 65, 255 });

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



                DrawRectangle(0, 0, screenW, screenH, (Color){ 0, 0, 0, 215 });

                DrawRectangle(sx, sy, sw, sh, (Color){ 10, 11, 14, 252 });

                DrawRectangleLines(sx, sy, sw, sh, (Color){ 140, 35, 25, 255 });



                DrawTextSharpCentered(g_fontHeadSub, "S U R V I V A L   R E C O R D", sx + sw/2, sy + 18, 26.0f, (Color){ 245, 235, 225, 255 }, 2.0f);

                DrawLine(sx + 35, sy + 50, sx + sw - 35, sy + 50, (Color){ 90, 30, 25, 220 });



                DrawTextSharp(g_fontSmall, "ROUTE 9 SERVICE STATION // ATTENDANT SHIFT DOSSIER", sx + 45, sy + 66, 14.0f, (Color){ 190, 65, 55, 240 });



                DrawTextSharp(g_fontBody, "SURVEILLANCE ENGINE: ROCK-SOLID 144+ FPS ACTIVE", sx + 45, sy + 98, 16.0f, (Color){ 215, 210, 200, 245 });

                DrawTextSharp(g_fontBody, "LOCATION: 44.9184° N, 71.3820° W (MILE 14)", sx + 45, sy + 126, 16.0f, (Color){ 195, 190, 185, 230 });

                DrawTextSharp(g_fontBody, "WEATHER TELEMETRY: NIGHT TIME PRECIPITATION (TORRENTIAL)", sx + 45, sy + 154, 16.0f, (Color){ 195, 190, 185, 230 });

                DrawTextSharp(g_fontBody, "ANOMALY THREAT LEVEL: HIGH (NOCTURNAL ENTITY ACTIVE)", sx + 45, sy + 182, 16.0f, (Color){ 240, 60, 50, 255 });



                DrawLine(sx + 35, sy + 216, sx + sw - 35, sy + 216, (Color){ 70, 25, 22, 190 });



                DrawTextSharp(g_fontHeadSub, "INVESTIGATION MILESTONES:", sx + 45, sy + 232, 19.0f, (Color){ 235, 225, 215, 255 });

                DrawTextSharp(g_fontBody, "[+] STALLED SEDAN LOCATED ON HIGHWAY", sx + 55, sy + 262, 16.0f, (Color){ 150, 215, 160, 245 });

                DrawTextSharp(g_fontBody, "[+] GRETHNAR'S 24/7 STATION ACCESSED", sx + 55, sy + 290, 16.0f, (Color){ 150, 215, 160, 245 });

                DrawTextSharp(g_fontBody, "[+] ROOF SURVEILLANCE OPTICS CALIBRATED", sx + 55, sy + 318, 16.0f, (Color){ 150, 215, 160, 245 });

                DrawTextSharp(g_fontBody, "[!] MEAT LOCKER BLOOD ANOMALY UNRESOLVED", sx + 55, sy + 346, 16.0f, (Color){ 245, 75, 65, 255 });



                // Back Button

                Rectangle btnBackSurv = { (float)(sx + sw/2 - 85), (float)(sy + sh - 48), 170.0f, 36.0f };

                bool hBackS = CheckCollisionPointRec(mPos, btnBackSurv);

                DrawRectangleRec(btnBackSurv, hBackS ? (Color){ 95, 30, 25, 255 } : (Color){ 35, 18, 16, 240 });

                DrawRectangleLinesEx(btnBackSurv, 1.0f, hBackS ? WHITE : (Color){ 160, 50, 40, 255 });

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



                DrawRectangle(0, 0, screenW, screenH, (Color){ 0, 0, 0, 200 });

                DrawRectangle(sx, sy, sw, sh, (Color){ 8, 10, 12, 252 });

                DrawRectangleLines(sx, sy, sw, sh, (Color){ 140, 35, 25, 255 });



                DrawTextSharpCentered(g_fontHeadSub, "O P T I O N S", sx + sw/2, sy + 18, 26.0f, (Color){ 245, 235, 225, 255 }, 2.0f);

                DrawLine(sx + 35, sy + 48, sx + sw - 35, sy + 48, (Color){ 90, 30, 25, 220 });



                // 1. Audio Volume

                DrawTextSharp(g_fontBody, "MASTER AUDIO", sx + 45, sy + 60, 16.0f, (Color){ 215, 210, 205, 255 });

                Rectangle volTrack = { (float)(sx + 45), (float)(sy + 84), 280.0f, 14.0f };

                DrawRectangleRec(volTrack, (Color){ 20, 22, 25, 255 });

                DrawRectangleLinesEx(volTrack, 1.0f, (Color){ 75, 38, 32, 240 });

                DrawRectangle((int)volTrack.x, (int)volTrack.y, (int)(volTrack.width * g_userMasterVolume), (int)volTrack.height, (Color){ 220, 55, 45, 255 });

                char volStr[32]; snprintf(volStr, sizeof(volStr), "%d%%", (int)(g_userMasterVolume * 100.0f));

                DrawTextSharp(g_fontBody, volStr, sx + 340, sy + 80, 16.0f, (Color){ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, (Rectangle){ volTrack.x - 10, volTrack.y - 6, volTrack.width + 20, volTrack.height + 16 })) {

                    g_userMasterVolume = Clamp((mPos.x - volTrack.x) / volTrack.width, 0.0f, 1.0f);

                    SetMasterVolume(g_userMasterVolume);

                }



                // 2. Mouse Sensitivity

                DrawTextSharp(g_fontBody, "MOUSE SENSITIVITY", sx + 45, sy + 110, 16.0f, (Color){ 215, 210, 205, 255 });

                Rectangle sensTrack = { (float)(sx + 45), (float)(sy + 134), 280.0f, 14.0f };

                DrawRectangleRec(sensTrack, (Color){ 20, 22, 25, 255 });

                DrawRectangleLinesEx(sensTrack, 1.0f, (Color){ 75, 38, 32, 240 });

                float sensNorm = Clamp((g_userMouseSensitivity - 0.5f) / 2.0f, 0.0f, 1.0f);

                DrawRectangle((int)sensTrack.x, (int)sensTrack.y, (int)(sensTrack.width * sensNorm), (int)sensTrack.height, (Color){ 220, 55, 45, 255 });

                char sensStr[32]; snprintf(sensStr, sizeof(sensStr), "%.1fx", g_userMouseSensitivity);

                DrawTextSharp(g_fontBody, sensStr, sx + 340, sy + 130, 16.0f, (Color){ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, (Rectangle){ sensTrack.x - 10, sensTrack.y - 6, sensTrack.width + 20, sensTrack.height + 16 })) {

                    g_userMouseSensitivity = 0.5f + Clamp((mPos.x - sensTrack.x) / sensTrack.width, 0.0f, 1.0f) * 2.0f;

                }



                // 3. FOV Slider

                DrawTextSharp(g_fontBody, "FIELD OF VIEW", sx + 45, sy + 160, 16.0f, (Color){ 215, 210, 205, 255 });

                Rectangle fovTrack = { (float)(sx + 45), (float)(sy + 184), 280.0f, 14.0f };

                DrawRectangleRec(fovTrack, (Color){ 20, 22, 25, 255 });

                DrawRectangleLinesEx(fovTrack, 1.0f, (Color){ 75, 38, 32, 240 });

                float fovNorm = Clamp((g_userFov - 50.0f) / 40.0f, 0.0f, 1.0f);

                DrawRectangle((int)fovTrack.x, (int)fovTrack.y, (int)(fovTrack.width * fovNorm), (int)fovTrack.height, (Color){ 220, 55, 45, 255 });

                char fovStr[32]; snprintf(fovStr, sizeof(fovStr), "%d°", (int)g_userFov);

                DrawTextSharp(g_fontBody, fovStr, sx + 340, sy + 180, 16.0f, (Color){ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, (Rectangle){ fovTrack.x - 10, fovTrack.y - 6, fovTrack.width + 20, fovTrack.height + 16 })) {

                    g_userFov = 50.0f + Clamp((mPos.x - fovTrack.x) / fovTrack.width, 0.0f, 1.0f) * 40.0f;

                }



                // Horror Gamma Calibration Box

                Rectangle calibBox = { (float)(sx + 420), (float)(sy + 60), 200.0f, 142.0f };

                DrawRectangleRec(calibBox, (Color){ 4, 5, 7, 255 });

                DrawRectangleLinesEx(calibBox, 1.0f, (Color){ 65, 40, 35, 210 });

                Color faceCol = { (unsigned char)(28 * g_userHorrorGamma), (unsigned char)(28 * g_userHorrorGamma), (unsigned char)(34 * g_userHorrorGamma), 255 };

                DrawCircle((int)calibBox.x + 100, (int)calibBox.y + 48, 26.0f, faceCol);

                DrawCircle((int)calibBox.x + 91, (int)calibBox.y + 43, 3.5f, (Color){ 0, 0, 0, 255 });

                DrawCircle((int)calibBox.x + 109, (int)calibBox.y + 43, 3.5f, (Color){ 0, 0, 0, 255 });

                DrawTextSharp(g_fontSmall, "CALIBRATION:", (int)calibBox.x + 14, (int)calibBox.y + 88, 13.0f, (Color){ 170, 165, 160, 230 });

                DrawTextSharp(g_fontSmall, "Adjust display so", (int)calibBox.x + 14, (int)calibBox.y + 104, 12.0f, (Color){ 140, 135, 130, 200 });

                DrawTextSharp(g_fontSmall, "entity is barely visible.", (int)calibBox.x + 14, (int)calibBox.y + 120, 12.0f, (Color){ 140, 135, 130, 200 });



                // Fullscreen Toggle

                Rectangle btnFs = { (float)(sx + 45), (float)(sy + 214), 250.0f, 32.0f };

                bool hFs = CheckCollisionPointRec(mPos, btnFs);

                DrawRectangleRec(btnFs, hFs ? (Color){ 70, 28, 22, 255 } : (Color){ 28, 20, 18, 240 });

                DrawRectangleLinesEx(btnFs, 1.0f, hFs ? (Color){ 230, 75, 55, 255 } : (Color){ 110, 45, 35, 220 });

                DrawTextSharp(g_fontBody, IsWindowFullscreen() ? "[ F11 ] FULLSCREEN: ON" : "[ F11 ] FULLSCREEN: OFF", sx + 55, sy + 221, 15.0f, WHITE);

                if (hFs && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    ToggleGameFullscreen();

                }



                // Divider

                DrawLine(sx + 35, sy + 258, sx + sw - 35, sy + 258, (Color){ 70, 25, 22, 190 });



                // Keybindings Cheatsheet

                DrawTextSharp(g_fontHeadSub, "C O N T R O L S", sx + 45, sy + 270, 17.0f, (Color){ 215, 90, 80, 255 });

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

                    DrawTextSharp(g_fontBody, ctrlList[c], sx + 55, cy, 14.0f, (Color){ 180, 175, 170, 240 });

                }



                // Back Button

                Rectangle btnBack = { (float)(sx + sw/2 - 85), (float)(sy + sh - 46), 170.0f, 34.0f };

                bool hBack = CheckCollisionPointRec(mPos, btnBack);

                DrawRectangleRec(btnBack, hBack ? (Color){ 95, 30, 25, 255 } : (Color){ 35, 18, 16, 240 });

                DrawRectangleLinesEx(btnBack, 1.0f, hBack ? WHITE : (Color){ 160, 50, 40, 255 });

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
                    float titleDist = Vector2Distance((Vector2){ (float)menuX, (float)titleY }, g_menuLightPos);
                    float titleGleam = Clamp(1.0f - titleDist / 400.0f, 0.0f, 1.0f) * g_menuAwakeIntensity;

                    const char* mainTitle = "WHAT THE GROUND KEEPS";
                    Color shadowCol = { 135, 20, 16, (unsigned char)(210 * menuDormancyAlpha) };
                    Color textCol   = { (unsigned char)(238 + (int)(17 * titleGleam)), (unsigned char)(232 + (int)(23 * titleGleam)), (unsigned char)(224 + (int)(23 * titleGleam)), (unsigned char)(255 * menuDormancyAlpha) };

                    DrawTextSharp(g_fontTitle, mainTitle, menuX + 2 + jx, titleY + 2 + jy, 46.0f, shadowCol, 2.5f);
                    DrawTextSharp(g_fontTitle, mainTitle, menuX + jx, titleY + jy, 46.0f, textCol, 2.5f);

                    // Delicate crimson accent line & poetic atmospheric tagline
                    DrawLine(menuX, titleY + 54, menuX + 360, titleY + 54, (Color){ 180, 40, 32, (unsigned char)(200 * menuDormancyAlpha) });
                    DrawTextSharp(g_fontHeadSub, "Some graves were never meant to be opened.", menuX, titleY + 64, 16.0f, (Color){ 190, 75, 65, (unsigned char)(210 * menuDormancyAlpha) }, 1.2f);

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

                    Vector2 mDelta = GetMouseDelta();
                    bool mouseMoved = (fabsf(mDelta.x) > 0.2f || fabsf(mDelta.y) > 0.2f);

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
                        Color labelCol = isSelected ? (Color){ 255, 250, 242, (unsigned char)(255 * menuDormancyAlpha) } : (Color){ 150, 155, 165, (unsigned char)(190 * menuDormancyAlpha) };
                        if (isSelected) {
                            // Vertical blood-crimson needle
                            DrawRectangle(menuX - 14 + (int)slideX, itemY + 10, 3, 24, (Color){ 235, 45, 35, (unsigned char)(255 * menuDormancyAlpha) });
                            // Crimson pointer caret >
                            DrawTextSharp(g_fontTitle, ">", menuX - 4 + (int)slideX, itemY + 6, 28.0f, (Color){ 235, 45, 35, (unsigned char)(255 * menuDormancyAlpha) });
                            // Subtle red drop shadow for selected item
                            DrawTextSharp(g_fontTitle, kMenuLabels[i], menuX + 18 + (int)slideX, itemY + 2, 36.0f, (Color){ 160, 30, 25, (unsigned char)(170 * menuDormancyAlpha) }, 1.5f);
                            // Underline trace in blood-crimson
                            DrawLine(menuX + 16 + (int)slideX, itemY + 40, menuX + 16 + (int)slideX + (int)textLen, itemY + 40, (Color){ 235, 45, 35, (unsigned char)(180 * menuDormancyAlpha) });
                            
                            // Add some eerie randomized symbols that jitter on hover
                            if (!g_isMenuStartingGame && GetRandomValue(0, 100) > 90) {
                                const char* glitched[] = { "+", "x", "-", "|", ".", ":" };
                                DrawTextSharp(g_fontSmall, glitched[GetRandomValue(0, 5)], menuX + 20 + (int)slideX + textLen + GetRandomValue(-4, 4), itemY + 12 + GetRandomValue(-4, 4), 16.0f, (Color){ 220, 50, 40, (unsigned char)(120 * menuDormancyAlpha) });
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

                            RunIntroCinematic();

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

            DrawRectangle(0, 0, screenW, screenH, (Color){ 6, 8, 12, 215 });



            // Feather-light CRT scanlines

            for (int y = 0; y < screenH; y += 4) {

                DrawLine(0, y, screenW, y, (Color){ 0, 0, 0, 24 });

            }



            // Heavy horror vignette

            int vH = screenH / 3;

            int vW = screenW / 4;

            DrawRectangleGradientV(0, 0, screenW, vH, (Color){ 0, 0, 0, 240 }, BLANK);

            DrawRectangleGradientV(0, screenH - vH, screenW, vH, BLANK, (Color){ 0, 0, 0, 255 });

            DrawRectangleGradientH(0, 0, vW, screenH, (Color){ 0, 0, 0, 230 }, BLANK);

            DrawRectangleGradientH(screenW - vW, 0, vW, screenH, BLANK, (Color){ 0, 0, 0, 230 });



            // If submodals are open inside pause menu, render them

            if (g_showSettingsModal) {

                int sw = 660, sh = 520;

                int sx = screenW/2 - sw/2, sy = screenH/2 - sh/2;



                DrawRectangle(0, 0, screenW, screenH, (Color){ 0, 0, 0, 200 });

                DrawRectangle(sx, sy, sw, sh, (Color){ 8, 10, 12, 252 });

                DrawRectangleLines(sx, sy, sw, sh, (Color){ 140, 35, 25, 255 });



                DrawTextSharpCentered(g_fontHeadSub, "O P T I O N S", sx + sw/2, sy + 18, 26.0f, (Color){ 245, 235, 225, 255 }, 2.0f);

                DrawLine(sx + 35, sy + 48, sx + sw - 35, sy + 48, (Color){ 90, 30, 25, 220 });



                // 1. Audio Volume

                DrawTextSharp(g_fontBody, "MASTER AUDIO", sx + 45, sy + 60, 16.0f, (Color){ 215, 210, 205, 255 });

                Rectangle volTrack = { (float)(sx + 45), (float)(sy + 84), 280.0f, 14.0f };

                DrawRectangleRec(volTrack, (Color){ 20, 22, 25, 255 });

                DrawRectangleLinesEx(volTrack, 1.0f, (Color){ 75, 38, 32, 240 });

                DrawRectangle((int)volTrack.x, (int)volTrack.y, (int)(volTrack.width * g_userMasterVolume), (int)volTrack.height, (Color){ 220, 55, 45, 255 });

                char volStr[32]; snprintf(volStr, sizeof(volStr), "%d%%", (int)(g_userMasterVolume * 100.0f));

                DrawTextSharp(g_fontBody, volStr, sx + 340, sy + 80, 16.0f, (Color){ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, (Rectangle){ volTrack.x - 10, volTrack.y - 6, volTrack.width + 20, volTrack.height + 16 })) {

                    g_userMasterVolume = Clamp((mPos.x - volTrack.x) / volTrack.width, 0.0f, 1.0f);

                    SetMasterVolume(g_userMasterVolume);

                }



                // 2. Mouse Sensitivity

                DrawTextSharp(g_fontBody, "MOUSE SENSITIVITY", sx + 45, sy + 110, 16.0f, (Color){ 215, 210, 205, 255 });

                Rectangle sensTrack = { (float)(sx + 45), (float)(sy + 134), 280.0f, 14.0f };

                DrawRectangleRec(sensTrack, (Color){ 20, 22, 25, 255 });

                DrawRectangleLinesEx(sensTrack, 1.0f, (Color){ 75, 38, 32, 240 });

                float sensNorm = Clamp((g_userMouseSensitivity - 0.5f) / 2.0f, 0.0f, 1.0f);

                DrawRectangle((int)sensTrack.x, (int)sensTrack.y, (int)(sensTrack.width * sensNorm), (int)sensTrack.height, (Color){ 220, 55, 45, 255 });

                char sensStr[32]; snprintf(sensStr, sizeof(sensStr), "%.1fx", g_userMouseSensitivity);

                DrawTextSharp(g_fontBody, sensStr, sx + 340, sy + 130, 16.0f, (Color){ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, (Rectangle){ sensTrack.x - 10, sensTrack.y - 6, sensTrack.width + 20, sensTrack.height + 16 })) {

                    g_userMouseSensitivity = 0.5f + Clamp((mPos.x - sensTrack.x) / sensTrack.width, 0.0f, 1.0f) * 2.0f;

                }



                // 3. FOV Slider

                DrawTextSharp(g_fontBody, "FIELD OF VIEW", sx + 45, sy + 160, 16.0f, (Color){ 215, 210, 205, 255 });

                Rectangle fovTrack = { (float)(sx + 45), (float)(sy + 184), 280.0f, 14.0f };

                DrawRectangleRec(fovTrack, (Color){ 20, 22, 25, 255 });

                DrawRectangleLinesEx(fovTrack, 1.0f, (Color){ 75, 38, 32, 240 });

                float fovNorm = Clamp((g_userFov - 50.0f) / 40.0f, 0.0f, 1.0f);

                DrawRectangle((int)fovTrack.x, (int)fovTrack.y, (int)(fovTrack.width * fovNorm), (int)fovTrack.height, (Color){ 220, 55, 45, 255 });

                char fovStr[32]; snprintf(fovStr, sizeof(fovStr), "%d°", (int)g_userFov);

                DrawTextSharp(g_fontBody, fovStr, sx + 340, sy + 180, 16.0f, (Color){ 235, 230, 225, 255 });



                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mPos, (Rectangle){ fovTrack.x - 10, fovTrack.y - 6, fovTrack.width + 20, fovTrack.height + 16 })) {

                    g_userFov = 50.0f + Clamp((mPos.x - fovTrack.x) / fovTrack.width, 0.0f, 1.0f) * 40.0f;

                }



                // Fullscreen Toggle

                Rectangle btnFs = { (float)(sx + 45), (float)(sy + 214), 250.0f, 32.0f };

                bool hFs = CheckCollisionPointRec(mPos, btnFs);

                DrawRectangleRec(btnFs, hFs ? (Color){ 70, 28, 22, 255 } : (Color){ 28, 20, 18, 240 });

                DrawRectangleLinesEx(btnFs, 1.0f, hFs ? (Color){ 230, 75, 55, 255 } : (Color){ 110, 45, 35, 220 });

                DrawTextSharp(g_fontBody, IsWindowFullscreen() ? "[ F11 ] FULLSCREEN: ON" : "[ F11 ] FULLSCREEN: OFF", sx + 55, sy + 221, 15.0f, WHITE);

                if (hFs && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    ToggleGameFullscreen();

                }



                // Divider

                DrawLine(sx + 35, sy + 258, sx + sw - 35, sy + 258, (Color){ 70, 25, 22, 190 });



                // Keybindings Cheatsheet

                DrawTextSharp(g_fontHeadSub, "C O N T R O L S", sx + 45, sy + 270, 17.0f, (Color){ 215, 90, 80, 255 });

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

                    DrawTextSharp(g_fontBody, ctrlList[c], sx + 55, cy, 14.0f, (Color){ 180, 175, 170, 240 });

                }



                // Back Button

                Rectangle btnBack = { (float)(sx + sw/2 - 85), (float)(sy + sh - 46), 170.0f, 34.0f };

                bool hBack = CheckCollisionPointRec(mPos, btnBack);

                DrawRectangleRec(btnBack, hBack ? (Color){ 95, 30, 25, 255 } : (Color){ 35, 18, 16, 240 });

                DrawRectangleLinesEx(btnBack, 1.0f, hBack ? WHITE : (Color){ 160, 50, 40, 255 });

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



                DrawRectangle(0, 0, screenW, screenH, (Color){ 0, 0, 0, 215 });

                DrawRectangle(dx, dy, dw, dh, (Color){ 10, 11, 14, 252 });

                DrawRectangleLines(dx, dy, dw, dh, (Color){ 140, 35, 25, 255 });



                DrawTextSharp(g_fontMenu, "STATE POLICE DEPARTMENT // CLASSIFIED EVIDENCE DOSSIER", dx + 28, dy + 20, 16.0f, (Color){ 220, 215, 205, 255 });

                DrawTextSharp(g_fontSmall, "CASE #89-094 // ROUTE 9 SERVICE STATION & BORDER MIRE", dx + 28, dy + 42, 12.0f, (Color){ 160, 50, 45, 240 });

                DrawLine(dx + 25, dy + 62, dx + dw - 25, dy + 62, (Color){ 80, 25, 20, 200 });



                int listW = 210;

                const char* caseTitles[4] = { "01. DISPATCH TAPE", "02. SIBLING INCIDENT", "03. THE MIRE HOUND", "04. SUB-SURFACE SOIL" };

                for (int f = 0; f < 4; f++) {

                    Rectangle fRec = { (float)(dx + 25), (float)(dy + 76 + f * 52), (float)listW, 42.0f };

                    bool fHover = CheckCollisionPointRec(mPos, fRec);

                    bool fActive = (g_caseFileSelected == f);



                    DrawRectangleRec(fRec, fActive ? (Color){ 50, 22, 18, 255 } : (fHover ? (Color){ 24, 18, 16, 220 } : (Color){ 14, 15, 18, 200 }));

                    DrawRectangleLinesEx(fRec, 1.0f, fActive ? (Color){ 200, 50, 40, 255 } : (fHover ? WHITE : (Color){ 60, 40, 35, 180 }));

                    DrawTextSharp(g_fontBody, caseTitles[f], (int)fRec.x + 12, (int)fRec.y + 12, 13.0f, fActive ? WHITE : (Color){ 180, 175, 170, 220 });



                    if (fHover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_caseFileSelected != f) {

                        g_caseFileSelected = f;

                        PlaySound(g_sndMenuNav);

                    }

                }



                int docX = dx + 255, docY = dy + 76, docW = dw - 280, docH = dh - 140;

                DrawRectangle(docX, docY, docW, docH, (Color){ 14, 16, 20, 255 });

                DrawRectangleLines(docX, docY, docW, docH, (Color){ 45, 35, 32, 220 });



                if (g_caseFileSelected == 0) {

                    DrawTextSharp(g_fontMenu, "TRANSCRIPT: 911 LOG // CALL REC 22:14:08", docX + 18, docY + 16, 14.0f, (Color){ 220, 60, 50, 255 });

                    DrawTextSharp(g_fontSmall, "LOCATION: Mile Marker 14, Route 9 Northern Pass", docX + 18, docY + 38, 11.0f, (Color){ 150, 145, 140, 220 });

                    DrawLine(docX + 18, docY + 54, docX + docW - 18, docY + 54, (Color){ 55, 30, 25, 200 });

                    DrawTextSharp(g_fontBody, "\"Patrol, our vehicle radiator blew near the abandoned", docX + 18, docY + 68, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "gas stop. It's pouring rain. There are no lights out here", docX + 18, docY + 90, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "except the pumps. My sister says she heard clicking sounds", docX + 18, docY + 112, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "in the drainage ditch... Wait, something is watching us", docX + 18, docY + 134, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "from the spruce tree line. Send someone out here now--\"", docX + 18, docY + 156, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "[TRANSMISSION CUT - SIGNAL LOST // 00:01:24 RECORDED]", docX + 18, docY + 192, 13.0f, (Color){ 210, 45, 40, 255 });

                } else if (g_caseFileSelected == 1) {

                    DrawTextSharp(g_fontMenu, "INCIDENT LOG: MISSING PERSON // SIBLING DOSSIER", docX + 18, docY + 16, 14.0f, (Color){ 220, 60, 50, 255 });

                    DrawTextSharp(g_fontSmall, "STATUS: Unresolved / Active Search Warrant", docX + 18, docY + 38, 11.0f, (Color){ 150, 145, 140, 220 });

                    DrawLine(docX + 18, docY + 54, docX + docW - 18, docY + 54, (Color){ 55, 30, 25, 200 });

                    DrawTextSharp(g_fontBody, "When state troopers inspected the stalled sedan at dawn,", docX + 18, docY + 68, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "the driver's door was swung open into the mud.", docX + 18, docY + 90, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "The passenger side was empty. A pair of footprints led", docX + 18, docY + 112, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "from the road toward the mire. The footprints stopped", docX + 18, docY + 134, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "abruptly 40 feet into the dark mud with no return trail.", docX + 18, docY + 156, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Only deep claw indentations were pressed into the peat.", docX + 18, docY + 184, 13.0f, (Color){ 200, 195, 185, 240 });

                } else if (g_caseFileSelected == 2) {

                    DrawTextSharp(g_fontMenu, "ANOMALOUS ENTITY // CLASSIFICATION: SKELETON HOUND", docX + 18, docY + 16, 14.0f, (Color){ 220, 60, 50, 255 });

                    DrawTextSharp(g_fontSmall, "OBSERVER: Station Attendant Security Cam #02", docX + 18, docY + 38, 11.0f, (Color){ 150, 145, 140, 220 });

                    DrawLine(docX + 18, docY + 54, docX + docW - 18, docY + 54, (Color){ 55, 30, 25, 200 });

                    DrawTextSharp(g_fontBody, "Entity displays the anatomy of a massive canine, but with", docX + 18, docY + 68, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "externalized skeletal structure and exposed vertebral ribs.", docX + 18, docY + 90, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Exhibits luminescence in ocular cavities when in shadows.", docX + 18, docY + 112, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Does not consume flesh conventionally; appears drawn to", docX + 18, docY + 134, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "sub-surface mineral deposits and fresh blood pooling", docX + 18, docY + 156, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "around the store's cold storage meat hook.", docX + 18, docY + 184, 13.0f, (Color){ 200, 195, 185, 240 });

                } else {

                    DrawTextSharp(g_fontMenu, "FORENSIC GEOLOGY // ANOMALOUS MIRE PRESERVATION", docX + 18, docY + 16, 14.0f, (Color){ 220, 60, 50, 255 });

                    DrawTextSharp(g_fontSmall, "SAMPLE ANALYSIS: Route 9 Bog Core 12-F", docX + 18, docY + 38, 11.0f, (Color){ 150, 145, 140, 220 });

                    DrawLine(docX + 18, docY + 54, docX + docW - 18, docY + 54, (Color){ 55, 30, 25, 200 });

                    DrawTextSharp(g_fontBody, "Core drilling 8 meters into the mire revealed biological", docX + 18, docY + 68, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "specimens buried decades ago with zero cellular decay.", docX + 18, docY + 90, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Tissues retain hydration and microscopic muscle twitching.", docX + 18, docY + 112, 13.0f, (Color){ 200, 195, 185, 240 });

                    DrawTextSharp(g_fontBody, "Local saying carved into the gas station counter:", docX + 18, docY + 140, 12.0f, (Color){ 160, 155, 150, 200 });

                    DrawTextSharp(g_fontMenu, "\"WHAT THE GROUND KEEPS, IT NEVER RELEASES.\"", docX + 18, docY + 160, 13.0f, (Color){ 230, 45, 40, 255 });

                    DrawTextSharp(g_fontBody, "Excavation without proper protective tools is lethal.", docX + 18, docY + 188, 12.0f, (Color){ 180, 175, 170, 220 });

                }



                Rectangle btnCloseDossier = { (float)(dx + dw/2 - 70), (float)(dy + dh - 44), 140.0f, 30.0f };

                bool hClose = CheckCollisionPointRec(mPos, btnCloseDossier);

                DrawRectangleRec(btnCloseDossier, hClose ? (Color){ 80, 25, 20, 255 } : (Color){ 30, 16, 14, 240 });

                DrawRectangleLinesEx(btnCloseDossier, 1.0f, hClose ? WHITE : (Color){ 140, 45, 35, 255 });

                DrawTextSharpCentered(g_fontBody, "CLOSE [ESC]", dx + dw/2, dy + dh - 36, 13.0f, WHITE);

                if (hClose && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

                    PlaySound(g_sndMenuNav);

                    g_showCaseFilesModal = false;

                }

            } else {

                // Pause Menu Main Display (Strong Gothic Heading Font)

                const char* pauseTitle = "INVESTIGATION SUSPENDED";

                int pauseY = (int)(screenH * 0.18f);

                DrawTextSharpCentered(g_fontTitle, pauseTitle, screenW/2 + 3, pauseY + 3, 44.0f, (Color){ 120, 18, 14, 210 }, 3.0f);

                DrawTextSharpCentered(g_fontTitle, pauseTitle, screenW/2, pauseY, 44.0f, (Color){ 245, 240, 232, 255 }, 3.0f);



                DrawTextSharpCentered(g_fontSmall, "ROUTE 9 SERVICE STATION // SIMULATION FROZEN", screenW/2, pauseY + 50, 14.0f, (Color){ 190, 65, 55, 240 });



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

                        DrawLine(ox - 20, oy + 37, (int)(ox + ow + 20), oy + 37, (Color){ emberCol.r, emberCol.g, emberCol.b, 150 });

                        DrawTextSharp(g_fontMenu, ">", ox - 35, oy, 28.0f, emberCol);

                        DrawTextSharp(g_fontMenu, "<", ox + ow + 18, oy, 28.0f, emberCol);

                    }



                    Color optCol = isSel ? WHITE : (Color){ 175, 170, 160, 220 };

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

        // CURSOR UNLOCKED / RESIZE WINDOW BANNER (Clean UI notification)

        // ---------------------------------------------------------------------

        if (!isCursorCaptured && !showQuitConfirm && !isShopOpen && g_gameState == STATE_GAMEPLAY) {
            const char* unlockMsg = "[ MOUSE FREED: Drag edges to resize | Press [F11] for Fullscreen | Left-Click window to Play ]";
            float bw = MeasureTextSharp(g_fontSmall, unlockMsg, 12.0f);
            float pw = bw + 36.0f;
            float px = ((float)screenW - pw) * 0.5f;
            DrawAAAPanel((Rectangle){ px, 8.0f, pw, 28.0f }, (Color){ 12, 16, 24, 235 }, (Color){ 80, 160, 245, 230 }, 5.0f, true);
            DrawTextSharpCentered(g_fontSmall, unlockMsg, (float)screenW * 0.5f, 15.0f, 12.0f, (Color){ 215, 235, 255, 255 });
        }



        // Silky-smooth post-intro cinematic fade-in from black

        if (gameIntroFade > 0.0f) {

            gameIntroFade = fmaxf(0.0f, gameIntroFade - dt * 1.5f);

            DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, gameIntroFade));

        }



        if (g_gameState == STATE_GAMEPLAY) DrawFPS(10, 10);

        EndDrawing();

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

    CloseAudioDevice();

    if (g_fontTitle.texture.id != GetFontDefault().texture.id)   UnloadFont(g_fontTitle);
    if (g_fontHeadSub.texture.id != GetFontDefault().texture.id) UnloadFont(g_fontHeadSub);
    if (g_fontMenu.texture.id != GetFontDefault().texture.id)    UnloadFont(g_fontMenu);
    if (g_fontBody.texture.id != GetFontDefault().texture.id)    UnloadFont(g_fontBody);
    if (g_fontSmall.texture.id != GetFontDefault().texture.id)   UnloadFont(g_fontSmall);

    if (g_proceduralAssetsLoaded) {
        UnloadModel(g_milkLiquidModel);
        UnloadModel(g_bloodLiquidModel);
        UnloadModel(g_breadModel);
        UnloadRenderTexture(g_milkTexRT);
        UnloadRenderTexture(g_bloodTexRT);
        UnloadRenderTexture(g_crustTexRT);
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

    CloseWindow();

    return 0;
    exit(0);

}