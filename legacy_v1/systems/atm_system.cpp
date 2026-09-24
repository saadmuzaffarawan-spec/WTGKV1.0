#include "atm_system.h"
#include "audio_synthesis.h"
#include "hud_manager.h"
#include "ui_helpers.h"
#include "rlgl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// =============================================================================
// WHAT THE GROUND KEEPS - HYPER-REALISTIC CONVENIENCE STORE ATM SYSTEM
// =============================================================================

static ATMState g_atmState = ATM_IDLE;
static Vector3  g_atmPos = { 107.45f, 10.0f, 143.5f }; // Along East wall facing -X (beside entrance sliding door)
static RenderTexture2D g_atmScreenRT;
static bool     g_atmLoaded = false;
static float    g_atmStateTimer = 0.0f;
static char     g_atmPinBuffer[8] = "";
static int      g_atmPinLen = 0;
static float    g_atmWithdrawAmount = 0.0f;
static float    g_atmAccountBalance = 840.00f;
static float    g_atmCardAnim = 0.0f;  // 0.0f = ejected, 1.0f = fully inserted
static float    g_atmCashAnim = 0.0f;  // 0.0f = retracted, 1.0f = fully dispensed
static bool     g_isInteractingWithATM = false;

static Sound    g_sndATMKey;
static Sound    g_sndATMFeed;
static Sound    g_sndATMEject;
static Sound    g_sndATMDispense;
static Sound    g_sndATMRuffle;

extern Font     g_fontMenu;
extern Font     g_fontBody;
extern Font     g_fontSmall;
extern float    g_camLandingDip;

void InitATMSystem() {
    if (g_atmLoaded) return;

    g_atmScreenRT = LoadRenderTexture(320, 240);
    SetTextureFilter(g_atmScreenRT.texture, TEXTURE_FILTER_BILINEAR);

    g_sndATMKey      = GenerateATMKeyBeep();
    g_sndATMFeed     = GenerateATMCardFeed();
    g_sndATMEject    = GenerateATMCardEject();
    g_sndATMDispense = GenerateATMCashDispense();
    g_sndATMRuffle   = GenerateATMCashRuffle();

    g_atmState = ATM_IDLE;
    g_atmStateTimer = 0.0f;
    g_atmCardAnim = 0.0f;
    g_atmCashAnim = 0.0f;
    g_atmPinLen = 0;
    g_atmPinBuffer[0] = '\0';
    g_isInteractingWithATM = false;
    g_atmLoaded = true;
}

void UnloadATMSystem() {
    if (!g_atmLoaded) return;
    UnloadRenderTexture(g_atmScreenRT);
    UnloadSound(g_sndATMKey);
    UnloadSound(g_sndATMFeed);
    UnloadSound(g_sndATMEject);
    UnloadSound(g_sndATMDispense);
    UnloadSound(g_sndATMRuffle);
    g_atmLoaded = false;
}

bool IsPlayerNearATM(Vector3 playerPos) {
    Vector3 atmFront = { g_atmPos.x - 0.75f, 11.5f, g_atmPos.z };
    return Vector3Distance(playerPos, atmFront) < 1.85f;
}

void StartATMInteraction() {
    g_isInteractingWithATM = true;
    g_atmState = ATM_INSERTING_CARD;
    g_atmCardAnim = 0.0f;
    g_atmStateTimer = 0.0f;
    PlaySound(g_sndATMFeed);
}

void CloseATMInteraction() {
    g_isInteractingWithATM = false;
    g_atmState = ATM_IDLE;
    g_atmPinLen = 0;
    g_atmPinBuffer[0] = '\0';
    g_atmCardAnim = 0.0f;
    g_atmCashAnim = 0.0f;
}

bool IsATMActive() {
    return g_isInteractingWithATM;
}

