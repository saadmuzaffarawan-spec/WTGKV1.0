// GLSL 330 sources for every render pass.
#pragma once

#define WTGK_MAX_LIGHTS 16

// ---------------------------------------------------------------------------
// Shared lighting / fog snippet (lit surfaces, sky and particles).
// ---------------------------------------------------------------------------
static const char* kShaderCommon = R"GLSL(
#define MAXL 16
uniform vec3 uCamPos;
uniform float uTime;
uniform vec3 uFogCol;
uniform vec4 uFog;          // x density, y height falloff, z base height, w max opacity
uniform int  uNumLights;
uniform vec4 uLPos[MAXL];   // xyz position, w range
uniform vec4 uLCol[MAXL];   // rgb colour * intensity, w = 1 spot / 0 point
uniform vec4 uLDir[MAXL];   // xyz spot direction, w = cos(outer)
uniform vec4 uLExt[MAXL];   // x = cos(inner), y = casts spot shadow, z = volumetric strength, w = unused
uniform mat4 uSpotVP;
uniform float uSpotShadowOn;
uniform sampler2D texture1;  // spot shadow depth
uniform float uScatter;      // global volumetric multiplier

float IGN(vec2 p) { return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715)))); }

float FogAmount(vec3 cam, vec3 p) {
    vec3 d = p - cam;
    float dist = length(d);
    float k = uFog.y;
    float h0 = cam.y - uFog.z;
    float dy = d.y;
    float integral;
    if (abs(dy) > 0.02) integral = uFog.x * exp(-k * h0) * (1.0 - exp(-k * dy)) / (k * dy) * dist;
    else integral = uFog.x * exp(-k * h0) * dist;
    return clamp(1.0 - exp(-integral), 0.0, uFog.w);
}

float SpotShadow(vec3 wp, float bias) {
    if (uSpotShadowOn < 0.5) return 1.0;
    vec4 lp = uSpotVP * vec4(wp, 1.0);
    vec3 pr = lp.xyz / lp.w * 0.5 + 0.5;
    if (pr.x < 0.0 || pr.x > 1.0 || pr.y < 0.0 || pr.y > 1.0 || pr.z > 1.0) return 1.0;
    vec2 texel = 1.0 / vec2(textureSize(texture1, 0));
    float s = 0.0;
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++) {
            float d = texture(texture1, pr.xy + vec2(x, y) * texel * 1.5).r;
            s += (pr.z - bias > d) ? 0.0 : 1.0;
        }
    return s / 9.0;
}
float SpotShadowFast(vec3 wp) {
    vec4 lp = uSpotVP * vec4(wp, 1.0);
    vec3 pr = lp.xyz / lp.w * 0.5 + 0.5;
    if (pr.x < 0.0 || pr.x > 1.0 || pr.y < 0.0 || pr.y > 1.0 || pr.z > 1.0) return 1.0;
    return (pr.z - 0.0015 > texture(texture1, pr.xy).r) ? 0.0 : 1.0;
}

// In-scattered light along the view ray from the camera to `dist` metres.
vec3 Scatter(vec3 ro, vec3 rd, float dist, vec2 fragXY) {
    vec3 acc = vec3(0.0);
    float dens = uFog.x * uScatter;
    if (dens <= 0.0) return acc;
    float jitter = fract(sin(dot(fragXY + fract(uTime * 3.17) * 91.0, vec2(12.9898, 78.233))) * 43758.5453);
    for (int i = 0; i < MAXL; i++) {
        if (i >= uNumLights) break;
        float strength = uLExt[i].z;
        if (strength <= 0.0) continue;
        vec3 lp = uLPos[i].xyz;
        float range = uLPos[i].w;
        vec3 col = uLCol[i].rgb * strength;
        if (uLCol[i].w < 0.5) {
            // Point light: analytic integral of 1/(h^2 + t^2) along the ray
            float t0 = dot(lp - ro, rd);
            vec3 cp = ro + rd * t0;
            float h = max(length(lp - cp), 0.05);
            float a = atan((dist - t0) / h) - atan(-t0 / h);
            float fall = exp(-max(length(lp - cp) - range * 0.5, 0.0) / range);
            acc += col * (a / h) * fall * dens * 0.12;
        } else {
            // Spot light: short ray-march with the spot cone (and its shadow)
            float maxT = min(dist, range);
            const int STEPS = 10;
            float stepL = maxT / float(STEPS);
            vec3 sum = vec3(0.0);
            for (int s = 0; s < STEPS; s++) {
                float t = (float(s) + jitter) * stepL;
                vec3 p = ro + rd * t;
                vec3 L = lp - p;
                float d2 = dot(L, L);
                float d = sqrt(d2);
                vec3 Ln = L / d;
                float cosA = dot(-Ln, uLDir[i].xyz);
                float cone = smoothstep(uLDir[i].w, uLExt[i].x, cosA);
                if (cone <= 0.0) continue;
                float att = pow(clamp(1.0 - pow(d / range, 4.0), 0.0, 1.0), 2.0) / (d2 + 0.6);
                float sh = (uLExt[i].y > 0.5 && uSpotShadowOn > 0.5) ? SpotShadowFast(p) : 1.0;
                // Henyey-Greenstein-ish forward scattering
                float mu = dot(rd, -Ln);
                float phase = 0.35 + 0.65 * pow(max(mu, 0.0), 6.0);
                sum += col * cone * att * sh * phase;
            }
            acc += sum * stepL * dens * 0.35;
        }
    }
    return acc;
}
)GLSL";

