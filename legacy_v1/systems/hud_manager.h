#pragma once

#include <raylib.h>
#include <raymath.h>
#include <vector>
#include "gas_station_system.h"
#include "receipt_printer.h"
#include "hound_npc.h"
#include "shop_item_types.inl"

// =========================================================================
// IN-GAME AAA HUD & OVERLAY MANAGER
// - Top-left permanent Cash Card, Abandoned College location tag, floating popups
// - Centralized priority queue interaction manager (counter, printer, pumps, etc.)
// - Dynamic bottom navigation dock (responsive keycaps ribbon)
// - Context-sensitive held item hints / product interaction hints
// - Day / Night survival clock & celestial telemetry HUD
// - Full-screen horror receipt reading / inspection overlay
// - Surreal shopkeeper store interface overlay
// - Tactical crosshair reticle
// =========================================================================

// Shared State Declarations
extern float g_playerCash;
extern float g_cashPopupAmount;
extern float g_cashPopupTimer;
extern char  g_cashPopupText[80];
extern bool  g_showDebugFPS;

void TriggerCashPopup(float amount, const char* label = nullptr);

extern float g_stationBellBannerTimer;
extern char  g_stationBellBanner[128];

extern float g_stationFuelGallons;
extern float g_fuelPricePerGallon;
extern bool  g_holdingFuelNozzle;
extern int   g_activePumpIndex;
extern bool  g_nozzleInCar;
extern float g_fuelSoundCooldown;
extern CustomerCar g_customerCar;

extern bool  g_isHoldingCart;
extern std::vector<int> g_cartProductIndices;

extern bool  g_hasReceipt;
extern bool  g_inspectingReceipt;
extern bool  g_receiptThrown;
extern Vector3 g_thrownReceiptPos;
extern Texture2D g_receiptTex;

extern bool  g_carpetMoved;
extern bool  g_tunnelHatchOpen;
extern bool  g_hasShovel;
extern bool  g_shovelEquipped;
extern bool  g_tunnelDug;
extern float g_digProgress;

extern bool  g_salvageCrateOpened;

extern Sound g_sndCashRegister;
extern Sound g_sndDrivewayBell;
extern Sound g_sndPumpFlow;
extern Sound g_sndNozzleLatch;
extern Sound g_sndNozzleShutoff;
extern Sound g_sndFogBell;
extern Sound g_sndChestOpen;

// HUD Rendering & Management Functions
void DrawHeldProductHUD(int heldIdx, int focusIdx, bool nearCounter, bool hasShovel, bool isHoldingCart, bool phoneActive, bool shovelEquipped);
void DrawPlayerHUD(const Camera3D &camera);
void DrawBottomNavigationDock(bool hasReceipt, bool inspectingReceipt, bool flashlightActive, bool phoneActive, bool hasShovel, bool shovelEquipped);
void DrawInteractionManager(const Camera3D &camera, float dt, float timeVal, float &camLandingDip);
void DrawSubterraneanBanner(float bannerTimer, const char* bannerText);
void DrawSurvivalClockHUD(float dayCycleTime, float dayCycleDuration);
void DrawReceiptInspectionOverlay();
void DrawShopkeeperStoreUI(bool isShopOpen, const char* shopFeedbackMsg);
void DrawTacticalCrosshair(int hudFocusIdx, bool nearCounter, int heldProductIdx);
