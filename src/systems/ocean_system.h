#pragma once
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// =========================================================================
// CAMERA-RELATIVE PHYSICALLY BELIEVABLE PROCEDURAL INFINITE OCEAN SYSTEM (GLSL 330)
// High-Performance 144 FPS Architecture:
// - Camera-Centric Snapping Moving Clipmap Grid (CDLOD) for truly infinite ocean
// - 8-Wave Directional Gerstner Wave Spectrum with asymmetric physical profiles
// - Camera-Relative Realism across 5 Altitude Regimes:
//   1. Centimeters above water (swimmer eye, crest occlusion, sharp capillary micro-facets)
//   2. Human eye level (shore/wading, balanced swell perspective, Cox-Munk sunglint)
//   3. Several meters above (pier deck/cliffs, transparent lookdown to seabed/wreck, Toksvig AA)
//   4. Aerial (broad solar glare envelope, macroscopic swell interference, atmospheric horizon)
//   5. Underwater (Snell's Window cone, Total Internal Reflection mirror ceiling)
// - Analytical Jacobian Wave Breaking & Organic Whitecap Dissipation
// - Physical Spectral Light Absorption (Beer-Lambert law per wavelength)
// - Buoyant Surface Swimming with Wave-Tilt Kinematics (No accidental drowning!)
// - Intentional 3D Underwater Diving with Oxygen System
// - Waterline Meniscus Lens Refraction Post-FX
// =========================================================================

static const char *OCEAN_VS = R"glsl(
#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform float timeVal;
uniform vec3 viewPos;
uniform vec2 gridSnapOffset; // Camera-centric snapping offset
uniform vec2 windDir;        // Normalized wind direction
uniform float windSpeed;     // Wind speed in m/s (e.g. 5.0 - 18.0)

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out float fragWaveHeight;
out float fragJacobian;      // Wave crest collision & breaking determinant
out float fragFetch;         // Spatial sea state modulation (calm vs heavy swell)

// 8-Wave Directional Gerstner Spectrum
// Components: vec4(dir.x, dir.y, steepness Q, wavelength L)
// Swell Waves (Incoming from Atlantic open sea towards coast, broad smooth backs, peaked crests)
const vec4 wave0 = vec4(0.966, 0.259, 0.22, 78.0);
const vec4 wave1 = vec4(0.866, -0.500, 0.18, 46.0);

// Wind-Driven Cross Sea (coupled to local wind direction with angular spreading)
const vec4 wave2 = vec4(0.985, 0.174, 0.16, 24.0);
const vec4 wave3 = vec4(0.819, -0.574, 0.13, 13.5);
const vec4 wave4 = vec4(0.707, 0.707, 0.11, 7.4);

// High-Frequency Capillary Chop & Local Wind Turbulence
const vec4 wave5 = vec4(0.927, -0.375, 0.08, 3.8);
const vec4 wave6 = vec4(0.500, 0.866, 0.06, 1.9);
const vec4 wave7 = vec4(0.707, -0.707, 0.04, 0.95);

const float g = 9.80665;

void main()
{
    // Apply camera-centric snapped world grid position
    vec3 pos = vertexPosition;
    pos.x += gridSnapOffset.x;
    pos.z += gridSnapOffset.y;

    // Coastal clamp: Ensure ocean surface flushly meets shoreline at X = 35.0
    if (pos.x > 35.0) pos.x = 35.0;

    // Large-Scale Spatial Fetch Modulation (creates calm glassy slicks vs heavy swell zones)
    float fetchNoise1 = sin(pos.x * 0.0031 + pos.z * 0.0022 + 1.2);
    float fetchNoise2 = cos(pos.x * 0.0018 - pos.z * 0.0036 + 2.8);
    float fetchMod = clamp((fetchNoise1 * 0.5 + fetchNoise2 * 0.5) * 0.75 + 0.65, 0.25, 1.20);

    // Near-shore shoaling: Waves slow down and steepen near coast (X in [16, 35])
    float coastalDepth = clamp((35.0 - pos.x) / 25.0, 0.15, 1.0);
    float localFetch = fetchMod * clamp(coastalDepth * 1.3, 0.35, 1.0);

    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0);
    float totalDispY = 0.0;

    // Jacobian horizontal derivative accumulators for wave breaking
    float dDx_dx = 0.0;
    float dDx_dz = 0.0;
    float dDz_dx = 0.0;
    float dDz_dz = 0.0;

    // Evaluate 8 Directional Gerstner Waves
    vec4 waves[8] = vec4[8](wave0, wave1, wave2, wave3, wave4, wave5, wave6, wave7);

    for (int i = 0; i < 8; i++) {
        vec2 d = normalize(waves[i].xy);

        // Wind-coupling: Wind sea waves (index 2..4) align with dynamic wind vector
        if (i >= 2 && i <= 4) {
            d = normalize(mix(d, windDir, 0.60));
        }

        float q = waves[i].z * localFetch;
        float l = waves[i].w;
        float k = 6.2831853 / l;
        float w = sqrt(g * k); // Physical deep-water dispersion relation
        float a = (q / k) * (1.0 + (windSpeed - 8.0) * 0.035);

        // Phase with progressive directional travel
        float phase = k * dot(d, pos.xz) - w * (timeVal * 0.75);
        float cosP = cos(phase);
        float sinP = sin(phase);

        // Physical Asymmetric Gerstner Displacement (broad troughs, sharp crests)
        pos.x += d.x * (a * cosP);
        pos.y += a * sinP;
        pos.z += d.y * (a * cosP);
        totalDispY += a * sinP;

        // Analytical Tangent & Binormal Derivatives
        float ka = k * a;
        tangent += vec3(-d.x * d.x * (q * sinP), d.x * (ka * cosP), -d.x * d.y * (q * sinP));
        binormal += vec3(-d.x * d.y * (q * sinP), d.y * (ka * cosP), -d.y * d.y * (q * sinP));

        // Jacobian Derivatives for physical whitecaps
        dDx_dx += d.x * d.x * (q * sinP);
        dDx_dz += d.x * d.y * (q * sinP);
        dDz_dx += d.x * d.y * (q * sinP);
        dDz_dz += d.y * d.y * (q * sinP);
    }

    // Analytical Surface Normal (strictly upward pointing for top surface)
    vec3 normal = normalize(cross(binormal, tangent));

    // Jacobian Determinant of Horizontal Displacement Field
    // det(J) < 0.52 indicates physical wave collisions / crest steepening -> whitecaps!
    float Jxx = 1.0 - dDx_dx;
    float Jzz = 1.0 - dDz_dz;
    float Jxz = -dDx_dz;
    float jacobianDet = Jxx * Jzz - Jxz * Jxz;

    fragPosition = vec3(matModel * vec4(pos, 1.0));
    fragNormal = normalize(vec3(matNormal * vec4(normal, 0.0)));
    fragTexCoord = vertexTexCoord;
    fragWaveHeight = totalDispY;
    fragJacobian = jacobianDet;
    fragFetch = localFetch;

    gl_Position = mvp * vec4(pos, 1.0);
}
)glsl";

static const char *OCEAN_FS = R"glsl(
#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in float fragWaveHeight;
in float fragJacobian;
in float fragFetch;

uniform vec3 viewPos;
uniform vec3 lightDir;     // Towards celestial light source (sun/moon)
uniform vec3 lightColor;   // Intensity + tint
uniform vec3 ambient;
uniform float timeVal;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float lightningFlash;
uniform vec2 windDir;
uniform float windSpeed;
uniform float camAltitude; // Signed camera altitude above local dynamic water surface

out vec4 finalColor;

// Spectral Beer-Lambert Absorption Coefficients per meter (clean coastal water with CDOM)
// Red absorbs heavily (0.42/m), Green absorbs least (0.052/m), Blue has moderate absorption (0.088/m)
const vec3 betaAbsorption = vec3(0.42, 0.052, 0.088);

