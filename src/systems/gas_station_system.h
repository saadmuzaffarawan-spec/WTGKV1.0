#pragma once
#include <raylib.h>

// =========================================================================
// GAS STATION & ROUTE 9 CUSTOMER CAR SYSTEM
// =========================================================================

enum CustomerCarState {
    CAR_INACTIVE = 0,
    CAR_APPROACHING,
    CAR_PARKED,
    CAR_REFUELING,
    CAR_PAID,
    CAR_DEPARTING
};

struct CustomerCar {
    CustomerCarState state;
    Vector3 pos;
    float speed;
    int targetPump;          // 0 = Pump 1 (Z=137.5), 1 = Pump 2 (Z=142.5)
    float targetZ;
    float requestedGallons;
    float dispensedGallons;
    float totalSale;
    float tipAmount;
    Color bodyColor;
    float waitTimer;
    char driverDialogue[128];
};

// Spawns a customer car approaching from Route 9 North with randomized model color and driver dialogue
void SpawnCustomerCar(CustomerCar& car, float fuelPricePerGallon = 3.89f);

// Renders a hanging catenary rubber fuel hose with sag and brass end fittings
void DrawCatenaryHose(Vector3 start, Vector3 end, float maxSag, int segments, float radius, Color col);

// Renders the first-person fuel nozzle viewmodel in player's hands with step bob, breathing, and vapor particles
void DrawFirstPersonFuelNozzle(Camera3D cam, bool isFlowing, float walkTime, float timeVal);

// Renders the 3D customer car parked or driving on Route 9, including fuel flap and connected nozzle hose
void DrawCustomerCar(const CustomerCar& car, bool nozzleInCar);

// Updates the in-world 3D Fuel Pump CRT screen render texture with retro phosphorescent raster scanlines,
// octane grade badge, digital meter price/gallon readouts, and dynamic flow bargraph
void UpdatePumpCrtTextureEx(RenderTexture2D rt, bool rtLoaded, Font fontSmall, Font fontTitle, int pumpNum,
                            float gallons, float salePrice, bool isFlowing, float fuelPricePerGallon,
                            float stationFuelGallons, float flk);

// Draws the textured 3D quad for the CRT display screen on the fuel dispenser face
void DrawPumpCrtScreen3D(Vector3 center, float width, float height, Texture2D tex, bool faceWest, Color tint = WHITE);

