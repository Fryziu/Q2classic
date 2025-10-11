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

/*
** RW_SDL.C
**
** This file contains ALL Linux specific stuff having to do with the
** software refresh.  When a port is being made the following functions
** must be implemented by the port:
**
** GLimp_EndFrame
** GLimp_Init
** GLimp_InitGraphics
** GLimp_Shutdown
** GLimp_SwitchFullscreen
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include <SDL2/SDL.h>
#ifdef GL_QUAKE
#include "../ref_gl/gl_local.h"
#include "../linux/glw_linux.h"
#else
#include "../ref_soft/r_local.h"
#endif

#include "../client/keys.h"




/*****************************************************************************/
static qboolean                 input_active = false;

//static SDL_Surface *surface;
static SDL_Window *window = NULL;
static SDL_Surface *surface = NULL;

#ifndef GL_QUAKE
static unsigned int sdl_palettemode;
#endif

struct
{
	int key;
	int down;
} keyq[64];
int keyq_head=0;
int keyq_tail=0;

int config_notify=0;
int config_notify_width;
int config_notify_height;

#ifdef GL_QUAKE
glwstate_t glw_state;
#endif


//=================
// MOUSE
//=================
static int     mouse_buttonstate = 0;
static int     mouse_oldbuttonstate = 0;
int	mx, my;
static float old_windowed_mouse;
qboolean mouse_active = false;
extern qboolean	mouse_avail;

static cvar_t	*_windowed_mouse;
void KBD_Close(void);

void IN_MouseEvent (void)
{
	int		i;

	if (!mouse_avail)
		return;

// perform button actions
	for (i=0 ; i<5 ; i++)
	{
		if ( (mouse_buttonstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)) )
			Key_Event (K_MOUSE1 + i, true, Sys_Milliseconds());

		if ( !(mouse_buttonstate & (1<<i)) && (mouse_oldbuttonstate & (1<<i)) )
				Key_Event (K_MOUSE1 + i, false, Sys_Milliseconds());
	}

	mouse_oldbuttonstate = mouse_buttonstate;
}


static void IN_DeactivateMouse( void )
{
	if (!mouse_avail)
		return;

	if (mouse_active) {
		/* uninstall_grabs(); */
		mouse_active = false;
	}
}

static void IN_ActivateMouse( void )
{

	if (!mouse_avail)
		return;

	if (!mouse_active) {
		mx = my = 0; // don't spazz
		_windowed_mouse = NULL;
		old_windowed_mouse = 0;
		mouse_active = true;
	}
}

void IN_Activate(qboolean active)
{
    /*	if (active || vidmode_active) */
    if (active)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse();
}

/*****************************************************************************/

#if 0 /* SDL parachute should catch everything... */
// ========================================================================
// Tragic death handler
// ========================================================================

void TragicDeath(int signal_num)
{
	/* SDL_Quit(); */
	Sys_Error("This death brought to you by the number %d\n", signal_num);
}
#endif

//

