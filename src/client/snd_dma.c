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
// Gemini 3 Flash [X-JC-STRICTOR-V2] 2026-02-04
#include "client.h"
#include "../qcommon/qcommon.h"
#include "snd_loc.h"
#include "SDL2/SDL.h"

/// RING BUFFER IMPLEMENTATION

ring_buffer_t s_ringbuffer;

static void S_Audio_InitRingBuffer(void)
{
	atomic_init(&s_ringbuffer.head, 0);
	atomic_init(&s_ringbuffer.tail, 0);
	memset(s_ringbuffer.buffer, 0, AUDIO_RING_BUFFER_SIZE);
}

int S_Audio_AvailableToRead(void)
{
	int head = atomic_load(&s_ringbuffer.head);
	int tail = atomic_load(&s_ringbuffer.tail);
	return (tail - head) & (AUDIO_RING_BUFFER_SIZE - 1);
}

int S_Audio_AvailableToWrite(void)
{
	int head = atomic_load(&s_ringbuffer.head);
	int tail = atomic_load(&s_ringbuffer.tail);
	return (head - tail - 1) & (AUDIO_RING_BUFFER_SIZE - 1);
}

void S_Audio_Read(void* dest, int count)
{
	int head = atomic_load(&s_ringbuffer.head);
	byte* p_dest = (byte*)dest;
	
	int first_chunk = AUDIO_RING_BUFFER_SIZE - head;
	if (first_chunk > count) first_chunk = count;

	memcpy(p_dest, &s_ringbuffer.buffer[head], first_chunk);
	
	if (count > first_chunk)
	{
		memcpy(p_dest + first_chunk, &s_ringbuffer.buffer[0], count - first_chunk);
	}

	atomic_store(&s_ringbuffer.head, (head + count) & (AUDIO_RING_BUFFER_SIZE - 1));
}

void S_Audio_Write(const void* src, int count)
{
	int tail = atomic_load(&s_ringbuffer.tail);
	const byte* p_src = (const byte*)src;

	int first_chunk = AUDIO_RING_BUFFER_SIZE - tail;
	if (first_chunk > count) first_chunk = count;

	memcpy(&s_ringbuffer.buffer[tail], p_src, first_chunk);

	if (count > first_chunk)
	{
		memcpy(&s_ringbuffer.buffer[0], p_src + first_chunk, count - first_chunk);
	}

	atomic_store(&s_ringbuffer.tail, (tail + count) & (AUDIO_RING_BUFFER_SIZE - 1));
}

/// CONSTANTS & GLOBALS

#define		SOUND_FULLVOLUME	80
#define		MAX_SFX				(MAX_SOUNDS*2)
#define		SND_HASH_SIZE		32
#define		MAX_PLAYSOUNDS		128

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

static playsound_t s_freeplays;
static playsound_t s_playsounds[MAX_PLAYSOUNDS];
static int	s_registration_sequence;
vec3_t listener_origin, listener_forward, listener_right, listener_up;
static qboolean	s_registering;
static sfx_t known_sfx[MAX_SFX];
static int num_sfx;
static sfx_t *sfx_hash[SND_HASH_SIZE];

// Forward declarations
static void S_Play_f(void);
static void S_SoundList_f(void);
static void S_SoundInfo_f(void);
void S_Spatialize(channel_t *ch);
static void S_SpatializeOrigin (vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol);
static struct sfx_s *S_RegisterSexedSound(int entnum, const char *base);
static channel_t *S_PickChannel(int entnum, int entchannel);
static sfx_t *S_FindName (const char *name, qboolean create);
static sfx_t *S_AliasName (const char *aliasname, const char *truename);

// HRTF
sound_export_t *snd_backend = &snd_soft_export;
cvar_t *s_hrtf;


/// SYSTEM FUNCTIONS

static void OnChange_SoundRestart(cvar_t *self, const char *oldValue)
{
	(void)self; (void)oldValue;
	Cbuf_AddText("snd_restart\n");
}

void S_Init(void)
{
	S_Audio_InitRingBuffer();
	Com_Printf("\n------- sound initialization -------\n");

	cvar_t *cv = Cvar_Get ("s_initsound", "1", CVAR_LATCHED);
	s_hrtf = Cvar_Get("s_hrtf", "0", CVAR_ARCHIVE | CVAR_LATCHED);
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

	// Wybór backendu przed uruchomieniem DMA
    if (s_hrtf->integer) {
        snd_backend = &snd_hrtf_export;
    } else {
        snd_backend = &snd_soft_export;
    }

	if (SNDDMA_Init()) {
		sound_started = true;
	 // Inicjalizacja wybranego backendu (np. Steam Audio Context)
        if (snd_backend->Init) {
            snd_backend->Init();
        }

		s_registration_sequence = 1;
		num_sfx = 0;
		paintedtime = 0;
		
		S_StopAllSounds();

		Cmd_AddCommand("play", S_Play_f);
		Cmd_AddCommand("stopsound", S_StopAllSounds);
		Cmd_AddCommand("soundlist", S_SoundList_f);
		Cmd_AddCommand("soundinfo", S_SoundInfo_f);
	}

	
	// HRTF
//snd_backend = &snd_soft_export; // unfinished bussiness :-)
	/*
	    s_hrtf = Cvar_Get("s_hrtf", "0", CVAR_LATCHED);
    
    	if (s_hrtf->integer == 1) {
    	snd_backend = &snd_hrtf_export;
		} else {
    	snd_backend = &snd_soft_export;
		}
*/
    Com_Printf("Sound backend: %s\n", snd_backend->name);
}

