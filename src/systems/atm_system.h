#pragma once
#include <raylib.h>
#include <raymath.h>
#include <functional>

// =============================================================================
// WHAT THE GROUND KEEPS - HYPER-REALISTIC CONVENIENCE STORE ATM SYSTEM
// - Diebold/NCR style convenience store cash terminal near the front entrance
// - Backlit illuminated ATM marquee with soft glow
// - Recessed 9-inch green phosphor CRT terminal with authentic bank telemetry
// - Tactile metallic numeric keypad with DTMF audio tone beeps
// - Card reader slot with flashing translucent emerald green LED bezel
// - Physical 3D Debit Card animation into and out of reader slot
// - Motorized cash dispenser shutter with mechanical stepper bill roller sounds
// - Physically animated cash currency bills protruding from dispenser tray
// - Full authentic state machine: Card Insert -> PIN -> Balance -> Dispense -> Collect -> Eject
// =============================================================================

typedef std::function<Color(Vector3 pos, Color baseAlbedo, Vector3 normal, int occludeAisle)> ShopLightFn;

enum ATMState {
    ATM_IDLE,             // Attractor loop screen, green card LED pulsing
    ATM_INSERTING_CARD,   // Debit card animating into slot, roller feed sound
    ATM_ENTER_PIN,        // Masked 4-digit PIN input, keypad DTMF audio feedback
    ATM_AUTHENTICATING,   // Network communication spinner
    ATM_MENU,             // Account checking, fast cash $20, $40, $60, $100, $200
    ATM_DISPENSING_CASH,  // Stepper motor whir, bills slide out of shutter slot
    ATM_COLLECT_CASH,     // Cash protruding from slot, prompt [E] TAKE CASH
    ATM_EJECTING_CARD,    // Card slides out of reader, reminder chime
    ATM_INSUFFICIENT_FUNDS, // Error screen when withdrawing more than balance
    ATM_INACTIVE
};

void InitATMSystem();
void UpdateATMSystem(float dt, Vector3 playerPos, bool &isPlayerInteracting);
void PreRenderATMScreen(float timeVal);
void DrawATM3D(ShopLightFn lightFn, bool lightsOn, float timeVal);
void DrawATMOverlay2D();
bool IsPlayerNearATM(Vector3 playerPos);
void StartATMInteraction();
void CloseATMInteraction();
bool IsATMActive();
void UnloadATMSystem();

