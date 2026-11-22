// Gemini 3 Flash [X-JC-STRICTOR-V2] 2026-02-04

// Steam Audio 4.8 + Occlusion & SIMD (GCC 13 compat)

// snd_hrtf.c 
#include "client.h"
#include "snd_loc.h"
#include "phonon.h"
#include <dlfcn.h>
#include <emmintrin.h>
#include <smmintrin.h>

#define HRTF_FRAME_SIZE 512

extern cvar_t *s_occlusion;

static void             *phonon_handle = NULL;
static IPLContext       ipl_context = NULL;
static IPLHRTF          ipl_hrtf    = NULL;
static IPLAudioSettings ipl_settings;

static float* in_channel_data[1];
static float* out_channel_data[2]; 
static IPLAudioBuffer ipl_in_buffer;
static IPLAudioBuffer ipl_out_buffer;

static IPLBinauralEffect ipl_effects[MAX_CHANNELS];
static IPLDirectEffect   ipl_direct_effects[MAX_CHANNELS];
static const void       *hrtf_last_sfx[MAX_CHANNELS];
static int               hrtf_last_entnum[MAX_CHANNELS];

// --- Dynamic Loader ---
typedef int  (*f_iplContextCreate)(IPLContextSettings*, IPLContext*);
typedef void (*f_iplContextRelease)(IPLContext*);
typedef int  (*f_iplHRTFCreate)(IPLContext, IPLAudioSettings*, IPLHRTFSettings*, IPLHRTF*);
typedef void (*f_iplHRTFRelease)(IPLHRTF*);
typedef IPLAudioEffectState (*f_iplBinauralEffectApply)(IPLBinauralEffect, IPLBinauralEffectParams*, IPLAudioBuffer*, IPLAudioBuffer*);
typedef int  (*f_iplBinauralEffectCreate)(IPLContext, IPLAudioSettings*, IPLBinauralEffectSettings*, IPLBinauralEffect*);
typedef void (*f_iplBinauralEffectRelease)(IPLBinauralEffect*);
typedef void (*f_iplBinauralEffectReset)(IPLBinauralEffect);
typedef int  (*f_iplDirectEffectCreate)(IPLContext, IPLAudioSettings*, IPLDirectEffectSettings*, IPLDirectEffect*);
typedef void (*f_iplDirectEffectRelease)(IPLDirectEffect*);
typedef void (*f_iplDirectEffectReset)(IPLDirectEffect);
typedef IPLAudioEffectState (*f_iplDirectEffectApply)(IPLDirectEffect, IPLDirectEffectParams*, IPLAudioBuffer*, IPLAudioBuffer*);

static f_iplContextCreate ptr_iplContextCreate;
static f_iplContextRelease ptr_iplContextRelease;
static f_iplHRTFCreate ptr_iplHRTFCreate;
static f_iplHRTFRelease ptr_iplHRTFRelease;
static f_iplBinauralEffectCreate ptr_iplBinauralEffectCreate;
static f_iplBinauralEffectRelease ptr_iplBinauralEffectRelease;
static f_iplBinauralEffectReset ptr_iplBinauralEffectReset;
static f_iplBinauralEffectApply ptr_iplBinauralEffectApply;
static f_iplDirectEffectCreate ptr_iplDirectEffectCreate;
static f_iplDirectEffectRelease ptr_iplDirectEffectRelease;
static f_iplDirectEffectReset ptr_iplDirectEffectReset;
static f_iplDirectEffectApply ptr_iplDirectEffectApply;