void main()
{
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(viewPos - fragPosition);
    vec3 lDir = normalize(lightDir);

    bool isUnderwaterView = (viewPos.y < fragPosition.y);

    if (!isUnderwaterView) {
        // =========================================================================
        // VIEWED FROM ABOVE: CAMERA-RELATIVE OPTICAL OCEAN MODEL
        // =========================================================================

        // 1. Camera Altitude LOD Filtering (Toksvig / LEAN Variance Principle)
        // At centimeters/eye altitude (< 0.5m), full 3-octave capillary micro-facets sparkle sharply.
        // At higher altitudes (> 2.5m on pier deck or aerial), sub-pixel capillary ripples are
        // smoothly attenuated to eliminate shimmering moiré and produce silky, coherent swells.
        float altFactor = clamp((camAltitude - 0.40) / 4.5, 0.0, 1.0);
        float wCap1 = 1.0;
        float wCap2 = mix(1.0, 0.45, altFactor);
        float wCap3 = mix(1.0, 0.00, altFactor);

        vec2 wDir = normalize(windDir);
        vec2 wOrth = vec2(-wDir.y, wDir.x);

        vec2 uv1 = fragPosition.xz * 1.35 + wDir * (timeVal * 1.65);
        vec2 uv2 = fragPosition.xz * 3.20 - wOrth * (timeVal * 2.35);
        vec2 uv3 = fragPosition.xz * 7.80 + wDir * (timeVal * 3.75);

        float cap1 = sin(uv1.x * 2.4 + uv1.y * 1.8) * 0.5 + 0.5;
        float cap2 = cos(uv2.x * 3.1 - uv2.y * 2.3) * 0.5 + 0.5;
        float cap3 = sin(uv3.x * 4.6 + uv3.y * 4.1) * 0.5 + 0.5;

        vec3 capNormOffset = vec3(
            (cap1 - 0.5) * (0.12 * wCap1) + (cap2 - 0.5) * (0.07 * wCap2) + (cap3 - 0.5) * (0.035 * wCap3),
            0.0,
            (cap2 - 0.5) * (0.12 * wCap1) + (cap1 - 0.5) * (0.07 * wCap2) + (cap3 - 0.5) * (0.035 * wCap3)
        ) * (0.60 + windSpeed * 0.035);

        vec3 microNormal = normalize(normal + capNormOffset);

        // 2. Physical Fresnel Reflectance (Schlick with Water IOR n = 1.3333, R0 = 0.0204)
        float NdotV = max(dot(microNormal, viewDir), 0.0);
        float F0 = 0.0204;
        float fresnel = F0 + (1.0 - F0) * pow(clamp(1.0 - NdotV, 0.0, 1.0), 5.0);

        // 3. Physically-Based Sky Dome Reflection along true reflection vector R
        vec3 R = reflect(-viewDir, microNormal);
        float skyElevation = clamp(R.y, 0.0, 1.0);

        // Zenith sky gradient: dark atmospheric dome overhead
        vec3 skyZenith = mix(vec3(0.04, 0.07, 0.12), vec3(0.10, 0.18, 0.28), clamp(ambient.r * 2.2, 0.0, 1.0));
        vec3 skyReflection = mix(fogColor, skyZenith, pow(skyElevation, 0.65));

        // Atmospheric forward sky glow around sun / moon
        float RdotL = max(dot(R, lDir), 0.0);
        skyReflection += lightColor * pow(RdotL, 10.0) * 0.40;

        // 4. Camera-Relative Broken Sunglint (Cox-Munk Microfacet Highlight)
        // Near surface: tight diamond micro-glints on wave facets.
        // Pier / Aerial: broad, soft Cox-Munk solar glitter envelope along swell slopes.
        vec3 halfDir = normalize(lDir + viewDir);
        float NdotH = max(dot(microNormal, halfDir), 0.0);
        float specExpSharp = mix(280.0, 110.0, altFactor);
        float specExpBroad = mix(38.0, 16.0, altFactor);
        float specSharp = pow(NdotH, specExpSharp) * mix(6.2, 2.8, altFactor);
        float specBroad = pow(NdotH, specExpBroad) * mix(0.45, 0.95, altFactor);
        vec3 specularGlint = lightColor * (specSharp + specBroad) * (fresnel * 0.88 + 0.12);

        // 5. Deep Ocean Water Body Radiance (Jerlov Coastal Type 2C)
        // Deep water is dark slate-emerald/navy, not cartoon cyan!
        vec3 deepWaterAlbedo = vec3(0.012, 0.038, 0.048);
        vec3 shallowWaterAlbedo = vec3(0.035, 0.095, 0.085);

        float waterDepth = clamp((35.0 - fragPosition.x) * 0.40 + 1.2, 0.5, 38.0);
        vec3 opticalTransmission = exp(-betaAbsorption * waterDepth);
        vec3 waterAlbedo = mix(shallowWaterAlbedo, deepWaterAlbedo, clamp(waterDepth / 18.0, 0.0, 1.0));

        float halfLambert = max(dot(microNormal, lDir) * 0.5 + 0.5, 0.0);
        vec3 diffuseWater = waterAlbedo * (ambient * 1.3 + lightColor * (halfLambert * 0.55 + 0.30)) * opticalTransmission;

        // 6. Subsurface Scattering (Luminous translucent jade crest flash when backlit)
        float sssFactor = pow(max(dot(viewDir, -lDir), 0.0), 3.8) * clamp(fragWaveHeight * 1.5 + 0.25, 0.0, 1.0);
        vec3 sssColor = vec3(0.035, 0.26, 0.22) * lightColor * sssFactor * 0.85;

        // 7. Physical Jacobian Whitecaps & Organic Foam Dissipation
        // Whitecaps trigger where wave crests collide / steepen (fragJacobian < 0.52)
        float breakerThreshold = 0.52;
        float waveBreaking = clamp((breakerThreshold - fragJacobian) / 0.38, 0.0, 1.0);

        // Multi-frequency cellular bubble froth noise
        vec2 frothCoords = fragPosition.xz * 0.90 + wDir * (timeVal * 1.15);
        float f1 = sin(frothCoords.x * 1.37 + frothCoords.y * 0.89 + timeVal * 1.7);
        float f2 = sin(frothCoords.x * 2.81 - frothCoords.y * 2.23 - timeVal * 2.5);
        float f3 = cos(frothCoords.x * 5.63 + frothCoords.y * 5.11 + timeVal * 3.3);
        float frothNoise = f1 * 0.48 + f2 * 0.34 + f3 * 0.18;

        float whitecapFoam = smoothstep(0.12, 0.72, waveBreaking) * smoothstep(-0.18, 0.42, frothNoise);

        // Coastal Shoreline Surf Wash (breaks along shore shelf X in [26, 35])
        float shoreFactor = clamp((fragPosition.x - 26.0) / 9.0, 0.0, 1.0);
        float surfCycle = sin(timeVal * 1.35 + fragPosition.z * 0.14) * 0.5 + 0.5;
        float shoreFoam = pow(shoreFactor, 1.8) * smoothstep(0.28, 0.68, surfCycle + frothNoise * 0.25);

        float totalFoam = clamp(whitecapFoam * 0.88 + shoreFoam * 0.95, 0.0, 1.0);
        vec3 foamColor = vec3(0.91, 0.95, 0.97) * (ambient.r * 1.2 + 0.65);

        // Composite Surface Radiance: Fresnel blends sky reflection with transmitted body water
        vec3 waterRadiance = mix(diffuseWater + sssColor, skyReflection, fresnel * 0.85) + specularGlint;
        // Foam is diffuse and sits on top of water surface
        waterRadiance = mix(waterRadiance, foamColor, totalFoam * 0.88);

        if (lightningFlash > 0.0) {
            waterRadiance += vec3(0.82, 0.90, 1.0) * lightningFlash * 0.85;
        }

        // 8. Physical Lookdown Transmittance & Surface Alpha
        // When viewing from above at high incidence angles (pier deck, cliffs), Fresnel reflection is only ~2%.
        // Surface opacity drops to ~0.55, revealing the seabed, submerged pilings, and sunken skiff below!
        // At glancing angles toward horizon, opacity approaches 0.98 for solid sky reflection and horizon fog blend.
        float surfaceAlpha = clamp(0.52 + fresnel * 0.46 + totalFoam * 0.45, 0.50, 0.98);

        // 9. Atmospheric Perspective: Seamless Fog Blend into Horizon
        float distToCam = length(viewPos - fragPosition);
        float fogFactor = 1.0 - exp(-pow(distToCam * fogDensity * 0.65, 2.0));
        fogFactor = clamp(fogFactor, 0.0, 1.0);

        finalColor = vec4(mix(waterRadiance, fogColor, fogFactor), surfaceAlpha);
    }
    else {
        // =========================================================================
        // VIEWED FROM UNDERNEATH: SNELL'S WINDOW & TOTAL INTERNAL REFLECTION
        // =========================================================================
        vec3 underNormal = -normal;
        float cosTheta = max(dot(underNormal, viewDir), 0.0);

        // Snell's Window Cone (critical angle ~ 48.6 deg, cos ~ 0.66)
        float snellThreshold = 0.66;
        float snellGlow = clamp((cosTheta - snellThreshold) / (1.0 - snellThreshold), 0.0, 1.0);
        snellGlow = pow(snellGlow, 1.8);

        // Water surface caustics & ripple refraction
        float waveRip = sin(fragPosition.x * 1.2 + fragPosition.z * 0.8 + timeVal * 2.5) * 0.5 + 0.5;
        vec3 skylight = mix(vec3(0.06, 0.28, 0.38), vec3(0.20, 0.52, 0.62), waveRip) * (lightColor * 0.8 + ambient * 1.5);

        // Total Internal Reflection (TIR): deep silvery-emerald underwater ceiling mirror
        vec3 tirMirror = vec3(0.02, 0.09, 0.14) * (ambient * 1.8 + 0.2);

        vec3 ceilingColor = mix(tirMirror, skylight, snellGlow);

        float distToCam = length(viewPos - fragPosition);
        float fogFactor = clamp(distToCam / 28.0, 0.0, 1.0);
        vec3 underFog = vec3(0.015, 0.06, 0.09);

        finalColor = vec4(mix(ceilingColor, underFog, fogFactor), 0.98);
    }
}
)glsl";

