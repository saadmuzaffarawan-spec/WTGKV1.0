#include "procedural_shop_assets.h"
#include "shop_item_types.inl"
#include "texture_factories.h"
#include <vector>

static RenderTexture2D g_milkTexRT;
static RenderTexture2D g_bloodTexRT;
static RenderTexture2D g_crustTexRT;

Model g_milkLiquidModel;
Model g_bloodLiquidModel;
Model g_breadModel;

bool g_proceduralAssetsLoaded = false;
extern Vector3 g_cartPos;

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

void UnloadProceduralShopAssets() {
    if (!g_proceduralAssetsLoaded) return;
    UnloadModel(g_milkLiquidModel);
    UnloadModel(g_bloodLiquidModel);
    UnloadModel(g_breadModel);
    UnloadRenderTexture(g_milkTexRT);
    UnloadRenderTexture(g_bloodTexRT);
    UnloadRenderTexture(g_crustTexRT);
    g_proceduralAssetsLoaded = false;
}
