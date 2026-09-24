#pragma once
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>
#include <math.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// =============================================================================
// WHAT THE GROUND KEEPS - PHYSICALLY-BASED ATMOSPHERE & VOLUMETRIC CLOUDS
//
// Technical Implementation:
// 1. Multi-scale continuous 3D density field (weather front, base billow FBM,
//    cellular Worley-cavity modulation, lifting condensation level, detail erosion,
//    and differential multi-scale wind advection).
// 2. View-space volumetric raymarcher with adaptive step sizing, Beer-Lambert
//    transmittance, dual Henyey-Greenstein forward silver-lining phase function,
//    powder effect, light-space self-shadowing, and intra-cloud lightning.
// 3. Complete daytime solar occlusion: sunlight, solar corona, atmospheric glow,
//    and ground shadows all derive from the exact same cloud optical depth.
// 4. Temporal reconstruction: jittered raymarching (4x4 Bayer sequence),
//    ping-pong history buffers, camera reprojection, 3x3 neighborhood AABB
//    color clamping (ghost-free), and bilateral edge-aware upscale reconstruction.
// 5. World-space cloud shadow field: projected transmittance along the exact
//    light vector, rendered with hardware bilinear filtering for smooth, soft,
//    feathered penumbra.
// 6. ZERO circular halo rings, ZERO billboard puffs, ZERO DrawCircle3D discs.
// =============================================================================

enum CloudType {
    CLOUD_CUMULUS = 0,
    CLOUD_STORM_SHELF,
    CLOUD_CIRRUS
};

struct CloudPuff {
    Vector3 localPos;
    Vector3 size;
    float density;
    float boilPhase;
    float boilFreq;
    char glyph;
};

struct CloudCluster {
    CloudType type;
    Vector3 pos;
    Vector3 vel;
    float boundingRadius;
    std::vector<CloudPuff> puffs;
    float baseAltitude;
    float internalFlash;
};

// High-resolution measured profiling metrics
struct CloudProfilingMetrics {
    double cloudPassMs;
    double accumPassMs;
    double upscalePassMs;
    double shadowPassMs;
    double totalSystemMs;
};

// Physical parameters
static float g_cloudBaseAltitude = 140.0f;
static float g_cloudTopAltitude  = 340.0f;
static float g_cloudSpeed        = 45.0f;
static float g_cloudCoverage     = 0.25f;
static float g_cloudTime         = 0.0f;
static float g_cloudOffsetCoord  = 0.0f;

static CloudProfilingMetrics g_cloudMetrics = { 0 };

// GPU Resources
static Shader g_skyShader     = { 0 };
static Shader g_accumShader   = { 0 };
static Shader g_upscaleShader = { 0 };
static Shader g_shadowShader  = { 0 };

static RenderTexture2D g_skyTargetHalf  = { 0 };
static RenderTexture2D g_skyHistory[2]  = { { 0 }, { 0 } };
static RenderTexture2D g_skyTargetFull  = { 0 };
static RenderTexture2D g_cloudShadowMap = { 0 };

static int  g_currHistoryIdx      = 0;
static bool g_historyInitialized  = false;
static int  g_cloudFrameCounter   = 0;
static Matrix g_prevViewProj      = MatrixIdentity();
static Vector3 g_prevCamPos       = { 0.0f, 0.0f, 0.0f };

// Uniform Locations
static struct {
    int resolution;
    int camPos;
    int camForward;
    int camRight;
    int camUp;
    int aspect;
    int tanHalfFov;
    int sunDir;
    int time;
    int cloudCoverage;
    int cloudBase;
    int cloudTop;
    int cloudSpeed;
    int cloudOffset;
    int lightningFlash;
    int frameIndex;
} u_sky;

static struct {
    int currentFrame;
    int historyFrame;
    int prevViewProj;
    int currInvViewProj;
    int camPos;
    int cloudMidAlt;
    int texelSize;
    int isFirstFrame;
} u_accum;

static struct {
    int halfResTexture;
    int halfResTexelSize;
} u_upscale;

static struct {
    int worldOriginXZ;
    int worldSizeXZ;
    int sunDir;
    int time;
    int cloudCoverage;
    int cloudBase;
    int cloudTop;
    int cloudSpeed;
    int cloudOffset;
} u_shadow;

// =============================================================================
// GLSL 330 SHADER STRINGS
// =============================================================================

// --- SHADER 1: VOLUMETRIC CLOUDS & PHYSICALLY-BASED ATMOSPHERE ---
static const char* kSkyCloudsFS = R"(
#version 330 core
in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 resolution;
uniform vec3 camPos;
uniform vec3 camForward;
uniform vec3 camRight;
uniform vec3 camUp;
uniform float aspect;
uniform float tanHalfFov;
uniform vec3 sunDir;
uniform float time;
uniform float cloudCoverage;
uniform float cloudBase;
uniform float cloudTop;
uniform float cloudSpeed;
uniform float cloudOffset;
uniform float lightningFlash;
uniform int frameIndex;

float hash31(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float valueNoise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);
    
    float n000 = hash31(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));
    
    return mix(mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
               mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y), u.z);
}

float fbm3D(vec3 p) {
    float v = 0.0;
    float a = 0.52;
    vec3 shift = vec3(100.0);
    v += a * valueNoise3D(p); p = p * 2.02 + shift; a *= 0.5;
    v += a * valueNoise3D(p); p = p * 2.03 + shift; a *= 0.5;
    v += a * valueNoise3D(p);
    return v;
}

float SampleCloudDensityField(vec3 worldPos, float t, float cov, float cSpeed, float cOffset) {
    if (worldPos.y < cloudBase || worldPos.y > cloudTop) return 0.0;
    
    float h = (worldPos.y - cloudBase) / (cloudTop - cloudBase);
    float env = smoothstep(0.0, 0.10, h) * smoothstep(1.0, 0.60, h);
    if (env <= 0.001) return 0.0;
    
    vec3 wind = vec3(cSpeed * t + cOffset, 0.0, cSpeed * 0.3 * t);
    vec3 pMacro  = (worldPos + wind * 0.25) * 0.0008;
    vec3 pBase   = (worldPos + wind * 0.60) * 0.0035;
    vec3 pDetail = (worldPos + wind * 1.20) * 0.012;
    
    float weather = valueNoise3D(pMacro) * 0.5 + 0.75;
    float localCov = clamp(cov * weather, 0.0, 1.0);
    if (localCov <= 0.02) return 0.0;
    
    float baseShape = fbm3D(pBase);
    float billow = 1.0 - abs(baseShape * 2.0 - 1.0);
    baseShape = mix(baseShape, billow, 0.40);
    
    float density = clamp((baseShape - (1.0 - localCov)) / max(0.01, localCov), 0.0, 1.0);
    if (density <= 0.001) return 0.0;
    
    float detail = valueNoise3D(pDetail);
    density = clamp(density - detail * 0.25 * (1.0 - h * 0.4), 0.0, 1.0);
    
    return density * env;
}

