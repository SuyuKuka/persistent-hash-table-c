#include "../include/utils.h"

int open_table(const char *nom_table, struct table_entree *t, const int flags_open, const int prot_mmap, const long slowns) {
    t->fd = open(nom_table, flags_open);
    if (t->fd < 0) {
        perror("[ERREUR] open");
        return -1;
    }

    t->b = get_b(t->fd);
    t->table = mmap(NULL, t->b * sizeof(struct hache_entree), prot_mmap, MAP_SHARED, t->fd, 0);
    t->slowns = slowns;
    if (t->table == MAP_FAILED) {
        perror("[ERREUR] mmap");
        close(t->fd);
        return -1;
    }

    return 0;
}

void close_table(struct table_entree *t) {
    munmap(t->table, t->b * sizeof(struct hache_entree));
    close(t->fd);
}

uint32_t insert_nom_fichier(struct table_entree *t, const char *donnee, const int premier_bloc) {
    uint32_t dernier_bloc = premier_bloc;

    verrouiller_bloc(t->fd, dernier_bloc, F_RDLCK);
    uint32_t suivant = t->table[dernier_bloc].suivant;
    uint32_t longueur = t->table[dernier_bloc].longueur;
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, dernier_bloc);
    while (suivant != FIN_CHAINE && longueur > ESPACE_MAX) {
        dernier_bloc = suivant;
        
        verrouiller_bloc(t->fd, dernier_bloc, F_RDLCK);
        suivant = t->table[dernier_bloc].suivant;
        longueur = t->table[dernier_bloc].longueur;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, dernier_bloc);
    }

    if (longueur + sizeof(uint32_t) <= ESPACE_MAX) {
        verrouiller_bloc(t->fd, dernier_bloc, F_WRLCK);
        memcpy(t->table[dernier_bloc].obj + longueur, donnee, sizeof(uint32_t));
        t->table[dernier_bloc].longueur += sizeof(uint32_t);
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, dernier_bloc);
    } else {
        uint32_t nouveau_bloc = bloc_libre(t, dernier_bloc);
        if (nouveau_bloc == BLOC_ERREUR) {
            return BLOC_ERREUR;
        }

        verrouiller_bloc(t->fd, dernier_bloc, F_WRLCK);
        uint32_t ancien_suivant = t->table[dernier_bloc].suivant;
        t->table[dernier_bloc].suivant = nouveau_bloc;
        t->table[dernier_bloc].longueur += sizeof(uint32_t);
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, dernier_bloc);

        verrouiller_bloc(t->fd, nouveau_bloc, F_WRLCK);
        t->table[nouveau_bloc].premier = BLOC_LIBRE;
        t->table[nouveau_bloc].precedent = dernier_bloc;
        t->table[nouveau_bloc].suivant = t->table[dernier_bloc].suivant;
        t->table[nouveau_bloc].type = TYPE_NOM_FICHIER;
        t->table[nouveau_bloc].longueur = sizeof(uint32_t);
        memcpy(t->table[nouveau_bloc].obj, donnee, sizeof(uint32_t));
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, nouveau_bloc);
        
        if (ancien_suivant != FIN_CHAINE) {
            verrouiller_bloc(t->fd, ancien_suivant, F_WRLCK);
            t->table[ancien_suivant].precedent = nouveau_bloc;
            attendre(t->slowns);
            deverrouiller_bloc(t->fd, ancien_suivant);
        }
    }

    uint32_t longueur_maj = premier_bloc;
    while (longueur_maj != dernier_bloc) {
        verrouiller_bloc(t->fd, longueur_maj, F_WRLCK);
        t->table[longueur_maj].longueur += sizeof(uint32_t);
        suivant = t->table[longueur_maj].suivant;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, longueur_maj);

        longueur_maj = suivant;
    }

    return 0;
}

uint32_t handle_data(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee) {
    uint32_t bloc = recherche_bloc(nom_table, t, cle, taille_cle, type_donnee);
    if (bloc != BLOC_ERREUR) {
        if (type_donnee == TYPE_NOM_FICHIER) {
            uint32_t insert = insert_nom_fichier(t, donnee + strlen(donnee) + 1, bloc);
            return insert;
        } else {
            fprintf(stderr, "[ALERTE] handle_data: la donnee existe deja dans la table.\n");
            return BLOC_ERREUR;
        }
    }

    return insertdata(nom_table, cle, taille_cle, t, donnee, taille_donnee, type_donnee);
}

