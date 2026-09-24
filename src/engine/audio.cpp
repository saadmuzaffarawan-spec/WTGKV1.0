#include "audio.h"
#include "collision.h"
#include <map>
#include <mutex>
#include <functional>

namespace {

const int SR = 44100;

// ---------------------------------------------------------------------------
// DSP toolkit
// ---------------------------------------------------------------------------
struct Rnd {
    uint32_t s;
    explicit Rnd(uint32_t seed = 22222) : s(seed ? seed : 1) {}
    inline uint32_t U() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline float W() { return (U() & 0xffffff) / 8388608.0f - 1.0f; }       // white [-1,1]
    inline float F() { return (U() & 0xffffff) / 16777216.0f; }             // [0,1)
    inline float R(float a, float b) { return a + (b - a) * F(); }
};

struct Biquad {
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float P(float x) {
        float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void Set(int type, float f, float q, float gainDb = 0) {
        f = Clamp(f, 10.0f, SR * 0.45f);
        float w = 2 * PI * f / SR, cw = cosf(w), sw = sinf(w), al = sw / (2 * q);
        float A = powf(10.0f, gainDb / 40.0f);
        float a0;
        switch (type) {
        case 0: b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = (1 - cw) / 2; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;  // LP
        case 1: b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = (1 + cw) / 2; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;  // HP
        case 2: b0 = al; b1 = 0; b2 = -al; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al; break;                           // BP 0dB peak
        default: b0 = 1 + al * A; b1 = -2 * cw; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cw; a2 = 1 - al / A; break;  // peaking
        }
        b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    }
    static Biquad LP(float f, float q = 0.707f) { Biquad b; b.Set(0, f, q); return b; }
    static Biquad HP(float f, float q = 0.707f) { Biquad b; b.Set(1, f, q); return b; }
    static Biquad BP(float f, float q = 1.0f) { Biquad b; b.Set(2, f, q); return b; }
};

struct OnePole {
    float a = 0.5f, y = 0;
    void SetLP(float f) { a = 1.0f - expf(-2 * PI * f / SR); }
    inline float LP(float x) { y += a * (x - y); return y; }
    inline float HP(float x) { y += a * (x - y); return x - y; }
};

struct Pink {
    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    inline float P(float w) {
        b0 = 0.99886f * b0 + w * 0.0555179f; b1 = 0.99332f * b1 + w * 0.0750759f;
        b2 = 0.96900f * b2 + w * 0.1538520f; b3 = 0.86650f * b3 + w * 0.3104856f;
        b4 = 0.55000f * b4 + w * 0.5329522f; b5 = -0.7616f * b5 - w * 0.0168980f;
        float o = b0 + b1 + b2 + b3 + b4 + b5 + b6 + w * 0.5362f;
        b6 = w * 0.115926f;
        return o * 0.11f;
    }
};

struct Brown {
    float y = 0;
    inline float P(float w) { y = (y + w * 0.02f) * 0.998f; return y * 3.5f; }
};

using Buf = std::vector<float>;
inline int N(float sec) { return (int)(sec * SR); }

// Filtered noise burst with exponential decay
void Burst(Buf& b, Rnd& r, float t0, float attack, float decay, int type, float fc, float q, float amp, float len = -1) {
    Biquad f; f.Set(type, fc, q);
    if (len < 0) len = attack + decay * 6;
    int s0 = N(t0), n = N(len);
    for (int i = 0; i < n && s0 + i < (int)b.size(); i++) {
        float t = (float)i / SR;
        float env = t < attack ? t / attack : expf(-(t - attack) / decay);
        b[s0 + i] += f.P(r.W()) * env * amp;
    }
}
// Sum of damped sinusoids (modal impact)
void Modal(Buf& b, float t0, const std::vector<float>& fr, const std::vector<float>& dec, const std::vector<float>& amp, float phaseJit = 0.0f) {
    int s0 = N(t0);
    for (size_t m = 0; m < fr.size(); m++) {
        int n = N(dec[m] * 7.0f);
        float w = 2 * PI * fr[m] / SR;
        float ph = phaseJit * m;
        for (int i = 0; i < n && s0 + i < (int)b.size(); i++)
            b[s0 + i] += sinf(w * i + ph) * expf(-(float)i / SR / dec[m]) * amp[m];
    }
}
// Low sine thump with pitch drop
void Thump(Buf& b, float t0, float f0, float f1, float decay, float amp) {
    int s0 = N(t0), n = N(decay * 6);
    float ph = 0;
    for (int i = 0; i < n && s0 + i < (int)b.size(); i++) {
        float t = (float)i / SR;
        float f = f1 + (f0 - f1) * expf(-t * 30.0f);
        ph += 2 * PI * f / SR;
        b[s0 + i] += sinf(ph) * expf(-t / decay) * (t < 0.003f ? t / 0.003f : 1.0f) * amp;
    }
}
// Poisson grains of short band-passed noise (crunch, crackle, gravel)
void Grains(Buf& b, Rnd& r, float t0, float t1, float rate, float fmin, float fmax, float amp, float gdur) {
    float t = t0;
    while (t < t1) {
        t += -logf(fmaxf(r.F(), 1e-5f)) / rate;
        if (t >= t1) break;
        float env = 1.0f - (t - t0) / (t1 - t0) * 0.6f;
        Burst(b, r, t, 0.0005f, gdur * r.R(0.5f, 1.5f), 2, r.R(fmin, fmax), r.R(1.0f, 3.0f), amp * env * r.R(0.3f, 1.0f), gdur * 5);
    }
}
// Stick-slip friction excitation of a resonator bank (creaks, groans)
void Creak(Buf& b, Rnd& r, float t0, float dur, float rate0, float rate1, const std::vector<float>& fr, float amp) {
    std::vector<Biquad> res;
    for (float f : fr) res.push_back(Biquad::BP(f, 18.0f));
    int s0 = N(t0), n = N(dur);
    float phase = 0;
    for (int i = 0; i < n && s0 + i < (int)b.size(); i++) {
        float t = (float)i / n;
        float rate = rate0 + (rate1 - rate0) * t + sinf(t * 17.0f) * rate0 * 0.3f + r.W() * rate0 * 0.15f;
        phase += rate / SR;
        float exc = 0;
        if (phase >= 1.0f) { phase -= 1.0f; exc = 1.0f + r.W() * 0.4f; }
        float env = sinf(PI * fminf(t * 1.2f, 1.0f)) * (0.6f + 0.4f * sinf(t * 23.0f + r.W()));
        float o = 0;
        for (auto& q : res) o += q.P(exc);
        b[s0 + i] += o * env * amp;
    }
}
// Room reverb (Freeverb-lite)
void Reverb(Buf& b, float room, float damp, float wet) {
    const int combs[4] = { 1116, 1188, 1277, 1356 }, aps[2] = { 556, 441 };
    std::vector<std::vector<float>> cb(4), ab(2);
    int ci[4] = { 0 }, ai[2] = { 0 };
    float cf[4] = { 0 };
    for (int i = 0; i < 4; i++) cb[i].assign(combs[i], 0.0f);
    for (int i = 0; i < 2; i++) ab[i].assign(aps[i], 0.0f);
    float fb = 0.7f + room * 0.28f;
    size_t tail = (size_t)(SR * (0.3f + room * 1.2f));
    b.resize(b.size() + tail, 0.0f);
    for (size_t n = 0; n < b.size(); n++) {
        float in = b[n] * 0.3f, out = 0;
        for (int i = 0; i < 4; i++) {
            float y = cb[i][ci[i]];
            cf[i] = y * (1 - damp) + cf[i] * damp;
            cb[i][ci[i]] = in + cf[i] * fb;
            ci[i] = (ci[i] + 1) % combs[i];
            out += y;
        }
        for (int i = 0; i < 2; i++) {
            float y = ab[i][ai[i]];
            ab[i][ai[i]] = out + y * 0.5f;
            out = y - out;
            ai[i] = (ai[i] + 1) % aps[i];
        }
        b[n] += out * wet;
    }
}
void Normalize(Buf& b, float peak) {
    float m = 1e-6f;
    for (float v : b) m = fmaxf(m, fabsf(v));
    float k = peak / m;
    for (float& v : b) v *= k;
    // tiny fades to avoid clicks
    int f = std::min((int)b.size() / 4, 64);
    for (int i = 0; i < f; i++) { b[i] *= (float)i / f; b[b.size() - 1 - i] *= (float)i / f; }
}
Sound ToSound(const Buf& b) {
    Wave w{};
    w.frameCount = (unsigned)b.size(); w.sampleRate = SR; w.sampleSize = 16; w.channels = 1;
    short* d = (short*)MemAlloc((unsigned)(b.size() * sizeof(short)));
    for (size_t i = 0; i < b.size(); i++) d[i] = (short)(Clamp(b[i], -1.0f, 1.0f) * 32000.0f);
    w.data = d;
    Sound s = LoadSoundFromWave(w);
    UnloadWave(w);
    return s;
}

// ---------------------------------------------------------------------------
// One-shot generators
// ---------------------------------------------------------------------------
Buf Footstep(int surf, Rnd& r, bool indoor) {
    Buf b(N(0.45f), 0.0f);
    float heel = 0.005f, toe = heel + r.R(0.05f, 0.09f);
    float k = r.R(0.8f, 1.15f);
    switch (surf) {
    case SURF_ASPHALT:
    case SURF_CONCRETE: {
        float fc = surf == SURF_CONCRETE ? 2600.0f : 1800.0f;
        Burst(b, r, heel, 0.001f, 0.012f * k, 2, fc * k, 0.8f, 0.7f);
        Thump(b, heel, 140, 70, 0.02f, 0.5f);
        Grains(b, r, heel, heel + 0.05f, surf == SURF_ASPHALT ? 260.0f : 90.0f, 3000, 8000, 0.12f, 0.002f);
        Burst(b, r, toe, 0.001f, 0.01f, 2, fc * 1.3f, 0.9f, 0.4f);
        if (r.F() < 0.35f) Burst(b, r, toe + 0.01f, 0.01f, 0.03f, 1, 2500, 0.7f, 0.15f);   // scuff
        break;
    }
    case SURF_GRAVEL:
        Thump(b, heel, 110, 55, 0.025f, 0.45f);
        Grains(b, r, heel, heel + 0.14f, 1400.0f * k, 1200, 6500, 0.3f, 0.003f);
        Grains(b, r, toe, toe + 0.09f, 700.0f, 1500, 7000, 0.22f, 0.003f);
        break;
    case SURF_GRASS:
        Thump(b, heel, 90, 50, 0.03f, 0.3f);
        Burst(b, r, heel, 0.012f, 0.06f * k, 1, 2200, 0.7f, 0.22f, 0.3f);
        Grains(b, r, heel, heel + 0.12f, 160.0f, 2500, 6000, 0.14f, 0.003f);
        Burst(b, r, toe, 0.01f, 0.05f, 1, 3000, 0.7f, 0.14f, 0.25f);
        break;
    case SURF_DIRT:
        Thump(b, heel, 100, 45, 0.035f, 0.6f);
        Burst(b, r, heel, 0.002f, 0.03f, 2, 500 * k, 0.8f, 0.45f);
        Grains(b, r, heel, heel + 0.08f, 260.0f, 900, 3500, 0.14f, 0.003f);
        Burst(b, r, toe, 0.002f, 0.02f, 2, 700, 0.8f, 0.25f);
        break;
    case SURF_WOOD:
        Burst(b, r, heel, 0.001f, 0.01f, 2, 900 * k, 1.0f, 0.4f);
        Modal(b, heel, { 105 * k, 228 * k, 405 * k, 690 * k }, { 0.09f, 0.06f, 0.04f, 0.025f }, { 0.25f, 0.18f, 0.12f, 0.06f });
        Thump(b, heel, 120, 60, 0.03f, 0.35f);
        Burst(b, r, toe, 0.001f, 0.008f, 2, 1200, 1.0f, 0.25f);
        if (r.F() < 0.25f) Creak(b, r, toe + 0.02f, 0.25f, 60, 30, { 620 * k, 1310 * k, 2050 * k }, 0.05f);
        break;
    case SURF_TILE:
        Burst(b, r, heel, 0.0005f, 0.006f, 2, 3500 * k, 1.2f, 0.8f);
        Modal(b, heel, { 2150 * k, 3380 * k, 5100 * k }, { 0.018f, 0.012f, 0.008f }, { 0.08f, 0.05f, 0.03f });
        Thump(b, heel, 150, 80, 0.015f, 0.25f);
        Burst(b, r, toe, 0.0005f, 0.005f, 2, 4200, 1.2f, 0.45f);
        break;
    case SURF_METAL:
        Burst(b, r, heel, 0.0005f, 0.008f, 2, 2000, 1.0f, 0.5f);
        Modal(b, heel, { 320 * k, 781 * k, 1453 * k, 2377 * k, 3510 * k }, { 0.25f, 0.18f, 0.12f, 0.08f, 0.05f }, { 0.12f, 0.1f, 0.08f, 0.05f, 0.03f });
        Thump(b, heel, 120, 60, 0.03f, 0.3f);
        Modal(b, toe, { 330 * k, 790 * k, 1480 * k }, { 0.15f, 0.1f, 0.07f }, { 0.06f, 0.05f, 0.03f });
        break;
    case SURF_MUD:
    case SURF_FLESH: {
        Thump(b, heel, 80, 40, 0.04f, 0.5f);
        Biquad f; int s0 = N(heel), n = N(0.18f);
        for (int i = 0; i < n; i++) {
            float t = (float)i / n;
            f.Set(2, 300 + t * (surf == SURF_FLESH ? 1400.0f : 700.0f), 4.0f);
            b[s0 + i] += f.P(r.W()) * sinf(PI * t) * 0.9f;
        }
        Thump(b, heel + 0.17f, 260, 160, 0.01f, 0.25f);   // suction pop
        if (surf == SURF_FLESH) Grains(b, r, heel, heel + 0.1f, 300, 800, 3000, 0.15f, 0.004f);
        break;
    }
    case SURF_WATER:
        Burst(b, r, heel, 0.004f, 0.05f, 1, 1500, 0.7f, 0.45f, 0.3f);
        for (int i = 0; i < 6; i++) {
            float t0 = heel + r.R(0.01f, 0.2f), f0 = r.R(500, 1400);
            int s0 = N(t0), n = N(0.03f);
            float ph = 0;
            for (int j = 0; j < n && s0 + j < (int)b.size(); j++) {
                float tt = (float)j / SR;
                ph += 2 * PI * f0 * (1 + tt * 40) / SR;
                b[s0 + j] += sinf(ph) * expf(-tt * 90) * 0.12f;
            }
        }
        break;
    case SURF_BONE:
    case SURF_GLASS:
        Thump(b, heel, 100, 50, 0.02f, 0.35f);
        Grains(b, r, heel, heel + 0.1f, 500, 2500, 8000, 0.25f, 0.002f);
        for (int i = 0; i < 5; i++) Modal(b, heel + r.R(0, 0.1f), { r.R(2500, 7000) }, { 0.03f }, { 0.05f });
        break;
    default:
        Thump(b, heel, 100, 50, 0.03f, 0.5f);
        Burst(b, r, heel, 0.001f, 0.02f, 2, 1200, 0.8f, 0.4f);
    }
    if (indoor) Reverb(b, 0.35f, 0.4f, 0.22f);
    Normalize(b, 0.9f);
    return b;
}

using Gen = std::function<Buf(Rnd&)>;

Buf DoorCreak(Rnd& r, float dur, float r0, float r1, std::vector<float> modes, float thump) {
    Buf b(N(dur + 0.6f), 0.0f);
    Creak(b, r, 0.02f, dur, r0 * r.R(0.85f, 1.2f), r1 * r.R(0.8f, 1.2f), modes, 0.5f);
    if (thump > 0) Thump(b, dur, 90, 45, 0.06f, thump);
    Reverb(b, 0.3f, 0.5f, 0.15f);
    Normalize(b, 0.85f);
    return b;
}

Buf Click(Rnd& r, float f, float body, float two) {
    Buf b(N(0.15f), 0.0f);
    Burst(b, r, 0.002f, 0.0003f, 0.002f, 2, f * r.R(0.9f, 1.1f), 1.5f, 0.9f);
    Modal(b, 0.002f, { body, body * 2.7f }, { 0.012f, 0.006f }, { 0.2f, 0.08f });
    if (two > 0) Burst(b, r, two, 0.0003f, 0.002f, 2, f * 0.8f, 1.5f, 0.5f);
    Normalize(b, 0.8f);
    return b;
}

Buf Bell(Rnd& r, float f0, float dur, float strikes, float spacing) {
    // Struck metal bell: inharmonic partials (hum, prime, tierce, quint, nominal)
    Buf b(N(dur + spacing * strikes + 0.1f), 0.0f);
    std::vector<float> ratios{ 0.5f, 1.0f, 1.183f, 1.506f, 2.0f, 2.514f, 2.662f, 3.011f };
    for (int s = 0; s < (int)strikes; s++) {
        std::vector<float> fr, dec, amp;
        for (size_t i = 0; i < ratios.size(); i++) {
            fr.push_back(f0 * ratios[i] * r.R(0.998f, 1.002f));
            dec.push_back(dur * (1.2f - i * 0.12f));
            amp.push_back(0.3f / (1 + i * 0.6f) * r.R(0.7f, 1.0f));
        }
        Modal(b, s * spacing + 0.002f, fr, dec, amp);
        Burst(b, r, s * spacing, 0.0003f, 0.002f, 2, 6000, 1, 0.2f);
    }
    Normalize(b, 0.8f);
    return b;
}

Buf PhoneRing(Rnd& r) {
    // Electromechanical ringer: clapper hits two gongs ~20 times/s for 2 s
    Buf b(N(2.4f), 0.0f);
    for (int i = 0; i < 40; i++) {
        float t = i * 0.05f;
        float f = (i % 2) ? 1840.0f : 2210.0f;
        Modal(b, t, { f, f * 1.47f, f * 2.3f }, { 0.12f, 0.07f, 0.04f }, { 0.25f, 0.12f, 0.06f });
        Burst(b, r, t, 0.0003f, 0.002f, 2, 4000, 1, 0.1f);
    }
    Normalize(b, 0.8f);
    return b;
}

Buf GlassBreak(Rnd& r, float size) {
    Buf b(N(1.6f), 0.0f);
    Burst(b, r, 0.0f, 0.001f, 0.03f, 1, 1500, 0.7f, 0.9f);
    Thump(b, 0.0f, 200, 80, 0.03f, 0.4f * size);
    for (int i = 0; i < (int)(60 * size); i++) {
        float t = r.F() * r.F() * 1.2f;
        Modal(b, t, { r.R(2200, 9000), r.R(3000, 11000) }, { r.R(0.01f, 0.05f), r.R(0.006f, 0.02f) }, { r.R(0.03f, 0.12f), r.R(0.02f, 0.06f) });
    }
    Grains(b, r, 0.0f, 0.6f, 400 * size, 3000, 9000, 0.2f, 0.002f);
    Normalize(b, 0.9f);
    return b;
}

Buf Crash(Rnd& r) {
    // Car into steel tower: impact boom, crumpling sheet metal, glass, scraping, settling
    Buf b(N(4.5f), 0.0f);
    Brown br;
    OnePole lp; lp.SetLP(300);
    for (int i = 0; i < N(0.9f); i++) {
        float t = (float)i / SR;
        b[i] += lp.LP(br.P(r.W())) * expf(-t * 4.0f) * 2.5f;
    }
    Thump(b, 0.0f, 70, 28, 0.25f, 1.0f);
    for (int k = 0; k < 45; k++) {
        float t = r.F() * r.F() * 0.9f;
        std::vector<float> fr{ r.R(180, 600), r.R(700, 1600), r.R(1800, 4000) };
        Modal(b, t, fr, { r.R(0.03f, 0.12f), r.R(0.02f, 0.08f), r.R(0.01f, 0.04f) }, { r.R(0.05f, 0.2f), r.R(0.04f, 0.12f), r.R(0.02f, 0.06f) });
    }
    Grains(b, r, 0.0f, 1.2f, 900, 800, 6000, 0.25f, 0.004f);
    Buf g = GlassBreak(r, 1.3f);
    for (size_t i = 0; i < g.size() && i + N(0.05f) < b.size(); i++) b[i + N(0.05f)] += g[i] * 0.5f;
    // settling: creaks, dripping fluid, tick of hot metal
    Creak(b, r, 1.2f, 1.2f, 30, 12, { 180, 410, 950 }, 0.15f);
    for (int i = 0; i < 8; i++) Modal(b, 2.2f + r.F() * 2.0f, { r.R(1500, 3500) }, { 0.01f }, { 0.08f });
    Reverb(b, 0.8f, 0.3f, 0.2f);
    Normalize(b, 0.98f);
    return b;
}

Buf Screech(Rnd& r, float dur) {
    Buf b(N(dur + 0.2f), 0.0f);
    Biquad f1 = Biquad::BP(r.R(1300, 1600), 7), f2 = Biquad::BP(r.R(2200, 2600), 9);
    float ph = 0;
    for (int i = 0; i < N(dur); i++) {
        float t = (float)i / SR;
        ph += 2 * PI * (40 + 15 * sinf(t * 3)) / SR;
        float am = 0.6f + 0.4f * sinf(ph);
        float env = fminf(t * 8, 1.0f) * fminf((dur - t) * 4, 1.0f);
        float n = r.W();
        b[i] = (f1.P(n) * 1.0f + f2.P(n) * 0.6f) * am * env;
    }
    Normalize(b, 0.9f);
    return b;
}

Buf Thunder(Rnd& r) {
    Buf b(N(6.0f), 0.0f);
    Brown br; OnePole lp; lp.SetLP(180);
    Burst(b, r, 0.0f, 0.002f, 0.08f, 1, 1500, 0.7f, 0.6f);
    for (int i = 0; i < N(6.0f); i++) {
        float t = (float)i / SR;
        float env = (t < 0.3f ? t / 0.3f : expf(-(t - 0.3f) * 0.7f)) * (0.7f + 0.3f * sinf(t * 5 + r.W() * 0.2f));
        b[i] += lp.LP(br.P(r.W())) * env * 2.0f;
    }
    Normalize(b, 0.95f);
    return b;
}

Buf Heartbeat(Rnd&) {
    Buf b(N(0.6f), 0.0f);
    Thump(b, 0.0f, 70, 42, 0.07f, 1.0f);
    Thump(b, 0.26f, 85, 50, 0.05f, 0.7f);
    Normalize(b, 0.95f);
    return b;
}

// Breath/whisper through formant filters
Buf Breath(Rnd& r, float dur, float f1, float f2, float f3, bool gasp) {
    Buf b(N(dur + 0.1f), 0.0f);
    Biquad a = Biquad::BP(f1, 4), c = Biquad::BP(f2, 5), d = Biquad::BP(f3, 6);
    Biquad hp = Biquad::HP(300);
    for (int i = 0; i < N(dur); i++) {
        float t = (float)i / N(dur);
        float env = gasp ? (t < 0.15f ? t / 0.15f : expf(-(t - 0.15f) * 4)) : sinf(PI * t) * sinf(PI * t);
        float n = hp.P(r.W());
        b[i] = (a.P(n) + c.P(n) * 0.7f + d.P(n) * 0.4f + n * 0.1f) * env;
    }
    Normalize(b, 0.7f);
    return b;
}

Buf Growl(Rnd& r, float dur, float pitch, float rough) {
    Buf b(N(dur + 0.2f), 0.0f);
    Biquad fa = Biquad::BP(420, 5), fb = Biquad::BP(900, 6), fc = Biquad::BP(2500, 5);
    float ph = 0;
    for (int i = 0; i < N(dur); i++) {
        float t = (float)i / SR;
        float f = pitch * (1 + 0.15f * sinf(t * 7 + r.W() * 0.3f)) * (1 + r.W() * rough * 0.3f);
        ph += f / SR;
        float pulse = 0;
        if (ph >= 1) { ph -= 1; pulse = 1; }
        float src = pulse * 3.0f + r.W() * rough * 0.3f;
        float env = fminf(t * 10, 1.0f) * fminf((dur - t) * 3, 1.0f) * (0.7f + 0.3f * sinf(t * 11));
        float o = fa.P(src) + fb.P(src) * 0.7f + fc.P(src) * 0.3f;
        b[i] = tanhf(o * 3.0f) * env;
    }
    Normalize(b, 0.9f);
    return b;
}

Buf Scrape(Rnd& r, float dur, float fc) {
    Buf b(N(dur + 0.1f), 0.0f);
    Biquad f = Biquad::BP(fc, 1.2f);
    float am = 0, tgt = 1;
    for (int i = 0; i < N(dur); i++) {
        if ((i % 800) == 0) tgt = r.R(0.2f, 1.0f);
        am += (tgt - am) * 0.002f;
        float t = (float)i / N(dur);
        b[i] = f.P(r.W()) * am * sinf(PI * t);
    }
    Normalize(b, 0.8f);
    return b;
}

Buf Whoosh(Rnd& r, float dur, float f0, float f1) {
    Buf b(N(dur + 0.2f), 0.0f);
    Biquad f;
    for (int i = 0; i < N(dur); i++) {
        float t = (float)i / N(dur);
        if ((i & 31) == 0) f.Set(2, f0 + (f1 - f0) * t, 1.2f);
        b[i] = f.P(r.W()) * sinf(PI * powf(t, 0.6f));
    }
    Normalize(b, 0.9f);
    return b;
}

Buf Crackle(Rnd& r, float dur, float rate) {
    Buf b(N(dur), 0.0f);
    Grains(b, r, 0, dur, rate, 1500, 8000, 0.8f, 0.0015f);
    Normalize(b, 0.8f);
    return b;
}

Buf Engine(Rnd& r) {  // car engine start
    Buf b(N(1.6f), 0.0f);
    float ph = 0;
    OnePole lp; lp.SetLP(400);
    for (int i = 0; i < N(1.6f); i++) {
        float t = (float)i / SR;
        float rpm = t < 0.6f ? 180 + 60 * sinf(t * 40) : 900 + 800 * expf(-(t - 0.6f) * 3);
        ph += rpm / 60.0f * 2 / SR;
        float pulse = expf(-fmodf(ph, 1.0f) * 12.0f);
        b[i] = lp.LP(pulse + r.W() * 0.2f) * fminf(t * 4, 1.0f);
    }
    Normalize(b, 0.8f);
    return b;
}

struct Bank {
    std::vector<Sound> variants;
    std::vector<std::vector<Sound>> aliases;
    int last = -1;
    int rr = 0;
};

std::map<std::string, Bank> g_bank;
std::mutex g_bankMutex;
Vector3 g_listenerPos{}, g_listenerFwd{ 0, 0, 1 };
float g_master = 1.0f;
AmbienceParams g_amb;
AudioStream g_stream{};
bool g_ready = false;

void AddVariants(const std::string& id, int count, const Gen& gen, uint32_t seed) {
    Bank bank;
    // file overrides: assets/sounds/<id>_<n>.wav
    for (int i = 0; i < 16; i++) {
        const char* p = TextFormat("assets/sounds/%s_%d.wav", id.c_str(), i);
        if (FileExists(p)) bank.variants.push_back(LoadSound(p));
    }
    if (bank.variants.empty()) {
        Rnd r(seed * 2654435761u + 17);
        for (int i = 0; i < count; i++) bank.variants.push_back(ToSound(gen(r)));
    }
    for (auto& s : bank.variants) {
        std::vector<Sound> al;
        for (int k = 0; k < 3; k++) al.push_back(LoadSoundAlias(s));
        bank.aliases.push_back(al);
    }
    g_bank[id] = std::move(bank);
}

// ---------------------------------------------------------------------------
// Live ambience mixer (audio thread)
// ---------------------------------------------------------------------------
struct Cricket { float f, pan, amp, timer, pulseT; int pulses; bool on; float ph; };

struct Mixer {
    Rnd r{ 991 };
    Pink pinkL, pinkR; Brown brownL, brownR, brownRoad, brownFire, brownDrone;
    Biquad windL, windR, whistle, treesHP, rainHP, rainBP, humLP, fridgeLP, engLP, intake, roadLP, cabinLP;
    Biquad scr1, scr2, pumpLP, flowLP, pourBP, fireLP, fireHP, dragBP, droneLP, whisperF[3];
    Biquad outLPL, outLPR, mufL, mufR;
    float gustEnv = 0.5f, gustTarget = 0.5f, gustTimer = 0;
    float windCf = 400;
    Cricket crickets[8];
    float cricketGain = 1.0f;
    double t = 0;
    float humPh = 0, engPh = 0, engPulse = 0, scrPh = 0;
    float heartT = 0, breathT = 0;
    float dripTimer = 1.0f;
    struct Voice { float f, t, dur, amp, pan; bool on; } drips[6]{}, bubbles[6]{};
    float whisperTimer = 0, whisperPan = 0.5f, whisperAmpT = 0, whisperAmp = 0;
    float formT[3] = { 500, 1500, 2500 }, form[3] = { 500, 1500, 2500 };
    int blockCount = 0;
    std::vector<float> heartBuf, breathIn, breathOut;
    int heartPos = -1, breathPos = -1; bool breathIsIn = true;
    float indoorS = 0, muffleS = 0;

