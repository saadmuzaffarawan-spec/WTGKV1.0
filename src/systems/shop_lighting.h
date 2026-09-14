#pragma once

#include <raylib.h>
#include <raymath.h>
#include "shop_item_types.inl"

// =========================================================================
// SHOP LIGHTING CONTEXT & PHYSICAL MULTI-LIGHT ILLUMINATION ENGINE
// Evaluates light contributions from the central swaying bulb and ceiling tubelights
// =========================================================================

struct ShopLightSource {
    Vector3 pos;
    Color   color;
    float   intensity;
    float   radius;
    float   radiusSq;
    float   invRadius;
    float   colR;
    float   colG;
    float   colB;
};

struct ShopLightingContext {
    ShopLightSource lights[8]; // 0: Central Bulb, 1: North Tube, 2: Counter Spotlight, 3: South Tube, 4: Freezer LEDs, 5: Entrance Tube, 6: Storage Tube, 7: Washroom Fixture
};

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

extern ShopLightingContext g_shopLighting;
extern GhostCart g_ghostCart;

void SetShopLight(int idx, Vector3 pos, Color color, float intensity, float radius);
Color ApplyShopLighting(Vector3 pos, Color baseAlbedo, Vector3 normal = (Vector3){ 0.0f, 1.0f, 0.0f }, int occludeAisle = 0);
float GetShopLightFactorAt(Vector3 pos);

// In-store item rendering & cart functions
void DrawPopcornTin(Vector3 pos, bool opened, float wobble, bool held, float distSq = 0.0f);
void DrawBottle(Vector3 pos, float bottleH, float bottleR, Model &liquidModel, float fill, Color glassColor, bool opened, bool held, Vector3 fwdDir, float distSq = 0.0f);
void DrawBread(Vector3 pos, Model &crustModel, bool held, float distSq = 0.0f);
void DrawShoppingCartLocal(float wheelSpinDeg, Vector3 worldPos, float distSq = 0.0f);
void DrawGhostShoppingCartLocal(float wheelSpinDeg, Vector3 worldPos, float alpha, float timeVal);
void UpdateGhostCart(float dt, float nightFactor);
void UpdateShopParticle(ShopParticle &p, float dt, float floorY);
void DrawShopParticle(const ShopParticle &p);