#define LOAD_FUNC(type, name) do { \
    void *temp_ptr = dlsym(phonon_handle, #name); \
    if (!temp_ptr) return false; \
    *(void **)(&ptr_##name) = temp_ptr; \
} while(0)

static float* AlignedMalloc(size_t size) {
    void* ptr = NULL;
    if (posix_memalign(&ptr, 32, size) != 0) return NULL;
    memset(ptr, 0, size);
    return (float*)ptr;
}

static float S_CheckOcclusion(const vec3_t target) {
    if (!s_occlusion->integer || !cl.model_clip[1]) return 1.0f;
    trace_t tr;
    vec3_t start;
    // Lekki offset od oczu gracza, aby nie uderzyć we własny bounding box
    VectorMA(listener_origin, 2.0f, listener_forward, start);
    tr = CM_BoxTrace(start, target, vec3_origin, vec3_origin, cl.model_clip[1]->headnode, MASK_SOLID);
    return (tr.fraction < 1.0f) ? 0.0f : 1.0f;
}

static qboolean S_HRTF_Init(void) {
    if (dma.channels != 2) {
        Com_Printf("Sound: HRTF requires stereo output (s_force_mono 0).\n");
        return false;
    }

    phonon_handle = dlopen("libphonon.so", RTLD_LAZY);
    if (!phonon_handle) return false;

    LOAD_FUNC(f_iplContextCreate, iplContextCreate);
    LOAD_FUNC(f_iplContextRelease, iplContextRelease);
    LOAD_FUNC(f_iplHRTFCreate, iplHRTFCreate);
    LOAD_FUNC(f_iplHRTFRelease, iplHRTFRelease);
    LOAD_FUNC(f_iplBinauralEffectCreate, iplBinauralEffectCreate);
    LOAD_FUNC(f_iplBinauralEffectRelease, iplBinauralEffectRelease);
    LOAD_FUNC(f_iplBinauralEffectReset, iplBinauralEffectReset);
    LOAD_FUNC(f_iplBinauralEffectApply, iplBinauralEffectApply);
    LOAD_FUNC(f_iplDirectEffectCreate, iplDirectEffectCreate);
    LOAD_FUNC(f_iplDirectEffectRelease, iplDirectEffectRelease);
    LOAD_FUNC(f_iplDirectEffectReset, iplDirectEffectReset);
    LOAD_FUNC(f_iplDirectEffectApply, iplDirectEffectApply);

    IPLContextSettings ctxSet = { .version = STEAMAUDIO_VERSION, .simdLevel = IPL_SIMDLEVEL_SSE4 };
    if (ptr_iplContextCreate(&ctxSet, &ipl_context) != IPL_STATUS_SUCCESS) return false;

    ipl_settings.samplingRate = dma.speed;
    ipl_settings.frameSize = HRTF_FRAME_SIZE;

    IPLHRTFSettings hrtfSet = { .type = IPL_HRTFTYPE_DEFAULT, .volume = 1.0f, .normType = IPL_HRTFNORMTYPE_NONE };
    if (ptr_iplHRTFCreate(ipl_context, &ipl_settings, &hrtfSet, &ipl_hrtf) != IPL_STATUS_SUCCESS) return false;

    in_channel_data[0] = AlignedMalloc(HRTF_FRAME_SIZE * sizeof(float));
    out_channel_data[0] = AlignedMalloc(HRTF_FRAME_SIZE * sizeof(float));
    out_channel_data[1] = AlignedMalloc(HRTF_FRAME_SIZE * sizeof(float));

    ipl_in_buffer = (IPLAudioBuffer){ .numChannels = 1, .numSamples = HRTF_FRAME_SIZE, .data = in_channel_data };
    ipl_out_buffer = (IPLAudioBuffer){ .numChannels = 2, .numSamples = HRTF_FRAME_SIZE, .data = out_channel_data };

    IPLBinauralEffectSettings effSet = { .hrtf = ipl_hrtf };
    IPLDirectEffectSettings dirSet = { .numChannels = 1 };

    for (int i = 0; i < MAX_CHANNELS; i++) {
        ptr_iplBinauralEffectCreate(ipl_context, &ipl_settings, &effSet, &ipl_effects[i]);
        ptr_iplDirectEffectCreate(ipl_context, &ipl_settings, &dirSet, &ipl_direct_effects[i]);
    }
    return true;
}

static void S_HRTF_FillInputBuffer(const channel_t *ch, const sfxcache_t *sc, float vol, float *in_buf) {
    const short *src_base = (const short *)sc->data;
    const float inv_scale = vol * (1.0f / 32768.0f);
    const qboolean looping = (sc->loopstart >= 0) || ch->autosound;
    const int loop_start = (sc->loopstart >= 0) ? sc->loopstart : 0;
    const int len = sc->length;
    int pos = ch->pos;

    // Szybka ścieżka SIMD, gdy cała ramka mieści się bez zawijania/końca
    if (pos >= 0 && pos + HRTF_FRAME_SIZE <= len) {
        const short *src = src_base + (pos * sc->channels);
        __m128 v_scale = _mm_set1_ps(inv_scale);
        __m128 v_half = _mm_set1_ps(0.5f);

        for (int s = 0; s < HRTF_FRAME_SIZE; s += 4) {
            __m128 f32;
            if (sc->channels == 2) {
                __m128i s_l = _mm_set_epi32(src[(s+3)*2],   src[(s+2)*2],   src[(s+1)*2],   src[s*2]);
                __m128i s_r = _mm_set_epi32(src[(s+3)*2+1], src[(s+2)*2+1], src[(s+1)*2+1], src[s*2+1]);
                f32 = _mm_mul_ps(_mm_cvtepi32_ps(_mm_add_epi32(s_l, s_r)), v_half);
            } else {
                f32 = _mm_cvtepi32_ps(_mm_cvtepi16_epi32(_mm_loadl_epi64((const __m128i*)&src[s])));
            }
            _mm_store_ps(&in_buf[s], _mm_mul_ps(f32, v_scale));
        }
        return;
    }

    // Ścieżka brzegowa: koniec próbki lub zawijanie pętli
    for (int s = 0; s < HRTF_FRAME_SIZE; s++) {
        if (pos >= len) {
            if (looping && len > loop_start) {
                pos = loop_start + (pos - len) % (len - loop_start);
            } else {
                for (int j = s; j < HRTF_FRAME_SIZE; j++) in_buf[j] = 0.0f;
                break;
            }
        }

        float sample;
        if (sc->channels == 2) {
            sample = 0.5f * ((float)src_base[pos * 2] + (float)src_base[pos * 2 + 1]);
        } else {
            sample = (float)src_base[pos];
        }
        in_buf[s] = sample * inv_scale;
        pos++;
    }
}

static void S_HRTF_MixFrame(void) {
    static float mix_l[HRTF_FRAME_SIZE] __attribute__((aligned(32)));
    static float mix_r[HRTF_FRAME_SIZE] __attribute__((aligned(32)));
    static float direct_out[HRTF_FRAME_SIZE] __attribute__((aligned(32)));

    memset(mix_l, 0, sizeof(mix_l));
    memset(mix_r, 0, sizeof(mix_r));

    for (int i = 0; i < MAX_CHANNELS; i++) {
        channel_t* ch = &channels[i];
        
        if (!ch->sfx || !ch->sfx->cache || ch->sfx->cache->length <= 0) {
            hrtf_last_sfx[i] = NULL;
            hrtf_last_entnum[i] = 0;
            continue;
        }

        sfxcache_t* sc = ch->sfx->cache;
        vec3_t source_pos, rel_pos;
        S_GetChannelOrigin(ch, source_pos);

        // Lokalny gracz oraz dźwięki globalne (ATTN_NONE) miksujemy bezpośrednio 2D,
        // bez okluzji od własnych ścian i bez koloryzacji kierunkowej HRTF.
        qboolean is_direct_2d = (ch->entnum == cl.playernum + 1) || (ch->dist_mult <= 0.0f);

        if (is_direct_2d) {
            float vol = s_volume->value * (ch->master_vol / 255.0f) * 0.8f;
            if (vol > 0.0f) {
                float *in_buf = in_channel_data[0];
                S_HRTF_FillInputBuffer(ch, sc, vol, in_buf);
                for (int s = 0; s < HRTF_FRAME_SIZE; s += 4) {
                    __m128 v_in = _mm_load_ps(&in_buf[s]);
                    _mm_store_ps(&mix_l[s], _mm_add_ps(_mm_load_ps(&mix_l[s]), v_in));
                    _mm_store_ps(&mix_r[s], _mm_add_ps(_mm_load_ps(&mix_r[s]), v_in));
                }
            }
            hrtf_last_sfx[i] = ch->sfx;
            hrtf_last_entnum[i] = ch->entnum;
            if (ch->autosound || sc->loopstart >= 0)
                ch->end = paintedtime + HRTF_FRAME_SIZE + sc->length;
            S_ProcessChannelSamples(ch, HRTF_FRAME_SIZE);
            continue;
        }

        VectorSubtract(source_pos, listener_origin, rel_pos);
        float len = VectorLength(rel_pos);
        float dist = len - 80.0f;
        if (dist < 0.0f) dist = 0.0f;

        float scale = 1.0f - (dist * ch->dist_mult);
        if (scale <= 0.0f) {
            // Poza zasięgiem: przesuwamy czas próbki bez kosztownego DSP/okluzji
            if (ch->autosound || sc->loopstart >= 0)
                ch->end = paintedtime + HRTF_FRAME_SIZE + sc->length;
            S_ProcessChannelSamples(ch, HRTF_FRAME_SIZE);
            continue;
        }
        if (scale > 1.0f) scale = 1.0f;

        // Oczyszczanie stanu DSP przy nowym dźwięku, zmianie encji lub restarcie
        if (ch->sfx != hrtf_last_sfx[i] || ch->entnum != hrtf_last_entnum[i] || (ch->pos == 0 && !ch->autosound)) {
            ptr_iplBinauralEffectReset(ipl_effects[i]);
            ptr_iplDirectEffectReset(ipl_direct_effects[i]);
        }
        hrtf_last_sfx[i] = ch->sfx;
        hrtf_last_entnum[i] = ch->entnum;

        float occlusion = S_CheckOcclusion(source_pos);
        float vol = s_volume->value * (ch->master_vol / 255.0f) * (scale * scale) * 0.8f;

        // 1. Konwersja PCM -> Float (SIMD + płynne zawijanie pętli)
        S_HRTF_FillInputBuffer(ch, sc, vol, in_channel_data[0]);

        // 2. Direct Effect - Inicjalizacja WSZYSTKICH pól dla 4.8
        IPLDirectEffectParams dirParams = {0};
        dirParams.flags = IPL_DIRECTEFFECTFLAGS_APPLYOCCLUSION | IPL_DIRECTEFFECTFLAGS_APPLYTRANSMISSION;
        dirParams.transmissionType = IPL_TRANSMISSIONTYPE_FREQDEPENDENT;
        dirParams.distanceAttenuation = 1.0f;
        dirParams.occlusion = occlusion;
        dirParams.transmission[0] = 0.6f;
        dirParams.transmission[1] = 0.4f;
        dirParams.transmission[2] = 0.2f;
        dirParams.directivity = 1.0f;

        float* direct_ptr = direct_out;
        IPLAudioBuffer direct_buf = { .numChannels = 1, .numSamples = HRTF_FRAME_SIZE, .data = &direct_ptr };
        ptr_iplDirectEffectApply(ipl_direct_effects[i], &dirParams, &ipl_in_buffer, &direct_buf);

        // 3. Binaural Effect
        IPLBinauralEffectParams binParams = {0};
        binParams.interpolation = IPL_HRTFINTERPOLATION_BILINEAR;
        binParams.spatialBlend = 1.0f;
        binParams.hrtf = ipl_hrtf;
        binParams.direction.x = DotProduct(rel_pos, listener_right);
        binParams.direction.y = DotProduct(rel_pos, listener_up);
        binParams.direction.z = -DotProduct(rel_pos, listener_forward);

        if (len > 0.1f) { 
            float inv = 1.0f/len; 
            binParams.direction.x *= inv; binParams.direction.y *= inv; binParams.direction.z *= inv; 
        } else {
            binParams.direction.z = -1.0f;
        }
        ptr_iplBinauralEffectApply(ipl_effects[i], &binParams, &direct_buf, &ipl_out_buffer);

        // 4. Mix Output
        for (int s = 0; s < HRTF_FRAME_SIZE; s += 4) {
            _mm_store_ps(&mix_l[s], _mm_add_ps(_mm_load_ps(&mix_l[s]), _mm_load_ps(&out_channel_data[0][s])));
            _mm_store_ps(&mix_r[s], _mm_add_ps(_mm_load_ps(&mix_r[s]), _mm_load_ps(&out_channel_data[1][s])));
        }
        if (ch->autosound || sc->loopstart >= 0)
            ch->end = paintedtime + HRTF_FRAME_SIZE + sc->length;
        S_ProcessChannelSamples(ch, HRTF_FRAME_SIZE);
    }

    // 5. Delicate Soft Limiter & Final Interleaved PCM
    short pcm[HRTF_FRAME_SIZE * 2] __attribute__((aligned(16)));
    const qboolean swap_lr = (s_swapstereo && s_swapstereo->integer);
    __m128 v_scale = _mm_set1_ps(32767.0f);
    __m128 v_one   = _mm_set1_ps(1.0f);
    __m128 v_limit = _mm_set1_ps(0.12f);

    for (int s = 0; s < HRTF_FRAME_SIZE; s += 4) {
        __m128 l = _mm_load_ps(&mix_l[s]);
        __m128 r = _mm_load_ps(&mix_r[s]);

        __m128 l2 = _mm_mul_ps(l, l);
        __m128 r2 = _mm_mul_ps(r, r);
        
        l = _mm_mul_ps(l, _mm_sub_ps(v_one, _mm_mul_ps(l2, v_limit)));
        r = _mm_mul_ps(r, _mm_sub_ps(v_one, _mm_mul_ps(r2, v_limit)));

        __m128i i_l = _mm_cvtps_epi32(_mm_mul_ps(l, v_scale));
        __m128i i_r = _mm_cvtps_epi32(_mm_mul_ps(r, v_scale));

        __m128i first = swap_lr ? i_r : i_l;
        __m128i second = swap_lr ? i_l : i_r;
        __m128i packed = _mm_packs_epi32(_mm_unpacklo_epi32(first, second), 
                                         _mm_unpackhi_epi32(first, second));
        _mm_store_si128((__m128i*)&pcm[s*2], packed);
    }

    S_Audio_Write(pcm, (int)sizeof(pcm));
    paintedtime += HRTF_FRAME_SIZE;
}

// snd_hrtf.c - S_HRTF_Update

static void S_HRTF_Update(const vec3_t origin, const vec3_t forward, const vec3_t right, const vec3_t up) {
    if (!sound_started || !ipl_hrtf) return;

    VectorCopy(origin, listener_origin); 
    VectorCopy(forward, listener_forward);
    VectorCopy(right, listener_right); 
    VectorCopy(up, listener_up);

    // Zarządzanie osieroconymi kanałami autosound
    qboolean was_autosound[MAX_CHANNELS];
    for (int i = 0; i < MAX_CHANNELS; i++) {
        was_autosound[i] = channels[i].autosound;
        channels[i].autosound = false;
    }

    if (s_ambient->integer && cls.state == ca_active) 
        S_AddLoopSounds();

    // Jeśli dźwięk był autosoundem, a już go nie ma w AddLoopSounds -> zabij kanał
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (was_autosound[i] && !channels[i].autosound) {
            memset(&channels[i], 0, sizeof(channel_t));
        }
    }

    int target = (int)(s_mixahead->value * dma.speed);
    while ((int)(S_Audio_AvailableToRead() / (dma.channels * 2)) < target) {
        if ((int)S_Audio_AvailableToWrite() < (int)(HRTF_FRAME_SIZE * dma.channels * 2)) break;

        SDL_LockMutex(s_sound_mutex);
        while (s_pendingplays.next != &s_pendingplays && s_pendingplays.next->begin <= paintedtime) {
            S_IssuePlaysound(s_pendingplays.next);
        }
        SDL_UnlockMutex(s_sound_mutex);

        S_HRTF_MixFrame();
    }
}

