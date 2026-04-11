#ifndef CMD_H
#define CMD_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "bloc.h"

void make_cmd(const char *nom_table, uint32_t b);

void free_cmd();

void insertstring_cmd();

void insertfile_cmd();

void lookup_cmd();

void deletehash_cmd();

void delete_cmd();

void check_cmd();

void extend_cmd();

void shrink_cmd();

void ajust_cmd();

#endif