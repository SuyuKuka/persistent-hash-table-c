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

typedef struct {
    int thread_id;
    const char *nom_table;
    struct table_entree *t;
    const char *contenu;
    size_t taille_fichier;
    size_t debut;
    size_t fin;
    pthread_mutex_t *mutex_insert;
    uint32_t erreur;
} thread_gros_fichier_args;


int open_table(const char *nom_table, struct table_entree *t, const int flags_open, const int prot_mmap, const long slowns);

void close_table(struct table_entree *t);

uint32_t insert_nom_fichier(struct table_entree *t, const char *donnee, const int premier_bloc);

uint32_t handle_data(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

uint32_t insertdata(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

uint32_t *recuperer_premiere_donnee(struct table_entree *t);

char *recuperer_chaine_complete(struct table_entree *t, uint32_t index);

/**
 * @brief Recupere les index des versions associees a un nom de fichier.
 * @param t La table de hachage ouverte et projetee en memoire.
 * @param index L'index du premier bloc de l'entree de type TYPE_NOM_FICHIER.
 * @return Retourne un tableau alloue contenant les index des versions, sinon NULL en cas d'erreur.
 */
uint32_t *recuperer_versions_fichier(struct table_entree *t, uint32_t index);

void mettre_a_jour_versions_fichiers(struct table_entree *t, uint32_t *versions, const uint32_t nb_elements, const uint32_t i);

void supprimer_bloc(struct table_entree *t, uint32_t index);

void supprimer_par_derriere(struct table_entree *t, uint32_t index, int tout_supprimer);

uint32_t supprimer_versions_fichier(const char *nom_table, struct table_entree *t, uint32_t index);

/**
 * @brief Supprime toutes les versions de fichier associees a un nom de fichier.
 * @param t La table de hachage ouverte et projetee en memoire.
 * @param index L'index du premier bloc de l'entree de type TYPE_NOM_FICHIER.
 * @return Retourne 0 en cas de succes, sinon BLOC_ERREUR.
 */
uint32_t supprimer_nom_fichier(struct table_entree *t, uint32_t index);

uint32_t deplacer_bloc(const char *nom_table, struct table_entree *t);

/**
 * @brief Sauvegarde une version de fichier obtenue par la commande lookup.
 * @param t La table de hachage ouverte et projetee en memoire.
 * @param index L'index du premier bloc du fichier a sauvegarder.
 * @param numero Le numero de version ajoute au nom du fichier sauvegarde.
 * @return Retourne 0 en cas de succes, sinon -1.
 */
int sauvegarder_fichier_lookup(struct table_entree *t, uint32_t index, uint32_t numero);

/**
 * @brief Affiche les metadonnees principales d'une version de fichier.
 * @param t La table de hachage ouverte et projetee en memoire.
 * @param index L'index du premier bloc du fichier a afficher.
 * @return Ne retourne aucune valeur.
 */
void afficher_resume_fichier(struct table_entree *t, uint32_t index);

/**
 * @brief Reconstruit un fichier stocke dans la table et extrait son contenu et ses metadonnees.
 * @param t La table de hachage ouverte et projetee en memoire.
 * @param index L'index du premier bloc du fichier a extraire.
 * @param objet_complet L'adresse recevant l'objet complet alloue en memoire.
 * @param contenu L'adresse recevant un pointeur vers le contenu reel du fichier.
 * @param taille_contenu L'adresse recevant la taille du contenu du fichier.
 * @param nom_fichier La zone recevant le nom du fichier extrait.
 * @param modif L'adresse recevant la date de modification du fichier.
 * @param droit L'adresse recevant les droits du fichier.
 * @return Retourne 0 en cas de succes, sinon -1.
 */
int extraire_fichier_depuis_bloc(struct table_entree *t, uint32_t index, char **objet_complet, char **contenu, size_t *taille_contenu, char *nom_fichier, long *modif, int *droit);

/**
 * @brief Lit et verifie le numero de version choisi par l'utilisateur.
 * @param nb_elements Le nombre de versions disponibles.
 * @param choix_version L'adresse recevant le numero de version choisi.
 * @return Retourne 0 si le numero est valide, sinon -1.
 */
int lire_numero_version(uint32_t nb_elements, uint32_t *choix_version);

void *insert_nouvelle_table(void *arg);

uint32_t init_extend_shrink(const char *nom_table, const uint32_t est_extend, const long slowns);

/**
 * @brief Insere les lignes d'une zone de fichier traitee par un thread.
 * @param arg Les arguments du thread contenant la zone a traiter et la table partagee.
 * @return Retourne toujours NULL ; une erreur est indiquee dans les arguments du thread.
 */
void *insert_lignes_gros_fichier(void *arg);


#endif