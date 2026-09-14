#pragma once

#include <raylib.h>
#include <raymath.h>
#include <vector>
#include <functional>

// =========================================================================
// SURVEILLANCE CCTV SYSTEM & CRT OVERLAYS
// - Rooftop motorized sliding CCTV box camera & physical mounting rail
// - In-game surveillance CRT HUD with live telemetry, scanlines, targeting
// - Main menu surveillance camera switcher (Portico, Courtyard, Vestibule)
// - Mini CRT surveillance monitor on supermarket checkout counter
// =========================================================================

extern bool g_isRoofCamActive;
extern float g_roofCamSlideX;   // Sliding along front roof wall rail (-3.2 to +3.2)
extern float g_roofCamPitch;    // Tilt angle (-65 to +28 deg)
extern float g_roofCamYaw;      // Pan angle (-85 to +85 deg)
extern float g_roofCamFOV;      // Optical zoom (15 to 80 deg)

extern int   g_menuCCTVFeed;     // 0: Portico Arch, 1: Courtyard, 2: Vestibule
extern float g_cctvSwitchGlitch; // Video scanline flutter timer

#ifndef isRoofCamActive
#define isRoofCamActive g_isRoofCamActive
#endif
#ifndef roofCamSlideX
#define roofCamSlideX g_roofCamSlideX
#endif
#ifndef roofCamPitch
#define roofCamPitch g_roofCamPitch
#endif
#ifndef roofCamYaw
#define roofCamYaw g_roofCamYaw
#endif
#ifndef roofCamFOV
#define roofCamFOV g_roofCamFOV
#endif

void UpdateRoofCCTV(float dt, float timeVal, bool isCursorCaptured, bool showQuitConfirm, bool isShopOpen);
void SetupRoofCCTVCamera(Camera3D &renderCam, const Camera3D &playerCam, std::vector<Matrix> playerInstances[256]);
void DrawPhysicalCCTVCameraAssembly(float timeVal);
void DrawCCTVSurveillanceOverlay(float timeVal);

void UpdateMenuCCTV(float dt, float wheelMove, bool modalOpen, Sound sndStatic);
void DrawMenuCCTVOverlay(int screenW, int screenH, Vector2 mPos, float dt, Sound sndStatic);
void GetMenuCCTVCamera(int feedIndex, float timeVal, float smoothX, float smoothY, Vector3 &outPos, Vector3 &outTarget);

void DrawMiniCRTSurveillanceMonitor(Vector3 crtPos, float timeVal, std::function<Color(Vector3, Color)> applyLighting);
