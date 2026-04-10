#include "hachage.h"

uint32_t checksum32b(const void *C, size_t n, uint32_t b) {
    if (b == 0) {
        fprintf(stderr, "[ERREUR] checksomme32b: b ne peut pas etre égal a 0.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t somme = 0;
    const uint8_t *donnee = (const uint8_t *)C;
    size_t i;

    for (i = 0; i + 3 < n; i += 4) {
        uint32_t mot;
        mot = ((uint32_t)donnee[i] << 24) | ((uint32_t)donnee[i+1] << 16) | ((uint32_t)donnee[i+2] << 8) | donnee[i+3];
        somme += mot; 
    }

    uint32_t dernier_mot = 0;
    switch (n % 4) {
        case 3:
            dernier_mot |= ((uint32_t)donnee[i + 2] << 8);
            __attribute__((fallthrough));
        case 2:
            dernier_mot |= ((uint32_t)donnee[i + 1] << 16);
            __attribute__((fallthrough));
        case 1:
            dernier_mot |= ((uint32_t)donnee[i] << 24);
            somme += dernier_mot;
            break;
        case 0:
            break;
    }

    return somme % b;
}

uint32_t sha1b(const void *C, size_t n, uint32_t b) {
    if (b == 0) {
        fprintf(stderr, "[ERREUR] sha1b: b ne peut pas etre égal a 0.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t indice = 0;
    unsigned char md[20];

    SHA1((const unsigned char *)C, n, md);
    indice = ((uint32_t)md[16] << 24) | ((uint32_t)md[17] << 16) | ((uint32_t)md[18] << 8)  | ((uint32_t)md[19]);

    return indice % b;
}

uint32_t h(const char* nom_table, const void *C, size_t n, uint32_t b) {
    if (strstr(nom_table, ".cks") != NULL) {
        return checksum32b(C, n, b);
    } else if (strstr(nom_table, ".sha") != NULL) {
        return sha1b(C, n, b);
    } else {
        fprintf(stderr, "[ERREUR] h: l'extension de la table %s n'est pas pris en compte.\n", nom_table);
        exit(EXIT_FAILURE);
    }
}