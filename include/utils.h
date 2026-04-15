#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "cmd.h"

struct table_entree;
struct hache_entree;

int open_table(const char *nom_table, struct table_entree *t, const int flags_open, const int prot_mmap);

void close_table(struct table_entree *t);

uint32_t insert_nom_fichier(struct table_entree *t, const char *donnee, const int premier_bloc);

uint32_t handle_data(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

uint32_t insertdata(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

uint32_t *recuperer_premiere_donnee(struct table_entree *t);

char *recuperer_chaine_complete(struct hache_entree *table, uint32_t index);

uint32_t *recuperer_versions_fichier(struct hache_entree *table, uint32_t index);

void supprimer_bloc(struct hache_entree *table, uint32_t index);

uint32_t supprimer_versions_fichier(const char *nom_table, struct hache_entree *table, const char *nom_fichier, const uint32_t hashcode, const uint32_t b);

void deplacer_bloc(struct hache_entree *table, const uint32_t hashcode, uint32_t b);

#endif