// ---------------------------------------------------------------------------
// Lit surface shader
// ---------------------------------------------------------------------------
static const char* kLitVS = R"GLSL(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
#ifdef INSTANCED
in mat4 instanceTransform;
#endif
uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matProjection;
uniform float uTime;
uniform vec4 uWind;   // xy direction, z strength, w gust
out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vColor;
void main() {
#ifdef INSTANCED
    mat4 M = instanceTransform;
#else
    mat4 M = matModel;
#endif
    vec4 wp = M * vec4(vertexPosition, 1.0);
    float flex = vertexColor.a < 0.99 ? (1.0 - vertexColor.a) : 0.0;
    if (flex > 0.0) {
        float ph = dot(wp.xz, vec2(0.37, 0.61));
        float gust = 0.6 + 0.4 * sin(uTime * 0.7 + ph * 0.05) * uWind.w;
        float sway = sin(uTime * 1.9 + ph) * 0.6 + sin(uTime * 3.7 + ph * 1.7) * 0.25;
        vec2 off = uWind.xy * (uWind.z * gust + sway * uWind.z * 0.5) * flex * flex;
        wp.xz += off;
        wp.y -= length(off) * 0.25 * flex;
    }
    vWorldPos = wp.xyz;
    vNormal = normalize(mat3(M) * vertexNormal);
    vUV = vertexTexCoord;
    vColor = vec4(vertexColor.rgb, 1.0);
    gl_Position = matProjection * matView * wp;
}
)GLSL";

static const char* kLitFS = R"GLSL(
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec4 vColor;
uniform sampler2D texture0;  // albedo A (a = roughness)
uniform sampler2D texture2;  // normal A (a = AO)
uniform sampler2D texture3;  // albedo B
uniform sampler2D texture4;  // normal B
uniform sampler2D texture5;  // albedo C
uniform sampler2D texture6;  // normal C
uniform sampler2D texture10; // moon shadow depth
uniform vec4 colDiffuse;
uniform vec4 uMat;    // x tex scale, y roughness mul, z metalness, w mode
uniform vec4 uMat2;   // x normal strength, y wetness, z emissive strength, w wrap (sss)
uniform vec3 uEmissive;
uniform vec3 uMoonDir;
uniform vec3 uMoonCol;
uniform vec3 uSkyAmb;
uniform vec3 uGroundAmb;
uniform mat4 uMoonVP;
uniform float uMoonShadowOn;
uniform float uLightning;
uniform float uWetWorld;
out vec4 finalColor;

vec3 BlendW(vec3 n) { vec3 w = pow(abs(n), vec3(4.0)); return w / (w.x + w.y + w.z); }

