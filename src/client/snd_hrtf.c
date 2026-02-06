#include "client.h"
#include "snd_loc.h"
#include "phonon.h"
#include <stdlib.h>
#include <math.h>

#define HRTF_FRAME_SIZE 512 

static IPLContext       ipl_context = NULL;
static IPLHRTF          ipl_hrtf    = NULL;
static IPLAudioSettings ipl_settings;

static float* in_channel_data[1];
static float* out_channel_data[2];
static IPLAudioBuffer ipl_in_buffer;
static IPLAudioBuffer ipl_out_buffer;

static IPLBinauralEffect ipl_effects[MAX_CHANNELS];

static float* AlignedMalloc(size_t size) {
    void* ptr = NULL;
    if (posix_memalign(&ptr, 32, size) != 0) return NULL;
    memset(ptr, 0, size);
    return (float*)ptr;
}

static void S_HRTF_Init(void) {
    // Sanity Check: Steam Audio 4.x wymaga stabilnego próbkowania.
    // 11kHz i 22kHz są przestarzałe i destabilizują potok przetwarzania.
    if (dma.speed < 44100) {
        Com_Printf("HRTF Error: dma.speed (%d) too low. Use s_khz 44.\n", dma.speed);
        return;
    }

    IPLContextSettings contextSettings = {
        .version = STEAMAUDIO_VERSION,
        .simdLevel = IPL_SIMDLEVEL_SSE2
    };
    
    if (iplContextCreate(&contextSettings, &ipl_context) != IPL_STATUS_SUCCESS) return;

    ipl_settings.samplingRate = dma.speed;
    ipl_settings.frameSize = HRTF_FRAME_SIZE;

    IPLHRTFSettings hrtfSettings = { 
        .type = IPL_HRTFTYPE_DEFAULT,
        .volume = 1.0f,
        .normType = IPL_HRTFNORMTYPE_NONE
    };
    
    if (iplHRTFCreate(ipl_context, &ipl_settings, &hrtfSettings, &ipl_hrtf) != IPL_STATUS_SUCCESS) return;

    in_channel_data[0] = AlignedMalloc(HRTF_FRAME_SIZE * sizeof(float));
    out_channel_data[0] = AlignedMalloc(HRTF_FRAME_SIZE * sizeof(float));
    out_channel_data[1] = AlignedMalloc(HRTF_FRAME_SIZE * sizeof(float));

    ipl_in_buffer = (IPLAudioBuffer){ .numChannels = 1, .numSamples = HRTF_FRAME_SIZE, .data = in_channel_data };
    ipl_out_buffer = (IPLAudioBuffer){ .numChannels = 2, .numSamples = HRTF_FRAME_SIZE, .data = out_channel_data };

    IPLBinauralEffectSettings effSet = { .hrtf = ipl_hrtf };
    for (int i = 0; i < MAX_CHANNELS; i++) {
        iplBinauralEffectCreate(ipl_context, &ipl_settings, &effSet, &ipl_effects[i]);
    }

    atomic_store(&s_ringbuffer.head, 0);
    atomic_store(&s_ringbuffer.tail, 0);

    Com_Printf("HRTF: Initialized at %d Hz.\n", dma.speed);
}

static void S_HRTF_Shutdown(void) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (ipl_effects[i]) iplBinauralEffectRelease(&ipl_effects[i]);
        ipl_effects[i] = NULL;
    }
    if (in_channel_data[0]) free(in_channel_data[0]);
    if (out_channel_data[0]) free(out_channel_data[0]);
    if (out_channel_data[1]) free(out_channel_data[1]);
    
    if (ipl_hrtf) iplHRTFRelease(&ipl_hrtf);
    if (ipl_context) iplContextRelease(&ipl_context);
    ipl_context = NULL;
    ipl_hrtf = NULL;
}

