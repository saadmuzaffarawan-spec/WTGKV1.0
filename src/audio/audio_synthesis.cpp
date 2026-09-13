#include "audio_synthesis.h"
#include <stdlib.h>
#include <math.h>


// PROCEDURAL AUDIO SYNTHESIS

// ============================================================





Sound GenerateFootstepSound() {

    int sampleRate = 44100;

    float duration = 0.2f; 

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    

    float filterOut1 = 0.0f; // Low mud crunch

    float filterOut2 = 0.0f; // Grass rustle

    

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float n = (float)(rand() % 2000 - 1000) / 1000.0f;

        

        // Mud/Sand displacement (Low-mid noise)

        filterOut1 = filterOut1 + 0.15f * (n - filterOut1);

        float env1 = (t < 0.03f) ? (t / 0.03f) : expf(-(t - 0.03f) * 15.0f);

        float mud = filterOut1 * env1;

        

        // Grass rustle (Subtract low frequencies to get high freq rustle)

        filterOut2 = filterOut2 + 0.4f * (n - filterOut2);

        float env2 = (t < 0.02f) ? (t / 0.02f) : expf(-(t - 0.02f) * 20.0f);

        float grass = (filterOut2 - filterOut1) * env2; 

        

        // Very soft subtle weight (NO pitch sweep, just a soft 40Hz bump)

        float weight = sinf(2.0f * PI * 40.0f * t) * expf(-t * 25.0f);

        

        // Mix: Mostly grass and mud, tiny bit of weight

        float mixed = (mud * 1.5f + grass * 1.2f + weight * 0.15f) * 0.8f;

        

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        

        data[i] = (short)(mixed * 32767.0f);

    }

    

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}

Sound GeneratePhoneSlideSound() {

    int sampleRate = 44100;

    float duration = 0.22f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    float hp = 0.0f;

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float noise = (float)(rand() % 2000 - 1000) / 1000.0f;

        hp = hp + 0.45f * (noise - hp);

        float clothHiss = (noise - hp) * expf(-t * 16.0f);

        // Subtle haptic vibration motor click / tap (110Hz bump)

        float haptic = sinf(2.0f * PI * 110.0f * t) * expf(-t * 32.0f);

        float mixed = (clothHiss * 0.70f + haptic * 0.45f) * 0.85f;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}




Sound GenerateCashRegisterSound() {
    int sampleRate = 44100;
    float duration = 0.42f;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)MemAlloc(frameCount * sizeof(short));
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        float bell1 = sinf(2.0f * PI * 1480.0f * t) * 0.35f;
        float bell2 = sinf(2.0f * PI * 2240.0f * t) * 0.22f;
        float bell3 = sinf(2.0f * PI * 3360.0f * t) * 0.12f;
        float envBell = expf(-t * 8.5f);
        float latch = 0.0f;
        if (t > 0.04f && t < 0.12f) {
            float tc = t - 0.04f;
            float noise = (float)(rand() % 2000 - 1000) / 1000.0f;
            latch = (sinf(2.0f * PI * 680.0f * tc) * 0.35f + noise * 0.25f) * expf(-tc * 45.0f);
        }
        float mixed = (bell1 + bell2 + bell3) * envBell + latch;
        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        data[i] = (short)(mixed * 32767.0f);
    }
    Wave wave = { 0 };
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

Sound GenerateDrivewayBellSound() {
    int sampleRate = 44100;
    float duration = 0.55f;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)MemAlloc(frameCount * sizeof(short));
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        float s1 = sinf(2.0f * PI * 1280.0f * t) * 0.40f + sinf(2.0f * PI * 2560.0f * t) * 0.18f;
        float env1 = expf(-t * 9.5f);
        float s2 = 0.0f;
        if (t > 0.14f) {
            float t2 = t - 0.14f;
            s2 = (sinf(2.0f * PI * 1720.0f * t2) * 0.45f + sinf(2.0f * PI * 3440.0f * t2) * 0.20f) * expf(-t2 * 9.5f);
        }
        float mixed = s1 * env1 + s2;
        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        data[i] = (short)(mixed * 32767.0f);
    }
    Wave wave = { 0 };
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