void Triplanar(sampler2D alb, sampler2D nrm, vec3 p, vec3 n, float sc, float nstr, out vec4 A, out vec3 N, out float ao) {
    vec3 w = BlendW(n);
    vec2 uvX = p.zy * sc, uvY = p.xz * sc, uvZ = p.xy * sc;
    A = texture(alb, uvX) * w.x + texture(alb, uvY) * w.y + texture(alb, uvZ) * w.z;
    vec4 nx = texture(nrm, uvX), ny = texture(nrm, uvY), nz = texture(nrm, uvZ);
    ao = nx.a * w.x + ny.a * w.y + nz.a * w.z;
    vec3 tx = nx.xyz * 2.0 - 1.0, ty = ny.xyz * 2.0 - 1.0, tz = nz.xyz * 2.0 - 1.0;
    tx.xy *= nstr; ty.xy *= nstr; tz.xy *= nstr;
    tx = vec3(tx.xy + n.zy, abs(tx.z) * n.x);
    ty = vec3(ty.xy + n.xz, abs(ty.z) * n.y);
    tz = vec3(tz.xy + n.xy, abs(tz.z) * n.z);
    N = normalize(tx.zyx * w.x + ty.xzy * w.y + tz.xyz * w.z);
}

// Two-scale sampling for large surfaces to hide tiling
void TriplanarAT(sampler2D alb, sampler2D nrm, vec3 p, vec3 n, float sc, float nstr, out vec4 A, out vec3 N, out float ao) {
    vec4 A1; vec3 N1; float ao1;
    vec4 A2; vec3 N2; float ao2;
    Triplanar(alb, nrm, p, n, sc, nstr, A1, N1, ao1);
    Triplanar(alb, nrm, p.zyx * vec3(1.0, 1.0, -1.0) + 13.7, n.zyx * vec3(1.0, 1.0, -1.0), sc * 0.27, nstr, A2, N2, ao2);
    N2 = N2.zyx * vec3(-1.0, 1.0, 1.0);
    float m = smoothstep(0.35, 0.65, texture(alb, p.xz * sc * 0.031).g * 2.2);
    A = mix(A1, A2, m * 0.5);
    A.rgb *= 0.82 + 0.36 * texture(alb, p.xz * sc * 0.013 + 0.5).r * 2.0;
    N = normalize(mix(N1, N2, m * 0.5));
    ao = mix(ao1, ao2, m * 0.5);
}

float MoonShadow(vec3 wp, vec3 n) {
    if (uMoonShadowOn < 0.5) return 1.0;
    vec4 lp = uMoonVP * vec4(wp + n * 0.08, 1.0);
    vec3 pr = lp.xyz / lp.w * 0.5 + 0.5;
    if (pr.x < 0.0 || pr.x > 1.0 || pr.y < 0.0 || pr.y > 1.0 || pr.z > 1.0) return 1.0;
    vec2 texel = 1.0 / vec2(textureSize(texture10, 0));
    float s = 0.0;
    float ang = IGN(gl_FragCoord.xy) * 6.2831;
    mat2 rot = mat2(cos(ang), -sin(ang), sin(ang), cos(ang));
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++) {
            vec2 o = rot * vec2(x, y) * texel * 1.6;
            float d = texture(texture10, pr.xy + o).r;
            s += (pr.z - 0.0012 > d) ? 0.0 : 1.0;
        }
    return s / 9.0;
}

float D_GGX(float NoH, float a) { float a2 = a * a; float d = NoH * NoH * (a2 - 1.0) + 1.0; return a2 / (3.14159 * d * d); }
float V_Smith(float NoV, float NoL, float a) {
    float k = a * 0.5;
    return 0.25 / ((NoV * (1.0 - k) + k) * (NoL * (1.0 - k) + k));
}

