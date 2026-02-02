#ifndef G_MOVE_REG_H
#define G_MOVE_REG_H

void InitMoveRegistry(void);
void FreeMoveRegistry(void);
qboolean RegisterMMove(const char *name, mmove_t *move);
mmove_t *FindMMoveByName(const char *name);

#endif