void S_Activate(qboolean active)
{
    (void)active;
}

void S_FreeAllSounds(void)
{
	for (int i=0; i < num_sfx; i++) {
		if (known_sfx[i].cache) Z_Free(known_sfx[i].cache);
		if (known_sfx[i].truename) Z_Free(known_sfx[i].truename);
	}
	memset(known_sfx, 0, sizeof(known_sfx));
	memset(sfx_hash, 0, sizeof(sfx_hash));
	num_sfx = 0;
}


void S_Shutdown(void)
{
	if (!sound_started) return;

    if (snd_backend->Shutdown) {
        snd_backend->Shutdown();
    }

    SNDDMA_Shutdown();
	
	if (s_sound_mutex) { 
		SDL_DestroyMutex(s_sound_mutex); 
		s_sound_mutex = NULL; 
	}

	Cmd_RemoveCommand("play");
	Cmd_RemoveCommand("stopsound");
	Cmd_RemoveCommand("soundlist");
	Cmd_RemoveCommand("soundinfo");

	S_FreeAllSounds();
	
	sound_started = false;
}

/// SOUND LOADING

sfxcache_t *S_LoadSound(sfx_t *s)
{
	byte *raw_data;
	int raw_len;
	char path[MAX_QPATH];
	SDL_AudioSpec spec;
	Uint8 *wav_buffer;
	Uint32 wav_length;

	if (s->name[0] == '*') return NULL;
	if (s->cache) return s->cache;

	// Ścieżka pliku
	if (s->truename) Com_sprintf(path, sizeof(path), "sound/%s", s->truename);
	else if (s->name[0] == '#') Q_strncpyz(path, &s->name[1], sizeof(path));
	else Com_sprintf(path, sizeof(path), "sound/%s", s->name);

	raw_len = FS_LoadFile(path, (void **)&raw_data);
	if (!raw_data) return NULL;

	SDL_RWops *rw = SDL_RWFromConstMem(raw_data, raw_len);
	if (!SDL_LoadWAV_RW(rw, 1, &spec, &wav_buffer, &wav_length)) {
		Com_Printf("S_LoadSound: SDL_LoadWAV failed %s\n", path);
		FS_FreeFile(raw_data);
		return NULL;
	}
	FS_FreeFile(raw_data);
	
	SDL_AudioCVT cvt;
	SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, 
	                        AUDIO_S16LSB, dma.channels, dma.speed);

	// Alokujemy jeden bufor roboczy dla CVT
	cvt.len = wav_length;
	cvt.buf = Z_TagMalloc(cvt.len * cvt.len_mult, TAG_CL_SFX);
	memcpy(cvt.buf, wav_buffer, wav_length);
	SDL_FreeWAV(wav_buffer);

	SDL_ConvertAudio(&cvt);

	// Finalna paczka dźwiękowa
	int final_len = cvt.len_cvt;
	sfxcache_t *sc = Z_TagMalloc(sizeof(sfxcache_t) + final_len, TAG_CL_SFX);
	sc->length = final_len / (dma.channels * 2);
	sc->speed = dma.speed;
	sc->width = 2;
	sc->channels = dma.channels;
	sc->loopstart = -1;
	memcpy(sc->data, cvt.buf, final_len);

	Z_Free(cvt.buf);
	s->cache = sc;

	if (s_show->integer) Com_Printf("Loaded: %s\n", path);
	return sc;
}

/// PLAYBACK CONTROL

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

void S_StopAllSounds(void)
{
	SDL_LockMutex(s_sound_mutex);

	memset(channels, 0, sizeof(channels));
	s_freeplays.next = s_freeplays.prev = &s_freeplays;
	s_pendingplays.next = s_pendingplays.prev = &s_pendingplays;

	for (int i=0; i<MAX_PLAYSOUNDS; i++)
	{
		s_playsounds[i].prev = &s_freeplays;
		s_playsounds[i].next = s_freeplays.next;
		s_playsounds[i].prev->next = &s_playsounds[i];
		s_playsounds[i].next->prev = &s_playsounds[i];
	}

	int current_tail = atomic_load(&s_ringbuffer.tail);
	atomic_store(&s_ringbuffer.head, current_tail);	
	
	memset(s_ringbuffer.buffer, 0, AUDIO_RING_BUFFER_SIZE);

	SDL_UnlockMutex(s_sound_mutex);
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
	// Debug
    if (s_show->integer)
        Com_Printf("S_StartSound: %s (vol: %.2f)\n", sfx->name, fvol);
}

