#pragma once
#include "raylib.h"
#include "raymath.h"
#include <functional>

// =============================================================================
// WHAT THE GROUND KEEPS - PHASE 3: SHOP ATMOSPHERE & TEXTURING SYSTEM
// Modular architecture for high-fidelity 1970s Appalachian convenience store:
// - Procedural tongue-and-groove dark timber wall paneling
// - Moisture seepage, water stains, mold gradients & peeling plaster grunge
// - Environmental storytelling: Missing person posters, vintage tin signs, notices
// - Water damaged acoustic ceiling grid with water ring stains
// - Atmospheric airborne dust motes floating in light cones
// - Fluttering moth around the swaying central tungsten bulb
// - Floor rain puddle specular reflections & electrical conduit details
// =============================================================================

typedef std::function<Color(Vector3 pos, Color baseAlbedo, Vector3 normal, int occludeAisle)> ShopLightFn;

// Initialize all procedural textures, particle buffers, and audio for the shop
void InitShopAtmosphere();

// Update dust motes, fluttering insects, puddle ripple timers, and electrical sounds
void UpdateShopAtmosphere(float dt, float timeVal, Vector3 playerPos, bool isMoving, bool lightsOn, float bulbSwayX, float bulbSwayZ);

// Render textured walls with procedural timber paneling, wainscoting, and grunge
void DrawShopAtmosphereWalls(ShopLightFn lightFn, bool lightsOn, float timeVal);

// Render weathered acoustic ceiling tiles with water damage stains and rusted grid tracks
void DrawShopAtmosphereCeiling(ShopLightFn lightFn, bool lightsOn);

// Render environmental storytelling props (Missing posters, vintage signs, breaker box, floor puddles)
void DrawShopAtmosphereDetails(ShopLightFn lightFn, bool lightsOn, float timeVal);

// Render volumetric dust motes and fluttering moths caught in the shop light beams
void DrawShopAtmosphereParticles(Camera3D camera, bool lightsOn, float timeVal);

// Render textured shop floor with procedural 1970s commercial checkered vinyl composite tiles
void DrawShopAtmosphereFloor(ShopLightFn lightFn, bool lightsOn, float timeVal);

// Render environmental shop props (drink cooler, newspaper rack, coffee station, crates, phone, signs)
void DrawShopInteriorProps(ShopLightFn lightFn, bool lightsOn, float timeVal);

// Update real planar reflection mirror camera and render texture for the washroom
void UpdateShopWashroomMirror(Camera3D playerCam, ShopLightFn lightFn, bool lightsOn, float timeVal);

// Render haunted washroom architecture, porcelain toilet, sink, real reflection mirror, and blood stains
void DrawShopHauntedWashroom(Camera3D camera, ShopLightFn lightFn, bool lightsOn, float timeVal);

// Check if player is standing within interactive reach of the washroom ceramic sink
bool IsPlayerNearWashroomSink(Vector3 playerPos);

// Toggle washroom ceramic sink faucet ON / OFF
void ToggleWashroomSinkFaucet();

// Check if the washroom sink faucet is currently flowing water
bool IsWashroomSinkRunning();

// Render convenience superstore double-sided gondolas stocked with automotive motor oil, coolant, and road snacks
void DrawShopSuperstoreGondolas(ShopLightFn lightFn, bool lightsOn, float timeVal);

// Free all GPU textures and resources allocated by the shop atmosphere system
void UnloadShopAtmosphere();

// Interactive washroom entrance door
extern float g_washroomDoorAngle;
bool IsPlayerNearWashroomDoor(Vector3 playerPos);
void ToggleWashroomDoor();
bool IsWashroomDoorOpen();

// Midnight Security Monitor (CCTV CRT on Checkout Desk)
void UpdateCounterCCTV(Camera3D playerCam, ShopLightFn lightFn, bool lightsOn, float timeVal);
void DrawCounterSecurityMonitor(Vector3 pos, ShopLightFn lightFn, bool lightsOn, float timeVal);