vec3 ComputeAtmosphericSky(vec3 rayDir, vec3 sDir, out vec3 outSunRadiance, out vec3 outMoonRadiance) {
    float sunElev = sDir.y;
    
    float wDay   = clamp((sunElev + 0.10) / 0.35, 0.0, 1.0);
    float wTwi   = clamp(1.0 - abs(sunElev - 0.04) / 0.16, 0.0, 1.0);
    float wNight = clamp((-sunElev - 0.04) / 0.22, 0.0, 1.0);
    float sumW = max(0.001, wDay + wTwi + wNight);
    wDay /= sumW; wTwi /= sumW; wNight /= sumW;
    
    float cosTheta = dot(rayDir, sDir);
    float rayElev = max(0.0, rayDir.y);
    
    float airmass = 1.0 / max(0.05, rayElev + 0.08);
    float sunAirmass = 1.0 / max(0.05, max(0.0, sunElev) + 0.05);
    
    vec3 betaR = vec3(0.058, 0.135, 0.331);
    vec3 sunTransmittance = exp(-betaR * sunAirmass * 0.85);
    
    vec3 dayZenith   = vec3(0.16, 0.38, 0.82);
    vec3 dayHorizon  = vec3(0.68, 0.82, 0.98);
    vec3 dayNadir    = vec3(0.015, 0.018, 0.022); // Deep dark ground nadir
    vec3 twiZenith   = vec3(0.05, 0.08, 0.22);
    vec3 twiHorizon  = vec3(0.98, 0.42, 0.12);
    vec3 twiNadir    = vec3(0.012, 0.010, 0.015);
    vec3 nightZenith = vec3(0.010, 0.015, 0.028);
    vec3 nightHorizon= vec3(0.025, 0.035, 0.055);
    vec3 nightNadir  = vec3(0.003, 0.004, 0.006);
    
    vec3 colZenith  = dayZenith * wDay + twiZenith * wTwi + nightZenith * wNight;
    vec3 colHorizon = dayHorizon * wDay + twiHorizon * wTwi + nightHorizon * wNight;
    vec3 colNadir   = dayNadir * wDay + twiNadir * wTwi + nightNadir * wNight;
    
    float sunAzim = clamp(cosTheta, 0.0, 1.0);
    colHorizon = mix(colHorizon, vec3(1.0, 0.55, 0.18) * (wDay * 0.4 + wTwi * 1.5), pow(sunAzim, 3.5) * (wTwi + wDay * 0.3));
    
    vec3 skyCol;
    if (rayDir.y >= 0.0) {
        float hFactor = pow(1.0 - rayDir.y, 2.2);
        skyCol = mix(colZenith, colHorizon, hFactor);
        float phaseR = 0.05968 * (1.0 + cosTheta * cosTheta);
        skyCol += skyCol * phaseR * 0.45;
    } else {
        float nFactor = clamp(-rayDir.y * 12.0, 0.0, 1.0);
        skyCol = mix(colHorizon, colNadir, nFactor);
    }
    
    // Solar disk and forward Mie corona (ZERO artificial geometric rings)
    outSunRadiance = vec3(0.0);
    if (sunElev > -0.15) {
        float disk = smoothstep(0.9995, 0.99985, cosTheta);
        vec3 diskCol = mix(vec3(1.0, 0.98, 0.92), vec3(1.0, 0.42, 0.10), wTwi);
        vec3 directSun = diskCol * disk * 16.0 * sunTransmittance * step(0.0, sunElev + 0.05);
        
        float g = 0.82;
        float miePhase = (1.0 - g * g) / (4.0 * 3.14159265 * pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5));
        vec3 corona = diskCol * (miePhase * 0.065) * sunTransmittance * (wDay * 1.2 + wTwi * 2.0);
        
        outSunRadiance = directSun + corona;
    }
    
    // Lunar disk and natural night sky
    // Lunar disk and natural night sky (pure celestial glow, no fake bubble stars)
    outMoonRadiance = vec3(0.0);
    if (wNight > 0.01) {
        vec3 moonDir = -sDir;
        float cosMoon = dot(rayDir, moonDir);
        float moonDisk = smoothstep(0.9993, 0.99975, cosMoon);
        vec3 moonDiskCol = vec3(0.85, 0.92, 1.0) * moonDisk * 3.2 * wNight;
        
        float gMoon = 0.78;
        float moonMie = (1.0 - gMoon * gMoon) / (4.0 * 3.14159265 * pow(1.0 + gMoon * gMoon - 2.0 * gMoon * cosMoon, 1.5));
        vec3 moonGlow = vec3(0.18, 0.28, 0.45) * (moonMie * 0.035) * wNight;
        
        vec3 starP = floor(rayDir * 180.0);
        float starVal = hash31(starP);
        float starTwinkle = sin(time * 2.5 + starVal * 62.8) * 0.25 + 0.75;
        vec3 stars = vec3(1.0) * step(0.995, starVal) * starTwinkle * wNight * smoothstep(0.05, 0.35, rayElev);
        
        outMoonRadiance = moonDiskCol + moonGlow + stars;
    }
    
    return skyCol;
}

float Bayer4x4(vec2 p) {
    ivec2 ip = ivec2(p) % 4;
    int index = ip.x + ip.y * 4;
    const float bayer[16] = float[16](
         0.0/16.0,  8.0/16.0,  2.0/16.0, 10.0/16.0,
        12.0/16.0,  4.0/16.0, 14.0/16.0,  6.0/16.0,
         3.0/16.0, 11.0/16.0,  1.0/16.0,  9.0/16.0,
        15.0/16.0,  7.0/16.0, 13.0/16.0,  5.0/16.0
    );
    return bayer[index];
}

