#pragma once
#include <raylib.h>
#include <vector>

// =========================================================================

// PROCEDURAL SUPERSTORE PRODUCTS & PARTICLE SYSTEM

// Real interactive 3D products populated in filled supermarket racks:

// - Popcorn Tin   -> procedural cylinder body + stripes + opening lid + kernel physics

// - Sparrow's milk -> glass bottle + white '~' glyph procedural liquid + pouring physics

// - B+ Blood       -> glass bottle + dark red '~' glyph procedural liquid + pouring physics

// - Bread          -> oblong baked loaf with procedurally generated crust & score marks

// - Shopping Cart  -> fully assembled rod-grid basket, frame, wheels, handle

// =========================================================================

enum ShopProductType {

    PROD_TIN = 0,

    PROD_MILK,

    PROD_BLOOD,

    PROD_BREAD,

    PROD_CART,

    PROD_CHOCOLATE,

    PROD_GUN

};



struct ShopParticle {

    Vector3 pos;

    Vector3 vel;

    float   radius;

    Color   color;

    bool    landed;

    float   settleTimer;

    bool    isCube;

};



struct DustParticle {

    Vector3 pos;

    Vector3 vel;

    float   size;

    float   life;

    float   maxLife;

    Color   color;

    float   spin;

    float   spinSpeed;

};



struct FootstepTrail {

    Vector3 pos;

    float   yaw;

    float   life;

    float   maxLife;

    bool    isLeft;

};



struct GunSpark {

    Vector3 pos;

    Vector3 vel;

    float   life;

    Color   color;

};



struct ShellCasing {

    Vector3 pos;

    Vector3 vel;

    Vector3 rot;

    Vector3 rotVel;

    float   life;

    bool    landed;

};



struct ShopProduct {

    ShopProductType type;

    Vector3 homePos;

    Vector3 originalPos;

    Vector3 vel;

    bool    held;

    bool    opened;

    float   fill;

    float   spawnTimer;

    int     spawnedCount;

    int     maxSpawn;

    const char* label;

    const char* price;

    int     subType = 0; // 0, 1, 2 for chocolate varieties

};



extern std::vector<ShopProduct> g_shopProducts;
extern std::vector<ShopParticle> g_shopParticles;
extern std::vector<DustParticle> g_dustParticles;
extern std::vector<FootstepTrail> g_footstepTrails;
extern std::vector<GunSpark> g_gunSparks;
extern std::vector<ShellCasing> g_shellCasings;

extern float g_gunRecoilTimer;
extern float g_gunMuzzleFlashTimer;
extern bool  g_isLeftFootStep;



