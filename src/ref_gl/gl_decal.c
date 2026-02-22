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
//Decals, from egl, orginally from qfusion?

#include "gl_local.h"
#include "gl_decal.h"
#include "gl_decal_svg.h"


#include <GL/glu.h> // Potrzebne dla gluBuild2DMipmaps

static cvar_t	*gl_decals_debug;

// =======================================================================
// SVG DECAL IMPLEMENTATION
// =======================================================================

// Definiujemy implementację NanoSVG w tym jednym pliku.
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

// Nowa funkcja do rasteryzacji SVG i stworzenia tekstury OpenGL.
static GLuint GL_RasterizeSVGToTexture(const char* svg_data, int width, int height)
{
    struct NSVGimage* image = NULL;
    struct NSVGrasterizer* rast = NULL;
    unsigned char* img_buffer = NULL;
    GLuint tex_id = 0;

    Com_Printf("Rasterizing SVG decal...\n");

    // Parsuj dane SVG
    // Uwaga: nanosvg modyfikuje wejściowy string, więc tworzymy jego kopię.
    char* svg_copy = Z_TagMalloc(strlen(svg_data) + 1, TAG_RENDER_IMAGE);
    strcpy(svg_copy, svg_data);
    image = nsvgParse(svg_copy, "px", 96.0f);
    Z_Free(svg_copy);

    if (!image) {
        Com_Printf("Could not parse SVG image.\n");
        return 0;
    }

    // Stwórz rasteryzator
    rast = nsvgCreateRasterizer();
    if (!rast) {
        Com_Printf("Could not create SVG rasterizer.\n");
        nsvgDelete(image);
        return 0;
    }

    // Zaalokuj bufor na piksele (RGBA)
    img_buffer = Z_TagMalloc(width * height * 4, TAG_RENDER_IMAGE);
    if (!img_buffer) {
        Com_Printf("Could not allocate image buffer for SVG.\n");
        nsvgDeleteRasterizer(rast);
        nsvgDelete(image);
        return 0;
    }

    // Rasteryzuj obraz
    nsvgRasterize(rast, image, 0, 0, 1.0f, img_buffer, width, height, width * 4);

    // Stwórz teksturę OpenGL
    qglGenTextures(1, &tex_id);
    qglBindTexture(GL_TEXTURE_2D, tex_id);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Użyj gluBuild2DMipmaps do automatycznego wygenerowania mipmap
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA, width, height, GL_RGBA, GL_UNSIGNED_BYTE, img_buffer);

    qglBindTexture(GL_TEXTURE_2D, 0);

    // Posprzątaj
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);
    Z_Free(img_buffer);

    if (tex_id == 0) {
        Com_Printf("Failed to create OpenGL texture from SVG.\n");
    } else {
        Com_Printf("SVG decal texture created successfully (ID: %u).\n", tex_id);
    }

    return tex_id;
}
// ===

#define INSTANT_DECAL	-10000.0


typedef struct
{
	int			firstvert;
	int			numverts;
	mnode_t		*node;
	msurface_t	*surf;
} fragment_t;

static int maxDecals;
static cdecal_t	*decals;
static cdecal_t	active_decals, *free_decals;

static int R_GetClippedFragments (vec3_t origin, float radius, mat3_t axis, int maxfverts, vec3_t *fverts, int maxfragments, fragment_t *fragments);
//void R_DrawDecal (int numverts, vec3_t *verts, vec2_t *stcoords, vec4_t color, int type, int flags);

cvar_t	*gl_decals;
static cvar_t	*gl_decals_time;
static cvar_t	*gl_decals_max;

static qboolean loadedDecalImages;

// Przechowujemy wskaźniki na wszystkie załadowane tekstury decali.
image_t		*r_decal_bhole;     // Ślad po kuli
image_t		*r_decal_scorch;    // Opalenizna po wybuchu
image_t		*r_decal_blood;     // Krew
image_t		*r_decal_energy;    // Ślad po broni energetycznej

