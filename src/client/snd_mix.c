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

#define	PAINTBUFFER_SIZE	2048
static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
static int		snd_scaletable[32][256];

/*
====================================================================
Mixing and Transfer Functions
====================================================================
*/

void S_TransferPaintBuffer(int endtime)
{
    int count = endtime - paintedtime;
    if (count <= 0) return;

    short *out = (short *)dma.buffer;
    portable_samplepair_t *in = paintbuffer;

    int samples_to_write = count;
    int start_pos = paintedtime % dma.samples;

    while (samples_to_write > 0)
    {
        int pos_in_dma = start_pos % dma.samples;
        int samples_this_run = dma.samples - pos_in_dma;
        if (samples_this_run > samples_to_write)
            samples_this_run = samples_to_write;

        for (int i = 0; i < samples_this_run; i++)
        {
            out[(pos_in_dma + i) * 2]     = bound(-32768, in[i].left >> 8, 32767);
            out[(pos_in_dma + i) * 2 + 1] = bound(-32768, in[i].right >> 8, 32767);
        }

        in += samples_this_run;
        samples_to_write -= samples_this_run;
        start_pos += samples_this_run;
    }
}

void S_InitScaletable (void)
{
	for (int i = 0; i < 32; i++)
		for (int j = 0; j < 256; j++)
			snd_scaletable[i][j] = ((signed char)(j - 128)) * (i * 8 * 256);
}

void S_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int count, int offset) { /* Implement if needed */ }

// ====================================================================
// FINAL, CORRECTED S_PaintChannelFrom16 handling both Mono and Stereo
// ====================================================================
void S_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count, int offset)
{
	int	leftvol = (ch->leftvol * s_volume->value);
	int	rightvol = (ch->rightvol * s_volume->value);

	portable_samplepair_t *samp = &paintbuffer[offset];

	if (sc->channels == 1) // Handle Mono sounds
	{
		short *sfx = (short *)sc->data + ch->pos;
		for (int i=0; i < count; i++) {
			int sample = *sfx++;
			samp[i].left += (sample * leftvol);
			samp[i].right += (sample * rightvol);
		}
	}
	else // Handle Stereo sounds
	{
		// CRITICAL FIX: Multiply position by number of channels
		short *sfx = (short *)sc->data + (ch->pos * 2);
		for (int i=0; i < count; i++) {
			samp[i].left += (*sfx++ * leftvol);
			samp[i].right += (*sfx++ * rightvol);
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

    // --- Main loop that guarantees forward progress in time ---
    while (paintedtime < endtime)
    {
        // Step 1: Process all sounds that are due to start *now* or are overdue.
        SDL_LockMutex(s_sound_mutex);
        while (s_pendingplays.next != &s_pendingplays && s_pendingplays.next->begin <= paintedtime)
        {
            S_IssuePlaysound(s_pendingplays.next);
        }
        SDL_UnlockMutex(s_sound_mutex);

        // Step 2: Determine the size of the next audio slice to mix.
        int paint_count = endtime - paintedtime;

        SDL_LockMutex(s_sound_mutex);
        if (s_pendingplays.next != &s_pendingplays)
        {
            int time_to_next = s_pendingplays.next->begin - paintedtime;
            if (time_to_next > 0 && time_to_next < paint_count)
            {
                paint_count = time_to_next;
            }
        }
        SDL_UnlockMutex(s_sound_mutex);

        if (paint_count > PAINTBUFFER_SIZE)
            paint_count = PAINTBUFFER_SIZE;

        // This check prevents a stuck loop if the next sound starts exactly now.
        if (paint_count <= 0)
        {
            // We've processed the sound at 'paintedtime', so loop again to recalculate.
            // A forced increment is a safety valve against any theoretical freeze.
            if (paintedtime < endtime) paintedtime++;
            continue;
        }

        // Step 3: Mix this slice.
        int paint_end = paintedtime + paint_count;
        memset(paintbuffer, 0, paint_count * sizeof(portable_samplepair_t));

        for (int i = 0; i < MAX_CHANNELS; i++)
        {
            channel_t *ch = &channels[i];
            if (!ch->sfx || (!ch->leftvol && !ch->rightvol)) continue;

            sfxcache_t *sc = ch->sfx->cache;
            if (!sc) continue;

            // Mix the part of the sound that falls into our current slice [paintedtime, paint_end)
            if (paintedtime < ch->end)
            {
                int mix_end_ch = (paint_end < ch->end) ? paint_end : ch->end;
                int count = mix_end_ch - paintedtime;
                if (count > 0)
                {
                    int offset = 0; // Offset is always 0 because we mix into a fresh buffer
                    if (sc->width == 1) S_PaintChannelFrom8(ch, sc, count, offset);
                    else S_PaintChannelFrom16(ch, sc, count, offset);
                }
            }
        }

        // Step 4: After mixing, check which channels have FINISHED by the end of this slice.
        // This is the CRITICAL FIX for the resource leak.
        for(int i = 0; i < MAX_CHANNELS; i++)
        {
            channel_t *ch = &channels[i];
            if (!ch->sfx) continue;

            if (ch->end <= paint_end)
            {
                sfxcache_t *sc = ch->sfx->cache;
                if (ch->autosound) {
                    ch->pos = 0; ch->end = paint_end + sc->length;
                } else if (sc->loopstart >= 0) {
                    ch->pos = sc->loopstart; ch->end = paint_end + (sc->length - sc->loopstart);
                } else {
                    ch->sfx = NULL; // Sound is finished, FREE THE CHANNEL.
                }
            }
        }

        // Step 5: Transfer the mixed slice to the DMA buffer.
        S_TransferPaintBuffer(paint_end);

        // Step 6: Advance time. This is the most crucial part.
        paintedtime = paint_end;
    }
}
