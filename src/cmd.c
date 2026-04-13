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

long insert_nom_fichier(struct table_entree *t, const char *donnee, const int premier_bloc) {
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
        return -1;
    }
    
    t->table[dernier_bloc].suivant = nouveau_bloc;
    
    t->table[nouveau_bloc].premier = premier_bloc;
    t->table[nouveau_bloc].suivant = FIN_CHAINE;
    t->table[nouveau_bloc].type = TYPE_NOM_FICHIER;
    t->table[nouveau_bloc].longueur = sizeof(uint32_t);

    memcpy(t->table[nouveau_bloc].obj, donnee, sizeof(uint32_t));
    return dernier_bloc;
}

long handle_data(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, uint32_t type_donnee) {
    int bloc = recherche_bloc(nom_table, t->table, cle, taille_cle, type_donnee, t->b);
    if (type_donnee == TYPE_NOM_FICHIER) {
        if (bloc >= 0) {
            return insert_nom_fichier(t, donnee, bloc);
        }
    } else {
        if (bloc >= 0) {
            fprintf(stderr, "[ALERTE] handle_data: la donnee existe deja dans la table.\n");
            return -1;
        }
    }

    return insertdata(nom_table, cle, taille_cle, t, donnee, taille_donnee, type_donnee);
}

long insertdata(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, uint32_t type_donnee) {
    size_t offset = 0;
    uint32_t index_actuel;
    uint32_t index_precedent = FIN_CHAINE;
    uint32_t index = h(nom_table, cle, taille_cle, t->b);
    while (taille_donnee > 0) {
        size_t taille_actuel;
        if (taille_donnee > ESPACE_MAX) {
            taille_actuel = ESPACE_MAX;
        } else {
            taille_actuel = taille_donnee;
        }
        
        index_actuel = bloc_libre(t->table, t->b, index);
        if (index_actuel == BLOC_ERREUR) {
            return -1;
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

    return index;
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
    if (bloc_suffisant(t.table, taille_chaine, t.b) < 0) {
        close_table(&t);
        return -1;
    }

    if (handle_data(nom_table, chaine, taille_chaine, &t, chaine, taille_chaine, TYPE_CHAINE) < 0) {
        close_table(&t);
        return -1;
    }

    close_table(&t);
    return 0;
}

void insertfile_cmd(const char *nom_table, const char *nom_fichier) {
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
    if (bloc_suffisant(t.table, taille_necessaire, t.b) < 0) {
        free(donnee);
        munmap(contenu, st.st_size);
        close(fd);
        close_table(&t);
        return -1;
    }

    long index = handle_data(nom_table, donnee, taille_totale, &t, donnee, taille_totale, TYPE_FICHIER);
    if (index >= 0) {
        uint32_t i = (uint32_t)index;
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
        memcpy(nom_donnee + taille_nom, &i, sizeof(uint32_t));

        handle_data(nom_table, nom_fichier, strlen(nom_fichier), &t, nom_donnee, taille_nom + sizeof(uint32_t), TYPE_NOM_FICHIER);
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