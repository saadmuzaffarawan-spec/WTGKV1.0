#pragma once
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>
#include <math.h>

// =========================================================================

// HYBRID RAYLIB SHAPE + DYNAMIC ASCII CLOUD SYSTEM

// Formations: Cumulus Humilis, Cumulonimbus Shelf Storm, Cirrus Wisps

// Physics: Wind Advection, Altitude Shear, Thermal Boiling Turbulence

// Optics: Direct Solar/Lunar, Beer-Lambert Absorption, Mie Rim ("Silver Lining")

// =========================================================================

enum CloudType {

    CLOUD_CUMULUS = 0,

    CLOUD_STORM_SHELF,

    CLOUD_CIRRUS

};



struct CloudPuff {

    Vector3 localPos;

    Vector3 size;

    float density;       // 0.2 (evaporating wisp) to 1.0 (dense core)

    float boilPhase;     // phase offset for thermal boiling

    float boilFreq;      // frequency of turbulence

    char glyph;          // '@', '#', '8', 'O', '0', '&', '~', '.'

};



struct CloudCluster {

    CloudType type;

    Vector3 pos;         // Center position

    Vector3 vel;         // Drift velocity

    float boundingRadius;

    std::vector<CloudPuff> puffs;

    float baseAltitude;

    float internalFlash; // Intra-cloud lightning flash

};



std::vector<CloudCluster> g_cloudClusters;



static inline Color BlendColors(Color c1, Color c2, float t) {

    t = Clamp(t, 0.0f, 1.0f);

    return (Color){

        (unsigned char)(c1.r + (c2.r - c1.r) * t),

        (unsigned char)(c1.g + (c2.g - c1.g) * t),

        (unsigned char)(c1.b + (c2.b - c1.b) * t),

        (unsigned char)(c1.a + (c2.a - c1.a) * t)

    };

}



