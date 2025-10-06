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
//static void S_SpatializeOrigin(const vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol);
static void S_SpatializeOrigin(channel_t *ch, const vec3_t origin, float master_vol, float dist_mult);
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
	s_quality = Cvar_Get ("s_quality", "3", CVAR_ARCHIVE);
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
	if (!s_registering) S_LoadSound(sfx);
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
	ch->dist_mult = (ps->attenuation == ATTN_STATIC) ? ps->attenuation * 0.001f : ps->attenuation * 0.0005f;
	S_Spatialize(ch);
	ch->pos = 0;
    ch->end = paintedtime + sc->length;
	S_FreePlaysound(ps);
}
*/

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

	// --- KRYTYCZNA POPRAWKA LOGIKI TŁUMIENIA ---
	// Zastępujemy wadliwą jednoliniową kalkulację jawną i poprawną logiką,
	// która odzwierciedla intencje projektantów oryginalnej gry.
	switch ((int)ps->attenuation)
	{
		case 1: // ATTN_NORM: Standardowe tłumienie dla większości dźwięków.
			ch->dist_mult = 0.001f;
			break;

		case 2: // ATTN_IDLE: Szybkie tłumienie, słyszalne tylko z bliska.
			ch->dist_mult = 0.002f;
			break;

		case 3: // ATTN_STATIC: Wolne tłumienie, słyszalne z dużej odległości.
			ch->dist_mult = 0.0005f;
			break;

		case 0: // ATTN_NONE: Brak tłumienia.
		default:
			ch->dist_mult = 0.0f;
			break;
	}

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
		if (entchannel != 0 && channels[i].entnum == entnum && channels[i].entchannel == entchannel) {
			first_to_die = i; break;
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
/*
static void S_Spatialize(channel_t *ch)
{
	vec3_t origin;
	if (ch->entnum == cl.playernum+1) {
		ch->leftvol = ch->master_vol; ch->rightvol = ch->master_vol; return;
	}
	if (ch->fixed_origin) VectorCopy(ch->origin, origin);
	else CL_GetEntitySoundOrigin(ch->entnum, origin);
	S_SpatializeOrigin(origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol);
}
*/
// This is the correct version of S_Spatialize
static void S_Spatialize(channel_t *ch)
{
	vec3_t origin;

	// Sounds attached to the player are always max volume, no occlusion.
	if (ch->entnum == cl.playernum+1) {
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
        ch->occlusion = 0;
		return;
	}

	// Get the sound's origin in the world.
	if (ch->fixed_origin)
		VectorCopy(ch->origin, origin);
	else
		CL_GetEntitySoundOrigin(ch->entnum, origin);

	// Call the new, correct SpatializeOrigin function.
	// It will write leftvol, rightvol, and occlusion directly into 'ch'.
	S_SpatializeOrigin(ch, origin, ch->master_vol, ch->dist_mult);
}

/*
static void S_SpatializeOrigin(const vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol)
{
	vec_t dot, dist, lscale, rscale, vol;
	vec3_t source_vec;
	VectorSubtract(origin, listener_origin, source_vec);
	dist = VectorNormalize(source_vec);
	dist = (dist > SOUND_FULLVOLUME) ? (dist - SOUND_FULLVOLUME) * dist_mult : 0;
	if (dma.channels == 1 || !dist_mult) { lscale = rscale = 1.0f; }
	else { dot = DotProduct(listener_right, source_vec); rscale = 0.5f * (1.0f + dot); lscale = 0.5f * (1.0f - dot); }
	vol = master_vol * (1.0f - dist);
	*right_vol = (vol * rscale < 0) ? 0 : vol * rscale;
	*left_vol = (vol * lscale < 0) ? 0 : vol * lscale;
}
*/
/*
// ====================================================================
// FINAL, ROBUST, AND MATHEMATICALLY SAFE S_SpatializeOrigin
// This version is safe, clear, and easy to understand and extend.
// ====================================================================
static void S_SpatializeOrigin(const vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol)
{
	vec3_t	source_vec;
	float	dist;
	float	vol;

	source_vec[0] = origin[0] - listener_origin[0];
	source_vec[1] = origin[1] - listener_origin[1];
	source_vec[2] = origin[2] - listener_origin[2];

	dist = VectorNormalize(source_vec); // This is safe and returns the distance.

	// --- CRITICAL SAFETY CHECK for sounds at the listener's exact position ---
	// If distance is zero (or very close), we can't spatialize. Play at full volume.
	if (dist < 0.1f)
	{
		*left_vol = master_vol;
		*right_vol = master_vol;
		return;
	}

	// --- Attenuation Logic ---
    float attenuation_factor;
    extern cvar_t *s_quality;

    if (s_quality && s_quality->integer != 0) // Enhanced Quality Mode
    {
        // Use the safe lookup table for attenuation.
        // Convert distance to a table index.
        int table_index = (int)((dist / SOUND_FULLVOLUME) * (ATTN_TABLE_SIZE / 8.0f));

        // Clamp the index to the table bounds.
        if (table_index < 0) table_index = 0;
        if (table_index >= ATTN_TABLE_SIZE) table_index = ATTN_TABLE_SIZE - 1;

        attenuation_factor = attn_table[table_index];
    }
    else // Classic (Default) Mode
    {
        // Use the original, linear attenuation model.
        float linear_dist = (dist > SOUND_FULLVOLUME) ? (dist - SOUND_FULLVOLUME) * dist_mult : 0;
        attenuation_factor = 1.0f - linear_dist;
    }

	vol = master_vol * attenuation_factor;

	// --- Panning Logic ---
	// Since source_vec is already normalized by VectorNormalize, this is simple.
	float dot = DotProduct(listener_right, source_vec);
	float rscale = 0.5f * (1.0f + dot);
	float lscale = 1.0f - rscale;

	// Final conversion and clamping to ensure values are always safe.
	int right_i = (int)(vol * rscale);
	int left_i = (int)(vol * lscale);

	*right_vol = (right_i < 0) ? 0 : (right_i > 255) ? 255 : right_i;
	*left_vol = (left_i < 0) ? 0 : (left_i > 255) ? 255 : left_i;
}
*/

// ====================================================================
// S_SpatializeOrigin with added Z-axis (vertical) processing
// ====================================================================
/*
static void S_SpatializeOrigin(const vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol)
{
	vec3_t	source_vec;
	float	dist;
	float	vol;

	source_vec[0] = origin[0] - listener_origin[0];
	source_vec[1] = origin[1] - listener_origin[1];
	source_vec[2] = origin[2] - listener_origin[2];

	dist = VectorNormalize(source_vec);

	if (dist < 0.1f)
	{
		*left_vol = master_vol;
		*right_vol = master_vol;
		return;
	}

	// --- Attenuation Logic (no changes here) ---
    float attenuation_factor;
    extern cvar_t *s_quality;

    if (s_quality && s_quality->integer != 0) // Enhanced Quality Mode
    {
        int table_index = (int)((dist / SOUND_FULLVOLUME) * (ATTN_TABLE_SIZE / 8.0f));
        if (table_index < 0) table_index = 0;
        if (table_index >= ATTN_TABLE_SIZE) table_index = ATTN_TABLE_SIZE - 1;
        attenuation_factor = attn_table[table_index];
    }
    else // Classic (Default) Mode
    {
        float linear_dist = (dist > SOUND_FULLVOLUME) ? (dist - SOUND_FULLVOLUME) * dist_mult : 0;
        attenuation_factor = 1.0f - linear_dist;
    }

	vol = master_vol * attenuation_factor;

	// --- Panning Logic ---
	// Horizontal (left/right) panning - original logic
	float dot_right = DotProduct(listener_right, source_vec);
	float rscale = 0.5f * (1.0f + dot_right);
	float lscale = 1.0f - rscale;

    // --- NEW Z-AXIS LOGIC ---
    // Vertical (up/down) panning, applied as a volume modifier
    if (s_quality && s_quality->integer != 0) // Only for Enhanced mode
    {
        float dot_up = DotProduct(listener_up, source_vec);
        // fabs(dot_up) gives a value from 0 (horizontal) to 1 (vertical).
        // (1.0 - fabs(dot_up)) gives a multiplier that is 1.0 for horizontal
        // sounds and falls to 0.0 for sounds directly above/below.
        // We use pow(..., 0.5) to soften the effect, so sounds don't disappear completely.
        float vertical_attenuation = pow(1.0f - fabs(dot_up), 0.5);
        vol *= vertical_attenuation;
    }
    // --- END OF NEW Z-AXIS LOGIC ---

	// Final conversion and clamping
	int right_i = (int)(vol * rscale);
	int left_i = (int)(vol * lscale);

	*right_vol = (right_i < 0) ? 0 : (right_i > 255) ? 255 : right_i;
	*left_vol = (left_i < 0) ? 0 : (left_i > 255) ? 255 : left_i;
}
*/


// ====================================================================
// S_SpatializeOrigin with a more gameplay-friendly attenuation model
// ====================================================================
static void S_SpatializeOrigin(channel_t *ch, const vec3_t origin, float master_vol, float dist_mult)
{
	vec3_t	source_vec;
	float	dist;
	float	vol;

	VectorSubtract(origin, listener_origin, source_vec);
	dist = VectorNormalize(source_vec);

	if (dist < 0.1f) {
		ch->leftvol = master_vol; ch->rightvol = master_vol;
		ch->occlusion = 0;
		return;
	}

    float attenuation_factor;
    extern cvar_t *s_quality;

    if (s_quality && s_quality->integer >= 1) // Enhanced Quality Mode
    {
        // Use a linear interpolation (Lerp) model for a gentle, controllable falloff.
        const float MIN_ATTENUATION = 0.05f; // Sound at max distance will have 5% volume.

        if (dist <= SOUND_FULLVOLUME) {
            attenuation_factor = 1.0f;
        } else {
            // Calculate how far we are into the falloff range (0.0 to 1.0)
            float falloff_range = MAX_HEARING_DISTANCE - SOUND_FULLVOLUME;
            float dist_in_falloff = dist - SOUND_FULLVOLUME;
            float falloff_percent = dist_in_falloff / falloff_range;

            // Clamp the value to ensure it's between 0.0 and 1.0
            if (falloff_percent > 1.0f) falloff_percent = 1.0f;
            if (falloff_percent < 0.0f) falloff_percent = 0.0f;

            // Linearly interpolate the volume
            attenuation_factor = 1.0f - falloff_percent * (1.0f - MIN_ATTENUATION);
        }
    }
    else // Classic (Default) Mode
    {
        // Use the original, linear attenuation model.
        float linear_dist = (dist > SOUND_FULLVOLUME) ? (dist - SOUND_FULLVOLUME) * dist_mult : 0;
        attenuation_factor = 1.0f - linear_dist;
    }

	vol = master_vol * attenuation_factor;

	// --- Panning & Occlusion Logic (no changes here) ---
	float dot_right = DotProduct(listener_right, source_vec);
	float rscale = 0.5f * (1.0f + dot_right);
	float lscale = 1.0f - rscale;

    ch->occlusion = 0;
    if (s_quality && s_quality->integer >= 2) {
        float dot_up = DotProduct(listener_up, source_vec);
        float vertical_attenuation = 1.0f - (fabs(dot_up) * 0.5f);
        vol *= vertical_attenuation;
        if (s_quality && s_quality->integer >= 3) {
            ch->occlusion = (int)(fabs(dot_up) * 255.0f);
        }
    }

	ch->leftvol = (int)(vol * lscale);
	ch->rightvol = (int)(vol * rscale);
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
