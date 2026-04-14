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
};

int make_cmd(const char *nom_table, const uint32_t b);

int free_cmd(const char *nom_table);

int insertstring_cmd(const char *nom_table, const char *chaine);

int insertfile_cmd(const char *nom_table, const char *nom_fichier);

int lookup_cmd();

int deletehash_cmd();

int delete_cmd(const char *nom_table, const char *chaine);

int check_cmd(const char *nom_table);

int extend_cmd();

int shrink_cmd();

int ajust_cmd();

#endif