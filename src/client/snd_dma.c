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
// snd_dma.c -- main control for any streaming sound output device
#include "client.h"
#include "../qcommon/qcommon.h"
#include "snd_loc.h"
#include "SDL2/SDL.h"

/*
====================================================================
ZMIANA: Implementacja bezblokadowej kolejki pierścieniowej
====================================================================
*/

// Globalna instancja naszej kolejki
ring_buffer_t s_ringbuffer;

// Inicjalizuje kolejkę (ustawia wskaźniki na zero)
static void S_Audio_InitRingBuffer(void)
{
	atomic_init(&s_ringbuffer.head, 0);
	atomic_init(&s_ringbuffer.tail, 0);
	memset(s_ringbuffer.buffer, 0, AUDIO_RING_BUFFER_SIZE);
}

// Zwraca ilość bajtów, które można bezpiecznie odczytać z kolejki
int S_Audio_AvailableToRead(void)
{
	int head = atomic_load(&s_ringbuffer.head);
	int tail = atomic_load(&s_ringbuffer.tail);
	return (tail - head) & (AUDIO_RING_BUFFER_SIZE - 1);
}

// Zwraca ilość wolnego miejsca do zapisu w kolejce
int S_Audio_AvailableToWrite(void)
{
	int head = atomic_load(&s_ringbuffer.head);
	int tail = atomic_load(&s_ringbuffer.tail);
	// Zostawiamy 1 bajt wolnego, aby odróżnić kolejkę pełną od pustej
	return (head - tail - 1) & (AUDIO_RING_BUFFER_SIZE - 1);
}

// Odczytuje 'count' bajtów z kolejki do 'dest'
void S_Audio_Read(void* dest, int count)
{
	int head = atomic_load(&s_ringbuffer.head);
	byte* p_dest = (byte*)dest;

	for (int i = 0; i < count; i++)
	{
		p_dest[i] = s_ringbuffer.buffer[head];
		head = (head + 1) & (AUDIO_RING_BUFFER_SIZE - 1);
	}
	atomic_store(&s_ringbuffer.head, head);
}

// Zapisuje 'count' bajtów z 'src' do kolejki
void S_Audio_Write(const void* src, int count)
{
	int tail = atomic_load(&s_ringbuffer.tail);
	const byte* p_src = (const byte*)src;

	for (int i = 0; i < count; i++)
	{
		s_ringbuffer.buffer[tail] = p_src[i];
		tail = (tail + 1) & (AUDIO_RING_BUFFER_SIZE - 1);
	}
	atomic_store(&s_ringbuffer.tail, tail);
}

/*
====================================================================
Internal defines and constants
====================================================================
*/
#define		SOUND_FULLVOLUME	80
#define		MAX_SFX				(MAX_SOUNDS*2)
#define		SND_HASH_SIZE		32
#define		MAX_PLAYSOUNDS		128
#define		MAX_RAW_SAMPLES		8192
#define 	MAX_HEARING_DISTANCE 2048.0f

/*
====================================================================
Global variable definitions for the sound system
====================================================================
*/

channel_t   channels[MAX_CHANNELS];
qboolean    sound_started = false;
dma_t       dma;
int         paintedtime;
playsound_t s_pendingplays;
SDL_mutex  *s_sound_mutex = NULL;

cvar_t *s_volume, *s_show, *s_loadas8bit;
cvar_t *s_testsound, *s_khz, *s_mixahead, *s_primary;
cvar_t *s_swapstereo, *s_ambient, *s_oldresample, *s_quality;
cvar_t *s_sdl_resample, *s_force_mono, *s_sound_overlap;

//AI s_quality 0=Classic, 1=Attenuation, 2=Positional, 3=Filtering


//AI A pre-calculated lookup table for our enhanced attenuation model.
//AI Replaces a slow float division with a fast array lookup.
#define ATTN_TABLE_SIZE 1024
static float attn_table[ATTN_TABLE_SIZE];

static playsound_t s_freeplays;
static playsound_t s_playsounds[MAX_PLAYSOUNDS];
static int	s_registration_sequence;
static vec3_t listener_origin, listener_forward, listener_right, listener_up;
static qboolean	s_registering;
static sfx_t known_sfx[MAX_SFX];
static int num_sfx;
static sfx_t *sfx_hash[SND_HASH_SIZE];

// Forward declarations for local functions
static void S_Play_f(void);
static void S_SoundList_f(void);
static void S_SoundInfo_f(void);
static void S_Spatialize(channel_t *ch);
static void S_SpatializeOrigin (vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol);
static struct sfx_s *S_RegisterSexedSound(int entnum, const char *base);
static channel_t *S_PickChannel(int entnum, int entchannel);
static sfx_t *S_FindName (const char *name, qboolean create);
static sfx_t *S_AliasName (const char *aliasname, const char *truename);
static void S_ClearBuffer(void);
/*
====================
S_AddLoopSounds

Znajduje wszystkie byty (entities) w świecie, które powinny emitować
dźwięk pętli i upewnia się, że są one odtwarzane na kanale audio.
====================
*/

