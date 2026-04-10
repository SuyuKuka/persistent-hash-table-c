#include "bloc.h"

uint32_t bloc_libre(struct hache_entree *table, uint32_t b, uint32_t indice) {
    if (b == 0) {
        fprintf(stderr, "[ERREUR] bloc_libre: b ne peut pas etre égal a 0.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t i = indice;

    while (table[i].longueur != BLOC_LIBRE) {
        i = (i + 1) % b;
        if (i == indice) {
            return BLOC_ERREUR;
        }
    }

    return i;
}