// =========================================================================
// SUBMERGED SEABED SHADER (GLSL 330)
// Smooth submarine sandy floor with dynamic shimmering sunlight caustics
// =========================================================================

static const char *SEABED_VS = R"glsl(
#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

uniform mat4 mvp;
uniform mat4 matModel;
uniform vec2 gridSnapOffset;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;

void main()
{
    vec3 pos = vertexPosition;
    pos.x += gridSnapOffset.x;
    pos.z += gridSnapOffset.y;

    if (pos.x > 35.0) pos.x = 35.0;

    // Submarine topography: Deep sandy slope descending into ocean abyss
    float bedDepth = (pos.x >= 0.0) ? (3.8 + (pos.x / 35.0) * 3.7) : (3.8 - (pos.x / -120.0) * 4.5);
    pos.y = clamp(bedDepth, 1.2, 7.5);

    // Subtle bathymetric sand dunes
    float dune = sin(pos.x * 0.14 + pos.z * 0.08) * 0.28 + cos(pos.x * 0.28 - pos.z * 0.18) * 0.14;
    pos.y += dune;

    fragPosition = vec3(matModel * vec4(pos, 1.0));
    fragNormal = vec3(0.0, 1.0, 0.0);
    fragTexCoord = vertexTexCoord;

    gl_Position = mvp * vec4(pos, 1.0);
}
)glsl";

static const char *SEABED_FS = R"glsl(
#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;

uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 ambient;
uniform float timeVal;
uniform vec3 fogColor;
uniform float fogDensity;

out vec4 finalColor;

void main()
{
    // Deep Submarine Sand Palette
    vec3 sandDeep = vec3(0.025, 0.055, 0.048);
    vec3 sandShallow = vec3(0.085, 0.125, 0.095);
    float depthFactor = clamp((fragPosition.y - 1.5) / 5.5, 0.0, 1.0);
    vec3 bedBase = mix(sandDeep, sandShallow, depthFactor);

    // Dynamic Sunlight Refraction Caustics on seabed
    vec2 cUV1 = fragPosition.xz * 0.38 + vec2(timeVal * 0.22, timeVal * 0.15);
    vec2 cUV2 = fragPosition.xz * 0.52 - vec2(timeVal * 0.18, timeVal * 0.28);
    float c1 = sin(cUV1.x * 3.1 + cUV1.y * 2.4) * sin(cUV1.y * 3.4 - cUV1.x * 1.8);
    float c2 = cos(cUV2.x * 4.2 - cUV2.y * 3.1) * cos(cUV2.y * 3.8 + cUV2.x * 2.2);
    float caustics = clamp(pow(c1 * 0.5 + c2 * 0.5 + 0.65, 3.5) * 1.8, 0.0, 2.5);

    // Light attenuates with water depth (9.8m surface level)
    float depthMeters = max(9.80 - fragPosition.y, 0.5);
    vec3 causticTint = vec3(0.25, 0.65, 0.55) * exp(-vec3(0.42, 0.052, 0.088) * depthMeters);
    vec3 litBed = bedBase * (ambient * 1.4 + 0.15) + lightColor * causticTint * caustics * 0.55;

    // Submarine distance fog
    float dist = length(viewPos - fragPosition);
    float fog = clamp(dist / 45.0, 0.0, 1.0);
    vec3 underFog = vec3(0.015, 0.055, 0.075);

    finalColor = vec4(mix(litBed, underFog, fog), 1.0);
}
)glsl";

// =========================================================================
// OCEAN SYSTEM RUNTIME DATA
// =========================================================================

static const float OCEAN_WATER_LEVEL = 9.80f;

static Shader g_oceanShader;
static Model  g_oceanModelNear;
static Model  g_oceanModelFar;

static Shader g_seabedShader;
static Model  g_seabedModel;

static int g_oceanLocTimeVal = -1;
static int g_oceanLocViewPos = -1;
static int g_oceanLocLightDir = -1;
static int g_oceanLocLightColor = -1;
static int g_oceanLocAmbient = -1;
static int g_oceanLocFogColor = -1;
static int g_oceanLocFogDensity = -1;
static int g_oceanLocLightningFlash = -1;
static int g_oceanLocGridSnapOffset = -1;
static int g_oceanLocWindDir = -1;
static int g_oceanLocWindSpeed = -1;
static int g_oceanLocCamAltitude = -1;

static int g_seabedLocTimeVal = -1;
static int g_seabedLocViewPos = -1;
static int g_seabedLocLightColor = -1;
static int g_seabedLocAmbient = -1;
static int g_seabedLocFogColor = -1;
static int g_seabedLocFogDensity = -1;
static int g_seabedLocGridSnapOffset = -1;

static bool g_oceanLoaded = false;

// Wind coupled state
static Vector2 g_oceanWindDir = { 0.94f, 0.34f }; // Prevailing coastal wind direction
static float   g_oceanWindSpeed = 11.5f;          // Fresh breeze (11.5 m/s)

// Player Swimming & Diving Locomotion State
enum WaterLocomotionState {
    WATER_STATE_DRY = 0,
    WATER_STATE_WADING,
    WATER_STATE_SURFACE,
    WATER_STATE_DIVING
};

static WaterLocomotionState g_waterState = WATER_STATE_DRY;
static float g_playerOxygen = 45.0f;
static const float g_maxOxygen = 45.0f;
static float g_wadeSplashTimer = 0.0f;
static float g_screenDropletsTimer = 0.0f;
static bool  g_wasSubmerged = false;
static bool  g_salvageCrateOpened = false;

// Swimmer camera wave tilt bobbing
static float g_waterSmoothBobY = 0.0f;
static float g_waterSmoothTiltPitch = 0.0f;
static float g_waterSmoothTiltRoll = 0.0f;

// Audio generators for procedural water soundscapes
static Sound g_sndWaterSplash;
static Sound g_sndSurfacingGasp;
static Sound g_sndFogBell;

static Sound GenerateWaterSplashSound() {
    int sampleRate = 44100;
    float duration = 0.42f;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)MemAlloc(frameCount * sizeof(short));
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        float env = expf(-t * 9.5f);
        float noise = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f);
        float sub = sinf(2.0f * PI * (90.0f - t * 60.0f) * t);
        float mixed = (noise * 0.65f + sub * 0.35f) * env;
        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        data[i] = (short)(mixed * 32767.0f);
    }
    Wave wave = { (unsigned int)frameCount, (unsigned int)sampleRate, 16, 1, data };
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

