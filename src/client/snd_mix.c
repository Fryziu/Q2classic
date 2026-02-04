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
#include "client.h"
#include "snd_loc.h"
#include "SDL2/SDL.h"

#define	PAINTBUFFER_SIZE	2048
static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
static int		snd_scaletable[32][256];

/*
====================================================================
Mixing and Transfer Functions
====================================================================
*/
// ZMIANA: Ta funkcja nie pisze już do dma.buffer, ale pcha dane do naszej kolejki.
void S_TransferPaintBuffer(int endtime)
{
    int count = endtime - paintedtime;
    if (count <= 0) return;

	// Maksymalny rozmiar bufora malowania to PAINTBUFFER_SIZE.
    if (count > PAINTBUFFER_SIZE)
	{
		Com_Printf("S_TransferPaintBuffer: count > PAINTBUFFER_SIZE\n");
		return;
	}

	// Bufor `paintbuffer` zawiera teraz `count` zmiksowanych próbek (samples).
	// Musimy je przekonwertować z powrotem do formatu docelowego (np. 16-bit)
	// i zapisać do kolejki pierścieniowej.

	// Tworzymy tymczasowy bufor na dane do zapisu.
	static short pcm_buffer[PAINTBUFFER_SIZE * 2];

	portable_samplepair_t *in = paintbuffer;
	short *out = pcm_buffer;

	// Konwersja z formatu wewnętrznego (32-bit) na 16-bit PCM.
	for (int i = 0; i < count; i++)
	{
		out[i * 2]     = bound(-32768, in[i].left >> 8, 32767);
		out[i * 2 + 1] = bound(-32768, in[i].right >> 8, 32767);
	}

	// Oblicz, ile to bajtów i zapisz do kolejki.
	int bytes_to_write = count * dma.channels * (dma.samplebits / 8);
	S_Audio_Write(pcm_buffer, bytes_to_write);
}
///
void S_InitScaletable (void)
{
	for (int i = 0; i < 32; i++)
		for (int j = 0; j < 256; j++)
			snd_scaletable[i][j] = ((signed char)(j - 128)) * (i * 8 * 256);
}

void S_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int count, int offset)
{
	(void)ch;
	(void)sc;
	(void)count;
	(void)offset;
	/* Implement if needed */
}

// ====================================================================
// FINAL, CORRECTED S_PaintChannelFrom16 handling both Mono and Stereo
// ====================================================================
void S_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count, int offset)
{
	// Wyliczamy głośność raz dla całego bloku
	float vol = s_volume->value;
	int leftvol = (int)(ch->leftvol * vol);
	int rightvol = (int)(ch->rightvol * vol);

	// Jeśli kanał jest niemy, tylko przesuwamy wskaźnik pozycji
	if (leftvol <= 0 && rightvol <= 0)
	{
		ch->pos += count;
		return;
	}

	portable_samplepair_t *samp = &paintbuffer[offset];

	if (sc->channels == 1) // Mono
	{
		short *sfx = (short *)sc->data + ch->pos;
		for (int i=0; i < count; i++) {
			int sample = sfx[i];
			samp[i].left += (sample * leftvol);
			samp[i].right += (sample * rightvol);
		}
	}
	else // Stereo
	{
		short *sfx = (short *)sc->data + (ch->pos * 2);
		for (int i=0; i < count; i++) {
			samp[i].left += (sfx[i*2] * leftvol);
			samp[i].right += (sfx[i*2+1] * rightvol);
		}
	}
	ch->pos += count;
}
/*
====================================================================
S_PaintChannels: The Heart of the Mixer (Final, Robust Version)
====================================================================
*/
void S_PaintChannels(int endtime)
{
    if (!sound_started || paintedtime >= endtime)
        return;

    while (paintedtime < endtime)
    {
        // 1. Obsłuż oczekujące dźwięki
        SDL_LockMutex(s_sound_mutex);
        while (s_pendingplays.next != &s_pendingplays && s_pendingplays.next->begin <= paintedtime)
        {
            S_IssuePlaysound(s_pendingplays.next);
        }
        SDL_UnlockMutex(s_sound_mutex);

        // 2. Oblicz krok (nie więcej niż do następnego dźwięku lub końca bufora)
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

        if (paint_count <= 0) break; // Powinno być niemożliwe, ale bezpieczeństwo przede wszystkim

        // 3. Miksujemy
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

        // 4. Zarządzanie pętlami i zwalnianie kanałów
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
/*
====================================================================
ZMIANA: Główna funkcja "Producenta" - miksuje audio i pcha do kolejki
====================================================================
*/
void S_MixAudio(void)
{
	if (!sound_started)
		return;

	int bytes_per_sample = dma.channels * (dma.samplebits / 8);
	
	// Używamy s_mixahead (np. 0.1 = 100ms) zamiast sztywnej wartości 2048
	int target_buffer_level_samples = (int)(s_mixahead->value * dma.speed);
	
	// Zabezpieczenie: minimalny i maksymalny bufor (0.01s - 0.5s)
	if (target_buffer_level_samples < 512) target_buffer_level_samples = 512;
	
	int target_buffer_level_bytes = target_buffer_level_samples * bytes_per_sample;
	int available_to_read_bytes = S_Audio_AvailableToRead();

	if (available_to_read_bytes >= target_buffer_level_bytes)
		return;

	int samples_to_mix = (target_buffer_level_bytes - available_to_read_bytes) / bytes_per_sample;

	// Sprawdź miejsce w kolejce
	int available_to_write_samples = S_Audio_AvailableToWrite() / bytes_per_sample;
	if (samples_to_mix > available_to_write_samples)
		samples_to_mix = available_to_write_samples;

	if (samples_to_mix <= 0)
		return;

	int endtime = paintedtime + samples_to_mix;

	dma.buffer = NULL; 
	S_PaintChannels(endtime); 
	// paintedtime jest aktualizowane wewnątrz S_PaintChannels
}

// HRTF

static void S_Soft_Update(const vec3_t origin, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up) {
	if (!sound_started) return;

	S_MixAudio();

	if (cls.disable_screen) return;
	if (s_volume->modified) { S_InitScaletable(); s_volume->modified = false; }

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