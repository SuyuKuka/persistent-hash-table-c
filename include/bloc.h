#ifndef BLOC_H
#define BLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "hachage.h"

#define BLOC_LIBRE 0xFFFFFFFFUL
#define BLOC_ERREUR 0xFFFFFFFFUL
#define FIN_CHAINE 0xFFFFFFFFUL
#define ESPACE_MAX 1012

struct hache_entree {
    uint32_t premier; // numero du premier bloc de la liste chainée
    uint32_t suivant; // numero du bloc suivant de la liste chainée
    uint32_t longueur; // longueur, en octets, de l’objet stocké.
    uint8_t obj[ESPACE_MAX]; // l’objet stocké.
};

uint32_t bloc_libre(struct hache_entree *table, uint32_t b, uint32_t indice);

int recherche_bloc(const char *nom_table, struct hache_entree *table, const void *C, size_t taille_C, uint32_t b);

int bloc_suffisant(struct hache_entree *table, size_t taille_chaine, uint32_t b);

uint32_t get_b(int fd);

void print_bloc(const char *nom_table);

#endif