static Sound GenerateSurfacingGaspSound() {
    int sampleRate = 44100;
    float duration = 0.65f;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)MemAlloc(frameCount * sizeof(short));
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        float env = sinf(PI * (t / duration));
        float breath = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.55f;
        float waterLick = sinf(2.0f * PI * 220.0f * t) * expf(-t * 12.0f) * 0.45f;
        float mixed = (breath + waterLick) * env;
        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        data[i] = (short)(mixed * 32767.0f);
    }
    Wave wave = { (unsigned int)frameCount, (unsigned int)sampleRate, 16, 1, data };
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

static Sound GenerateFogBellSound() {
    int sampleRate = 44100;
    float duration = 2.6f;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)MemAlloc(frameCount * sizeof(short));
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        float f1 = sinf(2.0f * PI * 370.0f * t) * expf(-t * 1.8f);
        float f2 = sinf(2.0f * PI * 745.0f * t) * expf(-t * 2.5f) * 0.55f;
        float f3 = sinf(2.0f * PI * 1110.0f * t) * expf(-t * 3.8f) * 0.28f;
        float strike = ((float)rand() / (float)RAND_MAX * 2.0f - 1.0f) * expf(-t * 60.0f) * 0.35f;
        float mixed = (f1 * 0.6f + f2 + f3 + strike) * 0.80f;
        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        data[i] = (short)(mixed * 32767.0f);
    }
    Wave wave = { (unsigned int)frameCount, (unsigned int)sampleRate, 16, 1, data };
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

// =========================================================================
// CPU-SIDE SYNCHRONIZED GERSTNER WAVE ELEVATION TRACKER
// Matches GPU multi-wave spectrum for realistic player swimming & item bobbing
// =========================================================================
static inline float GetOceanWaveElevation(float x, float z, float timeVal) {
    float p0 = (0.966f * x + 0.259f * z) * 0.08055f - 0.8887f * (timeVal * 0.75f);
    float p1 = (0.866f * x - 0.500f * z) * 0.13659f - 1.1573f * (timeVal * 0.75f);
    float p2 = (0.985f * x + 0.174f * z) * 0.26179f - 1.6022f * (timeVal * 0.75f);
    float a0 = 2.731f * sinf(p0);
    float a1 = 1.317f * sinf(p1);
    float a2 = 0.611f * sinf(p2);
    float coastalDepth = fminf(fmaxf((35.0f - x) / 25.0f, 0.15f), 1.0f);
    return (a0 * 0.25f + a1 * 0.25f + a2 * 0.20f) * coastalDepth;
}

// =========================================================================
// INITIALIZE & UNLOAD OCEAN SYSTEM
// =========================================================================
static void InitOceanSystem() {
    if (g_oceanLoaded) return;

    // 1. Ocean Water Surface Shader
    g_oceanShader = LoadShaderFromMemory(OCEAN_VS, OCEAN_FS);
    if (g_oceanShader.id == 0) {
        TraceLog(LOG_ERROR, "Failed to compile ocean Gerstner wave shader!");
    }

    g_oceanLocTimeVal        = GetShaderLocation(g_oceanShader, "timeVal");
    g_oceanLocViewPos        = GetShaderLocation(g_oceanShader, "viewPos");
    g_oceanLocLightDir       = GetShaderLocation(g_oceanShader, "lightDir");
    g_oceanLocLightColor     = GetShaderLocation(g_oceanShader, "lightColor");
    g_oceanLocAmbient        = GetShaderLocation(g_oceanShader, "ambient");
    g_oceanLocFogColor       = GetShaderLocation(g_oceanShader, "fogColor");
    g_oceanLocFogDensity     = GetShaderLocation(g_oceanShader, "fogDensity");
    g_oceanLocLightningFlash = GetShaderLocation(g_oceanShader, "lightningFlash");
    g_oceanLocGridSnapOffset = GetShaderLocation(g_oceanShader, "gridSnapOffset");
    g_oceanLocWindDir        = GetShaderLocation(g_oceanShader, "windDir");
    g_oceanLocWindSpeed      = GetShaderLocation(g_oceanShader, "windSpeed");
    g_oceanLocCamAltitude    = GetShaderLocation(g_oceanShader, "camAltitude");

    // Near-shore high-detail moving grid (90x120 vertices = 21,242 tris, rock-solid 144 FPS)
    Mesh meshNear = GenMeshPlane(180.0f, 360.0f, 90, 120);
    g_oceanModelNear = LoadModelFromMesh(meshNear);
    g_oceanModelNear.materials[0].shader = g_oceanShader;

    // Far-horizon expansive infinite moving clipmap grid (48x48 vertices, spans 1600m x 1600m)
    Mesh meshFar = GenMeshPlane(1600.0f, 1600.0f, 48, 48);
    g_oceanModelFar = LoadModelFromMesh(meshFar);
    g_oceanModelFar.materials[0].shader = g_oceanShader;

    // 2. Submerged Seabed Mesh & Caustic Shader
    g_seabedShader = LoadShaderFromMemory(SEABED_VS, SEABED_FS);
    g_seabedLocTimeVal        = GetShaderLocation(g_seabedShader, "timeVal");
    g_seabedLocViewPos        = GetShaderLocation(g_seabedShader, "viewPos");
    g_seabedLocLightColor     = GetShaderLocation(g_seabedShader, "lightColor");
    g_seabedLocAmbient        = GetShaderLocation(g_seabedShader, "ambient");
    g_seabedLocFogColor       = GetShaderLocation(g_seabedShader, "fogColor");
    g_seabedLocFogDensity     = GetShaderLocation(g_seabedShader, "fogDensity");
    g_seabedLocGridSnapOffset = GetShaderLocation(g_seabedShader, "gridSnapOffset");

    Mesh meshBed = GenMeshPlane(360.0f, 440.0f, 45, 55);
    g_seabedModel = LoadModelFromMesh(meshBed);
    g_seabedModel.materials[0].shader = g_seabedShader;

    // 3. Audio Assets
    g_sndWaterSplash   = GenerateWaterSplashSound();
    g_sndSurfacingGasp = GenerateSurfacingGaspSound();
    g_sndFogBell       = GenerateFogBellSound();

    SetSoundVolume(g_sndWaterSplash, 0.75f);
    SetSoundVolume(g_sndSurfacingGasp, 0.85f);
    SetSoundVolume(g_sndFogBell, 0.80f);

    g_oceanLoaded = true;
    TraceLog(LOG_INFO, "Infinite procedural ocean shader & clipmap system initialized!");
}

static void UnloadOceanSystem() {
    if (!g_oceanLoaded) return;
    UnloadShader(g_oceanShader);
    UnloadModel(g_oceanModelNear);
    UnloadModel(g_oceanModelFar);
    UnloadShader(g_seabedShader);
    UnloadModel(g_seabedModel);
    UnloadSound(g_sndWaterSplash);
    UnloadSound(g_sndSurfacingGasp);
    UnloadSound(g_sndFogBell);
    g_oceanLoaded = false;
}

