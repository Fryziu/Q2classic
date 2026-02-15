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


/// RW_SDL.C

// This file contains ALL Linux specific stuff having to do with the
// software refresh.  When a port is being made the following functions
// must be implemented by the port:

// GLimp_EndFrame
// GLimp_Init
// GLimp_InitGraphics
// GLimp_Shutdown
// GLimp_SwitchFullscreen

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>

#include <SDL2/SDL.h>
#ifdef GL_QUAKE
#include "../ref_gl/gl_local.h"
#include "../linux/glw_linux.h"
#endif

#include "../client/keys.h"

// --- Globalne stany ---
static SDL_Window *window = NULL;
static SDL_GLContext gl_context = NULL;
static qboolean input_active = false;

struct {
    int key;
    int down;
} keyq[64];
int keyq_head = 0;
int keyq_tail = 0;

#ifdef GL_QUAKE
glwstate_t glw_state;
#endif

int mx, my;
qboolean mouse_active = false;
extern qboolean mouse_avail;

// --- Obsługa Klawiatury i Myszy ---

static int XLateKey(SDL_Keysym *key) {
    // Sprawdź stan modyfikatorów (NumLock)
    SDL_Keymod mod = SDL_GetModState();
    qboolean numlock = (mod & KMOD_NUM) != 0;

    switch (key->sym) {
        // Klawiatura numeryczna - mapowanie zależne od NumLock
        case SDLK_KP_7:      return numlock ? '7' : K_KP_HOME;
        case SDLK_KP_8:      return numlock ? '8' : K_KP_UPARROW;
        case SDLK_KP_9:      return numlock ? '9' : K_KP_PGUP;
        case SDLK_KP_4:      return numlock ? '4' : K_KP_LEFTARROW;
        case SDLK_KP_5:      return numlock ? '5' : K_KP_5;
        case SDLK_KP_6:      return numlock ? '6' : K_KP_RIGHTARROW;
        case SDLK_KP_1:      return numlock ? '1' : K_KP_END;
        case SDLK_KP_2:      return numlock ? '2' : K_KP_DOWNARROW;
        case SDLK_KP_3:      return numlock ? '3' : K_KP_PGDN;
        case SDLK_KP_0:      return numlock ? '0' : K_KP_INS;
        case SDLK_KP_PERIOD: return numlock ? '.' : K_KP_DEL;
        
        // Klawisze KP, które nie zależą od NumLock
        case SDLK_KP_DIVIDE:   return K_KP_SLASH;
        case SDLK_KP_MULTIPLY: return '*';
        case SDLK_KP_MINUS:    return K_KP_MINUS;
        case SDLK_KP_PLUS:     return K_KP_PLUS;
        case SDLK_KP_ENTER:    return K_KP_ENTER;
        case SDLK_KP_EQUALS:   return K_KP_ENTER;

        // Klawisze funkcyjne (F1-F19)
        case SDLK_F1:  return K_F1;
        case SDLK_F2:  return K_F2;
        case SDLK_F3:  return K_F3;
        case SDLK_F4:  return K_F4;
        case SDLK_F5:  return K_F5;
        case SDLK_F6:  return K_F6;
        case SDLK_F7:  return K_F7;
        case SDLK_F8:  return K_F8;
        case SDLK_F9:  return K_F9;
        case SDLK_F10: return K_F10;
        case SDLK_F11: return K_F11;
        case SDLK_F12: return K_F12;
        case SDLK_F13:  return K_F13;
        case SDLK_F14:  return K_F14;
        case SDLK_F15:  return K_F15;
        case SDLK_F16:  return K_F16;
        case SDLK_F17:  return K_F17;
        case SDLK_F18:  return K_F18;
        case SDLK_F19:  return K_F19;
   
        // Strzałki (poza numeryczną) i nawigacja
        case SDLK_UP:        return K_UPARROW;
        case SDLK_DOWN:      return K_DOWNARROW;
        case SDLK_RIGHT:     return K_RIGHTARROW;
        case SDLK_LEFT:      return K_LEFTARROW;
        case SDLK_INSERT:    return K_INS;
        case SDLK_HOME:      return K_HOME;
        case SDLK_END:       return K_END;
        case SDLK_PAGEUP:    return K_PGUP;
        case SDLK_PAGEDOWN:  return K_PGDN;
        case SDLK_DELETE:    return K_DEL;
        case SDLK_BACKSPACE: return K_BACKSPACE;

        // Modyfikatory
        case SDLK_CAPSLOCK:  return K_CAPSLOCK;
        case SDLK_RSHIFT:
        case SDLK_LSHIFT:    return K_SHIFT;
        case SDLK_RCTRL:
        case SDLK_LCTRL:     return K_CTRL;
        case SDLK_RALT:
        case SDLK_LALT:      return K_ALT;
        
        // Pozostałe specjalne
        case SDLK_TAB:       return K_TAB;
        case SDLK_RETURN:    return K_ENTER;
        case SDLK_ESCAPE:    return K_ESCAPE;
        case SDLK_SPACE:     return K_SPACE;
        case SDLK_PAUSE:     return K_PAUSE;
        case SDLK_PRINTSCREEN: return K_PRINT_SCREEN;

        default:
            // Dla standardowych znaków ASCII (a-z, 0-9 na górze, itp.)
            if (key->sym >= 0 && key->sym < 256) {
                return tolower(key->sym);
            }
            return 0;
    }
}