void UpdateATMSystem(float dt, Vector3 playerPos, bool &isPlayerInteracting) {
    if (!g_atmLoaded) return;

    if (g_isInteractingWithATM) {
        isPlayerInteracting = true;
        g_atmStateTimer += dt;

        switch (g_atmState) {
            case ATM_IDLE:
                break;

            case ATM_INSERTING_CARD: {
                g_atmCardAnim = fminf(1.0f, g_atmCardAnim + dt * 2.2f);
                if (g_atmCardAnim >= 1.0f && g_atmStateTimer > 0.65f) {
                    g_atmState = ATM_ENTER_PIN;
                    g_atmPinLen = 0;
                    g_atmPinBuffer[0] = '\0';
                    g_atmStateTimer = 0.0f;
                }
            } break;

            case ATM_ENTER_PIN: {
                // Read numeric keys 0-9
                for (int key = KEY_ZERO; key <= KEY_NINE; key++) {
                    if (IsKeyPressed(key) && g_atmPinLen < 4) {
                        g_atmPinBuffer[g_atmPinLen++] = (char)('0' + (key - KEY_ZERO));
                        g_atmPinBuffer[g_atmPinLen] = '\0';
                        PlaySound(g_sndATMKey);
                    }
                }
                for (int key = KEY_KP_0; key <= KEY_KP_9; key++) {
                    if (IsKeyPressed(key) && g_atmPinLen < 4) {
                        g_atmPinBuffer[g_atmPinLen++] = (char)('0' + (key - KEY_KP_0));
                        g_atmPinBuffer[g_atmPinLen] = '\0';
                        PlaySound(g_sndATMKey);
                    }
                }
                if (IsKeyPressed(KEY_BACKSPACE) && g_atmPinLen > 0) {
                    g_atmPinLen--;
                    g_atmPinBuffer[g_atmPinLen] = '\0';
                    PlaySound(g_sndATMKey);
                }
                if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) && g_atmPinLen == 4) {
                    g_atmState = ATM_AUTHENTICATING;
                    g_atmStateTimer = 0.0f;
                    PlaySound(g_sndATMKey);
                } else if (g_atmPinLen == 4 && g_atmStateTimer > 0.4f) {
                    // Auto-validate once 4 digits entered
                    g_atmState = ATM_AUTHENTICATING;
                    g_atmStateTimer = 0.0f;
                    PlaySound(g_sndATMKey);
                }
                if (IsKeyPressed(KEY_ESCAPE)) {
                    g_atmState = ATM_EJECTING_CARD;
                    g_atmStateTimer = 0.0f;
                    PlaySound(g_sndATMEject);
                }
            } break;

            case ATM_AUTHENTICATING: {
                if (g_atmStateTimer > 0.85f) {
                    g_atmState = ATM_MENU;
                    g_atmStateTimer = 0.0f;
                }
            } break;

            case ATM_MENU: {
                float withdrawChoice = 0.0f;
                if (IsKeyPressed(KEY_ONE)   || IsKeyPressed(KEY_KP_1)) withdrawChoice = 20.0f;
                if (IsKeyPressed(KEY_TWO)   || IsKeyPressed(KEY_KP_2)) withdrawChoice = 40.0f;
                if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) withdrawChoice = 60.0f;
                if (IsKeyPressed(KEY_FOUR)  || IsKeyPressed(KEY_KP_4)) withdrawChoice = 100.0f;
                if (IsKeyPressed(KEY_FIVE)  || IsKeyPressed(KEY_KP_5)) withdrawChoice = 200.0f;

                if (withdrawChoice > 0.0f) {
                    PlaySound(g_sndATMKey);
                    if (g_atmAccountBalance >= withdrawChoice) {
                        g_atmWithdrawAmount = withdrawChoice;
                        g_atmState = ATM_DISPENSING_CASH;
                        g_atmStateTimer = 0.0f;
                        g_atmCashAnim = 0.0f;
                        PlaySound(g_sndATMDispense);
                    } else {
                        g_atmState = ATM_INSUFFICIENT_FUNDS;
                        g_atmStateTimer = 0.0f;
                        PlaySound(g_sndATMEject); // Error sound
                    }
                }
                if (IsKeyPressed(KEY_ESCAPE)) {
                    g_atmState = ATM_EJECTING_CARD;
                    g_atmStateTimer = 0.0f;
                    PlaySound(g_sndATMEject);
                }
            } break;

            case ATM_DISPENSING_CASH: {
                g_atmCashAnim = fminf(1.0f, g_atmStateTimer / 0.70f);
                if (g_atmStateTimer >= 0.75f) {
                    g_atmState = ATM_COLLECT_CASH;
                    g_atmStateTimer = 0.0f;
                }
            } break;

            case ATM_COLLECT_CASH: {
                if (IsKeyPressed(KEY_E) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    g_playerCash += g_atmWithdrawAmount;
                    g_atmAccountBalance -= g_atmWithdrawAmount;
                    PlaySound(g_sndATMRuffle);
                    g_camLandingDip = -0.015f; // Physical tactile feedback

                    // Trigger diegetic HUD popup
                    g_cashPopupAmount = g_atmWithdrawAmount;
                    g_cashPopupTimer = 3.5f;
                    snprintf(g_cashPopupText, sizeof(g_cashPopupText), "+$%.2f (ATM DISPENSED)", g_atmWithdrawAmount);

                    g_atmCashAnim = 0.0f;
                    g_atmState = ATM_EJECTING_CARD;
                    g_atmStateTimer = 0.0f;
                    PlaySound(g_sndATMEject);
                }
            } break;

            case ATM_INSUFFICIENT_FUNDS: {
                if (g_atmStateTimer > 3.0f) {
                    g_atmState = ATM_MENU;
                    g_atmStateTimer = 0.0f;
                }
            } break;

            case ATM_EJECTING_CARD: {
                g_atmCardAnim = fmaxf(0.0f, 1.0f - g_atmStateTimer * 1.8f);
                if (g_atmStateTimer > 1.2f || (g_atmCardAnim <= 0.0f && (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)))) {
                    CloseATMInteraction();
                }
            } break;

            default:
                break;
        }
    } else {
        // Not interacting; reset timers
        g_atmStateTimer += dt;
        g_atmCardAnim = 0.0f;
        g_atmCashAnim = 0.0f;
    }
}

