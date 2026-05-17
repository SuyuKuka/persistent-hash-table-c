#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include "cmd.h"

struct table_entree;
struct hache_entree;
typedef struct {
    int thread_id;
    struct table_entree *t;
    const char *nom_nouvelle_table;
    struct table_entree *t_nouveau;
    uint32_t *premiere_donnee;
    size_t debut;
    size_t fin;
    uint32_t b;
    uint32_t erreur;
    uint32_t *index_nom_fichier;
    pthread_mutex_t *mutex;
} thread_ags;

int open_table(const char *nom_table, struct table_entree *t, const int flags_open, const int prot_mmap, const long slowns);

void close_table(struct table_entree *t);

uint32_t insert_nom_fichier(struct table_entree *t, const char *donnee, const int premier_bloc);

uint32_t handle_data(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

uint32_t insertdata(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

uint32_t *recuperer_premiere_donnee(struct table_entree *t);

char *recuperer_chaine_complete(struct table_entree *t, uint32_t index);

uint32_t *recuperer_versions_fichier(struct table_entree *t, uint32_t index);

void mettre_a_jour_versions_fichiers(struct table_entree *t, uint32_t *versions, const uint32_t nb_elements, const uint32_t i);

void supprimer_bloc(struct table_entree *t, uint32_t index);

void supprimer_par_derriere(struct table_entree *t, uint32_t index, int tout_supprimer);

uint32_t supprimer_versions_fichier(const char *nom_table, struct table_entree *t, uint32_t index);

uint32_t supprimer_nom_fichier(struct table_entree *t, uint32_t index);

uint32_t deplacer_bloc(const char *nom_table, struct table_entree *t);

int sauvegarder_fichier_lookup(struct table_entree *t, uint32_t index, uint32_t numero);

void afficher_resume_fichier(struct table_entree *t, uint32_t index);

int extraire_fichier_depuis_bloc(struct table_entree *t, uint32_t index, char **objet_complet, char **contenu, size_t *taille_contenu, char *nom_fichier, long *modif, int *droit);

int lire_numero_version(uint32_t nb_elements, uint32_t *choix_version);

void *insert_nouvelle_table(void *arg);

uint32_t init_extend_shrink(const char *nom_table, const uint32_t est_extend, const long slowns);

#endif