void InitCloudSystem() {

    g_cloudClusters.clear();

    g_cloudClusters.reserve(16);

    

    // 1. CUMULUS HUMILIS (7 Fair-Weather Clusters, Alt 78 - 105m)

    struct CumulusSeed {

        Vector3 pos;

        float radius;

        Vector3 vel;

    } cumulusSeeds[7] = {

        { {  50.0f,  84.0f,  80.0f }, 28.0f, { 3.4f, 0.0f, 1.2f } },

        { { 165.0f,  92.0f,  40.0f }, 32.0f, { 3.2f, 0.0f, 1.1f } },

        { { 235.0f,  88.0f, 175.0f }, 26.0f, { 3.5f, 0.0f, 1.3f } },

        { { -30.0f,  80.0f, 215.0f }, 30.0f, { 3.3f, 0.0f, 1.2f } },

        { { 105.0f,  96.0f, 285.0f }, 25.0f, { 3.6f, 0.0f, 1.4f } },

        { { 295.0f,  90.0f, 315.0f }, 29.0f, { 3.1f, 0.0f, 1.1f } },

        { {  35.0f,  82.0f, -45.0f }, 27.0f, { 3.4f, 0.0f, 1.2f } }

    };

    

    for (int i = 0; i < 7; i++) {

        CloudCluster cl;

        cl.type = CLOUD_CUMULUS;

        cl.pos = cumulusSeeds[i].pos;

        cl.vel = cumulusSeeds[i].vel;

        cl.boundingRadius = cumulusSeeds[i].radius;

        cl.baseAltitude = cumulusSeeds[i].pos.y;

        cl.internalFlash = 0.0f;

        

        // Base flat layer (lifting condensation level)

        for (int b = 0; b < 6; b++) {

            float ang = (float)b * (2.0f * PI / 6.0f);

            float r = cumulusSeeds[i].radius * 0.45f;

            CloudPuff p;

            p.localPos = (Vector3){ cosf(ang) * r, -1.8f, sinf(ang) * r };

            p.size = (Vector3){ 16.0f, 8.5f, 16.0f };

            p.density = 0.85f;

            p.boilPhase = (float)(b * 1.05f);

            p.boilFreq = 0.8f + (b % 3) * 0.2f;

            p.glyph = (b % 2 == 0) ? '@' : '#';

            cl.puffs.push_back(p);

        }

        // Center buoyant thermal core

        {

            CloudPuff p;

            p.localPos = (Vector3){ 0.0f, 1.5f, 0.0f };

            p.size = (Vector3){ 18.0f, 12.0f, 18.0f };

            p.density = 0.95f;

            p.boilPhase = 0.5f;

            p.boilFreq = 1.0f;

            p.glyph = '@';

            cl.puffs.push_back(p);

        }

        // Bubbling upper turrets (cauliflower head)

        for (int t = 0; t < 5; t++) {

            float ang = (float)t * (2.0f * PI / 5.0f) + 0.35f;

            float r = cumulusSeeds[i].radius * 0.32f;

            CloudPuff p;

            p.localPos = (Vector3){ cosf(ang) * r, 4.2f, sinf(ang) * r };

            p.size = (Vector3){ 13.5f, 10.0f, 13.5f };

            p.density = 0.65f;

            p.boilPhase = (float)(t * 1.35f + 1.2f);

            p.boilFreq = 1.1f + (t % 2) * 0.3f;

            p.glyph = (t % 2 == 0) ? 'O' : '0';

            cl.puffs.push_back(p);

        }

        // Evaporating outer wisps

        for (int w = 0; w < 4; w++) {

            float ang = (float)w * (2.0f * PI / 4.0f) + 0.75f;

            float r = cumulusSeeds[i].radius * 0.78f;

            CloudPuff p;

            p.localPos = (Vector3){ cosf(ang) * r, 0.5f, sinf(ang) * r };

            p.size = (Vector3){ 10.0f, 6.0f, 10.0f };

            p.density = 0.35f;

            p.boilPhase = (float)(w * 1.7f + 2.1f);

            p.boilFreq = 1.4f;

            p.glyph = (w % 2 == 0) ? '~' : '.';

            cl.puffs.push_back(p);

        }

        g_cloudClusters.push_back(cl);

    }

    

    // 2. CUMULONIMBUS / STORM SHELF SUPERCELL (3 Ominous Clusters)

    struct StormSeed {

        Vector3 pos;

        float radius;

        Vector3 vel;

    } stormSeeds[3] = {

        { { 120.0f,  64.0f, 140.0f }, 58.0f, { 2.1f, 0.0f, 0.7f } },

        { { 230.0f,  72.0f, 270.0f }, 64.0f, { 2.2f, 0.0f, 0.8f } },

        { { -50.0f,  68.0f,  60.0f }, 52.0f, { 2.0f, 0.0f, 0.7f } }

    };

    

    for (int i = 0; i < 3; i++) {

        CloudCluster cl;

        cl.type = CLOUD_STORM_SHELF;

        cl.pos = stormSeeds[i].pos;

        cl.vel = stormSeeds[i].vel;

        cl.boundingRadius = stormSeeds[i].radius;

        cl.baseAltitude = stormSeeds[i].pos.y;

        cl.internalFlash = 0.0f;

        

        // Dark bruised underbelly

        for (int b = 0; b < 8; b++) {

            float ang = (float)b * (2.0f * PI / 8.0f);

            float r = stormSeeds[i].radius * 0.55f;

            CloudPuff p;

            p.localPos = (Vector3){ cosf(ang) * r, -5.0f, sinf(ang) * r };

            p.size = (Vector3){ 26.0f, 14.0f, 26.0f };

            p.density = 0.98f;

            p.boilPhase = (float)(b * 0.9f);

            p.boilFreq = 0.7f;

            p.glyph = '#';

            cl.puffs.push_back(p);

        }

        // Central column

        for (int c = 0; c < 6; c++) {

            float yOff = (float)c * 4.5f;

            float rScale = 1.0f + (float)c * 0.12f;

            CloudPuff p;

            p.localPos = (Vector3){ ((c % 2 == 0) ? -2.0f : 2.0f), yOff, ((c % 2 == 0) ? 2.0f : -2.0f) };

            p.size = (Vector3){ 28.0f * rScale, 16.0f, 28.0f * rScale };

            p.density = 0.95f;

            p.boilPhase = (float)(c * 1.1f + 0.4f);

            p.boilFreq = 0.85f;

            p.glyph = '%';

            cl.puffs.push_back(p);

        }

        // Spreading anvil head

        for (int a = 0; a < 6; a++) {

            float ang = (float)a * (2.0f * PI / 6.0f);

            float r = stormSeeds[i].radius * 0.75f;

            CloudPuff p;

            p.localPos = (Vector3){ cosf(ang) * r, 24.0f, sinf(ang) * r * 1.3f };

            p.size = (Vector3){ 25.0f, 8.0f, 32.0f };

            p.density = 0.75f;

            p.boilPhase = (float)(a * 1.25f + 2.0f);

            p.boilFreq = 0.9f;

            p.glyph = 'W';

            cl.puffs.push_back(p);

        }

        // Turbulent shelf rim

        for (int r = 0; r < 4; r++) {

            float ang = (float)r * (2.0f * PI / 4.0f) + 0.4f;

            CloudPuff p;

            p.localPos = (Vector3){ cosf(ang) * stormSeeds[i].radius * 0.85f, -2.0f, sinf(ang) * stormSeeds[i].radius * 0.85f };

            p.size = (Vector3){ 18.0f, 10.0f, 18.0f };

            p.density = 0.50f;

            p.boilPhase = (float)(r * 1.5f + 3.0f);

            p.boilFreq = 1.2f;

            p.glyph = '&';

            cl.puffs.push_back(p);

        }

        g_cloudClusters.push_back(cl);

    }

    

    // 3. CIRRUS / STRATUS WISPS (4 High-Altitude Wind-Sheared Veils)

    struct CirrusSeed {

        Vector3 pos;

        float radius;

        Vector3 vel;

    } cirrusSeeds[4] = {

        { {  45.0f, 160.0f, 110.0f }, 46.0f, { 9.6f, 0.0f, 3.5f } },

        { { 175.0f, 168.0f, 215.0f }, 52.0f, { 9.4f, 0.0f, 3.3f } },

        { { 255.0f, 156.0f,  35.0f }, 48.0f, { 9.8f, 0.0f, 3.6f } },

        { { -15.0f, 165.0f, 335.0f }, 50.0f, { 9.5f, 0.0f, 3.4f } }

    };

    

    for (int i = 0; i < 4; i++) {

        CloudCluster cl;

        cl.type = CLOUD_CIRRUS;

        cl.pos = cirrusSeeds[i].pos;

        cl.vel = cirrusSeeds[i].vel;

        cl.boundingRadius = cirrusSeeds[i].radius;

        cl.baseAltitude = cirrusSeeds[i].pos.y;

        cl.internalFlash = 0.0f;

        

        for (int s = 0; s < 9; s++) {

            float offsetT = ((float)s - 4.0f) * 11.0f;

            CloudPuff p;

            p.localPos = (Vector3){ offsetT * 0.95f, sinf(offsetT * 0.05f) * 2.5f, offsetT * 0.35f };

            p.size = (Vector3){ 18.0f, 3.5f, 12.0f };

            p.density = 0.28f;

            p.boilPhase = (float)(s * 0.8f);

            p.boilFreq = 0.6f;

            p.glyph = (s % 2 == 0) ? '~' : '-';

            cl.puffs.push_back(p);

        }

        g_cloudClusters.push_back(cl);

    }

}



