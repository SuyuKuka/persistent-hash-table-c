#include "cmd.h"

int make_cmd(const char *nom_table, const uint32_t b) {
    if (b == 0) {
        fprintf(stderr, "[ERREUR] make: b ne peut pas etre egal a 0.\n");
        return -1;
    }

    int fd = open(nom_table, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("[ERREUR] open (make)");
        return -1;
    }
    
    size_t taille_table = b * sizeof(struct hache_entree);
    if (ftruncate(fd, taille_table) < 0) {
        perror("[ERREUR] ftruncate (make)");
        close(fd);
        return -1;
    }

    struct hache_entree *table = mmap(NULL, taille_table, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (table == MAP_FAILED) {
        perror("[ERREUR] mmap (make)");
        close(fd);
        return -1;
    }

    for (uint32_t i = 0; i < b; i++) {
        table[i].premier = i;
        table[i].precedent = FIN_CHAINE;
        table[i].suivant = FIN_CHAINE;
        table[i].longueur = BLOC_LIBRE;
        table[i].type = TYPE_VIDE;
    }

    munmap(table, taille_table);
    close(fd);
    return 0;
}

int free_cmd(const char *nom_table) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ) < 0) {
        return -1;
    }

    uint32_t bloc_libre = 0;
    uint32_t nb_chaine = 0;
    uint32_t nb_fichier = 0;
    uint32_t *premiere_donnee = recuperer_premiere_donnee(&t);
    if (premiere_donnee == NULL) {
        close_table(&t);
        return -1;
    }

    for (uint32_t i = 0; i < t.b; i++) {
        if (t.table[i].longueur == BLOC_LIBRE) {
            bloc_libre++;
            continue;
        }
        
        if (premiere_donnee[i] == 0) {
            if (t.table[i].type == TYPE_CHAINE) {
                nb_chaine++;
            } else if (t.table[i].type == TYPE_FICHIER) {
                nb_fichier++;
            }
        }
    }

    printf("[INFO] Blocs libres: %u\n", bloc_libre);
    printf("[INFO] Blocs occupes: %u\n", t.b - bloc_libre);
    printf("[INFO] Taux de remplissage: %.2f%% \n\n", ((double)(t.b - bloc_libre) / (double)t.b) * 100.0);

    printf("[INFO] Nombre de chaine: %u\n", nb_chaine);
    printf("[INFO] Nombre de fichier: %u\n", nb_fichier);
    printf("[INFO] Nombre d'objet: %u\n", nb_chaine + nb_fichier);

    free(premiere_donnee);
    close_table(&t);
    return 0;
}

int insertstring_cmd(const char *nom_table, const char *chaine) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE) < 0) {
        return -1;
    }

    size_t taille_chaine = strlen(chaine);
    if (bloc_suffisant(t.table, taille_chaine, t.b) == BLOC_ERREUR) {
        close_table(&t);
        return -1;
    }

    if (handle_data(nom_table, chaine, taille_chaine, &t, chaine, taille_chaine, TYPE_CHAINE) == BLOC_ERREUR) {
        close_table(&t);
        return -1;
    }

    close_table(&t);
    return 0;
}