    void Init() {
        windL = Biquad::BP(400, 0.6f); windR = Biquad::BP(420, 0.6f);
        whistle = Biquad::BP(900, 25);
        treesHP = Biquad::HP(2500); rainHP = Biquad::HP(600); rainBP = Biquad::BP(3500, 1.5f);
        humLP = Biquad::LP(800); fridgeLP = Biquad::LP(180); engLP = Biquad::LP(500); intake = Biquad::BP(1200, 2);
        roadLP = Biquad::LP(350); cabinLP = Biquad::LP(250);
        scr1 = Biquad::BP(1450, 7); scr2 = Biquad::BP(2350, 9);
        pumpLP = Biquad::LP(700); flowLP = Biquad::LP(900); pourBP = Biquad::BP(1100, 1.2f);
        fireLP = Biquad::LP(250); fireHP = Biquad::HP(3000); dragBP = Biquad::BP(750, 1.3f); droneLP = Biquad::LP(70);
        for (int i = 0; i < 3; i++) whisperF[i] = Biquad::BP(form[i], 8);
        outLPL = Biquad::LP(18000); outLPR = Biquad::LP(18000); mufL = Biquad::LP(18000); mufR = Biquad::LP(18000);
        for (auto& c : crickets) { c.f = r.R(4100, 5300); c.pan = r.F(); c.amp = r.R(0.2f, 1.0f); c.timer = r.R(0, 2); c.pulses = 0; c.on = false; c.ph = 0; c.pulseT = 0; }
        Rnd rr(7);
        heartBuf = Heartbeat(rr);
        breathIn = Breath(rr, 1.1f, 650, 1300, 2600, false);
        breathOut = Breath(rr, 1.3f, 500, 1100, 2400, false);
    }