void main() {
    int mode = int(uMat.w + 0.5);
    vec3 n0 = normalize(vNormal);
    if (!gl_FrontFacing) n0 = -n0;
    vec4 A; vec3 N; float ao = 1.0;
    if (mode == 1 || mode == 3 || mode == 4) {
        A = texture(texture0, vUV);
        N = n0;
        if (mode == 1) { A.a = 0.6; }
    } else if (mode == 2) {
        vec4 A1, A2, A3; vec3 N1, N2, N3; float o1, o2, o3;
        float sc = uMat.x;
        vec3 w = vColor.rgb;
        w /= max(w.r + w.g + w.b, 0.001);
        TriplanarAT(texture0, texture2, vWorldPos, n0, sc, uMat2.x, A1, N1, o1);
        TriplanarAT(texture3, texture4, vWorldPos, n0, sc * 1.3, uMat2.x, A2, N2, o2);
        TriplanarAT(texture5, texture6, vWorldPos, n0, sc * 1.1, uMat2.x, A3, N3, o3);
        // height-aware blend so dirt shows between grass tufts
        float h1 = A1.g + w.r * 1.2, h2 = (1.0 - A2.g) * 0.6 + w.g * 1.2, h3 = A3.r + w.b * 1.2;
        float mx = max(h1, max(h2, h3)) - 0.25;
        vec3 bw = max(vec3(h1, h2, h3) - mx, 0.0);
        bw /= max(bw.x + bw.y + bw.z, 0.001);
        A = A1 * bw.x + A2 * bw.y + A3 * bw.z;
        N = normalize(N1 * bw.x + N2 * bw.y + N3 * bw.z);
        ao = o1 * bw.x + o2 * bw.y + o3 * bw.z;
    } else {
        Triplanar(texture0, texture2, vWorldPos, n0, uMat.x, uMat2.x, A, N, ao);
    }

    vec3 tint = pow(colDiffuse.rgb, vec3(2.2));
    vec3 albedo = pow(A.rgb, vec3(2.2)) * tint;
    if (mode != 2) albedo *= vColor.rgb;

    vec3 V = normalize(uCamPos - vWorldPos);
    float dist = length(uCamPos - vWorldPos);

    if (mode == 3) {
        // unlit emissive (bulbs, screens, glowing signs)
        vec3 c = albedo * uEmissive * uMat2.z;
        float f = FogAmount(uCamPos, vWorldPos);
        c = mix(c, uFogCol, f * 0.6);
        c += Scatter(uCamPos, -V, dist, gl_FragCoord.xy);
        finalColor = vec4(c, colDiffuse.a * A.a);
        return;
    }

    float rough = clamp(A.a * uMat.y, 0.045, 1.0);
    float metal = uMat.z;
    float wet = clamp(uMat2.y + uWetWorld * max(N.y, 0.0) * (mode == 2 || mode == 0 ? 1.0 : 0.0), 0.0, 1.0);
    albedo *= mix(1.0, 0.55, wet);
    rough = mix(rough, 0.07, wet * max(N.y, 0.3));
    vec3 F0 = mix(vec3(0.04), albedo, metal);
    vec3 diff = albedo * (1.0 - metal);
    float NoV = max(dot(N, V), 0.001);
    float a = rough * rough;
    float wrap = uMat2.w;

    vec3 Lo = vec3(0.0);
    // Moon (directional)
    {
        vec3 L = normalize(uMoonDir);
        float NoLr = dot(N, L);
        float NoL = clamp((NoLr + wrap) / (1.0 + wrap), 0.0, 1.0);
        if (NoL > 0.0) {
            vec3 H = normalize(L + V);
            float NoH = max(dot(N, H), 0.0);
            vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
            vec3 spec = D_GGX(NoH, a) * V_Smith(NoV, max(NoLr, 0.001), a) * F;
            float sh = MoonShadow(vWorldPos, n0);
            Lo += (diff / 3.14159 + spec * step(0.0, NoLr)) * uMoonCol * NoL * sh * 3.14159;
        }
    }
    for (int i = 0; i < MAXL; i++) {
        if (i >= uNumLights) break;
        vec3 Lv = uLPos[i].xyz - vWorldPos;
        float d2 = dot(Lv, Lv);
        float range = uLPos[i].w;
        if (d2 > range * range) continue;
        float d = sqrt(d2);
        vec3 L = Lv / d;
        float att = pow(clamp(1.0 - pow(d / range, 4.0), 0.0, 1.0), 2.0) / (d2 + 0.6);
        if (uLCol[i].w > 0.5) {
            float cosA = dot(-L, uLDir[i].xyz);
            float cone = smoothstep(uLDir[i].w, uLExt[i].x, cosA);
            // slight "cookie": hot centre ring + dim outer spill like a real torch reflector
            float ring = 0.8 + 0.35 * smoothstep(uLExt[i].x, 1.0, cosA) - 0.12 * smoothstep(0.0, 1.0, sin(acos(clamp(cosA,0.0,1.0)) * 90.0));
            att *= cone * ring;
            if (uLExt[i].y > 0.5) att *= SpotShadow(vWorldPos + n0 * 0.02, 0.0008 + 0.002 * (1.0 - max(dot(n0, L), 0.0)));
        }
        if (att <= 0.0) continue;
        float NoLr = dot(N, L);
        float NoL = clamp((NoLr + wrap) / (1.0 + wrap), 0.0, 1.0);
        vec3 H = normalize(L + V);
        float NoH = max(dot(N, H), 0.0);
        vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
        vec3 spec = D_GGX(NoH, a) * V_Smith(NoV, max(NoLr, 0.001), a) * F;
        Lo += (diff / 3.14159 + spec * step(0.0, NoLr)) * uLCol[i].rgb * NoL * att * 3.14159;
    }
    // Ambient (hemisphere) + faint environment specular
    vec3 amb = mix(uGroundAmb, uSkyAmb, N.y * 0.5 + 0.5);
    amb += vec3(0.55, 0.62, 0.8) * uLightning * max(N.y * 0.6 + 0.4, 0.0);
    vec3 Fa = F0 + (max(vec3(1.0 - rough), F0) - F0) * pow(1.0 - NoV, 5.0);
    vec3 color = Lo + diff * amb * ao + Fa * (uFogCol * 0.6 + amb) * ao * (1.0 - rough);
    color += uEmissive * uMat2.z * albedo;

    float fog = FogAmount(uCamPos, vWorldPos);
    color = mix(color, uFogCol * (1.0 + uLightning * 2.0), fog);
    color += Scatter(uCamPos, -V, dist, gl_FragCoord.xy);

    float alpha = colDiffuse.a;
    if (mode == 4) {
        // glass: fresnel-weighted reflection of the fog/sky + highlights
        float fres = 0.08 + 0.92 * pow(1.0 - NoV, 4.0);
        vec3 refl = uFogCol * 1.4 + Lo * 2.0;
        color = mix(albedo * 0.2, refl, fres) + Lo;
        alpha = clamp(colDiffuse.a + fres * 0.6, 0.0, 1.0);
    }
    finalColor = vec4(color, alpha);
}
)GLSL";

