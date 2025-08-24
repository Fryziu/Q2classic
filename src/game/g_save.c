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

#include "g_local.h"

#define Function(f) {#f, f}

mmove_t mmove_reloc;

field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"model", FOFS(model), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"accel", FOFS(accel), F_FLOAT},
	{"decel", FOFS(decel), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"pathtarget", FOFS(pathtarget), F_LSTRING},
	{"deathtarget", FOFS(deathtarget), F_LSTRING},
	{"killtarget", FOFS(killtarget), F_LSTRING},
	{"combattarget", FOFS(combattarget), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"delay", FOFS(delay), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"move_origin", FOFS(move_origin), F_VECTOR},
	{"move_angles", FOFS(move_angles), F_VECTOR},
	{"style", FOFS(style), F_INT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"sounds", FOFS(sounds), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(dmg), F_INT},
	{"mass", FOFS(mass), F_INT},
	{"volume", FOFS(volume), F_FLOAT},
	{"attenuation", FOFS(attenuation), F_FLOAT},
	{"map", FOFS(map), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},

	{"goalentity", FOFS(goalentity), F_EDICT, FFL_NOSPAWN},
	{"movetarget", FOFS(movetarget), F_EDICT, FFL_NOSPAWN},
	{"enemy", FOFS(enemy), F_EDICT, FFL_NOSPAWN},
	{"oldenemy", FOFS(oldenemy), F_EDICT, FFL_NOSPAWN},
	{"activator", FOFS(activator), F_EDICT, FFL_NOSPAWN},
	{"groundentity", FOFS(groundentity), F_EDICT, FFL_NOSPAWN},
	{"teamchain", FOFS(teamchain), F_EDICT, FFL_NOSPAWN},
	{"teammaster", FOFS(teammaster), F_EDICT, FFL_NOSPAWN},
	{"owner", FOFS(owner), F_EDICT, FFL_NOSPAWN},
	{"mynoise", FOFS(mynoise), F_EDICT, FFL_NOSPAWN},
	{"mynoise2", FOFS(mynoise2), F_EDICT, FFL_NOSPAWN},
	{"target_ent", FOFS(target_ent), F_EDICT, FFL_NOSPAWN},
	{"chain", FOFS(chain), F_EDICT, FFL_NOSPAWN},

	{"prethink", FOFS(prethink), F_FUNCTION, FFL_NOSPAWN},
	{"think", FOFS(think), F_FUNCTION, FFL_NOSPAWN},
	{"blocked", FOFS(blocked), F_FUNCTION, FFL_NOSPAWN},
	{"touch", FOFS(touch), F_FUNCTION, FFL_NOSPAWN},
	{"use", FOFS(use), F_FUNCTION, FFL_NOSPAWN},
	{"pain", FOFS(pain), F_FUNCTION, FFL_NOSPAWN},
	{"die", FOFS(die), F_FUNCTION, FFL_NOSPAWN},

	{"stand", FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN},
	{"idle", FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN},
	{"search", FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN},
	{"walk", FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN},
	{"run", FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN},
	{"dodge", FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN},
	{"attack", FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN},
	{"melee", FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN},
	{"sight", FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN},
	{"checkattack", FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN},
	{"currentmove", FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN},

	{"endfunc", FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN},

	// temp spawn vars -- only valid when the spawn function is called
	{"lip", STOFS(lip), F_INT, FFL_SPAWNTEMP},
	{"distance", STOFS(distance), F_INT, FFL_SPAWNTEMP},
	{"height", STOFS(height), F_INT, FFL_SPAWNTEMP},
	{"noise", STOFS(noise), F_LSTRING, FFL_SPAWNTEMP},
	{"pausetime", STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP},
	{"item", STOFS(item), F_LSTRING, FFL_SPAWNTEMP},

	//need for item field in edict struct, FFL_SPAWNTEMP item will be skipped on saves
	{"item", FOFS(item), F_ITEM},

	{"gravity", STOFS(gravity), F_LSTRING, FFL_SPAWNTEMP},
	{"sky", STOFS(sky), F_LSTRING, FFL_SPAWNTEMP},
	{"skyrotate", STOFS(skyrotate), F_FLOAT, FFL_SPAWNTEMP},
	{"skyaxis", STOFS(skyaxis), F_VECTOR, FFL_SPAWNTEMP},
	{"minyaw", STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"maxyaw", STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"minpitch", STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"maxpitch", STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"nextmap", STOFS(nextmap), F_LSTRING, FFL_SPAWNTEMP},

	{0, 0, 0, 0}

};

