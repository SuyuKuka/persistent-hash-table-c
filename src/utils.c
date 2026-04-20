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

    while (t->table[dernier_bloc].suivant != FIN_CHAINE && t->table[dernier_bloc].longueur > ESPACE_MAX) {
        dernier_bloc = t->table[dernier_bloc].suivant;
    }

    uint32_t longueur = t->table[dernier_bloc].longueur;
    if (longueur + sizeof(uint32_t) <= ESPACE_MAX) {
        memcpy(t->table[dernier_bloc].obj + longueur, donnee, sizeof(uint32_t));

        uint32_t longueur_maj = premier_bloc;
        while (longueur_maj != dernier_bloc) {
            t->table[longueur_maj].longueur += sizeof(uint32_t);
            longueur_maj = t->table[longueur_maj].suivant;
        }

        t->table[dernier_bloc].longueur += sizeof(uint32_t);
        return 0;
    }

    uint32_t nouveau_bloc = bloc_libre(t->table, t->b, dernier_bloc);
    if (nouveau_bloc == BLOC_ERREUR) {
        return BLOC_ERREUR;
    }

    t->table[nouveau_bloc].premier = BLOC_LIBRE;
    t->table[nouveau_bloc].precedent = dernier_bloc;
    t->table[nouveau_bloc].suivant = t->table[dernier_bloc].suivant;
    t->table[nouveau_bloc].type = TYPE_NOM_FICHIER;
    t->table[nouveau_bloc].longueur = sizeof(uint32_t);
    memcpy(t->table[nouveau_bloc].obj, donnee, sizeof(uint32_t));
    
    t->table[dernier_bloc].suivant = nouveau_bloc;
    if (t->table[nouveau_bloc].suivant != FIN_CHAINE) {
        t->table[t->table[nouveau_bloc].suivant].precedent = nouveau_bloc;
    }

    uint32_t longueur_maj = premier_bloc;
    while (longueur_maj != dernier_bloc) {
        t->table[longueur_maj].longueur += sizeof(uint32_t);
        longueur_maj = t->table[longueur_maj].suivant;
    }

    t->table[dernier_bloc].longueur += sizeof(uint32_t);
    return 0;
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
    uint32_t index = h(nom_table, cle, taille_cle, t->b);
    if (index == BLOC_ERREUR) {
        return BLOC_ERREUR;
    }

    uint32_t index_actuel = index;
    uint32_t index_premiere_donnee = BLOC_LIBRE;
    uint32_t index_precedent = FIN_CHAINE;
    while (taille_donnee > 0) {
        size_t taille_actuel;
        if (taille_donnee > ESPACE_MAX) {
            taille_actuel = ESPACE_MAX;
        } else {
            taille_actuel = taille_donnee;
        }
        
        index_actuel = bloc_libre(t->table, t->b, index_actuel);
        if (index_actuel == BLOC_ERREUR) {
            return BLOC_ERREUR;
        }

        if (index_premiere_donnee == BLOC_LIBRE) {
            if ((t->table[index].premier == BLOC_LIBRE) || (t->table[index].premier == index && t->table[index].longueur == BLOC_LIBRE)) {
                t->table[index].premier = index_actuel;
                index_precedent = FIN_CHAINE;

                if (index_actuel != index && t->table[index_actuel].premier == index_actuel) {
                    t->table[index_actuel].premier = BLOC_LIBRE;
                }
            } else {
                uint32_t dernier = t->table[index].premier;
                while (t->table[dernier].suivant != FIN_CHAINE) {
                    dernier = t->table[dernier].suivant;   
                }

                index_precedent = dernier;
                t->table[index_actuel].premier = BLOC_LIBRE;
            }

            index_premiere_donnee = index_actuel;
        } else {
            t->table[index_actuel].premier = BLOC_LIBRE;
        }
        
        t->table[index_actuel].precedent = index_precedent;
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

    return index_premiere_donnee;
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
                    if (t->table[i].longueur > ESPACE_MAX) {
                        premiere_donnee[suivant] = 1; 
                    }
                } else {
                    fprintf(stderr, "[ERREUR] recuperer_premiere_donnee: le bloc %u est corrompu: il pointe vers un bloc inexistant (%u) !\n", i, suivant);
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

        if (table[i].longueur <= ESPACE_MAX) {
            break; 
        }
        
        offset = 0;
        i = table[i].suivant;
    }

    return versions;
}

