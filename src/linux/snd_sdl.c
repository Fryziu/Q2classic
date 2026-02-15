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
extern cvar_t *s_khz;

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

		// len to bajty, zamieniamy na sample (16-bit stereo = 4 bajty/sample)
    	int bytes_per_sample = dma.channels * (dma.samplebits / 8);
    	atomic_fetch_add(&total_samples_played, len / bytes_per_sample);
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
    
    if (!sdl_audio_initialized) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            Com_Printf ("Couldn't init SDL audio: %s\n", SDL_GetError());
            return false;
        }
        sdl_audio_initialized = true;
    }

    // Pobieramy s_khz i s_force_mono (już widoczne dzięki snd_loc.h)
    int freq = s_khz ? (int)s_khz->value : 44100;
    int force_mono = s_force_mono ? s_force_mono->integer : 0;

    // Restrykcyjna walidacja: tylko 44100 lub 48000
    if (freq != 44100 && freq != 48000) {
        Com_Printf("Sound: %d Hz is not supported. Forcing 44100 Hz.\n", freq);
        freq = 44100;
        Cvar_FullSet("s_khz", "44100", CVAR_ARCHIVE | CVAR_LATCHED);
    }

    desired.freq = freq;
    desired.format = AUDIO_S16SYS;
    desired.channels = (s_force_mono && s_force_mono->integer) ? 1 : 2;
    desired.samples = 512; //1024; // Stabilny bufor dla 44.1/48kHz
    desired.callback = sdl_audio_callback;
    desired.userdata = NULL;

    audio_device_id = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (audio_device_id == 0) {
        Com_Printf ("SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
        return false;
    }

    dma.samplebits = SDL_AUDIO_BITSIZE(obtained.format);
    dma.speed = obtained.freq;
    dma.channels = obtained.channels;
    dma.samples = obtained.samples;
	dma.samples = AUDIO_RING_BUFFER_SIZE / (dma.channels * (dma.samplebits / 8)); // poprawka
    dma.samplepos = 0;
    
    SDL_PauseAudioDevice(audio_device_id, 0);

    snd_inited = true;
    return true;
}

int SNDDMA_GetDMAPos (void)
{
	if(!snd_inited)
		return 0;
// Silnik oczekuje pozycji w buforze kołowym dma.samples
    return (int)(atomic_load(&total_samples_played) % dma.samples);
//	return dma.samplepos;
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
