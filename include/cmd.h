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
    long slowns;
};

/**
 * @brief Créer et initialise une table de hachage de taille b et de nom nom_table.
 * @param nom_table Le nom de la table de hachage.
 * @param b Le nombre de bloc dans la table de hachage.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int make_cmd(const char *nom_table, const uint32_t b);

/**
 * @brief Affique les details de la table de hachage (nombres de blocs libres, occupés, de chaines, de fichiers, d'objets et le taux de remplissage).
 * @param nom_table Le nom de la table de hachage.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int free_cmd(const char *nom_table, const long slowns);

/**
 * @brief Insère une chaine de caractère dans la table de hachage si elle n'est pas présente.
 * @param nom_table Le nom de la table de hachage.
 * @param chaine La chaine qu'on veut insérer dans la table de hachage.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int insertstring_cmd(const char *nom_table, const char *chaine, const long slowns);

/**
 * @brief Insère un fichier dans la table de hachage si il n'est pas présent, ainsi que le nom du fichier.
 * @param nom_table Le nom de la table de hachage.
 * @param nom_fichier Le fichier qu'on veut insérer dans la table de hachage.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int insertfile_cmd(const char *nom_table, const char *nom_fichier, const long slowns);

/**
 * @brief Recherche une chaine ou un nom de fichier dans la table de hachage.
 * @param nom_table Le nom de la table de hachage.
 * @param chaine La chaine ou le nom de fichier a rechercher.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return Retourne 0 en cas de succes, sinon -1.
 */
int lookup_cmd(const char *nom_table, const char *chaine, const long slowns);

/**
 * @brief Supprime toutes les données qui ont le hashcode puis remet en place les données de la table de hachage.
 * @param nom_table Le nom de la table de hachage.
 * @param hashcode Le hashcode des données qui vont être supprimer.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int deletehash_cmd(const char *nom_table, const uint32_t hashcode, const long slowns);

/**
 * @brief Supprime une chaine ou les fichiers associes a un nom dans la table.
 * @param nom_table Le nom de la table de hachage.
 * @param chaine La chaine ou le nom de fichier a supprimer.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return Retourne 0 en cas de succes, sinon -1.
 */
int delete_cmd(const char *nom_table, const char *chaine, const long slowns);

/**
 * @brief Vérifie l'intégrité de la table de hachage et signale les corruptions.
 * @param nom_table Le nom de la table de hachage.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int check_cmd(const char *nom_table, const long slowns);

/**
 * @brief Double la taille de la table de hachage par 2, puis réorganise la table.
 * @param nom_table Le nom de la table de hachage.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int extend_cmd(const char *nom_table, const long slowns);

/**
 * @brief Divise la taille de la table de hachage par 2, puis réorganise la table.
 * @param nom_table Le nom de la table de hachage.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int shrink_cmd(const char *nom_table, const long slowns);

/**
 * @brief Calcule le taux de remplissage et redimensionne la table si nécessaire.
 * @param nom_table Le nom de la table de hachage.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return retourne 0 en cas de succès, sinon -1.
 */
int ajust_cmd(const char *nom_table, const long slowns);

/**
 * @brief Insere chaque ligne non vide d'un fichier comme une chaine en utilisant plusieurs threads.
 * @param nom_table Le nom de la table de hachage.
 * @param nom_fichier Le nom du fichier contenant les lignes a inserer.
 * @param slowns Le temps d'attente en nanosecondes pour simuler la latence du disque.
 * @return Retourne 0 en cas de succes, sinon -1.
 */
int insertgrosfichier_cmd(const char *nom_table, const char *nom_fichier, const long slowns);


#endif