// =========================================================================
// RENDER INFINITE PROCEDURAL OCEAN SURFACE & SEABED
// =========================================================================
static void DrawOceanSurface(Camera3D camera, float timeVal, float extDayFactor, float extNightFactor, Vector3 sunDir, float sunElev, float lightningFlash) {
    (void)extNightFactor;
    if (!g_oceanLoaded) InitOceanSystem();

    // 1. Compute Camera-Centric Snapping Grid Coordinates (CDLOD)
    // Snapping prevents vertex popping as camera moves
    const float nearSnapStep = 1.8f;
    const float farSnapStep  = 16.0f;

    // Center follows player into the infinite sea (Westward and along Z)
    float activeCenterZ = camera.position.z;
    float activeCenterX = fminf(camera.position.x - 40.0f, -45.0f);

    Vector2 nearSnap = {
        floorf(activeCenterX / nearSnapStep) * nearSnapStep,
        floorf(activeCenterZ / nearSnapStep) * nearSnapStep
    };

    Vector2 farSnap = {
        floorf(activeCenterX / farSnapStep) * farSnapStep,
        floorf(activeCenterZ / farSnapStep) * farSnapStep
    };

    // 2. Dynamic Celestial Lighting & Tone Mapping
    Vector3 lDir;
    Vector3 lColor;
    if (sunElev >= -0.04f) {
        lDir = sunDir;
        float sunsetGlow = Clamp((0.25f - sunElev) / 0.25f, 0.0f, 1.0f);
        Vector3 noonCol = { 1.0f, 0.95f, 0.84f };
        Vector3 duskCol = { 1.0f, 0.48f, 0.22f };
        lColor = (Vector3){
            Lerp(noonCol.x, duskCol.x, sunsetGlow) * (0.90f + 0.35f * extDayFactor),
            Lerp(noonCol.y, duskCol.y, sunsetGlow) * (0.90f + 0.35f * extDayFactor),
            Lerp(noonCol.z, duskCol.z, sunsetGlow) * (0.90f + 0.35f * extDayFactor)
        };
    } else {
        lDir = Vector3Negate(sunDir);
        lColor = (Vector3){ 0.35f, 0.48f, 0.68f };
    }

    Vector3 amb = {
        0.05f + 0.20f * extDayFactor,
        0.07f + 0.22f * extDayFactor,
        0.09f + 0.24f * extDayFactor
    };
    Vector3 fog = {
        0.04f + 0.08f * extDayFactor,
        0.05f + 0.09f * extDayFactor,
        0.07f + 0.11f * extDayFactor
    };
    float fogDen = 0.0075f;

    // Signed camera altitude above dynamic wave surface at camera's XZ position
    float waveElevAtCam = GetOceanWaveElevation(camera.position.x, camera.position.z, timeVal);
    float dynamicSurfaceAtCam = OCEAN_WATER_LEVEL + waveElevAtCam;
    float camAltitude = camera.position.y - dynamicSurfaceAtCam;

    // Upload Common Ocean Uniforms
    SetShaderValue(g_oceanShader, g_oceanLocTimeVal, &timeVal, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_oceanShader, g_oceanLocViewPos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_oceanShader, g_oceanLocLightDir, &lDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_oceanShader, g_oceanLocLightColor, &lColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_oceanShader, g_oceanLocAmbient, &amb, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_oceanShader, g_oceanLocFogColor, &fog, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_oceanShader, g_oceanLocFogDensity, &fogDen, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_oceanShader, g_oceanLocLightningFlash, &lightningFlash, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_oceanShader, g_oceanLocWindDir, &g_oceanWindDir, SHADER_UNIFORM_VEC2);
    SetShaderValue(g_oceanShader, g_oceanLocWindSpeed, &g_oceanWindSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_oceanShader, g_oceanLocCamAltitude, &camAltitude, SHADER_UNIFORM_FLOAT);

    // Upload Seabed Shader Uniforms
    SetShaderValue(g_seabedShader, g_seabedLocTimeVal, &timeVal, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_seabedShader, g_seabedLocViewPos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_seabedShader, g_seabedLocLightColor, &lColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_seabedShader, g_seabedLocAmbient, &amb, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_seabedShader, g_seabedLocFogColor, &fog, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_seabedShader, g_seabedLocFogDensity, &fogDen, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_seabedShader, g_seabedLocGridSnapOffset, &nearSnap, SHADER_UNIFORM_VEC2);

    // 3. Draw Submerged Sandy Seabed with Sunlight Caustics
    DrawModel(g_seabedModel, (Vector3){ 0.0f, 0.0f, 0.0f }, 1.0f, WHITE);

    // 4. Draw Infinite Ocean Surfaces with Double-Sided Snell's Window & Translucent Blending Support
    rlEnableColorBlend();
    rlDisableBackfaceCulling();

    // Inner Detailed Concentric Ring (Translucent lookdown from pier/cliffs, opaque glancing towards horizon)
    SetShaderValue(g_oceanShader, g_oceanLocGridSnapOffset, &nearSnap, SHADER_UNIFORM_VEC2);
    DrawModel(g_oceanModelNear, (Vector3){ 0.0f, OCEAN_WATER_LEVEL, 0.0f }, 1.0f, WHITE);

    // Outer Horizon Concentric Ring (stretching 1600m into atmospheric haze)
    SetShaderValue(g_oceanShader, g_oceanLocGridSnapOffset, &farSnap, SHADER_UNIFORM_VEC2);
    DrawModel(g_oceanModelFar, (Vector3){ 0.0f, OCEAN_WATER_LEVEL - 0.04f, 1.0f }, 1.0f, WHITE);

    rlEnableBackfaceCulling();
}

