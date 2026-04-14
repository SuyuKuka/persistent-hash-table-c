#include "cmd.h"

int open_table(const char *nom_table, struct table_entree *t, const int flags_open, const int prot_mmap) {
    t->fd = open(nom_table, flags_open);
    if (t->fd < 0) {
        perror("[ERREUR] open");
        return -1;
    }

    t->b = get_b(t->fd);
    t->taille_table = t->b * sizeof(struct hache_entree);
    t->table = mmap(NULL, t->taille_table, prot_mmap, MAP_SHARED, t->fd, 0);
    if (t->table == MAP_FAILED) {
        perror("[ERREUR] mmap");
        close(t->fd);
        return -1;
    }

    return 0;
}

void close_table(struct table_entree *t) {
    munmap(t->table, t->taille_table);
    close(t->fd);
}

uint32_t insert_nom_fichier(struct table_entree *t, const char *donnee, const int premier_bloc) {
    uint32_t dernier_bloc = premier_bloc;

    while (t->table[dernier_bloc].suivant != FIN_CHAINE) {
        t->table[dernier_bloc].longueur += sizeof(uint32_t);
        dernier_bloc = t->table[dernier_bloc].suivant;
    }

    uint32_t longueur = t->table[dernier_bloc].longueur;
    if (longueur + sizeof(uint32_t) <= ESPACE_MAX) {
        memcpy(t->table[dernier_bloc].obj + longueur, donnee, sizeof(uint32_t));
        t->table[dernier_bloc].longueur += sizeof(uint32_t);
        return dernier_bloc;
    }

    t->table[dernier_bloc].longueur += sizeof(uint32_t);
    uint32_t nouveau_bloc = bloc_libre(t->table, t->b, dernier_bloc);
    if (nouveau_bloc == BLOC_ERREUR) {
        uint32_t bloc_annuler = premier_bloc;
        while (t->table[bloc_annuler].suivant != FIN_CHAINE) {
            t->table[bloc_annuler].longueur -= sizeof(uint32_t);
            bloc_annuler = t->table[bloc_annuler].suivant;
        }

        t->table[dernier_bloc].longueur -= 2 * sizeof(uint32_t);
        return BLOC_ERREUR;
    }
    
    t->table[dernier_bloc].suivant = nouveau_bloc;
    
    t->table[nouveau_bloc].premier = premier_bloc;
    t->table[nouveau_bloc].suivant = FIN_CHAINE;
    t->table[nouveau_bloc].type = TYPE_NOM_FICHIER;
    t->table[nouveau_bloc].longueur = sizeof(uint32_t);

    memcpy(t->table[nouveau_bloc].obj, donnee, sizeof(uint32_t));
    return premier_bloc;
}

uint32_t handle_data(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, uint32_t type_donnee) {
    uint32_t bloc = recherche_bloc(nom_table, t->table, cle, taille_cle, type_donnee, t->b);
    if (type_donnee == TYPE_NOM_FICHIER) {
        if (bloc != BLOC_ERREUR) {
            printf("doneee : %s\n", donnee);
            return insert_nom_fichier(t, donnee + strlen(donnee) + 1, bloc);
        }
    } else {
        if (bloc != BLOC_ERREUR) {
            fprintf(stderr, "[ALERTE] handle_data: la donnee existe deja dans la table.\n");
            return BLOC_ERREUR;
        }
    }

    return insertdata(nom_table, cle, taille_cle, t, donnee, taille_donnee, type_donnee);
}