field_t		levelfields[] =
{
	{"changemap", LLOFS(changemap), F_LSTRING},
                   
	{"sight_client", LLOFS(sight_client), F_EDICT},
	{"sight_entity", LLOFS(sight_entity), F_EDICT},
	{"sound_entity", LLOFS(sound_entity), F_EDICT},
	{"sound2_entity", LLOFS(sound2_entity), F_EDICT},

	{NULL, 0, F_INT}
};

field_t		clientfields[] =
{
	{"pers.weapon", CLOFS(pers.weapon), F_ITEM},
	{"pers.lastweapon", CLOFS(pers.lastweapon), F_ITEM},
	{"newweapon", CLOFS(newweapon), F_ITEM},

	{NULL, 0, F_INT}
};

/*
============
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
void InitGame (void)
{
	gi.dprintf ("==== InitGame ====\n");

	gun_x = gi.cvar ("gun_x", "0", 0);
	gun_y = gi.cvar ("gun_y", "0", 0);
	gun_z = gi.cvar ("gun_z", "0", 0);

	//FIXME: sv_ prefix is wrong for these
	sv_rollspeed = gi.cvar ("sv_rollspeed", "200", 0);
	sv_rollangle = gi.cvar ("sv_rollangle", "2", 0);
	sv_maxvelocity = gi.cvar ("sv_maxvelocity", "2000", 0);
	sv_gravity = gi.cvar ("sv_gravity", "800", 0);

	// noset vars
	dedicated = gi.cvar ("dedicated", "0", CVAR_NOSET);

	// latched vars
	sv_cheats = gi.cvar ("cheats", "0", CVAR_SERVERINFO|CVAR_LATCH);
	gi.cvar ("gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar ("gamedate", __DATE__ , CVAR_SERVERINFO | CVAR_LATCH);

	maxclients = gi.cvar ("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	maxspectators = gi.cvar ("maxspectators", "4", CVAR_SERVERINFO);
	deathmatch = gi.cvar ("deathmatch", "0", CVAR_LATCH);
	coop = gi.cvar ("coop", "0", CVAR_LATCH);
	skill = gi.cvar ("skill", "1", CVAR_LATCH);
	maxentities = gi.cvar ("maxentities", "1024", CVAR_LATCH);

	// change anytime vars
	dmflags = gi.cvar ("dmflags", "0", CVAR_SERVERINFO);
	fraglimit = gi.cvar ("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar ("timelimit", "0", CVAR_SERVERINFO);
	password = gi.cvar ("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar ("spectator_password", "", CVAR_USERINFO);
	needpass = gi.cvar ("needpass", "0", CVAR_SERVERINFO);
	filterban = gi.cvar ("filterban", "1", 0);

	g_select_empty = gi.cvar ("g_select_empty", "0", CVAR_ARCHIVE);

	run_pitch = gi.cvar ("run_pitch", "0.002", 0);
	run_roll = gi.cvar ("run_roll", "0.005", 0);
	bob_up  = gi.cvar ("bob_up", "0.005", 0);
	bob_pitch = gi.cvar ("bob_pitch", "0.002", 0);
	bob_roll = gi.cvar ("bob_roll", "0.002", 0);

	// flood control
	flood_msgs = gi.cvar ("flood_msgs", "4", 0);
	flood_persecond = gi.cvar ("flood_persecond", "4", 0);
	flood_waitdelay = gi.cvar ("flood_waitdelay", "10", 0);

	// dm map list
	sv_maplist = gi.cvar ("sv_maplist", "", 0);

	// items
	InitItems ();

	game.helpmessage1[0] = 0;
	game.helpmessage2[0] = 0;

	// initialize all entities for this game
	game.maxentities = maxentities->value;
	g_edicts =  gi.TagMalloc (game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// initialize all clients for this game
	game.maxclients = maxclients->value;
	game.clients = gi.TagMalloc (game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients+1;
}

//=========================================================

void WriteField1 (FILE *f, field_t *field, byte *base)
{
	void		*p;
	int			len;
	int			index;

	if (field->flags & FFL_SPAWNTEMP)
		return;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR:
	case F_IGNORE:
		break;

	case F_LSTRING:
	case F_GSTRING:
		if ( *(char **)p )
			len = strlen(*(char **)p) + 1;
		else
			len = 0;
		*(int *)p = len;
		break;
	case F_EDICT:
		if ( *(edict_t **)p == NULL)
			index = -1;
		else
			index = *(edict_t **)p - g_edicts;
		*(int *)p = index;
		break;
	case F_CLIENT:
		if ( *(gclient_t **)p == NULL)
			index = -1;
		else
			index = *(gclient_t **)p - game.clients;
		*(int *)p = index;
		break;
	case F_ITEM:
		if ( *(edict_t **)p == NULL)
			index = -1;
		else
			index = *(gitem_t **)p - itemlist;
		*(int *)p = index;
		break;

	//relative to code segment
	case F_FUNCTION:
		if (*(byte **)p == NULL)
			index = 0;
		else
			index = *(byte **)p - ((byte *)InitGame);
		*(int *)p = index;
		break;

	//relative to data segment
	case F_MMOVE:
		if (*(byte **)p == NULL)
			index = 0;
		else
			index = *(byte **)p - (byte *)&mmove_reloc;
		*(int *)p = index;
		break;

	default:
		gi.error ("WriteEdict: unknown field type");
	}
}


void WriteField2 (FILE *f, field_t *field, byte *base)
{
	int			len;
	void		*p;

	if (field->flags & FFL_SPAWNTEMP)
		return;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_LSTRING:
		if ( *(char **)p )
		{
			len = strlen(*(char **)p) + 1;
			if (fwrite (*(char **)p, len, 1, f) !=1)
				{
				fclose(f);
					gi.error("WriteField2 error");
				}
		}
		break;
	default:
		break;
	}
}

void ReadField (FILE *f, field_t *field, byte *base)
{
	void		*p;
	int			len;
	int			index;

	if (field->flags & FFL_SPAWNTEMP)
		return;

	p = (void *)(base + field->ofs);
	switch (field->type)
	{
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR:
	case F_IGNORE:
		break;

	case F_LSTRING:
		len = *(int *)p;
		if (!len)
			*(char **)p = NULL;
		else
		{
			*(char **)p = gi.TagMalloc (len, TAG_LEVEL);
			if (fread (*(char **)p, len, 1, f) !=1)
				{
				fclose(f);
						 gi.error("ReadField error - F_LSTRING");
				}
		}
		break;
	case F_EDICT:
		index = *(int *)p;
		if ( index == -1 )
			*(edict_t **)p = NULL;
		else
			*(edict_t **)p = &g_edicts[index];
		break;
	case F_CLIENT:
		index = *(int *)p;
		if ( index == -1 )
			*(gclient_t **)p = NULL;
		else
			*(gclient_t **)p = &game.clients[index];
		break;
	case F_ITEM:
		index = *(int *)p;
		if ( index == -1 )
			*(gitem_t **)p = NULL;
		else
			*(gitem_t **)p = &itemlist[index];
		break;

	//relative to code segment
	case F_FUNCTION:
		index = *(int *)p;
		if ( index == 0 )
			*(byte **)p = NULL;
		else
			*(byte **)p = ((byte *)InitGame) + index;
		break;

	//relative to data segment
	case F_MMOVE:
		index = *(int *)p;
		if (index == 0)
			*(byte **)p = NULL;
		else
			*(byte **)p = (byte *)&mmove_reloc + index;
		break;

	default:
		gi.error ("ReadEdict: unknown field type");
	}
}

//=========================================================

/*
==============
WriteClient

All pointer variables (except function pointers) must be handled specially.
==============
*/
qboolean WriteClient (FILE *f, gclient_t *client)
{
	field_t		*field;

    // Sprawdź operację zapisu
	if (fwrite (client, sizeof(*client), 1, f) != 1)
        return false; // Błąd zapisu

	for (field=clientfields ; field->name ; field++)
	{
		// Załóżmy, że WriteField nie może zawieść lub jest sprawdzane wewnątrz
		WriteField1 (f, field, (byte *)client);
	}
    return true; // Sukces
}

