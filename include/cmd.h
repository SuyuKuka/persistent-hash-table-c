#ifndef CMD_H
#define CMD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "bloc.h"

struct table_entree {
    int fd;
    struct hache_entree *table;
    uint32_t b;
    size_t taille_table;
};

int open_table(const char *nom_table, struct table_entree *t, const int flags_open, const int prot_mmap);

void close_table(struct table_entree *t);

long insert_nom_fichier(struct table_entree *t, const char *donnee, const int premier_bloc);

long handle_data(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

long insertdata(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee);

void make_cmd(const char *nom_table, const uint32_t b);

void free_cmd(const char *nom_table);

int insertstring_cmd(const char *nom_table, const char *chaine);

void insertfile_cmd(const char *nom_table, const char *nom_fichier);

void lookup_cmd();

void deletehash_cmd();

void delete_cmd();

void check_cmd();

void extend_cmd();

void shrink_cmd();

void ajust_cmd();

#endif