// ---------------------------------------------------------------------------
// Depth-only (shadow) shader
// ---------------------------------------------------------------------------
static const char* kDepthVS = R"GLSL(
#version 330
in vec3 vertexPosition;
uniform mat4 mvp;
void main() { gl_Position = mvp * vec4(vertexPosition, 1.0); }
)GLSL";
static const char* kDepthFS = R"GLSL(
#version 330
out vec4 finalColor;
void main() { finalColor = vec4(1.0); }
)GLSL";

// ---------------------------------------------------------------------------
// Sky: drawn on a large box around the camera.
// ---------------------------------------------------------------------------
static const char* kSkyVS = R"GLSL(
#version 330
in vec3 vertexPosition;
uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matProjection;
out vec3 vWorldPos;
void main() {
    vec4 wp = matModel * vec4(vertexPosition, 1.0);
    vWorldPos = wp.xyz;
    gl_Position = matProjection * matView * wp;
}
)GLSL";

static const char* kSkyFS = R"GLSL(
in vec3 vWorldPos;
uniform vec3 uMoonDir;
uniform vec3 uZenith;
uniform vec3 uHorizon;
uniform vec3 uGlowCol;     // sodium light pollution from the town
uniform vec3 uGlowDir;
uniform float uClouds;     // coverage 0..1
uniform float uLightning;
uniform float uStars;
uniform float uMoonBright;
out vec4 finalColor;

float H(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float VN(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(H(i), H(i + vec2(1, 0)), f.x), mix(H(i + vec2(0, 1)), H(i + vec2(1, 1)), f.x), f.y);
}
float FBM(vec2 p) { float s = 0.0, a = 0.5; for (int i = 0; i < 6; i++) { s += VN(p) * a; p = p * 2.03 + 7.1; a *= 0.5; } return s; }