void UpdateCloudPhysics(float dt, float timeVal, float lightningFlash) {

    for (auto& cluster : g_cloudClusters) {

        cluster.pos.x += cluster.vel.x * dt;

        cluster.pos.z += cluster.vel.z * dt;

        

        // Seamless toroidal boundary wrapping across sky

        if (cluster.pos.x > 380.0f) cluster.pos.x = -120.0f;

        if (cluster.pos.x < -120.0f) cluster.pos.x = 380.0f;

        if (cluster.pos.z > 440.0f) cluster.pos.z = -120.0f;

        if (cluster.pos.z < -120.0f) cluster.pos.z = 440.0f;

        

        // Intra-cloud lightning flash response

        if (lightningFlash > 0.0f && cluster.type == CLOUD_STORM_SHELF) {

            cluster.internalFlash = lightningFlash * 1.5f;

        } else if (cluster.internalFlash > 0.0f) {

            cluster.internalFlash = fmaxf(0.0f, cluster.internalFlash - dt * 2.8f);

        }

    }

}



Color GetCloudPuffColor(const CloudCluster& cluster, const CloudPuff& puff, Vector3 puffWorldPos,

                        Vector3 camPos, Vector3 sunDir, float sunElev, float lightningFlash) {

    float h = sunElev;

    float wDay   = Clamp((h + 0.10f) / 0.35f, 0.0f, 1.0f);

    float wTwi   = Clamp(1.0f - fabsf(h - 0.04f) / 0.16f, 0.0f, 1.0f);

    float wNight = Clamp((-h - 0.04f) / 0.22f, 0.0f, 1.0f);

    float sumW = wDay + wTwi + wNight;

    if (sumW > 0.001f) { wDay /= sumW; wTwi /= sumW; wNight /= sumW; }

    

    // Normal estimation (tops face zenith, flanks face outward)

    Vector3 norm = Vector3Normalize((Vector3){ puff.localPos.x * 0.35f, puff.localPos.y + 4.5f, puff.localPos.z * 0.35f });

    

    // 1. Direct Solar & Lunar illumination

    float nDotL = Vector3DotProduct(norm, sunDir);

    float sunDiff = fmaxf(0.0f, nDotL) * fmaxf(0.0f, h);

    float moonDiff = fmaxf(0.0f, Vector3DotProduct(norm, Vector3Negate(sunDir))) * fmaxf(0.0f, -h) * 0.35f;

    

    // 2. Beer-Lambert internal optical depth absorption

    float yRatio = Clamp((puff.localPos.y + 6.0f) / 14.0f, 0.0f, 1.0f);

    float beer = 0.32f + 0.68f * yRatio;

    if (cluster.type == CLOUD_STORM_SHELF) {

        beer = 0.18f + 0.52f * yRatio;

    }

    

    // 3. Forward Mie Rim Lighting ("Silver Lining")

    Vector3 toPuff = Vector3Normalize(Vector3Subtract(puffWorldPos, camPos));

    float cosPsi = Vector3DotProduct(toPuff, sunDir);

    float rim = 0.0f;

    if (cosPsi > 0.05f && h > -0.05f) {

        rim = powf(cosPsi, 5.0f) * (1.1f - puff.density) * 1.8f * (wDay * 0.9f + wTwi * 1.6f);

    }

    

    // 4. Color Palette Synthesis

    Vector3 colSun = Vector3Add(Vector3Scale((Vector3){ 255, 252, 245 }, wDay),

                                Vector3Scale((Vector3){ 255, 165,  55 }, wTwi));

    Vector3 colAmb = Vector3Add(Vector3Scale((Vector3){ 130, 160, 205 }, wDay),

                     Vector3Add(Vector3Scale((Vector3){  85,  60, 110 }, wTwi),

                                Vector3Scale((Vector3){  18,  24,  44 }, wNight)));

    if (cluster.type == CLOUD_STORM_SHELF) {

        colAmb = Vector3Multiply(colAmb, (Vector3){ 0.45f, 0.48f, 0.55f });

        colSun = Vector3Multiply(colSun, (Vector3){ 0.60f, 0.58f, 0.55f });

    } else if (cluster.type == CLOUD_CIRRUS) {

        colSun = Vector3Scale(colSun, 1.15f);

    }

    

    float lightFactor = (sunDiff + moonDiff) * 0.75f;

    Vector3 finalRgb = Vector3Add(Vector3Scale(colAmb, beer), Vector3Scale(colSun, lightFactor * beer));

    

    if (rim > 0.01f) {

        Vector3 rimCol = (wTwi > 0.25f) ? (Vector3){ 255, 190, 80 } : (Vector3){ 255, 255, 240 };

        finalRgb = Vector3Add(finalRgb, Vector3Scale(rimCol, rim));

    }

    

    // 5. Intra-cloud lightning flash & global lightning flash

    float totalFlash = cluster.internalFlash + lightningFlash * 0.75f;

    if (totalFlash > 0.01f) {

        float fIntensity = Clamp(totalFlash * 220.0f, 0.0f, 255.0f);

        finalRgb.x = fminf(255.0f, finalRgb.x + fIntensity * 0.90f);

        finalRgb.y = fminf(255.0f, finalRgb.y + fIntensity * 0.98f);

        finalRgb.z = fminf(255.0f, finalRgb.z + fIntensity * 1.10f);

    }

    

    unsigned char alpha = (unsigned char)(Clamp(puff.density * (cluster.type == CLOUD_CIRRUS ? 160.0f : 240.0f), 40.0f, 255.0f));

    return (Color){ (unsigned char)Clamp(finalRgb.x, 0.0f, 255.0f),

                    (unsigned char)Clamp(finalRgb.y, 0.0f, 255.0f),

                    (unsigned char)Clamp(finalRgb.z, 0.0f, 255.0f), alpha };

}



// CURATED FRUSTUM-CULLED ASCII CLOUD PARTICLES

// Renders subtle, artistic ASCII cloud contour wisps strictly when the player is looking at them