void PreRenderATMScreen(float timeVal) {
    BeginTextureMode(g_atmScreenRT);
    // Dark CRT green phosphor background
    ClearBackground(Color{ 6, 18, 12, 255 });

    // Subtle phosphor scanlines
    for (int y = 0; y < 240; y += 4) {
        DrawRectangle(0, y, 320, 2, Color{ 0, 0, 0, 45 });
    }

    // Top bank header bar
    DrawRectangle(0, 0, 320, 32, Color{ 12, 45, 28, 255 });
    DrawTextSharp(g_fontMenu, "SUMMIT VALLEY TRUST", 16, 6, 14, Color{ 140, 255, 175, 255 });
    DrawTextSharp(g_fontSmall, "24-HR CASHPOINT // CIRRUS-PLUS", 16, 20, 10, Color{ 80, 210, 120, 220 });
    DrawLine(0, 32, 320, 32, Color{ 60, 230, 110, 255 });

    // Bottom telemetry bar
    DrawLine(0, 218, 320, 218, Color{ 40, 160, 80, 180 });
    DrawTextSharp(g_fontSmall, "TERM #448-SV | ENCRYPTION: 3DES", 12, 224, 10, Color{ 70, 180, 110, 200 });

    switch (g_atmState) {
        case ATM_IDLE: {
            float blink = sinf(timeVal * 4.0f) * 0.5f + 0.5f;
            DrawTextSharpCentered(g_fontMenu, "WELCOME TO", 160, 58, 16, Color{ 110, 240, 145, 255 });
            DrawTextSharpCentered(g_fontMenu, "SUMMIT VALLEY TRUST", 160, 78, 18, Color{ 160, 255, 190, 255 });

            DrawRectangle(24, 114, 272, 54, Color{ 14, 38, 24, 255 });
            DrawRectangleLines(24, 114, 272, 54, Color{ 80, 230, 120, 240 });
            DrawTextSharpCentered(g_fontMenu, "PLEASE INSERT YOUR", 160, 124, 14, Color{ 130, 255, 160, (unsigned char)(255 * blink) });
            DrawTextSharpCentered(g_fontMenu, "BANK DEBIT CARD", 160, 144, 15, Color{ 180, 255, 200, (unsigned char)(255 * blink) });

            DrawTextSharpCentered(g_fontSmall, "[E] INSERT CARD TO BEGIN", 160, 188, 13, Color{ 110, 235, 145, 255 });
        } break;

        case ATM_INSERTING_CARD: {
            DrawTextSharpCentered(g_fontMenu, "READING CHIP DATA...", 160, 90, 16, Color{ 140, 255, 175, 255 });
            DrawRectangle(40, 120, 240, 18, Color{ 14, 38, 24, 255 });
            DrawRectangle(42, 122, (int)(236.0f * g_atmCardAnim), 14, Color{ 60, 240, 120, 255 });
            DrawTextSharpCentered(g_fontSmall, "DO NOT REMOVE CARD", 160, 154, 13, Color{ 255, 220, 90, 255 });
        } break;

        case ATM_ENTER_PIN: {
            DrawTextSharpCentered(g_fontMenu, "ENTER 4-DIGIT PIN:", 160, 55, 16, Color{ 140, 255, 175, 255 });

            // 4 masked boxes
            for (int i = 0; i < 4; i++) {
                int bx = 80 + i * 44;
                DrawRectangle(bx, 84, 36, 44, Color{ 16, 48, 30, 255 });
                DrawRectangleLines(bx, 84, 36, 44, Color{ 80, 240, 130, 255 });
                if (i < g_atmPinLen) {
                    DrawTextSharp(g_fontMenu, "*", bx + 12, 92, 30, Color{ 220, 255, 230, 255 });
                }
            }

            DrawTextSharpCentered(g_fontSmall, "USE KEYPAD OR NUMBER KEYS", 160, 146, 12, Color{ 100, 220, 130, 230 });
            DrawTextSharpCentered(g_fontSmall, "[ENTER] CONFIRM   [CLEAR] ERASE", 160, 168, 11, Color{ 120, 240, 150, 240 });
            DrawTextSharpCentered(g_fontSmall, "[ESC] CANCEL & RETURN CARD", 160, 192, 11, Color{ 255, 140, 120, 230 });
        } break;

        case ATM_AUTHENTICATING: {
            DrawTextSharpCentered(g_fontMenu, "VERIFYING WITH CENTRAL HOST...", 160, 95, 14, Color{ 140, 255, 175, 255 });
            float barProg = fminf(1.0f, g_atmStateTimer / 0.85f);
            DrawRectangle(40, 125, 240, 16, Color{ 14, 38, 24, 255 });
            DrawRectangle(42, 127, (int)(236.0f * barProg), 12, Color{ 75, 250, 130, 255 });
            DrawTextSharpCentered(g_fontSmall, "ENCRYPTED NETWORK HANDSHAKE", 160, 156, 11, Color{ 90, 210, 120, 220 });
        } break;

        case ATM_MENU: {
            DrawTextSharp(g_fontMenu, "ACCOUNT: CHECKING (***-7192)", 16, 40, 12, Color{ 120, 240, 150, 240 });
            char balBuf[64];
            snprintf(balBuf, sizeof(balBuf), "AVAILABLE: $%.2f", g_atmAccountBalance);
            DrawTextSharp(g_fontMenu, balBuf, 16, 56, 14, Color{ 220, 255, 180, 255 });

            // 6 buttons
            DrawRectangle(14, 80, 140, 32, Color{ 14, 42, 26, 255 });
            DrawRectangleLines(14, 80, 140, 32, Color{ 60, 210, 100, 240 });
            DrawText("[1] CASH  $20", 24, 88, 14, Color{ 160, 255, 180, 255 });

            DrawRectangle(166, 80, 140, 32, Color{ 14, 42, 26, 255 });
            DrawRectangleLines(166, 80, 140, 32, Color{ 60, 210, 100, 240 });
            DrawText("[4] CASH $100", 176, 88, 14, Color{ 160, 255, 180, 255 });

            DrawRectangle(14, 120, 140, 32, Color{ 14, 42, 26, 255 });
            DrawRectangleLines(14, 120, 140, 32, Color{ 60, 210, 100, 240 });
            DrawText("[2] CASH  $40", 24, 128, 14, Color{ 160, 255, 180, 255 });

            DrawRectangle(166, 120, 140, 32, Color{ 14, 42, 26, 255 });
            DrawRectangleLines(166, 120, 140, 32, Color{ 60, 210, 100, 240 });
            DrawText("[5] CASH $200", 176, 128, 14, Color{ 160, 255, 180, 255 });

            DrawRectangle(14, 160, 140, 32, Color{ 14, 42, 26, 255 });
            DrawRectangleLines(14, 160, 140, 32, Color{ 60, 210, 100, 240 });
            DrawText("[3] CASH  $60", 24, 168, 14, Color{ 160, 255, 180, 255 });

            DrawRectangle(166, 160, 140, 32, Color{ 14, 42, 26, 255 });
            DrawRectangleLines(166, 160, 140, 32, Color{ 60, 210, 100, 240 });
            DrawTextSharp(g_fontMenu, "[ESC] EXIT / EJECT", 172, 168, 12, Color{ 255, 170, 140, 255 });
        } break;

        case ATM_DISPENSING_CASH: {
            DrawText("COUNTING BILLS...", 88, 85, 16, Color{ 140, 255, 175, 255 });
            DrawRectangle(40, 115, 240, 18, Color{ 14, 38, 24, 255 });
            DrawRectangle(42, 117, (int)(236.0f * g_atmCashAnim), 14, Color{ 75, 250, 130, 255 });
            DrawText("MECHANICAL ROLLER ACTIVE", 74, 150, 12, Color{ 110, 230, 140, 220 });
        } break;

        case ATM_COLLECT_CASH: {
            float blink = sinf(timeVal * 6.0f) * 0.5f + 0.5f;
            DrawText("TRANSACTION COMPLETE", 64, 55, 16, Color{ 150, 255, 185, 255 });
            char disBuf[64];
            snprintf(disBuf, sizeof(disBuf), "DISPENSED: $%.2f", g_atmWithdrawAmount);
            DrawText(disBuf, 95, 82, 16, Color{ 230, 255, 190, 255 });

            DrawRectangle(20, 118, 280, 50, Color{ 18, 55, 32, 255 });
            DrawRectangleLines(20, 118, 280, 50, Color{ 80, 250, 140, (unsigned char)(255 * blink) });
            DrawText("PLEASE TAKE YOUR CASH", 54, 128, 15, Color{ 220, 255, 220, (unsigned char)(255 * blink) });
            DrawText("FROM DISPENSER TRAY", 68, 146, 14, Color{ 220, 255, 220, (unsigned char)(255 * blink) });

            DrawText("[E] COLLECT CASH BILLS", 78, 186, 13, Color{ 130, 255, 170, 255 });
        } break;

        case ATM_EJECTING_CARD: {
            float blink = sinf(timeVal * 5.0f) * 0.5f + 0.5f;
            DrawText("PLEASE TAKE YOUR CARD", 60, 80, 16, Color{ 150, 255, 180, (unsigned char)(255 * blink) });
            DrawText("FROM THE CARD READER", 66, 105, 15, Color{ 150, 255, 180, (unsigned char)(255 * blink) });
            DrawText("THANK YOU FOR BANKING WITH US", 40, 155, 13, Color{ 90, 220, 130, 230 });
            DrawText("[E] RETRIEVE CARD", 100, 185, 12, Color{ 120, 240, 150, 255 });
        } break;

        default:
            break;
    }

    // Screen border bevel
    DrawRectangleLines(0, 0, 320, 240, Color{ 30, 80, 50, 255 });
    EndTextureMode();
}