uint32_t insertdata(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee) {
    size_t offset = 0;
    uint32_t index = h(nom_table, cle, taille_cle, t->b);
    if (index == BLOC_ERREUR) {
        return BLOC_ERREUR;
    }

    uint32_t index_actuel = index;
    uint32_t index_premiere_donnee = BLOC_LIBRE;
    uint32_t index_precedent = FIN_CHAINE;
    while (taille_donnee > 0) {
        size_t taille_actuel = (taille_donnee > ESPACE_MAX) ? ESPACE_MAX : taille_donnee;
        index_actuel = bloc_libre(t, index_actuel);
        if (index_actuel == BLOC_ERREUR) {
            return BLOC_ERREUR;
        }

        if (index_premiere_donnee == BLOC_LIBRE) {
            verrouiller_bloc(t->fd, index, F_WRLCK);
            if ((t->table[index].premier == BLOC_LIBRE) || (t->table[index].premier == index && t->table[index].longueur == 0)) {
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

            attendre(t->slowns);
            deverrouiller_bloc(t->fd, index);
            index_premiere_donnee = index_actuel;
        }   

        verrouiller_bloc(t->fd, index_actuel, F_WRLCK);
        if (index_premiere_donnee != BLOC_LIBRE && index_actuel != index) {
            t->table[index_actuel].premier = BLOC_LIBRE;
        }
        
        t->table[index_actuel].precedent = index_precedent;
        t->table[index_actuel].suivant = FIN_CHAINE;
        t->table[index_actuel].longueur = taille_donnee;
        t->table[index_actuel].type = type_donnee;
        memcpy(t->table[index_actuel].obj, donnee + offset, taille_actuel);
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, index_actuel);
        
        if (index_precedent != FIN_CHAINE) {
            verrouiller_bloc(t->fd, index_precedent, F_WRLCK);
            t->table[index_precedent].suivant = index_actuel;
            attendre(t->slowns);
            deverrouiller_bloc(t->fd, index_precedent);
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
        verrouiller_bloc(t->fd, i, F_RDLCK);
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

        attendre(t->slowns);
        deverrouiller_bloc(t->fd, i);
    }

    return premiere_donnee;
}

char *recuperer_chaine_complete(struct table_entree *t, uint32_t index) {
    verrouiller_bloc(t->fd, index, F_RDLCK);
    uint32_t taille = t->table[index].longueur;
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, index);

    char *chaine = malloc(taille + 1);
    if (chaine == NULL) {
        perror("malloc (recuperer_chaine_complete)");
        return NULL;
    }

    uint32_t i = index;
    uint32_t offset = 0;
    while (i != FIN_CHAINE && taille > 0) {
        verrouiller_bloc(t->fd, i, F_RDLCK);
        uint32_t suivant = t->table[i].suivant;
        uint32_t taille_chaine = (taille < ESPACE_MAX) ? taille : ESPACE_MAX;
        memcpy(chaine + offset, t->table[i].obj, taille_chaine);
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, i);

        taille -= taille_chaine;
        offset += taille_chaine;
        i = suivant;
    }

    chaine[offset] = '\0';
    return chaine;
}

uint32_t *recuperer_versions_fichier(struct table_entree *t, uint32_t index) {
    verrouiller_bloc(t->fd, index, F_RDLCK);
    uint32_t offset = strlen(t->table[index].obj) + 1;
    uint32_t *versions = malloc(t->table[index].longueur - offset);
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, index);
    if (versions == NULL) {
        perror("malloc (recuperer_versions_fichier)");
        return NULL;
    }

    uint32_t i = index;
    uint32_t offset_versions = 0;
    while(i != FIN_CHAINE) {
        verrouiller_bloc(t->fd, i, F_RDLCK);
        uint32_t longueur_bloc = t->table[i].longueur;
        uint32_t taille_bloc = (longueur_bloc < ESPACE_MAX) ? longueur_bloc : ESPACE_MAX; 
        while (offset + sizeof(uint32_t) <= taille_bloc) {
            memcpy(versions + offset_versions, t->table[i].obj + offset, sizeof(uint32_t));
            offset += sizeof(uint32_t);
            offset_versions++;
        }

        uint32_t suivant = t->table[i].suivant;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, i);

        if (longueur_bloc <= ESPACE_MAX) {
            break; 
        }
        
        offset = 0;
        i = suivant;
    }

    return versions;
}