static void S_HRTF_Shutdown(void) {
    for (int i = 0; i < MAX_CHANNELS; i++) {
        if (ipl_effects[i]) {
            ptr_iplBinauralEffectRelease(&ipl_effects[i]);
            ipl_effects[i] = NULL;
        }
        if (ipl_direct_effects[i]) {
            ptr_iplDirectEffectRelease(&ipl_direct_effects[i]);
            ipl_direct_effects[i] = NULL;
        }
        hrtf_last_sfx[i] = NULL;
        hrtf_last_entnum[i] = 0;
    }
    if (ipl_hrtf) {
        ptr_iplHRTFRelease(&ipl_hrtf);
        ipl_hrtf = NULL;
    }
    if (ipl_context) {
        ptr_iplContextRelease(&ipl_context);
        ipl_context = NULL;
    }
    free(in_channel_data[0]);
    free(out_channel_data[0]);
    free(out_channel_data[1]);
    in_channel_data[0] = NULL;
    out_channel_data[0] = NULL;
    out_channel_data[1] = NULL;
    if (phonon_handle) {
        dlclose(phonon_handle);
        phonon_handle = NULL;
    }
}

sound_export_t snd_hrtf_export = { "steamaudio", S_HRTF_Init, S_HRTF_Shutdown, S_HRTF_Update, NULL, NULL };