void main() {
    vec3 rd = normalize(vWorldPos - uCamPos);
    float el = rd.y;
    // Base night gradient
    vec3 col = mix(uHorizon, uZenith, pow(clamp(el, 0.0, 1.0), 0.45));
    // Light pollution glow near horizon toward the town
    float gdot = max(dot(normalize(vec3(rd.x, 0.0, rd.z)), normalize(uGlowDir)), 0.0);
    col += uGlowCol * pow(gdot, 3.0) * exp(-max(el, 0.0) * 9.0);
    col += uGlowCol * 0.25 * exp(-max(el, 0.0) * 14.0);

    // Stars
    if (el > 0.0) {
        vec3 sd = rd * 420.0;
        vec2 cell = floor(vec2(atan(rd.z, rd.x) * 180.0, el * 360.0));
        float h = H(cell);
        if (h > 0.985) {
            vec2 cp = fract(vec2(atan(rd.z, rd.x) * 180.0, el * 360.0)) - 0.5;
            float star = smoothstep(0.18, 0.0, length(cp)) * (h - 0.985) * 66.0;
            float tw = 0.6 + 0.4 * sin(uTime * (2.0 + h * 9.0) + h * 100.0);
            col += vec3(0.8, 0.85, 1.0) * star * tw * uStars * smoothstep(0.0, 0.25, el) * 0.9;
        }
    }

    // Moon disc + halo
    float md = dot(rd, normalize(uMoonDir));
    float disc = smoothstep(0.99985, 0.99992, md);
    vec3 moonTex = vec3(0.9, 0.92, 1.0) * (0.75 + 0.25 * VN(rd.xy * 900.0) * VN(rd.zy * 1300.0 + 3.0));
    col += moonTex * disc * uMoonBright * 6.0;
    col += vec3(0.55, 0.62, 0.8) * (pow(max(md, 0.0), 900.0) * 0.6 + pow(max(md, 0.0), 40.0) * 0.05 + pow(max(md, 0.0), 6.0) * 0.012) * uMoonBright;

    // Clouds on a virtual plane
    if (el > -0.02) {
        vec2 uv = rd.xz / (el + 0.12) * 1.8 + vec2(uTime * 0.012, uTime * 0.004);
        float n = FBM(uv);
        float cov = smoothstep(1.0 - uClouds, 1.0 - uClouds + 0.35, n);
        float thick = FBM(uv * 2.0 + 3.0);
        float lit = pow(max(md, 0.0), 8.0) * 1.2 + 0.25;
        vec3 cloudCol = mix(vec3(0.012, 0.014, 0.02), vec3(0.08, 0.09, 0.11) * lit * uMoonBright, 1.0 - thick);
        cloudCol += uGlowCol * 0.5 * exp(-max(el, 0.0) * 5.0);
        cloudCol += vec3(0.7, 0.75, 0.95) * uLightning * (0.6 + thick);
        float fade = smoothstep(-0.02, 0.15, el);
        col = mix(col, cloudCol, cov * fade * 0.95);
    }
    // Horizon fog merge
    float hf = exp(-max(el, 0.0) * 7.0);
    col = mix(col, uFogCol, clamp(hf * uFog.w, 0.0, 1.0));
    if (el < 0.0) col = uFogCol;
    col += uLightning * vec3(0.25, 0.28, 0.35) * (1.0 - clamp(el, 0.0, 1.0));
    col += Scatter(uCamPos, rd, 70.0, gl_FragCoord.xy);
    // dither
    col += (IGN(gl_FragCoord.xy) - 0.5) / 255.0;
    finalColor = vec4(col, 1.0);
}
)GLSL";

// ---------------------------------------------------------------------------
// Particles: soft billboards with fog, additive or alpha.
// ---------------------------------------------------------------------------
static const char* kParticleVS = R"GLSL(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
out vec2 vUV;
out vec4 vColor;
out vec3 vWorldPos;
void main() {
    vUV = vertexTexCoord;
    vColor = vertexColor;
    vWorldPos = vertexPosition;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)GLSL";
static const char* kParticleFS = R"GLSL(
in vec2 vUV;
in vec4 vColor;
in vec3 vWorldPos;
uniform sampler2D texture0;
uniform float uIntensity;
out vec4 finalColor;
void main() {
    vec4 t = texture(texture0, vUV);
    vec3 c = pow(vColor.rgb, vec3(2.2)) * uIntensity;
    float f = FogAmount(uCamPos, vWorldPos);
    c = mix(c, uFogCol, f * 0.8);
    finalColor = vec4(c, t.a * vColor.a);
}
)GLSL";

// ---------------------------------------------------------------------------
// Post: bloom helpers and final composite
// ---------------------------------------------------------------------------
static const char* kPostVS = R"GLSL(
#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
out vec2 fragTexCoord;
void main() { fragTexCoord = vertexTexCoord; gl_Position = mvp * vec4(vertexPosition, 1.0); }
)GLSL";

