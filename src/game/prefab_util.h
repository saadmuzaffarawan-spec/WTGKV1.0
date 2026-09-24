// Helpers shared by prefab builders.
#pragma once
#include "engine/scene.h"
#include "engine/ui.h"
#include "engine/materials.h"

// Weathered sign texture: text on a background, then faded, streaked and chipped.
Texture2D MakeSignTexture(const char* text, ui::FontId font, float fontSize, Color fg, Color bg,
                          int w, int h, float weather, uint32_t seed, int align = 0 /*0 centre,1 left*/);
// Sign material from a text texture (cached by key)
int SignMaterial(const std::string& key, const char* text, ui::FontId font, float fontSize, Color fg, Color bg,
                 int w, int h, float weather, bool emissive = false, float strength = 1.0f);

// Recursive branch generator for dead trees and bushes
// Branches at this depth and deeper are not emitted (used to build distance LODs).
extern int g_branchLodSkip;
void GrowBranch(ModelBuilder& mb, Rng& rng, Vector3 base, Vector3 dir, float len, float rad, int depth, int maxDepth, int mat);

// Catenary points between two anchors
std::vector<Vector3> Catenary(Vector3 a, Vector3 b, float sag, int segs);

struct Opening { float at; float width; float sill; float top; };   // along-wall centre offset from A
// Wall from a to b (local XZ, y = base height) with openings; outer layer faces the left of a->b.
void BuildWall(PrefabBuild& b, Vector2 a, Vector2 bpt, float y0, float height, float thick,
               int matOut, int matIn, std::vector<Opening> openings, int surf = SURF_CONCRETE, float baseboard = 0.0f);
// Glass pane with mullion frame filling an opening
void BuildWindow(PrefabBuild& b, Vector2 a, Vector2 bpt, float y0, const Opening& o, int glassMat, int frameMat, int panesX = 2, bool broken = false, bool boarded = false);

// Cow skeleton parts (shared by the field prefab and the story's walking skeleton)
enum CowPart { COW_BODY, COW_HEAD, COW_UPPER, COW_LOWER };
void CowPartGeometry(ModelBuilder& mb, int part, uint32_t seed);
Vector3 CowHip(int leg);   // leg joint in the body frame: 0 FL, 1 FR, 2 BL, 3 BR

void RegisterDoorPrefabs();
void RegisterEnvPrefabs();
void RegisterStationPrefabs();
void RegisterStorePrefabs();
void RegisterCrashPrefabs();
void RegisterCollegePrefabs();
void RegisterFieldPrefabs();
void RegisterUnderPrefabs();
void BuildPowerLines(Scene& scene);