//Architekt (M-AI-812)

static image_t* R_CreateFakeImage(const char* name, int width, int height, GLuint tex_id)
{
    int i;
    image_t *img = NULL;
    for (i = MAX_GLTEXTURES - 1; i >= 0; --i)
    {
        if (gltextures[i].registration_sequence == 0)
        {
            img = &gltextures[i];
            strcpy(img->name, name);
            img->width = width;
            img->height = height;
            img->upload_width = width;
            img->upload_height = height;
            img->texnum = tex_id;
            img->registration_sequence = registration_sequence;
            img->type = it_sprite;
            return img;
        }
    }
    Com_Printf("WARNING: No free texture slots for procedural image '%s'.\n", name);
    return NULL;
}
void GL_InitDecalImages (void)
{
    GLuint  tex_id;
    image_t *img;

    Com_Printf("Initializing decal textures...\n");
    loadedDecalImages = true;

    // #1: Tekstura Śladu po Kuli
    r_decal_bhole = GL_FindImage("pics/bullethole.png", it_sprite);
    if (!r_decal_bhole || r_decal_bhole == r_notexture) {
        Com_Printf(" - INFO: 'bullethole.png' not found. Falling back to procedural SVG.\n");
        tex_id = GL_RasterizeSVGToTexture(g_decal_svg_bhole, 64, 64);
        img = (tex_id != 0) ? R_CreateFakeImage("procedural/bhole_svg", 64, 64, tex_id) : NULL;
        r_decal_bhole = img ? img : r_notexture;
    } else {
        Com_Printf(" - Loaded 'pics/bullethole.png'.\n");
    }

    // #2: Tekstura Opalenizny po Wybuchu
    r_decal_scorch = GL_FindImage("pics/scorch.png", it_sprite);
    if (!r_decal_scorch || r_decal_scorch == r_notexture) {
        Com_Printf(" - INFO: 'scorch.png' not found. Falling back to procedural SVG.\n");
        tex_id = GL_RasterizeSVGToTexture(g_decal_svg_scorch, 128, 128);
        img = (tex_id != 0) ? R_CreateFakeImage("procedural/scorch_svg", 128, 128, tex_id) : NULL;
        r_decal_scorch = img ? img : r_notexture;
    } else {
        Com_Printf(" - Loaded 'pics/scorch.png'.\n");
    }

    // #3: Tekstura Broni Energetycznej
    r_decal_energy = GL_FindImage("pics/energy_mark.png", it_sprite);
    if (!r_decal_energy || r_decal_energy == r_notexture) {
        Com_Printf(" - INFO: 'energy_mark.png' not found. Falling back to procedural SVG.\n");
        tex_id = GL_RasterizeSVGToTexture(g_decal_svg_blaster, 64, 64);
        img = (tex_id != 0) ? R_CreateFakeImage("procedural/energy_svg", 64, 64, tex_id) : NULL;
        r_decal_energy = img ? img : r_notexture;
    } else {
        Com_Printf(" - Loaded 'pics/energy_mark.png'.\n");
    }
    // =======================================================================
     // #4: Placeholder dla tekstury Krwi
     // =======================================================================
     // r_decal_blood = GL_FindImage("pics/blood.png", it_sprite);
     // if (!r_decal_blood || r_decal_blood == r_notexture) {
     //     // Tutaj można dodać fallback do SVG dla krwi w przyszłości
     // } else {
     //     Com_Printf(" - Loaded external decal from 'pics/blood.png'.\n");
     // }
}



void GL_FreeUnusedDecalImages(void)
{
	if (!gl_decals->integer) {
		loadedDecalImages = false;
		return;
	}

	if(!loadedDecalImages) {
		GL_InitDecalImages();
		return;
	}

	r_decal_bhole->registration_sequence = registration_sequence;
}