static void S_AddLoopSounds (void)
{
	int			i, j;
	channel_t	*ch;
	sfx_t		*sfx;
	sfxcache_t	*sc;
	centity_t	*cent;
	int			entnum;
	const char *sound_name; // Zmienna do przechowywania nazwy dźwięku

	for (i=0 ; i < cl.frame.num_entities ; i++)
	{
		entnum = cl_parse_entities[ (cl.frame.parse_entities + i) & PARSE_ENTITIES_MASK ].number;
		cent = &cl_entities[entnum];

		if (!cent->current.sound)
			continue;

		// --- KRYTYCZNA POPRAWKA BEZPIECZEŃSTWA ---
		// Krok 1: Pobierz nazwę dźwięku ze stringów konfiguracyjnych.
		sound_name = cl.configstrings[cent->current.sound];

		// Krok 2: Sprawdź, czy pobrana nazwa nie jest pusta lub nieważna.
		// To jest nasza siatka bezpieczeństwa, która łapie błąd, zanim on wystąpi.
		if (!sound_name || !sound_name[0])
			continue; // Jeśli nazwa jest pusta, zignoruj ten dźwięk w tej klatce.

		// Krok 3: Dopiero teraz, z pewnością, że nazwa jest poprawna, jej użyj.
		sfx = S_FindName(sound_name, false);

		if (!sfx)
			continue;

		sc = S_LoadSound(sfx);
		if (!sc)
			continue;

		for (j=0 ; j<MAX_CHANNELS ; j++)
		{
			if (channels[j].entnum == entnum && channels[j].sfx == sfx)
			{
				channels[j].autosound = true;
				goto next_entity;
			}
		}

		ch = S_PickChannel(entnum, 0);
		if (!ch)
			continue;

		ch->entnum = entnum;
		ch->sfx = sfx;
		ch->master_vol = 255;
		ch->dist_mult = 1.0f;
		ch->fixed_origin = false;
		ch->pos = 0;
		ch->end = paintedtime + sc->length;
		ch->autosound = true;

		S_Spatialize(ch);

next_entity:;
	}
}


//AI A helper function to build the table for enhanced attenuation model
static void S_BuildAttenuationTable(void)
{
    for (int i = 0; i < ATTN_TABLE_SIZE; i++)
    {
        // We map the range [0, ATTN_TABLE_SIZE] to a distance ratio.
        // Let's say the table covers up to 8x the SOUND_FULLVOLUME distance.
        float distance_ratio = (float)i / (ATTN_TABLE_SIZE / 8.0f);

        if (distance_ratio < 1.0f) {
            attn_table[i] = 1.0f;
        } else {
            // Inverse square model: 1 / distance^2
            // Add a small epsilon to avoid division by zero, though our logic prevents it.
            attn_table[i] = 1.0f / (distance_ratio * distance_ratio + 0.001f);
        }
    }
}

// ====================================================================
// NEW, SAFE DIAGNOSTIC FUNCTION
// ====================================================================
static void S_PrintDebugInfo(const char *moment)
{
    // Safety check: Do not run if the core sound system hasn't been initialized at least once.
    if (!sound_started) {
        Com_Printf("\n===== SOUND SYSTEM STATE [%s] (Not Initialized) =====\n", moment);
        return;
    }

    int free_count = 0;
    int pending_count = 0;
    int active_channels = 0;
    playsound_t *ps;

    // These checks prevent crashes if the list head is somehow corrupted.
    if (s_freeplays.next && s_freeplays.prev)
        for (ps = s_freeplays.next; ps != &s_freeplays; ps = ps->next)
            free_count++;

    if (s_pendingplays.next && s_pendingplays.prev)
        for (ps = s_pendingplays.next; ps != &s_pendingplays; ps = ps->next)
            pending_count++;

    for (int i=0; i < MAX_CHANNELS; i++)
        if (channels[i].sfx)
            active_channels++;

    Com_Printf("\n===== SOUND SYSTEM STATE [%s] =====\n", moment);
    Com_Printf("sound_started: %s\n", sound_started ? "true" : "false");
    Com_Printf("paintedtime: %d\n", paintedtime);
    Com_Printf("Free playsounds: %d / %d\n", free_count, MAX_PLAYSOUNDS);
    Com_Printf("Pending playsounds: %d\n", pending_count);
    Com_Printf("Active channels: %d / %d\n", active_channels, MAX_CHANNELS);
    Com_Printf("Known sfx count: %d\n", num_sfx);
    Com_Printf("=============================================\n\n");
}

static void OnChange_SoundRestart(cvar_t *self, const char *oldValue)
{
	// Ta funkcja jest wywoływana automatycznie, gdy cvar się zmieni.
	// Dodaje komendę 'snd_restart' do wykonania w następnej klatce.
	Cbuf_AddText("snd_restart\n");
}

/*
====================================================================
Initialization and Shutdown
====================================================================
*/