    void Process(float* out, unsigned frames) {
        const float master = g_amb.master.load() * g_master;
        const float wind = g_amb.wind, gust = g_amb.gust, trees = g_amb.trees, crick = g_amb.crickets, threat = g_amb.threat;
        const float hum = g_amb.hum, fridge = g_amb.fridge, ballast = g_amb.ballast, rain = g_amb.rain;
        const float engine = g_amb.engine, rpm = g_amb.rpm, road = g_amb.road, cabin = g_amb.cabin, screech = g_amb.screech;
        const float pump = g_amb.pumpMotor, flow = g_amb.fuelFlow, pour = g_amb.pour, fire = g_amb.fire, drag = g_amb.drag;
        const float drone = g_amb.drone, dripsL = g_amb.drips, tinn = g_amb.tinnitus, heart = g_amb.heart, heartRate = g_amb.heartRate;
        const float breath = g_amb.breath, breathRate = g_amb.breathRate, whisp = g_amb.whispers;
        const float indoorT = g_amb.indoor, muffleT = g_amb.muffle;
        const float dt = 1.0f / SR;

        // per-block control updates
        gustTimer -= frames * dt;
        if (gustTimer <= 0) { gustTarget = r.R(0.15f, 1.0f); gustTimer = r.R(1.0f, 4.5f); }
        if ((blockCount++ & 3) == 0) {
            windCf = 180 + gustEnv * 520;
            windL.Set(2, windCf, 0.6f); windR.Set(2, windCf * 1.07f, 0.6f);
            whistle.Set(2, 780 + gustEnv * 420, 30);
            float in = 18000 - indoorS * 17000;
            outLPL.Set(0, in, 0.707f); outLPR.Set(0, in, 0.707f);
            float mf = 18000 - muffleS * 17600;
            mufL.Set(0, mf, 0.707f); mufR.Set(0, mf, 0.707f);
            float rr = fmaxf(rpm, 300.0f);
            engLP.Set(0, 250 + rr * 0.15f, 1.2f);
        }
        indoorS += (indoorT - indoorS) * fminf(frames * dt * 3.0f, 1.0f);
        muffleS += (muffleT - muffleS) * fminf(frames * dt * 2.0f, 1.0f);
        float cgT = crick * (1.0f - Saturate(threat * 1.5f));
        cricketGain += (cgT - cricketGain) * fminf(frames * dt * (cgT < cricketGain ? 1.5f : 0.15f), 1.0f);

        for (unsigned i = 0; i < frames; i++) {
            t += dt;
            gustEnv += (gustTarget * gust + (1 - gust) * 0.5f - gustEnv) * 0.00003f;
            float L = 0, R = 0, outL = 0, outR = 0;
            // ---- outdoor beds (muffled indoors)
            if (wind > 0.001f) {
                float nl = brownL.P(r.W()), nr = brownR.P(r.W());
                float w = (0.4f + gustEnv) * wind;
                outL += windL.P(nl) * w * 0.9f; outR += windR.P(nr) * w * 0.9f;
                float ws = whistle.P(pinkL.P(r.W())) * wind * gustEnv * gustEnv * 0.25f;
                outL += ws; outR += ws * 0.7f;
                if (trees > 0.001f) {
                    float h = treesHP.P(r.W()) * trees * gustEnv * gustEnv * 0.18f;
                    outL += h; outR += h * 0.8f + treesHP.P(r.W()) * 0.0f;
                }
            }
            if (cricketGain > 0.002f) {
                for (auto& c : crickets) {
                    if (!c.on) {
                        c.timer -= dt;
                        if (c.timer <= 0) { c.on = true; c.pulses = 3 + (r.U() % 3); c.pulseT = 0; }
                        continue;
                    }
                    c.pulseT += dt;
                    float period = 0.03f, pl = 0.013f;
                    float within = fmodf(c.pulseT, period);
                    if (c.pulseT > period * c.pulses) { c.on = false; c.timer = r.R(0.25f, 1.3f); continue; }
                    float env = within < pl ? sinf(PI * within / pl) : 0.0f;
                    c.ph += 2 * PI * c.f * dt;
                    float s = sinf(c.ph) * env * c.amp * cricketGain * 0.035f;
                    outL += s * c.pan; outR += s * (1 - c.pan);
                }
            }
            if (rain > 0.001f) {
                float n = rainHP.P(pinkR.P(r.W())) * rain * 0.35f;
                float drop = (r.F() < rain * 0.004f) ? rainBP.P(r.W() * 6.0f) : rainBP.P(0);
                outL += n + drop * 0.2f; outR += n * 0.9f + drop * 0.15f;
            }
            outL = outLPL.P(outL); outR = outLPR.P(outR);
            L += outL; R += outR;

            // ---- local machines
            if (hum > 0.001f || ballast > 0.001f) {
                humPh += 60.0f * dt; if (humPh > 1) humPh -= 1;
                float hh = sinf(2 * PI * humPh * 2) * 0.6f + sinf(2 * PI * humPh * 4) * 0.25f + sinf(2 * PI * humPh * 6) * 0.1f;
                float buzz = tanhf(hh * 3.0f) * 0.25f + humLP.P(r.W()) * 0.05f;
                float tick = (r.F() < ballast * 0.0006f) ? r.W() * 0.6f : 0.0f;
                float s = buzz * hum * 0.2f + tick;
                L += s; R += s;
            }
            if (fridge > 0.001f) {
                float s = fridgeLP.P(r.W()) * 0.5f + sinf((float)t * 2 * PI * 50) * 0.04f + sinf((float)t * 2 * PI * 100) * 0.03f;
                s *= fridge * 0.25f * (0.9f + 0.1f * sinf((float)t * 0.7f));
                L += s; R += s;
            }
            if (engine > 0.001f || road > 0.001f || cabin > 0.001f) {
                float fire = fmaxf(rpm, 300.0f) / 60.0f * 2.0f;
                engPh += fire * dt;
                if (engPh >= 1) { engPh -= 1; engPulse = 1.0f + r.W() * 0.1f; }
                engPulse *= 0.9985f;
                float e = engLP.P(engPulse * 2.0f - 0.5f + r.W() * 0.08f) + intake.P(r.W()) * 0.05f * (rpm / 3000.0f);
                float rd = roadLP.P(brownRoad.P(r.W())) * road * 0.8f;
                float cb = cabinLP.P(r.W()) * cabin * 0.15f;
                float s = e * engine * 0.35f + rd + cb;
                L += s; R += s;
            }
            if (screech > 0.001f) {
                scrPh += 43.0f * dt;
                float am = 0.6f + 0.4f * sinf(2 * PI * scrPh);
                float n = r.W();
                float s = (scr1.P(n) + scr2.P(n) * 0.6f) * am * screech * 0.9f;
                L += s; R += s;
            }
            if (pump > 0.001f) {
                float s = (sinf((float)t * 2 * PI * 118) * 0.3f + sinf((float)t * 2 * PI * 236) * 0.15f + pumpLP.P(r.W()) * 0.2f) * pump * 0.3f;
                L += s; R += s;
            }
            if (flow > 0.001f) {
                float s = flowLP.P(r.W()) * flow * 0.35f * (0.8f + 0.2f * sinf((float)t * 13 + r.W()));
                L += s; R += s;
            }
            if (pour > 0.001f) {
                float s = pourBP.P(r.W()) * pour * 0.4f * (0.6f + 0.4f * fabsf(sinf((float)t * 9)));
                for (auto& v : bubbles) {
                    if (!v.on) { if (r.F() < pour * 0.0004f) { v = { r.R(300, 900), 0, r.R(0.02f, 0.05f), r.R(0.1f, 0.3f), 0.5f, true }; } continue; }
                    v.t += dt; if (v.t > v.dur) { v.on = false; continue; }
                    s += sinf(2 * PI * v.f * (1 + v.t * 30) * v.t) * expf(-v.t / v.dur * 3) * v.amp * pour;
                }
                L += s; R += s;
            }
            if (fire > 0.001f) {
                float roar = fireLP.P(brownFire.P(r.W())) * fire * 0.9f;
                float cr = (r.F() < fire * 0.003f) ? fireHP.P(r.W() * 4.0f) : fireHP.P(0);
                L += roar + cr * 0.4f; R += roar * 0.95f + cr * 0.3f;
            }
            if (drag > 0.001f) {
                float s = dragBP.P(r.W()) * drag * 0.6f * (0.5f + 0.5f * fabsf(sinf((float)t * 3.1f + sinf((float)t * 7.3f))));
                L += s; R += s;
            }
            if (drone > 0.001f) {
                float s = droneLP.P(brownDrone.P(r.W())) * drone * 1.2f;
                L += s; R += s;
            }
            if (dripsL > 0.001f) {
                dripTimer -= dt;
                if (dripTimer <= 0) {
                    dripTimer = r.R(0.4f, 2.5f) / fmaxf(dripsL, 0.2f);
                    for (auto& v : drips) if (!v.on) { v = { r.R(900, 1700), 0, 0.06f, r.R(0.05f, 0.15f), r.F(), true }; break; }
                }
                for (auto& v : drips) {
                    if (!v.on) continue;
                    v.t += dt; if (v.t > v.dur * 4) { v.on = false; continue; }
                    float f = v.f * (1 - v.t * 3);
                    float s = sinf(2 * PI * f * v.t) * expf(-v.t / v.dur * 3) * v.amp * dripsL;
                    L += s * v.pan; R += s * (1 - v.pan);
                }
            }
            if (whisp > 0.001f) {
                whisperTimer -= dt;
                if (whisperTimer <= 0) {
                    whisperTimer = r.R(0.06f, 0.14f);
                    formT[0] = r.R(300, 800); formT[1] = r.R(900, 2200); formT[2] = r.R(2300, 3200);
                    whisperAmpT = r.F() < 0.25f ? 0.0f : r.R(0.3f, 1.0f);
                    whisperPan += r.W() * 0.08f; whisperPan = Saturate(whisperPan);
                }
                if ((i & 63) == 0) for (int k = 0; k < 3; k++) { form[k] += (formT[k] - form[k]) * 0.2f; whisperF[k].Set(2, form[k], 9); }
                whisperAmp += (whisperAmpT - whisperAmp) * 0.002f;
                float n = r.W();
                float s = (whisperF[0].P(n) + whisperF[1].P(n) * 0.8f + whisperF[2].P(n) * 0.5f) * whisperAmp * whisp * 0.5f;
                L += s * whisperPan; R += s * (1 - whisperPan);
            }
            if (tinn > 0.001f) {
                float s = (sinf((float)t * 2 * PI * 6300) + sinf((float)t * 2 * PI * 6337) * 0.6f) * tinn * 0.03f;
                L += s; R += s;
            }
            if (heart > 0.001f) {
                heartT += dt;
                float period = 60.0f / fmaxf(heartRate, 20.0f);
                if (heartT >= period) { heartT -= period; heartPos = 0; }
                if (heartPos >= 0) {
                    float s = heartBuf[heartPos++] * heart * 0.7f;
                    L += s; R += s;
                    if (heartPos >= (int)heartBuf.size()) heartPos = -1;
                }
            }
            if (breath > 0.001f) {
                breathT += dt;
                float period = 60.0f / fmaxf(breathRate, 4.0f);
                if (breathT >= period * 0.5f) { breathT -= period * 0.5f; breathPos = 0; breathIsIn = !breathIsIn; }
                const std::vector<float>& bb = breathIsIn ? breathIn : breathOut;
                if (breathPos >= 0) {
                    float stretch = fminf(1.0f, (period * 0.5f) / (bb.size() / (float)SR));
                    int idx = (int)(breathPos * (1.0f / fmaxf(stretch, 0.3f)));
                    breathPos++;
                    if (idx < (int)bb.size()) { float s = bb[idx] * breath * 0.25f; L += s; R += s; }
                    else breathPos = -1;
                }
            }
            L = mufL.P(L); R = mufR.P(R);
            out[i * 2] = tanhf(L * master * 1.2f) * 0.9f;
            out[i * 2 + 1] = tanhf(R * master * 1.2f) * 0.9f;
        }
    }
};

Mixer g_mixer;
void MixerCallback(void* buffer, unsigned int frames) { g_mixer.Process((float*)buffer, frames); }

void BuildBank() {
    // Footsteps: 8 variants per surface, outdoor and indoor versions
    for (int s = 1; s < SURF_COUNT; s++) {
        std::string base = std::string("step_") + SurfaceName(s);
        AddVariants(base, 8, [s](Rnd& r) { return Footstep(s, r, false); }, 100 + s);
        AddVariants(base + "_in", 6, [s](Rnd& r) { return Footstep(s, r, true); }, 200 + s);
    }
    AddVariants("door_wood_open", 3, [](Rnd& r) { return DoorCreak(r, 1.1f, 45, 20, { 540, 1210, 1890, 2600 }, 0.0f); }, 1);
    AddVariants("door_wood_close", 3, [](Rnd& r) { Buf b = DoorCreak(r, 0.5f, 60, 30, { 560, 1250, 1900 }, 0.9f); return b; }, 2);
    AddVariants("door_metal_open", 2, [](Rnd& r) { return DoorCreak(r, 0.9f, 35, 15, { 320, 760, 1420, 2210 }, 0.0f); }, 3);
    AddVariants("door_metal_close", 2, [](Rnd& r) {
        Buf b(N(1.2f), 0.0f);
        Thump(b, 0, 110, 50, 0.08f, 1.0f);
        Modal(b, 0, { 180, 430, 910, 1640 }, { 0.35f, 0.25f, 0.15f, 0.08f }, { 0.3f, 0.2f, 0.12f, 0.06f });
        Burst(b, r, 0.02f, 0.001f, 0.01f, 2, 2500, 1, 0.3f);
        Reverb(b, 0.5f, 0.4f, 0.25f); Normalize(b, 0.9f); return b; }, 4);
    AddVariants("door_glass_open", 2, [](Rnd& r) {
        Buf b = Whoosh(r, 0.5f, 400, 1200);
        for (float& v : b) v *= 0.25f;
        Buf bell = Bell(r, 2350, 0.6f, 5, 0.07f);   // shop bell on a spring
        b.resize(std::max(b.size(), bell.size()), 0.0f);
        for (size_t i = 0; i < bell.size(); i++) b[i] += bell[i] * 0.6f;
        Normalize(b, 0.85f); return b; }, 5);
    AddVariants("door_latch", 3, [](Rnd& r) { return Click(r, 2800, 420, 0.04f); }, 6);
    AddVariants("door_locked", 3, [](Rnd& r) {
        Buf b(N(0.5f), 0.0f);
        for (int k = 0; k < 3; k++) { Burst(b, r, k * 0.09f, 0.0005f, 0.004f, 2, 2200, 1.2f, 0.6f); Modal(b, k * 0.09f, { 380, 910 }, { 0.03f, 0.02f }, { 0.2f, 0.1f }); }
        Normalize(b, 0.8f); return b; }, 7);
    AddVariants("iron_door", 1, [](Rnd& r) {
        Buf b(N(4.0f), 0.0f);
        Creak(b, r, 0.1f, 3.0f, 18, 9, { 120, 260, 470, 830 }, 0.6f);
        Thump(b, 3.1f, 60, 30, 0.4f, 1.0f);
        Reverb(b, 0.95f, 0.2f, 0.45f); Normalize(b, 0.95f); return b; }, 8);
    AddVariants("switch", 3, [](Rnd& r) { return Click(r, 3500, 900, 0.0f); }, 9);
    AddVariants("flashlight", 2, [](Rnd& r) { return Click(r, 4200, 1300, 0.012f); }, 10);
    AddVariants("breaker", 2, [](Rnd& r) { Buf b = Click(r, 1800, 300, 0.0f); Thump(b, 0, 150, 70, 0.03f, 0.5f); Normalize(b, 0.9f); return b; }, 11);
    AddVariants("nozzle_lift", 2, [](Rnd& r) { Buf b(N(0.5f), 0.0f); Modal(b, 0.01f, { 520, 1310, 2400 }, { 0.12f, 0.07f, 0.04f }, { 0.3f, 0.2f, 0.1f }); Burst(b, r, 0, 0.001f, 0.01f, 2, 1800, 1, 0.4f); Normalize(b, 0.85f); return b; }, 12);
    AddVariants("nozzle_hang", 2, [](Rnd& r) { Buf b(N(0.6f), 0.0f); Modal(b, 0.0f, { 480, 1220, 2300 }, { 0.15f, 0.08f, 0.05f }, { 0.35f, 0.2f, 0.1f }); Thump(b, 0, 180, 90, 0.02f, 0.4f); Burst(b, r, 0, 0.001f, 0.01f, 2, 1500, 1, 0.3f); Normalize(b, 0.9f); return b; }, 13);
    AddVariants("pump_shutoff", 1, [](Rnd& r) { return Click(r, 1500, 250, 0.03f); }, 14);
    AddVariants("register_key", 4, [](Rnd& r) { return Click(r, 2400, 600, 0.0f); }, 15);
    AddVariants("register_drawer", 1, [](Rnd& r) {
        Buf b(N(1.2f), 0.0f);
        Buf bell = Bell(r, 1650, 0.9f, 1, 0);
        for (size_t i = 0; i < bell.size(); i++) b[i] += bell[i] * 0.5f;
        Buf sc = Scrape(r, 0.3f, 1400);
        for (size_t i = 0; i < sc.size(); i++) b[i + N(0.05f)] += sc[i] * 0.4f;
        Thump(b, 0.33f, 140, 70, 0.04f, 0.6f);
        for (int k = 0; k < 6; k++) Modal(b, 0.35f + r.F() * 0.15f, { r.R(3000, 6000) }, { 0.04f }, { 0.08f });
        Normalize(b, 0.85f); return b; }, 16);
    AddVariants("receipt", 1, [](Rnd& r) {
        Buf b(N(1.4f), 0.0f);
        for (int k = 0; k < 40; k++) Burst(b, r, k * 0.03f, 0.001f, 0.01f, 2, 3000, 2, 0.3f);
        Burst(b, r, 1.25f, 0.002f, 0.02f, 1, 3000, 0.7f, 0.5f);
        Normalize(b, 0.7f); return b; }, 17);
    AddVariants("coins", 3, [](Rnd& r) { Buf b(N(0.7f), 0.0f); for (int k = 0; k < 9; k++) Modal(b, r.F() * 0.35f, { r.R(2500, 5500), r.R(6000, 9000) }, { 0.05f, 0.03f }, { 0.15f, 0.08f }); Normalize(b, 0.8f); return b; }, 18);
    AddVariants("counter_bell", 1, [](Rnd& r) { return Bell(r, 2640, 1.6f, 1, 0); }, 19);
    AddVariants("phone_ring", 1, PhoneRing, 20);
    AddVariants("phone_pickup", 2, [](Rnd& r) { Buf b = Click(r, 1200, 260, 0.05f); Buf bell = Bell(r, 2210, 0.08f, 1, 0); for (size_t i = 0; i < bell.size() && i < b.size(); i++) b[i] += bell[i] * 0.2f; Normalize(b, 0.8f); return b; }, 21);
    AddVariants("glass_break", 3, [](Rnd& r) { return GlassBreak(r, 1.0f); }, 22);
    AddVariants("crash", 1, Crash, 23);
    AddVariants("screech", 1, [](Rnd& r) { return Screech(r, 2.4f); }, 24);
    AddVariants("thunder", 3, Thunder, 25);
    AddVariants("heartbeat", 1, Heartbeat, 26);
    AddVariants("gasp", 3, [](Rnd& r) { return Breath(r, 0.6f, 700, 1250, 2700, true); }, 27);
    AddVariants("breath_out", 3, [](Rnd& r) { return Breath(r, 1.1f, 500, 1100, 2400, false); }, 28);
    AddVariants("whisper", 4, [](Rnd& r) {
        Buf b(N(1.8f), 0.0f);
        for (int syl = 0; syl < 7; syl++) {
            Buf s = Breath(r, r.R(0.12f, 0.25f), r.R(300, 800), r.R(900, 2200), r.R(2300, 3200), false);
            size_t off = N(syl * 0.22f + r.R(0, 0.05f));
            for (size_t i = 0; i < s.size() && off + i < b.size(); i++) b[off + i] += s[i];
        }
        Normalize(b, 0.7f); return b; }, 29);
    AddVariants("growl", 3, [](Rnd& r) { return Growl(r, r.R(1.0f, 1.8f), r.R(38, 60), 0.6f); }, 30);
    AddVariants("screech_creature", 3, [](Rnd& r) { return Growl(r, r.R(0.8f, 1.3f), r.R(180, 260), 1.0f); }, 31);
    AddVariants("bone_crack", 3, [](Rnd& r) { Buf b(N(0.4f), 0.0f); Burst(b, r, 0, 0.0003f, 0.004f, 2, 2500, 0.8f, 1.0f); Grains(b, r, 0.005f, 0.08f, 400, 1500, 6000, 0.3f, 0.002f); Thump(b, 0, 180, 90, 0.02f, 0.4f); Normalize(b, 0.9f); return b; }, 32);
    AddVariants("squelch", 3, [](Rnd& r) { return Footstep(SURF_FLESH, r, false); }, 33);
    AddVariants("dig", 4, [](Rnd& r) {
        Buf b(N(0.8f), 0.0f);
        Burst(b, r, 0.0f, 0.001f, 0.01f, 2, 2500, 1, 0.5f);
        Modal(b, 0, { 900, 2100 }, { 0.05f, 0.03f }, { 0.1f, 0.05f });
        Grains(b, r, 0.01f, 0.25f, 900, 700, 4000, 0.3f, 0.004f);
        Thump(b, 0.02f, 90, 40, 0.05f, 0.6f);
        Grains(b, r, 0.4f, 0.7f, 500, 500, 3000, 0.2f, 0.004f);   // soil tossed
        Normalize(b, 0.9f); return b; }, 34);
    AddVariants("paper", 3, [](Rnd& r) { Buf b(N(0.6f), 0.0f); Grains(b, r, 0, 0.5f, 700, 2000, 9000, 0.4f, 0.003f); Burst(b, r, 0, 0.05f, 0.1f, 1, 3000, 0.7f, 0.2f); Normalize(b, 0.6f); return b; }, 35);
    AddVariants("pickup", 3, [](Rnd& r) { Buf b(N(0.4f), 0.0f); Burst(b, r, 0, 0.02f, 0.05f, 2, 1800, 0.8f, 0.4f); Grains(b, r, 0, 0.15f, 200, 2000, 6000, 0.2f, 0.002f); Normalize(b, 0.6f); return b; }, 36);
    AddVariants("drop_box", 3, [](Rnd& r) { Buf b(N(0.5f), 0.0f); Thump(b, 0, 120, 55, 0.05f, 0.8f); Burst(b, r, 0, 0.001f, 0.03f, 2, 700, 0.8f, 0.5f); Modal(b, 0, { 210, 380 }, { 0.05f, 0.03f }, { 0.15f, 0.08f }); Normalize(b, 0.9f); return b; }, 37);
    AddVariants("keys", 2, [](Rnd& r) { Buf b(N(0.8f), 0.0f); for (int k = 0; k < 12; k++) Modal(b, r.F() * 0.4f, { r.R(3000, 6500), r.R(7000, 10000) }, { 0.06f, 0.03f }, { 0.1f, 0.05f }); Normalize(b, 0.7f); return b; }, 38);
    AddVariants("unlock", 1, [](Rnd& r) { Buf b(N(0.8f), 0.0f); Burst(b, r, 0, 0.001f, 0.01f, 2, 2500, 1, 0.3f); Creak(b, r, 0.05f, 0.25f, 120, 80, { 900, 2100 }, 0.2f); Burst(b, r, 0.32f, 0.0005f, 0.006f, 2, 1800, 1.2f, 1.0f); Thump(b, 0.32f, 200, 90, 0.02f, 0.5f); Normalize(b, 0.9f); return b; }, 39);
    AddVariants("fire_ignite", 2, [](Rnd& r) { Buf b = Whoosh(r, 1.4f, 150, 900); Buf c = Crackle(r, 1.4f, 60); for (size_t i = 0; i < c.size() && i < b.size(); i++) b[i] += c[i] * 0.3f; Normalize(b, 0.95f); return b; }, 40);
    AddVariants("match", 2, [](Rnd& r) { Buf b(N(0.8f), 0.0f); Burst(b, r, 0, 0.005f, 0.04f, 1, 2500, 0.7f, 0.8f); Buf w = Whoosh(r, 0.5f, 300, 1500); for (size_t i = 0; i < w.size() && i + N(0.05f) < b.size(); i++) b[i + N(0.05f)] += w[i] * 0.3f; Normalize(b, 0.8f); return b; }, 41);
    AddVariants("fuel_splash", 3, [](Rnd& r) { return Footstep(SURF_WATER, r, false); }, 42);
    AddVariants("ui_hover", 4, [](Rnd& r) { Buf b = Click(r, 5200, 1700, 0.0f); for (float& v : b) v *= 0.5f; return b; }, 43);
    AddVariants("ui_select", 3, [](Rnd& r) { Buf b(N(0.35f), 0.0f); Burst(b, r, 0, 0.0004f, 0.004f, 2, 2800, 1.2f, 0.8f); Thump(b, 0.004f, 160, 80, 0.03f, 0.6f); Modal(b, 0.004f, { 620, 1450 }, { 0.03f, 0.015f }, { 0.12f, 0.05f }); Burst(b, r, 0.06f, 0.0004f, 0.003f, 2, 3500, 1.2f, 0.35f); Normalize(b, 0.85f); return b; }, 44);
    AddVariants("ui_back", 2, [](Rnd& r) { Buf b = Click(r, 2200, 700, 0.035f); return b; }, 45);
    AddVariants("horn", 1, [](Rnd& r) {
        Buf b(N(0.6f), 0.0f);
        for (int i = 0; i < N(0.5f); i++) { float t = (float)i / SR; float s = tanhf((sinf(2 * PI * 420 * t) + sinf(2 * PI * 500 * t)) * 2.5f); b[i] = s * fminf(t * 60, 1.0f) * fminf((0.5f - t) * 40, 1.0f); }
        Biquad lp = Biquad::LP(2500); for (float& v : b) v = lp.P(v) + r.W() * 0.01f;
        Normalize(b, 0.7f); return b; }, 46);
    AddVariants("indicator", 2, [](Rnd& r) { return Click(r, 1600, 500, 0.0f); }, 47);
    AddVariants("seatbelt", 1, [](Rnd& r) { Buf b = Scrape(r, 0.4f, 2500); b.resize(N(0.6f), 0.0f); Buf c = Click(r, 3000, 800, 0.0f); for (size_t i = 0; i < c.size(); i++) if (i + N(0.38f) < b.size()) b[i + N(0.38f)] += c[i]; Normalize(b, 0.8f); return b; }, 48);
    AddVariants("car_door", 1, [](Rnd& r) { Buf b(N(1.0f), 0.0f); Thump(b, 0, 90, 40, 0.08f, 1.0f); Modal(b, 0, { 160, 390, 820 }, { 0.12f, 0.08f, 0.05f }, { 0.2f, 0.12f, 0.06f }); Burst(b, r, 0, 0.001f, 0.015f, 2, 1800, 1, 0.3f); Normalize(b, 0.9f); return b; }, 49);
    AddVariants("engine_start", 1, Engine, 50);
    AddVariants("body_thud", 3, [](Rnd& r) { Buf b(N(0.7f), 0.0f); Thump(b, 0, 90, 38, 0.09f, 1.0f); Burst(b, r, 0, 0.002f, 0.04f, 2, 400, 0.7f, 0.6f); Grains(b, r, 0.02f, 0.2f, 300, 600, 3000, 0.15f, 0.003f); Normalize(b, 0.95f); return b; }, 51);
    AddVariants("metal_hit", 3, [](Rnd& r) { Buf b(N(1.2f), 0.0f); Burst(b, r, 0, 0.0005f, 0.006f, 2, 2500, 1, 0.6f); Modal(b, 0, { r.R(200, 300), r.R(600, 800), r.R(1300, 1600), r.R(2200, 2600) }, { 0.4f, 0.3f, 0.2f, 0.1f }, { 0.3f, 0.2f, 0.12f, 0.06f }); Normalize(b, 0.9f); return b; }, 52);
    AddVariants("drip", 4, [](Rnd& r) { Buf b(N(0.4f), 0.0f); int n = N(0.08f); float f0 = r.R(900, 1600); for (int i = 0; i < n; i++) { float t = (float)i / SR; b[i] = sinf(2 * PI * f0 * (1 - t * 3) * t) * expf(-t * 40); } Reverb(b, 0.6f, 0.3f, 0.3f); Normalize(b, 0.6f); return b; }, 53);
    AddVariants("twig", 3, [](Rnd& r) { Buf b(N(0.3f), 0.0f); Burst(b, r, 0, 0.0003f, 0.003f, 2, 3000, 1, 0.9f); Grains(b, r, 0.002f, 0.04f, 500, 2000, 7000, 0.3f, 0.0015f); Normalize(b, 0.8f); return b; }, 54);
    AddVariants("rattle_bones", 3, [](Rnd& r) { Buf b(N(0.9f), 0.0f); for (int k = 0; k < 14; k++) { float t = r.F() * 0.6f; Burst(b, r, t, 0.0004f, 0.004f, 2, r.R(1200, 3000), 1.5f, 0.5f); Modal(b, t, { r.R(700, 1400) }, { 0.02f }, { 0.12f }); } Normalize(b, 0.85f); return b; }, 55);
    AddVariants("lighter", 2, [](Rnd& r) { Buf b(N(0.6f), 0.0f); Grains(b, r, 0, 0.05f, 2000, 2000, 7000, 0.4f, 0.001f); Burst(b, r, 0.05f, 0.01f, 0.1f, 2, 900, 0.7f, 0.25f); Normalize(b, 0.7f); return b; }, 56);
    AddVariants("slam", 2, [](Rnd& r) { Buf b(N(1.5f), 0.0f); Thump(b, 0, 80, 35, 0.15f, 1.0f); Burst(b, r, 0, 0.001f, 0.05f, 2, 500, 0.7f, 0.8f); Reverb(b, 0.7f, 0.3f, 0.35f); Normalize(b, 0.95f); return b; }, 57);
    AddVariants("drag_short", 2, [](Rnd& r) { return Scrape(r, 1.4f, 700); }, 58);
    AddVariants("stinger_low", 2, [](Rnd& r) {   // a physical boom, not a musical sting
        Buf b(N(3.0f), 0.0f); Thump(b, 0, 60, 25, 0.6f, 1.0f); Brown br; OnePole lp; lp.SetLP(120);
        for (int i = 0; i < N(2.5f); i++) b[i] += lp.LP(br.P(r.W())) * expf(-(float)i / SR * 1.5f) * 1.5f;
        Reverb(b, 0.9f, 0.2f, 0.4f); Normalize(b, 0.95f); return b; }, 59);
}

}  // namespace

namespace audio {

AmbienceParams& Amb() { return g_amb; }

void Init() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) { TraceLog(LOG_WARNING, "Audio device unavailable"); return; }
    BuildBank();
    g_mixer.Init();
    SetAudioStreamBufferSizeDefault(2048);
    g_stream = LoadAudioStream(SR, 32, 2);
    SetAudioStreamCallback(g_stream, MixerCallback);
    PlayAudioStream(g_stream);
    g_ready = true;
}

