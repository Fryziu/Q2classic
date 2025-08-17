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
// snd_loc.h -- private sound functions

#ifndef SND_LOC_H
#define SND_LOC_H

#include <SDL/SDL_mutex.h>

/*
====================================================================
Data Structures
====================================================================
*/

// !!! if this is changed, the asm code must change !!!
typedef struct
{
	int			left;
	int			right;
} portable_samplepair_t;

typedef struct
{
	int 		length;
	int 		loopstart;
	int 		speed;
	int 		width;
	int 		channels;
	byte		data[1];		// variable sized
} sfxcache_t;

typedef struct sfx_s
{
	char 		name[MAX_QPATH];
	int			registration_sequence;
	sfxcache_t	*cache;
	char 		*truename;
	struct sfx_s *hashNext;
} sfx_t;

typedef struct playsound_s
{
	struct playsound_s	*prev, *next;
	sfx_t		*sfx;
	float		volume;
	float		attenuation;
	int			entnum;
	int			entchannel;
	qboolean	fixed_origin;
	vec3_t		origin;
	unsigned	begin;
} playsound_t;

typedef struct
{
	int			channels;
	int			samples;
	int			submission_chunk;
	int			samplepos;
	int			samplebits;
	int			speed;
	byte		*buffer;
} dma_t;

// !!! if this is changed, the asm code must change !!!
typedef struct
{
	sfx_t		*sfx;
	int			leftvol;
	int			rightvol;
	int			end;
	int 		pos;
	int			entnum;
	int			entchannel;
	vec3_t		origin;
	vec_t		dist_mult;
	int			master_vol;
	qboolean	fixed_origin;
	qboolean	autosound;
} channel_t;

/*
====================================================================
Shared Global Variables
====================================================================
*/

#define	MAX_CHANNELS 32
extern	channel_t   channels[MAX_CHANNELS];

extern	int		paintedtime;
extern	dma_t	dma;
extern	playsound_t	s_pendingplays;

extern cvar_t	*s_volume;
extern cvar_t	*s_show;
extern cvar_t	*s_loadas8bit;
extern cvar_t	*s_quality;

extern qboolean sound_started;
extern SDL_mutex *s_sound_mutex;

/*
====================================================================
Shared Function Prototypes
====================================================================
*/

// System specific functions
qboolean SNDDMA_Init(void);
int		SNDDMA_GetDMAPos(void);
void	SNDDMA_Shutdown(void);
void	SNDDMA_BeginPainting (void);
void	SNDDMA_Submit(void);

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
void Snd_Memset (void* dest, const int val, const size_t count);
#else
#define Snd_Memset memset
#endif

// Mixer functions
void S_InitScaletable (void);
sfxcache_t *S_LoadSound (sfx_t *s);
void S_IssuePlaysound (playsound_t *ps);
void S_PaintChannels(int endtime);
void S_TransferPaintBuffer(int endtime);
void S_PaintChannelFrom8 (channel_t *ch, sfxcache_t *sc, int count, int offset);
void S_PaintChannelFrom16 (channel_t *ch, sfxcache_t *sc, int count, int offset);

#endif // SND_LOC_H
