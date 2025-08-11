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
// snd_mem.c: sound caching
#include "client.h"
#include "snd_loc.h"
#include "SDL/SDL.h"

// ====================================================================
// S_LoadSound using SDL's robust conversion and resampling
// ====================================================================
sfxcache_t *S_LoadSound (sfx_t *s)
{
    char	    namebuffer[MAX_QPATH], *name;
	byte	    *wav_file_data;
	int		    wav_file_size;
	sfxcache_t	*sc;

	if (s->name[0] == '*') return NULL;
	if (s->cache) return s->cache;

	name = (s->truename) ? s->truename : s->name;
	if (name[0] == '#') Q_strncpyz(namebuffer, &name[1], sizeof(namebuffer));
	else Com_sprintf(namebuffer, sizeof(namebuffer), "sound/%s", name);

	wav_file_size = FS_LoadFile(namebuffer, (void **)&wav_file_data);
	if (!wav_file_data) {
		Com_DPrintf ("Couldn't load %s\n", namebuffer);
		return NULL;
	}

	SDL_RWops *rw = SDL_RWFromConstMem(wav_file_data, wav_file_size);
	if (!rw) {
		FS_FreeFile(wav_file_data);
		Com_Printf("SDL_RWFromConstMem failed: %s\n", SDL_GetError());
		return NULL;
	}

	SDL_AudioSpec wav_spec;
	Uint32 wav_length;
	Uint8 *wav_buffer;

	if (SDL_LoadWAV_RW(rw, 1, &wav_spec, &wav_buffer, &wav_length) == NULL) {
		FS_FreeFile(wav_file_data);
		Com_Printf("SDL_LoadWAV_RW failed for %s: %s\n", name, SDL_GetError());
		return NULL;
	}
    FS_FreeFile(wav_file_data); // Original file data is no longer needed

    SDL_AudioCVT cvt;
    // We want all sounds to be converted to the mixer's format (e.g., 44100Hz, 16-bit, Stereo)
	int conversion_needed = SDL_BuildAudioCVT(&cvt,
                                            wav_spec.format, wav_spec.channels, wav_spec.freq,
                                            AUDIO_S16SYS, dma.channels, dma.speed);

	if (conversion_needed == -1) {
		Com_Printf("Cannot build audio converter for %s!\n", name);
		SDL_FreeWAV(wav_buffer);
		return NULL;
	}

    if (conversion_needed) {
        cvt.len = wav_length;
        cvt.buf = (Uint8 *)malloc(cvt.len * cvt.len_mult);
        if (!cvt.buf) {
            Com_Printf("Out of memory for audio conversion buffer!\n");
            SDL_FreeWAV(wav_buffer);
            return NULL;
        }
        memcpy(cvt.buf, wav_buffer, wav_length);
        SDL_ConvertAudio(&cvt);
        SDL_FreeWAV(wav_buffer); // Free the original WAV data
        wav_buffer = cvt.buf;    // Point to the new, converted data
        wav_length = cvt.len_cvt;
    }

	// Allocate our internal cache structure
	sc = Z_TagMalloc(sizeof(sfxcache_t) + wav_length, TAG_CL_SOUNDCACHE);
    s->cache = sc;

	sc->length = wav_length / (dma.channels * (dma.samplebits / 8));
	sc->loopstart = -1;
	sc->speed = dma.speed;
	sc->width = dma.samplebits / 8;
	sc->channels = dma.channels;

	memcpy(sc->data, wav_buffer, wav_length);

	free(wav_buffer); // Free the final buffer (either from SDL_LoadWAV or our conversion)

	return sc;
}
