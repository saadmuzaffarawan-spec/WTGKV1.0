#pragma once

#include <raylib.h>
#include <vector>
#include <string>
#include "core/game_types.h"
#include "systems/gas_station_system.h"
#include "systems/receipt_printer.h"

// --- KINEMATIC HORROR CAMERA & VIEWMODEL INERTIA ---
extern float g_vmSwayX;
extern float g_vmSwayY;
extern float g_camIdleTimer;
extern float g_camRoll;
extern float g_camStepOffset;
extern float g_camLandingDip;
extern float g_lastPlayerVelY;
extern bool  g_wasOnGround;
extern float g_camDynamicFov;
extern bool  g_isSprinting;

// --- DYNAMIC FLASHLIGHT & WORLD LIGHTING ---
extern bool g_flashlightActive;
extern bool g_shopLightsOn;
extern float g_curExtDayFactor;
extern float g_curExtNightFactor;
extern float g_curLightningFlash;
extern Vector3 g_curSunDir;
extern Vector3 g_playerCamPos;
extern Vector3 g_playerCamFwd;

// --- GAS STATION & FUEL ECONOMY ---
extern RenderTexture2D g_pumpScreenRT;
extern bool g_pumpScreenRTLoaded;
extern bool g_autoDispenseMode;
extern float g_customerCarCooldown;

// --- AUDIO RESOURCES ---
extern Sound g_sndFlashlightToggle;
extern Sound g_sndLightSwitch;
extern Sound g_sndGunshot;
extern Sound g_sndStoreFootstep;
extern Sound g_sndFoil;
extern Sound g_sndMenuNav;
extern Sound g_sndMenuBoom;
extern Sound g_sndRadioStatic;
extern Sound g_sndWaterDrip;
extern Sound g_sndShovelDig;
extern Sound g_sndPhoneSlide;
extern Sound g_sndPhoneTap;
extern Sound g_sndStepperMotor;

// --- FONTS ---
extern Font g_fontTitle;
extern Font g_fontHeadSub;
extern Font g_fontMenu;
extern Font g_fontBody;
extern Font g_fontSmall;

// --- SHOP & ECONOMY ---
extern int g_heldProductIndex;
extern Vector3 g_cartPos;
extern Vector3 g_cartVel;
extern float g_cartYaw;
extern float g_cartWheelSpin;
extern PrinterState g_printerState;
extern float g_printerProgress;
extern Texture2D g_receiptTex;

// --- GHOST TROLLEY ---
extern GhostCart g_ghostCart;

// --- TEST & DEBUG FLAGS ---
extern int g_testFrames;
extern float g_pinnedTimeOfDay;
extern float g_pinnedCoverage;
extern float g_pinnedCloudOffset;
extern const char* g_testScreenshot;
extern int g_testFrameCount;
extern bool g_lookAtSun;
extern bool g_lookAtGround;
extern bool g_lookAtShop;
extern bool g_lookAtWashroom;
extern bool g_lookAtAtm;

// --- GAME STATE & UI ---
enum GameState {
    STATE_MAIN_MENU = 0,
    STATE_GAMEPLAY,
    STATE_PAUSED
};
extern GameState g_gameState;
extern bool g_hasPlayedIntro;
extern bool g_showSettingsModal;
extern bool g_showCaseFilesModal;
extern bool g_showManifestModal;
extern bool g_showSurvivalModal;
extern int g_caseFileSelected;

// --- MENU STATE ---
extern float g_menuIdleTimer;
extern float g_menuAwakeIntensity;
extern Vector2 g_menuLightPos;
extern float g_lurkerEyeFlee;
extern float g_menuCamSmoothX;
extern float g_menuCamSmoothY;
extern float g_skullEyeSmoothX;
extern float g_skullEyeSmoothY;
extern float g_skullGazeFlare;
extern int g_prevMenuSelection;
extern float g_menuOptionHover[5];

// --- WORLD & SECRET STATE ---
extern float g_carpetAnim;
extern float g_hatchAnim;
extern bool g_radioPower;
extern float g_radioAnim;
extern float g_radioMsgTimer;
extern bool g_showDossierModal;
extern bool g_dossierReadOnce;
extern int g_dossierFileSelected;
extern bool g_chestUnlocked;
extern float g_chestLidAnim;
extern bool g_chestLooted;
extern float g_tunnelDripTimer;
extern float g_workLightFlicker;
extern float g_workLightBuzzTimer;
extern float g_digAnimTimer;
extern ShovelAnimState g_shovelAnimState;
extern float g_shovelAnimTime;
extern float g_shovelIdleClock;
extern bool g_shovelDigImpactDone;
extern bool g_shovelDigThrowDone;
extern bool g_shovelAttackImpactDone;
extern float g_tunnelBannerTimer;
extern char g_tunnelBannerText[160];

// --- PHONE SYSTEM ---
extern bool g_phoneActive;
extern float g_phoneAnim;
extern int g_phoneZoomMode;
extern float g_phoneRadarPulse;
extern float g_phoneSignalFlicker;

// --- GAME TRANSITIONS ---
extern bool g_isMenuStartingGame;
extern float g_menuPlayTransitionTimer;

// --- USER SETTINGS ---
extern float g_userMasterVolume;
extern float g_userMouseSensitivity;
extern float g_userFov;
extern float g_userHorrorGamma;