void S_StartLocalSound (const char *sound)
{
	if (!sound_started) return;
	sfx_t *sfx = S_RegisterSound(sound);
	if (!sfx) return;
	S_StartSound (NULL, cl.playernum+1, 0, sfx, 1, ATTN_NONE, 0);
}

/// UPDATES & SPATIALIZATION

void S_AddLoopSounds (void)
{
    for (int i=0 ; i < cl.frame.num_entities ; i++)
    {
        int entnum = cl_parse_entities[ (cl.frame.parse_entities + i) & PARSE_ENTITIES_MASK ].number;
        centity_t *cent = &cl_entities[entnum];

        if (!cent->current.sound) continue;

        const char *sound_name = cl.configstrings[CS_SOUNDS + cent->current.sound];
        if (!sound_name || !sound_name[0]) continue;

        sfx_t *sfx = S_FindName(sound_name, false);
        if (!sfx) continue;

        sfxcache_t *sc = S_LoadSound(sfx);
        if (!sc) continue;

        // Szukaj istniejącego kanału dla tej encji i tego dźwięku
        channel_t *ch = NULL;
        for (int j=0 ; j<MAX_CHANNELS ; j++) {
            if (channels[j].entnum == entnum && channels[j].sfx == sfx) {
                ch = &channels[j];
                break;
            }
        }

        if (!ch) {
            ch = S_PickChannel(entnum, 0);
            if (!ch) continue;
            ch->entnum = entnum;
            ch->sfx = sfx;
            ch->pos = 0;
            ch->end = paintedtime + sc->length;
        }

        ch->autosound = true;
        ch->master_vol = 127;

        // FIX DLA RAKIET: 
        // 1. fixed_origin = false zmusza S_Spatialize do wołania CL_GetEntitySoundOrigin
        ch->fixed_origin = false; 
        
        // 2. Rakiety używają zazwyczaj ATTN_NORM (1.0) -> 0.0005f
        ch->dist_mult = 1.0f * 0.0005f; 

        S_Spatialize(ch);
    }
}

/// HRTF
void S_Update(const vec3_t origin, const vec3_t v_forward, const vec3_t v_right, const vec3_t v_up) {
    if (!sound_started) return;
   
    snd_backend->Update(origin, v_forward, v_right, v_up);
}