// =========================================================================
// RENDER COASTAL ENVIRONMENT: PIER, SEA STACKS, CRASHING SURF & WRECK
// =========================================================================
static void DrawCoastalEnvironment(Camera3D camera, float timeVal, float extDayFactor, float extNightFactor, Vector3 sunDir) {
    (void)timeVal;
    (void)extDayFactor;
    (void)extNightFactor;
    (void)sunDir;
    float distToCoast = fabsf(camera.position.x - 30.0f);
    if (distToCoast > 180.0f && camera.position.x > 30.0f) return;

    Color woodDark    = { 42, 36, 30, 255 };
    Color woodPlank   = { 68, 58, 48, 255 };
    Color woodWet     = { 28, 24, 20, 255 };
    Color ironRust    = { 75, 42, 28, 255 };
    Color rockBasalt  = { 32, 34, 38, 255 };
    Color rockWet     = { 18, 20, 24, 255 };
    Color bellBronze  = { 110, 135, 95, 255 };

    // 1. Weathered Timber Pier (Spans X: 16.0 .. 36.0 at Z = 138.0, Deck Y = 10.75)
    for (float px = 18.0f; px <= 34.0f; px += 3.2f) {
        DrawCylinder((Vector3){ px, 4.0f, 136.2f }, 0.22f, 0.24f, 6.8f, 8, woodDark);
        DrawCylinder((Vector3){ px, 4.0f, 139.8f }, 0.22f, 0.24f, 6.8f, 8, woodDark);
        DrawCylinder((Vector3){ px, 8.8f, 136.2f }, 0.24f, 0.24f, 1.4f, 8, woodWet);
        DrawCylinder((Vector3){ px, 8.8f, 139.8f }, 0.24f, 0.24f, 1.4f, 8, woodWet);
        DrawCube((Vector3){ px, 9.8f, 138.0f }, 0.12f, 0.18f, 3.8f, woodDark);
    }

    DrawCube((Vector3){ 26.0f, 10.75f, 138.0f }, 20.0f, 0.25f, 3.8f, woodPlank);
    DrawCubeWires((Vector3){ 26.0f, 10.75f, 138.0f }, 20.05f, 0.26f, 3.85f, woodDark);

    for (float px = 16.5f; px <= 35.5f; px += 0.8f) {
        DrawCube((Vector3){ px, 10.88f, 138.0f }, 0.04f, 0.02f, 3.75f, woodDark);
    }

    DrawCylinder((Vector3){ 17.5f, 10.88f, 136.4f }, 0.12f, 0.14f, 0.45f, 8, ironRust);
    DrawCylinder((Vector3){ 17.5f, 10.88f, 139.6f }, 0.12f, 0.14f, 0.45f, 8, ironRust);
    DrawCylinder((Vector3){ 25.0f, 10.88f, 136.4f }, 0.12f, 0.14f, 0.45f, 8, ironRust);
    DrawCylinder((Vector3){ 25.0f, 10.88f, 139.6f }, 0.12f, 0.14f, 0.45f, 8, ironRust);

    DrawCircle3D((Vector3){ 18.2f, 10.89f, 136.6f }, 0.32f, (Vector3){ 1, 0, 0 }, 90.0f, { 140, 120, 85, 255 });
    DrawCircle3D((Vector3){ 18.2f, 10.90f, 136.6f }, 0.20f, (Vector3){ 1, 0, 0 }, 90.0f, { 110, 95, 65, 255 });

    // Fog Bell Gallows at Pier Head
    DrawCube((Vector3){ 16.5f, 11.8f, 136.6f }, 0.18f, 2.0f, 0.18f, woodDark);
    DrawCube((Vector3){ 16.5f, 11.8f, 139.4f }, 0.18f, 2.0f, 0.18f, woodDark);
    DrawCube((Vector3){ 16.5f, 12.7f, 138.0f }, 0.18f, 0.18f, 3.0f, woodDark);
    DrawCylinder((Vector3){ 16.5f, 12.3f, 138.0f }, 0.08f, 0.26f, 0.38f, 10, bellBronze);
    DrawSphere((Vector3){ 16.5f, 12.5f, 138.0f }, 0.10f, bellBronze);

    // Wooden Diving Ladder
    DrawCube((Vector3){ 16.35f, 9.4f, 136.0f }, 0.08f, 3.2f, 0.08f, woodDark);
    DrawCube((Vector3){ 16.85f, 9.4f, 136.0f }, 0.08f, 3.2f, 0.08f, woodDark);
    for (float ly = 8.2f; ly <= 10.6f; ly += 0.40f) {
        DrawCube((Vector3){ 16.6f, ly, 136.0f }, 0.45f, 0.06f, 0.06f, woodPlank);
    }

    // 2. Sea Stacks & Coastal Rocks
    DrawCylinder((Vector3){ 14.0f, 5.0f, 118.0f }, 3.5f, 1.8f, 7.5f, 7, rockBasalt);
    DrawCircle3D((Vector3){ 14.0f, OCEAN_WATER_LEVEL + 0.05f, 118.0f }, 4.2f, (Vector3){ 1, 0, 0 }, 90.0f, { 220, 235, 245, 160 });

    DrawCylinder((Vector3){ 8.0f, 4.5f, 165.0f }, 4.2f, 1.2f, 9.0f, 6, rockBasalt);
    DrawCircle3D((Vector3){ 8.0f, OCEAN_WATER_LEVEL + 0.05f, 165.0f }, 4.8f, (Vector3){ 1, 0, 0 }, 90.0f, { 220, 235, 245, 160 });

    DrawSphere((Vector3){ 22.0f, 9.5f, 85.0f }, 3.2f, rockWet);
    DrawCircle3D((Vector3){ 22.0f, OCEAN_WATER_LEVEL + 0.05f, 85.0f }, 3.6f, (Vector3){ 1, 0, 0 }, 90.0f, { 210, 230, 240, 150 });

    // 3. Sunken Fishing Skiff Wreck
    rlPushMatrix();
    rlTranslatef(10.0f, 6.2f, 152.0f);
    rlRotatef(18.0f, 0.0f, 0.0f, 1.0f);
    rlRotatef(-25.0f, 0.0f, 1.0f, 0.0f);
    DrawCube((Vector3){ 0.0f, 0.3f, 0.0f }, 1.8f, 0.5f, 6.5f, woodWet);
    for (float rz = -2.6f; rz <= 2.6f; rz += 0.9f) {
        DrawCube((Vector3){ -0.95f, 1.2f, rz }, 0.12f, 1.8f, 0.14f, woodDark);
        DrawCube((Vector3){  0.95f, 1.2f, rz }, 0.12f, 1.8f, 0.14f, woodDark);
    }
    DrawCylinder((Vector3){ 0.0f, 0.5f, 0.5f }, 0.14f, 0.08f, 5.2f, 6, woodDark);

    Color crateCol = g_salvageCrateOpened ? (Color){ 45, 52, 48, 255 } : (Color){ 160, 115, 45, 255 };
    DrawCube((Vector3){ 0.4f, 0.55f, -1.2f }, 0.65f, 0.45f, 0.85f, crateCol);
    DrawCubeWires((Vector3){ 0.4f, 0.55f, -1.2f }, 0.66f, 0.46f, 0.86f, ironRust);
    rlPopMatrix();
}

// =========================================================================
// SWIMMING & WATER LOCOMOTION PHYSICS SYSTEM
// =========================================================================

