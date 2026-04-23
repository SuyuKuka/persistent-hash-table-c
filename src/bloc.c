#include "bloc.h"

uint32_t bloc_libre(struct hache_entree *table, uint32_t b, uint32_t indice) {
    uint32_t i = indice;
    while (table[i].longueur != BLOC_LIBRE) {
        i = (i + 1) % b;
        if (i == indice) {
            fprintf(stderr, "[ERREUR] bloc_libre: la table n'a plus de blocs libres.\n");
            return BLOC_ERREUR;
        }
    }

    return i;
}

uint32_t compare_bloc(struct hache_entree *table, uint32_t bloc_depart, const char *donnee, size_t taille_donnee) {
    uint32_t actuel = bloc_depart;
    size_t offset = 0;

    while (taille_donnee > 0) {
        if (actuel == FIN_CHAINE) {
            return BLOC_ERREUR;
        }

        size_t donnee_sub = (taille_donnee > ESPACE_MAX) ? ESPACE_MAX : taille_donnee;
        if (memcmp(table[actuel].obj, donnee + offset, donnee_sub) != 0) {
            return BLOC_ERREUR;
        }

        offset += donnee_sub;
        taille_donnee -= donnee_sub;
        actuel = table[actuel].suivant;
    }

    return 0;
}

uint32_t recherche_bloc(const char *nom_table, struct hache_entree *table, const void *C, size_t taille_C, uint32_t type_donnee, uint32_t b) {
    uint32_t index = h(nom_table, C, taille_C, b);
    if (index == BLOC_ERREUR) {
        return BLOC_ERREUR;
    }

    uint32_t i = table[index].premier;
    if (i == BLOC_LIBRE) {
        return BLOC_ERREUR;
    }

    while (i != FIN_CHAINE) {
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

        while (i != FIN_CHAINE && table[i].longueur > ESPACE_MAX) {
            i = table[i].suivant;
        }

        if (i != FIN_CHAINE) {
            i = table[i].suivant;
        }
    }

    return BLOC_ERREUR;
}

uint32_t bloc_suffisant(struct hache_entree *table, size_t taille_chaine, const uint32_t b) {
    uint32_t dispo = 0;
    for (uint32_t i = 0; i < b; i++) {
        if (table[i].longueur == BLOC_LIBRE) dispo++;
    }

    uint32_t besoin = (taille_chaine + ESPACE_MAX - 1) / ESPACE_MAX;
    if (dispo < besoin) {
        fprintf(stderr, "[ERREUR] bloc_suffisant: la table n'a pas assez de blocs pour inserer toutes les donnees.\n");
        return BLOC_ERREUR;
    }

    return 0;
}

uint32_t bloc_suffisant_fichier(const char *nom_table, struct hache_entree *table, const char *nom_fichier, uint32_t b) {
    uint32_t index = h(nom_table, nom_fichier, strlen(nom_fichier), b);
    if (index == BLOC_ERREUR) {
        return BLOC_ERREUR;
    }

    uint32_t i = table[index].premier;
    if (i == BLOC_LIBRE) {
        return 1;
    }

    while (i != FIN_CHAINE) {
        if (table[i].type == TYPE_NOM_FICHIER && strcmp(table[i].obj, nom_fichier) == 0) {
            uint32_t actuel = i;
            
            while (table[actuel].suivant != FIN_CHAINE && table[actuel].longueur > ESPACE_MAX) {
                actuel = table[actuel].suivant;
            }

            if (table[actuel].longueur + sizeof(uint32_t) <= ESPACE_MAX) {
                return 0;
            } else {
                return 1;
            }
        }

        while (i != FIN_CHAINE && table[i].longueur > ESPACE_MAX) {
            i = table[i].suivant;
        }

        if (i != FIN_CHAINE) {
            i = table[i].suivant;
        }
    }

    return 1;
}

uint32_t get_b(int fd) {
    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("[ERREUR] fstat (get_b)");
        return BLOC_ERREUR;
    }

    if (st.st_size % sizeof(struct hache_entree) != 0) {
        fprintf(stderr, "[ERREUR] get_b: la table est corrompue (taille correspond pas).\n");
        close(fd);
        return BLOC_ERREUR;
    }

    uint32_t b = st.st_size / sizeof(struct hache_entree);
    if (b == 0) {
        fprintf(stderr, "[ERREUR] get_b: la table est vide.\n");
        close(fd);
        return BLOC_ERREUR;
    }

    return b;
}

int print_bloc(const char *nom_table) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ) < 0) {
        return -1;
    }

    printf("\n------------------------------------------------- CONTENU DE LA TABLE -------------------------------------------------\n");
    for (uint32_t i = 0; i < t.b; i++) {
        if (t.table[i].longueur == BLOC_LIBRE) {
            printf("[DEBUG] %10u | Vide\n", i);
        } else {
            printf("[DEBUG] %10u | Occupe | Type: %u | Taille: %4u | Premier: %10u | Precedent: %10u |Suivant: %10u\n", i, t.table[i].type, t.table[i].longueur, t.table[i].premier, t.table[i].precedent, t.table[i].suivant);
        }
    }
    printf("-----------------------------------------------------------------------------------------------------------------------\n");

    uint32_t *premiere_donnee = recuperer_premiere_donnee(&t);
    if (premiere_donnee == NULL) {
        close_table(&t);
        return -1;
    }

    for (uint32_t i = 0; i < t.b; i++) { 
        if (t.table[i].longueur != BLOC_LIBRE && t.table[i].type == TYPE_NOM_FICHIER && premiere_donnee[i] == 0) {
            uint32_t *versions = recuperer_versions_fichier(t.table, i);
            if (versions == NULL) {
                free(premiere_donnee);
                return -1;
            }

            uint32_t offset = strlen(t.table[i].obj) + 1;
            uint32_t nb_elements = (t.table[i].longueur - offset) / sizeof(uint32_t);
            printf("[DEBUG] Fichier %u: %s\n", i, t.table[i].obj);
            printf("[DEBUG] Nb elements: %u\n", nb_elements);
            printf("[DEBUG] Index: [ ");
            for (uint32_t k = 0; k < nb_elements; k++) {
                printf("%u ", versions[k]);
            }
            printf("]\n");
            printf("-----------------------------------------------------------------------------------------------------------------------\n");
            free(versions);
        }
    }
    
    free(premiere_donnee);
    close_table(&t);
    return 0;
}