static void GL_FreeDecals(void)
{
	if (decals)
		Z_Free(decals);

	maxDecals = 0;
	decals = NULL;
}

static void GL_AllocDecals(void)
{
	maxDecals = gl_decals_max->integer;
	decals = Z_TagMalloc(maxDecals * sizeof(cdecal_t), TAG_RENDER_SCRSHOT);
	GL_ClearDecals();
}

static void OnChange_Decals(cvar_t *self, const char *oldValue)
{
	if (!self->integer)
		return;

	if (!decals) {
		GL_AllocDecals();
	}
	if(!loadedDecalImages)
		GL_InitDecalImages();

}

static void OnChange_DecalsMax(cvar_t *self, const char *oldValue)
{
	if (self->integer < 256)
		Cvar_Set(self->name, "256");
	else if (self->integer > MAX_DECALS)
		Cvar_Set(self->name, "8192");

	if (maxDecals == self->integer)
		return;

	GL_FreeDecals();
	OnChange_Decals(gl_decals, gl_decals->resetString);
}

void GL_InitDecals (void)
{
	gl_decals = Cvar_Get( "gl_decals", "1", CVAR_ARCHIVE );
	gl_decals_time = Cvar_Get( "gl_decals_time", "30", CVAR_ARCHIVE );
	gl_decals_max = Cvar_Get( "gl_decals_max", "256", CVAR_ARCHIVE );
	gl_decals_debug = Cvar_Get( "gl_decals_debug", "0", CVAR_ARCHIVE );

	gl_decals->OnChange = OnChange_Decals;
	gl_decals_max->OnChange = OnChange_DecalsMax;

	loadedDecalImages = false;

	OnChange_DecalsMax(gl_decals_max, gl_decals_max->resetString);
}

void GL_ShutDownDecals (void)
{

	if(!loadedDecalImages)
		return;

	GL_FreeDecals();

	gl_decals->OnChange = NULL;
	gl_decals_max->OnChange = NULL;
}

/*
=================
CG_ClearDecals
=================
*/
void GL_ClearDecals (void)
{
	int i;

	if (!decals)
		return;

	memset ( decals, 0, maxDecals * sizeof(cdecal_t) );

	// link decals
	free_decals = decals;
	active_decals.prev = &active_decals;
	active_decals.next = &active_decals;
	for ( i = 0; i < maxDecals - 1; i++ )
		decals[i].next = &decals[i+1];
}

/*
=================
CG_AllocDecal

Returns either a free decal or the oldest one
=================
*/
static cdecal_t *GL_AllocDecal (void)
{
	cdecal_t *dl;

	if ( free_decals ) {	// take a free decal if possible
		dl = free_decals;
		free_decals = dl->next;
	} else {				// grab the oldest one otherwise
		dl = active_decals.prev;
		dl->prev->next = dl->next;
		dl->next->prev = dl->prev;
	}

	// put the decal at the start of the list
	dl->prev = &active_decals;
	dl->next = active_decals.next;
	dl->next->prev = dl;
	dl->prev->next = dl;

	return dl;
}

