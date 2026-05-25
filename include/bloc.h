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
#define ESPACE_MAX 1004

enum type_donnee {
    TYPE_VIDE = 0,
    TYPE_CHAINE = 1,
    TYPE_FICHIER = 2,
    TYPE_NOM_FICHIER = 3
};

struct table_entree;
struct hache_entree {
    uint32_t premier; // numero du premier bloc de la liste chainée
    uint32_t precedent; // numero du bloc precedent de la liste chainee
    uint32_t suivant; // numero du bloc suivant de la liste chainée
    uint32_t longueur; // longueur, en octets, de l’objet stocké.
    uint32_t type; // le type de donnee qu'on insere (chaine, fichier...)
    char obj[ESPACE_MAX]; // l’objet stocké.
};

/**
 * @brief Cherche et réserve si trouver le prochain bloc libre dans la table via un sondage linéaire.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param index L'index avec lequel on va commencer la recherche.
 * @return retourne 0 si il y a au moins un bloc libre, sinon BLOC_ERREUR.
 */
uint32_t bloc_libre(struct table_entree *t, uint32_t index);

/**
 * @brief Compare les données du bloc actuel avec ceux donner en paramètre.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param bloc_depart Le bloc avec lequel on va commencer la comparaison.
 * @param donnee La donnée qu'on va comparer. 
 * @param taille_donnee La taille de la donnée
 * @return retourne 0 en cas de succès, sinon BLOC_ERREUR.
 */
uint32_t compare_bloc(struct table_entree *t, uint32_t bloc_depart, const char *donnee, size_t taille_donnee);

/**
 * @brief Cherche si la donnée est dans la table de hachage ou pas.
 * @param nom_table Le nom de la table de hachage.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param C La donnée que l'on cherche.
 * @param taille_C La taille de la donnée.
 * @param type_donnee Le type de la donnée.
 * @return retourne l'index de la donnée de la table si elle y est, sinon BLOC_ERREUR si la donnée n'est pas dans la table.
 */
uint32_t recherche_bloc(const char *nom_table, struct table_entree *t, const void *C, size_t taille_C, const uint32_t type_donnee);

/**
 * @brief Vérifie si la table contient assez de blocs pour insérer la chaine de caractère.
 * @param table La structure d'un bloc.
 * @param taille_chaine La taille de la chaine de caractère.
 * @param b Le nombre de bloc dans la table de hachage.
 * @return retourne 0 en cas de succès, sinon BLOC_ERREUR.
 */
uint32_t bloc_suffisant(struct hache_entree *table, size_t taille_chaine, const uint32_t b);

/**
 * @brief Vérifie si la table contient assez de blocs pour insérer le fichier.
 * @param nom_table Le nom de la table de hachage.
 * @param t La structure qui contient la table de hachage ouverte et projetée en memoire, le descripteur de fichier, le nombre de bloc dans la table et slowns.
 * @param nom_fichier Le nom du fichier.
 * @return retourne 0 si l'espace est suffisant dans le bloc actuel, 1 s'il faut un nouveau bloc, sinon BLOC_ERREUR en cas d'échec.
 */
uint32_t bloc_suffisant_fichier(const char *nom_table, struct table_entree *t, const char *nom_fichier);

/**
 * @brief Récupère le nombre de bloc dans la table de hachage.
 * @param fd Le descripteur de fichier.
 * @return retourne 0 en cas de succès, sinon BLOC_ERREUR.
 */
uint32_t get_b(int fd);

/**
 * @brief Affiche tous les blocs de la table de hachage.
 * @param nom_table Le nom de la table de hachage.
 * @return retourne 0 en cas de succès, sinon BLOC_ERREUR.
 */
uint32_t print_bloc(const char *nom_table);

/**
 * @brief Pose un verrou POSIX (lecture ou écriture) sur le bloc donné en paramètre.
 * @param fd Le descripteur de fichier qu'on vérrouille.
 * @param index La position du bloc dans la table pour vérrouiller le bon bloc.
 * @param type_verrou Le type de verrou qu'on applique au fichier.
 * @return retourne 0 en cas de succès, sinon BLOC_ERREUR.
 */
uint32_t verrouiller_bloc(int fd, uint32_t index, short type_verrou);

/**
 * @brief Enlève le verrou POSIX du bloc donné en paramètre.
 * @param fd Le descripteur de fichier qu'on devérrouille.
 * @param index La position du bloc dans la table pour devérrouiller le bon bloc.
 * @return retourne 0 en cas de succès, sinon BLOC_ERREUR.
 */
uint32_t deverrouiller_bloc(int fd, uint32_t index);

/**
 * @brief Simule la latence du disque en mettant en pause l'exécution du programme.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès.
 */
uint32_t attendre(const long slowns);

#endif