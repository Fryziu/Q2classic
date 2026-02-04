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
// snd_sdl.c
#include "SDL2/SDL.h"

#include "../client/client.h"
#include "../client/snd_loc.h"

// ZMIANA: Dodajemy zmienną do przechowywania ID naszego urządzenia audio.
static SDL_AudioDeviceID audio_device_id = 0;
static qboolean snd_inited = false;

cvar_t *sndbits;
cvar_t *sndspeed;
cvar_t *sndchannels;

void Snd_Memset (void* dest, const int val, const size_t count)
{
	memset(dest,val,count);
}

// Ten callback jest sercem systemu. Jego poprawne działanie jest kluczowe.
// ZMIANA: Całkowicie nowa, uproszczona wersja callbacku audio.
// Jego jedynym zadaniem jest pobieranie gotowych danych z naszej kolejki.
static void sdl_audio_callback (void *unused, Uint8 * stream, int len)
{
	if (!snd_inited)
	{
		// Na wszelki wypadek, wypełnij ciszą, jeśli callback zostanie wywołany za wcześnie
		memset(stream, 0, len);
		return;
	}

	int available = S_Audio_AvailableToRead();

	if (available >= len)
	{
		// Mamy wystarczająco danych, po prostu je skopiuj
		S_Audio_Read(stream, len);
	}
	else
	{
		// Buffer underrun! Nie mamy wystarczająco danych.
		// To się zdarza, jeśli główny wątek gry ma "czkawkę" (np. ładowanie).
		// Musimy uniknąć trzasków, dostarczając ciszę.

		// 1. Skopiuj tyle danych, ile mamy.
		if (available > 0)
		{
			S_Audio_Read(stream, available);
		}

		// 2. Wypełnij resztę bufora ciszą.
		// Wartość "ciszy" zależy od formatu audio. Dla 16-bit (AUDIO_S16SYS) to 0.
		int silence = (dma.samplebits == 8) ? 128 : 0;
		memset(stream + available, silence, len - available);
	}
}

////
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

	// ZMIANA: Używamy nowej funkcji SDL2 do otwarcia urządzenia.
	// NULL oznacza domyślne urządzenie wyjściowe.
	// 0 oznacza, że zmiany w formacie nie są dozwolone.
	audio_device_id = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
	if (audio_device_id == 0) {
		Com_Printf ("SDL_OpenAudioDevice() failed: %s\n", SDL_GetError());
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

    // ZMIANA: Odpauzowujemy konkretne urządzenie, a nie globalny system.
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

	// ZMIANA: Zamykamy konkretne urządzenie audio, które otworzyliśmy.
    // Nie wywołujemy SDL_QuitSubSystem, aby inne części programu mogły dalej działać.
	if (audio_device_id != 0)
	{
		SDL_PauseAudioDevice(audio_device_id, 1); // Jawne zatrzymanie urządzenia
		SDL_CloseAudioDevice(audio_device_id);
		audio_device_id = 0;
	}

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