int insertfile_cmd(const char *nom_table, const char *nom_fichier) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE) < 0) {
        return -1;
    }

    int fd = open(nom_fichier, O_RDONLY);
    if (fd < 0) {
        perror("[ERREUR] open (insertfile)");
        close_table(&t);
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("[ERREUR] fstat (insertfile)");
        close(fd);
        close_table(&t);
        return -1;
    }

    char *contenu = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (contenu == MAP_FAILED) {
        perror("[ERREUR] mmap (insertfile)");
        close(fd);
        close_table(&t);
        return -1;
    }

    char metadonnee[512];
    size_t taille_meta = snprintf(metadonnee, sizeof(metadonnee), "nom:%s|taille:%ld|modif:%ld|droit:%d\n", nom_fichier, st.st_size, st.st_mtime, st.st_mode);

    size_t taille_totale = taille_meta + st.st_size;
    char *donnee = malloc(taille_totale);
    if (donnee == NULL) {
        perror("[ERREUR] malloc (insertfile)");
        munmap(contenu, st.st_size);
        close(fd);
        close_table(&t);
        return -1;
    }

    memcpy(donnee, metadonnee, taille_meta);
    memcpy(donnee + taille_meta, contenu, st.st_size);

    uint32_t bloc_necessaire = bloc_suffisant_fichier(nom_table, t.table, nom_fichier, t.b);
    if (bloc_necessaire == BLOC_ERREUR) {
        free(donnee);
        munmap(contenu, st.st_size);
        close(fd);
        close_table(&t);
        return -1;
    }

    size_t taille_necessaire = taille_totale + (bloc_necessaire * ESPACE_MAX);
    if (bloc_suffisant(t.table, taille_necessaire, t.b) == BLOC_ERREUR) {
        free(donnee);
        munmap(contenu, st.st_size);
        close(fd);
        close_table(&t);
        return -1;
    }

    uint32_t index = handle_data(nom_table, donnee, taille_totale, &t, donnee, taille_totale, TYPE_FICHIER);
    if (index != BLOC_ERREUR) {
        size_t taille_nom = strlen(nom_fichier) + 1;
        char *nom_donnee = malloc(taille_nom + sizeof(uint32_t));
        if (nom_donnee == NULL) {
            perror("[ERREUR] malloc (insertfile)");
            free(donnee);
            munmap(contenu, st.st_size);
            close(fd);
            close_table(&t);
            return -1;
        }

        memcpy(nom_donnee, nom_fichier, taille_nom);
        memcpy(nom_donnee + taille_nom, &index, sizeof(uint32_t));

        if (handle_data(nom_table, nom_fichier, strlen(nom_fichier), &t, nom_donnee, taille_nom + sizeof(uint32_t), TYPE_NOM_FICHIER) == BLOC_ERREUR) {
            free(nom_donnee);
            return -1;
        }
        
        free(nom_donnee);
    }
    
    free(donnee);
    munmap(contenu, st.st_size);
    close(fd);
    close_table(&t);
    return 0;
}

int lookup_cmd() {

    return 0;
}

int deletehash_cmd(const char *nom_table, const uint32_t hashcode) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE) < 0) {
        return -1;
    }

    if (t.b <= hashcode) {
        fprintf(stderr, "[ERREUR] deletehash: le hashcode depasse la taille de la table.\n");
        close_table(&t);
        return -1;
    }

    if ((t.table[hashcode].premier == hashcode && t.table[hashcode].longueur == BLOC_LIBRE)
        || (t.table[hashcode].premier == BLOC_LIBRE)) {
        close_table(&t);
        return 0;
    }

    uint32_t index = t.table[hashcode].premier;
    uint32_t bloc_actuel = index;
    while (bloc_actuel != FIN_CHAINE) {
        if (t.table[bloc_actuel].type == TYPE_FICHIER) {
            if (strncmp(t.table[bloc_actuel].obj, "nom:", 4) == 0) {
                supprimer_versions_fichier(nom_table, t.table, bloc_actuel, t.b);
            }
        } else if (t.table[bloc_actuel].type == TYPE_NOM_FICHIER) {
            if (t.table[bloc_actuel].precedent == FIN_CHAINE || t.table[t.table[bloc_actuel].precedent].type != TYPE_NOM_FICHIER) {
                supprimer_nom_fichier(t.table, bloc_actuel, t.b);
            }
        }
        
        bloc_actuel = t.table[bloc_actuel].suivant;
    }

    uint32_t dernier_bloc = index;
    while (t.table[dernier_bloc].suivant != FIN_CHAINE) {
        dernier_bloc = t.table[dernier_bloc].suivant;
    }

    uint32_t bloc_supprimer = dernier_bloc;
    while (bloc_supprimer != FIN_CHAINE) {
        uint32_t precedent = t.table[bloc_supprimer].precedent;
        supprimer_bloc(t.table, bloc_supprimer, t.b);
        if (bloc_supprimer == index) {
            break;
        }

        bloc_supprimer = precedent;
    }

    if (deplacer_bloc(nom_table, t.table, t.b) == BLOC_ERREUR) {
        close_table(&t);
        return -1;
    }

    close_table(&t);
    return 0;
}

int delete_cmd(const char *nom_table, const char *chaine) {
    (void)nom_table;
    (void)chaine;
    return 0;
}