void mettre_a_jour_versions_fichiers(struct table_entree *t, uint32_t *versions, uint32_t nb_elements, uint32_t i) {
    uint32_t index = i;
    uint32_t index_version = 0;
    int est_premier_bloc = 0;

    while (index != FIN_CHAINE && index_version < nb_elements) {
        verrouiller_bloc(t->fd, index, F_WRLCK);
        uint32_t offset = 0;
        if (est_premier_bloc == 0) {
            offset = strlen(t->table[index].obj) + 1;
            est_premier_bloc = 1;
        }

        uint32_t espace_dispo = ESPACE_MAX - offset;
        while (espace_dispo >= sizeof(uint32_t) && index_version < nb_elements) {
            memcpy(t->table[index].obj + offset, &versions[index_version], sizeof(uint32_t));
            offset += sizeof(uint32_t);
            espace_dispo -= sizeof(uint32_t);
            index_version++;
        }

        uint32_t suivant = t->table[index].suivant;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, index);

        index = suivant;
    }
}

void supprimer_bloc(struct table_entree *t, uint32_t index) {
    verrouiller_bloc(t->fd, index, F_WRLCK);
    uint32_t precedent = t->table[index].precedent;
    uint32_t suivant = t->table[index].suivant;
    uint32_t premier = t->table[index].premier;
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, index);

    if (precedent != FIN_CHAINE) {
        verrouiller_bloc(t->fd, precedent, F_WRLCK);
        t->table[precedent].suivant = suivant;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, precedent);
    }

    if (suivant != FIN_CHAINE) {
        verrouiller_bloc(t->fd, suivant, F_WRLCK);
        t->table[suivant].precedent = precedent;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, suivant);
    }

    if (precedent == FIN_CHAINE) { 
        uint32_t hash_origine = index;
        for (uint32_t h = 0; h < t->b; h++) {
            if (t->table[h].premier == index) {
                hash_origine = h;
                break;
            }
        }

        verrouiller_bloc(t->fd, hash_origine, F_WRLCK);
        if (suivant == FIN_CHAINE) {
            if (t->table[hash_origine].type != TYPE_VIDE && hash_origine != index) {
                t->table[hash_origine].premier = BLOC_LIBRE;
            } else {
                t->table[hash_origine].premier = hash_origine;
            }
        } else {
            t->table[hash_origine].premier = suivant;
        }

        attendre(t->slowns);
        deverrouiller_bloc(t->fd, hash_origine);
    }

    verrouiller_bloc(t->fd, index, F_WRLCK);
    if (precedent == FIN_CHAINE && suivant != FIN_CHAINE && premier == index) {
        t->table[index].premier = suivant;
    } else if (premier == index || premier == BLOC_LIBRE) {
        t->table[index].premier = index;
    }

    t->table[index].precedent = FIN_CHAINE;
    t->table[index].suivant = FIN_CHAINE;
    t->table[index].longueur = BLOC_LIBRE;
    t->table[index].type = TYPE_VIDE;
    memset(t->table[index].obj, 0, ESPACE_MAX);
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, index);
}

void supprimer_par_derriere(struct table_entree *t, uint32_t index, int tout_supprimer) {
    uint32_t dernier_bloc = index;
    while (1) {
        verrouiller_bloc(t->fd, dernier_bloc, F_RDLCK);
        uint32_t suivant = t->table[dernier_bloc].suivant;
        uint32_t longueur = t->table[dernier_bloc].longueur;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, dernier_bloc);

        if (suivant != FIN_CHAINE && (tout_supprimer == 0 || longueur > ESPACE_MAX)) {
            dernier_bloc = suivant;
        } else {
            break;
        }
    }

    uint32_t bloc_supprimer = dernier_bloc;
    while (bloc_supprimer != FIN_CHAINE) {
        verrouiller_bloc(t->fd, bloc_supprimer, F_RDLCK);
        uint32_t precedent = t->table[bloc_supprimer].precedent;
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, bloc_supprimer);

        supprimer_bloc(t, bloc_supprimer);
        if (bloc_supprimer == index) {
            break;
        }

        bloc_supprimer = precedent;
    }
}