void DrawATM3D(ShopLightFn lightFn, bool lightsOn, float timeVal) {
    if (!g_atmLoaded) return;

    // First, update CRT screen texture

    Vector3 p = g_atmPos; // Center base on floor { 113.35f, 10.0f, 143.5f }

    // -------------------------------------------------------------------------
    // 1. LOWER HEAVY STEEL VAULT SAFE CASING (Dark industrial charcoal steel)
    // -------------------------------------------------------------------------
    Vector3 vaultP = { p.x, p.y + 0.50f, p.z };
    Color vaultCol = lightFn(vaultP, Color{ 38, 42, 48, 255 }, Vector3{ -1.0f, 0.0f, 0.0f }, 0);
    DrawCube(vaultP, 0.65f, 1.00f, 0.78f, vaultCol);
    DrawCubeWires(vaultP, 0.652f, 1.002f, 0.782f, lightFn(vaultP, Color{ 24, 26, 30, 255 }, Vector3{ -1,0,0 }, 0));

    // Vault door seam & heavy hinges on side
    DrawCube(Vector3{ p.x - 0.328f, p.y + 0.50f, p.z }, 0.015f, 0.94f, 0.72f, lightFn(vaultP, Color{ 28, 30, 34, 255 }, Vector3{ -1,0,0 }, 0));
    DrawCylinder(Vector3{ p.x - 0.32f, p.y + 0.25f, p.z - 0.35f }, 0.022f, 0.022f, 0.12f, 8, lightFn(vaultP, Color{ 60, 65, 72, 255 }, Vector3{ -1,0,0 }, 0));
    DrawCylinder(Vector3{ p.x - 0.32f, p.y + 0.75f, p.z - 0.35f }, 0.022f, 0.022f, 0.12f, 8, lightFn(vaultP, Color{ 60, 65, 72, 255 }, Vector3{ -1,0,0 }, 0));

    // -------------------------------------------------------------------------
    // 2. UPPER FASCIA CONSOLE (Housing screen, keypad, card reader, cash dispenser)
    // -------------------------------------------------------------------------
    Vector3 fasciaP = { p.x, p.y + 1.35f, p.z };
    Color fasciaCol = lightFn(fasciaP, Color{ 48, 54, 62, 255 }, Vector3{ -1.0f, 0.0f, 0.0f }, 0);
    DrawCube(fasciaP, 0.60f, 0.70f, 0.76f, fasciaCol);
    DrawCubeWires(fasciaP, 0.602f, 0.702f, 0.762f, lightFn(fasciaP, Color{ 30, 34, 40, 255 }, Vector3{ -1,0,0 }, 0));

    // -------------------------------------------------------------------------
    // 3. TOP ILLUMINATED MARQUEE LIGHTBOX ("ATM")
    // -------------------------------------------------------------------------
    Vector3 topP = { p.x, p.y + 1.78f, p.z };
    DrawCube(topP, 0.56f, 0.18f, 0.74f, lightFn(topP, Color{ 35, 40, 46, 255 }, Vector3{ -1,0,0 }, 0));

    // Translucent illuminated face facing -X
    Vector3 signP = { p.x - 0.285f, topP.y, topP.z };
    Color signCol = lightsOn ? Color{ 220, 245, 255, 255 } : Color{ 90, 105, 115, 255 };
    DrawCube(signP, 0.02f, 0.14f, 0.68f, signCol);
    // Dark bold "ATM" badge letters embossed on front
    DrawCube(Vector3{ signP.x - 0.012f, signP.y, signP.z }, 0.01f, 0.09f, 0.32f, Color{ 16, 52, 115, 255 });
    DrawCube(Vector3{ signP.x - 0.014f, signP.y, signP.z }, 0.006f, 0.07f, 0.28f, Color{ 245, 250, 255, 255 });

    // Soft cyan halo in front of marquee when lit
    if (lightsOn) {
        DrawSphere(Vector3{ signP.x - 0.15f, signP.y, signP.z }, 0.22f, Color{ 140, 220, 255, 35 });
    }

    // -------------------------------------------------------------------------
    // 4. CRT DISPLAY SCREEN QUAD
    // -------------------------------------------------------------------------
    // Recessed screen at Y = 11.42, X = 113.04, facing -X
    Vector3 screenP = { p.x - 0.305f, p.y + 1.44f, p.z };
    // Bezel border
    DrawCube(screenP, 0.02f, 0.38f, 0.52f, Color{ 24, 28, 32, 255 });

    // CRT Screen Face (Using g_atmScreenRT)
    float scrHalfH = 0.16f;
    float scrHalfW = 0.22f;
    float sx = screenP.x - 0.012f;

    rlSetTexture(g_atmScreenRT.texture.id);
    rlBegin(RL_QUADS);
    rlNormal3f(-1.0f, 0.0f, 0.0f);
    rlColor4ub(255, 255, 255, 255);
    // Note: Raylib render textures are vertically flipped
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(sx, screenP.y + scrHalfH, screenP.z - scrHalfW);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f(sx, screenP.y + scrHalfH, screenP.z + scrHalfW);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f(sx, screenP.y - scrHalfH, screenP.z + scrHalfW);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(sx, screenP.y - scrHalfH, screenP.z - scrHalfW);
    rlEnd();
    rlSetTexture(0);

    // Soft green phosphor glow emanating from CRT onto user's chest
    DrawSphere(Vector3{ sx - 0.12f, screenP.y, screenP.z }, 0.28f, Color{ 40, 255, 120, 25 });

    // Side softkey buttons flanking the screen (4 on each side)
    for (int k = 0; k < 4; k++) {
        float ky = screenP.y + 0.12f - (float)k * 0.08f;
        // Left side buttons (at +Z)
        DrawCube(Vector3{ sx + 0.005f, ky, screenP.z + 0.235f }, 0.015f, 0.025f, 0.018f, Color{ 75, 80, 88, 255 });
        // Right side buttons (at -Z)
        DrawCube(Vector3{ sx + 0.005f, ky, screenP.z - 0.235f }, 0.015f, 0.025f, 0.018f, Color{ 75, 80, 88, 255 });
    }

    // -------------------------------------------------------------------------
    // 5. METALLIC TACTILE KEYPAD WITH PRIVACY SHIELD
    // -------------------------------------------------------------------------
    Vector3 padP = { p.x - 0.315f, p.y + 1.12f, p.z };
    // Brushed aluminum sloped key base plate
    DrawCube(padP, 0.02f, 0.18f, 0.24f, lightFn(padP, Color{ 145, 150, 158, 255 }, Vector3{ -1,0,0 }, 0));

    // Left and right privacy wings
    DrawCube(Vector3{ padP.x - 0.02f, padP.y, padP.z - 0.125f }, 0.05f, 0.18f, 0.012f, lightFn(padP, Color{ 42, 46, 52, 255 }, Vector3{ -1,0,0 }, 0));
    DrawCube(Vector3{ padP.x - 0.02f, padP.y, padP.z + 0.125f }, 0.05f, 0.18f, 0.012f, lightFn(padP, Color{ 42, 46, 52, 255 }, Vector3{ -1,0,0 }, 0));

    // 12 Tactile buttons (3 cols x 4 rows)
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 3; c++) {
            float by = padP.y + 0.06f - (float)r * 0.04f;
            float bz = padP.z - 0.06f + (float)c * 0.06f;
            Color btnCol = Color{ 55, 60, 68, 255 };
            if (r == 3 && c == 0) btnCol = Color{ 210, 65, 55, 255 };   // CANCEL [RED]
            if (r == 3 && c == 1) btnCol = Color{ 220, 195, 45, 255 };  // CLEAR [YELLOW]
            if (r == 3 && c == 2) btnCol = Color{ 45, 185, 80, 255 };   // ENTER [GREEN]
            DrawCube(Vector3{ padP.x - 0.012f, by, bz }, 0.010f, 0.022f, 0.035f, btnCol);
        }
    }

    // -------------------------------------------------------------------------
    // 6. CARD READER SLOT & TRANSLUCENT EMERALD GREEN BEZEL
    // -------------------------------------------------------------------------
    Vector3 cardSlotP = { p.x - 0.312f, p.y + 1.14f, p.z - 0.22f };
    // Translucent emerald plastic bezel
    Color bezelCol = Color{ 35, 175, 85, 220 };
    DrawCube(cardSlotP, 0.025f, 0.08f, 0.07f, bezelCol);
    // Narrow horizontal card mouth
    DrawCube(Vector3{ cardSlotP.x - 0.014f, cardSlotP.y, cardSlotP.z }, 0.005f, 0.012f, 0.055f, Color{ 12, 14, 16, 255 });

    // Pulsing green LED indicator on card reader
    float ledPulse = (g_atmState == ATM_IDLE) ? (sinf(timeVal * 4.0f) * 0.5f + 0.5f) : 1.0f;
    Color ledCol = (g_atmState == ATM_ENTER_PIN || g_atmState == ATM_AUTHENTICATING)
        ? Color{ 40, 245, 110, 255 }
        : Color{ 35, 230, 95, (unsigned char)(255 * ledPulse) };
    DrawSphere(Vector3{ cardSlotP.x - 0.016f, cardSlotP.y + 0.025f, cardSlotP.z }, 0.008f, ledCol);
    DrawSphere(Vector3{ cardSlotP.x - 0.016f, cardSlotP.y + 0.025f, cardSlotP.z }, 0.020f, Color{ ledCol.r, ledCol.g, ledCol.b, 60 });

    // 3D Debit Card Animation
    if (g_atmCardAnim > 0.01f) {
        // Card slides into/out of slot along X axis
        float cardX = cardSlotP.x - 0.06f + (g_atmCardAnim * 0.06f);
        Vector3 cardPos = { cardX, cardSlotP.y, cardSlotP.z };
        // Navy blue plastic card body (8.5cm x 5.4cm)
        DrawCube(cardPos, 0.075f, 0.003f, 0.052f, Color{ 22, 60, 130, 255 });
        // Metallic gold EMV chip
        DrawCube(Vector3{ cardPos.x - 0.015f, cardPos.y + 0.002f, cardPos.z - 0.008f }, 0.014f, 0.002f, 0.012f, Color{ 225, 195, 65, 255 });
        // White signature / network stripe
        DrawCube(Vector3{ cardPos.x, cardPos.y + 0.002f, cardPos.z + 0.014f }, 0.065f, 0.002f, 0.010f, Color{ 235, 235, 240, 255 });
    }

    // -------------------------------------------------------------------------
    // 7. RECEIPT DISPENSER SLIT
    // -------------------------------------------------------------------------
    Vector3 recSlotP = { p.x - 0.312f, p.y + 1.14f, p.z + 0.22f };
    DrawCube(recSlotP, 0.015f, 0.03f, 0.08f, Color{ 35, 38, 44, 255 });
    DrawCube(Vector3{ recSlotP.x - 0.008f, recSlotP.y, recSlotP.z }, 0.005f, 0.006f, 0.065f, Color{ 10, 12, 14, 255 });

    // -------------------------------------------------------------------------
    // 8. MOTORIZED CASH DISPENSER SHUTTER & DISPENSED BILLS
    // -------------------------------------------------------------------------
    Vector3 cashSlotP = { p.x - 0.318f, p.y + 0.88f, p.z };
    // Shutter frame
    DrawCube(cashSlotP, 0.022f, 0.06f, 0.32f, lightFn(cashSlotP, Color{ 32, 35, 40, 255 }, Vector3{ -1,0,0 }, 0));
    // Internal dark slot
    DrawCube(Vector3{ cashSlotP.x - 0.012f, cashSlotP.y, cashSlotP.z }, 0.005f, 0.028f, 0.26f, Color{ 8, 10, 12, 255 });

    // Dispensed Cash Bills (Animated protruding stack of green US currency)
    if (g_atmCashAnim > 0.02f) {
        float billProtrusion = g_atmCashAnim * 0.11f; // Slides out 11cm towards player (-X)
        Vector3 billP = { cashSlotP.x - (billProtrusion * 0.5f), cashSlotP.y, cashSlotP.z };

        // Stack thickness (representing multiple $20 notes)
        DrawCube(billP, billProtrusion, 0.012f, 0.16f, Color{ 45, 95, 55, 255 });
        // Top currency bill detail (crisp greenback olive tone with border)
        DrawCube(Vector3{ billP.x, billP.y + 0.007f, billP.z }, billProtrusion * 0.96f, 0.002f, 0.155f, Color{ 68, 145, 82, 255 });
        DrawCube(Vector3{ billP.x, billP.y + 0.008f, billP.z }, billProtrusion * 0.75f, 0.002f, 0.12f, Color{ 90, 185, 105, 255 });
        // Oval treasury seal
        DrawCube(Vector3{ billP.x - (billProtrusion * 0.2f), billP.y + 0.009f, billP.z }, 0.020f, 0.002f, 0.035f, Color{ 32, 75, 42, 255 });
    }

    // -------------------------------------------------------------------------
    // 9. GROUND CONTACT SHADOW
    // -------------------------------------------------------------------------
    DrawCircle3D(Vector3{ p.x, p.y + 0.012f, p.z }, 0.55f, Vector3{ 1, 0, 0 }, 90.0f, Color{ 8, 10, 14, 110 });
}

