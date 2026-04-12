#include "bloc.h"

uint32_t bloc_libre(struct hache_entree *table, uint32_t b, uint32_t indice) {
    uint32_t i = indice;
    while (table[i].longueur != BLOC_LIBRE) {
        i = (i + 1) % b;
        if (i == indice) {
            return BLOC_ERREUR;
        }
    }

    return i;
}

int compare_bloc(struct hache_entree *table, uint32_t bloc_depart, const char *donnee, size_t taille_donnee) {
    uint32_t actuel = bloc_depart;
    size_t offset = 0;

    while (taille_donnee > 0) {
        if (actuel == FIN_CHAINE) {
            return 0;
        }

        size_t donnee_sub = (taille_donnee > ESPACE_MAX) ? ESPACE_MAX : taille_donnee;
        if (memcmp(table[actuel].obj, donnee + offset, donnee_sub) != 0) {
            return 0;
        }

        offset += donnee_sub;
        taille_donnee -= donnee_sub;
        actuel = table[actuel].suivant;
    }

    return 1;
}

int recherche_bloc(const char *nom_table, struct hache_entree *table, const void *C, size_t taille_C, uint32_t type_donnee, uint32_t b) {
    uint32_t index = h(nom_table, C, taille_C, b);
    uint32_t i = index;
    do {
        if (table[i].longueur == BLOC_LIBRE) {
            i = (i + 1) % b;
            continue;
        }

        if (table[i].type == type_donnee) {
            if (type_donnee == TYPE_NOM_FICHIER) {
                if (strcmp(table[i].obj, C) == 0) {
                    return i;
                }
            } else {
                if (table[i].longueur == taille_C && compare_bloc(table, i, C, taille_C) == 0) {
                    return i;
                }
            }
        }

        i = (i + 1) % b;
    } while (i != index);

    return -1;
}

int bloc_suffisant(struct hache_entree *table, size_t taille_chaine, const uint32_t b) {
    uint32_t dispo = 0;
    for (uint32_t i = 0; i < b; i++) {
        if (table[i].longueur == BLOC_LIBRE) dispo++;
    }

    uint32_t besoin = (taille_chaine + ESPACE_MAX - 1) / ESPACE_MAX;
    if (dispo < besoin) {
        return -1;
    }

    return 0;
}

int bloc_suffisant_fichier(const char *nom_table, struct hache_entree *table, const char *nom_fichier, uint32_t b) {
    uint32_t index = h(nom_table, nom_fichier, strlen(nom_fichier), b);
    uint32_t i = index;

    do {
        if (table[i].longueur == BLOC_LIBRE) {
            return 1; 
        }

        if (table[i].type == TYPE_NOM_FICHIER && strcmp(table[i].obj, nom_fichier) == 0) {
            uint32_t actuel = i;
            while (table[actuel].suivant != FIN_CHAINE) {
                actuel = table[actuel].suivant;
            }

            if (table[actuel].longueur + 4 <= ESPACE_MAX) {
                return 0;
            } else {
                return 1;
            }
        }

        i = (i + 1) % b;
    } while (i != index);

    return 1;
}

uint32_t get_b(int fd) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("[ERREUR] fstat (get_b)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (st.st_size % sizeof(struct hache_entree) != 0) {
        fprintf(stderr, "[ERREUR] get_b: la table est corrompue.\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    uint32_t b = st.st_size / sizeof(struct hache_entree);
    if (b == 0) {
        fprintf(stderr, "[ERREUR] get_b: la table est vide.\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return b;
}

void print_bloc(const char *nom_table) {
    int fd = open(nom_table, O_RDONLY);
    if (fd < 0) {
        perror("[ERREUR] open (dump)");
        return;
    }

    uint32_t b = get_b(fd);
    size_t taille_table = b * sizeof(struct hache_entree);
    struct hache_entree *table = mmap(NULL, taille_table, PROT_READ, MAP_SHARED, fd, 0);
    if (table == MAP_FAILED) {
        perror("[ERREUR] mmap (print_bloc)");
        close(fd);
        return;
    }

    printf("\n--------------------------- CONTENU DE LA TABLE ---------------------------\n");
    for (uint32_t i = 0; i < b; i++) {
        if (table[i].longueur == BLOC_LIBRE) {
            printf("[DEBUG] %u | Vide\n", i);
        } else {
            printf("[DEBUG] %u | Occupe | Taille: %4u | Premier: %u | Suivant: %u\n", i, table[i].longueur, table[i].premier, table[i].suivant);
        }
    }
    printf("----------------------------------------------------------------------------\n");

    munmap(table, taille_table);
    close(fd);
}