uint32_t insertdata(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, uint32_t type_donnee) {
    size_t offset = 0;
    uint32_t index_premier = BLOC_ERREUR;
    uint32_t index_actuel;
    uint32_t index_precedent = FIN_CHAINE;
    uint32_t index = h(nom_table, cle, taille_cle, t->b);
    if (index == BLOC_ERREUR) {
        return BLOC_ERREUR;
    }

    while (taille_donnee > 0) {
        size_t taille_actuel;
        if (taille_donnee > ESPACE_MAX) {
            taille_actuel = ESPACE_MAX;
        } else {
            taille_actuel = taille_donnee;
        }
        
        index_actuel = bloc_libre(t->table, t->b, index);
        if (index_actuel == BLOC_ERREUR) {
            return BLOC_ERREUR;
        }

        if (index_premier == BLOC_ERREUR) {
            index_premier = index_actuel;
        }

        t->table[index_actuel].premier = index;
        t->table[index_actuel].suivant = FIN_CHAINE;
        t->table[index_actuel].longueur = taille_donnee;
        t->table[index_actuel].type = type_donnee;
        memcpy(t->table[index_actuel].obj, donnee + offset, taille_actuel);
        
        if (index_precedent != FIN_CHAINE) {
            t->table[index_precedent].suivant = index_actuel;
        }

        taille_donnee -= taille_actuel;
        offset += taille_actuel;
        index_precedent = index_actuel;
    }

    return index_premier;
}

uint32_t *recuperer_premiere_donnee(struct table_entree *t) {
    uint32_t *premiere_donnee = malloc(t->b * (sizeof(uint32_t)));
    if (premiere_donnee == NULL) {
        perror("malloc (check)");
        return NULL;
    }

    for (uint32_t i = 0; i < t->b; i++) {
        if (t->table[i].longueur != BLOC_LIBRE) {
            uint32_t suivant = t->table[i].suivant;
            if (suivant != FIN_CHAINE) {
                if (suivant < t->b) {
                    premiere_donnee[suivant] = 1; 
                } else {
                    fprintf(stderr, "[ALERTE] Le bloc %u est corrompu: il pointe vers un bloc inexistant (%ls) !\n", i, premiere_donnee);
                }
            }
        }
    }

    return premiere_donnee;
}

char *recuperer_chaine_complete(struct hache_entree *table, uint32_t index) {
    uint32_t taille = table[index].longueur;
    char *chaine = malloc(taille + 1);
    if (chaine == NULL) {
        perror("malloc (recuperer_chaine_complete)");
        return NULL;
    }

    uint32_t i = index;
    uint32_t offset = 0;
    while (i != FIN_CHAINE && taille > 0) {
        uint32_t taille_chaine = (taille < ESPACE_MAX) ? taille : ESPACE_MAX;
        memcpy(chaine + offset, table[i].obj, taille_chaine);
        taille -= taille_chaine;
        offset += taille_chaine;

        i = table[i].suivant;
    }

    chaine[offset] = '\0';
    return chaine;
}

