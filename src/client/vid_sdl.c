/*
=============================================================================

VID_SDL.C -- SDL2 video backend

=============================================================================
*/

#include "client.h"

cvar_t		*vid_gamma;
cvar_t		*vid_xpos;
cvar_t		*vid_ypos;
cvar_t		*vid_fullscreen;
cvar_t		*vid_fullscreen_exclusive;

viddef_t	viddef;

static qboolean vid_restart = false;
static qboolean vid_active = false;

void VID_Restart_f (void)
{
	vid_restart = true;
}

void VID_NewWindow ( int width, int height)
{
	viddef.width  = width;
	viddef.height = height;
}

void VID_Shutdown (void)
{
	if ( vid_active )
	{
		R_Shutdown ();
		vid_active = false;
	}
}

void VID_CheckChanges (void)
{
	if ( vid_restart )
	{
		cl.force_refdef = true;
		S_StopAllSounds();

		vid_fullscreen->modified = true;
		cl.refresh_prepped = false;
		cls.disable_screen = true;

		VID_Shutdown();

		Com_Printf( "--------- [Loading Renderer] ---------\n" );

		vid_active = true;
		if ( R_Init( 0, 0 ) == -1 )
		{
			R_Shutdown();
			Com_Error (ERR_FATAL, "Couldn't initialize renderer!");
		}

		Com_Printf( "------------------------------------\n");

		vid_restart = false;
		cls.disable_screen = false;

		IN_Activate(false);
		IN_Activate(true);
	}
}

void VID_Init (void)
{
	vid_xpos = Cvar_Get ("vid_xpos", "3", CVAR_ARCHIVE);
	vid_ypos = Cvar_Get ("vid_ypos", "22", CVAR_ARCHIVE);
	vid_fullscreen = Cvar_Get ("vid_fullscreen", "1", CVAR_ARCHIVE);
	vid_fullscreen_exclusive = Cvar_Get ("vid_fullscreen_exclusive", "0", CVAR_ARCHIVE);
	vid_gamma = Cvar_Get( "vid_gamma", "1.0", CVAR_ARCHIVE );

	Cmd_AddCommand ("vid_restart", VID_Restart_f);

	vid_restart = true;
	vid_active = false;
	VID_CheckChanges();
}
