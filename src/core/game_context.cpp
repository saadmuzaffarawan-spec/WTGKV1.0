#include "core/game_context.h"

// --- KINEMATIC HORROR CAMERA & VIEWMODEL INERTIA ---
float g_vmSwayX = 0.0f;
float g_vmSwayY = 0.0f;
float g_camIdleTimer = 0.0f;
float g_camRoll = 0.0f;
float g_camStepOffset = 0.0f;
float g_camLandingDip = 0.0f;
float g_lastPlayerVelY = 0.0f;
bool  g_wasOnGround = true;
float g_camDynamicFov = 60.0f;
bool  g_isSprinting = false;

// --- DYNAMIC FLASHLIGHT & WORLD LIGHTING ---
bool g_flashlightActive = false;
bool g_shopLightsOn = true;
float g_curExtDayFactor = 0.5f;
float g_curExtNightFactor = 0.5f;
float g_curLightningFlash = 0.0f;
Vector3 g_curSunDir = { 0.0f, 1.0f, 0.0f };
Vector3 g_playerCamPos = { 100.0f, 12.2f, 140.0f };
Vector3 g_playerCamFwd = { 0.0f, 0.0f, 1.0f };

// --- GAS STATION & FUEL ECONOMY ---
RenderTexture2D g_pumpScreenRT = { 0 };
bool g_pumpScreenRTLoaded = false;
bool g_autoDispenseMode = false;
float g_customerCarCooldown = 35.0f;

// --- AUDIO RESOURCES ---
Sound g_sndFlashlightToggle = { 0 };
Sound g_sndLightSwitch = { 0 };
Sound g_sndGunshot = { 0 };
Sound g_sndStoreFootstep = { 0 };
Sound g_sndFoil = { 0 };
Sound g_sndMenuNav = { 0 };
Sound g_sndMenuBoom = { 0 };
Sound g_sndRadioStatic = { 0 };
Sound g_sndWaterDrip = { 0 };
Sound g_sndShovelDig = { 0 };
Sound g_sndPhoneSlide = { 0 };
Sound g_sndPhoneTap = { 0 };
Sound g_sndStepperMotor = { 0 };

// --- FONTS ---
Font g_fontTitle = { 0 };
Font g_fontHeadSub = { 0 };
Font g_fontMenu = { 0 };
Font g_fontBody = { 0 };
Font g_fontSmall = { 0 };

// --- SHOP & ECONOMY ---
int g_heldProductIndex = -1;
Vector3 g_cartPos = { 104.5f, 10.02f, 142.5f };
Vector3 g_cartVel = { 0.0f, 0.0f, 0.0f };
float g_cartYaw = -90.0f;
float g_cartWheelSpin = 0.0f;
PrinterState g_printerState = PRINTER_IDLE;
float g_printerProgress = 0.0f;
Texture2D g_receiptTex = { 0 };

// --- GHOST TROLLEY ---
GhostCart g_ghostCart = { { 97.0f, 10.02f, 142.5f }, 180.0f, 0.0f, 0.0f, 0, 5.0f, 0, false, 0.0f, 0.0f };

// --- TEST & DEBUG FLAGS ---
int g_testFrames = -1;
float g_pinnedTimeOfDay = -1.0f;
float g_pinnedCoverage = -1.0f;
float g_pinnedCloudOffset = 0.0f;
const char* g_testScreenshot = "test_output.png";
int g_testFrameCount = 0;
bool g_lookAtSun = false;
bool g_lookAtGround = false;
bool g_lookAtShop = false;
bool g_lookAtWashroom = false;
bool g_lookAtAtm = false;

// --- GAME STATE & UI ---
GameState g_gameState = STATE_MAIN_MENU;
bool g_hasPlayedIntro = false;
bool g_showSettingsModal = false;
bool g_showCaseFilesModal = false;
bool g_showManifestModal = false;
bool g_showSurvivalModal = false;
int g_caseFileSelected = 0;

// --- MENU STATE ---
float g_menuIdleTimer = 0.0f;
float g_menuAwakeIntensity = 1.0f;
Vector2 g_menuLightPos = { 640.0f, 360.0f };
float g_lurkerEyeFlee = 0.0f;
float g_menuCamSmoothX = 0.0f;
float g_menuCamSmoothY = 0.0f;
float g_skullEyeSmoothX = 0.0f;
float g_skullEyeSmoothY = 0.0f;
float g_skullGazeFlare = 0.0f;
int g_prevMenuSelection = -1;
float g_menuOptionHover[5] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

// --- WORLD & SECRET STATE ---
float g_carpetAnim = 0.0f;
float g_hatchAnim = 0.0f;
bool g_radioPower = true;
float g_radioAnim = 0.0f;
float g_radioMsgTimer = 0.0f;
bool g_showDossierModal = false;
bool g_dossierReadOnce = false;
int g_dossierFileSelected = 0;
bool g_chestUnlocked = false;
float g_chestLidAnim = 0.0f;
bool g_chestLooted = false;
float g_tunnelDripTimer = 0.0f;
float g_workLightFlicker = 1.0f;
float g_workLightBuzzTimer = 0.0f;
float g_digAnimTimer = 0.0f;
ShovelAnimState g_shovelAnimState = SHOVEL_ANIM_IDLE;
float g_shovelAnimTime = 0.0f;
float g_shovelIdleClock = 0.0f;
bool g_shovelDigImpactDone = false;
bool g_shovelDigThrowDone = false;
bool g_shovelAttackImpactDone = false;
float g_tunnelBannerTimer = 0.0f;
char g_tunnelBannerText[160] = { 0 };

// --- PHONE SYSTEM ---
bool g_phoneActive = false;
float g_phoneAnim = 0.0f;
int g_phoneZoomMode = 0;
float g_phoneRadarPulse = 0.0f;
float g_phoneSignalFlicker = 0.0f;

// --- GAME TRANSITIONS ---
bool g_isMenuStartingGame = false;
float g_menuPlayTransitionTimer = 0.0f;

// --- USER SETTINGS ---
float g_userMasterVolume = 1.0f;
float g_userMouseSensitivity = 1.0f;
float g_userFov = 60.0f;
float g_userHorrorGamma = 1.0f;