void S_Init(void)
{
	S_Audio_InitRingBuffer(); // ZMIANA: Inicjalizujemy naszą nową kolejkę
	S_PrintDebugInfo("Before S_Init"); // debug function
	Com_Printf("\n------- sound initialization -------\n");
	cvar_t *cv = Cvar_Get ("s_initsound", "1", CVAR_LATCHED);
	if (!cv->integer) { Com_Printf ("not initializing.\n------------------------------------\n"); return; }

	if (!s_sound_mutex) s_sound_mutex = SDL_CreateMutex();
	if (!s_sound_mutex) Com_Error(ERR_FATAL, "S_Init: Could not create sound mutex!");

	s_volume = Cvar_Get ("s_volume", "0.7", CVAR_ARCHIVE);
	s_loadas8bit = Cvar_Get ("s_loadas8bit", "0", CVAR_ARCHIVE|CVAR_LATCHED);
	s_khz = Cvar_Get ("s_khz", "22", CVAR_ARCHIVE|CVAR_LATCHED);
	s_mixahead = Cvar_Get ("s_mixahead", "0.2", CVAR_ARCHIVE);
	s_show = Cvar_Get ("s_show", "0", 0);
	s_testsound = Cvar_Get ("s_testsound", "0", 0);
	s_swapstereo = Cvar_Get( "s_swapstereo", "0", CVAR_ARCHIVE );
	s_ambient = Cvar_Get ("s_ambient", "1", 0);
	s_oldresample = Cvar_Get ("s_oldresample", "0", CVAR_LATCHED);
	s_quality = Cvar_Get ("s_quality", "0", CVAR_ARCHIVE);
	s_sdl_resample = Cvar_Get("s_sdl_resample", "1", CVAR_LATCHED);
	s_force_mono = Cvar_Get("s_force_mono", "1", CVAR_LATCHED);
	s_sound_overlap = Cvar_Get("s_sound_overlap", "0", CVAR_ARCHIVE);
	s_sdl_resample->OnChange = OnChange_SoundRestart;
	s_force_mono->OnChange = OnChange_SoundRestart;
	S_BuildAttenuationTable(); // Build the attenuation lookup table.

	if (SNDDMA_Init()) {
		S_InitScaletable ();
		sound_started = true;
		s_registration_sequence = 1;
		num_sfx = 0;
		paintedtime = 0;
		S_StopAllSounds();
		Cmd_AddCommand("play", S_Play_f);
		Cmd_AddCommand("stopsound", S_StopAllSounds);
		Cmd_AddCommand("soundlist", S_SoundList_f);
		Cmd_AddCommand("soundinfo", S_SoundInfo_f);
	}
	Com_Printf("------------------------------------\n");
	S_PrintDebugInfo("After S_Init"); // debug function
}

void S_Shutdown(void)
{
	S_PrintDebugInfo("Before S_Shutdown"); // debug function
	if (!sound_started) return;
	SNDDMA_Shutdown();
	if (s_sound_mutex) { SDL_DestroyMutex(s_sound_mutex); s_sound_mutex = NULL; }
	Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("stopsound");
	Cmd_RemoveCommand("soundlist");
	Cmd_RemoveCommand("soundinfo");
	for (int i=0; i < num_sfx; i++) {
		if (known_sfx[i].cache) Z_Free(known_sfx[i].cache);
		if (known_sfx[i].truename) Z_Free(known_sfx[i].truename);
	}
	memset(known_sfx, 0, sizeof(known_sfx));
	memset(sfx_hash, 0, sizeof(sfx_hash));
	num_sfx = 0;
	sound_started = false;
	S_PrintDebugInfo("After S_Shutdown"); // debug function
}

/*
====================================================================
Sound Start/Stop and Allocation
====================================================================
*/

static playsound_t *S_AllocPlaysound(void)
{
	playsound_t *ps = s_freeplays.next;
	if (ps == &s_freeplays) return NULL;
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;
	return ps;
}

void S_FreePlaysound(playsound_t *ps)
{
	ps->prev->next = ps->next;
	ps->next->prev = ps->prev;
	ps->next = s_freeplays.next;
	s_freeplays.next->prev = ps;
	ps->prev = &s_freeplays;
	s_freeplays.next = ps;
}

void S_StartSound(const vec3_t origin, int entnum, int entchannel, sfx_t *sfx, float fvol, float attenuation, float timeofs)
{
	if (!sound_started || !sfx) return;
	if (entchannel < 0) return;

	if (sfx->name[0] == '*') {
		if(entnum < 1 || entnum >= MAX_EDICTS ) {
			Com_Error( ERR_DROP, "S_StartSound: bad entnum: %d", entnum );
		}
		sfx = S_RegisterSexedSound(cl_entities[entnum].current.number, sfx->name);
		// CRITICAL FIX: Check if S_RegisterSexedSound failed
		if (!sfx) return;
	}

	sfxcache_t *sc = S_LoadSound(sfx);
	if (!sc || sc->length <= 0) return;

	SDL_LockMutex(s_sound_mutex);
	playsound_t *ps = S_AllocPlaysound();
	if (!ps) { SDL_UnlockMutex(s_sound_mutex); return; }

	if (origin) { VectorCopy(origin, ps->origin); ps->fixed_origin = true; }
	else { ps->fixed_origin = false; }
	ps->entnum = entnum;
	ps->entchannel = entchannel;
	ps->attenuation = attenuation;
	ps->sfx = sfx;
	ps->volume = fvol * 255.0f;
	ps->begin = paintedtime + (int)(timeofs * dma.speed);

	playsound_t *sort;
	for (sort = s_pendingplays.next; sort != &s_pendingplays && sort->begin < ps->begin; sort = sort->next);
	ps->next = sort;
	ps->prev = sort->prev;
	ps->next->prev = ps;
	ps->prev->next = ps;
	SDL_UnlockMutex(s_sound_mutex);
}