/*
=================
CG_FreeDecal
=================
*/
static void GL_FreeDecal ( cdecal_t *dl )
{
	if (!dl->prev)
		return;

	// remove from linked active list
	dl->prev->next = dl->next;
	dl->next->prev = dl->prev;

	// insert into linked free list
	dl->next = free_decals;
	free_decals = dl;
}
void R_AddDecal	(vec3_t origin, vec3_t dir, float red, float green, float blue, float alpha,
				 float size, int type, int flags, float angle,
				 const vec4_t end_color, float fadetime)
{
	int			i, j, numfragments;
	vec3_t		verts[MAX_DECAL_VERTS], shade;
	fragment_t	*fr, fragments[MAX_DECAL_FRAGMENTS];
	mat3_t		axis;
	cdecal_t	*d;

	// ===================================================================
	// KROK 1: Blok diagnostyczny (tylko do logowania)
	// ===================================================================
	if (gl_decals_debug && gl_decals_debug->integer > 0)
	{
		char *typeName = "UNKNOWN";
		switch(type)
		{
			case DECAL_BHOLE:  typeName = "DECAL_BHOLE"; break;
			case DECAL_SCORCH: typeName = "DECAL_SCORCH"; break;
			case DECAL_ENERGY: typeName = "DECAL_ENERGY"; break;
			case DECAL_BLOOD:  typeName = "DECAL_BLOOD"; break;
		}
		Com_Printf("R_AddDecal: Received request to create '%s' decal.\n", typeName);

		if (gl_decals_debug->integer > 1)
		{
			Com_Printf("  at: (%.1f, %.1f, %.1f), size: %.1f, flags: 0x%X\n",
				origin[0], origin[1], origin[2], size, flags);
		}
	}

	if (!gl_decals->integer)
		return;

	// invalid decal
	if (size <= 0 || VectorCompare (dir, vec3_origin))
		return;

	// calculate orientation matrix
	VectorNormalize2 (dir, axis[0]);
	PerpendicularVector (axis[1], axis[0]);
	RotatePointAroundVector (axis[2], axis[0], axis[1], angle);
	CrossProduct (axis[0], axis[2], axis[1]);

	numfragments = R_GetClippedFragments (origin, size, axis,
		MAX_DECAL_VERTS, verts, MAX_DECAL_FRAGMENTS, fragments);

	if (!numfragments)
		return;

	size = 0.5f / size;
	VectorScale (axis[1], size, axis[1]);
	VectorScale (axis[2], size, axis[2]);

	for (i=0, fr=fragments ; i<numfragments ; i++, fr++)
	{
		if (fr->numverts > MAX_DECAL_VERTS)
			fr->numverts = MAX_DECAL_VERTS;
		else if (fr->numverts < 1)
			continue;

		// ===================================================================
		// KROK 2: Alokacja i przypisanie tekstury (poprawna kolejność)
		// ===================================================================
		d = GL_AllocDecal ();
		d->time = r_newrefdef.time;

		switch(type)
		{
			case DECAL_SCORCH:
				d->image = r_decal_scorch;
				break;
			case DECAL_ENERGY:
				d->image = r_decal_energy;
				break;
			case DECAL_BLOOD:
				d->image = r_notexture; // Placeholder
				break;
			case DECAL_BHOLE:
			default:
				d->image = r_decal_bhole;
				break;
		}

		if (!d->image || d->image == r_notexture)
		{
			GL_FreeDecal(d);
			continue;
		}

		// ===================================================================
		// KROK 3: Reszta logiki funkcji (bez zmian)
		// ===================================================================
		d->numverts = fr->numverts;
		d->node = fr->node;

		VectorCopy(fr->surf->plane->normal, d->direction);
		if (!(fr->surf->flags & SURF_PLANEBACK))
			VectorNegate(d->direction, d->direction);

			Vector4Set(d->color, red, green, blue, alpha);
		VectorCopy (origin, d->org);

		if (flags & DF_FADE_COLOR)
		{
			if (end_color)
				Vector4Copy(end_color, d->end_color);
			d->fadetime = fadetime;
		}

		if (flags & DF_SHADE) {
			R_LightPoint (origin, shade);
			for (j=0 ; j<3 ; j++)
				d->color[j] = (d->color[j] * shade[j] * 0.6f) + (d->color[j] * 0.4f);
		}

		d->type = type;
		d->flags = flags;

		for (j = 0; j < fr->numverts; j++) {
			vec3_t v_temp; // Zmieniono nazwę, aby uniknąć konfliktu z logiem
			VectorCopy (verts[fr->firstvert+j], d->verts[j]);
			VectorSubtract (d->verts[j], origin, v_temp);
			d->stcoords[j][0] = DotProduct (v_temp, axis[1]) + 0.5f;
			d->stcoords[j][1] = DotProduct (v_temp, axis[2]) + 0.5f;
		}
	}
}

