#ifndef BLOC_H
#define BLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define BLOC_LIBRE 0xFFFFFFFFUL
#define BLOC_ERREUR 0xFFFFFFFFUL
#define DERNIER_BLOC 0xFFFFFFFFUL

struct hache_entree {
    uint32_t premier; // numero du premier bloc de la liste chainée
    uint32_t suivant; // numero du bloc suivant de la liste chainée
    uint32_t longueur; // longueur, en octets, de l’objet stocké.
    uint8_t obj[1012]; // l’objet stocké.
};

uint32_t bloc_libre(struct hache_entree *table, uint32_t b, uint32_t indice_depart);

#endif