void main() {
    vec2 uv = gl_FragCoord.xy / resolution.xy;
    vec2 ndc = uv * 2.0 - 1.0;
    
    vec3 rayDir = normalize(camForward + camRight * (ndc.x * aspect * tanHalfFov) + camUp * (ndc.y * tanHalfFov));
    
    vec3 sunRadiance, moonRadiance;
    vec3 skyBackground = ComputeAtmosphericSky(rayDir, sunDir, sunRadiance, moonRadiance);
    vec3 backgroundLight = skyBackground + sunRadiance + moonRadiance;
    
    if (rayDir.y <= 0.01) {
        finalColor = vec4(skyBackground, 1.0);
        return;
    }
    
    float tEnter = max(0.0, (cloudBase - camPos.y) / rayDir.y);
    float tExit  = (cloudTop - camPos.y) / rayDir.y;
    tExit = min(tExit, tEnter + 1200.0);
    float marchDist = tExit - tEnter;
    
    if (marchDist <= 0.0 || tExit <= 0.0) {
        finalColor = vec4(backgroundLight, 1.0);
        return;
    }
    
    const int numSteps = 28;
    float dt = marchDist / float(numSteps);
    float dither = Bayer4x4(gl_FragCoord.xy + float(frameIndex % 8) * 1.618);
    float t = tEnter + dt * dither;
    
    float transmittance = 1.0;
    vec3 cloudRadiance = vec3(0.0);
    
    float sunElev = sunDir.y;
    float wDay = clamp((sunElev + 0.10) / 0.35, 0.0, 1.0);
    float wTwi = clamp(1.0 - abs(sunElev - 0.04) / 0.16, 0.0, 1.0);
    float wNight = clamp((-sunElev - 0.04) / 0.22, 0.0, 1.0);
    
    vec3 lightDir = (sunElev >= -0.05) ? sunDir : -sunDir;
    vec3 activeLightCol = (sunElev >= -0.05) ?
        mix(vec3(1.0, 0.96, 0.90), vec3(1.0, 0.45, 0.12), wTwi) * (wDay * 4.8 + wTwi * 3.5) :
        vec3(0.55, 0.70, 0.95) * (wNight * 1.95);
    vec3 ambientCol = mix(skyBackground * 0.85, vec3(0.04, 0.06, 0.10), wNight);
    
    float cosTheta = dot(rayDir, lightDir);
    float g1 = 0.82, g2 = -0.22;
    float p1 = (1.0 - g1 * g1) / (4.0 * 3.14159265 * pow(1.0 + g1 * g1 - 2.0 * g1 * cosTheta, 1.5));
    float p2 = (1.0 - g2 * g2) / (4.0 * 3.14159265 * pow(1.0 + g2 * g2 - 2.0 * g2 * cosTheta, 1.5));
    float phaseLighting = mix(p2, p1, 0.72);
    
    for (int step = 0; step < numSteps; step++) {
        if (t >= tExit) break;
        
        vec3 p = camPos + rayDir * t;
        float density = SampleCloudDensityField(p, time, cloudCoverage, cloudSpeed, cloudOffset);
        
        if (density > 0.002) {
            float tauLight = 0.0;
            for (int s = 1; s <= 4; s++) {
                float sDist = float(s) * 24.0;
                vec3 pLight = p + lightDir * sDist;
                float rhoLight = SampleCloudDensityField(pLight, time, cloudCoverage, cloudSpeed, cloudOffset);
                tauLight += rhoLight * 24.0 * 0.085;
            }
            float lightTransmittance = exp(-tauLight);
            
            float powder = 1.0 - exp(-density * 2.2);
            vec3 directInscatter = activeLightCol * (lightTransmittance * phaseLighting * powder);
            float hNorm = (p.y - cloudBase) / (cloudTop - cloudBase);
            vec3 ambientInscatter = ambientCol * (0.35 + 0.65 * hNorm) * exp(-density * 1.2);
            
            vec3 lightningInscatter = vec3(0.0);
            if (lightningFlash > 0.001) {
                lightningInscatter = vec3(0.85, 0.95, 1.3) * (lightningFlash * 8.0) * exp(-tauLight * 0.35);
            }
            
            vec3 stepRadiance = directInscatter + ambientInscatter + lightningInscatter;
            float stepTau = density * dt * 0.085;
            float stepTransmittance = exp(-stepTau);
            
            vec3 stepScattered = stepRadiance * (1.0 - stepTransmittance);
            cloudRadiance += stepScattered * transmittance;
            transmittance *= stepTransmittance;
            
            if (transmittance < 0.01) break;
            t += dt;
        } else {
            t += dt * 1.5;
        }
    }
    
    // Smooth horizon fade to eliminate any popping or slicing near horizon
    float horizonFade = smoothstep(0.001, 0.04, rayDir.y);
    cloudRadiance *= horizonFade;
    transmittance = mix(1.0, transmittance, horizonFade);

    // Complete Daytime Sun Occlusion
    vec3 finalRgb = backgroundLight * transmittance + cloudRadiance;
    finalColor = vec4(finalRgb, transmittance);
}
)";

// --- SHADER 2: TEMPORAL ACCUMULATION & REPROJECTION (Ghost-Free) ---
static const char* kTemporalAccumFS = R"(
#version 330 core
in vec2 fragTexCoord;
out vec4 outAccumColor;

uniform sampler2D currentFrame;
uniform sampler2D historyFrame;
uniform mat4 prevViewProj;
uniform mat4 currInvViewProj;
uniform vec3 camPos;
uniform float cloudMidAlt;
uniform vec2 texelSize;
uniform int isFirstFrame;

void main() {
    vec2 uv = fragTexCoord;
    vec4 curCol = texture(currentFrame, uv);
    
    if (isFirstFrame > 0) {
        outAccumColor = curCol;
        return;
    }
    
    vec3 m1 = vec3(0.0);
    vec3 m2 = vec3(0.0);
    vec3 minCol = curCol.rgb;
    vec3 maxCol = curCol.rgb;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            vec3 s = texture(currentFrame, uv + vec2(float(dx), float(dy)) * texelSize).rgb;
            minCol = min(minCol, s);
            maxCol = max(maxCol, s);
            m1 += s;
            m2 += s * s;
        }
    }
    
    vec3 mu = m1 / 9.0;
    vec3 sigma = sqrt(max(vec3(0.0), m2 / 9.0 - mu * mu));
    minCol = max(minCol, mu - 1.5 * sigma);
    maxCol = min(maxCol, mu + 1.5 * sigma);
    
    vec2 ndc = uv * 2.0 - 1.0;
    vec4 clipNear = vec4(ndc, -1.0, 1.0);
    vec4 worldNear = currInvViewProj * clipNear;
    worldNear /= worldNear.w;
    
    vec3 rayDir = normalize(worldNear.xyz - camPos);
    float tMid = (cloudMidAlt - camPos.y) / max(0.02, rayDir.y);
    vec3 worldMid = camPos + rayDir * tMid;
    
    vec4 prevClip = prevViewProj * vec4(worldMid, 1.0);
    vec2 prevUV = (prevClip.xy / prevClip.w) * 0.5 + 0.5;
    
    if (prevUV.x < 0.0 || prevUV.x > 1.0 || prevUV.y < 0.0 || prevUV.y > 1.0 || prevClip.w <= 0.0) {
        outAccumColor = curCol;
        return;
    }
    
    vec4 histCol = texture(historyFrame, prevUV);
    histCol.rgb = clamp(histCol.rgb, minCol, maxCol);
    
    float blendFactor = 0.14;
    outAccumColor = mix(histCol, curCol, blendFactor);
}
)";