void R_AddDecals (void)
{
	cdecal_t	*dl, *next,	*active;
	float		mindist, time;
	vec3_t		v;
	vec4_t		color;
    GLuint      last_texnum = 0;

	if (!gl_decals->integer)
		return;

	active = &active_decals;
	if (active->next == active)
		return;

	mindist = DotProduct(r_origin, viewAxis[0]) + 4.0f;

	qglEnable(GL_POLYGON_OFFSET_FILL);
	qglPolygonOffset(-1, -2);

	qglDepthMask(GL_FALSE);
	qglEnable(GL_BLEND);
	GL_TexEnv(GL_MODULATE);

	for (dl = active->next; dl != active; dl = next)
	{
		next = dl->next;

		// Sprawdź czas życia decala
		if (dl->time + gl_decals_time->value <= r_newrefdef.time ) {
			GL_FreeDecal ( dl );
			continue;
		}

		// Sprawdź, czy węzeł, w którym jest decal, jest widoczny
		if (dl->node == NULL || dl->node->visframe != r_visframecount)
			continue;

        // ===================================================================
        // PRZYWRÓCONY KOD: Sprawdza, czy decal nie jest za plecami gracza
        // lub odwrócony tyłem. To naprawia ostrzeżenia kompilatora.
        // ===================================================================
		// Nie renderuj, jeśli decal jest za płaszczyzną widoku
		if ( DotProduct(dl->org, viewAxis[0]) < mindist)
			continue;

		// Nie renderuj, jeśli patrzymy na "tył" decala
		VectorSubtract(dl->org, r_origin, v);
		if (DotProduct(dl->direction, v) < 0)
			continue;
        // ===================================================================

		// Binduj teksturę tylko wtedy, gdy jest inna niż poprzednia.
        if (dl->image && dl->image->texnum != last_texnum)
        {
            GL_Bind(dl->image->texnum);
            last_texnum = dl->image->texnum;
        }

				Vector4Copy (dl->color, color);

		// Interpolacja koloru, jeśli flaga jest ustawiona
		if (dl->flags & DF_FADE_COLOR)
		{
			float frac = (r_newrefdef.time - dl->time) / dl->fadetime;
			if (frac < 0.0f) frac = 0.0f;
			if (frac > 1.0f) frac = 1.0f;

			for (int i = 0; i < 4; i++)
			{
				color[i] = dl->color[i] + frac * (dl->end_color[i] - dl->color[i]);
			}
		}

		// Logika zanikania (fade out)
		time = dl->time + gl_decals_time->value - r_newrefdef.time;
		if ( (dl->flags & DF_FADEOUT) && (time < 1.5f) )
			color[3] *= time / 1.5f;

		// Rysowanie
		qglColor4fv (color);
		qglTexCoordPointer( 2, GL_FLOAT, 0, dl->stcoords);
		qglVertexPointer( 3, GL_FLOAT, 0, dl->verts );
		qglDrawArrays( GL_TRIANGLE_FAN, 0, dl->numverts );
	}

	GL_TexEnv(GL_REPLACE);
	qglDisable(GL_BLEND);
	qglColor4fv(colorWhite);
	qglDepthMask(GL_TRUE);
	qglDisable(GL_POLYGON_OFFSET_FILL);
	qglVertexPointer( 3, GL_FLOAT, 0, r_arrays.vertices );
}

#define	ON_EPSILON			0.1			// point on plane side epsilon
#define BACKFACE_EPSILON	0.01

static int numFragmentVerts;
static int maxFragmentVerts;
static vec3_t *fragmentVerts;

static int numClippedFragments;
static int maxClippedFragments;
static fragment_t *clippedFragments;

