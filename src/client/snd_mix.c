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
// Gemini 3 Flash [X-JC-STRICTOR-V2] 2026-02-04
#include "client.h"
#include "snd_loc.h"
#include "SDL2/SDL.h"

#define	PAINTBUFFER_SIZE	2048
static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];

/// Mixing and Transfer Functions

void S_TransferPaintBuffer(int endtime)
{
    int count = endtime - paintedtime;
    if (count <= 0) return;
    if (count > PAINTBUFFER_SIZE) count = PAINTBUFFER_SIZE;

    static short pcm_buffer[PAINTBUFFER_SIZE * 2];
    
    // Wskaźniki z restrict
    const portable_samplepair_t * restrict in = paintbuffer;
    short * restrict out = pcm_buffer;
    
    for (int i = 0; i < count; i++)
    {
        // Przesunięcie bitowe >> 8 jest szybkie i zgodne z tablicą głośności (x256).
        // Ręczny clamping jest bardziej czytelny dla kompilatora niż generyczne makro bound().
        
        int l = in[i].left >> 8;
        int r = in[i].right >> 8;

        if (l > 32767) l = 32767;
        else if (l < -32768) l = -32768;

        if (r > 32767) r = 32767;
        else if (r < -32768) r = -32768;

        out[i * 2]     = (short)l;
        out[i * 2 + 1] = (short)r;
    }   

    // Zakładamy 16-bit, bo tak ustawiliśmy w snd_sdl.c
    int bytes_to_write = count * 2 * sizeof(short); 
    S_Audio_Write(pcm_buffer, bytes_to_write);
}

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
    float vol = s_volume->value;
    int leftvol = (int)(ch->leftvol * vol);
    int rightvol = (int)(ch->rightvol * vol);

    // Optymalizacja: Pomiń ciszę
    if (leftvol <= 0 && rightvol <= 0)
    {
        ch->pos += count;
        return;
    }

    // Użycie 'restrict' informuje kompilator, że te obszary pamięci się nie nakładają.
    // Umożliwia to agresywną wektoryzację (SSE/AVX).
    portable_samplepair_t * restrict samp = &paintbuffer[offset];
    
    // Pobieramy wskaźnik na dane
    const short * restrict sfx_data = (const short *)sc->data;

    int pos = ch->pos;

    if (sc->channels == 1) // Mono
    {
        for (int i = 0; i < count; i++) {
            int sample = sfx_data[pos + i];
            samp[i].left  += (sample * leftvol);
            samp[i].right += (sample * rightvol);
        }
    }
    else // Stereo
    {
        for (int i = 0; i < count; i++) {
            int idx = (pos + i) * 2;
            samp[i].left  += (sfx_data[idx] * leftvol);
            samp[i].right += (sfx_data[idx+1] * rightvol);
        }
    }
    ch->pos += count;
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

	S_MixAudio();

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
}

sound_export_t snd_soft_export = {
    "software",
    NULL, // Init i Shutdown są na razie wspólne w snd_dma.c
    NULL,
    S_Soft_Update,
    NULL, // StartSound na razie idzie przez kolejkę s_pendingplays
    S_StopAllSounds
};