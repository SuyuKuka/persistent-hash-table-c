#include "bloc.h"

uint32_t bloc_libre(struct table_entree *t, uint32_t index) {
    uint32_t i = index;
    do {
        verrouiller_bloc(t->fd, i, F_WRLCK);
        if (t->table[i].longueur == BLOC_LIBRE) {
            t->table[i].longueur = 0; 
            attendre(t->slowns);
            deverrouiller_bloc(t->fd, i);
            return i;
        }

        attendre(t->slowns);
        deverrouiller_bloc(t->fd, i);
        i = (i + 1) % t->b;
    } while (i != index);

    fprintf(stderr, "[ERREUR] bloc_libre: la table n'a plus de blocs libres.\n");
    return BLOC_ERREUR;
}

uint32_t compare_bloc(struct table_entree *t, uint32_t bloc_depart, const char *donnee, size_t taille_donnee) {
    uint32_t actuel = bloc_depart;
    size_t offset = 0;

    while (taille_donnee > 0) {
        if (actuel == FIN_CHAINE) {
            return BLOC_ERREUR;
        }

        verrouiller_bloc(t->fd, actuel, F_RDLCK);
        size_t donnee_sub = (taille_donnee > ESPACE_MAX) ? ESPACE_MAX : taille_donnee;
        int comp = memcmp(t->table[actuel].obj, donnee + offset, donnee_sub);
        uint32_t suivant_actuel = t->table[actuel].suivant;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, actuel);

        if (comp != 0) {
            return BLOC_ERREUR;
        } 

        offset += donnee_sub;
        taille_donnee -= donnee_sub;
        actuel = suivant_actuel;
    }

    return 0;
}

uint32_t recherche_bloc(const char *nom_table, struct table_entree *t, const void *C, size_t taille_C, const uint32_t type_donnee) {
    uint32_t index = h(nom_table, C, taille_C, t->b);
    if (index == BLOC_ERREUR) {
        return BLOC_ERREUR;
    }

    verrouiller_bloc(t->fd, index, F_RDLCK);
    uint32_t i = t->table[index].premier;
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, index);
    if (i == BLOC_LIBRE) {
        return BLOC_ERREUR;
    }

    uint32_t trouver = 0;
    while (i != FIN_CHAINE) {
        verrouiller_bloc(t->fd, i, F_RDLCK);
        uint32_t longueur_actuelle = t->table[i].longueur;
        uint32_t suivant_actuel = t->table[i].suivant;
        if (t->table[i].type == type_donnee) {
            if (type_donnee == TYPE_NOM_FICHIER) {
                if (strcmp(t->table[i].obj, C) == 0) {
                    trouver = 1;
                }
            } else {
                if (longueur_actuelle == taille_C && compare_bloc(t, i, C, taille_C) == 0) {
                    trouver = 1;
                }
            }
        }
        
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, i);

        if (trouver == 1) {
            return i;
        }

        while (suivant_actuel != FIN_CHAINE && longueur_actuelle > ESPACE_MAX && longueur_actuelle != BLOC_LIBRE) {
            i = suivant_actuel;
            
            verrouiller_bloc(t->fd, i, F_RDLCK);
            longueur_actuelle = t->table[i].longueur;
            suivant_actuel = t->table[i].suivant;
            attendre(t->slowns);
            deverrouiller_bloc(t->fd, i);
        }

        if (i != FIN_CHAINE) {
            i = suivant_actuel;
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

uint32_t bloc_suffisant_fichier(const char *nom_table, struct table_entree *t, const char *nom_fichier) {
    uint32_t index = h(nom_table, nom_fichier, strlen(nom_fichier), t->b);
    if (index == BLOC_ERREUR) {
        return BLOC_ERREUR;
    }

    verrouiller_bloc(t->fd, index, F_RDLCK);
    uint32_t i = t->table[index].premier;
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, index);
    if (i == BLOC_LIBRE) {
        return 1;
    }

    while (i != FIN_CHAINE) {
        verrouiller_bloc(t->fd, i, F_RDLCK);
        uint32_t type_actuel = t->table[i].type;
        uint32_t longueur_actuelle = t->table[i].longueur;
        uint32_t suivant_actuel = t->table[i].suivant;
        int trouver = type_actuel == TYPE_NOM_FICHIER && strcmp(t->table[i].obj, nom_fichier) == 0;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, i);

        if (trouver) {
            uint32_t actuel = i;
            
            while (suivant_actuel != FIN_CHAINE && longueur_actuelle > ESPACE_MAX) {
                actuel = suivant_actuel;

                verrouiller_bloc(t->fd, actuel, F_RDLCK);
                longueur_actuelle = t->table[actuel].longueur;
                suivant_actuel = t->table[actuel].suivant;
                attendre(t->slowns);
                deverrouiller_bloc(t->fd, actuel);
            }

            if (longueur_actuelle + sizeof(uint32_t) <= ESPACE_MAX) {
                return 0;
            } else {
                return 1;
            }
        }

        while (suivant_actuel != FIN_CHAINE && longueur_actuelle > ESPACE_MAX) {
            i = suivant_actuel;
            
            verrouiller_bloc(t->fd, i, F_RDLCK);
            longueur_actuelle = t->table[i].longueur;
            suivant_actuel = t->table[i].suivant;
            attendre(t->slowns);
            deverrouiller_bloc(t->fd, i);
        }

        i = suivant_actuel;
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

uint32_t print_bloc(const char *nom_table) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ, 0) < 0) {
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
        return BLOC_ERREUR;
    }

    for (uint32_t i = 0; i < t.b; i++) { 
        if (t.table[i].longueur != BLOC_LIBRE && t.table[i].type == TYPE_NOM_FICHIER && premiere_donnee[i] == 0) {
            uint32_t *versions = recuperer_versions_fichier(&t, i);
            if (versions == NULL) {
                free(premiere_donnee);
                return BLOC_ERREUR;
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

uint32_t verrouiller_bloc(int fd, uint32_t index, short type_verrou) {
    struct flock lock;
    lock.l_type = type_verrou;
    lock.l_whence = SEEK_SET;
    lock.l_start = index * 1024;
    lock.l_len = 1024;

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("[ERREUR] fcntl verrouillage");
        return BLOC_ERREUR;
    }
    return 0;
}

uint32_t deverrouiller_bloc(int fd, uint32_t index) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = index * sizeof(struct hache_entree);
    lock.l_len = sizeof(struct hache_entree);

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("fcntl");
        return BLOC_ERREUR;
    }

    return 0;
}

uint32_t attendre(const long slowns) {
    if (slowns > 0) {
        struct timespec time;
        time.tv_sec = slowns / 1000000000L; 
        time.tv_nsec = slowns % 1000000000L;
        nanosleep(&time, NULL);
    }

    return 0;
}
