// Audio: physically-modelled synthesized sounds (no music, ever).
//
// * One-shots are pre-rendered in several randomized variants per id and
//   played with 3D panning/attenuation.
// * Continuous sounds (wind, crickets, rain, engine, hum, fire, heartbeat...)
//   are synthesized live on the audio thread by the ambience mixer, so they
//   never loop audibly. Their levels are driven by AmbienceParams.
// * A file at assets/sounds/<id>_<n>.wav replaces the synthesized variants.
#pragma once
#include "common.h"
#include <atomic>

struct AmbienceParams {
    std::atomic<float> master{ 1.0f };
    std::atomic<float> wind{ 0.5f }, gust{ 0.5f }, trees{ 0.4f };
    std::atomic<float> crickets{ 0.6f }, frogs{ 0.2f }, threat{ 0.0f };
    std::atomic<float> hum{ 0.0f }, fridge{ 0.0f }, ballast{ 0.0f };
    std::atomic<float> rain{ 0.0f };
    std::atomic<float> engine{ 0.0f }, rpm{ 900.0f }, road{ 0.0f }, cabin{ 0.0f };
    std::atomic<float> screech{ 0.0f };
    std::atomic<float> pumpMotor{ 0.0f }, fuelFlow{ 0.0f }, pour{ 0.0f };
    std::atomic<float> fire{ 0.0f };
    std::atomic<float> drag{ 0.0f };
    std::atomic<float> drone{ 0.0f }, drips{ 0.0f };
    std::atomic<float> tinnitus{ 0.0f };
    std::atomic<float> heart{ 0.0f }, heartRate{ 70.0f };
    std::atomic<float> breath{ 0.0f }, breathRate{ 14.0f };
    std::atomic<float> indoor{ 0.0f };      // muffles outdoor beds
    std::atomic<float> muffle{ 0.0f };      // global low-pass (concussion, death)
    std::atomic<float> whispers{ 0.0f };
};

namespace audio {

void Init();
void Shutdown();
void Update(Vector3 listenerPos, Vector3 listenerFwd);
AmbienceParams& Amb();

// Play a one-shot. Returns nothing; variants chosen randomly (never the same twice in a row).
void Play(const char* id, float volume = 1.0f, float pitch = 1.0f);
void Play3D(const char* id, Vector3 pos, float volume = 1.0f, float pitch = 1.0f, float refDist = 2.0f, float maxDist = 40.0f);
void PlayFootstep(int surface, Vector3 pos, float volume, bool indoor);
bool Has(const char* id);
void SetMasterVolume(float v);
void StopAll(const char* id);

}  // namespace audio