void S_StartLocalSound (const char *sound)
{
	if (!sound_started) return;
	sfx_t *sfx = S_RegisterSound(sound);
	if (!sfx) return;
	S_StartSound (NULL, cl.playernum+1, 0, sfx, 1, ATTN_NONE, 0);
}

void S_StopAllSounds(void)
{
	// NOTE: sound_started might not be true here, but we need to init the lists.

	// clear all active mixing channels
	memset(channels, 0, sizeof(channels));

	// Rebuild the free and pending playsound lists cleanly.
    // DO NOT use memset on s_playsounds, as it corrupts the list structure.
	s_freeplays.next = s_freeplays.prev = &s_freeplays;
	s_pendingplays.next = s_pendingplays.prev = &s_pendingplays;

    // This loop correctly rebuilds the free list from the existing array.
	for (int i=0; i<MAX_PLAYSOUNDS; i++)
	{
		s_playsounds[i].prev = &s_freeplays;
		s_playsounds[i].next = s_freeplays.next;
		s_playsounds[i].prev->next = &s_playsounds[i];
		s_playsounds[i].next->prev = &s_playsounds[i];
	}

	S_ClearBuffer();
}

static void S_ClearBuffer(void)
{
	if (!sound_started) return;
	int clear = (dma.samplebits == 8) ? 0x80 : 0;
	if (dma.buffer) Snd_Memset(dma.buffer, clear, dma.samples * dma.samplebits/8);
}

/*
====================================================================
Main Update Loop
====================================================================
*/
// ====================================================================
// NOWA, ROZBUDOWANA FUNKCJA DEBUGUJĄCA / debug function 02
// ====================================================================
static void S_Debug_ShowChannelInfo(void)
{
    // Nagłówek tabeli, wyświetlany tylko raz, jeśli są aktywne dźwięki
    qboolean header_printed = false;
    vec3_t   sound_origin;
    vec3_t   source_vec;
    float    dist;
    int      active_channels = 0;

    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        channel_t *ch = &channels[i];

        // Interesują nas tylko aktywne kanały, które mają przypisany dźwięk
        if (!ch->sfx || !ch->sfx->name[0])
            continue;

        // Jeśli to pierwszy aktywny dźwięk, drukuj nagłówek
        if (!header_printed)
        {
            Com_Printf("\n--- Sound Debug Info (s_show 1) ---\n");
            Com_Printf("CH | L/R Vol | Distance | Origin (X, Y, Z)      | Sound Name\n");
            Com_Printf("--------------------------------------------------------------------------\n");
            header_printed = true;
        }

        active_channels++;

        // Pobierz pozycję dźwięku (tak samo jak w S_Spatialize)
        if (ch->fixed_origin)
        {
            VectorCopy(ch->origin, sound_origin);
        }
        else
        {
            // Jeśli byt (entity) już nie istnieje, możemy dostać dziwne wartości, ale nie powinno to crashować
            CL_GetEntitySoundOrigin(ch->entnum, sound_origin);
        }

        // Oblicz dystans - to jest kluczowa informacja
        VectorSubtract(sound_origin, listener_origin, source_vec);
        dist = VectorLength(source_vec); // Używamy VectorLength, bo nie potrzebujemy normalizacji

        // Drukuj sformatowane informacje o kanale
        Com_Printf("%2i | %3i/%3i | %8.1f | %7.1f, %7.1f, %7.1f | %s\n",
                   i,
                   ch->leftvol,
                   ch->rightvol,
                   dist,
                   sound_origin[0],
                   sound_origin[1],
                   sound_origin[2],
                   ch->sfx->name);
    }

    if (header_printed)
    {
        Com_Printf("--------------------------------------------------------------------------\n");
        Com_Printf("Total active channels: %d\n\n", active_channels);
    }
}

/// main functions