static int XLateKey(SDL_Keysym *key)
{
	int				key_code;

	switch (key->sym)
	{
		case SDLK_KP_9:			key_code = K_KP_PGUP; break;
		case SDLK_KP_8:			key_code = K_KP_UPARROW; break;
		case SDLK_KP_7:			key_code = K_KP_HOME; break;
		case SDLK_KP_6:			key_code = K_KP_RIGHTARROW; break;
		case SDLK_KP_5:			key_code = K_KP_5; break;
		case SDLK_KP_4:			key_code = K_KP_LEFTARROW; break;
		case SDLK_KP_3:			key_code = K_KP_PGDN; break;
		case SDLK_KP_2:			key_code = K_KP_DOWNARROW; break;
		case SDLK_KP_1:			key_code = K_KP_END; break;
		case SDLK_KP_0:			key_code = K_KP_INS; break;
		case SDLK_KP_PERIOD:	key_code = K_KP_DEL; break;
		case SDLK_KP_DIVIDE:	key_code = K_KP_SLASH; break;
		case SDLK_KP_MULTIPLY:	key_code = '*'; break;
		case SDLK_KP_MINUS:		key_code = K_KP_MINUS; break;
		case SDLK_KP_PLUS:		key_code = K_KP_PLUS; break;
		case SDLK_KP_ENTER:		key_code = K_KP_ENTER; break;
		case SDLK_KP_EQUALS:	key_code = K_KP_ENTER; break;

		case SDLK_UP:			key_code = K_UPARROW; break;
		case SDLK_DOWN:			key_code = K_DOWNARROW; break;
		case SDLK_RIGHT:		key_code = K_RIGHTARROW; break;
		case SDLK_LEFT:			key_code = K_LEFTARROW; break;
		case SDLK_INSERT:		key_code = K_INS; break;
		case SDLK_HOME:			key_code = K_HOME; break;
		case SDLK_END:			key_code = K_END; break;
		case SDLK_PAGEUP:		key_code = K_PGUP; break;
		case SDLK_PAGEDOWN:		key_code = K_PGDN; break;
		case SDLK_DELETE:		key_code = K_DEL; break;
		case SDLK_BACKSPACE:	key_code = K_BACKSPACE; break;
		case SDLK_F1:			key_code = K_F1; break;
		case SDLK_F2:			key_code = K_F2; break;
		case SDLK_F3:			key_code = K_F3; break;
		case SDLK_F4:			key_code = K_F4; break;
		case SDLK_F5:			key_code = K_F5; break;
		case SDLK_F6:			key_code = K_F6; break;
		case SDLK_F7:			key_code = K_F7; break;
		case SDLK_F8:			key_code = K_F8; break;
		case SDLK_F9:			key_code = K_F9; break;
		case SDLK_F10:		key_code = K_F10; break;
		case SDLK_F11:		key_code = K_F11; break;
		case SDLK_F12:		key_code = K_F12; break;
		case SDLK_CAPSLOCK:		key_code = K_CAPSLOCK; break;
		case SDLK_RSHIFT:
		case SDLK_LSHIFT:		key_code = K_SHIFT; break;
		case SDLK_RCTRL:
		case SDLK_LCTRL:		key_code = K_CTRL; break;
		case SDLK_RALT:
		case SDLK_LALT:			key_code = K_ALT; break;
		case SDLK_TAB:			key_code = K_TAB; break;
		case SDLK_RETURN:		key_code = K_ENTER; break;
		case SDLK_ESCAPE:		key_code = K_ESCAPE; break;
		case SDLK_SPACE:		key_code = K_SPACE; break;
		case SDLK_PAUSE:		key_code = K_PAUSE; break;
		default:
			// FINAL FIX: Convert all character symbols to lowercase.
			// The engine expects lowercase for bindings and console commands.
			// tolower() will correctly handle A-Z and leave other symbols untouched.
			key_code = tolower(key->sym);
			break;
	}

	return key_code;
}


// This GetEvent function is written to use the *actual* local queue
// structure that exists in this file.
static void GetEvent(SDL_Event *event)
{
	int       key_code;

	switch (event->type)
	{
		// Usunęliśmy stąd obsługę SDL_WINDOWEVENT, ponieważ GLimp_AppActivate
		// jest lepszym i bardziej niezawodnym miejscem na tę logikę.

		case SDL_KEYDOWN:
			key_code = XLateKey(&event->key.keysym);
			keyq[keyq_head].key = key_code;
			keyq[keyq_head].down = true;
			keyq_head = (keyq_head + 1) & 63;
			break;

		case SDL_KEYUP:
			key_code = XLateKey(&event->key.keysym);
			keyq[keyq_head].key = key_code;
			keyq[keyq_head].down = false;
			keyq_head = (keyq_head + 1) & 63;
			break;

		case SDL_MOUSEWHEEL:
					if (event->wheel.y > 0) // Przewinięcie w górę (od siebie)
					{
						// Symulujemy pełne kliknięcie: wciśnięcie i natychmiastowe puszczenie
						Key_Event(K_MWHEELUP, true, Sys_Milliseconds());
						Key_Event(K_MWHEELUP, false, Sys_Milliseconds());
					}
					else if (event->wheel.y < 0) // Przewinięcie w dół (do siebie)
					{
						// Symulujemy pełne kliknięcie: wciśnięcie i natychmiastowe puszczenie
						Key_Event(K_MWHEELDOWN, true, Sys_Milliseconds());
						Key_Event(K_MWHEELDOWN, false, Sys_Milliseconds());
					}
					break;

		case SDL_QUIT:
			Sys_Quit();
			break;
	}
}

