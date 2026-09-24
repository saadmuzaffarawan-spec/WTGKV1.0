#pragma once

#include <raylib.h>

// =========================================================================
// PROCEDURAL SUPERSTORE PRODUCT & ASSET ENGINE
// Populates supermarket shelves with real interactive 3D products:
// - Popcorn Tins, Sparrow Milk & B+ Blood Bottles, Baked Bread Loaves,
// - Chocolate Bars, Shopping Carts, Tactical 9mm Pistol
// =========================================================================

extern Model g_milkLiquidModel;
extern Model g_bloodLiquidModel;
extern Model g_breadModel;
extern bool g_proceduralAssetsLoaded;

void InitProceduralShopAssets();
void UnloadProceduralShopAssets();
