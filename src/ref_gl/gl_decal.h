#ifndef GL_DECAL_H
#define GL_DECAL_H

// =======================================================================
//        Publiczny Interfejs Modułu Renderowania Decali
// =======================================================================

// --- Dołączenia Zależności ---
// Potrzebujemy definicji typów wektorów.
#include "../game/q_shared.h"

// =======================================================================
//         Deklaracje Wstępne (Forward Declarations)
// =======================================================================
//
// M-AI-812: Kluczowa poprawka. Zamiast dołączać cały "prywatny" nagłówek
// renderera, używamy deklaracji wstępnej. Mówimy kompilatorowi, że typ
// 'image_t' istnieje, co pozwala nam na używanie wskaźników (image_t *),
// nie ujawniając wewnętrznej budowy tej struktury.
//
struct image_s;
typedef struct image_s image_t;

struct mnode_s;
typedef struct mnode_s mnode_t;

struct msurface_s;
typedef struct msurface_s msurface_t;

// =======================================================================
//                      Definicje i Typy Danych
// =======================================================================

// --- Stałe Konfiguracyjne Modułu ---
// M-AI-812: Przeniesione z gl_decal.c, ponieważ są używane przez
// publiczną strukturę cdecal_t.
#define MAX_DECALS				8192 //	2048 //256     // Maksymalna liczba aktywnych decali
#define MAX_DECAL_VERTS			64      // Maksymalna liczba wierzchołków na decal
#define MAX_DECAL_FRAGMENTS		64      // Maksymalna liczba fragmentów przy cięciu

// --- Typy Decali ---
#define DECAL_BHOLE         1   // Standardowy ślad po kuli.
#define DECAL_BLOOD         2   // Plama krwi.
#define DECAL_SCORCH        3   // Opalony ślad po wybuchu lub broni energetycznej.
#define DECAL_ENERGY        4   // Ślad po broni energetycznej.

// --- Flagi Zachowania Decali ---
#define DF_SHADE            0x00000400  // Cieniuj decal zgodnie z oświetleniem mapy.
#define DF_NO_SHADE         0x00004000  // Ignoruj cieniowanie.
#define DF_GLOW             0x00001000  // Spraw, by decal "świecił" (RF_FULLBRIGHT).
#define DF_FADEOUT          0x00002000  // Standardowe, liniowe zanikanie z czasem.
#define DF_PULSE            0x00008000  // Efekt pulsowania.
#define DF_FADE_COLOR       0x00010000  // NOWA FLAGA: Zanikanie koloru z A do B.

// --- Główna Struktura Danych dla Decala ---
typedef struct cdecal_s
{
	struct cdecal_s	*prev, *next;
	float		time;

	image_t		*image;         // Wskaźnik na teksturę.

	int			numverts;
	vec3_t		verts[MAX_DECAL_VERTS];
	vec2_t		stcoords[MAX_DECAL_VERTS];
	mnode_t		*node;

	vec3_t		direction;
	vec4_t		color;
	vec3_t		org;

	int			type;
	int			flags;

	// --- NOWE POLA ---
	vec4_t		end_color;
	float		fadetime;

} cdecal_t;

// for generating substitute image  conchars.pcx
image_t* GL_GenerateImageFromSVG(const char* svg_data, int width, int height, const char* name);

// =======================================================================
//                       Prototypy Funkcji Publicznych
// =======================================================================

void GL_InitDecals (void);
void GL_ShutDownDecals (void);
void GL_ClearDecals (void);
void R_AddDecals (void);

void GL_InitDecalImages (void);
void GL_FreeUnusedDecalImages (void);

void R_AddDecal (vec3_t origin, vec3_t dir,
				 float red, float green, float blue, float alpha,
				 float size, int type, int flags, float angle,
				 const vec4_t end_color, float fadetime);

#endif // GL_DECAL_H