void S_Update(const vec3_t origin, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up)
{
	// Ta funkcja musi być ZAWSZE wywoływana, aby pchać dane do kolejki
	// i zapobiegać trzaskom (buffer underrun).
	S_MixAudio();

	if (!sound_started) return;

	if (cls.disable_screen) { /*S_ClearBuffer();*/ return; } // S_ClearBuffer jest teraz niebezpieczne
	if (s_volume->modified) { S_InitScaletable(); s_volume->modified = false; }

	VectorCopy(origin, listener_origin);
	VectorCopy(v_forward, listener_forward);
	VectorCopy(v_right, listener_right);
	VectorCopy(v_up, listener_up);

	// Aktualizuj pozycję DYNAMICZNYCH dźwięków (wystrzały, kroki itp.)
	for (int i=0; i<MAX_CHANNELS; i++) {
		if (channels[i].autosound) memset(&channels[i], 0, sizeof(channels[i]));
		else if (channels[i].sfx) S_Spatialize(&channels[i]);
	}

	// ZMIANA: Dodajemy kluczowe sprawdzenie flagi `cl.sound_prepped`.
	// Uruchamiamy logikę dźwięków pętli (ambient) TYLKO wtedy, gdy
	// główny silnik klienta potwierdzi, że wszystko jest gotowe.
	if (s_ambient->integer && cl.sound_prepped)
		S_AddLoopSounds();

	// debug function 02
	if (s_show->integer)
	    {
	        S_Debug_ShowChannelInfo();
	    }

	// debug function 01
	/*
	if (s_show->integer) {
		int total = 0;
		for (int i=0; i<MAX_CHANNELS; i++) {
			if (channels[i].sfx && (channels[i].leftvol || channels[i].rightvol) ) {
				Com_Printf ("%3i %3i %s\n", channels[i].leftvol, channels[i].rightvol, channels[i].sfx->name);
				total++;
			}
		}
		Com_Printf ("----(%i)---- painted: %i\n", total, paintedtime);
	}
*/
}

/*
====================================================================
Sound Registration (Full, correct versions)
====================================================================
*/
void S_BeginRegistration(void) { s_registration_sequence++; s_registering = true; }

sfx_t *S_RegisterSound(const char *name) {
	if (!sound_started) return NULL;
	sfx_t *sfx = S_FindName(name, true);

	// KLUCZOWA ZMIANA:
	// Jeśli nie jesteśmy w trakcie rejestracji poziomu,
	// dźwięk jest "efektem specjalnym" i musi być załadowany natychmiast.
	// W przeciwnym razie, S_EndRegistration załaduje go hurtowo.
	if (!s_registering) {
		S_LoadSound(sfx);
	}
	return sfx;
}

// ====================================================================
// FINAL, ROBUST, AND CORRECT S_EndRegistration
// This version correctly and safely removes sfx_t entries from the hash list,
// preventing the infinite loop in S_FindName.
// ====================================================================
void S_EndRegistration(void)
{
	int		i;
	sfx_t	*sfx;

	// Unlink and free any sounds not used in the current registration sequence
	for (i=0; i < num_sfx; i++)
	{
		sfx = &known_sfx[i];

		if (!sfx->name[0])
			continue;

		if (sfx->registration_sequence == s_registration_sequence)
			continue; // This sound is still in use, skip it.

		// This sound is old and needs to be freed.
		// First, safely remove it from the hash table.
		unsigned int hash = Com_HashKey(sfx->name, SND_HASH_SIZE);
		sfx_t **prev = &sfx_hash[hash];
		sfx_t *current = *prev;

		while (current)
		{
			if (current == sfx)
			{
				*prev = current->hashNext; // Unlink it
				break;
			}
			prev = &current->hashNext;
			current = current->hashNext;
		}

		// Now, free its resources.
		if (sfx->cache)
			Z_Free(sfx->cache);
		if (sfx->truename)
			Z_Free(sfx->truename);

		// Finally, clear the sfx_t struct itself.
		memset(sfx, 0, sizeof(*sfx));
	}

	// Load all sounds that are part of the new registration sequence
	for (i=0; i < num_sfx; i++)
	{
		sfx = &known_sfx[i];
		if (sfx->name[0] && sfx->registration_sequence == s_registration_sequence)
		{
			// ZASTĘPUJEMY starą, wbudowaną logikę JEDNYM wywołaniem.
			// Funkcja S_LoadSound jest inteligentna i nie załaduje
			// dźwięku drugi raz, jeśli już jest w pamięci.
			S_LoadSound(sfx);
		}
	}

	s_registering = false;
}

static sfx_t *S_FindName (const char *name, qboolean create)
{
	if (!name || !name[0]) Com_Error(ERR_DROP, "S_FindName: empty name");
	unsigned int hash = Com_HashKey(name, SND_HASH_SIZE);
	for (sfx_t *sfx = sfx_hash[hash]; sfx; sfx = sfx->hashNext) {
		if (!strcmp(sfx->name, name)) {
			sfx->registration_sequence = s_registration_sequence;
			return sfx;
		}
	}
	if (!create) return NULL;
	int i;
	for (i = 0; i < num_sfx; i++)
		if (!known_sfx[i].name[0]) break;
	if (i == num_sfx) {
		if (num_sfx == MAX_SFX) Com_Error(ERR_FATAL, "S_FindName: out of sfx_t");
		num_sfx++;
	}
	sfx_t *sfx = &known_sfx[i];
	sfx->cache = NULL;
	sfx->truename = NULL;
	Q_strncpyz(sfx->name, name, sizeof(sfx->name));
	sfx->registration_sequence = s_registration_sequence;
	sfx->hashNext = sfx_hash[hash];
	sfx_hash[hash] = sfx;
	return sfx;
}