/*
==============
ReadClient

All pointer variables (except function pointers) must be handled specially.
==============
*/
void ReadClient (FILE *f, gclient_t *client)
{
	field_t		*field;

	 if (fread (client, sizeof(*client), 1, f) !=1)
		 {
		 fclose(f);
		 gi.error("ReadClient error: Savegame is corrupted or incomplete.");
		 }

	for (field=clientfields ; field->name ; field++)
	{
		ReadField (f, field, (byte *)client);
	}
}

/*
============
WriteGame

This will be called whenever the game goes to a new level,
and when the user explicitly saves the game.

Game information include cross level data, like multi level
triggers, help computer info, and all client states.

A single player death will automatically restore from the
last save position.
============
*/
// WERSJA FINALNA, BEZPIECZNA I SOLIDNA
void WriteGame (char *filename, qboolean autosave)
{
	FILE	*f;
	int		i;
	char	str[16];
    qboolean success = true; // Flaga śledząca, czy wszystko się udało

	if (!autosave)
		SaveClientData ();

	f = fopen (filename, "wb");
	if (!f)
    {
		gi.error ("Couldn't open savegame file: %s", filename);
        return;
    }

    // Zapisz nagłówek z datą kompilacji
	memset (str, 0, sizeof(str));
	strcpy (str, __DATE__);
	if (fwrite (str, sizeof(str), 1, f) != 1)
    {
        success = false;
    }

    // Zapisz główną strukturę gry
    if (success)
    {
        game.autosaved = autosave;
        if (fwrite (&game, sizeof(game), 1, f) != 1)
        {
            success = false;
        }
        game.autosaved = false; // Przywróć stan na wszelki wypadek
    }

	// Zapisz dane wszystkich klientów
    if (success)
    {
        for (i=0 ; i<game.maxclients ; i++)
        {
            // Użyj poprawionej funkcji WriteClient i sprawdź jej wynik
            if (!WriteClient (f, &game.clients[i]))
            {
                success = false;
                break; // Przerwij pętlę, jeśli zapis jednego klienta się nie powiedzie
            }
        }
    }

    //
    // Tutaj dodałbyś pętlę dla WriteEdict w identyczny sposób
    //
    // if (success)
    // {
    //     for (i=0; i < game.num_edicts; i++)
    //     {
    //         if (!WriteEdict(f, &g_edicts[i]))
    //         {
    //             success = false;
    //             break;
    //         }
    //     }
    // }


	// --- Zakończ operację ---

	fclose (f); // Zawsze zamknij plik, niezależnie od wyniku

    // Jeśli w którymkolwiek momencie wystąpił błąd, poinformuj o tym.
	if (!success)
	{
        // Opcjonalnie: usuń uszkodzony plik, aby nie zostawiać śmieci
        remove(filename);
		gi.error ("Failed to write savegame to %s. The disk may be full or protected.", filename);
	}
    // Jeśli nie było błędu, funkcja po prostu się kończy, a gracz wie, że zapis się powiódł.
}

