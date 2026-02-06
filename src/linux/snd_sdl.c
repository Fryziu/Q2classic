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

// snd_sdl.c  - Gemini 3 Flash [X-JC-STRICTOR-V2] 2026-02-04
#include "SDL2/SDL.h"

#include "../client/client.h"
#include "../client/snd_loc.h"

static SDL_AudioDeviceID audio_device_id = 0;
static qboolean snd_inited = false;

cvar_t *sndbits;
cvar_t *sndspeed;
cvar_t *sndchannels;

void Snd_Memset (void* dest, const int val, const size_t count)
{
	memset(dest,val,count);
}

static void sdl_audio_callback (void *unused, Uint8 * stream, int len)
{
	if (!snd_inited)
	{		
		memset(stream, 0, len);
		return;
	}

	int available = S_Audio_AvailableToRead();

	if (available >= len)
	{		
		S_Audio_Read(stream, len);
	}
	else
	{
		if (available > 0)
		{
			S_Audio_Read(stream, available);
		}
		int silence = (dma.samplebits == 8) ? 128 : 0;
		memset(stream + available, silence, len - available);
	}
}

qboolean SNDDMA_Init (void)
{
	SDL_AudioSpec desired, obtained;  
	static qboolean sdl_audio_initialized = false;

	if(snd_inited)
		return true;
	
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
	
	// Wymuszamy S16SYS. Mikser (snd_mix.c) produkuje tylko 16-bit.
	// Sprzętowa konwersja 8-bit -> 16-bit jest zbędna i szkodliwa przy tym potoku.
	desired.format = AUDIO_S16SYS;
	
	desired.channels = (sndchannels->integer == 1) ? 1 : 2;
	desired.samples = 1024;
	desired.callback = sdl_audio_callback;
	desired.userdata = NULL;

	audio_device_id = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
	if (audio_device_id == 0) {
		Com_Printf ("SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
		return false;
	}

	// Poprawne pobranie głębi bitowej z formatu SDL
	dma.samplebits = SDL_AUDIO_BITSIZE(obtained.format);
	
	dma.speed = obtained.freq;
	dma.channels = obtained.channels;
	dma.samples = obtained.samples;
	dma.samplepos = 0;
	dma.submission_chunk = 1;
	dma.buffer = NULL;
  
    SDL_PauseAudioDevice(audio_device_id, 0);

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

	if (audio_device_id != 0)
	{
		SDL_PauseAudioDevice(audio_device_id, 1); 
		SDL_CloseAudioDevice(audio_device_id);
		audio_device_id = 0;
	}

	snd_inited = false;
}

void SNDDMA_Submit (void)
{
	
}

void SNDDMA_BeginPainting(void)
{
	
}