static int		fragmentFrame;
static cplane_t fragmentPlanes[6];

/*
=================
R_ClipPoly
=================
*/

static void R_ClipPoly (int nump, vec4_t vecs, int stage, fragment_t *fr)
{
	cplane_t *plane;
	qboolean	front, back;
	vec4_t	newv[MAX_DECAL_VERTS];
	float	*v, d, dists[MAX_DECAL_VERTS];
	int		newc, i, j, sides[MAX_DECAL_VERTS];

	if (nump > MAX_DECAL_VERTS-2)
		Com_Printf ("R_ClipPoly: MAX_DECAL_VERTS");

	if (stage == 6)
	{	// fully clipped
		if (nump > 2)
		{
			fr->numverts = nump;
			fr->firstvert = numFragmentVerts;

			if (numFragmentVerts+nump >= maxFragmentVerts)
				nump = maxFragmentVerts - numFragmentVerts;

			for (i=0, v=vecs ; i<nump ; i++, v+=4)
				VectorCopy (v, fragmentVerts[numFragmentVerts+i]);

			numFragmentVerts += nump;
		}

		return;
	}

	front = back = false;
	plane = &fragmentPlanes[stage];
	for (i=0, v=vecs ; i<nump ; i++ , v+= 4)
	{
		d = PlaneDiff (v, plane);
		if (d > ON_EPSILON)
		{
			front = true;
			sides[i] = SIDE_FRONT;
		}
		else if (d < -ON_EPSILON)
		{
			back = true;
			sides[i] = SIDE_BACK;
		}
		else
		{
			sides[i] = SIDE_ON;
		}

		dists[i] = d;
	}

	if (!front)
		return;

	// clip it
	sides[i] = sides[0];
	dists[i] = dists[0];
	VectorCopy (vecs, (vecs+(i*4)) );
	newc = 0;

	for (i=0, v=vecs ; i<nump ; i++, v+=4)
	{
		switch (sides[i])
		{
		case SIDE_FRONT:
			VectorCopy (v, newv[newc]);
			newc++;
			break;
		case SIDE_BACK:
			break;
		case SIDE_ON:
			VectorCopy (v, newv[newc]);
			newc++;
			break;
		}

		if (sides[i] == SIDE_ON || sides[i+1] == SIDE_ON || sides[i+1] == sides[i])
			continue;

		d = dists[i] / (dists[i] - dists[i+1]);
		for (j=0 ; j<3 ; j++)
			newv[newc][j] = v[j] + d * (v[j+4] - v[j]);
		newc++;
	}

	// continue
	R_ClipPoly (newc, newv[0], stage+1, fr);
}

/*
=================
R_PlanarSurfClipFragment
=================
*/

static void R_PlanarSurfClipFragment (mnode_t *node, msurface_t *surf, vec3_t normal)
{
	int			i;
	float		*v, *v2, *v3;
	fragment_t	*fr;
	vec4_t		verts[MAX_DECAL_VERTS];

	// bogus face
	if (surf->numedges < 3)
		return;

	// greater than 60 degrees
	if (surf->flags & SURF_PLANEBACK)
	{
		if (-DotProduct (normal, surf->plane->normal) < 0.5)
			return;
	}
	else
	{
		if (DotProduct (normal, surf->plane->normal) < 0.5)
			return;
	}

	v = surf->polys->verts[0];
	// copy vertex data and clip to each triangle
	for (i=0; i<surf->polys->numverts-2 ; i++)
	{
		fr = &clippedFragments[numClippedFragments];
		fr->numverts = 0;
		fr->node = node;
		fr->surf = surf;

		v2 = surf->polys->verts[0] + (i+1) * VERTEXSIZE;
		v3 = surf->polys->verts[0] + (i+2) * VERTEXSIZE;

		VectorCopy (v , verts[0]);
		VectorCopy (v2, verts[1]);
		VectorCopy (v3, verts[2]);
		R_ClipPoly (3, verts[0], 0, fr);

		if (fr->numverts)
		{
			numClippedFragments++;

			if ((numFragmentVerts >= maxFragmentVerts) || (numClippedFragments >= maxClippedFragments))
			{
				return;
			}
		}
	}
}