uint32_t supprimer_versions_fichier(const char *nom_table, struct table_entree *t, uint32_t index) {
    char nom_fichier[256];
    verrouiller_bloc(t->fd, index, F_RDLCK);
    if (sscanf(t->table[index].obj, "nom:%255[^|]", nom_fichier) != 1) {
        fprintf(stderr, "[ERREUR] deletehash: fichier corrompu (nom dans metadonnee).\n");
        return BLOC_ERREUR;
    }
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, index);

    uint32_t hash_nom_fichier = h(nom_table, nom_fichier, strlen(nom_fichier), t->b);
    verrouiller_bloc(t->fd, hash_nom_fichier, F_RDLCK);
    uint32_t index_nom_fichier = t->table[hash_nom_fichier].premier;
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, hash_nom_fichier);
    while (index_nom_fichier != FIN_CHAINE) {
        verrouiller_bloc(t->fd, index_nom_fichier, F_RDLCK);
        uint32_t type_actuel = t->table[index_nom_fichier].type;
        uint32_t longueur_actuelle = t->table[index_nom_fichier].longueur;
        uint32_t suivant_actuel = t->table[index_nom_fichier].suivant;
        int match = (type_actuel == TYPE_NOM_FICHIER && memcmp(nom_fichier, t->table[index_nom_fichier].obj, strlen(nom_fichier)) == 0);
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, index_nom_fichier);

        if (match) {
            uint32_t nb_elements = (t->table[index_nom_fichier].longueur - (strlen(t->table[index_nom_fichier].obj) + 1)) / sizeof(uint32_t);
            uint32_t *versions = recuperer_versions_fichier(t, index_nom_fichier);
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
                    supprimer_par_derriere(t, index_nom_fichier, 1);
                } else {
                    for (uint32_t k = pos; k < nb_elements - 1; k++) {
                        versions[k] = versions[k + 1]; 
                    }
                    nb_elements--;

                    mettre_a_jour_versions_fichiers(t, versions, nb_elements, index_nom_fichier);

                    uint32_t maj = index_nom_fichier;
                    while (maj != FIN_CHAINE) {
                        verrouiller_bloc(t->fd, maj, F_WRLCK);
                        t->table[maj].longueur -= sizeof(uint32_t);
                        uint32_t longueur_maj = t->table[maj].longueur;
                        uint32_t suivant_maj = t->table[maj].suivant;
                        attendre(t->slowns);
                        deverrouiller_bloc(t->fd, maj);

                        if (longueur_maj + sizeof(uint32_t) <= ESPACE_MAX) break;
                        maj = suivant_maj;
                    }
                }
            }

            free(versions);
            return 0;
        }

        while (suivant_actuel != FIN_CHAINE && longueur_actuelle > ESPACE_MAX) {
            index_nom_fichier = suivant_actuel;
            
            verrouiller_bloc(t->fd, index_nom_fichier, F_RDLCK);
            longueur_actuelle = t->table[index_nom_fichier].longueur;
            suivant_actuel = t->table[index_nom_fichier].suivant;
            attendre(t->slowns);
            deverrouiller_bloc(t->fd, index_nom_fichier);
        }

        index_nom_fichier = t->table[index_nom_fichier].suivant;
    }

    fprintf(stderr, "[ERREUR] supprimer_fichier: bloc %s n'a pas ete trouve.", nom_fichier);
    return BLOC_ERREUR;
}

uint32_t supprimer_nom_fichier(struct table_entree *t, uint32_t index) {
    verrouiller_bloc(t->fd, index, F_RDLCK);
    uint32_t longueur_actuelle = t->table[index].longueur;
    size_t taille_nom = strlen(t->table[index].obj);
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, index);

    uint32_t nb_elements = (longueur_actuelle - (taille_nom + 1)) / sizeof(uint32_t);
    uint32_t *versions = recuperer_versions_fichier(t, index);
    if (versions == NULL) {
        return BLOC_ERREUR;
    }

    for (uint32_t k = 0; k < nb_elements; k++) {
        uint32_t i = versions[k];
        supprimer_par_derriere(t, i, 1);
    }

    free(versions);
    return 0;
}