static sfx_t *S_AliasName (const char *aliasname, const char *truename)
{
	int i;
	for (i = 0; i < num_sfx; i++)
		if (!known_sfx[i].name[0]) break;
	if (i == num_sfx) {
		if (num_sfx == MAX_SFX) Com_Error (ERR_FATAL, "S_AliasName: out of sfx_t");
		num_sfx++;
	}
	sfx_t *sfx = &known_sfx[i];
	sfx->cache = NULL;
	strcpy(sfx->name, aliasname);
	sfx->registration_sequence = s_registration_sequence;
	sfx->truename = CopyString (truename, TAG_CL_SFX);
	unsigned int hash = Com_HashKey(aliasname, SND_HASH_SIZE);
	sfx->hashNext = sfx_hash[hash];
	sfx_hash[hash] = sfx;
	return sfx;
}

static struct sfx_s *S_RegisterSexedSound(int entnum, const char *base)
{
	char model[MAX_QPATH], sexedFilename[MAX_QPATH], maleFilename[MAX_QPATH];
	int n = CS_PLAYERSKINS + entnum - 1;
	model[0] = 0;
	if (cl.configstrings[n][0]) {
		char *p = strchr(cl.configstrings[n], '\\');
		if (p) {
			Q_strncpyz(model, p + 1, sizeof(model));
			p = strchr(model, '/');
			if (p) *p = 0;
		}
	}
	if (!model[0]) strcpy(model, "male");
	Com_sprintf(sexedFilename, sizeof(sexedFilename), "#players/%s/%s", model, base+1);
	sfx_t *sfx = S_FindName (sexedFilename, false);
	if (!sfx) {
		if (FS_LoadFile(sexedFilename + 1, NULL) > 0) {
			sfx = S_RegisterSound(sexedFilename);
		} else {
			Com_sprintf(maleFilename, sizeof(maleFilename), "player/%s/%s", "male", base+1);
			sfx = S_AliasName(sexedFilename, maleFilename);
		}
	}
	return sfx;
}

/*
====================================================================
Helper and Internal Functions (Full, correct versions)
====================================================================
*/


/*
====================================================================
Helper and Internal Functions (Full, correct versions)
====================================================================
*/

// Definicja pomocnicza do śledzenia, jak został zaalokowany bufor audio.
typedef enum {
	BUF_ORIGIN_SDL, // Zaalokowany przez SDL_LoadWAV
	BUF_ORIGIN_Z    // Zaalokowany przez Z_Malloc
} buffer_origin_t;

