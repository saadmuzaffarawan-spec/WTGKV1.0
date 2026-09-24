// Main menu, pause menu and settings screens (shared visual language with the HUD).
#pragma once

void MenuEnter();
void MenuUpdate(float dt);          // main menu (Mode::Menu)
void MenuDraw();
void PauseEnter();
void PauseUpdate(float dt);         // Mode::Pause
void PauseDraw();
void CreditsEnter();
void CreditsUpdate(float dt);
void CreditsDraw();
void DrawLoadingScreen(float progress, const char* status);
// camera used behind the main menu
void MenuCamera();
