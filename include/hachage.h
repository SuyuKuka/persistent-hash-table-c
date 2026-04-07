#ifndef HACHAGE
#define HACHAGE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint32_t checksum32b(const void *C, size_t n, uint32_t b);

uint32_t sha1b(const void *C, size_t n, uint32_t b);

#endif