void Shutdown() {
    if (!g_ready) return;
    StopAudioStream(g_stream);
    UnloadAudioStream(g_stream);
    for (auto& kv : g_bank) {
        for (auto& al : kv.second.aliases) for (auto& s : al) UnloadSoundAlias(s);
        for (auto& s : kv.second.variants) UnloadSound(s);
    }
    g_bank.clear();
    CloseAudioDevice();
    g_ready = false;
}

void Update(Vector3 p, Vector3 f) { g_listenerPos = p; g_listenerFwd = f; }
void SetMasterVolume(float v) { g_master = v; ::SetMasterVolume(v); }
bool Has(const char* id) { return g_bank.count(id) > 0; }

static Sound* Pick(const char* id) {
    auto it = g_bank.find(id);
    if (it == g_bank.end() || it->second.variants.empty()) return nullptr;
    Bank& b = it->second;
    int n = (int)b.variants.size();
    int v = GetRandomValue(0, n - 1);
    if (n > 1 && v == b.last) v = (v + 1) % n;
    b.last = v;
    auto& al = b.aliases[v];
    b.rr = (b.rr + 1) % (int)al.size();
    return &al[b.rr];
}

void Play(const char* id, float volume, float pitch) {
    if (!g_ready) return;
    Sound* s = Pick(id);
    if (!s) return;
    SetSoundVolume(*s, volume);
    SetSoundPitch(*s, pitch * (1.0f + (GetRandomValue(-100, 100) / 100.0f) * 0.03f));
    SetSoundPan(*s, 0.5f);
    PlaySound(*s);
}