////
void HandleEvents(void)
{
	SDL_Event event;

	// Ten warunek jest teraz kontrolowany przez GLimp_AppActivate
	if (!input_active)
		return;

	// 1. Wypełnij lokalną kolejkę zdarzeniami klawiatury
	while (SDL_PollEvent(&event))
	{
		GetEvent(&event);
	}

	// 2. Opróżnij kolejkę, wysyłając zdarzenia do silnika
	while (keyq_head != keyq_tail)
	{
		Key_Event (keyq[keyq_tail].key, keyq[keyq_tail].down, Sys_Milliseconds());
		keyq_tail = (keyq_tail + 1) & 63;
	}

	// 3. Przetwarzaj mysz w każdej klatce, w której wejście jest aktywne
	int bstate;

	if (!mx && !my)
		SDL_GetRelativeMouseState(&mx, &my);

	mouse_buttonstate = 0;
	bstate = SDL_GetMouseState(NULL, NULL);
	if (SDL_BUTTON(1) & bstate)
		mouse_buttonstate |= (1 << 0);
	if (SDL_BUTTON(3) & bstate)
		mouse_buttonstate |= (1 << 1);
	if (SDL_BUTTON(2) & bstate)
		mouse_buttonstate |= (1 << 2);

	IN_MouseEvent();
}
/*****************************************************************************/

/*
** SWimp_Init
**
** This routine is responsible for initializing the implementation
** specific stuff in a software rendering subsystem.
*/
void SWimp_Init( void )
{
	// check if SDL is already initialized
	// NOTE: SDL_INIT_CDROM was removed in SDL2
	if (SDL_WasInit(SDL_INIT_AUDIO | SDL_INIT_VIDEO) == 0)
	{
		if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) == -1)
			Sys_Error("SDL_Init failed: %s\n", SDL_GetError());
	}

	SDL_ShowCursor(0);
	// install atexit handler to ensure SDL_Quit is called on shutdown
	atexit(KBD_Close);
}

#ifdef GL_QUAKE
void *GLimp_GetProcAddress(const char *func)
{
	return SDL_GL_GetProcAddress(func);
}

int GLimp_Init( void *hInstance, void *wndProc )
{
	// Call the main SDL/input initialization function
	SWimp_Init();

	// The function's signature in gl_local.h requires an int return value.
	// Since SWimp_Init() calls Sys_Error() and terminates on failure,
	// reaching this point means success. Returning 1 signals this.
	return 1;
}
#endif