Sound GeneratePumpFlowSound() {
    int sampleRate = 44100;
    float duration = 0.35f;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)MemAlloc(frameCount * sizeof(short));
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        float motor = sinf(2.0f * PI * 92.0f * t) * 0.22f + sinf(2.0f * PI * 184.0f * t) * 0.12f;
        float hiss = ((float)(rand() % 2000 - 1000) / 1000.0f) * 0.15f;
        float click = 0.0f;
        if (t > 0.16f && t < 0.20f) {
            float tc = t - 0.16f;
            click = sinf(2.0f * PI * 1100.0f * tc) * expf(-tc * 80.0f) * 0.35f;
        }
        float mixed = (motor + hiss + click) * 0.65f;
        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        data[i] = (short)(mixed * 32767.0f);
    }
    Wave wave = { 0 };
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

Sound GenerateNozzleLatchSound() {
    int sampleRate = 44100;
    float duration = 0.12f;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)MemAlloc(frameCount * sizeof(short));
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        // Sharp metallic latch strike
        float latch = sinf(2.0f * PI * 1850.0f * t) * expf(-t * 95.0f) * 0.55f;
        // Solid cast-iron lever clunk
        float clunk = (t > 0.024f) ? (sinf(2.0f * PI * 420.0f * (t - 0.024f)) * expf(-(t - 0.024f) * 65.0f) * 0.45f) : 0.0f;
        // Metal spring resonance
        float ping = sinf(2.0f * PI * 3100.0f * t) * expf(-t * 140.0f) * 0.25f;
        float mixed = (latch + clunk + ping) * 0.85f;
        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        data[i] = (short)(mixed * 32767.0f);
    }
    Wave wave = { 0 };
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

Sound GenerateNozzleShutoffSound() {
    int sampleRate = 44100;
    float duration = 0.18f;
    int frameCount = (int)(sampleRate * duration);
    short* data = (short*)MemAlloc(frameCount * sizeof(short));
    for (int i = 0; i < frameCount; i++) {
        float t = (float)i / sampleRate;
        // Heavy spring-loaded valve trip snap (loud mechanical CLACK!)
        float snap = sinf(2.0f * PI * 2450.0f * t) * expf(-t * 85.0f) * 0.75f;
        // High impulse metallic click
        float click = sinf(2.0f * PI * 4200.0f * t) * expf(-t * 190.0f) * 0.45f;
        // Low-end pipe hammer thud
        float thud = sinf(2.0f * PI * 165.0f * t) * expf(-t * 40.0f) * 0.65f;
        float mixed = (snap + click + thud) * 0.90f;
        if (mixed > 1.0f) mixed = 1.0f;
        if (mixed < -1.0f) mixed = -1.0f;
        data[i] = (short)(mixed * 32767.0f);
    }
    Wave wave = { 0 };
    wave.frameCount = frameCount;
    wave.sampleRate = sampleRate;
    wave.sampleSize = 16;
    wave.channels = 1;
    wave.data = data;
    Sound snd = LoadSoundFromWave(wave);
    UnloadWave(wave);
    return snd;
}

