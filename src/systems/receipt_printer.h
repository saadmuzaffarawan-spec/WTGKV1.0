#pragma once
#include <raylib.h>
#include <functional>

enum PrinterState { PRINTER_IDLE = 0, PRINTER_PRINTING, PRINTER_DONE };
extern PrinterState g_printerState;
extern float g_printerProgress;

// Lighting callback for in-world printer chassis and parts
typedef std::function<Color(Vector3 pos, Color baseAlbedo)> PrinterLightingFn;

// Builds the procedurally generated authentic supermarket thermal horror receipt texture
Texture2D BuildReceiptTexture(void);

// Renders the 3D countertop horror receipt printer with chassis, buttons, LED, and blood trails
void DrawPrinter(Vector3 pos, float ledGlow, float t, PrinterLightingFn lightFn = nullptr);

// Renders the 3D physics-curling paper output chain segment from the printer slot or floor
void DrawReceiptChain(Vector3 slotPos, Texture2D tex, float revealedSegments, float time);

