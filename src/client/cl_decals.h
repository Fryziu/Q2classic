#ifndef CL_DECALS_H
#define CL_DECALS_H

// Dołączenie jest potrzebne, ponieważ prototypy używają typu 'vec3_t'
#include "../game/q_shared.h"

// Publiczny interfejs dla logiki decali po stronie klienta.
// Wywoływany przez parser zdarzeń sieciowych (cl_tent.c).

void CL_InitDecals(void);
void CL_HandleTempEntityForDecal(int type, const vec3_t pos, const vec3_t dir);

#endif // CL_DECALS_H