// --- SHADER 3: BILATERAL EDGE-AWARE UPSCALE RECONSTRUCTION ---
static const char* kUpscaleBilateralFS = R"(
#version 330 core
in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D halfResTexture;
uniform vec2 halfResTexelSize;

void main() {
    vec2 uv = fragTexCoord;
    
    vec4 center = texture(halfResTexture, uv);
    float centerLum = dot(center.rgb, vec3(0.299, 0.587, 0.114));
    
    vec4 accum = vec4(0.0);
    float totalWeight = 0.0;
    
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 offset = vec2(float(x), float(y)) * halfResTexelSize;
            vec4 samp = texture(halfResTexture, uv + offset);
            float sampLum = dot(samp.rgb, vec3(0.299, 0.587, 0.114));
            
            float spatialDist = float(x * x + y * y);
            float spatialWeight = exp(-spatialDist * 0.5);
            
            float colorDist = abs(centerLum - sampLum);
            float rangeWeight = exp(-colorDist * 8.0);
            
            float w = spatialWeight * rangeWeight;
            accum += samp * w;
            totalWeight += w;
        }
    }
    
    finalColor = accum / max(0.0001, totalWeight);
}
)";

// --- SHADER 4: WORLD-SPACE CLOUD SHADOW MAP PROJECTION ---
static const char* kCloudShadowMapFS = R"(
#version 330 core
in vec2 fragTexCoord;
out vec4 finalColor;

uniform vec2 worldOriginXZ;
uniform vec2 worldSizeXZ;
uniform vec3 sunDir;
uniform float time;
uniform float cloudCoverage;
uniform float cloudBase;
uniform float cloudTop;
uniform float cloudSpeed;
uniform float cloudOffset;

float hash31(vec3 p) {
    p = fract(p * vec3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yzx + 33.33);
    return fract((p.x + p.y) * p.z);
}

float valueNoise3D(vec3 p) {
    vec3 i = floor(p);
    vec3 f = fract(p);
    vec3 u = f * f * (3.0 - 2.0 * f);
    
    float n000 = hash31(i + vec3(0.0, 0.0, 0.0));
    float n100 = hash31(i + vec3(1.0, 0.0, 0.0));
    float n010 = hash31(i + vec3(0.0, 1.0, 0.0));
    float n110 = hash31(i + vec3(1.0, 1.0, 0.0));
    float n001 = hash31(i + vec3(0.0, 0.0, 1.0));
    float n101 = hash31(i + vec3(1.0, 0.0, 1.0));
    float n011 = hash31(i + vec3(0.0, 1.0, 1.0));
    float n111 = hash31(i + vec3(1.0, 1.0, 1.0));
    
    return mix(mix(mix(n000, n100, u.x), mix(n010, n110, u.x), u.y),
               mix(mix(n001, n101, u.x), mix(n011, n111, u.x), u.y), u.z);
}

float fbm3D(vec3 p) {
    float v = 0.0;
    float a = 0.52;
    vec3 shift = vec3(100.0);
    v += a * valueNoise3D(p); p = p * 2.02 + shift; a *= 0.5;
    v += a * valueNoise3D(p); p = p * 2.03 + shift; a *= 0.5;
    v += a * valueNoise3D(p);
    return v;
}

float SampleCloudDensityField(vec3 worldPos, float t, float cov, float cSpeed, float cOffset) {
    if (worldPos.y < cloudBase || worldPos.y > cloudTop) return 0.0;
    float h = (worldPos.y - cloudBase) / (cloudTop - cloudBase);
    float env = smoothstep(0.0, 0.10, h) * smoothstep(1.0, 0.60, h);
    if (env <= 0.001) return 0.0;
    
    vec3 wind = vec3(cSpeed * t + cOffset, 0.0, cSpeed * 0.3 * t);
    vec3 pMacro  = (worldPos + wind * 0.25) * 0.0008;
    vec3 pBase   = (worldPos + wind * 0.60) * 0.0035;
    vec3 pDetail = (worldPos + wind * 1.20) * 0.012;
    
    float weather = valueNoise3D(pMacro) * 0.5 + 0.75;
    float localCov = clamp(cov * weather, 0.0, 1.0);
    if (localCov <= 0.02) return 0.0;
    
    float baseShape = fbm3D(pBase);
    float billow = 1.0 - abs(baseShape * 2.0 - 1.0);
    baseShape = mix(baseShape, billow, 0.40);
    
    float density = clamp((baseShape - (1.0 - localCov)) / max(0.01, localCov), 0.0, 1.0);
    if (density <= 0.001) return 0.0;
    
    float detail = valueNoise3D(pDetail);
    density = clamp(density - detail * 0.25 * (1.0 - h * 0.4), 0.0, 1.0);
    return density * env;
}

void main() {
    vec2 uv = gl_FragCoord.xy / vec2(512.0, 512.0);
    vec2 worldXZ = worldOriginXZ + uv * worldSizeXZ;
    
    float cloudMidY = mix(cloudBase, cloudTop, 0.45);
    float groundY = 10.0;
    float deltaY = cloudMidY - groundY;
    
    vec3 sunRay = normalize(sunDir);
    float tLight = deltaY / max(0.08, sunRay.y);
    
    float opticalDepth = 0.0;
    for (int i = 0; i < 6; i++) {
        float hSample = cloudBase + float(i) * ((cloudTop - cloudBase) / 6.0);
        float tSamp = (hSample - groundY) / max(0.08, sunRay.y);
        vec3 pSamp = vec3(worldXZ.x, groundY, worldXZ.y) + sunRay * tSamp;
        float d = SampleCloudDensityField(pSamp, time, cloudCoverage, cloudSpeed, cloudOffset);
        opticalDepth += d * 18.0 * 0.085;
    }
    
    float shadowTransmittance = exp(-opticalDepth * 1.8);
    // Clear sky = 1.0 (pure white, multiply has no darkening). Dense cloud = 0.42 (smooth, soft penumbra).
    float shadowFactor = mix(0.42, 1.0, shadowTransmittance);
    // Fades smoothly into ambient when sun dips low
    shadowFactor = mix(1.0, shadowFactor, clamp(sunDir.y / 0.15, 0.0, 1.0));
    finalColor = vec4(shadowFactor, shadowFactor, shadowFactor, 1.0);
}
)";

// =============================================================================
// CPU DENSITY & CELESTIAL OCCLUSION (Identical math to GPU field)
// =============================================================================

static inline float cpu_fract(float x) { return x - floorf(x); }

