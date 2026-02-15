 /*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include <emmintrin.h> // SSE2
#include <smmintrin.h> // SSE4.1 for S_PaintChannelFrom16 & S_TransferPaintBuffer
#include "client.h"
#include "snd_loc.h"
#include "SDL2/SDL.h"


#define	PAINTBUFFER_SIZE	2048
static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE] __attribute__((aligned(16)));

/// Mixing and Transfer Functions

void S_TransferPaintBuffer(int endtime)
{
    int count = endtime - paintedtime;
    if (count <= 0) return;
    if (count > PAINTBUFFER_SIZE) count = PAINTBUFFER_SIZE;

    static short pcm_buffer[PAINTBUFFER_SIZE * 2] __attribute__((aligned(16)));
    
    int i = 0;
    // Przetwarzamy po 8 sampli (16 wartości L/R) na raz
    for (; i <= count - 8; i += 8)
    {
        // Załadowanie 32-bitowych intów (L, R) - paintbuffer musi być aligned 16!
        __m128i in01 = _mm_load_si128((__m128i*)&paintbuffer[i]);     
        __m128i in23 = _mm_load_si128((__m128i*)&paintbuffer[i + 2]); 
        __m128i in45 = _mm_load_si128((__m128i*)&paintbuffer[i + 4]); 
        __m128i in67 = _mm_load_si128((__m128i*)&paintbuffer[i + 6]); 

        // Skalowanie o 8 bitów (Quake 2 mixer standard)
        in01 = _mm_srai_epi32(in01, 8);
        in23 = _mm_srai_epi32(in23, 8);
        in45 = _mm_srai_epi32(in45, 8);
        in67 = _mm_srai_epi32(in67, 8);

        // Pakowanie z nasyceniem (32-bit int -> 16-bit short)
        // _mm_packs_epi32 automatycznie dba o to, by wartości nie wyszły poza -32768 do 32767
        __m128i packed03 = _mm_packs_epi32(in01, in23); 
        __m128i packed47 = _mm_packs_epi32(in45, in67); 

        _mm_store_si128((__m128i*)&pcm_buffer[i * 2], packed03);
        _mm_store_si128((__m128i*)&pcm_buffer[(i + 4) * 2], packed47);
    }

    // Pozostałe sample (tail) wykonujemy skalarnie
    for (; i < count; i++) {
        int l = paintbuffer[i].left >> 8;
        int r = paintbuffer[i].right >> 8;
        pcm_buffer[i * 2]     = (short)bound(-32768, l, 32767);
        pcm_buffer[i * 2 + 1] = (short)bound(-32768, r, 32767);
    }

    S_Audio_Write(pcm_buffer, count * dma.channels * sizeof(short));
}

///
void S_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int count, int offset)
{
	(void)ch;
	(void)sc;
	(void)count;
	(void)offset;
	/* Implement if needed */
}


/// S_PaintChannelFrom16 handling both Mono and Stereo

void S_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count, int offset)
{
    int leftvol = (int)(ch->leftvol * s_volume->value);
    int rightvol = (int)(ch->rightvol * s_volume->value);

    if (leftvol <= 0 && rightvol <= 0) {
        S_ProcessChannelSamples(ch, count);
        return;
    }

    portable_samplepair_t * restrict samp = &paintbuffer[offset];
    const short * restrict sfx_data = (const short *)sc->data;
    int pos = ch->pos;

    // Przygotuj wektor głośności: [R, L, R, L] (mapowanie do struktury portable_samplepair_t)
    __m128i v_vol = _mm_set_epi32(rightvol, leftvol, rightvol, leftvol);

    int i = 0;
    if (sc->channels == 1) // MONO
    {
        for (; i <= count - 2; i += 2) {
            short s0 = sfx_data[pos + i];
            short s1 = sfx_data[pos + i + 1];

            // Tworzymy wektor 4 krótkich wartości: [s1, s1, s0, s0]
            // i rozszerzamy je do 32-bitowych intów w jednym kroku SSE4.1
            __m128i s16 = _mm_set_epi16(0, 0, 0, 0, s1, s1, s0, s0);
            __m128i s32 = _mm_cvtepi16_epi32(s16); 

            __m128i mixed = _mm_mullo_epi32(s32, v_vol);
            __m128i dest = _mm_loadu_si128((__m128i*)&samp[i]);
            _mm_storeu_si128((__m128i*)&samp[i], _mm_add_epi32(dest, mixed));
        }
    }
    else // STEREO
    {
        for (; i <= count - 2; i += 2) {
            // Ładujemy 4 wartości 16-bit (L0, R0, L1, R1) bezpośrednio
            __m128i s16 = _mm_loadl_epi64((const __m128i *)&sfx_data[(pos + i) * 2]);
            // Rozszerzamy dolne 64-bity (4 shorts) do 128-bitów (4 ints)
            __m128i s32 = _mm_cvtepi16_epi32(s16);

            __m128i mixed = _mm_mullo_epi32(s32, v_vol);
            __m128i dest = _mm_loadu_si128((__m128i*)&samp[i]);
            _mm_storeu_si128((__m128i*)&samp[i], _mm_add_epi32(dest, mixed));
        }
    }

    // Tail (skalarny fallback)
    for (; i < count; i++) {
        if (sc->channels == 1) {
            int s = sfx_data[pos + i];
            samp[i].left  += s * leftvol;
            samp[i].right += s * rightvol;
        } else {
            samp[i].left  += sfx_data[(pos + i) * 2] * leftvol;
            samp[i].right += sfx_data[(pos + i) * 2 + 1] * rightvol;
        }
    }

    S_ProcessChannelSamples(ch, count);
}