/* fixme after implementing SDL2!
static void SetSDLIcon(void) {
#include "q2icon.xbm"
    SDL_Surface * icon;
    SDL_Color color;

*/
static unsigned char *SDLimp_InitGraphics( viddef_t *vid )
{
	Uint32 flags = 0;

	// =======================================================================
	//      FINAL FIX: Resource Cleanup ("Destroy Before Recreate")
	// =======================================================================
	// If a window already exists from a previous mode set, destroy it first.
	// This is critical for switching between fullscreen and windowed modes.
	if (window)
	{
		SDL_DestroyWindow(window);
		window = NULL;  // Set pointer to NULL after destroying
		surface = NULL; // The old surface is also invalid now
	}
	// =======================================================================

	// Use the engine's fullscreen cvar to determine the desired mode.
	// We use the modern, multi-monitor friendly "fullscreen desktop" mode.
	if (vid_fullscreen->value)
	{
		if (vid_fullscreen_exclusive->value)
			{
				// Użytkownik zażądał trybu ekskluzywnego
				flags |= SDL_WINDOW_FULLSCREEN;
				Com_Printf("Using exclusive fullscreen mode.\n");
			}
			else
			{
				// Domyślny, nowoczesny tryb pełnoekranowy (bez ramek)
				flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
				Com_Printf("Using borderless windowed fullscreen mode.\n");
			}
	}

	// OpenGL flag is required for the GL renderer
	flags |= SDL_WINDOW_OPENGL;

	// Create the window with the appropriate flags
	window = SDL_CreateWindow(
		"Quake II",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		vid->width,
		vid->height,
		flags
	);

	if (window == NULL)
	{
		Sys_Error("SDL_CreateWindow failed: %s", SDL_GetError());
		return NULL;
	}

	// Get the surface associated with the window (though less used in GL)
	surface = SDL_GetWindowSurface(window);
	if (surface == NULL)
	{
		Sys_Error("SDL_GetWindowSurface failed: %s", SDL_GetError());
		return NULL;
	}

	// Update the engine's viddef with the actual parameters we got.
	vid->width = surface->w;
	vid->height = surface->h;
	// The original Q2 viddef_t has no concept of 'rowbytes' or 'pitch';
	// the software renderer assumes pitch is equal to width. We honor that.
	// The buffer pointer is the RETURN VALUE of this function.
	// We are not using software rendering, but we keep this logic for consistency.

	// Software rendering would lock the surface here, but it's not needed for GL.
	// SDL_LockSurface(surface);

	return (unsigned char*)1; // Return a dummy non-NULL pointer for success in GL mode
}

#ifdef GL_QUAKE
void GLimp_BeginFrame( float camera_seperation )
{
}
#endif

/*
** GLimp_EndFrame
**
** This does an implementation specific copy from the backbuffer to the
** front buffer.  In the Win32 case it uses BitBlt or BltFast depending
** on whether we're using DIB sections/GDI or DDRAW.
*/
#ifdef GL_QUAKE
void GLimp_EndFrame (void)
{
	SDL_GL_SwapWindow(window);
}
#else
void SWimp_EndFrame (void)
{
	/* SDL_Flip(surface); */
	SDL_UpdateRect(surface, 0, 0, 0, 0);
}
#endif

/*
** GLimp_SetMode
*/
#ifdef GL_QUAKE
rserr_t GLimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
#else
rserr_t SWimp_SetMode( int *pwidth, int *pheight, int mode, qboolean fullscreen )
#endif
{
	viddef_t vid; // Create our temporary struct

	Com_Printf("setting mode %d:", mode );

	// Let the engine determine the width and height for this mode first. This is essential.
	if ( !R_GetModeInfo( pwidth, pheight, mode ) )
	{
		Com_Printf(" invalid mode\n" );
		return rserr_invalid_mode;
	}

	Com_Printf(" %d %d\n", *pwidth, *pheight);

	// Now, populate our temporary struct with the dimensions from R_GetModeInfo
	vid.width = *pwidth;
	vid.height = *pheight;

	// Call the new graphics init function, passing the address of our struct
	if ( !SDLimp_InitGraphics( &vid ) ) {
		Com_Printf( "SDLimp_InitGraphics failed.\n" );
		return rserr_invalid_mode;
	}

	// Update the original pointers with the actual dimensions we got from SDL
	*pwidth = vid.width;
	*pheight = vid.height;

#ifndef GL_QUAKE
	// This part remains for the software rendering path
	R_GammaCorrectAndSetPalette( ( const unsigned char * ) d_8to24table );
#endif

	// focus window SDL2 to grab mouse and keyboard inputs
	GLimp_AppActivate(true);


	return rserr_ok;
}

#ifndef GL_QUAKE
void SWimp_SetPalette( const unsigned char *palette )
{
	SDL_Color colors[256];

	int i;

	if (!input_active)
		return;

	if ( !palette )
	        palette = ( const unsigned char * ) sw_state.currentpalette;

	for (i = 0; i < 256; i++) {
		colors[i].r = palette[i*4+0];
		colors[i].g = palette[i*4+1];
		colors[i].b = palette[i*4+2];
	}

	SDL_SetPalette(surface, sdl_palettemode, colors, 0, 256);
}
#endif

