#include "client.h"
#include "snd_loc.h"

static void S_HRTF_Init(void) {
    Com_Printf("HRTF: Engine initialized (Stub)\n");
}

static void S_HRTF_Shutdown(void) {
    Com_Printf("HRTF: Engine shutdown\n");
}

static void S_HRTF_Update(const vec3_t origin, const vec3_t forward, const vec3_t right, const vec3_t up) {
    // Tutaj w przyszłości wejdzie Steam Audio
    // Na razie backend HRTF milczy
}

static void S_HRTF_StartSound(const vec3_t origin, int entnum, int entchannel, sfx_t *sfx, float vol, float attenuation, float timeofs) {
    // Rejestracja źródła dźwięku w API HRTF
}

static void S_HRTF_StopAllSounds(void) {
    // Reset silnika HRTF
}

sound_export_t snd_hrtf_export = {
    "steamaudio",
    S_HRTF_Init,
    S_HRTF_Shutdown,
    S_HRTF_Update,
    S_HRTF_StartSound,
    S_HRTF_StopAllSounds
};