static void GetEvent(SDL_Event *event) {
    int key;
    switch (event->type) {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            key = XLateKey(&event->key.keysym);
            keyq[keyq_head].key = key;
            keyq[keyq_head].down = (event->type == SDL_KEYDOWN);
            keyq_head = (keyq_head + 1) & 63;
            break;

        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            key = K_MOUSE1 + (event->button.button - 1);
            Key_Event(key, (event->type == SDL_MOUSEBUTTONDOWN), Sys_Milliseconds());
            break;

        case SDL_MOUSEWHEEL:
            if (event->wheel.y > 0) { Key_Event(K_MWHEELUP, true, Sys_Milliseconds()); Key_Event(K_MWHEELUP, false, Sys_Milliseconds()); }
            else if (event->wheel.y < 0) { Key_Event(K_MWHEELDOWN, true, Sys_Milliseconds()); Key_Event(K_MWHEELDOWN, false, Sys_Milliseconds()); }
            break;

        case SDL_QUIT:
            Sys_Quit();
            break;
    }
}

void HandleEvents(void) {
    SDL_Event event;
    if (!window) return;

    while (SDL_PollEvent(&event)) GetEvent(&event);

    while (keyq_head != keyq_tail) {
        Key_Event(keyq[keyq_tail].key, keyq[keyq_tail].down, Sys_Milliseconds());
        keyq_tail = (keyq_tail + 1) & 63;
    }

    if (input_active && mouse_active) {
        SDL_GetRelativeMouseState(&mx, &my);
    }
}

// --- Grafika ---

void SWimp_Init(void) {
    if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
        if (SDL_Init(SDL_INIT_VIDEO) == -1) Sys_Error("SDL_Init failed: %s\n", SDL_GetError());
    }
    SDL_ShowCursor(0);
}

#ifdef GL_QUAKE
int GLimp_Init(void *hInstance, void *wndProc) {
    SWimp_Init();
    return 1;
}

void *GLimp_GetProcAddress(const char *func) {
    return SDL_GL_GetProcAddress(func);
}
#endif

// enhanced version with multi monitor settings 

static unsigned char *SDLimp_InitGraphics(viddef_t *vid) {
    Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
    int display_index;
    int num_displays;
    SDL_Rect display_bounds;
    int x, y, w, h;

    // 1. Hint dla X11: Wyłącz kompozycję i wymuś omijanie dekoracji menedżera okien
    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "1");

    display_index = (int)Cvar_Get("vid_displayindex", "0", CVAR_ARCHIVE)->value;
    num_displays = SDL_GetNumVideoDisplays();

    if (display_index < 0 || display_index >= num_displays) {
        display_index = 0;
    }

    if (SDL_GetDisplayBounds(display_index, &display_bounds) != 0) {
        Sys_Error("SDL_GetDisplayBounds failed: %s", SDL_GetError());
    }

    if (vid_fullscreen->value) {
        if (vid_fullscreen_exclusive->value) {
            flags |= SDL_WINDOW_FULLSCREEN;
            w = vid->width;
            h = vid->height;
        } else {
            // JAWNIE wymuś rozmiar monitora dla trybu borderless
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
            w = display_bounds.w;
            h = display_bounds.h;
        }
        x = display_bounds.x;
        y = display_bounds.y;
    } else {
        w = vid->width;
        h = vid->height;
        x = display_bounds.x + (display_bounds.w - w) / 2;
        y = display_bounds.y + (display_bounds.h - h) / 2;
    }

    if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    window = SDL_CreateWindow(
        "Quake II Classic",
        x, y, w, h,
        flags
    );

    if (!window) Sys_Error("SDL_CreateWindow failed: %s", SDL_GetError());

    // 2. Brutalne wymuszenie pozycji po utworzeniu (naprawia błędy Xfce/X11)
    SDL_SetWindowPosition(window, x, y);

    SDL_SetWindowMinimumSize(window, 640, 480);

    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) Sys_Error("SDL_GL_CreateContext failed: %s", SDL_GetError());

    int dw, dh;
    SDL_GL_GetDrawableSize(window, &dw, &dh);
    vid->width = dw;
    vid->height = dh;

    Com_Printf("Display %i Geometry: %ix%i at %i,%i (Drawable: %ix%i)\n", 
               display_index, w, h, x, y, dw, dh);

    return (unsigned char *)1;
}

#ifdef GL_QUAKE
rserr_t GLimp_SetMode(int *pwidth, int *pheight, int mode, qboolean fullscreen) {
    viddef_t vid;
    if (!R_GetModeInfo(pwidth, pheight, mode)) return rserr_invalid_mode;

    vid.width = *pwidth;
    vid.height = *pheight;

    if (!SDLimp_InitGraphics(&vid)) return rserr_invalid_mode;

    *pwidth = vid.width;
    *pheight = vid.height;

    GLimp_AppActivate(true);
    return rserr_ok;
}

void GLimp_BeginFrame(float camera_seperation) {}

void GLimp_EndFrame(void) {
    SDL_GL_SwapWindow(window);
}

void GLimp_AppActivate(qboolean active) {
    input_active = active;
    if (active) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        mouse_active = true;
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        mouse_active = false;
    }
}

void GLimp_Shutdown(void) {
    if (gl_context) { SDL_GL_DeleteContext(gl_context); gl_context = NULL; }
    if (window) { SDL_DestroyWindow(window); window = NULL; }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
#endif

// --- System ---

void Sys_MakeCodeWriteable(unsigned long startaddr, unsigned long length) {
    int psize = getpagesize();
    unsigned long addr = (startaddr & ~(psize - 1)) - psize;
    if (mprotect((char *)addr, length + startaddr - addr + psize, 7) < 0) Sys_Error("mprotect failed\n");
}

void IN_Activate(qboolean active) { GLimp_AppActivate(active); }
void KBD_Close(void) { memset(keyq, 0, sizeof(keyq)); keyq_head = keyq_tail = 0; }
char *Sys_GetClipboardData(void) { return (char *)SDL_GetClipboardText(); }