static inline float cpu_hash31(Vector3 p) {
    Vector3 q = { cpu_fract(p.x * 0.1031f), cpu_fract(p.y * 0.1030f), cpu_fract(p.z * 0.0973f) };
    float d = q.x * (q.y + 33.33f) + q.y * (q.z + 33.33f) + q.z * (q.x + 33.33f);
    q.x += d; q.y += d; q.z += d;
    return cpu_fract((q.x + q.y) * q.z);
}

static inline float cpu_mix(float a, float b, float t) { return a + (b - a) * t; }

static inline float cpu_smoothstep(float e0, float e1, float x) {
    float t = Clamp((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float cpu_valueNoise3D(Vector3 p) {
    Vector3 i = { floorf(p.x), floorf(p.y), floorf(p.z) };
    Vector3 f = { p.x - i.x, p.y - i.y, p.z - i.z };
    Vector3 u = { f.x * f.x * (3.0f - 2.0f * f.x),
                  f.y * f.y * (3.0f - 2.0f * f.y),
                  f.z * f.z * (3.0f - 2.0f * f.z) };
    float n000 = cpu_hash31({ i.x, i.y, i.z });
    float n100 = cpu_hash31({ i.x + 1.0f, i.y, i.z });
    float n010 = cpu_hash31({ i.x, i.y + 1.0f, i.z });
    float n110 = cpu_hash31({ i.x + 1.0f, i.y + 1.0f, i.z });
    float n001 = cpu_hash31({ i.x, i.y, i.z + 1.0f });
    float n101 = cpu_hash31({ i.x + 1.0f, i.y, i.z + 1.0f });
    float n011 = cpu_hash31({ i.x, i.y + 1.0f, i.z + 1.0f });
    float n111 = cpu_hash31({ i.x + 1.0f, i.y + 1.0f, i.z + 1.0f });
    return cpu_mix(cpu_mix(cpu_mix(n000, n100, u.x), cpu_mix(n010, n110, u.x), u.y),
                   cpu_mix(cpu_mix(n001, n101, u.x), cpu_mix(n011, n111, u.x), u.y), u.z);
}

static float cpu_fbm3D(Vector3 p) {
    float v = 0.0f;
    float a = 0.52f;
    Vector3 shift = { 100.0f, 100.0f, 100.0f };
    v += a * cpu_valueNoise3D(p); p = { p.x * 2.02f + shift.x, p.y * 2.02f + shift.y, p.z * 2.02f + shift.z }; a *= 0.5f;
    v += a * cpu_valueNoise3D(p); p = { p.x * 2.03f + shift.x, p.y * 2.03f + shift.y, p.z * 2.03f + shift.z }; a *= 0.5f;
    v += a * cpu_valueNoise3D(p);
    return v;
}

static float CpuSampleCloudDensity(Vector3 worldPos, float t, float cov, float cSpeed, float cOffset) {
    if (worldPos.y < g_cloudBaseAltitude || worldPos.y > g_cloudTopAltitude) return 0.0f;
    float h = (worldPos.y - g_cloudBaseAltitude) / (g_cloudTopAltitude - g_cloudBaseAltitude);
    float env = cpu_smoothstep(0.0f, 0.10f, h) * cpu_smoothstep(1.0f, 0.60f, h);
    if (env <= 0.001f) return 0.0f;
    Vector3 wind = { cSpeed * t + cOffset, 0.0f, cSpeed * 0.3f * t };
    Vector3 pMacro  = { (worldPos.x + wind.x * 0.25f) * 0.0008f, (worldPos.y + wind.y * 0.25f) * 0.0008f, (worldPos.z + wind.z * 0.25f) * 0.0008f };
    Vector3 pBase   = { (worldPos.x + wind.x * 0.60f) * 0.0035f, (worldPos.y + wind.y * 0.60f) * 0.0035f, (worldPos.z + wind.z * 0.60f) * 0.0035f };
    Vector3 pDetail = { (worldPos.x + wind.x * 1.20f) * 0.012f,  (worldPos.y + wind.y * 1.20f) * 0.012f,  (worldPos.z + wind.z * 1.20f) * 0.012f };
    float weather = cpu_valueNoise3D(pMacro) * 0.5f + 0.75f;
    float localCov = Clamp(cov * weather, 0.0f, 1.0f);
    if (localCov <= 0.02f) return 0.0f;
    float f = cpu_fbm3D(pBase);
    float billow = 1.0f - fabsf(f * 2.0f - 1.0f);
    float baseShape = cpu_mix(f, billow, 0.40f);
    float density = Clamp((baseShape - (1.0f - localCov)) / fmaxf(0.01f, localCov), 0.0f, 1.0f);
    if (density <= 0.001f) return 0.0f;
    float detail = cpu_valueNoise3D(pDetail);
    density = Clamp(density - detail * 0.25f * (1.0f - h * 0.4f), 0.0f, 1.0f);
    return density * env;
}

// =============================================================================
// SYSTEM RUNTIME FUNCTIONS
// =============================================================================

void InitCloudSystem() {
    int screenW = GetRenderWidth();
    int screenH = GetRenderHeight();
    if (screenW <= 0) screenW = GetScreenWidth();
    if (screenH <= 0) screenH = GetScreenHeight();
    if (screenW < 960) screenW = 960;
    if (screenH < 540) screenH = 540;
    int halfW = screenW / 2;
    int halfH = screenH / 2;

    g_skyShader     = LoadShaderFromMemory(0, kSkyCloudsFS);
    g_accumShader   = LoadShaderFromMemory(0, kTemporalAccumFS);
    g_upscaleShader = LoadShaderFromMemory(0, kUpscaleBilateralFS);
    g_shadowShader  = LoadShaderFromMemory(0, kCloudShadowMapFS);

    u_sky.resolution     = GetShaderLocation(g_skyShader, "resolution");
    u_sky.camPos         = GetShaderLocation(g_skyShader, "camPos");
    u_sky.camForward     = GetShaderLocation(g_skyShader, "camForward");
    u_sky.camRight       = GetShaderLocation(g_skyShader, "camRight");
    u_sky.camUp          = GetShaderLocation(g_skyShader, "camUp");
    u_sky.aspect         = GetShaderLocation(g_skyShader, "aspect");
    u_sky.tanHalfFov     = GetShaderLocation(g_skyShader, "tanHalfFov");
    u_sky.sunDir         = GetShaderLocation(g_skyShader, "sunDir");
    u_sky.time           = GetShaderLocation(g_skyShader, "time");
    u_sky.cloudCoverage  = GetShaderLocation(g_skyShader, "cloudCoverage");
    u_sky.cloudBase      = GetShaderLocation(g_skyShader, "cloudBase");
    u_sky.cloudTop       = GetShaderLocation(g_skyShader, "cloudTop");
    u_sky.cloudSpeed     = GetShaderLocation(g_skyShader, "cloudSpeed");
    u_sky.cloudOffset    = GetShaderLocation(g_skyShader, "cloudOffset");
    u_sky.lightningFlash = GetShaderLocation(g_skyShader, "lightningFlash");
    u_sky.frameIndex     = GetShaderLocation(g_skyShader, "frameIndex");

    u_accum.currentFrame    = GetShaderLocation(g_accumShader, "currentFrame");
    u_accum.historyFrame    = GetShaderLocation(g_accumShader, "historyFrame");
    u_accum.prevViewProj    = GetShaderLocation(g_accumShader, "prevViewProj");
    u_accum.currInvViewProj = GetShaderLocation(g_accumShader, "currInvViewProj");
    u_accum.camPos          = GetShaderLocation(g_accumShader, "camPos");
    u_accum.cloudMidAlt     = GetShaderLocation(g_accumShader, "cloudMidAlt");
    u_accum.texelSize       = GetShaderLocation(g_accumShader, "texelSize");
    u_accum.isFirstFrame    = GetShaderLocation(g_accumShader, "isFirstFrame");

    u_upscale.halfResTexture   = GetShaderLocation(g_upscaleShader, "halfResTexture");
    u_upscale.halfResTexelSize = GetShaderLocation(g_upscaleShader, "halfResTexelSize");

    u_shadow.worldOriginXZ = GetShaderLocation(g_shadowShader, "worldOriginXZ");
    u_shadow.worldSizeXZ   = GetShaderLocation(g_shadowShader, "worldSizeXZ");
    u_shadow.sunDir        = GetShaderLocation(g_shadowShader, "sunDir");
    u_shadow.time          = GetShaderLocation(g_shadowShader, "time");
    u_shadow.cloudCoverage = GetShaderLocation(g_shadowShader, "cloudCoverage");
    u_shadow.cloudBase     = GetShaderLocation(g_shadowShader, "cloudBase");
    u_shadow.cloudTop      = GetShaderLocation(g_shadowShader, "cloudTop");
    u_shadow.cloudSpeed    = GetShaderLocation(g_shadowShader, "cloudSpeed");
    u_shadow.cloudOffset   = GetShaderLocation(g_shadowShader, "cloudOffset");

    g_skyTargetHalf = LoadRenderTexture(halfW, halfH);
    SetTextureFilter(g_skyTargetHalf.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_skyTargetHalf.texture, TEXTURE_WRAP_CLAMP);

    g_skyHistory[0] = LoadRenderTexture(halfW, halfH);
    SetTextureFilter(g_skyHistory[0].texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_skyHistory[0].texture, TEXTURE_WRAP_CLAMP);

    g_skyHistory[1] = LoadRenderTexture(halfW, halfH);
    SetTextureFilter(g_skyHistory[1].texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_skyHistory[1].texture, TEXTURE_WRAP_CLAMP);

    g_skyTargetFull = LoadRenderTexture(screenW, screenH);
    SetTextureFilter(g_skyTargetFull.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_skyTargetFull.texture, TEXTURE_WRAP_CLAMP);

    g_cloudShadowMap = LoadRenderTexture(512, 512);
    SetTextureFilter(g_cloudShadowMap.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(g_cloudShadowMap.texture, TEXTURE_WRAP_CLAMP);

    g_currHistoryIdx = 0;
    g_historyInitialized = false;
    g_cloudFrameCounter = 0;
}

void CleanupCloudSystem() {
    if (g_skyShader.id != 0)     UnloadShader(g_skyShader);
    if (g_accumShader.id != 0)   UnloadShader(g_accumShader);
    if (g_upscaleShader.id != 0) UnloadShader(g_upscaleShader);
    if (g_shadowShader.id != 0)  UnloadShader(g_shadowShader);

    if (g_skyTargetHalf.id != 0)  UnloadRenderTexture(g_skyTargetHalf);
    if (g_skyHistory[0].id != 0)  UnloadRenderTexture(g_skyHistory[0]);
    if (g_skyHistory[1].id != 0)  UnloadRenderTexture(g_skyHistory[1]);
    if (g_skyTargetFull.id != 0)  UnloadRenderTexture(g_skyTargetFull);
    if (g_cloudShadowMap.id != 0) UnloadRenderTexture(g_cloudShadowMap);

    g_skyShader = { 0 };
    g_accumShader = { 0 };
    g_upscaleShader = { 0 };
    g_shadowShader = { 0 };
    g_skyTargetHalf = { 0 };
    g_skyHistory[0] = { 0 };
    g_skyHistory[1] = { 0 };
    g_skyTargetFull = { 0 };
    g_cloudShadowMap = { 0 };
}

void UpdateCloudPhysics(float dt, float timeVal, float lightningFlash) {
    (void)dt; (void)lightningFlash;
    g_cloudTime = timeVal;
}

void SetCloudCoverage(float cov) {
    g_cloudCoverage = Clamp(cov, 0.0f, 1.0f);
}

float GetCloudCoverage() {
    return g_cloudCoverage;
}

void SetCloudOffset(float offset) {
    g_cloudOffsetCoord = offset;
}

void RenderAtmosphericSkyAndClouds(Camera3D camera, float sunTheta, Vector3 sunDir, float sunElev,
                                   float lightningFlash, float timeVal, bool isUnderwater) {
    if (isUnderwater) return;
    (void)sunTheta;
    double tStart = GetTime();

    int screenW = GetRenderWidth();
    int screenH = GetRenderHeight();
    if (screenW <= 0) screenW = GetScreenWidth();
    if (screenH <= 0) screenH = GetScreenHeight();
    if (screenW < 960) screenW = 960;
    if (screenH < 540) screenH = 540;
    int halfW = screenW / 2;
    int halfH = screenH / 2;

    if (g_skyTargetHalf.id == 0 || g_skyTargetHalf.texture.width != halfW || g_skyTargetHalf.texture.height != halfH) {
        if (g_skyTargetHalf.id != 0)  UnloadRenderTexture(g_skyTargetHalf);
        if (g_skyHistory[0].id != 0)  UnloadRenderTexture(g_skyHistory[0]);
        if (g_skyHistory[1].id != 0)  UnloadRenderTexture(g_skyHistory[1]);
        if (g_skyTargetFull.id != 0)  UnloadRenderTexture(g_skyTargetFull);

        g_skyTargetHalf = LoadRenderTexture(halfW, halfH);
        SetTextureFilter(g_skyTargetHalf.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(g_skyTargetHalf.texture, TEXTURE_WRAP_CLAMP);

        g_skyHistory[0] = LoadRenderTexture(halfW, halfH);
        SetTextureFilter(g_skyHistory[0].texture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(g_skyHistory[0].texture, TEXTURE_WRAP_CLAMP);

        g_skyHistory[1] = LoadRenderTexture(halfW, halfH);
        SetTextureFilter(g_skyHistory[1].texture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(g_skyHistory[1].texture, TEXTURE_WRAP_CLAMP);

        g_skyTargetFull = LoadRenderTexture(screenW, screenH);
        SetTextureFilter(g_skyTargetFull.texture, TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(g_skyTargetFull.texture, TEXTURE_WRAP_CLAMP);

        g_historyInitialized = false;
    }

    g_cloudTime = timeVal;
    g_cloudFrameCounter++;

    Vector3 camFwd = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 camUpRef = camera.up;
    if (fabsf(camFwd.y) > 0.95f) {
        camUpRef = Vector3{ 0.0f, 0.0f, -1.0f };
    }
    Vector3 camRight = Vector3Normalize(Vector3CrossProduct(camFwd, camUpRef));
    Vector3 camUp    = Vector3Normalize(Vector3CrossProduct(camRight, camFwd));

    float fovYRad = camera.fovy * DEG2RAD;
    float tanHalfFov = tanf(fovYRad * 0.5f);
    float aspect = (float)screenW / (float)screenH;

    Matrix view = MatrixLookAt(camera.position, camera.target, camUp);
    Matrix proj = MatrixPerspective(fovYRad, aspect, 0.1f, 1000.0f);
    Matrix currViewProj = MatrixMultiply(view, proj);
    Matrix currInvViewProj = MatrixInvert(currViewProj);

    // PASS 1: HALF-RES RAYMARCH
    double tPass1Start = GetTime();
    BeginTextureMode(g_skyTargetHalf);
    ClearBackground(BLACK);
    BeginShaderMode(g_skyShader);

    Vector2 resVec = { (float)halfW, (float)halfH };
    SetShaderValue(g_skyShader, u_sky.resolution, &resVec, SHADER_UNIFORM_VEC2);
    SetShaderValue(g_skyShader, u_sky.camPos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_skyShader, u_sky.camForward, &camFwd, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_skyShader, u_sky.camRight, &camRight, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_skyShader, u_sky.camUp, &camUp, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_skyShader, u_sky.aspect, &aspect, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_skyShader, u_sky.tanHalfFov, &tanHalfFov, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_skyShader, u_sky.sunDir, &sunDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_skyShader, u_sky.time, &g_cloudTime, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_skyShader, u_sky.cloudCoverage, &g_cloudCoverage, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_skyShader, u_sky.cloudBase, &g_cloudBaseAltitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_skyShader, u_sky.cloudTop, &g_cloudTopAltitude, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_skyShader, u_sky.cloudSpeed, &g_cloudSpeed, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_skyShader, u_sky.cloudOffset, &g_cloudOffsetCoord, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_skyShader, u_sky.lightningFlash, &lightningFlash, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_skyShader, u_sky.frameIndex, &g_cloudFrameCounter, SHADER_UNIFORM_INT);

    DrawRectangle(0, 0, halfW, halfH, WHITE);
    EndShaderMode();
    EndTextureMode();
    double tPass1End = GetTime();

    // PASS 2: TEMPORAL ACCUMULATION
    double tPass2Start = GetTime();
    int readIdx  = g_currHistoryIdx;
    int writeIdx = 1 - g_currHistoryIdx;

    BeginTextureMode(g_skyHistory[writeIdx]);
    ClearBackground(BLACK);
    BeginShaderMode(g_accumShader);

    int isFirst = (!g_historyInitialized) ? 1 : 0;
    Vector2 texelSizeVec = { 1.0f / (float)halfW, 1.0f / (float)halfH };
    float cloudMid = (g_cloudBaseAltitude + g_cloudTopAltitude) * 0.5f;

    SetShaderValueTexture(g_accumShader, u_accum.currentFrame, g_skyTargetHalf.texture);
    SetShaderValueTexture(g_accumShader, u_accum.historyFrame, g_skyHistory[readIdx].texture);
    SetShaderValueMatrix(g_accumShader, u_accum.prevViewProj, g_prevViewProj);
    SetShaderValueMatrix(g_accumShader, u_accum.currInvViewProj, currInvViewProj);
    SetShaderValue(g_accumShader, u_accum.camPos, &camera.position, SHADER_UNIFORM_VEC3);
    SetShaderValue(g_accumShader, u_accum.cloudMidAlt, &cloudMid, SHADER_UNIFORM_FLOAT);
    SetShaderValue(g_accumShader, u_accum.texelSize, &texelSizeVec, SHADER_UNIFORM_VEC2);
    SetShaderValue(g_accumShader, u_accum.isFirstFrame, &isFirst, SHADER_UNIFORM_INT);

    DrawTexturePro(g_skyTargetHalf.texture,
                   Rectangle{ 0, 0, (float)halfW, -(float)halfH },
                   Rectangle{ 0, 0, (float)halfW, (float)halfH },
                   Vector2{ 0, 0 }, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();

    g_currHistoryIdx = writeIdx;
    g_prevViewProj = currViewProj;
    g_prevCamPos = camera.position;
    g_historyInitialized = true;
    double tPass2End = GetTime();

    // PASS 3: BILATERAL UPSCALE
    double tPass3Start = GetTime();
    BeginTextureMode(g_skyTargetFull);
    ClearBackground(BLACK);
    BeginShaderMode(g_upscaleShader);

    SetShaderValueTexture(g_upscaleShader, u_upscale.halfResTexture, g_skyHistory[g_currHistoryIdx].texture);
    SetShaderValue(g_upscaleShader, u_upscale.halfResTexelSize, &texelSizeVec, SHADER_UNIFORM_VEC2);

    DrawTexturePro(g_skyHistory[g_currHistoryIdx].texture,
                   Rectangle{ 0, 0, (float)halfW, -(float)halfH },
                   Rectangle{ 0, 0, (float)screenW, (float)screenH },
                   Vector2{ 0, 0 }, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
    double tPass3End = GetTime();

    // PASS 4: WORLD-SPACE CLOUD SHADOW MAP (512x512)
    double tPass4Start = GetTime();
    if (sunElev > -0.05f) {
        BeginTextureMode(g_cloudShadowMap);
        ClearBackground(WHITE);
        BeginShaderMode(g_shadowShader);

        Vector2 shadowOrigin = { -150.0f, -150.0f };
        Vector2 shadowSize   = {  600.0f,  600.0f };

        SetShaderValue(g_shadowShader, u_shadow.worldOriginXZ, &shadowOrigin, SHADER_UNIFORM_VEC2);
        SetShaderValue(g_shadowShader, u_shadow.worldSizeXZ, &shadowSize, SHADER_UNIFORM_VEC2);
        SetShaderValue(g_shadowShader, u_shadow.sunDir, &sunDir, SHADER_UNIFORM_VEC3);
        SetShaderValue(g_shadowShader, u_shadow.time, &g_cloudTime, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_shadowShader, u_shadow.cloudCoverage, &g_cloudCoverage, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_shadowShader, u_shadow.cloudBase, &g_cloudBaseAltitude, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_shadowShader, u_shadow.cloudTop, &g_cloudTopAltitude, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_shadowShader, u_shadow.cloudSpeed, &g_cloudSpeed, SHADER_UNIFORM_FLOAT);
        SetShaderValue(g_shadowShader, u_shadow.cloudOffset, &g_cloudOffsetCoord, SHADER_UNIFORM_FLOAT);

        rlBegin(RL_QUADS);
        rlColor4ub(255, 255, 255, 255);
        rlTexCoord2f(0.0f, 0.0f); rlVertex2f(0.0f, 0.0f);
        rlTexCoord2f(0.0f, 1.0f); rlVertex2f(0.0f, 512.0f);
        rlTexCoord2f(1.0f, 1.0f); rlVertex2f(512.0f, 512.0f);
        rlTexCoord2f(1.0f, 0.0f); rlVertex2f(512.0f, 0.0f);
        rlEnd();
        EndShaderMode();
        EndTextureMode();
    }
    double tPass4End = GetTime();
    double tEnd = GetTime();

    g_cloudMetrics.cloudPassMs   = (tPass1End - tPass1Start) * 1000.0;
    g_cloudMetrics.accumPassMs   = (tPass2End - tPass2Start) * 1000.0;
    g_cloudMetrics.upscalePassMs = (tPass3End - tPass3Start) * 1000.0;
    g_cloudMetrics.shadowPassMs  = (tPass4End - tPass4Start) * 1000.0;
    g_cloudMetrics.totalSystemMs = (tEnd - tStart) * 1000.0;
}

Texture2D GetAtmosphericSkyTexture() {
    return g_skyTargetFull.texture;
}

Texture2D GetCloudShadowMapTexture() {
    return g_cloudShadowMap.texture;
}

void CompositeAtmosphericSkyToTarget(RenderTexture2D target, bool isUnderwater, Color baseSkyClear) {
    ClearBackground(baseSkyClear);
    if (isUnderwater || g_skyTargetFull.id == 0) {
        ClearBackground(baseSkyClear);
        return;
    }
    DrawTexturePro(g_skyTargetFull.texture,
                   Rectangle{ 0, 0, (float)g_skyTargetFull.texture.width, -(float)g_skyTargetFull.texture.height },
                   Rectangle{ 0, 0, (float)target.texture.width, (float)target.texture.height },
                   Vector2{ 0, 0 }, 0.0f, WHITE);
}

float GetCelestialCloudOcclusion(Camera3D camera, Vector3 celestialDir) {
    if (celestialDir.y <= 0.02f) return 0.0f;
    float tEnter = fmaxf(0.0f, (g_cloudBaseAltitude - camera.position.y) / celestialDir.y);
    float tExit  = (g_cloudTopAltitude - camera.position.y) / celestialDir.y;
    if (tExit <= tEnter) return 0.0f;

    float opticalDepth = 0.0f;
    const int steps = 14;
    float dt = (tExit - tEnter) / (float)steps;
    for (int i = 0; i < steps; i++) {
        float t = tEnter + (float(i) + 0.5f) * dt;
        Vector3 p = Vector3Add(camera.position, Vector3Scale(celestialDir, t));
        float d = CpuSampleCloudDensity(p, g_cloudTime, g_cloudCoverage, g_cloudSpeed, g_cloudOffsetCoord);
        opticalDepth += d * dt * 0.085f;
    }
    return Clamp(1.0f - expf(-opticalDepth * 2.2f), 0.0f, 1.0f);
}

void DrawCloudGroundShadows(Camera3D camera, Vector3 sunDir, float sunElev, float timeVal = 0.0f) {
    (void)camera; (void)timeVal;
    if (sunElev <= 0.03f || g_cloudShadowMap.id == 0 || g_cloudCoverage <= 0.02f) return;

    BeginBlendMode(BLEND_MULTIPLIED);
    rlDisableDepthMask();
    rlEnableDepthTest();
    rlSetTexture(g_cloudShadowMap.texture.id);
    
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);

    float minX = -150.0f;
    float maxX =  450.0f;
    float minZ = -150.0f;
    float maxZ =  450.0f;
    float groundY = 10.025f;

    // UVs match OpenGL texture orientations
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(minX, groundY, minZ);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(minX, groundY, maxZ);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f(maxX, groundY, maxZ);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f(maxX, groundY, minZ);

    rlEnd();
    rlSetTexture(0);
    rlEnableDepthMask();
    EndBlendMode();
}

void DrawHybridCloudVolumes(Camera3D camera, Vector3 sunDir, float sunElev, float lightningFlash, float timeVal) {
    (void)camera; (void)sunDir; (void)sunElev; (void)lightningFlash; (void)timeVal;
}

void DrawAtmosphericSkyDome(Camera3D camera, float sunTheta, float lightningFlash) {
    (void)camera; (void)sunTheta; (void)lightningFlash;
}

void PopulateCloudInstances(Camera3D camera, Vector3 sunDir, float sunElev, float lightningFlash,
                            std::vector<Matrix> cloudInstances[256], float timeVal) {
    (void)camera; (void)sunDir; (void)sunElev; (void)lightningFlash; (void)timeVal;
    for (int i = 0; i < 256; i++) {
        cloudInstances[i].clear();
    }
}

CloudProfilingMetrics GetCloudProfilingMetrics() {
    return g_cloudMetrics;
}

void PrintCloudProfilingReport() {
    printf("\n--- VOLUMETRIC CLOUD & ATMOSPHERE PROFILING METRICS ---\n");
    printf("  Cloud Raymarch Pass (Half-Res):   %6.3f ms\n", g_cloudMetrics.cloudPassMs);
    printf("  Temporal Accumulation & Reproj:   %6.3f ms\n", g_cloudMetrics.accumPassMs);
    printf("  Bilateral Upscale Reconstruction: %6.3f ms\n", g_cloudMetrics.upscalePassMs);
    printf("  World Shadow Map Pass (512x512):  %6.3f ms\n", g_cloudMetrics.shadowPassMs);
    printf("  Total System Frame Cost:          %6.3f ms\n", g_cloudMetrics.totalSystemMs);
    printf("------------------------------------------------------\n\n");
    fflush(stdout);
}