static const char* kBrightFS = R"GLSL(
#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform float uThreshold;
out vec4 finalColor;
void main() {
    vec2 t = 1.0 / vec2(textureSize(texture0, 0));
    vec3 c = texture(texture0, fragTexCoord).rgb * 0.25;
    c += texture(texture0, fragTexCoord + vec2(t.x, 0)).rgb * 0.1875;
    c += texture(texture0, fragTexCoord - vec2(t.x, 0)).rgb * 0.1875;
    c += texture(texture0, fragTexCoord + vec2(0, t.y)).rgb * 0.1875;
    c += texture(texture0, fragTexCoord - vec2(0, t.y)).rgb * 0.1875;
    float l = max(c.r, max(c.g, c.b));
    float k = max(l - uThreshold, 0.0) / max(l, 0.0001);
    finalColor = vec4(min(c * k, vec3(40.0)), 1.0);
}
)GLSL";

static const char* kBlurFS = R"GLSL(
#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 uDir;
out vec4 finalColor;
void main() {
    vec2 t = uDir / vec2(textureSize(texture0, 0));
    vec3 c = texture(texture0, fragTexCoord).rgb * 0.227027;
    c += texture(texture0, fragTexCoord + t * 1.3846).rgb * 0.316216;
    c += texture(texture0, fragTexCoord - t * 1.3846).rgb * 0.316216;
    c += texture(texture0, fragTexCoord + t * 3.2308).rgb * 0.070270;
    c += texture(texture0, fragTexCoord - t * 3.2308).rgb * 0.070270;
    finalColor = vec4(c, 1.0);
}
)GLSL";

static const char* kCompositeFS = R"GLSL(
#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;   // HDR scene
uniform sampler2D texture1;   // bloom (half)
uniform sampler2D texture2;   // bloom (quarter)
uniform sampler2D texture3;   // glyph atlas
uniform vec2 uRes;
uniform float uTime;
uniform float uExposure;
uniform float uBloom;
uniform float uGrain;
uniform float uVignette;
uniform float uCA;
uniform float uAscii;        // spirit sight amount 0..1
uniform float uAsciiFull;    // 0..1 whole-screen ASCII (cutscenes / death)
uniform float uCell;
uniform float uGlyphs;
uniform float uBlink;        // 0 open .. 1 closed
uniform float uFade;         // 0 .. 1 to black
uniform float uWhite;        // flash to white
uniform float uDesat;
uniform float uRedPulse;
uniform float uBlur;         // concussion blur
uniform float uGamma;
uniform vec3 uGradeShadow;
uniform vec3 uGradeHigh;
out vec4 finalColor;

float Hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }
vec3 ACES(vec3 x) {
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
vec3 SceneAt(vec2 uv) {
    vec2 cc = uv - 0.5;
    float r2 = dot(cc, cc);
    vec2 off = cc * r2 * uCA * 0.012;
    vec3 c;
    c.r = texture(texture0, uv - off).r;
    c.g = texture(texture0, uv).g;
    c.b = texture(texture0, uv + off).b;
    // FXAA-lite: blend along edges detected in luminance
    {
        vec2 px = 1.0 / uRes;
        vec3 lw = vec3(0.299, 0.587, 0.114);
        float lN = dot(texture(texture0, uv + vec2(0, px.y)).rgb, lw);
        float lS = dot(texture(texture0, uv - vec2(0, px.y)).rgb, lw);
        float lE = dot(texture(texture0, uv + vec2(px.x, 0)).rgb, lw);
        float lW = dot(texture(texture0, uv - vec2(px.x, 0)).rgb, lw);
        float lM = dot(c, lw);
        float mn = min(lM, min(min(lN, lS), min(lE, lW)));
        float mx = max(lM, max(max(lN, lS), max(lE, lW)));
        float range = mx - mn;
        if (range > max(0.02, mx * 0.12)) {
            vec2 dir = vec2(-(lN - lS), (lE - lW));
            float dl = max(abs(dir.x), abs(dir.y));
            dir = clamp(dir / max(dl, 1e-4), -1.5, 1.5) * px;
            vec3 a = 0.5 * (texture(texture0, uv + dir * (1.0/3.0 - 0.5)).rgb + texture(texture0, uv + dir * (2.0/3.0 - 0.5)).rgb);
            c = mix(c, a, clamp(range / max(mx, 1e-3), 0.0, 1.0) * 0.8);
        }
    }
    if (uBlur > 0.0) {
        vec2 t = uBlur * 6.0 / uRes;
        vec3 b = vec3(0.0);
        b += texture(texture0, uv + vec2(t.x, t.y)).rgb;
        b += texture(texture0, uv + vec2(-t.x, t.y)).rgb;
        b += texture(texture0, uv + vec2(t.x, -t.y)).rgb;
        b += texture(texture0, uv + vec2(-t.x, -t.y)).rgb;
        b += texture(texture1, uv).rgb * 2.0;
        c = mix(c, b / 6.0, clamp(uBlur, 0.0, 1.0));
    }
    return c;
}
vec3 Grade(vec3 hdr) {
    vec3 c = hdr * uExposure;
    c = ACES(c);
    float l = dot(c, vec3(0.2126, 0.7152, 0.0722));
    c = mix(c, vec3(l), uDesat);
    // split tone: cold shadows, slightly warm highlights
    c = c + uGradeShadow * (1.0 - smoothstep(0.0, 0.35, l)) * 0.04 + uGradeHigh * smoothstep(0.4, 1.0, l) * 0.03;
    c = pow(max(c, 0.0), vec3(1.0 / uGamma));
    return c;
}

void main() {
    vec2 uv = fragTexCoord;
    vec3 hdr = SceneAt(uv);
    vec3 bloom = texture(texture1, uv).rgb * 0.6 + texture(texture2, uv).rgb * 0.8;
    hdr += bloom * uBloom;
    vec3 col = Grade(hdr);

    // --- Spirit sight: characters grow out of the dark ---------------------
    float lumPix = dot(col, vec3(0.2126, 0.7152, 0.0722));
    vec2 cc = uv - 0.5;
    float edge = smoothstep(0.18, 0.62, length(cc * vec2(uRes.x / uRes.y, 1.0)) );
    float darkMask = 1.0 - smoothstep(0.015, 0.11, lumPix);
    float amt = clamp(uAscii * (darkMask * (0.35 + 0.65 * edge) + edge * 0.25), 0.0, 1.0);
    amt = max(amt, uAsciiFull);
    if (amt > 0.002) {
        vec2 px = uv * uRes;
        vec2 cell = floor(px / uCell);
        vec2 inCell = fract(px / uCell);
        vec2 cuv = (cell + 0.5) * uCell / uRes;
        vec3 cellHdr = texture(texture0, cuv).rgb + texture(texture1, cuv).rgb * 0.5;
        vec3 cellCol = Grade(cellHdr);
        float lum = dot(cellCol, vec3(0.2126, 0.7152, 0.0722));
        float flick = Hash(cell + floor(uTime * 3.0 + Hash(cell) * 7.0));
        float g = clamp(pow(lum * 5.0, 0.55) + (flick - 0.5) * 0.18, 0.0, 0.999);
        float gi = floor(g * uGlyphs);
        vec2 auv = vec2((gi + inCell.x) / uGlyphs, inCell.y);
        float m = texture(texture3, auv).r;
        vec3 tintA = vec3(0.62, 0.66, 0.60);
        vec3 glyphCol = (cellCol * 2.2 + tintA * 0.05) * m;
        glyphCol += tintA * m * 0.035 * (0.5 + flick);
        vec3 asciiImg = mix(cellCol * 0.25, glyphCol, 0.9);
        col = mix(col, asciiImg, amt);
    }

    // --- Lens / film ------------------------------------------------------
    float vig = smoothstep(0.95, 0.25, length(cc * vec2(1.1, 1.0)));
    col *= mix(1.0, vig, uVignette);
    col = mix(col, vec3(0.5, 0.02, 0.02) * (0.6 + 0.4 * sin(uTime * 8.0)), uRedPulse * (1.0 - vig) * 0.6);
    float gr = Hash(uv * uRes + fract(uTime * 13.37) * 100.0) - 0.5;
    float grainAmt = uGrain * (0.35 + 0.65 * (1.0 - smoothstep(0.0, 0.6, lumPix)));
    col += gr * grainAmt;
    // eyelids
    if (uBlink > 0.0) {
        float lid = uBlink * 0.52;
        float curve = cc.x * cc.x * 0.35;
        float top = smoothstep(0.5 - lid - 0.06, 0.5 - lid + 0.02, cc.y - curve);
        float bot = smoothstep(0.5 - lid - 0.06, 0.5 - lid + 0.02, -cc.y - curve);
        float lids = max(top, bot);
        col = mix(col, vec3(0.012, 0.004, 0.003), lids);
        col *= 1.0 - uBlink * 0.5;
    }
    col = mix(col, vec3(1.0), uWhite);
    col *= 1.0 - uFade;
    finalColor = vec4(col, 1.0);
}
)GLSL";
