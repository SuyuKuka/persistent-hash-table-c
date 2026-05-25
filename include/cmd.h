#ifndef CMD_H
#define CMD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "bloc.h"
#include "utils.h"

struct table_entree {
    int fd;
    struct hache_entree *table;
    uint32_t b;
    size_t taille_table;
    long slowns;
};

int make_cmd(const char *nom_table, const uint32_t b);

int free_cmd(const char *nom_table, const long slowns);

int insertstring_cmd(const char *nom_table, const char *chaine, const long slowns);

int insertfile_cmd(const char *nom_table, const char *nom_fichier, const long slowns);

/**
 * @brief Recherche une chaine ou un nom de fichier dans la table de hachage.
 * @param nom_table Le nom de la table de hachage.
 * @param chaine La chaine ou le nom de fichier a rechercher.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return Retourne 0 en cas de succes, sinon -1.
 */
int lookup_cmd(const char *nom_table, const char *chaine, const long slowns);

int deletehash_cmd(const char *nom_table, const uint32_t hashcode, const long slowns);

/**
 * @brief Supprime une chaine ou les fichiers associes a un nom dans la table.
 * @param nom_table Le nom de la table de hachage.
 * @param chaine La chaine ou le nom de fichier a supprimer.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return Retourne 0 en cas de succes, sinon -1.
 */
int delete_cmd(const char *nom_table, const char *chaine, const long slowns);

int check_cmd(const char *nom_table, const long slowns);

int extend_cmd(const char *nom_table, const long slowns);

int shrink_cmd(const char *nom_table, const long slowns);

int ajust_cmd(const char *nom_table, const long slowns);

/**
 * @brief Insere chaque ligne non vide d'un fichier comme une chaine en utilisant plusieurs threads.
 * @param nom_table Le nom de la table de hachage.
 * @param nom_fichier Le nom du fichier texte contenant les lignes a inserer.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return Retourne 0 en cas de succes, sinon -1.
 */
int insertgrosfichier_cmd(const char *nom_table, const char *nom_fichier, const long slowns);


#endif