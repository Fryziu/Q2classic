#include "client.h"
#include "cl_decals.h"
#include "../ref_gl/gl_decal.h"

extern cvar_t *gl_decals;
static cvar_t *gl_decals_debug = NULL;

void CL_InitDecals(void)
{
    gl_decals_debug = Cvar_Get("gl_decals_debug", "0", CVAR_ARCHIVE);
}

static const char* GetTempEntityName(int type)
{
    switch(type){
        case TE_GUNSHOT: return "TE_GUNSHOT";
        case TE_SPARKS: return "TE_SPARKS";
        case TE_BULLET_SPARKS: return "TE_BULLET_SPARKS (Railgun Impact)";
        case TE_SHOTGUN: return "TE_SHOTGUN";
        case TE_BLASTER: return "TE_BLASTER";
        case TE_BLUEHYPERBLASTER: return "TE_BLUEHYPERBLASTER";
        case TE_RAILTRAIL: return "TE_RAILTRAIL (Visual)";
        case TE_EXPLOSION1: return "TE_EXPLOSION1";
        case TE_ROCKET_EXPLOSION: return "TE_ROCKET_EXPLOSION";
        case TE_GRENADE_EXPLOSION: return "TE_GRENADE_EXPLOSION";
        case TE_BFG_EXPLOSION: return "TE_BFG_EXPLOSION";
        case TE_BFG_BIGEXPLOSION: return "TE_BFG_BIGEXPLOSION";
        default: return "UNKNOWN";
    }
}

void CL_HandleTempEntityForDecal(int type, const vec3_t pos, const vec3_t dir)
{
    if (gl_decals_debug && gl_decals_debug->integer > 0)
        Com_Printf("CL_HandleTempEntityForDecal: Received event '%s'.\n", GetTempEntityName(type));

    if (!gl_decals || !gl_decals->integer)
        return;

    switch (type)
    {
        // GRUPA 1: Standardowe trafienia
        case TE_GUNSHOT:
        case TE_SPARKS:
        case TE_BULLET_SPARKS:
        case TE_SHOTGUN:
        case TE_BLASTER:
        case TE_BLUEHYPERBLASTER:
        {
        	trace_t tr;
        	vec3_t  start, end;
            int decal_type, decal_flags;
            float decal_size;

        	VectorMA(pos, 1, dir, start);
        	VectorMA(start, -16, dir, end);
        	tr = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SOLID);

            if (gl_decals_debug && gl_decals_debug->integer > 1)
                Com_Printf("[DECAL_DEBUG] Event: %s, Trace: Backwards, Result: %s (frac=%.2f)\n",
                    GetTempEntityName(type), (tr.fraction<1.0)?"SUCCEEDED":"FAILED", tr.fraction);

                     if (tr.fraction < 1.0 && !(tr.surface && (tr.surface->flags & SURF_SKY)))
            {
                if (type == TE_BLASTER || type == TE_BLUEHYPERBLASTER) {
                    decal_type = DECAL_ENERGY;
                    decal_size = 4.0f + (frand() * 1.0f);
                    decal_flags = DF_GLOW | DF_FADEOUT | DF_NO_SHADE | DF_FADE_COLOR;
					vec4_t end_color = {0.2, 0.2, 0.2, 1.0};   // Ciemnoszary
                    R_AddDecal(tr.endpos, tr.plane.normal, 1.0f, 1.0f, 0.0f, 1.0f,
                           decal_size, decal_type, decal_flags, rand() % 360,
                           end_color, 3.0f); // 3 sekundy zanikania
                } else {
                    decal_type = DECAL_BHOLE;
                    decal_size = 1.5f + (frand() * 0.5f);
                    decal_flags = DF_SHADE | DF_FADEOUT;
					R_AddDecal(tr.endpos, tr.plane.normal, 1.0f, 1.0f, 1.0f, 1.0f,
                           decal_size, decal_type, decal_flags, rand() % 360, NULL, 0);
                }
            }
            break;
        }

        // GRUPA 2: Railgun Visual
        case TE_RAILTRAIL:
        {
            trace_t tr;
            vec3_t start, end, rail_color; // ZMIANA: Dodano wektor na kolor

            VectorMA(pos, -8, dir, start);
            VectorMA(start, 16, dir, end);
            tr = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SOLID);

            if (gl_decals_debug && gl_decals_debug->integer > 1)
                Com_Printf("[DECAL_DEBUG] Event: %s, Trace: Through, Result: %s (frac=%.2f)\n",
                    GetTempEntityName(type), (tr.fraction<1.0)?"SUCCEEDED":"FAILED", tr.fraction);

            if (tr.fraction < 1.0 && !(tr.surface && (tr.surface->flags & SURF_SKY)))
            {
                // ZMIANA: Odczytaj cvary z kolorem
                rail_color[0] = Cvar_VariableValue("cl_railcolor_r");
                rail_color[1] = Cvar_VariableValue("cl_railcolor_g");
                rail_color[2] = Cvar_VariableValue("cl_railcolor_b");

                               // ZMIANA: Użyj odczytanego koloru zamiast białego
                R_AddDecal(tr.endpos, tr.plane.normal, rail_color[0], rail_color[1], rail_color[2], 1.0f,
                           4.0f + (frand() * 1.0f), DECAL_ENERGY, DF_GLOW | DF_FADEOUT | DF_NO_SHADE, rand() % 360,
                           NULL, 0);
            }
            break;
        }

        // GRUPA 3: Wybuchy
        case TE_EXPLOSION1:
        case TE_ROCKET_EXPLOSION:
        case TE_GRENADE_EXPLOSION:
        case TE_BFG_EXPLOSION:
        case TE_BFG_BIGEXPLOSION:
        {
            trace_t tr;
            vec3_t  start, end, directions[6];
            int i;
            VectorSet(directions[0], 0, 0, -1); VectorSet(directions[1], 0, 0, 1);
            VectorSet(directions[2], 1, 0, 0); VectorSet(directions[3], -1, 0, 0);
            VectorSet(directions[4], 0, 1, 0); VectorSet(directions[5], 0, -1, 0);

            for (i = 0; i < 6; i++)
            {
                VectorCopy(pos, start);
                VectorMA(start, 64, directions[i], end);
                tr = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SOLID);

                if (gl_decals_debug && gl_decals_debug->integer > 1)
                    Com_Printf("[DECAL_DEBUG] Event: %s (Probe %d), Result: %s (frac=%.2f)\n",
                        GetTempEntityName(type), i, (tr.fraction<1.0)?"OK":"FAIL", tr.fraction);

                               if (tr.fraction < 1.0 && !(tr.surface && (tr.surface->flags & SURF_SKY)))
                {
                     R_AddDecal(tr.endpos, tr.plane.normal, 1.0f, 1.0f, 1.0f, 1.0f,
                               20.0f + (frand() * 5.0f), DECAL_SCORCH, DF_SHADE | DF_FADEOUT, rand() % 360,
                               NULL, 0);
                     break;
                }
            }
            break;
        }
    }
}