/*
=================
R_RecursiveFragmentNode
=================
*/

static void R_RecursiveFragmentNode (mnode_t *node, vec3_t origin, float radius, vec3_t normal)
{
	float dist;
	cplane_t *plane;

mark0:
	if ((numFragmentVerts >= maxFragmentVerts) || (numClippedFragments >= maxClippedFragments))
		return;			// already reached the limit somewhere else

	if (node->contents == CONTENTS_SOLID)
		return;

	if (node->contents != CONTENTS_NODE)
	{
		// leaf
		int c;
		mleaf_t *leaf;
		msurface_t *surf, **mark;

		leaf = (mleaf_t *)node;
		if (!(c = leaf->nummarksurfaces))
			return;

		mark = leaf->firstmarksurface;
		do
		{
			if ((numFragmentVerts >= maxFragmentVerts) || (numClippedFragments >= maxClippedFragments))
				return;

			surf = *mark++;
			if (surf->fragmentframe == fragmentFrame)
				continue;

			surf->fragmentframe = fragmentFrame;
			if (!(surf->texinfo->flags & (SURF_SKY|SURF_WARP|SURF_TRANS33|SURF_TRANS66|SURF_FLOWING|SURF_NODRAW)))
			{
				R_PlanarSurfClipFragment (node, surf, normal);
			}
		} while (--c);

		return;
	}

	plane = node->plane;
	dist = PlaneDiff (origin, plane);

	if (dist > radius)
	{
		node = node->children[0];
		goto mark0;
	}
	if (dist < -radius)
	{
		node = node->children[1];
		goto mark0;
	}

	R_RecursiveFragmentNode (node->children[0], origin, radius, normal);
	R_RecursiveFragmentNode (node->children[1], origin, radius, normal);
}

/*
=================
R_GetClippedFragments
=================
*/

static int R_GetClippedFragments (vec3_t origin, float radius, mat3_t axis, int maxfverts, vec3_t *fverts, int maxfragments, fragment_t *fragments)
{
	int i;
	float d;

	fragmentFrame++;

	// initialize fragments
	numFragmentVerts = 0;
	maxFragmentVerts = maxfverts;
	fragmentVerts = fverts;

	numClippedFragments = 0;
	maxClippedFragments = maxfragments;
	clippedFragments = fragments;

	// calculate clipping planes
	for (i=0 ; i<3; i++)
	{
		d = DotProduct (origin, axis[i]);

		VectorCopy (axis[i], fragmentPlanes[i*2].normal);
		fragmentPlanes[i*2].dist = d - radius;
		fragmentPlanes[i*2].type = PlaneTypeForNormal (fragmentPlanes[i*2].normal);

		VectorNegate (axis[i], fragmentPlanes[i*2+1].normal);
		fragmentPlanes[i*2+1].dist = -d - radius;
		fragmentPlanes[i*2+1].type = PlaneTypeForNormal (fragmentPlanes[i*2+1].normal);
	}

	R_RecursiveFragmentNode (r_worldmodel->nodes, origin, radius, axis[0]);

	return numClippedFragments;
}

// Ta funkcja jest niemal identyczna jak GL_RasterizeSVGToTexture,
// ale zwraca image_t* i ma inną nazwę dla przejrzystości.
image_t* GL_GenerateImageFromSVG(const char* svg_data, int width, int height, const char* name)
{
    GLuint tex_id = GL_RasterizeSVGToTexture(svg_data, width, height);
    if (tex_id == 0)
    {
        return NULL;
    }
    return R_CreateFakeImage(name, width, height, tex_id);
}