sfxcache_t *S_LoadSound(sfx_t *s)
{
	byte *raw_data;
	int raw_len;
	char name[MAX_QPATH];
	char path[MAX_QPATH];

	// --- NOWA, KLUCZOWA POPRAWKA ---
	// Dźwięki z '*' na początku to dynamiczne aliasy (np. dźwięki gracza),
	// które są rozwiązywane w czasie rzeczywistym. Nie są to pliki,
	// więc nie próbujemy ich tutaj ładować.
	if (s->name[0] == '*')
		return NULL;

	if (s->cache)
		return s->cache;

	if (s->truename)
		Q_strncpyz(name, s->truename, sizeof(name));
	else
		Q_strncpyz(name, s->name, sizeof(name));

	// --- KLUCZOWA POPRAWKA ---
	// Sprawdzamy, czy nazwa dźwięku zaczyna się od specjalnego znacznika '#'.
	if (name[0] == '#')
	{
		// Jeśli tak, ścieżka jest względna od katalogu gry, a my pomijamy '#'.
		Q_strncpyz(path, &name[1], sizeof(path));
	}
	else
	{
		// W przeciwnym razie jest to standardowy dźwięk z katalogu "sound/".
		Com_sprintf(path, sizeof(path), "sound/%s", name);
	}

	raw_len = FS_LoadFile(path, (void **)&raw_data);
	if (!raw_data) {
		Com_Printf("S_LoadSound: Couldn't load %s\n", path);
		return NULL;
	}

	SDL_RWops *rw = SDL_RWFromConstMem(raw_data, raw_len);
	if (!rw) {
		Com_Printf("S_LoadSound: SDL_RWFromConstMem failed for %s\n", path);
		FS_FreeFile(raw_data);
		return NULL;
	}

	SDL_AudioSpec spec;
	Uint8 *wav_buffer;
	Uint32 wav_length;

	if (SDL_LoadWAV_RW(rw, 1, &spec, &wav_buffer, &wav_length) == NULL) {
		Com_Printf("S_LoadSound: SDL_LoadWAV_RW failed for %s: %s\n", path, SDL_GetError());
		FS_FreeFile(raw_data);
		return NULL;
	}
	FS_FreeFile(raw_data);
	buffer_origin_t buffer_type = BUF_ORIGIN_SDL;

	if (spec.channels == 2 && s_force_mono->integer) {
		int samples = wav_length / 4; // 2 kanały * 16-bit
		Sint16 *src = (Sint16 *)wav_buffer;
		Sint16 *mono_buffer = Z_TagMalloc(samples * sizeof(Sint16), TAG_CL_SFX);

		for (int i = 0; i < samples; ++i)
			mono_buffer[i] = (Sint16)(((int)src[i * 2] + (int)src[i * 2 + 1]) / 2);

		if (buffer_type == BUF_ORIGIN_SDL) SDL_FreeWAV(wav_buffer); else Z_Free(wav_buffer);
		wav_buffer = (Uint8 *)mono_buffer;
		buffer_type = BUF_ORIGIN_Z;
		wav_length = samples * sizeof(Sint16);
		spec.channels = 1;
	}

	qboolean needs_conversion = (spec.format != AUDIO_S16LSB || spec.freq != dma.speed || spec.channels != dma.channels);
	if (needs_conversion && s_sdl_resample->integer) {
		SDL_AudioCVT cvt;
		SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq,
						  AUDIO_S16LSB, dma.channels, dma.speed);

		cvt.buf = Z_TagMalloc(wav_length * cvt.len_mult, TAG_CL_SFX);
		memcpy(cvt.buf, wav_buffer, wav_length);
		cvt.len = wav_length;
		SDL_ConvertAudio(&cvt);

		if (buffer_type == BUF_ORIGIN_SDL) SDL_FreeWAV(wav_buffer); else Z_Free(wav_buffer);
		wav_buffer = cvt.buf;
		buffer_type = BUF_ORIGIN_Z;
		wav_length = cvt.len_cvt;
		spec.freq = dma.speed;
		spec.format = AUDIO_S16LSB;
		spec.channels = dma.channels;
	}

	sfxcache_t *sc = Z_TagMalloc(sizeof(sfxcache_t) + wav_length, TAG_CL_SFX);
	sc->length = wav_length / (spec.channels * (SDL_AUDIO_BITSIZE(spec.format) / 8));
	sc->speed = spec.freq;
	sc->width = (SDL_AUDIO_BITSIZE(spec.format) / 8);
	sc->channels = spec.channels;
	sc->loopstart = -1;
	memcpy(sc->data, wav_buffer, wav_length);

	if (buffer_type == BUF_ORIGIN_SDL) SDL_FreeWAV(wav_buffer); else Z_Free(wav_buffer);

	s->cache = sc;

	return sc;
}

void S_IssuePlaysound(playsound_t *ps)
{
	channel_t *ch = S_PickChannel(ps->entnum, ps->entchannel);
	if (!ch) { S_FreePlaysound(ps); return; }

	sfxcache_t *sc = S_LoadSound(ps->sfx);
	if (!sc || sc->length <= 0) { S_FreePlaysound(ps); return; }

	ch->master_vol = ps->volume;
	ch->entnum = ps->entnum;
	ch->entchannel = ps->entchannel;
	ch->sfx = ps->sfx;
	VectorCopy(ps->origin, ch->origin);
	ch->fixed_origin = ps->fixed_origin;

	if (ps->attenuation == ATTN_STATIC) // Zakładając, że ATTN_STATIC to 3
	    ch->dist_mult = ps->attenuation * 0.001;
	else
	    ch->dist_mult = ps->attenuation * 0.0005;

	S_Spatialize(ch);
	ch->pos = 0;
    ch->end = paintedtime + sc->length;
	S_FreePlaysound(ps);
}

static channel_t *S_PickChannel(int entnum, int entchannel)
{
	int first_to_die = -1;
	int life_left = 0x7fffffff;
	for (int i=0; i < MAX_CHANNELS; i++) {
		// --- KLUCZOWA ZMIANA ---
		// Wykonaj tę logikę tylko jeśli s_sound_overlap jest wyłączony.
		if (!s_sound_overlap || !s_sound_overlap->integer)
		{
			if (entchannel != 0 && channels[i].entnum == entnum && channels[i].entchannel == entchannel) {
				first_to_die = i; break;
			}
		}

		if (channels[i].entnum == cl.playernum+1 && entnum != cl.playernum+1 && channels[i].sfx) continue;
		if (channels[i].end - paintedtime < life_left) {
			life_left = channels[i].end - paintedtime;
			first_to_die = i;
		}
	}
	if (first_to_die == -1) return NULL;
	memset(&channels[first_to_die], 0, sizeof(channel_t));
	return &channels[first_to_die];
}

// This is the correct version of S_Spatialize
static void S_Spatialize(channel_t *ch)
{
	vec3_t		origin;

	// anything coming from the view entity will always be full volume
	if (ch->entnum == cl.playernum+1)
	{
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
		return;
	}

	if (ch->fixed_origin)
	{
		VectorCopy (ch->origin, origin);
	}
	else
		CL_GetEntitySoundOrigin (ch->entnum, origin);

    // Wywołanie z poprawnymi argumentami dla klasycznej wersji
	S_SpatializeOrigin (origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol);
}


