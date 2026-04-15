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
#include "cmd.h"

#define BLOC_LIBRE 0xFFFFFFFFUL
#define BLOC_ERREUR 0xFFFFFFFFUL
#define FIN_CHAINE 0xFFFFFFFFUL
#define ESPACE_MAX 1008

enum type_donnee {
    TYPE_VIDE = 0,
    TYPE_CHAINE = 1,
    TYPE_FICHIER = 2,
    TYPE_NOM_FICHIER = 3
};

struct hache_entree {
    uint32_t premier; // numero du premier bloc de la liste chainée
    uint32_t suivant; // numero du bloc suivant de la liste chainée
    uint32_t longueur; // longueur, en octets, de l’objet stocké.
    uint32_t type; // le type de donnee qu'on insere (chaine, fichier...)
    char obj[ESPACE_MAX]; // l’objet stocké.
};

uint32_t bloc_libre(struct hache_entree *table, uint32_t b, uint32_t indice);

uint32_t compare_bloc(struct hache_entree *table, uint32_t bloc_depart, const char *donnee, size_t taille_donnee);

uint32_t recherche_bloc(const char *nom_table, struct hache_entree *table, const void *C, size_t taille_C, uint32_t type_donnee, uint32_t b);

uint32_t bloc_suffisant(struct hache_entree *table, size_t taille_chaine, const uint32_t b);

int bloc_suffisant_fichier(const char *nom_table, struct hache_entree *table, const char *nom_fichier, uint32_t b);

uint32_t get_b(int fd);

int print_bloc(const char *nom_table);

#endif