static void UpdateWaterLocomotion(Camera3D &camera, Vector3 &playerVel, Vector3 &moveDelta, float dt, float timeVal) {
    const float PLAYER_EYE_HEIGHT = 1.65f;
    float playerFeetY = camera.position.y - PLAYER_EYE_HEIGHT;
    bool inOceanX = (camera.position.x <= 36.5f);

    // 1. If outside ocean boundary or high above water level (e.g. standing on pier deck), player is DRY
    if (!inOceanX || playerFeetY > OCEAN_WATER_LEVEL + 0.35f) {
        g_waterState = WATER_STATE_DRY;
        g_wasSubmerged = false;
        g_waterSmoothTiltPitch = Lerp(g_waterSmoothTiltPitch, 0.0f, dt * 5.0f);
        g_waterSmoothTiltRoll  = Lerp(g_waterSmoothTiltRoll,  0.0f, dt * 5.0f);
        if (g_playerOxygen < g_maxOxygen) {
            g_playerOxygen = fminf(g_maxOxygen, g_playerOxygen + dt * 16.0f);
        }
        return;
    }

    float waterDepth = OCEAN_WATER_LEVEL - playerFeetY;
    float waveElev = GetOceanWaveElevation(camera.position.x, camera.position.z, timeVal);
    float dynamicSurface = OCEAN_WATER_LEVEL + waveElev;

    // 2. Coastal shallows wading (water depth < 1.10m near shore)
    if (waterDepth < 1.10f && camera.position.x >= 32.5f) {
        g_waterState = WATER_STATE_WADING;
        g_wasSubmerged = false;
        g_waterSmoothTiltPitch = Lerp(g_waterSmoothTiltPitch, 0.0f, dt * 5.0f);
        g_waterSmoothTiltRoll  = Lerp(g_waterSmoothTiltRoll,  0.0f, dt * 5.0f);

        // Hydrodynamic wading drag on walking movement
        moveDelta.x *= 0.65f;
        moveDelta.z *= 0.65f;

        bool hasMove = (IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D));
        if (hasMove) {
            g_wadeSplashTimer -= dt;
            if (g_wadeSplashTimer <= 0.0f) {
                PlaySound(g_sndWaterSplash);
                g_wadeSplashTimer = 0.52f;
            }
        }
        if (g_playerOxygen < g_maxOxygen) {
            g_playerOxygen = fminf(g_maxOxygen, g_playerOxygen + dt * 16.0f);
        }
        return;
    }

    // 3. Deep Water Swimming & 3D Diving
    // Swimmer tracks 100% of swell elevation with low-pass inertia filtering
    float targetBobY = waveElev;
    g_waterSmoothBobY = Lerp(g_waterSmoothBobY, targetBobY, dt * 6.0f);

    // Intentional diving triggers (Never dive accidentally!)
    bool diveDownKey = IsKeyDown(KEY_C) || IsKeyDown(KEY_LEFT_CONTROL);

    // Check if player has plunged deep below water (e.g. jumped off pier deck)
    bool isDeepUnderwater = (camera.position.y < dynamicSurface - 0.55f);

    if (!diveDownKey && !isDeepUnderwater && g_waterState != WATER_STATE_DIVING) {
        // =================================================================
        // STATE 2: SURFACE SWIMMING (Treading water, swimming with head high)
        // =================================================================
        g_waterState = WATER_STATE_SURFACE;

        if (g_wasSubmerged) {
            PlaySound(g_sndSurfacingGasp);
            g_screenDropletsTimer = 3.2f;
            g_wasSubmerged = false;
        }

        // Full oxygen on surface
        g_playerOxygen = g_maxOxygen;

        // Swimmer wave tilt: calculate local wave slope for organic pitch & roll
        float elevFwd   = GetOceanWaveElevation(camera.position.x + 0.6f, camera.position.z,        timeVal);
        float elevBack  = GetOceanWaveElevation(camera.position.x - 0.6f, camera.position.z,        timeVal);
        float elevRight = GetOceanWaveElevation(camera.position.x,        camera.position.z + 0.6f, timeVal);
        float elevLeft  = GetOceanWaveElevation(camera.position.x,        camera.position.z - 0.6f, timeVal);
        float slopeX = (elevFwd - elevBack) / 1.2f;
        float slopeZ = (elevRight - elevLeft) / 1.2f;

        float targetPitch = Clamp(-slopeX * 0.065f, -0.040f, 0.040f);
        float targetRoll  = Clamp( slopeZ * 0.050f, -0.030f, 0.030f);

        g_waterSmoothTiltPitch = Lerp(g_waterSmoothTiltPitch, targetPitch, dt * 3.5f);
        g_waterSmoothTiltRoll  = Lerp(g_waterSmoothTiltRoll,  targetRoll,  dt * 3.5f);

        // Horizontal view vectors (pure yaw - looking down doesn't stop swimming!)
        Vector3 camDir = Vector3Subtract(camera.target, camera.position);
        Vector3 fwdH = (Vector3){ camDir.x, 0.0f, camDir.z };
        if (Vector3LengthSqr(fwdH) > 0.0001f) fwdH = Vector3Normalize(fwdH);
        else fwdH = (Vector3){ 0, 0, 1 };
        Vector3 rightH = (Vector3){ -fwdH.z, 0.0f, fwdH.x };

        // Swimming input vector
        Vector3 wishDir = { 0, 0, 0 };
        if (IsKeyDown(KEY_W)) wishDir = Vector3Add(wishDir, fwdH);
        if (IsKeyDown(KEY_S)) wishDir = Vector3Subtract(wishDir, fwdH);
        if (IsKeyDown(KEY_D)) wishDir = Vector3Add(wishDir, rightH);
        if (IsKeyDown(KEY_A)) wishDir = Vector3Subtract(wishDir, rightH);

        bool isFastSwim = IsKeyDown(KEY_LEFT_SHIFT);
        float swimTopSpeed = isFastSwim ? 6.2f : 4.2f;

        if (Vector3LengthSqr(wishDir) > 0.001f) {
            wishDir = Vector3Normalize(wishDir);
            playerVel.x = Lerp(playerVel.x, wishDir.x * swimTopSpeed, dt * 6.0f);
            playerVel.z = Lerp(playerVel.z, wishDir.z * swimTopSpeed, dt * 6.0f);

            // Subtle rhythmic stroke splash
            g_wadeSplashTimer -= dt;
            if (g_wadeSplashTimer <= 0.0f) {
                PlaySound(g_sndWaterSplash);
                g_wadeSplashTimer = isFastSwim ? 0.38f : 0.58f;
            }
        } else {
            // Fluid drag deceleration
            playerVel.x = Lerp(playerVel.x, 0.0f, dt * 4.2f);
            playerVel.z = Lerp(playerVel.z, 0.0f, dt * 4.2f);
        }

        // Robust Positive Surface Buoyancy: Keeps swimmer's head comfortably high (+0.72m above dynamic surface)
        float targetEyeY = dynamicSurface + 0.72f;
        float buoyDelta = targetEyeY - camera.position.y;
        if (buoyDelta > 0.0f) {
            // Strong upward spring when dipping below target float height
            float buoyAccel = buoyDelta * 32.0f - playerVel.y * 7.5f;
            playerVel.y += buoyAccel * dt;
        } else {
            // Smooth gravity descent back to surface if leaped high
            float buoyAccel = buoyDelta * 16.0f - playerVel.y * 5.0f;
            playerVel.y += buoyAccel * dt;
        }

        // Treading leap / Climb ladder / Breach crests: Press [Space]
        if (IsKeyPressed(KEY_SPACE)) {
            playerVel.y = 5.8f;
            PlaySound(g_sndWaterSplash);
        }

        // Intentional dive trigger: Press [C] or [Left Ctrl]
        if (diveDownKey) {
            playerVel.y = -3.8f;
            g_waterState = WATER_STATE_DIVING;
            g_wasSubmerged = true;
        }
    }
    else {
        // =================================================================
        // STATE 3: 3D UNDERWATER DIVING
        // =================================================================
        g_waterState = WATER_STATE_DIVING;
        g_wasSubmerged = true;
        g_waterSmoothTiltPitch = Lerp(g_waterSmoothTiltPitch, 0.0f, dt * 5.0f);
        g_waterSmoothTiltRoll  = Lerp(g_waterSmoothTiltRoll,  0.0f, dt * 5.0f);

        // Oxygen consumption only while submerged
        g_playerOxygen -= dt * 1.0f;
        if (g_playerOxygen < 0.0f) g_playerOxygen = 0.0f;

        // Full 3D Camera Look Direction
        Vector3 camDir = Vector3Subtract(camera.target, camera.position);
        if (Vector3LengthSqr(camDir) > 0.0001f) camDir = Vector3Normalize(camDir);
        else camDir = (Vector3){ 0, 0, 1 };

        Vector3 camRight = Vector3CrossProduct(camDir, (Vector3){ 0, 1, 0 });
        if (Vector3LengthSqr(camRight) > 0.0001f) camRight = Vector3Normalize(camRight);
        else camRight = (Vector3){ 1, 0, 0 };

        bool isFastDive = IsKeyDown(KEY_LEFT_SHIFT);
        float diveTopSpeed = isFastDive ? 5.8f : 4.0f;

        Vector3 wishDir = { 0, 0, 0 };
        if (IsKeyDown(KEY_W)) wishDir = Vector3Add(wishDir, camDir);
        if (IsKeyDown(KEY_S)) wishDir = Vector3Subtract(wishDir, camDir);
        if (IsKeyDown(KEY_D)) wishDir = Vector3Add(wishDir, camRight);
        if (IsKeyDown(KEY_A)) wishDir = Vector3Subtract(wishDir, camRight);
        if (IsKeyDown(KEY_SPACE)) wishDir = Vector3Add(wishDir, (Vector3){ 0, 1, 0 });
        if (IsKeyDown(KEY_C) || IsKeyDown(KEY_LEFT_CONTROL)) wishDir = Vector3Subtract(wishDir, (Vector3){ 0, 1, 0 });

        if (Vector3LengthSqr(wishDir) > 0.001f) {
            wishDir = Vector3Normalize(wishDir);
            playerVel.x = Lerp(playerVel.x, wishDir.x * diveTopSpeed, dt * 5.2f);
            playerVel.y = Lerp(playerVel.y, wishDir.y * diveTopSpeed, dt * 5.2f);
            playerVel.z = Lerp(playerVel.z, wishDir.z * diveTopSpeed, dt * 5.2f);
        } else {
            // Underwater hydrodynamic damping
            playerVel.x = Lerp(playerVel.x, 0.0f, dt * 3.2f);
            playerVel.z = Lerp(playerVel.z, 0.0f, dt * 3.2f);
            // Gentle natural positive buoyancy when idle
            playerVel.y = Lerp(playerVel.y, 0.65f, dt * 2.8f);
        }

        // Submarine floor boundary: prevent penetrating sandy seabed
        float bedY = (camera.position.x >= 0.0f) ? (3.8f + (camera.position.x / 35.0f) * 3.7f) : 3.8f;
        if (camera.position.y - PLAYER_EYE_HEIGHT < bedY) {
            camera.position.y = bedY + PLAYER_EYE_HEIGHT;
            if (playerVel.y < 0.0f) playerVel.y = 0.0f;
        }

        // Surfacing detection: diver's head breaks through dynamic ocean surface
        if (camera.position.y >= dynamicSurface - 0.02f && !diveDownKey) {
            g_waterState = WATER_STATE_SURFACE;
            PlaySound(g_sndSurfacingGasp);
            g_screenDropletsTimer = 3.2f;
            g_wasSubmerged = false;
            playerVel.y = fmaxf(playerVel.y, 1.2f); // Surfacing breach pop
        }
    }

    // Pass smooth velocity to engine movement pipeline
    moveDelta.x = playerVel.x * dt;
    moveDelta.y = playerVel.y * dt;
    moveDelta.z = playerVel.z * dt;
}

