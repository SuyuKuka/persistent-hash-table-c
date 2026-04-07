#include "hachage.h"

uint32_t checksum32b(const void *C, size_t n, uint32_t b) {
    if (b == 0) {
        fprintf(stderr, "checksomme32b: b ne peut pas etre égal a 0.\n");
        exit(EXIT_FAILURE);
    }

    uint32_t somme = 0;
    const uint8_t *donnee = (const uint8_t *)C;
    size_t i;

    for (i = 0; i + 3 < n; i += 4) {
        uint32_t mot;
        mot = donnee[i] | (donnee[i+1] << 8) | (donnee[i+2] << 16) | (donnee[i+3] << 24);
        somme += mot; 
    }

    if (i < n) {
        uint32_t dernier_mot  = 0;
        dernier_mot  |= donnee[i];
        if (i + 1 < n) dernier_mot  |= (donnee[i+1] << 8);
        if (i + 2 < n) dernier_mot  |= (donnee[i+2] << 16);
        somme += dernier_mot ;
    }

    return somme % b;
}

uint32_t sha1b(const void *C, size_t n, uint32_t b) {
    (void) C;
    (void) n;
    (void) b;
    return 0;
}

uint32_t h(const char* fichier, const void *C, size_t n, uint32_t b) {
    if (strstr(fichier, ".cks") != NULL) {
        return checksum32b(C, n, b);
    } else if (strstr(fichier, ".sha") != NULL) {
        return sha1b(C, n, b);
    } else {
        fprintf(stderr, "h: l'extension du fichier %s n'est pas pris en compte.\n", fichier);
        exit(EXIT_FAILURE);
    }
}