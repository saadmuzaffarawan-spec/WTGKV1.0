#include "hud_manager.h"
#include "ui_helpers.h"
#include "../core/game_types.h"
#include <cstdio>
#include <cmath>

// State Definitions
float g_playerCash         = 45.00f; // Starting wallet
float g_cashPopupAmount    = 0.0f;
float g_cashPopupTimer     = 0.0f;
char  g_cashPopupText[80]  = { 0 };

float g_stationBellBannerTimer = 0.0f;
char  g_stationBellBanner[128] = { 0 };

float g_stationFuelGallons  = 450.0f; // 10,000-Gal Underground Tank (Starts at 450.0 Gal)
float g_fuelPricePerGallon = 3.89f;  // Fuel price per gallon
bool  g_holdingFuelNozzle   = false;
int   g_activePumpIndex     = -1;    // 0 = Pump 1, 1 = Pump 2
bool  g_nozzleInCar         = false;
float g_fuelSoundCooldown   = 0.0f;
CustomerCar g_customerCar   = { CAR_INACTIVE };

bool  g_isHoldingCart       = false;
std::vector<int> g_cartProductIndices;

bool  g_hasReceipt          = false;
bool  g_inspectingReceipt   = false;
bool  g_receiptThrown       = false;
Vector3 g_thrownReceiptPos  = { 0.0f, 0.0f, 0.0f };
Texture2D g_receiptTex;

bool  g_carpetMoved         = false;
bool  g_tunnelHatchOpen     = false;
bool  g_hasShovel           = false;
bool  g_shovelEquipped      = true;
bool  g_tunnelDug           = false;
float g_digProgress         = 0.0f;

bool  g_salvageCrateOpened  = false;

Sound g_sndCashRegister;
Sound g_sndDrivewayBell;
Sound g_sndPumpFlow;
Sound g_sndNozzleLatch;
Sound g_sndNozzleShutoff;
Sound g_sndFogBell;
Sound g_sndChestOpen;

extern Vector3 g_cartPos;
extern bool g_shopLightsOn;
extern PrinterState g_printerState;
extern int g_heldProductIndex;

void DrawHeldProductHUD(int heldIdx, int focusIdx, bool nearCounter, bool hasShovel, bool isHoldingCart, bool phoneActive, bool shovelEquipped) {
    if (heldIdx != -1) {
        const ShopProduct &hp = g_shopProducts[heldIdx];
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
    } else if (focusIdx != -1 && !nearCounter) {
        const ShopProduct &fp = g_shopProducts[focusIdx];
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
    } else if (hasShovel && !isHoldingCart && !nearCounter && focusIdx == -1 && !phoneActive && shovelEquipped) {
        const char *hintBuf = "Equipped: Heavy Trench Shovel   [LMB] Swing   [RMB] Dig   [1/X] Holster";
        float tw = MeasureTextSharp(g_fontMenu, hintBuf, 14.0f);
        float pw = tw + 32.0f;
        float px = ((float)LOGICAL_W - pw) * 0.5f;
        float py = (float)LOGICAL_H - 48.0f;
        DrawAAAPanel((Rectangle){ px, py, pw, 28.0f }, (Color){ 16, 18, 22, 230 }, (Color){ 175, 145, 85, 220 }, 5.0f, true);
        DrawTextSharpCentered(g_fontMenu, hintBuf, (float)LOGICAL_W * 0.5f, py + 6.5f, 14.0f, (Color){ 245, 230, 185, 245 });
    }
}

void DrawPlayerHUD(const Camera3D &camera) {
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

void DrawBottomNavigationDock(bool hasReceipt, bool inspectingReceipt, bool flashlightActive, bool phoneActive, bool hasShovel, bool shovelEquipped) {
    float dockX = 24.0f;
    float dockY = (float)LOGICAL_H - 34.0f;
    auto DrawDockItem = [&](const char* key, const char* label, Color accentCol, bool activeState) {
        float kw = DrawAAAKeycap(key, dockX, dockY, accentCol);
        dockX += kw + 7.0f;
        float lw = MeasureTextSharp(g_fontSmall, label, 12.0f);
        DrawTextSharp(g_fontSmall, label, dockX, dockY + 4.5f, 12.0f, activeState ? (Color){ 245, 248, 252, 255 } : (Color){ 160, 170, 180, 200 });
        dockX += lw + 18.0f;
    };

    if (hasReceipt && !inspectingReceipt) {
        DrawDockItem("TAB", "RECEIPT", (Color){ 255, 175, 175, 255 }, true);
    }
    DrawDockItem("F", "FLASHLIGHT", flashlightActive ? (Color){ 255, 235, 110, 255 } : (Color){ 140, 145, 155, 200 }, flashlightActive);
    DrawDockItem("M", "PHONE GPS", phoneActive ? (Color){ 80, 220, 255, 255 } : (Color){ 150, 195, 220, 200 }, phoneActive);
    DrawDockItem("C", "CCTV ROOF", (Color){ 110, 220, 140, 220 }, false);
    if (hasShovel && shovelEquipped) {
        DrawDockItem("1 / X", "HOLSTER", (Color){ 220, 180, 90, 230 }, false);
    }
}

void DrawInteractionManager(const Camera3D &camera, float dt, float timeVal, float &camLandingDip) {
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
                    camLandingDip = -0.012f * sinf(timeVal * 55.0f);
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
                        camLandingDip = -0.06f;
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
            SpawnCustomerCar(g_customerCar, g_fuelPricePerGallon);
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

void DrawSubterraneanBanner(float bannerTimer, const char* bannerText) {
    if (bannerTimer <= 0.0f || bannerText == nullptr || bannerText[0] == '\0') return;

    float bannerAlpha = Clamp(bannerTimer, 0.0f, 1.0f);
    float bw = MeasureTextSharp(g_fontMenu, bannerText, 15.0f) + 40.0f;
    if (bw < 380.0f) bw = 380.0f;
    float bx = ((float)LOGICAL_W - bw) * 0.5f;
    float by = 68.0f;
    DrawAAAPanel((Rectangle){ bx, by, bw, 34.0f }, (Color){ 8, 10, 14, (unsigned char)(235 * bannerAlpha) }, (Color){ 215, 55, 45, (unsigned char)(255 * bannerAlpha) }, 5.0f, true);
    DrawTextSharpCentered(g_fontMenu, bannerText, (float)LOGICAL_W * 0.5f, by + 8.5f, 15.0f, (Color){ 245, 235, 220, (unsigned char)(255 * bannerAlpha) });
}

void DrawSurvivalClockHUD(float dayCycleTime, float dayCycleDuration) {
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

void DrawReceiptInspectionOverlay() {
    if (!g_inspectingReceipt || !g_hasReceipt) return;

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

void DrawShopkeeperStoreUI(bool isShopOpen, const char* shopFeedbackMsg) {
    if (!isShopOpen) return;

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

void DrawTacticalCrosshair(int hudFocusIdx, bool nearCounter, int heldProductIdx) {
    Color crosshairCol = (hudFocusIdx != -1 && !nearCounter && heldProductIdx == -1) ? (Color){ 90, 245, 130, 255 } : CYAN;
    int cx = LOGICAL_W / 2;
    int cy = LOGICAL_H / 2;
    DrawCircle(cx, cy, 1.8f, crosshairCol);
    DrawLine(cx - 8, cy, cx - 3, cy, crosshairCol);
    DrawLine(cx + 4, cy, cx + 9, cy, crosshairCol);
    DrawLine(cx, cy - 8, cx, cy - 3, crosshairCol);
    DrawLine(cx, cy + 4, cx, cy + 9, crosshairCol);
}