void mettre_a_jour_versions_fichiers(struct hache_entree *table, uint32_t *versions, uint32_t nb_elements, uint32_t i) {
    uint32_t index = i;
    uint32_t offset = strlen(table[index].obj) + 1;
    uint32_t index_version = 0;

    while (index != FIN_CHAINE && index_version < nb_elements) {
        uint32_t espace_dispo = ESPACE_MAX - offset;
        while (espace_dispo >= sizeof(uint32_t) && index_version < nb_elements) {
            memcpy(table[index].obj + offset, &versions[index_version], sizeof(uint32_t));
            offset += sizeof(uint32_t);
            espace_dispo -= sizeof(uint32_t);
            index_version++;
        }
        
        index = table[index].suivant;
        offset = 0;
    }
}

void supprimer_bloc(struct hache_entree *table, uint32_t index, const uint32_t b) {
    uint32_t precedent = table[index].precedent;
    uint32_t suivant = table[index].suivant;
    uint32_t premier = table[index].premier;

    if (precedent != FIN_CHAINE) {
        table[precedent].suivant = suivant;
    }

    if (suivant != FIN_CHAINE) {
        table[suivant].precedent = precedent;
    }

    if (precedent == FIN_CHAINE) { 
        uint32_t hash_origine = index;
        for (uint32_t h = 0; h < b; h++) {
            if (table[h].premier == index) {
                hash_origine = h;
                break;
            }
        }

        if (suivant == FIN_CHAINE) {
            if (table[hash_origine].type != TYPE_VIDE && hash_origine != index) {
                 table[hash_origine].premier = BLOC_LIBRE;
            } else {
                 table[hash_origine].premier = hash_origine;
            }
        } else {
            table[hash_origine].premier = suivant;
        }
    }

    if (premier == index || premier == BLOC_LIBRE) {
        table[index].premier = index;
    }

    table[index].precedent = FIN_CHAINE;
    table[index].suivant = FIN_CHAINE;
    table[index].longueur = BLOC_LIBRE;
    table[index].type = TYPE_VIDE;
    memset(table[index].obj, 0, ESPACE_MAX);
}

void supprimer_par_derriere(struct hache_entree *table, uint32_t index, const uint32_t b) {
    uint32_t dernier_bloc = index;
    while (table[dernier_bloc].suivant != FIN_CHAINE && table[dernier_bloc].longueur > ESPACE_MAX) {
        dernier_bloc = table[dernier_bloc].suivant;
    }

    uint32_t bloc_supprimer = dernier_bloc;
    while (bloc_supprimer != FIN_CHAINE) {
        uint32_t precedent = table[bloc_supprimer].precedent;
        supprimer_bloc(table, bloc_supprimer, b);
        if (bloc_supprimer == index) {
            break;
        }

        bloc_supprimer = precedent;
    }
}

uint32_t supprimer_versions_fichier(const char *nom_table, struct hache_entree *table, uint32_t index, uint32_t b) {
    char nom_fichier[256];
    if (sscanf(table[index].obj, "nom:%255[^|]", nom_fichier) != 1) {
        fprintf(stderr, "[ERREUR] deletehash: fichier corrompu (nom dans metadonnee).\n");
        return BLOC_ERREUR;
    }

    uint32_t hash_nom_fichier = h(nom_table, nom_fichier, strlen(nom_fichier), b);
    uint32_t index_nom_fichier = table[hash_nom_fichier].premier;
    while (index_nom_fichier != FIN_CHAINE) {
        if (table[index_nom_fichier].type == TYPE_NOM_FICHIER && memcmp(nom_fichier, table[index_nom_fichier].obj, strlen(nom_fichier)) == 0) {
            uint32_t nb_elements = (table[index_nom_fichier].longueur - (strlen(table[index_nom_fichier].obj) + 1)) / sizeof(uint32_t);
            uint32_t *versions = recuperer_versions_fichier(table, index_nom_fichier);
            if (versions == NULL) {
                return BLOC_ERREUR;
            }

            uint32_t pos = BLOC_ERREUR;
            for (uint32_t k = 0; k < nb_elements; k++) {
                if (versions[k] == index) {
                    pos = k;
                    break;
                }
            }

            if (pos != BLOC_ERREUR) {
                if (nb_elements == 1) {
                    supprimer_par_derriere(table, index_nom_fichier, b);
                } else {
                    for (uint32_t k = pos; k < nb_elements - 1; k++) {
                        versions[k] = versions[k + 1]; 
                    }
                    nb_elements--;

                    mettre_a_jour_versions_fichiers(table, versions, nb_elements, index_nom_fichier);

                    uint32_t maj = index_nom_fichier;
                    while (maj != FIN_CHAINE) {
                        table[maj].longueur -= sizeof(uint32_t);
                        if (table[maj].longueur + sizeof(uint32_t) <= ESPACE_MAX) break;
                        maj = table[maj].suivant;
                    }
                }
            }

            free(versions);
            return 0;
        }

        while (index_nom_fichier != FIN_CHAINE && table[index_nom_fichier].longueur > ESPACE_MAX) {
            index_nom_fichier = table[index_nom_fichier].suivant;
        }

        if (index_nom_fichier != FIN_CHAINE) {
            index_nom_fichier = table[index_nom_fichier].suivant;
        }
    }

    fprintf(stderr, "[ERREUR] supprimer_fichier: bloc %s n'a pas ete trouve.", nom_fichier);
    return BLOC_ERREUR;
}