int extraire_fichier_depuis_bloc(struct table_entree *t, uint32_t index, char **objet_complet, char **contenu, size_t *taille_contenu, char *nom_fichier, long *modif, int *droit) {
    verrouiller_bloc(t->fd, index, F_RDLCK);
    uint32_t longueur_totale = t->table[index].longueur;
    uint32_t type_bloc = t->table[index].type;
    attendre(t->slowns);
    deverrouiller_bloc(t->fd, index);

    if (longueur_totale == BLOC_LIBRE || type_bloc != TYPE_FICHIER) {
        return -1;
    }

    char *objet = recuperer_chaine_complete(t, index);
    if (objet == NULL) {
        return -1;
    }

    char *fin_meta = memchr(objet, '\n', longueur_totale);
    if (fin_meta == NULL) {
        fprintf(stderr, "[ERREUR] lookup: metadonnees fichier corrompues.\n");
        free(objet);
        return -1;
    }

    size_t taille_meta = (size_t)(fin_meta - objet + 1);
    char *meta = malloc(taille_meta + 1);
    if (meta == NULL) {
        perror("[ERREUR] malloc (lookup)");
        free(objet);
        return -1;
    }

    memcpy(meta, objet, taille_meta);
    meta[taille_meta] = '\0';

    long taille_fichier = 0;
    if (sscanf(meta, "nom:%255[^|]|taille:%ld|modif:%ld|droit:%d",
               nom_fichier, &taille_fichier, modif, droit) != 4) {
        fprintf(stderr, "[ERREUR] lookup: impossible de lire les metadonnees.\n");
        free(meta);
        free(objet);
        return -1;
    }

    free(meta);

    if ((size_t)taille_fichier > longueur_totale - taille_meta) {
        fprintf(stderr, "[ERREUR] lookup: taille de fichier incoherente.\n");
        free(objet);
        return -1;
    }

    *objet_complet = objet;
    *contenu = objet + taille_meta;
    *taille_contenu = (size_t)taille_fichier;
    return 0;
}

void afficher_resume_fichier(struct table_entree *t, uint32_t index) {
    char *objet = NULL;
    char *contenu = NULL;
    size_t taille_contenu = 0;
    char nom_fichier[256] = {0};
    long modif = 0;
    int droit = 0;

    if (extraire_fichier_depuis_bloc(t, index, &objet, &contenu, &taille_contenu, nom_fichier, &modif, &droit) < 0) {
        printf("[ALERTE] -> bloc %u : fichier corrompu ou illisible\n", index);
        return;
    }

    printf("[INFO] -> bloc %u | nom: %s | taille: %zu | modif: %ld | droit: %d\n",
           index, nom_fichier, taille_contenu, modif, droit);

    free(objet);
}