/// S_PaintChannels: The Heart of the Mixer 

void S_PaintChannels(int endtime)
{
    if (!sound_started || paintedtime >= endtime)
        return;

    while (paintedtime < endtime)
    {        
        SDL_LockMutex(s_sound_mutex);
        while (s_pendingplays.next != &s_pendingplays && s_pendingplays.next->begin <= paintedtime)
        {
            S_IssuePlaysound(s_pendingplays.next);
        }
        SDL_UnlockMutex(s_sound_mutex);

        int paint_count = endtime - paintedtime;
        if (paint_count > PAINTBUFFER_SIZE)
            paint_count = PAINTBUFFER_SIZE;

        SDL_LockMutex(s_sound_mutex);
        if (s_pendingplays.next != &s_pendingplays)
        {
            int time_to_next = s_pendingplays.next->begin - paintedtime;
            if (time_to_next > 0 && time_to_next < paint_count)
                paint_count = time_to_next;
        }
        SDL_UnlockMutex(s_sound_mutex);

        if (paint_count <= 0) break;

        memset(paintbuffer, 0, paint_count * sizeof(portable_samplepair_t));
        
        for (int i = 0; i < MAX_CHANNELS; i++)
        {
            channel_t *ch = &channels[i];
            if (!ch->sfx || (!ch->leftvol && !ch->rightvol)) continue;

              // DYNAMICZNA AKTUALIZACJA:
            // Jeśli dźwięk podąża za encją (!fixed_origin), uaktualnij jego pozycję L/R
            if (!ch->fixed_origin) {
                S_Spatialize(ch); 
            }

            sfxcache_t *sc = ch->sfx->cache;
            if (!sc) continue;

            int count = paint_count;
            if (paintedtime + count > ch->end)
                count = ch->end - paintedtime;

            if (count > 0)
                S_PaintChannelFrom16(ch, sc, count, 0);
        }

        for(int i = 0; i < MAX_CHANNELS; i++)
        {
            channel_t *ch = &channels[i];
            if (!ch->sfx || ch->end > (paintedtime + paint_count)) continue;

            sfxcache_t *sc = ch->sfx->cache;
            if (ch->autosound) {
                ch->pos = 0; ch->end = (paintedtime + paint_count) + sc->length;
            } else if (sc->loopstart >= 0) {
                ch->pos = sc->loopstart; ch->end = (paintedtime + paint_count) + (sc->length - sc->loopstart);
            } else {
                ch->sfx = NULL;
            }
        }

        S_TransferPaintBuffer(paintedtime + paint_count);
        paintedtime += paint_count;
    }
}

// Producer - mixing and pushing

void S_MixAudio(void)
{
	if (!sound_started)
		return;

	int bytes_per_sample = dma.channels * (dma.samplebits / 8);	
	int target_buffer_level_samples = (int)(s_mixahead->value * dma.speed);
	
	if (target_buffer_level_samples < 512) target_buffer_level_samples = 512;
	
	int target_buffer_level_bytes = target_buffer_level_samples * bytes_per_sample;
	int available_to_read_bytes = S_Audio_AvailableToRead();

	if (available_to_read_bytes >= target_buffer_level_bytes)
		return;

	int samples_to_mix = (target_buffer_level_bytes - available_to_read_bytes) / bytes_per_sample;

	int available_to_write_samples = S_Audio_AvailableToWrite() / bytes_per_sample;
	if (samples_to_mix > available_to_write_samples)
		samples_to_mix = available_to_write_samples;

	if (samples_to_mix <= 0)
		return;

	int endtime = paintedtime + samples_to_mix;

	dma.buffer = NULL; 
	S_PaintChannels(endtime);	
}

/// HRTF

static void S_Soft_Update(const vec3_t origin, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up) {
	if (!sound_started) return;

	if (cls.disable_screen) return;
	s_volume->modified = false; 

	VectorCopy(origin, listener_origin);
	VectorCopy(v_forward, listener_forward);
	VectorCopy(v_right, listener_right);
	VectorCopy(v_up, listener_up);

	qboolean was_autosound[MAX_CHANNELS];

	for (int i=0; i<MAX_CHANNELS; i++) {
		was_autosound[i] = channels[i].autosound;
		channels[i].autosound = false;
	}

	if (s_ambient->integer && cls.state == ca_active)
		S_AddLoopSounds();

	for (int i=0; i<MAX_CHANNELS; i++)
	{
		if (!channels[i].sfx) continue;

		if (channels[i].autosound)
		{
			S_Spatialize(&channels[i]);
		}
		else
		{
			if (was_autosound[i])
			{
				memset(&channels[i], 0, sizeof(channel_t));
			}
			else
			{
				S_Spatialize(&channels[i]);
			}
		}
	}
    S_MixAudio();
}

sound_export_t snd_soft_export = {
    "software",
    NULL, // Init i Shutdown są na razie wspólne w snd_dma.c
    NULL,
    S_Soft_Update,
    NULL, // StartSound na razie idzie przez kolejkę s_pendingplays
    S_StopAllSounds
};