uint32_t supprimer_nom_fichier(struct hache_entree *table, uint32_t index, const uint32_t b) {
    uint32_t nb_elements = (table[index].longueur - (strlen(table[index].obj) + 1)) / sizeof(uint32_t);
    uint32_t *versions = recuperer_versions_fichier(table, index);
    if (versions == NULL) {
        return BLOC_ERREUR;
    }

    for (uint32_t k = 0; k < nb_elements; k++) {
        uint32_t i = versions[k];
        supprimer_par_derriere(table, i, b);
    }

    free(versions);
    return 0;
}

uint32_t deplacer_bloc(const char *nom_table, struct hache_entree *table, const uint32_t b) {
    for (uint32_t i = 0; i < b; i++) {
        if (table[i].longueur == BLOC_LIBRE && table[i].premier != i && table[i].premier != BLOC_LIBRE) {
            uint32_t squatter = table[i].premier;
            table[i].premier = i;
            table[i].precedent = FIN_CHAINE;
            table[i].suivant = table[squatter].suivant;
            table[i].longueur = table[squatter].longueur;
            table[i].type = table[squatter].type;
            memcpy(table[i].obj, table[squatter].obj, ESPACE_MAX);

            if (table[i].suivant != FIN_CHAINE) {
                table[table[i].suivant].precedent = i;
            }

            if (table[i].type == TYPE_FICHIER) {
                char nom_fichier[256];
                if (sscanf(table[i].obj, "nom:%255[^|]", nom_fichier) != 1) {
                    fprintf(stderr, "[ERREUR] deplacer_bloc: fichier corrompu (nom dans metadonnee).\n");
                    return BLOC_ERREUR;
                }

                uint32_t hash = h(nom_table, nom_fichier, strlen(nom_fichier), b);
                uint32_t index = table[hash].premier;
                while (index != FIN_CHAINE) {
                    if (table[index].type == TYPE_NOM_FICHIER && memcmp(nom_fichier, table[index].obj, strlen(nom_fichier)) == 0) {
                        uint32_t *versions = recuperer_versions_fichier(table, index);
                        if (versions == NULL) {
                            return -1;
                        }

                        uint32_t modifier = 1;
                        uint32_t nb_elements = (table[index].longueur - (strlen(table[index].obj) + 1)) / sizeof(uint32_t);
                        for (uint32_t k = 0; k < nb_elements; k++) {
                            if (versions[k] == squatter) {
                                versions[k] = i;
                                modifier = 0;
                            }
                        }

                        if (modifier == 0) {
                            mettre_a_jour_versions_fichiers(table, versions, nb_elements, index);
                        } else {
                            fprintf(stderr, "[ERREUR] deplacer_bloc: fichier non trouver\n");
                        }
                        
                        free(versions);
                        break;
                    }

                    while (index != FIN_CHAINE && table[index].longueur > ESPACE_MAX) {
                        index = table[index].suivant;
                    }

                    if (index != FIN_CHAINE) {
                        index = table[index].suivant;
                    }
                }
            }

            if (table[squatter].premier == BLOC_LIBRE) {
                table[squatter].premier = squatter;
            }
            
            table[squatter].precedent = FIN_CHAINE;
            table[squatter].suivant = FIN_CHAINE;
            table[squatter].longueur = BLOC_LIBRE;
            table[squatter].type = TYPE_VIDE;
            memset(table[squatter].obj, 0, ESPACE_MAX);
        }
    }

    return 0;
}