// ====================================================================
// S_SpatializeOrigin with a more gameplay-friendly attenuation model
// ====================================================================
/*
static void S_SpatializeOrigin (vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol)
{
    vec_t		dot;
    vec_t		dist;
    vec_t		lscale, rscale, scale;
    vec3_t		source_vec;

	// W nowej wersji jest `if (cls.state != ca_active)`, ale zostawmy to,
	// jest to poprawka bezpieczeństwa i nie wpływa na logikę.
	if (cls.state != ca_active)
	{
		*left_vol = *right_vol = 255;
		return;
	}

    // --- ORYGINALNA LOGIKA MATEMATYCZNA ---
	VectorSubtract(origin, listener_origin, source_vec);

	dist = VectorNormalize(source_vec);
	dist -= SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= dist_mult;		// different attenuation levels

	dot = DotProduct(listener_right, source_vec);

	if (dma.channels == 1 || !dist_mult)
	{ // no attenuation = no spatialization
		rscale = 1.0;
		lscale = 1.0;
	}
	else
	{
		rscale = 0.5 * (1.0 + dot);
		lscale = 0.5*(1.0 - dot);
	}

	// add in distance effect
	scale = (1.0 - dist) * rscale;
	*right_vol = (int) (master_vol * scale);
	if (*right_vol < 0)
		*right_vol = 0;

	scale = (1.0 - dist) * lscale;
	*left_vol = (int) (master_vol * scale);
	if (*left_vol < 0)
		*left_vol = 0;
}
*/

// ====================================================================
// KLASYCZNA IMPLEMENTACJA QUAKE II - zgodna z R1Q2
// ====================================================================
static void S_SpatializeOrigin (vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol)
{
    vec_t		dot;
    vec_t		dist;
    vec_t		lscale, rscale, scale;
    vec3_t		source_vec;

	if (cls.state != ca_active)
	{
		*left_vol = *right_vol = 255;
		return;
	}

// calculate stereo seperation and distance attenuation
	VectorSubtract(origin, listener_origin, source_vec);

	dist = VectorNormalize(source_vec);
	dist -= SOUND_FULLVOLUME;
	if (dist < 0)
		dist = 0;			// close enough to be at full volume
	dist *= dist_mult;		// different attenuation levels

	dot = DotProduct(listener_right, source_vec);

	if (dma.channels == 1 || !dist_mult)
	{ // no attenuation = no spatialization
		rscale = 1.0;
		lscale = 1.0;
	}
	else
	{
		rscale = 0.5 * (1.0 + dot);
		lscale = 0.5*(1.0 - dot);
	}

	// add in distance effect
	scale = (1.0 - dist) * rscale;
	*right_vol = (int) (master_vol * scale);
	if (*right_vol < 0)
		*right_vol = 0;

	scale = (1.0 - dist) * lscale;
	*left_vol = (int) (master_vol * scale);
	if (*left_vol < 0)
		*left_vol = 0;
}

void S_RawSamples(int samples, int rate, int width, int nchannels, byte *data) { /* Your full function body here */ }


/*
====================================================================
Console Commands
====================================================================
*/
static void S_Play_f (void)
{
	int 	i;
	char	name[MAX_QPATH];
	sfx_t	*sfx;

	for (i = 1; i < Cmd_Argc(); i++)
	{
		if (!strrchr(Cmd_Argv(i), '.')) {
			Q_strncpyz(name, Cmd_Argv(i), sizeof(name)-4);
			strcat(name, ".wav");
		}
		else
			Q_strncpyz (name, Cmd_Argv(i), sizeof(name));

		sfx = S_RegisterSound(name);
		if (sfx)
			S_StartSound(NULL, cl.playernum+1, 0, sfx, 1, 1, 0);
	}
}

static void S_SoundList_f (void)
{
	int		i, count = 0;
	sfx_t	*sfx;
	sfxcache_t	*sc;
	int		size, total = 0;

	for (sfx=known_sfx, i=0 ; i<num_sfx ; i++, sfx++)
	{
		if (!sfx->registration_sequence)
			continue;
		sc = sfx->cache;
		if (sc)
		{
			size = sc->length * sc->width * sc->channels;
			total += size;
			if (sc->loopstart >= 0)
				Com_Printf ("L");
			else
				Com_Printf (" ");
			Com_Printf("(%2db) %6i : %s\n", sc->width*8,  size, sfx->name);
		}
		else
		{
			if (sfx->name[0] == '*')
				Com_Printf("  placeholder : %s\n", sfx->name);
			else
				Com_Printf("  not loaded  : %s\n", sfx->name);
		}
		count++;
	}
	Com_Printf ("%i sounds\n", count);
	Com_Printf ("Total resident: %i\n", total);
}
static void S_SoundInfo_f(void)
{
	if (!sound_started) {
		Com_Printf ("S_SoundInfo_f: sound system not started\n");
		return;
	}
}
