#include "client.h"
#include "cl_decals.h"
#include "../ref_gl/gl_decal.h" // Kluczowe dołączenie!

// Deklaracja 'extern', aby ten plik wiedział o istnieniu cvara
// Deklaracje 'extern', aby ten plik wiedział o istnieniu cvarów
// Deklaracje 'extern', aby ten plik wiedział o istnieniu cvarów
extern cvar_t *gl_decals;
static cvar_t *gl_decals_debug = NULL;

void CL_InitDecals(void)
{
    // Inicjalizujemy wskaźnik na cvara debugującego tylko raz
    gl_decals_debug = Cvar_Get("gl_decals_debug", "0", CVAR_ARCHIVE);
}

void CL_HandleTempEntityForDecal(int type, const vec3_t pos, const vec3_t dir)
{
    // Blok diagnostyczny
    if (gl_decals_debug && gl_decals_debug->integer > 0)
    {
        Com_Printf("CL_HandleTempEntityForDecal: Received TempEntity of type 0x%X.\n", type);
    }

    if (!gl_decals || !gl_decals->integer)
        return;

    switch (type)
    {
        // --- Przypadki dla broni "Hitscan" (ślad po kuli) ---
        case TE_GUNSHOT:
        case TE_SPARKS:
        //case TE_BULLET_SPARKS: // railgun bullethole missing - this temporairly is commented to find it
        case TE_SHOTGUN:
        {
        	trace_t tr;
        	vec3_t  start, end;

        	// Trace wsteczny: zacznij 1 jednostkę ZA punktem uderzenia...
        	VectorMA(pos, 1, dir, start);
        	// ...i strzel promień 16 jednostek WSTECZ.
        	VectorMA(start, -16, dir, end);
        	tr = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SOLID);

            if (tr.fraction < 1.0 && !(tr.surface && (tr.surface->flags & SURF_SKY)))
            {
                R_AddDecal(tr.endpos, tr.plane.normal, 1.0f, 1.0f, 1.0f, 1.0f,
                           1.5f + (frand() * 0.5f), DECAL_BHOLE, DF_SHADE | DF_FADEOUT, rand() % 360);
            }
            break;
        }

        // --- Przypadki dla broni energetycznej (ślad opalenia, świecący) ---
        case TE_BLASTER:
        case TE_BLUEHYPERBLASTER:
        case TE_RAILTRAIL:
        {
                    trace_t tr;
                    vec3_t  start, end;

                    // Używamy ostatecznej, pancernej wersji trace'a wstecznego
                    VectorMA(pos, 1, dir, start);
                    VectorMA(start, -16, dir, end);
                    tr = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SOLID);

                    if (tr.fraction < 1.0 && !(tr.surface && (tr.surface->flags & SURF_SKY)))
                    {
                        R_AddDecal(tr.endpos, tr.plane.normal, 1.0f, 1.0f, 1.0f, 1.0f,
                                   4.0f + (frand() * 1.0f), DECAL_ENERGY, DF_GLOW | DF_FADEOUT | DF_NO_SHADE, rand() % 360);
                    }
                    break;
                }
        // --- Przypadki dla wybuchów (duży, opalony ślad) ---
        case TE_EXPLOSION1:
        case TE_ROCKET_EXPLOSION:
        case TE_GRENADE_EXPLOSION:
        {
            trace_t tr;
            vec3_t  start, end, directions[6];
            int     i;

            VectorSet(directions[0], 0, 0, -1); // Najpierw szukaj w dół (najczęstszy przypadek)
            VectorSet(directions[1], 0, 0, 1);
            VectorSet(directions[2], 1, 0, 0); VectorSet(directions[3], -1, 0, 0);
            VectorSet(directions[4], 0, 1, 0); VectorSet(directions[5], 0, -1, 0);

            for (i = 0; i < 6; i++)
            {
                VectorCopy(pos, start);
                VectorMA(start, 32, directions[i], end);
                tr = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SOLID);

                if (tr.fraction < 1.0 && !(tr.surface && (tr.surface->flags & SURF_SKY)))
                {
                     R_AddDecal(tr.endpos, tr.plane.normal, 1.0f, 1.0f, 1.0f, 1.0f,
                               20.0f + (frand() * 5.0f), DECAL_SCORCH, DF_SHADE | DF_FADEOUT, rand() % 360);
                     break; // <- KLUCZOWA ZMIANA: przerywamy pętlę po znalezieniu pierwszej powierzchni.
                }
            }
            break;
        }

    }
}