uint32_t *recuperer_versions_fichier(struct hache_entree *table, uint32_t index) {
    uint32_t offset = strlen(table[index].obj) + 1;
    uint32_t *versions = malloc(table[index].longueur - offset);
    if (versions == NULL) {
        perror("malloc (recuperer_versions_fichier)");
        return NULL;
    }

    uint32_t i = index;
    uint32_t offset_versions = 0;
    while(i != FIN_CHAINE) {
        uint32_t taille_bloc = (table[i].longueur < ESPACE_MAX) ? table[i].longueur : ESPACE_MAX; 
        while (offset + sizeof(uint32_t) <= taille_bloc) {
            memcpy(versions + offset_versions, table[i].obj + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);
            offset_versions++;
        }
        
        offset = 0;
        i = table[i].suivant;
    }

    return versions;
}

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
    for (uint32_t i = 0; i < t.b; i++) {
        if (t.table[i].longueur == BLOC_LIBRE) {
            bloc_libre++;
        }
        
        if (t.table[i].suivant == FIN_CHAINE) {
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

        uint32_t i = handle_data(nom_table, nom_fichier, strlen(nom_fichier), &t, nom_donnee, taille_nom + sizeof(uint32_t), TYPE_NOM_FICHIER);
        free(nom_donnee);
        if (i == BLOC_ERREUR) {
            return -1;
        }
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

int deletehash_cmd() {
    
    return 0;
}

int delete_cmd(const char *nom_table, const char *chaine) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE) < 0) {
        return -1;
    }

    size_t taille_chaine = strlen(chaine);
    int bloc = -1;

    for (uint32_t i = 0; i < t.b; i++) {
        if (t.table[i].longueur == BLOC_LIBRE) {
            continue;
        }

        if (t.table[i].longueur != taille_chaine) {
            continue;
        }

        size_t offset = 0;
        uint32_t courant = i;
        int correspond = 1;

        while (1) {
            size_t reste = taille_chaine - offset;
            size_t a_comparer;

            if (reste > ESPACE_MAX) {
                a_comparer = ESPACE_MAX;
            } else {
                a_comparer = reste;
            }

            if (memcmp(t.table[courant].obj, chaine + offset, a_comparer) != 0) {
                correspond = 0;
                break;
            }

            offset += a_comparer;

            if (offset >= taille_chaine) {
                break;
            }

            if (t.table[courant].suivant == FIN_CHAINE) {
                correspond = 0;
                break;
            }

            courant = t.table[courant].suivant;
        }

        if (correspond) {
            bloc = i;
            break;
        }
    }

    if (bloc < 0) {
        fprintf(stderr, "[ALERTE] delete: la chaine est introuvable.\n");
        close_table(&t);
        return -1;
    }

    uint32_t courant = (uint32_t)bloc;

    while (1) {
        uint32_t suivant = t.table[courant].suivant;

        t.table[courant].premier = courant;
        t.table[courant].suivant = FIN_CHAINE;
        t.table[courant].longueur = BLOC_LIBRE;
        memset(t.table[courant].obj, 0, ESPACE_MAX);

        if (suivant == FIN_CHAINE) {
            break;
        }

        courant = suivant;
    }

    close_table(&t);
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
            uint32_t hash = h(nom_table, chaine, strlen(chaine), t.b);
            uint32_t j = i;
            while (j != FIN_CHAINE) {
                if (hash != t.table[j].premier) {
                    fprintf(stderr, "[ALERTE] Le bloc %u est corrompu, le hash du contenu est différent de premier.\n", j);
                    free(chaine);
                    free(premiere_donnee);
                    return -1;
                } 

                j = t.table[j].suivant;
            }

            free(chaine);
        } else {
            char *nom_fichier = t.table[i].obj;
            uint32_t hash_nom_fichier = h(nom_table, nom_fichier, strlen(nom_fichier), t.b);
            if (hash_nom_fichier != t.table[i].premier) {
                fprintf(stderr, "[ALERTE] Le bloc %u est corrompu (%s): le hash du contenu est différent de premier.\n", i, nom_fichier);
                return -1;
            }

            uint32_t offset = strlen(t.table[i].obj) + 1;
            if (t.table[i].obj[offset - 1] != '\0') {
                fprintf(stderr, "[ALERTE] Le bloc %u est corrompu: \\0 n'a pas ete trouve.\n", i);
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
                if (versions[k] > t.b) {
                    fprintf(stderr, "[ALERTE] Le bloc %u est corrompu (%s): l'index pointe vers une zone non accessible.\n", k, nom_fichier);
                    free(versions);
                    free(premiere_donnee);
                    close_table(&t);
                    return -1;
                }

                if (t.table[versions[k]].longueur == BLOC_LIBRE || t.table[versions[k]].type != TYPE_FICHIER) {
                    fprintf(stderr, "[ALERTE] Le bloc %u est corrompu (%s): l'index ne pointe pas vers le bon fichier.\n", k, nom_fichier);
                    free(versions);
                    free(premiere_donnee);
                    close_table(&t);
                    return -1;
                }

                if (memcmp(t.table[versions[k]].obj + strlen("nom:"), nom_fichier, strlen(nom_fichier)) != 0) {
                    fprintf(stderr, "[ALERTE] Le bloc %u est corrompu (%s): le nom du fichier ne correspond pas.\n", k, nom_fichier);
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

int extend_cmd() {
    
    return 0;
}

int shrink_cmd() {

    return 0;
}

int ajust_cmd(){
    
    return 0;
}