void PopulateCloudInstances(Camera3D camera, Vector3 sunDir, float sunElev, float lightningFlash,

                            std::vector<Matrix> cloudInstances[256], float timeVal) {

    for (int i = 0; i < 256; i++) cloudInstances[i].clear();

    

    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    

    for (const auto& cluster : g_cloudClusters) {

        float dx = cluster.pos.x - camera.position.x;

        float dy = cluster.pos.y - camera.position.y;

        float dz = cluster.pos.z - camera.position.z;

        float distSq = dx*dx + dz*dz;

        if (distSq > 280.0f * 280.0f) continue;

        

        // 3D Frustum culling: Skip clusters outside player's viewing cone

        float fullDist = sqrtf(dx*dx + dy*dy + dz*dz);

        if (fullDist > 0.1f) {

            float dot = (camForward.x * dx + camForward.y * dy + camForward.z * dz) / fullDist;

            if (dot < 0.15f) continue; // Outside camera FOV

        }

        

        // Populate a curated, tasteful subset of ASCII letters (core + prominent wisps)

        for (size_t p = 0; p < cluster.puffs.size(); p++) {

            // Keep selective artistic puffs (every 2nd puff and core) to prevent visual clutter

            if (p % 2 != 0 && p != 0) continue;

            

            const auto& puff = cluster.puffs[p];

            float dyPuff = sinf(timeVal * puff.boilFreq + puff.boilPhase) * 0.55f;

            float px = cosf(timeVal * puff.boilFreq * 0.75f + puff.boilPhase) * 0.45f;

            float pz = sinf(timeVal * puff.boilFreq * 1.25f + puff.boilPhase) * 0.45f;

            

            Vector3 worldPos = { cluster.pos.x + puff.localPos.x + px,

                                 cluster.pos.y + puff.localPos.y + dyPuff,

                                 cluster.pos.z + puff.localPos.z + pz };

            

            // Individual puff frustum culling: ONLY exist when player is directly seeing them

            Vector3 toPuff = Vector3Subtract(worldPos, camera.position);

            float pDist = Vector3Length(toPuff);

            if (pDist > 0.1f) {

                float pDot = (camForward.x * toPuff.x + camForward.y * toPuff.y + camForward.z * toPuff.z) / pDist;

                if (pDot < 0.20f) continue; // Not in view

            }

            

            Color col = GetCloudPuffColor(cluster, puff, worldPos, camera.position, sunDir, sunElev, lightningFlash);

            

            Matrix m = MatrixIdentity();

            m.m0 = col.r / 255.0f;

            m.m1 = col.g / 255.0f;

            m.m2 = col.b / 255.0f;

            m.m3 = (col.a / 255.0f) * 0.50f; // Delicate translucent blend with 3D puff volume

            m.m4 = puff.size.x * 0.95f;

            m.m5 = puff.size.y * 0.95f;

            m.m9 = 1.0f;  // Disable Fog

            m.m10 = 0.0f; // Billboard quad

            m.m11 = 1.0f;

            m.m12 = worldPos.x;

            m.m13 = worldPos.y;

            m.m14 = worldPos.z;

            cloudInstances[(uint8_t)puff.glyph].push_back(m);

        }

    }

}



// 3D Volumetric Puffy Clouds: Realistic atmospheric Mie/Beer-Lambert illumination,

// thermodynamic boiling, 3D spheres & cubes, with 3D frustum culling and optimized 8x8 tessellation.

void DrawHybridCloudVolumes(Camera3D camera, Vector3 sunDir, float sunElev, float lightningFlash, float timeVal) {

    Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

    

    for (const auto& cluster : g_cloudClusters) {

        float dx = cluster.pos.x - camera.position.x;

        float dy = cluster.pos.y - camera.position.y;

        float dz = cluster.pos.z - camera.position.z;

        float distSq = dx*dx + dz*dz;

        if (distSq > 300.0f * 300.0f) continue;

        

        // 3D Frustum culling: Skip cloud clusters completely outside camera field of view

        float fullDist = sqrtf(dx*dx + dy*dy + dz*dz);

        if (fullDist > 0.1f) {

            float dot = (camForward.x * dx + camForward.y * dy + camForward.z * dz) / fullDist;

            if (dot < 0.15f) continue;

        }

        

        for (size_t p = 0; p < cluster.puffs.size(); p++) {

            const auto& puff = cluster.puffs[p];

            float dyPuff = sinf(timeVal * puff.boilFreq + puff.boilPhase) * 0.55f;

            float px = cosf(timeVal * puff.boilFreq * 0.75f + puff.boilPhase) * 0.45f;

            float pz = sinf(timeVal * puff.boilFreq * 1.25f + puff.boilPhase) * 0.45f;

            

            Vector3 worldPos = { cluster.pos.x + puff.localPos.x + px,

                                 cluster.pos.y + puff.localPos.y + dyPuff,

                                 cluster.pos.z + puff.localPos.z + pz };

            

            // Individual puff frustum check

            Vector3 toPuff = Vector3Subtract(worldPos, camera.position);

            float pDist = Vector3Length(toPuff);

            if (pDist > 0.1f) {

                float pDot = (camForward.x * toPuff.x + camForward.y * toPuff.y + camForward.z * toPuff.z) / pDist;

                if (pDot < 0.20f) continue;

            }

            

            Color col = GetCloudPuffColor(cluster, puff, worldPos, camera.position, sunDir, sunElev, lightningFlash);

            

            if (cluster.type == CLOUD_CIRRUS) {

                DrawCube(worldPos, puff.size.x * 1.4f, puff.size.y * 0.45f, puff.size.z * 1.4f, col);

            } else {

                DrawSphereEx(worldPos, puff.size.x * 0.62f, 8, 8, col);

            }

        }

    }

}



// -------------------------------------------------------------------------