// WERSJA BEZPIECZNA I ZALECANA
void ReadGame (char *filename)
{
	FILE	*f;
	int		i;
	char	str[16];

	gi.FreeTags (TAG_GAME);

	f = fopen (filename, "rb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	// SPRAWDŹ WYNIK #1
	if (fread (str, sizeof(str), 1, f) != 1)
	{
		fclose(f);
		gi.error("Savegame is corrupted (could not read header): %s", filename);
	}

	if (strcmp (str, __DATE__))
	{
		fclose (f);
		gi.error ("Savegame from an older version.\n");
	}

	g_edicts =  gi.TagMalloc (game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;

	// SPRAWDŹ WYNIK #2
	if (fread (&game, sizeof(game), 1, f) != 1)
	{
		fclose(f);
		gi.error("Savegame is corrupted (could not read game state): %s", filename);
	}

	game.clients = gi.TagMalloc (game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	for (i=0 ; i<game.maxclients ; i++)
		ReadClient (f, &game.clients[i]);

	fclose (f);
}

//==========================================================


/*
==============
WriteEdict

All pointer variables (except function pointers) must be handled specially.
==============
*/
qboolean WriteEdict (FILE *f, edict_t *ent)
{
	field_t		*field;
	edict_t		temp;

	temp = *ent;

	for (field=fields ; field->name ; field++)
	{
		WriteField1 (f, field, (byte *)&temp);
	}

	// Sprawdź główną operację zapisu
	if (fwrite (&temp, sizeof(temp), 1, f) != 1)
		return false; // Błąd zapisu

	for (field=fields ; field->name ; field++)
	{
		// Tutaj również powinno być sprawdzanie, jeśli WriteField2 może zawieść
		WriteField2 (f, field, (byte *)ent);
	}

    return true; // Sukces
}

/*
==============
WriteLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
// ZMIANA 1: Zmień typ zwracany na qboolean
qboolean WriteLevelLocals (FILE *f)
{
	field_t		*field;
	level_locals_t		temp;
	// size_t			size; // Już niepotrzebna

	// all of the ints, floats, and vectors stay as they are
	temp = level;

	// change the pointers to lengths or indexes
	for (field=levelfields ; field->name ; field++)
	{
		WriteField1 (f, field, (byte *)&temp);
	}

	// write the block and check the return value
	if (fwrite (&temp, sizeof(temp), 1, f) != 1)
	{
		// ZMIANA 2: Zasygnalizuj błąd
		return false;
	}

	// now write any allocated data following the edict
	for (field=levelfields ; field->name ; field++)
	{
		// Tutaj również powinno być sprawdzanie, jeśli WriteField2 może zawieść
		WriteField2 (f, field, (byte *)&level);
	}

    // ZMIANA 3: Zasygnalizuj sukces
    return true;
}


/*
==============
ReadEdict

All pointer variables (except function pointers) must be handled specially.
==============
*/
void ReadEdict (FILE *f, edict_t *ent)
{
	field_t		*field;
	if (fread (ent, sizeof(*ent), 1, f) !=1)
		{
		fclose(f);
		gi.error("ReadEdict error: Savegame is corrupted or incomplete.");
		}

	for (field=fields ; field->name ; field++)
	{
		ReadField (f, field, (byte *)ent);
	}
}

/*
==============
ReadLevelLocals

All pointer variables (except function pointers) must be handled specially.
==============
*/
void ReadLevelLocals (FILE *f)
{
	field_t		*field;

	if (fread (&level, sizeof(level), 1, f) != 1)
	{
		// 1. Zamykamy plik PRZED zakończeniem programu.
		fclose(f);

		// 2. Ulepszony, spójny i czytelny komunikat o błędzie.
		gi.error("ReadLevelLocals error: Savegame is corrupted or incomplete.");
	}

	for (field=levelfields ; field->name ; field++)
	{
		ReadField (f, field, (byte *)&level);
	}
}

/*
=================
WriteLevel

=================
*/
// ZMIANA 1: Zmiana typu zwracanego
qboolean WriteLevel (char *filename)
{
	int		i;
	edict_t	*ent;
	FILE	*f;
	void	*base;
	qboolean success = true; // Flaga śledząca powodzenie

	f = fopen (filename, "wb");
	if (!f)
    {
		gi.error ("Couldn't open %s", filename);
        return false; // Zwróć błąd, jeśli nie można otworzyć pliku
    }

	// --- Sekwencja zapisu z pełnym sprawdzaniem ---

	// write out edict size for checking
	i = sizeof(edict_t);
	if (fwrite (&i, sizeof(i), 1, f) != 1)
        success = false;

	// write out a function pointer for checking
    if (success)
    {
	    base = (void *)InitGame;
	    if (fwrite (&base, sizeof(base), 1, f) != 1)
            success = false;
    }

	// write out level_locals_t
    if (success)
    {
        // Używamy naszej nowej, bezpiecznej funkcji!
	    if (!WriteLevelLocals (f))
            success = false;
    }

	// write out all the entities
    if (success)
    {
	    for (i=0 ; i<globals.num_edicts ; i++)
	    {
		    ent = &g_edicts[i];
		    if (!ent->inuse)
			    continue;

            // Sprawdź zapis indeksu
		    if (fwrite (&i, sizeof(i), 1, f) != 1)
            {
                success = false;
                break; // Przerwij pętlę
            }

            // Sprawdź zapis edictu za pomocą naszej nowej, bezpiecznej funkcji
		    if (!WriteEdict (f, ent))
            {
                success = false;
                break; // Przerwij pętlę
            }
	    }
    }

    // Zapisz znacznik końca
    if (success)
    {
	    i = -1;
	    if (fwrite (&i, sizeof(i), 1, f) != 1)
            success = false;
    }

	fclose (f);

    // Zwróć ostateczny status operacji
    return success;
}


/*
=================
ReadLevel

SpawnEntities will allready have been called on the
level the same way it was when the level was saved.

That is necessary to get the baselines
set up identically.

The server will have cleared all of the world links before
calling ReadLevel.

No clients are connected yet.
=================
*/
void ReadLevel (char *filename)
{
	int		entnum;
	FILE	*f;
	int		i;
	void	*base;
	edict_t	*ent;
	// size_t	size; // Już niepotrzebna

	f = fopen (filename, "rb");
	if (!f)
		gi.error ("Couldn't open %s", filename);

	// free any dynamic memory allocated by loading the level
	// base state
	gi.FreeTags (TAG_LEVEL);

	// wipe all the entities
	memset (g_edicts, 0, game.maxentities*sizeof(g_edicts[0]));
	globals.num_edicts = maxclients->value+1;

	// --- Rozpocznij sekwencję wczytywania z pełnym sprawdzaniem ---

	// Krok 1: Wczytaj zapisany rozmiar edict_t
	if (fread (&i, sizeof(i), 1, f) != 1)
	{
		fclose(f);
		gi.error("ReadLevel error: Savegame is corrupted or incomplete (could not read edict size).");
	}

	// Krok 2: Sprawdź, czy rozmiar się zgadza (logika integralności)
	if (i != sizeof(edict_t))
	{
		fclose (f);
		gi.error ("ReadLevel error: Savegame is from an incompatible version (mismatched edict size).");
	}

	// Krok 3: Wczytaj zapisany adres bazowy
	if (fread (&base, sizeof(base), 1, f) != 1)
	{
		fclose(f);
		gi.error("ReadLevel error: Savegame is corrupted or incomplete (could not read base address).");
	}

	// Krok 4: Sprawdź, czy adres się zgadza (logika integralności)
#ifdef _WIN32
	if (base != (void *)InitGame)
	{
		fclose (f);
		gi.error ("ReadLevel error: Savegame is from an incompatible version (function pointers have moved).");
	}
#else
    // W Linuksie ta wartość może się różnić z powodu ASLR, więc tylko drukujemy różnicę.
	gi.dprintf("Function offsets %d\n", ((byte *)base) - ((byte *)InitGame));
#endif

	// load the level locals
	ReadLevelLocals (f); // Zakładając, że ta funkcja sama obsługuje błędy i zamyka plik

	// load all the entities
	while (1)
	{
		if (fread (&entnum, sizeof(entnum), 1, f) != 1)
		{
			fclose (f);
			gi.error ("ReadLevel error: Savegame is corrupted (failed to read entnum).");
		}
		if (entnum == -1)
			break;
		if (entnum >= globals.num_edicts)
			globals.num_edicts = entnum+1;

		ent = &g_edicts[entnum];
		ReadEdict (f, ent); // Zakładając, że ta funkcja sama obsługuje błędy i zamyka plik

		// let the server rebuild world links for this ent
		memset (&ent->area, 0, sizeof(ent->area));
		gi.linkentity (ent);
	}

	fclose (f);

	// mark all clients as unconnected
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = &g_edicts[i+1];
		ent->client = game.clients + i;
		ent->client->pers.connected = false;
	}

	// do any load time things at this point
	for (i=0 ; i<globals.num_edicts ; i++)
	{
		ent = &g_edicts[i];

		if (!ent->inuse)
			continue;

		// fire any cross-level triggers
		if (ent->classname)
			if (strcmp(ent->classname, "target_crosslevel_target") == 0)
				ent->nextthink = level.time + ent->delay;
	}
}
