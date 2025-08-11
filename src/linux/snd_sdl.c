/*
	snd_sdl.c

	Sound code taken from SDLQuake and modified to work with Quake2
	Robert B�uml 2001-12-25

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

	$Id: snd_sdl.c,v 1.2 2002/02/09 20:29:38 relnev Exp $
*/

#include "SDL.h"

#include "../client/client.h"
#include "../client/snd_loc.h"

static qboolean snd_inited = false;

cvar_t *sndbits;
cvar_t *sndspeed;
cvar_t *sndchannels;

void Snd_Memset (void* dest, const int val, const size_t count)
{
	memset(dest,val,count);
}

// Ten callback jest sercem systemu. Jego poprawne działanie jest kluczowe.
static void sdl_audio_callback (void *unused, Uint8 * stream, int len)
{
	if (!snd_inited)
		return;

	// Zaktualizuj licznik czasu audio (dma.samplepos)
	// To jest POPRAWIONA, solidna matematyka.
	int bytes_per_frame = (dma.samplebits / 8) * dma.channels;
	if (bytes_per_frame > 0)
	{
		int num_frames = len / bytes_per_frame;
		dma.samplepos += num_frames;
	}

	// Przekaż wskaźnik do bufora i zaktualizowany czas do miksera
	dma.buffer = stream;
	S_PaintChannels (dma.samplepos);
}
qboolean SNDDMA_Init (void)
{
	SDL_AudioSpec desired, obtained;
    // Static flag to ensure SDL_Init is only called once.
	static qboolean sdl_audio_initialized = false;

	if(snd_inited)
		return true;

	// --- THE CRITICAL FIX: PART 1 ---
    // Initialize the SDL audio subsystem only ONCE per application run.
	if (!sdl_audio_initialized)
	{
		if (SDL_Init(SDL_INIT_AUDIO) < 0) {
			Com_Printf ("Couldn't init SDL audio: %s\n", SDL_GetError());
			return false;
		}
		sdl_audio_initialized = true;
	}

	if (!sndbits) {
		sndbits = Cvar_Get("sndbits", "16", CVAR_ARCHIVE);
		sndspeed = Cvar_Get("sndspeed", "44100", CVAR_ARCHIVE);
		sndchannels = Cvar_Get("sndchannels", "2", CVAR_ARCHIVE);
	}

	desired.freq = sndspeed->integer ? sndspeed->integer : 44100;
	desired.format = (sndbits->integer == 8) ? AUDIO_U8 : AUDIO_S16SYS;
	desired.channels = (sndchannels->integer == 1) ? 1 : 2;
	desired.samples = 1024;
	desired.callback = sdl_audio_callback;
	desired.userdata = NULL;

	// Open the audio device. This is safe to do multiple times.
	if (SDL_OpenAudio (&desired, &obtained) < 0) {
		Com_Printf ("SDL_OpenAudio() failed: %s\n", SDL_GetError());
		return false;
	}

	// Fill the DMA block with the OBTAINED values.
	dma.samplebits = (obtained.format & 0xFF);
	dma.speed = obtained.freq;
	dma.channels = obtained.channels;
	dma.samples = obtained.samples;
	dma.samplepos = 0;
	dma.submission_chunk = 1;
	dma.buffer = NULL;

    SDL_PauseAudio(0);

	snd_inited = true;
	return true;
}

int SNDDMA_GetDMAPos (void)
{
	if(!snd_inited)
		return 0;

	return dma.samplepos;
}

void SNDDMA_Shutdown (void)
{
	if (!snd_inited)
		return;

	// --- THE CRITICAL FIX: PART 2 ---
    // We ONLY close the device. We DO NOT quit the SDL subsystem here.
	SDL_PauseAudio(1);
	SDL_CloseAudio();

	snd_inited = false;
}

void SNDDMA_Submit (void)
{
	// Puste w implementacjach SDL, gdzie callback robi wszystko
}

void SNDDMA_BeginPainting(void)
{
	// Puste w implementacjach SDL
}