/*
** SWimp_Shutdown
**
** System specific graphics subsystem shutdown routine.  Destroys
** DIBs or DDRAW surfaces as appropriate.
*/

void SWimp_Shutdown( void )
{
	if (surface)
		SDL_FreeSurface(surface);
	surface = NULL;

	if (SDL_WasInit(SDL_INIT_EVERYTHING) == SDL_INIT_VIDEO)
		SDL_Quit();
	else
		SDL_QuitSubSystem(SDL_INIT_VIDEO);

	input_active = false;
}

#ifdef GL_QUAKE
void GLimp_Shutdown( void )
{
	SWimp_Shutdown();
}
#endif

/*
** GLimp_AppActivate
*/
#ifdef GL_QUAKE
void GLimp_AppActivate( qboolean active )
{
	if (active)
	{
		input_active = true;
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
	else
	{
		input_active = false;
		SDL_SetRelativeMouseMode(SDL_FALSE);
	}
}
#else
void SWimp_AppActivate( qboolean active )
{
}
#endif

//===============================================================================

/*
================
Sys_MakeCodeWriteable
================
*/
void Sys_MakeCodeWriteable (unsigned long startaddr, unsigned long length)
{

	int r;
	unsigned long addr;
	int psize = getpagesize();

	addr = (startaddr & ~(psize-1)) - psize;

//	fprintf(stderr, "writable code %lx(%lx)-%lx, length=%lx\n", startaddr,
//			addr, startaddr+length, length);

	r = mprotect((char*)addr, length + startaddr - addr + psize, 7);

	if (r < 0)
    		Sys_Error("Protection change failed\n");

}

/*****************************************************************************/
/* KEYBOARD                                                                  */
/*****************************************************************************/
/*
void HandleEvents(void)
{
	SDL_Event event;
	static int KBD_Update_Flag;

	if (KBD_Update_Flag == 1)
		return;

	KBD_Update_Flag = 1;

	// get events from x server
	if (X11_active)
	{
		int bstate;

		while (SDL_PollEvent(&event))
			GetEvent(&event);

		if (!mx && !my)
			SDL_GetRelativeMouseState(&mx, &my);

		mouse_buttonstate = 0;
		bstate = SDL_GetMouseState(NULL, NULL);
		if (SDL_BUTTON(1) & bstate)
			mouse_buttonstate |= (1 << 0);
		if (SDL_BUTTON(3) & bstate) // quake2 has the right button be mouse2
			mouse_buttonstate |= (1 << 1);
		if (SDL_BUTTON(2) & bstate) // quake2 has the middle button be mouse3
			mouse_buttonstate |= (1 << 2);
		if (SDL_BUTTON(6) & bstate)
			mouse_buttonstate |= (1 << 3);
		if (SDL_BUTTON(7) & bstate)
			mouse_buttonstate |= (1 << 4);

		if(!_windowed_mouse)
			_windowed_mouse = Cvar_Get("_windowed_mouse", "1", CVAR_ARCHIVE);

		if (old_windowed_mouse != _windowed_mouse->value) {
			old_windowed_mouse = _windowed_mouse->value;

			if (!_windowed_mouse->value) {
				// ungrab the pointer
				SDL_WM_GrabInput(SDL_GRAB_OFF);
			} else {
				// grab the pointer
				SDL_WM_GrabInput(SDL_GRAB_ON);
			}
		}
		IN_MouseEvent();

		while (keyq_head != keyq_tail)
		{
			Key_Event (keyq[keyq_tail].key, keyq[keyq_tail].down, Sys_Milliseconds());
			keyq_tail = (keyq_tail + 1) & 63;
		}
	}

	KBD_Update_Flag = 0;
}
*/


void KBD_Close(void)
{
	keyq_head = 0;
	keyq_tail = 0;

	memset(keyq, 0, sizeof(keyq));
}

char *Sys_GetClipboardData(void)
{
	return NULL;
}

