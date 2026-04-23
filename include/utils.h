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

void mettre_a_jour_versions_fichiers(struct hache_entree *table, uint32_t *versions, const uint32_t nb_elements, const uint32_t i);

void supprimer_bloc(struct hache_entree *table, uint32_t index, const uint32_t b);

void supprimer_par_derriere(struct hache_entree *table, uint32_t index, const uint32_t b);

uint32_t supprimer_versions_fichier(const char *nom_table, struct hache_entree *table, uint32_t index, const uint32_t b);

uint32_t supprimer_nom_fichier(struct hache_entree *table, uint32_t index, const uint32_t b);

uint32_t deplacer_bloc(const char *nom_table, struct hache_entree *table, const uint32_t b);

void supprimer_objet_complet(struct hache_entree *table, uint32_t tete, const uint32_t b);

int sauvegarder_fichier_lookup(struct hache_entree *table, uint32_t index, uint32_t numero);

void afficher_resume_fichier(struct hache_entree *table, uint32_t index);

int extraire_fichier_depuis_bloc(struct hache_entree *table, uint32_t index, char **objet_complet, char **contenu, size_t *taille_contenu, char *nom_fichier, size_t taille_nom, long *modif, int *droit);

int lire_numero_version(uint32_t nb_elements, uint32_t *choix_version);

#endif