static channel_t *S_PickChannel(int entnum, int entchannel)
{
	int first_to_die = -1;
	int life_left = 0x7fffffff;

	for (int i=0; i < MAX_CHANNELS; i++) {
		// ZMIANA: Wymuszamy nadpisywanie dla kanału broni (1),
		// aby przerwać dźwięk ładowania/odbezpieczania (Granat, BFG)
		// niezależnie od ustawienia overlap.
		if ((!s_sound_overlap || !s_sound_overlap->integer) || entchannel == 1)
		{
			if (entchannel != 0 && channels[i].entnum == entnum && channels[i].entchannel == entchannel) {
				first_to_die = i;
				break;
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

void S_Spatialize(channel_t *ch)
{
	vec3_t origin;
	if (ch->entnum == cl.playernum+1) {
		ch->leftvol = ch->master_vol;
		ch->rightvol = ch->master_vol;
		return;
	}
	if (ch->fixed_origin) VectorCopy (ch->origin, origin);
	else CL_GetEntitySoundOrigin (ch->entnum, origin);
	S_SpatializeOrigin (origin, ch->master_vol, ch->dist_mult, &ch->leftvol, &ch->rightvol);
}

static void S_SpatializeOrigin (vec3_t origin, float master_vol, float dist_mult, int *left_vol, int *right_vol)
{
    vec_t dot, dist, lscale, rscale, scale;
    vec3_t source_vec;

    if (cls.state != ca_active) {
        *left_vol = *right_vol = (int)master_vol;
        return;
    }

    VectorSubtract(origin, listener_origin, source_vec);
    dist = VectorNormalize(source_vec);

    // 1. Strefa pełnej głośności (80 jednostek)
    dist -= SOUND_FULLVOLUME;
    if (dist < 0) dist = 0;

    // 2. Właściwa atenuacja
    dist *= dist_mult;
    scale = 1.0f - dist;

    if (scale <= 0) {
        *left_vol = *right_vol = 0;
        return;
    }

    if (dma.channels == 1 || !dist_mult) {
        lscale = rscale = 1.0f;
    } else {
        dot = DotProduct(listener_right, source_vec);
        rscale = 0.5f * (1.0f + dot);
        lscale = 0.5f * (1.0f - dot);
    }

    *right_vol = (int)(master_vol * scale * rscale);
    *left_vol  = (int)(master_vol * scale * lscale);
}

/// REGISTRATION

void S_BeginRegistration(void) { s_registration_sequence++; s_registering = true; }

sfx_t *S_RegisterSound(const char *name) {
	if (!sound_started) return NULL;
	sfx_t *sfx = S_FindName(name, true);
	if (!s_registering) S_LoadSound(sfx);
	return sfx;
}

void S_EndRegistration(void)
{
	int i;
	sfx_t *sfx;

	for (i=0; i < num_sfx; i++) {
		sfx = &known_sfx[i];
		if (!sfx->name[0]) continue;
		if (sfx->registration_sequence == s_registration_sequence) continue;

		// Unlink from hash
		unsigned int hash = Com_HashKey(sfx->name, SND_HASH_SIZE);
		sfx_t **prev = &sfx_hash[hash];
		sfx_t *current = *prev;
		while (current) {
			if (current == sfx) {
				*prev = current->hashNext;
				break;
			}
			prev = &current->hashNext;
			current = current->hashNext;
		}

		if (sfx->cache) Z_Free(sfx->cache);
		if (sfx->truename) Z_Free(sfx->truename);
		memset(sfx, 0, sizeof(*sfx));
	}

	for (i=0; i < num_sfx; i++) {
		sfx = &known_sfx[i];
		if (sfx->name[0] && sfx->registration_sequence == s_registration_sequence) {
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
	sfx_t *sfx = S_FindName(aliasname, true);
	if (sfx->truename) Z_Free(sfx->truename);
	sfx->truename = CopyString (truename, TAG_CL_SFX);
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

/// CONSOLE COMMANDS

static void S_Play_f (void)
{
	int i;
	char name[MAX_QPATH];
	sfx_t *sfx;
	for (i = 1; i < Cmd_Argc(); i++) {
		if (!strrchr(Cmd_Argv(i), '.')) {
			Q_strncpyz(name, Cmd_Argv(i), sizeof(name)-4);
			strcat(name, ".wav");
		} else Q_strncpyz (name, Cmd_Argv(i), sizeof(name));
		sfx = S_RegisterSound(name);
		if (sfx) S_StartSound(NULL, cl.playernum+1, 0, sfx, 1, 1, 0);
	}
}

static void S_SoundList_f (void)
{
	int i, count = 0, size, total = 0;
	sfx_t *sfx;
	sfxcache_t *sc;
	for (sfx=known_sfx, i=0 ; i<num_sfx ; i++, sfx++) {
		if (!sfx->registration_sequence) continue;
		sc = sfx->cache;
		if (sc) {
			size = sc->length * sc->width * sc->channels;
			total += size;
			Com_Printf("%s(%2db) %6i : %s\n", (sc->loopstart>=0)?"L":" ", sc->width*8, size, sfx->name);
		} else Com_Printf("  %s : %s\n", (sfx->name[0]=='*')?"placeholder":"not loaded", sfx->name);
		count++;
	}
	Com_Printf ("%i sounds, Total resident: %i\n", count, total);
}

static void S_SoundInfo_f(void)
{
	if (!sound_started) { Com_Printf ("S_SoundInfo_f: sound system not started\n"); return; }
	Com_Printf("Ring Buffer Read: %d\n", S_Audio_AvailableToRead());
	Com_Printf("Ring Buffer Write: %d\n", S_Audio_AvailableToWrite());
}

void S_IssuePlaysound(playsound_t *ps) {
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
    
    // MATEMATYKA Q2PRO/SOFTWARE:
    if (ps->attenuation <= 0)
        ch->dist_mult = 0;
    else if (ps->attenuation >= 3.0f) // ATTN_STATIC i wyższe
        ch->dist_mult = ps->attenuation * 0.001f;
    else // ATTN_NORM (1.0) i inne
        ch->dist_mult = ps->attenuation * 0.0005f;
    
    S_Spatialize(ch);
    ch->pos = 0;
    ch->end = paintedtime + sc->length;
    S_FreePlaysound(ps);
}


// S_RawSamples
// Wymagana przez cl_cin.c do odtwarzania dźwięku z plików wideo (.cin).
// Sygnatura musi pasować do tej z client/sound.h (bez const).

void S_RawSamples(int samples, int rate, int width, int nchannels, byte *data)
{
	if (!sound_started) return;

	int len = samples * width * nchannels;	

	int avail = S_Audio_AvailableToWrite();
	if (len > avail)
		len = avail;

	if (len > 0)
	{		
		S_Audio_Write(data, len);
	}
}