Sound GenerateFlashlightToggleSound() {

    int sampleRate = 44100;

    float duration = 0.085f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float click1 = sinf(2.0f * PI * 3200.0f * t) * expf(-t * 220.0f);

        float click2 = (t > 0.026f) ? (sinf(2.0f * PI * 1650.0f * (t - 0.026f)) * expf(-(t - 0.026f) * 140.0f)) : 0.0f;

        float thunk  = sinf(2.0f * PI * 220.0f * t) * expf(-t * 50.0f) * 0.35f;

        float mixed = (click1 * 0.70f + click2 * 0.55f + thunk);

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateLightSwitchSound() {

    int sampleRate = 44100;

    float duration = 0.08f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float click1 = sinf(2.0f * PI * 2800.0f * t) * expf(-t * 200.0f);

        float click2 = (t > 0.020f) ? (sinf(2.0f * PI * 1500.0f * (t - 0.020f)) * expf(-(t - 0.020f) * 130.0f)) : 0.0f;

        float thunk  = sinf(2.0f * PI * 180.0f * t) * expf(-t * 45.0f) * 0.40f;

        float mixed = (click1 * 0.75f + click2 * 0.60f + thunk);

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GeneratePhoneTapSound() {

    int sampleRate = 44100;

    float duration = 0.045f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float click = sinf(2.0f * PI * 3200.0f * t) * expf(-t * 190.0f);

        float thud = sinf(2.0f * PI * 440.0f * t) * expf(-t * 110.0f) * 0.4f;

        float mixed = (click * 0.75f + thud) * 0.75f;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateRadioStaticSound() {

    int sampleRate = 44100;

    float duration = 0.65f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    float hp = 0.0f;

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float noise = (float)(rand() % 2000 - 1000) / 1000.0f;

        hp = hp + 0.35f * (noise - hp);

        float hiss = noise - hp;

        float whistle = sinf(2.0f * PI * 1750.0f * t) * 0.14f;

        float crackle = ((rand() % 100) > 88) ? (noise * 0.85f) : 0.0f;

        float env = 1.0f;

        if (t < 0.05f) env = t / 0.05f;

        else if (t > duration - 0.08f) env = (duration - t) / 0.08f;

        float mixed = (hiss * 0.55f + whistle + crackle) * env * 0.70f;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateWaterDripSound() {

    int sampleRate = 44100;

    float duration = 0.22f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float fDrop = 1420.0f + 230.0f * expf(-t * 85.0f);

        float ping = sinf(2.0f * PI * fDrop * t) * expf(-t * 22.0f);

        float overtone = sinf(2.0f * PI * 3120.0f * t) * expf(-t * 45.0f) * 0.35f;

        float cavity = sinf(2.0f * PI * 380.0f * t) * expf(-t * 16.0f) * 0.28f;

        float mixed = (ping * 0.65f + overtone + cavity) * 0.85f;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateChestOpenSound() {

    int sampleRate = 44100;

    float duration = 0.55f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float snap = (t < 0.05f) ? (sinf(2.0f * PI * 2200.0f * t) * expf(-t * 80.0f) * 1.5f) : 0.0f;

        float thud = (t < 0.12f) ? (sinf(2.0f * PI * 135.0f * t) * expf(-t * 35.0f) * 0.9f) : 0.0f;

        float creakFreq = 260.0f + 160.0f * sinf(t * 18.0f);

        float creak = (t >= 0.06f) ? (sinf(2.0f * PI * creakFreq * t) * expf(-(t - 0.06f) * 6.5f) * 0.45f) : 0.0f;

        float mixed = snap * 0.55f + thud * 0.45f + creak;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateShovelDigSound() {

    int sampleRate = 44100;

    float duration = 0.38f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float strike = sinf(2.0f * PI * 115.0f * t) * expf(-t * 22.0f);

        float noise = (float)(rand() % 2000 - 1000) / 1000.0f;

        float grit = noise * expf(-t * 9.0f);

        float sample = strike * 0.58f + grit * 0.42f;

        if (sample > 1.0f) sample = 1.0f;

        if (sample < -1.0f) sample = -1.0f;

        data[i] = (short)(sample * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}





Sound GenerateCricketAmbience() {

    int sampleRate = 44100;

    float duration = 5.0f; 

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        

        // Primary cricket group

        float freq = 4200.0f + sinf(t * 3.0f) * 30.0f; 

        float osc = sinf(2.0f * PI * freq * t);

        float chirpPulse = sinf(2.0f * PI * 22.0f * t); 

        chirpPulse = (chirpPulse > 0.0f) ? chirpPulse : 0.0f; 

        float burstEnv = sinf(2.0f * PI * 0.6f * t + sinf(t * 1.1f));

        burstEnv = (burstEnv > 0.4f) ? (burstEnv - 0.4f) * 1.6f : 0.0f;

        

        // Secondary distant crickets

        float osc2 = sinf(2.0f * PI * 5100.0f * t);

        float pulse2 = sinf(2.0f * PI * 26.0f * t);

        pulse2 = (pulse2 > 0.0f) ? pulse2 : 0.0f;

        float burst2 = sinf(2.0f * PI * 0.4f * t);

        burst2 = (burst2 > 0.2f) ? (burst2 - 0.2f) * 1.2f : 0.0f;

        

        float mixed = (osc * chirpPulse * burstEnv) + (osc2 * pulse2 * burst2 * 0.3f);

        mixed *= 0.035f; // Very quiet background ambience

        

        data[i] = (short)(mixed * 32767.0f);

    }

    

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateWindAmbience() {

    int sampleRate = 44100;

    float duration = 8.0f; 

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    

    float filterOut = 0.0f;

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float n = (float)(rand() % 2000 - 1000) / 1000.0f;

        

        float gustLFO = sinf(2.0f * PI * 0.08f * t) + sinf(2.0f * PI * 0.19f * t);

        gustLFO = (gustLFO + 2.0f) * 0.25f; 

        

        float filterAlpha = 0.005f + 0.02f * gustLFO; 

        filterOut = filterOut + filterAlpha * (n - filterOut);

        

        float vol = 0.15f + 0.2f * gustLFO;

        float mixed = filterOut * vol;

        

        // Fade edges for seamless looping

        float fade = 1.0f;

        if (t < 0.5f) fade = t / 0.5f;

        else if (duration - t < 0.5f) fade = (duration - t) / 0.5f;

        

        data[i] = (short)(mixed * fade * 32767.0f);

    }

    

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateThunderSound() {

    int sampleRate = 44100;

    float duration = 4.0f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    

    float filterOut1 = 0.0f;

    float filterOut2 = 0.0f;

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float n = (float)(rand() % 2000 - 1000) / 1000.0f; // white noise

        

        // Initial crack envelope

        float envCrack = expf(-t * 15.0f);

        // Rumble envelope

        float envRumble = expf(-t * 1.5f) * (1.0f - expf(-t * 10.0f));

        

        // Lowpass filter for deep rumble

        filterOut1 = filterOut1 + 0.015f * (n - filterOut1);

        filterOut2 = filterOut2 + 0.005f * (filterOut1 - filterOut2);

        

        float crack = n * envCrack * 0.4f;

        float rumble = filterOut2 * envRumble * 10.0f;

        

        float mixed = (crack + rumble) * 0.7f;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        

        data[i] = (short)(mixed * 32767.0f);

    }

    

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateRainSound() {

    int sampleRate = 44100;

    float duration = 2.0f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    

    float filterOut = 0.0f;

    for (int i = 0; i < frameCount; i++) {

        float n = (float)(rand() % 2000 - 1000) / 1000.0f;

        // Highpass filter for rain sizzle

        filterOut = filterOut + 0.8f * (n - filterOut);

        float mixed = (n - filterOut) * 0.2f; 

        

        data[i] = (short)(mixed * 32767.0f);

    }

    

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateMenuNavSound() {

    int sampleRate = 44100;

    float duration = 0.055f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float env = expf(-t * 85.0f);

        float tone = sinf(2.0f * PI * 140.0f * t) * 0.7f + sinf(2.0f * PI * 70.0f * t) * 0.3f;

        float noise = ((float)(rand() % 1000) / 500.0f - 1.0f) * 0.15f;

        float s = (tone + noise) * env;

        if (s > 1.0f) s = 1.0f;

        if (s < -1.0f) s = -1.0f;

        data[i] = (short)(s * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateMenuBoomSound() {

    int sampleRate = 44100;

    float duration = 0.50f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    float lp = 0.0f;

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float freq = 85.0f * expf(-t * 4.5f) + 36.0f;

        float env = expf(-t * 4.0f);

        float tone = sinf(2.0f * PI * freq * t);

        float noise = ((float)(rand() % 1000) / 500.0f - 1.0f);

        lp = lp + 0.07f * (noise - lp);

        float s = (tone * 0.75f + lp * 0.25f) * env;

        if (s > 1.0f) s = 1.0f;

        if (s < -1.0f) s = -1.0f;

        data[i] = (short)(s * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateElectricSparkSound() {

    int sampleRate = 44100;

    float duration = 0.16f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));



    float hpFilter = 0.0f;

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float n = (float)(rand() % 2000 - 1000) / 1000.0f;



        // High-pass filter for sharp electrical sizzle (cutoff ~3.5kHz)

        hpFilter = hpFilter + 0.65f * (n - hpFilter);

        float sizzle = n - hpFilter;



        // Rapid initial Dirac snap impulse (first 1.5ms)

        float snap = (t < 0.0015f) ? (sinf(2.0f * PI * 4200.0f * t) * (1.0f - t / 0.0015f)) : 0.0f;



        // Multi-frequency electrical arc hiss

        float arcBuzz = sinf(2.0f * PI * 120.0f * t) * 0.25f + sinf(2.0f * PI * 240.0f * t) * 0.15f;



        // Intermittent micro-pops inside the crackle

        float pop = 0.0f;

        if (i % 380 < 12) pop = (float)(rand() % 2000 - 1000) / 1000.0f * 0.6f;



        // Sharp exponential decay envelope

        float env = expf(-t * 28.0f);



        float mixed = (snap * 2.5f + sizzle * 1.8f + arcBuzz * 0.8f + pop) * env;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;



        data[i] = (short)(mixed * 32767.0f);

    }



    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateJumpscareSound() {

    int sampleRate = 44100;

    float duration = 1.35f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));



    float hpScream = 0.0f;



    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float n = (float)(rand() % 2000 - 1000) / 1000.0f;



        // 1. Violent visceral sub-bass punch (42Hz -> 28Hz kick slam)

        float bassFreq = 42.0f * expf(-t * 8.0f) + 26.0f;

        float subPunch = sinf(2.0f * PI * bassFreq * t) * expf(-t * 4.5f) * 1.8f;



        // 2. Screaming discordant metallic frequencies (piercing horror dissonance)

        float s1 = sinf(2.0f * PI * 680.0f * t);

        float s2 = sinf(2.0f * PI * 920.0f * t);

        float s3 = sinf(2.0f * PI * 1440.0f * t);

        float s4 = sinf(2.0f * PI * 2180.0f * t);

        float dissonantScream = (s1 * 0.4f + s2 * 0.35f + s3 * 0.3f + s4 * 0.25f) * expf(-t * 2.8f);



        // 3. Distorted high-frequency tearing noise burst

        hpScream = hpScream + 0.55f * (n - hpScream);

        float tearNoise = (n - hpScream) * expf(-t * 5.0f);



        // 4. Initial brutal clipping impulse (first 40ms)

        float clipSnap = (t < 0.04f) ? ((float)(rand() % 2000 - 1000) / 1000.0f * 1.5f) : 0.0f;



        float mixed = subPunch * 1.2f + dissonantScream * 1.4f + tearNoise * 1.2f + clipSnap;



        // Soft saturation wave shaper

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;



        data[i] = (short)(mixed * 32767.0f);

    }



    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateGunshotSound() {

    int sampleRate = 44100;

    float duration = 0.42f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    float lp = 0.0f;

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float noise = (float)(rand() % 2000 - 1000) / 1000.0f;

        // Explosive noise transient

        float envNoise = expf(-t * 34.0f);

        // Heavy concussive punch pitch sweep from 160Hz down to 42Hz

        float fPunch = 42.0f + 118.0f * expf(-t * 26.0f);

        float punch = sinf(2.0f * PI * fPunch * t) * expf(-t * 18.0f);

        // Room resonance reverb tail

        lp += 0.07f * (noise - lp);

        float tail = lp * expf(-t * 6.5f);

        float mixed = noise * envNoise * 0.78f + punch * 0.58f + tail * 0.22f;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateStoreFootstepSound() {

    int sampleRate = 44100;

    float duration = 0.075f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float noise = (float)(rand() % 2000 - 1000) / 1000.0f;

        // Crisp heel/sole tap on hard supermarket polished linoleum tile

        float tap = sinf(2.0f * PI * 920.0f * t) * expf(-t * 95.0f);

        float thud = sinf(2.0f * PI * 115.0f * t) * expf(-t * 48.0f);

        float friction = noise * expf(-t * 120.0f) * 0.32f;

        float mixed = (tap * 0.52f + thud * 0.44f + friction) * 0.82f;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}



Sound GenerateFoilSound() {

    int sampleRate = 44100;

    float duration = 0.16f;

    int frameCount = (int)(sampleRate * duration);

    short* data = (short*)MemAlloc(frameCount * sizeof(short));

    float hp = 0.0f;

    for (int i = 0; i < frameCount; i++) {

        float t = (float)i / sampleRate;

        float noise = (float)(rand() % 2000 - 1000) / 1000.0f;

        hp = noise - (hp * 0.85f); // High-pass crinkle

        float crackle = ((rand() % 100) > 82) ? (noise * 1.8f) : 0.0f;

        float env = (t < 0.02f) ? (t / 0.02f) : expf(-(t - 0.02f) * 14.0f);

        float mixed = (hp * 0.4f + crackle * 0.6f) * env * 0.65f;

        if (mixed > 1.0f) mixed = 1.0f;

        if (mixed < -1.0f) mixed = -1.0f;

        data[i] = (short)(mixed * 32767.0f);

    }

    Wave wave = { 0 };

    wave.frameCount = frameCount;

    wave.sampleRate = sampleRate;

    wave.sampleSize = 16;

    wave.channels = 1;

    wave.data = data;

    Sound snd = LoadSoundFromWave(wave);

    UnloadWave(wave);

    return snd;

}