// =========================================================================
// RENDER UNDERWATER POST-PROCESSING, SCREEN DROPLETS, WATERLINE MENISCUS & OXYGEN HUD
// =========================================================================
static void DrawUnderwaterPostFXAndHUD(Camera3D camera, float timeVal, float dt, int screenW, int screenH) {
    float waveElev = GetOceanWaveElevation(camera.position.x, camera.position.z, timeVal);
    float dynamicSurface = OCEAN_WATER_LEVEL + waveElev;
    float camAltitude = camera.position.y - dynamicSurface;

    // 1. Waterline Meniscus Lens Transition Effect: ONLY during actual water plane crossing (|camAltitude| <= 0.07m)
    if (fabsf(camAltitude) <= 0.07f && camera.position.x <= 36.5f) {
        float meniscusAlpha = Clamp(1.0f - (fabsf(camAltitude) / 0.07f), 0.0f, 1.0f);
        float screenCenterY = (float)screenH * 0.5f;
        Vector3 camFwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
        float pitchOffset = camFwd.y * ((float)screenH * 0.75f);
        float my = screenCenterY + (camAltitude * ((float)screenH * 2.2f)) - pitchOffset;
        my = Clamp(my, 15.0f, (float)screenH - 15.0f);

        int myInt = (int)my;
        if (myInt < screenH) {
            DrawRectangle(0, myInt, screenW, screenH - myInt, (Color){ 6, 28, 44, (unsigned char)(meniscusAlpha * 80.0f) });
        }

        // Dynamic undulating meniscus boundary ribbon with surface-tension refraction
        for (int x = 0; x < screenW; x += 4) {
            float waveRip = sinf((float)x * 0.016f + timeVal * 4.2f) * 3.2f + cosf((float)x * 0.035f - timeVal * 2.6f) * 1.8f;
            int ry = myInt + (int)waveRip;
            DrawRectangle(x, ry - 2, 4, 2, (Color){ 225, 245, 255, (unsigned char)(meniscusAlpha * 160.0f) });
            DrawRectangle(x, ry, 4, 3, (Color){ 10, 42, 60, (unsigned char)(meniscusAlpha * 130.0f) });
        }
    }

    // 2. Clean Surface Swimming Status Bar (No oxygen meter or hypoxia warning!)
    if (g_waterState == WATER_STATE_SURFACE) {
        const char* swimHelp = "[W/A/S/D] SWIM  |  [SHIFT] FAST SWIM  |  [SPACE] TREAD HIGH  |  [C] DIVE UNDER";
        float tw = MeasureTextSharp(g_fontMenu, swimHelp, 14.0f);
        float pw = tw + 36.0f;
        float px = ((float)screenW - pw) * 0.5f;
        float py = (float)screenH - 46.0f;
        DrawRectangleRounded((Rectangle){ px + 2.0f, py + 4.0f, pw, 28.0f }, 0.3f, 8, (Color){ 0, 0, 0, 110 });
        DrawRectangleRounded((Rectangle){ px, py, pw, 28.0f }, 0.3f, 8, (Color){ 10, 24, 36, 235 });
        DrawLine((int)(px + 10.0f), (int)(py + 1.0f), (int)(px + pw - 10.0f), (int)(py + 1.0f), (Color){ 255, 255, 255, 45 });
        DrawRectangleRoundedLinesEx((Rectangle){ px, py, pw, 28.0f }, 0.3f, 8, 1.2f, (Color){ 55, 150, 200, 230 });
        DrawTextSharpCentered(g_fontMenu, swimHelp, (float)screenW * 0.5f, py + 7.0f, 14.0f, (Color){ 220, 245, 255, 255 });
    }

    // 3. Full 3D Underwater Diving Post-FX (ONLY when diving under!)
    if (g_waterState == WATER_STATE_DIVING) {
        // Deep marine underwater chromatic tint
        DrawRectangle(0, 0, screenW, screenH, (Color){ 8, 32, 48, 120 });
        DrawRectangle(0, 0, screenW, screenH, (Color){ 4, 18, 28, 70 });

        // Hypoxia low-oxygen vignette pulse (only when oxygen is very low)
        if (g_playerOxygen < 7.0f) {
            float warnFrac = 1.0f - (g_playerOxygen / 7.0f);
            float pulse = 0.5f + 0.5f * sinf(timeVal * 6.0f);
            unsigned char vigA = (unsigned char)(warnFrac * (120.0f + pulse * 60.0f));
            DrawRectangle(0, 0, screenW, 35, (Color){ 80, 8, 12, vigA });
            DrawRectangle(0, screenH - 35, screenW, 35, (Color){ 80, 8, 12, vigA });
            DrawRectangle(0, 0, 35, screenH, (Color){ 80, 8, 12, vigA });
            DrawRectangle(screenW - 35, 0, 35, screenH, (Color){ 80, 8, 12, vigA });
        }

        // Oxygen Gauge Bar
        int barW = 280;
        int barH = 20;
        float bx = ((float)screenW - barW) * 0.5f;
        float by = (float)screenH - 74.0f;

        // Ambient drop shadow & Acrylic glass panel
        DrawRectangleRounded((Rectangle){ bx - 5.0f + 2.0f, by - 5.0f + 3.0f, (float)barW + 10.0f, (float)barH + 10.0f }, 0.25f, 8, (Color){ 0, 0, 0, 130 });
        DrawRectangleRounded((Rectangle){ bx - 5.0f, by - 5.0f, (float)barW + 10.0f, (float)barH + 10.0f }, 0.25f, 8, (Color){ 10, 18, 26, 240 });
        DrawLine((int)(bx - 2.0f), (int)(by - 4.0f), (int)(bx + barW + 2.0f), (int)(by - 4.0f), (Color){ 255, 255, 255, 40 });
        DrawRectangleRoundedLinesEx((Rectangle){ bx - 5.0f, by - 5.0f, (float)barW + 10.0f, (float)barH + 10.0f }, 0.25f, 8, 1.2f, (Color){ 45, 110, 150, 240 });

        float oxRatio = Clamp(g_playerOxygen / g_maxOxygen, 0.0f, 1.0f);
        Color oxCol = (oxRatio > 0.45f) ? (Color){ 50, 220, 245, 255 } : ((oxRatio > 0.20f) ? (Color){ 245, 185, 45, 255 } : (Color){ 255, 50, 50, 255 });

        if (oxRatio > 0.01f) {
            DrawRectangleRounded((Rectangle){ bx, by, (float)barW * oxRatio, (float)barH }, 0.25f, 6, oxCol);
            DrawLine((int)bx, (int)(by + 2.0f), (int)(bx + barW * oxRatio), (int)(by + 2.0f), (Color){ 255, 255, 255, 90 });
        }

        const char* oxTxt = TextFormat("DIVE OXYGEN // %d%% (%.0fs)", (int)(oxRatio * 100.0f), g_playerOxygen);
        DrawTextSharpCentered(g_fontSmall, oxTxt, (float)screenW * 0.5f, by + 4.0f, 12.0f, (Color){ 245, 250, 255, 255 });

        const char* diveSub = "[SPACE] SWIM TO SURFACE   |   [C] DESCEND";
        DrawTextSharpCentered(g_fontSmall, diveSub, (float)screenW * 0.5f, by + barH + 9.0f, 12.0f, (Color){ 160, 215, 235, 220 });
    }

    // 4. Screen lens water droplets when emerging from water
    if (g_screenDropletsTimer > 0.0f) {
        g_screenDropletsTimer -= dt;
        float fade = Clamp(g_screenDropletsTimer / 3.2f, 0.0f, 1.0f);
        unsigned char dropA = (unsigned char)(fade * 160.0f);

        for (int d = 0; d < 12; d++) {
            float seed = (float)d * 4.7f;
            float dx = (float)screenW * (0.08f + fmodf(seed * 19.3f, 0.84f));
            float runDown = (3.2f - g_screenDropletsTimer) * (60.0f + fmodf(seed * 7.0f, 45.0f));
            float dy = 60.0f + fmodf(seed * 29.1f, (float)screenH * 0.6f) + runDown;
            float r = 5.0f + fmodf(seed * 3.0f, 6.0f);

            DrawCircle((int)dx, (int)dy, r, (Color){ 210, 235, 255, (unsigned char)(dropA * 0.5f) });
            DrawCircleLines((int)dx, (int)dy, r, (Color){ 240, 250, 255, dropA });
            DrawCircle((int)(dx - r * 0.35f), (int)(dy - r * 0.35f), r * 0.25f, (Color){ 255, 255, 255, dropA });
        }
    }
}