// REAL-WORLD ATMOSPHERIC PHYSICS: CELESTIAL CLOUD OCCLUSION & EXTINCTION

// Ray-sphere intersection against volumetric cloud clusters to determine

// real-time extinction of direct solar and lunar beams.

// -------------------------------------------------------------------------

static float GetCelestialCloudOcclusion(Camera3D camera, Vector3 celestialDir) {

    if (celestialDir.y < -0.08f) return 0.0f;

    float maxCover = 0.0f;



    for (const auto& cluster : g_cloudClusters) {

        Vector3 toCluster = Vector3Subtract(cluster.pos, camera.position);

        float projDist = Vector3DotProduct(toCluster, celestialDir);

        if (projDist <= 5.0f) continue; // Behind camera



        Vector3 closestPt = Vector3Add(camera.position, Vector3Scale(celestialDir, projDist));

        float perpDist = Vector3Distance(cluster.pos, closestPt);

        float effRadius = cluster.boundingRadius * 1.35f;



        if (perpDist < effRadius) {

            float cover = 1.0f - (perpDist / effRadius);

            cover = cover * cover; // Smooth quadratic falloff toward edges

            float weight = (cluster.type == CLOUD_CIRRUS) ? 0.75f : ((cluster.type == CLOUD_STORM_SHELF) ? 1.4f : 1.0f);

            float effCover = Clamp(cover * weight, 0.0f, 1.0f);

            if (effCover > maxCover) maxCover = effCover;

        }

    }

    return maxCover;

}



void DrawCloudGroundShadows(Camera3D camera, Vector3 sunDir, float sunElev) {

    // Determine active celestial light source: Sun during day, Moon at night

    Vector3 lightDir = sunDir;

    float lightElev = sunElev;

    bool isMoonShadow = false;

    if (sunElev < -0.04f) {

        // Nighttime: Moon is directly opposite the Sun

        lightDir = Vector3Negate(sunDir);

        lightElev = lightDir.y;

        isMoonShadow = true;

    }

    if (lightElev < 0.035f) return; // Sun/Moon below horizon casts no ground shadows



    float invLy = 1.0f / fmaxf(0.10f, lightElev);



    rlEnableColorBlend();

    rlSetTexture(0);



    for (const auto& cluster : g_cloudClusters) {

        if (cluster.type == CLOUD_CIRRUS) continue;



        float groundH = cluster.pos.y - 10.025f;

        float shadowX = cluster.pos.x - lightDir.x * groundH * invLy;

        float shadowZ = cluster.pos.z - lightDir.z * groundH * invLy;



        float dx = shadowX - camera.position.x;

        float dz = shadowZ - camera.position.z;

        if (dx*dx + dz*dz > 250.0f * 250.0f) continue;



        // Physical Umbra / Penumbra synthesis:

        // Render organic, soft-feathered shadow volumes matching the cloud's constituent puffs

        float maxAlpha = isMoonShadow ? 42.0f : ((cluster.type == CLOUD_STORM_SHELF) ? 140.0f : 90.0f);

        maxAlpha *= Clamp(lightElev * 2.5f, 0.0f, 1.0f);

        if (maxAlpha < 2.0f) continue;



        Color shadowCol = isMoonShadow ? (Color){ 4, 6, 12, 0 } : (Color){ 10, 14, 20, 0 };



        const int fanSegs = 14;

        for (const auto& puff : cluster.puffs) {

            float pGroundH = (cluster.pos.y + puff.localPos.y) - 10.025f;

            float px = (cluster.pos.x + puff.localPos.x) - lightDir.x * pGroundH * invLy;

            float pz = (cluster.pos.z + puff.localPos.z) - lightDir.z * pGroundH * invLy;



            float pDistSq = (px - camera.position.x)*(px - camera.position.x) + (pz - camera.position.z)*(pz - camera.position.z);

            if (pDistSq > 220.0f * 220.0f) continue;



            float pRad = fmaxf(puff.size.x, puff.size.z) * 1.45f;

            float puffAlpha = maxAlpha * puff.density * 0.72f;

            unsigned char cAlpha = (unsigned char)Clamp(puffAlpha, 0.0f, 255.0f);



            // Continuous radial fan: Center is Umbra (cAlpha), Outer rim smoothly feathers to 0 ALPHA.

            // Completely eliminates geometric rings!

            rlBegin(RL_TRIANGLES);

            for (int i = 0; i < fanSegs; i++) {

                float a0 = (float)i * (2.0f * PI / (float)fanSegs);

                float a1 = (float)(i + 1) * (2.0f * PI / (float)fanSegs);

                float x0 = px + cosf(a0) * pRad;

                float z0 = pz + sinf(a0) * pRad;

                float x1 = px + cosf(a1) * pRad;

                float z1 = pz + sinf(a1) * pRad;



                // Center vertex: core umbra/penumbra

                rlColor4ub(shadowCol.r, shadowCol.g, shadowCol.b, cAlpha);

                rlVertex3f(px, 10.025f, pz);



                // Outer vertices: 0 alpha (feathered penumbra boundary, zero rings!)

                rlColor4ub(shadowCol.r, shadowCol.g, shadowCol.b, 0);

                rlVertex3f(x0, 10.025f, z0);



                rlColor4ub(shadowCol.r, shadowCol.g, shadowCol.b, 0);

                rlVertex3f(x1, 10.025f, z1);

            }

            rlEnd();

        }

    }

}