void Play3D(const char* id, Vector3 pos, float volume, float pitch, float refDist, float maxDist) {
    if (!g_ready) return;
    Vector3 d = Vector3Subtract(pos, g_listenerPos);
    float dist = Vector3Length(d);
    if (dist > maxDist) return;
    float att = refDist / fmaxf(dist, refDist);
    att *= 1.0f - SmoothStep(maxDist * 0.6f, maxDist, dist);
    // occlusion: walls between listener and source muffle by volume
    if (dist > 1.5f && !Phys().LineOfSight(g_listenerPos, pos)) att *= 0.45f;
    Vector3 right = Vector3Normalize(Vector3CrossProduct(g_listenerFwd, { 0, 1, 0 }));
    float side = dist > 0.01f ? Vector3DotProduct(Vector3Scale(d, 1.0f / dist), right) : 0.0f;
    Sound* s = Pick(id);
    if (!s) return;
    SetSoundVolume(*s, volume * att);
    SetSoundPitch(*s, pitch * (1.0f + (GetRandomValue(-100, 100) / 100.0f) * 0.04f));
    SetSoundPan(*s, 0.5f - side * 0.42f);
    PlaySound(*s);
}

void PlayFootstep(int surface, Vector3 pos, float volume, bool indoor) {
    std::string id = std::string("step_") + SurfaceName(surface) + (indoor ? "_in" : "");
    if (!Has(id.c_str())) id = "step_dirt";
    Play3D(id.c_str(), pos, volume, 1.0f, 2.0f, 30.0f);
}

void StopAll(const char* id) {
    auto it = g_bank.find(id);
    if (it == g_bank.end()) return;
    for (auto& al : it->second.aliases) for (auto& s : al) StopSound(s);
}

}  // namespace audio
