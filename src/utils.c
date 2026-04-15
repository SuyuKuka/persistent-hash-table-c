#include "../include/utils.h"

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
    uint32_t *premiere_donnee = calloc(t->b, (sizeof(uint32_t)));
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
                    fprintf(stderr, "[ALERTE] Le bloc %u est corrompu: il pointe vers un bloc inexistant (%u) !\n", i, suivant);
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

void supprimer_bloc(struct hache_entree *table, uint32_t index) {
    table[index].premier = index;
    table[index].suivant = FIN_CHAINE;
    table[index].longueur = BLOC_LIBRE;
    table[index].type = TYPE_VIDE;
    memset(table[index].obj, 0, ESPACE_MAX);
}

uint32_t supprimer_versions_fichier(const char *nom_table, struct hache_entree *table, const char *nom_fichier, const uint32_t hashcode, const uint32_t b) {
    uint32_t hash = h(nom_table, nom_fichier, strlen(nom_fichier), b);
    uint32_t i = hash;
    uint32_t premier_supprimer = 1;
    do {
        if (table[i].premier == hash && table[i].type == TYPE_NOM_FICHIER && memcmp(nom_fichier, table[i].obj, strlen(nom_fichier)) == 0) {
            uint32_t nb_elements = (table[i].longueur - strlen(table[i].obj) + 1) / sizeof(uint32_t);
            if (nb_elements == 1) {
                supprimer_bloc(table, i);
                premier_supprimer = 0;
                break;
            }

            uint32_t *versions = recuperer_versions_fichier(table, i);
            if (versions == NULL) {
                return BLOC_ERREUR;
            }

            uint32_t pos = BLOC_ERREUR;
            for (uint32_t k = 0; k < nb_elements; k++) {
                if (table[versions[k]].premier == hashcode) {
                    pos = k;
                }
            }

            if (pos != BLOC_ERREUR) {
                for (uint32_t k = pos; k < nb_elements - 1; k++) {
                    versions[k] = versions[k + 1]; 
                }
                nb_elements--;
                table[i].longueur -= sizeof(uint32_t);
                premier_supprimer = 0;
            } else {
                fprintf(stderr, "[ERREUR] supprimer_versions_fichier: l'index est corrompu\n");
                free(versions);
                return BLOC_ERREUR;
            }

            free(versions);
        }

        i = (i + 1) % b;
    } while (i != hash);

    return premier_supprimer;
}

void deplacer_bloc(struct hache_entree *table, const uint32_t hashcode, uint32_t b) {
    uint32_t index = hashcode;
    do {
        if (table[index].longueur != BLOC_LIBRE) {
            uint32_t premier = table[index].premier;
            if (table[premier].longueur == BLOC_LIBRE) {
                table[premier].premier = table[index].premier;
                table[premier].suivant = table[index].suivant;
                table[premier].longueur = table[index].longueur;
                table[premier].type = table[index].type;
                memcpy(table[premier].obj, table[index].obj, ESPACE_MAX);

                supprimer_bloc(table, index);
            }
        }

        index = (index + 1) % b;
    } while(index != hashcode);
}