int sauvegarder_fichier_lookup(struct table_entree *t, uint32_t index, uint32_t numero) {
    char *objet = NULL;
    char *contenu = NULL;
    size_t taille_contenu = 0;
    char nom_fichier[256] = {0};
    long modif = 0;
    int droit = 0;

    if (extraire_fichier_depuis_bloc(t, index, &objet, &contenu, &taille_contenu, nom_fichier, &modif, &droit) < 0) {
        return -1;
    }

    const char *base = strrchr(nom_fichier, '/');
    if (base != NULL) {
        base++;
    } else {
        base = nom_fichier;
    }

    char nom_sans_ext[256];
    char extension[128];

    const char *point = strrchr(base, '.');
    if (point != NULL && point != base) {
        size_t taille_base = (size_t)(point - base);
        if (taille_base >= sizeof(nom_sans_ext)) {
            taille_base = sizeof(nom_sans_ext) - 1;
        }

        memcpy(nom_sans_ext, base, taille_base);
        nom_sans_ext[taille_base] = '\0';

        snprintf(extension, sizeof(extension), "%s", point);
    } else {
        snprintf(nom_sans_ext, sizeof(nom_sans_ext), "%s", base);
        extension[0] = '\0';
    }

    char chemin[512];
    snprintf(chemin, sizeof(chemin), "%s_version%u%s", nom_sans_ext, numero, extension);

    int fd = open(chemin, O_CREAT | O_RDWR | O_TRUNC, (mode_t)(droit & 0777));
    if (fd < 0) {
        perror("[ERREUR] open (lookup save)");
        free(objet);
        return -1;
    }

    if (ftruncate(fd, (off_t)taille_contenu) < 0) {
        perror("[ERREUR] ftruncate (lookup save)");
        close(fd);
        free(objet);
        return -1;
    }

    if (taille_contenu > 0) {
        char *sortie = mmap(NULL, taille_contenu, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (sortie == MAP_FAILED) {
            perror("[ERREUR] mmap (lookup save)");
            close(fd);
            free(objet);
            return -1;
        }

        memcpy(sortie, contenu, taille_contenu);

        if (munmap(sortie, taille_contenu) < 0) {
            perror("[ERREUR] munmap (lookup save)");
            close(fd);
            free(objet);
            return -1;
        }
    }

    close(fd);
    printf("[INFO] Fichier sauvegarde: %s\n", chemin);
    free(objet);
    return 0;
}

uint32_t deplacer_bloc(const char *nom_table, struct table_entree *t) {
    for (uint32_t i = 0; i < t->b; i++) {
        verrouiller_bloc(t->fd, i, F_WRLCK);
        uint32_t longueur_i = t->table[i].longueur;
        uint32_t premier_i = t->table[i].premier;

        if (!(longueur_i == BLOC_LIBRE && premier_i != i && premier_i != BLOC_LIBRE)) {
            attendre(t->slowns);
            deverrouiller_bloc(t->fd, i);
            continue;
        }

        uint32_t squatter = premier_i;
        verrouiller_bloc(t->fd, squatter, F_WRLCK);
        t->table[i].premier = i;
        t->table[i].precedent = FIN_CHAINE;
        t->table[i].suivant = t->table[squatter].suivant;
        t->table[i].longueur = t->table[squatter].longueur;
        t->table[i].type = t->table[squatter].type;
        memcpy(t->table[i].obj, t->table[squatter].obj, ESPACE_MAX);

        uint32_t suivant_nouveau = t->table[i].suivant;
        uint32_t type_nouveau = t->table[i].type;

        attendre(t->slowns);
        if (suivant_nouveau != FIN_CHAINE) {
            verrouiller_bloc(t->fd, suivant_nouveau, F_WRLCK);
            t->table[suivant_nouveau].precedent = i;
            attendre(t->slowns);
            deverrouiller_bloc(t->fd, suivant_nouveau);
        }


        if (type_nouveau == TYPE_FICHIER) {
            char nom_fichier[256];
            if (sscanf(t->table[i].obj, "nom:%255[^|]", nom_fichier) != 1) {
                fprintf(stderr, "[ERREUR] deplacer_bloc: fichier corrompu (nom dans metadonnee).\n");
                deverrouiller_bloc(t->fd, squatter);
                deverrouiller_bloc(t->fd, i);
                return BLOC_ERREUR;
            }

            deverrouiller_bloc(t->fd, i);

            uint32_t hash = h(nom_table, nom_fichier, strlen(nom_fichier), t->b);
            verrouiller_bloc(t->fd, hash, F_RDLCK);
            uint32_t index = t->table[hash].premier;
            attendre(t->slowns);
            deverrouiller_bloc(t->fd, hash);
            while (index != FIN_CHAINE) {
                verrouiller_bloc(t->fd, index, F_RDLCK);
                uint32_t type_actuel = t->table[index].type;
                uint32_t longueur_actuelle = t->table[index].longueur;
                uint32_t suivant_actuel = t->table[index].suivant;
                int match = (type_actuel == TYPE_NOM_FICHIER && memcmp(nom_fichier, t->table[index].obj, strlen(nom_fichier)) == 0);
                attendre(t->slowns);
                deverrouiller_bloc(t->fd, index);

                if (match) {
                    uint32_t *versions = recuperer_versions_fichier(t, index);
                    if (versions == NULL) {
                        t->table[squatter].premier = squatter;
                        deverrouiller_bloc(t->fd, squatter);
                        return -1;
                    }

                    uint32_t modifier = 1;
                    uint32_t nb_elements = (longueur_actuelle - (strlen(nom_fichier) + 1)) / sizeof(uint32_t);
                    for (uint32_t k = 0; k < nb_elements; k++) {
                        if (versions[k] == squatter) {
                            versions[k] = i;
                            modifier = 0;
                        }
                    }

                    if (modifier == 0) {
                        mettre_a_jour_versions_fichiers(t, versions, nb_elements, index);
                    } else {
                        fprintf(stderr, "[ERREUR] deplacer_bloc: fichier non trouver\n");
                    }
                    
                    free(versions);
                    break;
                }

                while (suivant_actuel != FIN_CHAINE && longueur_actuelle > ESPACE_MAX) {
                    index = suivant_actuel;
                    verrouiller_bloc(t->fd, index, F_RDLCK);
                    longueur_actuelle = t->table[index].longueur;
                    suivant_actuel = t->table[index].suivant;
                    attendre(t->slowns);
                    deverrouiller_bloc(t->fd, index);
                }

                index = suivant_actuel;
            }
        } else {
            deverrouiller_bloc(t->fd, i);
        }

        if (t->table[squatter].premier == BLOC_LIBRE) {
            t->table[squatter].premier = squatter;
        }
        
        t->table[squatter].precedent = FIN_CHAINE;
        t->table[squatter].suivant = FIN_CHAINE;
        t->table[squatter].longueur = BLOC_LIBRE;
        t->table[squatter].type = TYPE_VIDE;
        memset(t->table[squatter].obj, 0, ESPACE_MAX);
        attendre(t->slowns);
        deverrouiller_bloc(t->fd, squatter);
    }

    return 0;
}

int lire_numero_version(uint32_t nb_elements, uint32_t *choix_version) {
    char ligne[32];

    printf("Entrez le numero de version (0-%u): ", nb_elements - 1);

    if (fgets(ligne, sizeof(ligne), stdin) == NULL) {
        return -1;
    }

    char *fin = NULL;
    unsigned long val = strtoul(ligne, &fin, 10);

    if (fin == ligne || val >= nb_elements) {
        fprintf(stderr, "[ERREUR] numero de version invalide.\n");
        return -1;
    }

    *choix_version = (uint32_t)val;
    return 0;
}

void *insert_nouvelle_table(void *arg) {
    thread_ags *args = (thread_ags *)arg;

    for (uint32_t i = args->debut; i < args->fin; i++) {
        if (args->t->table[i].longueur == BLOC_LIBRE || args->premiere_donnee[i] == 1 || args->t->table[i].type == TYPE_NOM_FICHIER) {
            continue;
        }

        char *donnee = recuperer_chaine_complete(args->t, i);
        if (donnee == NULL) {
            args->erreur = BLOC_ERREUR;
            return NULL;
        }

        pthread_mutex_lock(args->mutex);

        uint32_t index = handle_data(args->nom_nouvelle_table, donnee, args->t->table[i].longueur, args->t_nouveau, donnee, args->t->table[i].longueur, args->t->table[i].type);
        if (index == BLOC_ERREUR) {
            pthread_mutex_unlock(args->mutex);
            args->erreur = BLOC_ERREUR;
            free(donnee);
            return NULL;
        }

        args->index_nom_fichier[i] = index;
        pthread_mutex_unlock(args->mutex);
        free(donnee);
    }

    return NULL;
}

uint32_t init_extend_shrink(const char *nom_table, const uint32_t est_extend, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ, slowns) < 0) {
        return BLOC_ERREUR;
    }

    struct flock lock_global;
    lock_global.l_type = F_WRLCK;
    lock_global.l_whence = SEEK_SET;
    lock_global.l_start = 0;
    lock_global.l_len = 0;

    if (fcntl(t.fd, F_SETLK, &lock_global) == -1) {
        fprintf(stderr, "[ERREUR] Redimensionnement en cours par un autre processus !\n");
        close_table(&t);
        return BLOC_ERREUR;
    }

    uint32_t *premiere_donnee = recuperer_premiere_donnee(&t);
    if (premiere_donnee == NULL) {
        close_table(&t);
        return BLOC_ERREUR;
    }

    char nom_nouvelle_table[256];
    char *extension = strrchr(nom_table, '.');
    if (extension == NULL) {
        fprintf(stderr, "[ERREUR] extend: le fichier n'a pas d'extension\n");
        free(premiere_donnee);
        close_table(&t);
        return BLOC_ERREUR;
    }

    snprintf(nom_nouvelle_table, sizeof(nom_nouvelle_table), "table_temp%s", extension);
    uint32_t b = est_extend ? t.b * 2 : t.b / 2;
    if (make_cmd(nom_nouvelle_table, b) < 0) {
        free(premiere_donnee);
        close_table(&t);
        return BLOC_ERREUR;
    }

    struct table_entree t_nouveau;
    if (open_table(nom_nouvelle_table, &t_nouveau, O_RDWR, PROT_READ | PROT_WRITE, slowns) < 0) {
        free(premiere_donnee);
        close_table(&t);
        return BLOC_ERREUR;
    }

    long nb_threads = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t thread[nb_threads];
    thread_ags args[nb_threads];
    size_t taille = t.b / nb_threads;
    uint32_t *index_nom_fichier = calloc(t.b, sizeof(uint32_t));
    if (index_nom_fichier == NULL) {
        perror("calloc (init_extend_shrink)");
        free(premiere_donnee);
        close_table(&t);
        return BLOC_ERREUR;
    }

    pthread_mutex_t m;
    pthread_mutex_init(&m, NULL);
    for (long i = 0; i < nb_threads; i++) {
        args[i].thread_id = i;
        args[i].t = &t;
        args[i].nom_nouvelle_table = nom_nouvelle_table;
        args[i].t_nouveau = &t_nouveau;
        args[i].premiere_donnee = premiere_donnee;
        args[i].debut = i * taille;
        args[i].fin = (i == nb_threads - 1) ? t.b : (i + 1) * taille;
        args[i].b = t.b;
        args[i].erreur = 0;
        args[i].index_nom_fichier = index_nom_fichier;
        args[i].mutex = &m;
        pthread_create(&thread[i], NULL, insert_nouvelle_table, &args[i]);
    }

    uint32_t erreur = 0;
    for (int i = 0; i < nb_threads; i++) {
        pthread_join(thread[i], NULL);
        if (args[i].erreur == BLOC_ERREUR) {
            printf("[ERREUR] Le thread %d a une erreur.\n", i);
            erreur = BLOC_ERREUR;
        }
    }
    pthread_mutex_destroy(&m);

    if (erreur != BLOC_ERREUR) {
        for (uint32_t i = 0; i < t.b; i++) {
            if (t.table[i].longueur != BLOC_LIBRE && t.table[i].type == TYPE_NOM_FICHIER && premiere_donnee[i] == 0) {
                char *nom_fichier = t.table[i].obj;
                uint32_t *versions = recuperer_versions_fichier(&t, i);
                if (versions != NULL) {
                    uint32_t offset = strlen(nom_fichier) + 1;
                    uint32_t nb_elements = (t.table[i].longueur - offset) / sizeof(uint32_t);

                    for (uint32_t k = 0; k < nb_elements; k++) {
                        uint32_t ancien_index = versions[k];
                        uint32_t nouveau_index = index_nom_fichier[ancien_index];
                        size_t taille_nom = strlen(nom_fichier) + 1;
                        char *nom_donnee = malloc(taille_nom + sizeof(uint32_t));
                        if (nom_donnee != NULL) {
                            memcpy(nom_donnee, nom_fichier, taille_nom);
                            memcpy(nom_donnee + taille_nom, &nouveau_index, sizeof(uint32_t));
                            if (handle_data(nom_table, nom_fichier, strlen(nom_fichier), &t_nouveau, nom_donnee, taille_nom + sizeof(uint32_t), TYPE_NOM_FICHIER) == BLOC_ERREUR) {
                                free(nom_donnee);
                                erreur = BLOC_ERREUR;
                                break;
                            }

                            free(nom_donnee);
                        } else {
                            perror("malloc (init_extend_shrink)");
                            erreur = BLOC_ERREUR;
                        }
                    }
                    free(versions);
                } else {
                    erreur = BLOC_ERREUR;
                }
            }

            if (erreur == BLOC_ERREUR) {
                break;
            }
        }
    }
    
    free(premiere_donnee);
    free(index_nom_fichier);
    close_table(&t);
    close_table(&t_nouveau);
    if (erreur == BLOC_ERREUR) {
        remove(nom_nouvelle_table);
        return BLOC_ERREUR;
    }

    rename(nom_nouvelle_table, nom_table);
    return 0;
}