void DrawATMOverlay2D() {
    if (!g_isInteractingWithATM) return;

    // Sleek diegetic guidance pill at bottom-center
    const char* hint = nullptr;
    Color accentCol = Color{ 80, 230, 130, 255 };

    switch (g_atmState) {
        case ATM_INSERTING_CARD:
            hint = "FEEDING CARD INTO READER... PLEASE WAIT";
            accentCol = Color{ 140, 220, 255, 255 };
            break;
        case ATM_ENTER_PIN:
            hint = "PRESS [0 - 9] ON KEYBOARD / NUMPAD   |   [ENTER] SUBMIT   |   [ESC] CANCEL";
            accentCol = Color{ 255, 225, 120, 255 };
            break;
        case ATM_AUTHENTICATING:
            hint = "* ENCRYPTED BANK NETWORK HANDSHAKE *";
            accentCol = Color{ 120, 240, 150, 255 };
            break;
        case ATM_MENU:
            hint = "PRESS [1] $20   [2] $40   [3] $60   [4] $100   [5] $200   |   [ESC] RETURN CARD";
            accentCol = Color{ 130, 240, 160, 255 };
            break;
        case ATM_DISPENSING_CASH:
            hint = "* DISPENSER STEPPER MOTOR COUNTING BILLS... *";
            accentCol = Color{ 255, 195, 80, 255 };
            break;
        case ATM_COLLECT_CASH:
            hint = "[E] / [LMB] TAKE CASH FROM DISPENSER TRAY";
            accentCol = Color{ 60, 255, 130, 255 };
            break;
        case ATM_INSUFFICIENT_FUNDS:
            hint = "ERROR: INSUFFICIENT ACCOUNT BALANCE";
            accentCol = Color{ 255, 80, 80, 255 };
            break;
        case ATM_EJECTING_CARD:
            hint = "[E] TAKE YOUR CARD FROM READER";
            accentCol = Color{ 120, 220, 255, 255 };
            break;
        default:
            break;
    }

    if (hint) {
        float tw = MeasureTextSharp(g_fontMenu, hint, 16.0f);
        float pw = tw + 64.0f;
        float px = (1280.0f - pw) * 0.5f;
        float py = 720.0f - 80.0f;
        
        // Use our beautiful AAA UI panel for ATM interactions
        DrawAAAPanel(Rectangle{ px, py, pw, 48.0f }, Color{ 12, 16, 22, 230 }, accentCol, 8.0f, true);
        DrawTextSharpCentered(g_fontMenu, hint, 640.0f, py + 16.0f, 16.0f, WHITE);
    }
}