int check_cmd(const char *nom_table) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ) < 0) {
        return -1;
    }

    uint32_t *premiere_donnee = recuperer_premiere_donnee(&t);
    if (premiere_donnee == NULL) {
        close_table(&t);
        return -1;
    }

    for (uint32_t i = 0; i < t.b; i++) {
        if (t.table[i].longueur == BLOC_LIBRE) {
            continue;
        }
        
        if (premiere_donnee[i] == 1) {
            continue;
        }

        if (t.table[i].type != TYPE_NOM_FICHIER) {
            char *chaine = recuperer_chaine_complete(t.table, i);
            uint32_t hash = h(nom_table, chaine, t.table[i].longueur, t.b);
            if (hash == BLOC_ERREUR) {
                free(premiere_donnee);
                free(chaine);
                close_table(&t);
                return BLOC_ERREUR;
            }

            free(chaine);

            uint32_t index = t.table[hash].premier;
            int verif = 0;
            while (index != FIN_CHAINE) {
                if (index == i) {
                    verif = 1; 
                    break;
                }

                index = t.table[index].suivant;
            }

            if (verif == 0) {
                fprintf(stderr, "[ERREUR] check: le bloc %u est corrompu, le hash du contenu est différent de premier.\n", i);
                free(premiere_donnee);
                close_table(&t);
                return -1;
            }
        } else {
            char *nom_fichier = t.table[i].obj;
            uint32_t hash_nom_fichier = h(nom_table, nom_fichier, strlen(nom_fichier), t.b);
            if (hash_nom_fichier == BLOC_ERREUR) {
                free(premiere_donnee);
                close_table(&t);
                return BLOC_ERREUR;
            }

            uint32_t index = t.table[hash_nom_fichier].premier;
            int verif = 0;

            while (index != FIN_CHAINE) {
                if (index == i) {
                    verif = 1;
                    break;
                }
                
                index = t.table[index].suivant;
            }

            if (verif == 0) {
                fprintf(stderr, "[ERREUR] check: le bloc %u est corrompu (%s): il n'est pas lié à la boîte aux lettres %u.\n", i, nom_fichier, hash_nom_fichier);
                free(premiere_donnee);
                close_table(&t);
                return -1;
            }

            uint32_t offset = strlen(t.table[i].obj) + 1;
            if (t.table[i].obj[offset - 1] != '\0') {
                fprintf(stderr, "[ERREUR] check: le bloc %u est corrompu: \\0 n'a pas ete trouve.\n", i);
                free(premiere_donnee);
                close_table(&t);
                return -1;
            }

            uint32_t *versions = recuperer_versions_fichier(t.table, i);
            if (versions == NULL) {
                perror("malloc (check)");
                free(premiere_donnee);
                close_table(&t);
                return -1;
            }

            uint32_t nb_elements = (t.table[i].longueur - offset) / sizeof(uint32_t);
            for (uint32_t k = 0; k < nb_elements; k++) {
                if (versions[k] >= t.b) {
                    fprintf(stderr, "[ERREUR] check: le bloc %u est corrompu (%s): l'index pointe vers une zone non accessible.\n", versions[k], nom_fichier);
                    free(versions);
                    free(premiere_donnee);
                    close_table(&t);
                    return -1;
                }

                if (t.table[versions[k]].longueur == BLOC_LIBRE || t.table[versions[k]].type != TYPE_FICHIER) {
                    fprintf(stderr, "[ERREUR] check: le bloc %u est corrompu (%s): l'index ne pointe pas vers le bon fichier.\n", versions[k], nom_fichier);
                    free(versions);
                    free(premiere_donnee);
                    close_table(&t);
                    return -1;
                }

                if (memcmp(t.table[versions[k]].obj + strlen("nom:"), nom_fichier, strlen(nom_fichier)) != 0) {
                    fprintf(stderr, "[ERREUR] check_cmd: le bloc %u est corrompu (%s): le nom du fichier ne correspond pas.\n", versions[k], nom_fichier);
                    free(versions);
                    free(premiere_donnee);
                    close_table(&t);
                    return -1;
                }
            }

            free(versions);
        }
    }

    printf("[INFO] Check terminé: aucune donnée corrompu.\n");
    free(premiere_donnee);
    close_table(&t);
    return 0;
}

int extend_cmd(const char *nom_table) {
    (void)nom_table;
    return 0;
}

int shrink_cmd(const char *nom_table) {
    (void)nom_table;
    return 0;
}

int ajust_cmd(const char *nom_table) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ) < 0) {
        return -1;
    }

    uint32_t bloc_libre = 0;
    for (uint32_t i = 0; i < t.b; i++) {
        if (t.table[i].longueur == BLOC_LIBRE) {
            bloc_libre++;
        }
    }
    
    close_table(&t);
    double taux_remplissage = ((double)(t.b - bloc_libre) / (double)t.b) * 100.0;
    if (taux_remplissage > 70.0) {
        if (extend_cmd(nom_table) < 0) {
            return -1;
        }
    } else if (taux_remplissage < 10.0) {
        if (shrink_cmd(nom_table) < 0) {
            return -1;
        }
    } 

    return 0;
}