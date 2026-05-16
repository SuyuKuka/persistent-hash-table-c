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

int lookup_cmd(const char *nom_table, const char *chaine, const long slowns);

int deletehash_cmd(const char *nom_table, const uint32_t hashcode, const long slowns);

int delete_cmd(const char *nom_table, const char *chaine, const long slowns);

int check_cmd(const char *nom_table, const long slowns);

int extend_cmd(const char *nom_table, const long slowns);

int shrink_cmd(const char *nom_table, const long slowns);

int ajust_cmd(const char *nom_table, const long slowns);

#endif