void *insert_lignes_gros_fichier(void *arg) {
    thread_gros_fichier_args *args = (thread_gros_fichier_args *)arg;

    size_t pos = args->debut;

    if (pos > 0) {
        while (pos < args->taille_fichier && args->contenu[pos - 1] != '\n') {
            pos++;
        }
    }

    while (pos < args->taille_fichier && pos < args->fin) {
        size_t debut_ligne = pos;

        while (pos < args->taille_fichier && args->contenu[pos] != '\n') {
            pos++;
        }

        size_t taille_ligne = pos - debut_ligne;

        if (taille_ligne > 0) {
            char *ligne = malloc(taille_ligne + 1);
            if (ligne == NULL) {
                perror("[ERREUR] malloc (insertgrosfichier)");
                args->erreur = BLOC_ERREUR;
                return NULL;
            }

            memcpy(ligne, args->contenu + debut_ligne, taille_ligne);
            ligne[taille_ligne] = '\0';

            pthread_mutex_lock(args->mutex_insert);
            if (handle_data(args->nom_table, ligne, taille_ligne, args->t, ligne, taille_ligne, TYPE_CHAINE) == BLOC_ERREUR) {
                args->erreur = BLOC_ERREUR;
            }
            pthread_mutex_unlock(args->mutex_insert);

            free(ligne);

            if (args->erreur == BLOC_ERREUR) {
                return NULL;
            }
        }

        if (pos < args->taille_fichier && args->contenu[pos] == '\n') {
            pos++;
        }
    }

    return NULL;
}