void DrawAtmosphericSkyDome(Camera3D camera, float sunTheta, float lightningFlash) {

    rlDisableDepthTest();

    rlDisableDepthMask();

    rlDisableBackfaceCulling();

    

    float sunElev = sinf(sunTheta);

    Vector3 sunDir = Vector3Normalize((Vector3){ cosf(sunTheta), sunElev, cosf(sunTheta) * 0.28f });

    

    float wDay = Clamp((sunElev + 0.10f) / 0.35f, 0.0f, 1.0f);

    float wTwi = Clamp(1.0f - fabsf(sunElev - 0.04f) / 0.16f, 0.0f, 1.0f);

    float wNight = Clamp((-sunElev - 0.04f) / 0.22f, 0.0f, 1.0f);

    float sumW = wDay + wTwi + wNight;

    if (sumW > 0.001f) {

        wDay /= sumW;

        wTwi /= sumW;

        wNight /= sumW;

    }

    

    auto Blend3Col = [&](Color cDay, Color cTwi, Color cNight) -> Color {

        float r = cDay.r * wDay + cTwi.r * wTwi + cNight.r * wNight;

        float g = cDay.g * wDay + cTwi.g * wTwi + cNight.g * wNight;

        float b = cDay.b * wDay + cTwi.b * wTwi + cNight.b * wNight;

        if (lightningFlash > 0.0f) {

            float lf = lightningFlash * 190.0f;

            r = fminf(255.0f, r + lf);

            g = fminf(255.0f, g + lf * 1.05f);

            b = fminf(255.0f, b + lf * 1.15f);

        }

        return (Color){ (unsigned char)Clamp(r, 0.0f, 255.0f),

                        (unsigned char)Clamp(g, 0.0f, 255.0f),

                        (unsigned char)Clamp(b, 0.0f, 255.0f), 255 };

    };

    

    bool isUnderwater = (camera.position.y < 9.75f && camera.position.x <= 36.0f);

    // Dynamic Atmospheric Sky Dome Colors: switch to deep abyssal oceanic dome when submerged!
    Color colZenith = isUnderwater ? Blend3Col((Color){ 14, 75, 95, 255 }, (Color){ 8, 42, 58, 255 }, (Color){ 2, 8, 14, 255 })
                                   : Blend3Col((Color){ 42, 115, 225, 255 }, (Color){ 25, 35, 85, 255 }, (Color){ 3, 5, 10, 255 });

    Color colUpper  = isUnderwater ? Blend3Col((Color){ 9, 52, 70, 255 }, (Color){ 5, 28, 40, 255 }, (Color){ 2, 6, 11, 255 })
                                   : Blend3Col((Color){ 95, 168, 242, 255 }, (Color){ 118, 62, 125, 255 }, (Color){ 6, 9, 18, 255 });

    Color colHaze   = isUnderwater ? Blend3Col((Color){ 5, 28, 42, 255 }, (Color){ 3, 16, 26, 255 }, (Color){ 1, 4, 8, 255 })
                                   : Blend3Col((Color){ 188, 222, 252, 255 }, (Color){ 255, 115, 35, 255 }, (Color){ 10, 15, 28, 255 });

    Color colAntiSunHaze = colHaze;

    Color colNadir  = isUnderwater ? (Color){ 1, 5, 10, 255 }
                                   : Blend3Col((Color){ 45, 60, 75, 255 }, (Color){ 22, 18, 26, 255 }, (Color){ 4, 5, 8, 255 });

    

    const int sectors = 32;

    const int rings = 12;

    const float radius = 280.0f;

    

    // Explicitly bind the default 1x1 white texture to guarantee untextured shapes are NEVER multiplied by leftover textures

    rlSetTexture(0);

    rlBegin(RL_TRIANGLES);

    for (int r = 0; r < rings; r++) {

        float phi0 = -0.35f + (PI * 0.5f + 0.35f) * ((float)r / (float)rings);

        float phi1 = -0.35f + (PI * 0.5f + 0.35f) * ((float)(r + 1) / (float)rings);

        

        float cosPhi0 = cosf(phi0); float sinPhi0 = sinf(phi0);

        float cosPhi1 = cosf(phi1); float sinPhi1 = sinf(phi1);

        

        for (int s = 0; s < sectors; s++) {

            float psi0 = (float)s * (2.0f * PI / (float)sectors);

            float psi1 = (float)(s + 1) * (2.0f * PI / (float)sectors);

            

            float cosPsi0 = cosf(psi0); float sinPsi0 = sinf(psi0);

            float cosPsi1 = cosf(psi1); float sinPsi1 = sinf(psi1);

            

            Vector3 v00 = { camera.position.x + radius * cosPhi0 * cosPsi0, camera.position.y + radius * sinPhi0, camera.position.z + radius * cosPhi0 * sinPsi0 };

            Vector3 v10 = { camera.position.x + radius * cosPhi1 * cosPsi0, camera.position.y + radius * sinPhi1, camera.position.z + radius * cosPhi1 * sinPsi0 };

            Vector3 v01 = { camera.position.x + radius * cosPhi0 * cosPsi1, camera.position.y + radius * sinPhi0, camera.position.z + radius * cosPhi0 * sinPsi1 };

            Vector3 v11 = { camera.position.x + radius * cosPhi1 * cosPsi1, camera.position.y + radius * sinPhi1, camera.position.z + radius * cosPhi1 * sinPsi1 };

            

            auto VertexColor = [&](float sinPhi, float cosPsi) -> Color {

                float elevNorm = Clamp(sinPhi, 0.0f, 1.0f);

                float sunAlign = cosPsi * sunDir.x + (sinPhi * sunDir.y) + (sinf(psi0) * sunDir.z);

                Color hColor = (sunAlign > 0.0f) ? colHaze : colAntiSunHaze;

                

                // Twilight forward scattering

                if (sunAlign > 0.0f && wTwi > 0.05f) {

                    float glow = powf(Clamp(sunAlign, 0.0f, 1.0f), 3.0f) * wTwi;

                    hColor.r = (unsigned char)fminf(255.0f, hColor.r + glow * 60.0f);

                    hColor.g = (unsigned char)fminf(255.0f, hColor.g + glow * 30.0f);

                }

                // Daytime solar forward-scattering bloom

                if (sunAlign > 0.0f && wDay > 0.05f) {

                    float bloom = powf(Clamp(sunAlign, 0.0f, 1.0f), 4.0f) * wDay;

                    hColor.r = (unsigned char)fminf(255.0f, hColor.r + bloom * 65.0f);

                    hColor.g = (unsigned char)fminf(255.0f, hColor.g + bloom * 45.0f);

                    hColor.b = (unsigned char)fminf(255.0f, hColor.b + bloom * 20.0f);

                }

                

                if (elevNorm > 0.55f) {

                    float t = (elevNorm - 0.55f) / 0.45f;

                    return BlendColors(colUpper, colZenith, t);

                } else if (elevNorm > 0.05f) {

                    float t = (elevNorm - 0.05f) / 0.50f;

                    return BlendColors(hColor, colUpper, t);

                } else {

                    float t = Clamp((elevNorm + 0.35f) / 0.40f, 0.0f, 1.0f);

                    return BlendColors(colNadir, hColor, t);

                }

            };

            

            Color c00 = VertexColor(sinPhi0, cosPsi0);

            Color c10 = VertexColor(sinPhi1, cosPsi0);

            Color c01 = VertexColor(sinPhi0, cosPsi1);

            Color c11 = VertexColor(sinPhi1, cosPsi1);

            

            // Render inward-facing triangles with default white texcoords

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(c00.r, c00.g, c00.b, c00.a); rlVertex3f(v00.x, v00.y, v00.z);

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(c01.r, c01.g, c01.b, c01.a); rlVertex3f(v01.x, v01.y, v01.z);

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(c10.r, c10.g, c10.b, c10.a); rlVertex3f(v10.x, v10.y, v10.z);

            

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(c01.r, c01.g, c01.b, c01.a); rlVertex3f(v01.x, v01.y, v01.z);

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(c11.r, c11.g, c11.b, c11.a); rlVertex3f(v11.x, v11.y, v11.z);

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(c10.r, c10.g, c10.b, c10.a); rlVertex3f(v10.x, v10.y, v10.z);

        }

    }

    rlEnd();

    

    // Compact, Realistic-Proportioned 3D Radiant Sun & Multi-Ring Corona (ZERO square edges)

    if (sunElev > -0.12f && !isUnderwater) {

        Vector3 sunPos = Vector3Add(camera.position, Vector3Scale(sunDir, 260.0f));

        Vector3 toCam = Vector3Normalize(Vector3Subtract(camera.position, sunPos));

        Vector3 upRef = (fabsf(toCam.y) > 0.88f) ? (Vector3){ 0.0f, 0.0f, 1.0f } : (Vector3){ 0.0f, 1.0f, 0.0f };

        Vector3 right = Vector3Normalize(Vector3CrossProduct(upRef, toCam));

        Vector3 up    = Vector3Normalize(Vector3CrossProduct(toCam, right));



        const int sunSegs = 32;



        // 1. Central Solar Core Disc (32-segment circular fan, compact 3.5m radius)

        float rCore = 3.5f;

        Color centerCol = { 255, 255, 250, 255 };

        Color coreRimCol = (wTwi > 0.25f) ? (Color){ 255, 215, 140, 255 } : (Color){ 255, 252, 225, 255 };

        

        rlSetTexture(0);

        rlBegin(RL_TRIANGLES);

        for (int i = 0; i < sunSegs; i++) {

            float a0 = (float)i * (2.0f * PI / (float)sunSegs);

            float a1 = (float)(i + 1) * (2.0f * PI / (float)sunSegs);

            Vector3 p0 = Vector3Add(sunPos, Vector3Add(Vector3Scale(right, cosf(a0) * rCore), Vector3Scale(up, sinf(a0) * rCore)));

            Vector3 p1 = Vector3Add(sunPos, Vector3Add(Vector3Scale(right, cosf(a1) * rCore), Vector3Scale(up, sinf(a1) * rCore)));

            

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(centerCol.r, centerCol.g, centerCol.b, centerCol.a);     rlVertex3f(sunPos.x, sunPos.y, sunPos.z);

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(coreRimCol.r, coreRimCol.g, coreRimCol.b, coreRimCol.a); rlVertex3f(p0.x, p0.y, p0.z);

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(coreRimCol.r, coreRimCol.g, coreRimCol.b, coreRimCol.a); rlVertex3f(p1.x, p1.y, p1.z);

        }

        rlEnd();



        // Smooth gradient sun corona fan (zero stepped rings)

        auto DrawContinuousSunGlow = [&](float rInner, float rOuter, Color colInner, Color colOuter) {

            rlBegin(RL_TRIANGLES);

            for (int i = 0; i < sunSegs; i++) {

                float a0 = (float)i * (2.0f * PI / (float)sunSegs);

                float a1 = (float)(i + 1) * (2.0f * PI / (float)sunSegs);

                float c0 = cosf(a0), s0 = sinf(a0);

                float c1 = cosf(a1), s1 = sinf(a1);

                

                Vector3 in0  = Vector3Add(sunPos, Vector3Add(Vector3Scale(right, c0 * rInner), Vector3Scale(up, s0 * rInner)));

                Vector3 in1  = Vector3Add(sunPos, Vector3Add(Vector3Scale(right, c1 * rInner), Vector3Scale(up, s1 * rInner)));

                Vector3 out0 = Vector3Add(sunPos, Vector3Add(Vector3Scale(right, c0 * rOuter), Vector3Scale(up, s0 * rOuter)));

                Vector3 out1 = Vector3Add(sunPos, Vector3Add(Vector3Scale(right, c1 * rOuter), Vector3Scale(up, s1 * rOuter)));

                

                rlTexCoord2f(0.5f, 0.5f); rlColor4ub(colInner.r, colInner.g, colInner.b, colInner.a); rlVertex3f(in0.x, in0.y, in0.z);

                rlTexCoord2f(0.5f, 0.5f); rlColor4ub(colOuter.r, colOuter.g, colOuter.b, colOuter.a); rlVertex3f(out0.x, out0.y, out0.z);

                rlTexCoord2f(0.5f, 0.5f); rlColor4ub(colInner.r, colInner.g, colInner.b, colInner.a); rlVertex3f(in1.x, in1.y, in1.z);

                

                rlTexCoord2f(0.5f, 0.5f); rlColor4ub(colInner.r, colInner.g, colInner.b, colInner.a); rlVertex3f(in1.x, in1.y, in1.z);

                rlTexCoord2f(0.5f, 0.5f); rlColor4ub(colOuter.r, colOuter.g, colOuter.b, colOuter.a); rlVertex3f(out0.x, out0.y, out0.z);

                rlTexCoord2f(0.5f, 0.5f); rlColor4ub(colOuter.r, colOuter.g, colOuter.b, colOuter.a); rlVertex3f(out1.x, out1.y, out1.z);

            }

            rlEnd();

        };



        // Forward Mie Scattering Solar Glow (smooth gradient fading seamlessly to 0 alpha)

        Color auraInner = (wTwi > 0.25f) ? (Color){ 255, 185, 75, 180 } : (Color){ 255, 248, 205, 170 };

        Color auraOuter = (wTwi > 0.25f) ? (Color){ 255, 110, 30, 0 }   : (Color){ 255, 220, 110, 0 };

        DrawContinuousSunGlow(3.5f, 24.0f, auraInner, auraOuter);



        // =========================================================================

        // REAL-WORLD ATMOSPHERIC OPTICS: 22-DEGREE ICE CRYSTAL HALO & CORONA

        // Triggers when high clouds / ice crystals pass in front of the Sun!

        // Red on inside, blue on outside (opposite of atmospheric corona)

        // =========================================================================

        float sunCloudCover = GetCelestialCloudOcclusion(camera, sunDir);

        if (sunCloudCover > 0.03f) {

            // A. Forward Mie scattering optical corona through cloud veil

            float coronaAlpha = sunCloudCover * (wDay * 140.0f + wTwi * 190.0f);

            Color coronaCol = (wTwi > 0.25f) ? (Color){ 255, 200, 120, (unsigned char)coronaAlpha } : (Color){ 255, 252, 240, (unsigned char)coronaAlpha };

            Color coronaFade = (Color){ coronaCol.r, coronaCol.g, coronaCol.b, 0 };

            DrawContinuousSunGlow(3.5f, 38.0f * (1.0f + sunCloudCover * 0.5f), coronaCol, coronaFade);



            // B. 22-Degree Hexagonal Ice-Crystal Refraction Halo Ring

            // Radius: 260m * tan(22 deg) ~= 105 meters!

            float haloRadius = 105.0f;

            float haloThick  = 6.5f;

            float haloAlpha  = sunCloudCover * (wDay * 95.0f + wTwi * 120.0f);



            // Refraction physics: Red on inside (rIn), Pale Blue/Violet on outside (rOut)

            Color haloRedInside = { 245, 95, 65, (unsigned char)haloAlpha };

            Color haloBlueOut   = { 110, 165, 255, (unsigned char)(haloAlpha * 0.85f) };

            Color haloFade      = { 120, 180, 255, 0 };



            DrawContinuousSunGlow(haloRadius - haloThick, haloRadius, haloFade, haloRedInside);

            DrawContinuousSunGlow(haloRadius, haloRadius + haloThick, haloRedInside, haloBlueOut);

            DrawContinuousSunGlow(haloRadius + haloThick, haloRadius + haloThick * 2.0f, haloBlueOut, haloFade);

        }



        // 5. Subtle Delicate Sunbeams (12 delicate rays)

        const int rayCount = 12;

        rlBegin(RL_TRIANGLES);

        for (int r = 0; r < rayCount; r++) {

            float baseAngle = (float)r * (2.0f * PI / (float)rayCount);

            float rayLen = (r % 2 == 0) ? 25.0f : 18.0f;

            float rayHalfWidth = 0.045f;

            

            float aL = baseAngle - rayHalfWidth;

            float aR = baseAngle + rayHalfWidth;

            

            Vector3 pL = Vector3Add(sunPos, Vector3Add(Vector3Scale(right, cosf(aL) * 3.8f), Vector3Scale(up, sinf(aL) * 3.8f)));

            Vector3 pR = Vector3Add(sunPos, Vector3Add(Vector3Scale(right, cosf(aR) * 3.8f), Vector3Scale(up, sinf(aR) * 3.8f)));

            Vector3 pT = Vector3Add(sunPos, Vector3Add(Vector3Scale(right, cosf(baseAngle) * rayLen), Vector3Scale(up, sinf(baseAngle) * rayLen)));

            

            Color rayBaseCol = (wTwi > 0.25f) ? (Color){ 255, 155, 50, 75 } : (Color){ 255, 245, 195, 65 };

            Color rayTipCol  = (wTwi > 0.25f) ? (Color){ 255, 95, 20, 0 }   : (Color){ 255, 215, 130, 0 };

            

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(rayBaseCol.r, rayBaseCol.g, rayBaseCol.b, rayBaseCol.a); rlVertex3f(pL.x, pL.y, pL.z);

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(rayTipCol.r, rayTipCol.g, rayTipCol.b, rayTipCol.a);     rlVertex3f(pT.x, pT.y, pT.z);

            rlTexCoord2f(0.5f, 0.5f); rlColor4ub(rayBaseCol.r, rayBaseCol.g, rayBaseCol.b, rayBaseCol.a); rlVertex3f(pR.x, pR.y, pR.z);

        }

        rlEnd();

    }

    

    rlEnableBackfaceCulling();

    rlEnableDepthMask();

    rlEnableDepthTest();

}

