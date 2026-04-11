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

void make_cmd(const char *nom_table, uint32_t b);

void free_cmd(const char *nom_table);

int insertstring_cmd(const char *nom_table, const char *chaine);

void insertfile_cmd();

void lookup_cmd();

void deletehash_cmd();

void delete_cmd();

void check_cmd();

void extend_cmd();

void shrink_cmd();

void ajust_cmd();

#endif