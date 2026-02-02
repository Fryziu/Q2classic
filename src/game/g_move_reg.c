#include "g_local.h"
#include "g_move_reg.h"
#include "../include/uthash/uthash.h" // Dołączamy uthash

// Struktura, która będzie przechowywana w tablicy haszującej
typedef struct {
    const char *name; // Klucz (nazwa animacji)
    mmove_t *move;
    UT_hash_handle hh; // To sprawia, że struktura jest "hashowalna"
} mmove_registry_t;

static mmove_registry_t *move_registry = NULL;

void InitMoveRegistry(void)
{
    // Jeśli rejestr już istnieje, wyczyść go na wszelki wypadek
    if (move_registry) {
        FreeMoveRegistry();
    }
    move_registry = NULL;
}

void FreeMoveRegistry(void)
{
    // CARMACK'S ETHOS: Don't micromanage zone memory.
    // The engine wipes TAG_GAME/TAG_LEVEL automatically on shutdown.
    // Trying to unlink/free individual items causes double-free crashes.
    
    // Just reset the head pointer for the next game instance.
    move_registry = NULL; 
    
    gi.dprintf("FreeMoveRegistry: Pointer reset (Memory handled by Engine Tags)\n");
}

qboolean RegisterMMove(const char *name, mmove_t *move)
{
    mmove_registry_t *entry;

    HASH_FIND_STR(move_registry, name, entry);
    if (entry) {
        // Już zarejestrowany, być może przez inny mod - to nie błąd
        return true;
    }

    entry = gi.TagMalloc(sizeof(mmove_registry_t), TAG_LEVEL);
    if (!entry) {
        gi.error("RegisterMMove: Nie udało się zaalokować pamięci");
        return false;
    }
    entry->name = name;
    entry->move = move;
    HASH_ADD_KEYPTR(hh, move_registry, entry->name, strlen(entry->name), entry);
    return true;
}

mmove_t *FindMMoveByName(const char *name)
{
    mmove_registry_t *entry;

    if (!name || !*name) {
        return NULL;
    }

    HASH_FIND_STR(move_registry, name, entry);
    if (entry) {
        return entry->move;
    }

    gi.dprintf("OSTRZEŻENIE: Nie znaleziono animacji o nazwie '%s'\n", name);
    return NULL;
}