static void S_HRTF_MixFrame(void) {
    static float mix_l[HRTF_FRAME_SIZE], mix_r[HRTF_FRAME_SIZE];
    memset(mix_l, 0, sizeof(mix_l));
    memset(mix_r, 0, sizeof(mix_r));

    for (int i = 0; i < MAX_CHANNELS; i++) {
        channel_t* ch = &channels[i];
        if (!ch->sfx || !ipl_effects[i]) continue;
        sfxcache_t* sc = ch->sfx->cache;
        if (!sc) continue;

        vec3_t source_pos, rel_pos;
        if (ch->fixed_origin) VectorCopy(ch->origin, source_pos);
        else CL_GetEntitySoundOrigin(ch->entnum, source_pos);
        
        VectorSubtract(source_pos, listener_origin, rel_pos);
        
        IPLBinauralEffectParams params = {
            .interpolation = IPL_HRTFINTERPOLATION_BILINEAR,
            .spatialBlend = 1.0f,
            .hrtf = ipl_hrtf,
            .direction = {
                DotProduct(rel_pos, listener_right),
                DotProduct(rel_pos, listener_up),
                -DotProduct(rel_pos, listener_forward)
            }
        };

        float len = VectorLength(rel_pos);
        if (len > 0.1f) { 
            params.direction.x /= len; params.direction.y /= len; params.direction.z /= len; 
        } else {
            params.direction.z = -1.0f;
        }

        float scale = 1.0f - ((len - 80.0f) * ch->dist_mult);
        float vol = s_volume->value * (ch->master_vol / 255.0f) * bound(0.01f, scale, 1.0f);
        
        short* src = (short*)sc->data + (ch->pos * sc->channels);
        for (int s = 0; s < HRTF_FRAME_SIZE; s++) {
            if (ch->pos + s < sc->length) {
                float samp = (sc->channels == 2) ? (src[s*2] + src[s*2+1]) / 65536.0f : src[s] / 32768.0f;
                in_channel_data[0][s] = samp * vol;
            } else {
                in_channel_data[0][s] = 0.0f;
            }
        }

        iplBinauralEffectApply(ipl_effects[i], &params, &ipl_in_buffer, &ipl_out_buffer);

        for (int s = 0; s < HRTF_FRAME_SIZE; s++) {
            mix_l[s] += out_channel_data[0][s];
            mix_r[s] += out_channel_data[1][s];
        }

        ch->pos += HRTF_FRAME_SIZE;
        if (ch->pos >= sc->length) {
            if (sc->loopstart >= 0) ch->pos = sc->loopstart; else ch->sfx = NULL;
        }
    }

    short pcm[HRTF_FRAME_SIZE * 2];
    for (int s = 0; s < HRTF_FRAME_SIZE; s++) {
        pcm[s*2]   = (short)bound(-32768, mix_l[s] * 32767.0f, 32767);
        pcm[s*2+1] = (short)bound(-32768, mix_r[s] * 32767.0f, 32767);
    }

    S_Audio_Write(pcm, sizeof(pcm));
    paintedtime += HRTF_FRAME_SIZE;
}

static void S_HRTF_Update(const vec3_t origin, const vec3_t forward, const vec3_t right, const vec3_t up) {
    if (!sound_started || !ipl_hrtf) return;

    VectorCopy(origin, listener_origin);
    VectorCopy(forward, listener_forward);
    VectorCopy(right, listener_right);
    VectorCopy(up, listener_up);

    SDL_LockMutex(s_sound_mutex);
    while (s_pendingplays.next != &s_pendingplays && s_pendingplays.next->begin <= paintedtime) {
        S_IssuePlaysound(s_pendingplays.next);
    }
    SDL_UnlockMutex(s_sound_mutex);

    if (s_ambient->integer) S_AddLoopSounds();

    int bytes_per_sample = dma.channels * (dma.samplebits / 8);
    int target_samples = (int)(s_mixahead->value * dma.speed);
    int max_phys_samples = (AUDIO_RING_BUFFER_SIZE / bytes_per_sample) * 0.8;
    if (target_samples > max_phys_samples) target_samples = max_phys_samples;

    int safety = 0;
    while (safety++ < 64) {
        int current_samples = S_Audio_AvailableToRead() / bytes_per_sample;
        if (current_samples >= target_samples) break;
        if (S_Audio_AvailableToWrite() < (HRTF_FRAME_SIZE * bytes_per_sample)) break;

        S_HRTF_MixFrame();
    }
}

static void S_HRTF_StopAllSounds(void) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (ipl_effects[i]) iplBinauralEffectReset(ipl_effects[i]);
    }
}

sound_export_t snd_hrtf_export = {
    "steamaudio", S_HRTF_Init, S_HRTF_Shutdown, S_HRTF_Update, NULL, S_HRTF_StopAllSounds
};