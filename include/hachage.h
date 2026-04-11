#ifndef HACHAGE_H
#define HACHAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <openssl/sha.h>

/**
 * @brief Calcule le hashcode d'un objet binaire de n octets.
 * @param C Pointeur vers l'objet binaire.
 * @param n Taille de C en octets.
 * @param b Nombre de blocs dans la table.
 * @return L'indice du bloc (entre 0 et b - 1) dans la table.
 */
uint32_t checksum32b(const void *C, size_t n, uint32_t b);

/**
 * @brief Calcule le hashcode d'un objet binaire de n octets en utilisant SHA1.
 * @param C Pointeur vers l'objet binaire.
 * @param n Taille de C en octets.
 * @param b Nombre de blocs dans la table.
 * @return L'indice du bloc (entre 0 et b - 1) dans la table.
 */
uint32_t sha1b(const void *C, size_t n, uint32_t b);

/**
 * @brief Calcule le hashcode d'un objet binaire de n octets en fonction de l'extension du fichier.
 * @param nom_table Nom de la table.
 * @param C Pointeur vers l'objet binaire.
 * @param n Taille de C en octets.
 * @param b Nombre de blocs dans la table.
 * @return L'indice du bloc (entre 0 et b - 1) dans la table.
 */
uint32_t h(const char* nom_table, const void *C, size_t n, uint32_t b);

#endif