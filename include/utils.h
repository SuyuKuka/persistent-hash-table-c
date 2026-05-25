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

/**
 * @brief Ouvre la table de hachage, calcule sa taille et la projette en mémoire.
 * @param nom_table Le nom de la table de hachage.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param flags_open Les drapeaux d'ouverture pour l'appel système open.
 * @param prot_mmap Les drapeaux de protection pour l'appel système mmap.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int open_table(const char *nom_table, struct table_entree *t, const int flags_open, const int prot_mmap, const long slowns);

/**
 * @brief Ferme la projection en mémoire et le descripteur de fichier.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 */
void close_table(struct table_entree *t);

/**
 * @brief Ajoute le hashcode du nouveau fichier dans le nom du fichier.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param donnee La donnée qu'on veut insérer dans la table.
 * @param premier_bloc L'index du premier bloc du nom de fichier.
 * @return 0 en cas de succès, sinon BLOC_ERREUR si la table n'a pas de bloc libre.
 */
uint32_t insert_nom_fichier(struct table_entree *t, const char *donnee, const int premier_bloc);

/**
 * @brief Gère l'insertion de donnée en vérifiant si elle est déjà présente dans la table. Mais aussi, en mettant à jour le bloc du nom fichier si c'est un type nom fichier.
 * @param nom_table Le nom de la table de hachage.
 * @param cle La clé utilisée pour la recherche et le calcul du hashcode.
 * @param taille_cle La taille de la clé.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param donnee La donnée à insérer.
 * @param taille_donnee La taille de la donnée.
 * @param type_donnee Le type de donnée.
 * @return Retourne l'index dans lequel la donnée a été insérer, sinon BLOC_ERREUR si la donnee existe déjà dans la table.
 */
uint32_t handle_data(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

/**
 * @brief Insère la nouvelle donnée dans la table de hachage, en plusieurs blocs si nécessaire.
 * @param nom_table Le nom de la table de hachage.
 * @param cle La clé à hacher pour déterminer l'index de départ.
 * @param taille_cle La taille de la clé.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param donnee La donnée à insérer.
 * @param taille_donnee La taille de la donnée.
 * @param type_donnee Le type de donnée.
 * @return L'index du premier bloc alloué pour cette donnée, ou BLOC_ERREUR si la table est pleine.
 */
uint32_t insertdata(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

/**
 * @brief Récupère une liste de tous les blocs. Dans lequel on a 1 si la donnée est première sinon 0, si c'est le reste d'un multi-bloc.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @return La liste des premiers blocs dans la table de hachage.
 */
uint32_t *recuperer_premiere_donnee(struct table_entree *t);

/**
 * @brief Récupère la chaine complète si elle est séparé en plusieurs blocs.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param index L'index du premier bloc de la chaine.
 * @return Retourne la chaine complète, sinon NULL en cas d'erreur.
 */
char *recuperer_chaine_complete(struct table_entree *t, uint32_t index);

/**
 * @brief Recupere les index des versions associees a un nom de fichier.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param index L'index du premier bloc de l'entree de type TYPE_NOM_FICHIER.
 * @return Retourne un tableau alloue contenant les index des versions, sinon NULL en cas d'erreur.
 */
uint32_t *recuperer_versions_fichier(struct table_entree *t, uint32_t index);

/**
 * @brief Met à jour toutes les versions d'un nom fichier et réorganise tous les indéx.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param versions la liste des versions du fichiers après modification.
 * @param nb_elements Le nombre de versions du fichiers.
 * @param i L'index du premier bloc de l'entrée correspondant au nom du fichier.
 */
void mettre_a_jour_versions_fichiers(struct table_entree *t, uint32_t *versions, const uint32_t nb_elements, const uint32_t i);

/**
 * @brief Supprime le bloc à la position i de la table, met à jour les liens de chaînage et réinitialise son contenu.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param index L'index du bloc à supprimer.
 */
void supprimer_bloc(struct table_entree *t, uint32_t index);

/**
 * @brief Supprime une chaîne de blocs à partir du dernier bloc jusqu'au premier bloc (index).
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param index L'index du bloc à supprimer.
 * @param tout_supprimer vaut 0 si on doit supprimer toutes les données jusqu'à FIN_CHAINE, sinon 1 si il faut s'arreter à la donnée.
 */
void supprimer_par_derriere(struct table_entree *t, uint32_t index, int tout_supprimer);

/**
 * @brief Supprime la version du fichier dans le nom fichier, qui correspond à l'index en paramètre.
 * @param nom_table Le nom de la table de hachage.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param index L'index du bloc à supprimer.
 * @return .
 */
uint32_t supprimer_versions_fichier(const char *nom_table, struct table_entree *t, uint32_t index);

/**
 * @brief Supprime toutes les versions de fichier associees a un nom de fichier.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param index L'index du premier bloc de l'entree de type TYPE_NOM_FICHIER.
 * @return Retourne 0 en cas de succes, sinon BLOC_ERREUR.
 */
uint32_t supprimer_nom_fichier(struct table_entree *t, uint32_t index);

/**
 * @brief Réorganise la table en déplaçant les blocs mal placés vers leur index de hachage d'origine et met à jour les références.
 * @param nom_table Le nom de la table de hachage.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @return 0 en cas de succès, sinon BLOC_ERREUR dans le cas d'une corruption ou d'un problème de déplacement de bloc.
 */
uint32_t deplacer_bloc(const char *nom_table, struct table_entree *t);

/**
 * @brief Sauvegarde une version de fichier obtenue par la commande lookup.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param index L'index du premier bloc du fichier a sauvegarder.
 * @param numero Le numero de version ajoute au nom du fichier sauvegarde.
 * @return Retourne 0 en cas de succes, sinon -1.
 */
int sauvegarder_fichier_lookup(struct table_entree *t, uint32_t index, uint32_t numero);

/**
 * @brief Affiche les metadonnees principales d'une version de fichier.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param index L'index du premier bloc du fichier a afficher.
 */
void afficher_resume_fichier(struct table_entree *t, uint32_t index);

/**
 * @brief Reconstruit un fichier stocke dans la table et extrait son contenu et ses metadonnees.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
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

/**
 * @brief Routine exécutée par chaque thread pour transférer la portion des données qui leur a été attribuer vers la nouvelle table redimensionnée.
 * @param arg Tous les arguments mis .
 */
void *insert_nouvelle_table(void *arg);

/**
 * @brief Crée une nouvelle table redimensionnée, ensuite transfère les données en multi-threads, puis remplace l'ancienne table.
 * @param nom_table Le nom de la table de hachage.
 * @param est_extend vaut 1 si on utilise la commande extend sinon vaut 0 si on utilise shrink.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return NULL.
 */
uint32_t init_extend_shrink(const char *nom_table, const uint32_t est_extend, const long slowns);

/**
 * @brief Insere les lignes d'une zone de fichier traitee par un thread.
 * @param arg Les arguments du thread contenant la zone a traiter et la table partagee.
 */
void *insert_lignes_gros_fichier(void *arg);

#endif