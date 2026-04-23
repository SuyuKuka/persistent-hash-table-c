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

uint32_t insert_nom_fichier(struct table_entree *t, const char *donnee, const int premier_bloc, const long slowns) {
    uint32_t dernier_bloc = premier_bloc;

    verrouiller_bloc(t->fd, dernier_bloc, F_RDLCK);
    uint32_t suivant = t->table[dernier_bloc].suivant;
    uint32_t longueur = t->table[dernier_bloc].longueur;
    attendre(slowns);
    deverrouiller_bloc(t->fd, dernier_bloc);
    while (suivant != FIN_CHAINE && longueur > ESPACE_MAX) {
        dernier_bloc = suivant;
        
        verrouiller_bloc(t->fd, dernier_bloc, F_RDLCK);
        suivant = t->table[dernier_bloc].suivant;
        longueur = t->table[dernier_bloc].longueur;
        attendre(slowns);
        deverrouiller_bloc(t->fd, dernier_bloc);
    }

    if (longueur + sizeof(uint32_t) <= ESPACE_MAX) {
        verrouiller_bloc(t->fd, dernier_bloc, F_WRLCK);
        memcpy(t->table[dernier_bloc].obj + longueur, donnee, sizeof(uint32_t));
        t->table[dernier_bloc].longueur += sizeof(uint32_t);
        attendre(slowns);
        deverrouiller_bloc(t->fd, dernier_bloc);
    } else {
        uint32_t nouveau_bloc = bloc_libre(t, dernier_bloc, slowns);
        if (nouveau_bloc == BLOC_ERREUR) {
            return BLOC_ERREUR;
        }

        verrouiller_bloc(t->fd, dernier_bloc, F_WRLCK);
        uint32_t ancien_suivant = t->table[dernier_bloc].suivant;
        t->table[dernier_bloc].suivant = nouveau_bloc;
        t->table[dernier_bloc].longueur += sizeof(uint32_t);
        attendre(slowns);
        deverrouiller_bloc(t->fd, dernier_bloc);

        verrouiller_bloc(t->fd, nouveau_bloc, F_WRLCK);
        t->table[nouveau_bloc].premier = BLOC_LIBRE;
        t->table[nouveau_bloc].precedent = dernier_bloc;
        t->table[nouveau_bloc].suivant = t->table[dernier_bloc].suivant;
        t->table[nouveau_bloc].type = TYPE_NOM_FICHIER;
        t->table[nouveau_bloc].longueur = sizeof(uint32_t);
        memcpy(t->table[nouveau_bloc].obj, donnee, sizeof(uint32_t));
        attendre(slowns);
        deverrouiller_bloc(t->fd, nouveau_bloc);
        
        if (ancien_suivant != FIN_CHAINE) {
            verrouiller_bloc(t->fd, ancien_suivant, F_WRLCK);
            t->table[ancien_suivant].precedent = nouveau_bloc;
            attendre(slowns);
            deverrouiller_bloc(t->fd, ancien_suivant);
        }
    }

    uint32_t longueur_maj = premier_bloc;
    while (longueur_maj != dernier_bloc) {
        verrouiller_bloc(t->fd, longueur_maj, F_WRLCK);
        t->table[longueur_maj].longueur += sizeof(uint32_t);
        suivant = t->table[longueur_maj].suivant;
        attendre(slowns);
        deverrouiller_bloc(t->fd, longueur_maj);

        longueur_maj = suivant;
    }

    return 0;
}

uint32_t handle_data(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee, const long slowns) {
    uint32_t bloc = recherche_bloc(nom_table, t, cle, taille_cle, type_donnee, slowns);
    if (bloc != BLOC_ERREUR) {
        if (type_donnee == TYPE_NOM_FICHIER) {
            uint32_t insert = insert_nom_fichier(t, donnee + strlen(donnee) + 1, bloc, slowns);
            return insert;
        } else {
            fprintf(stderr, "[ALERTE] handle_data: la donnee existe deja dans la table.\n");
            return BLOC_ERREUR;
        }
    }

    return insertdata(nom_table, cle, taille_cle, t, donnee, taille_donnee, type_donnee, slowns);
}

uint32_t insertdata(const char *nom_table, const char *cle, size_t taille_cle, struct table_entree *t, const char *donnee, size_t taille_donnee, const uint32_t type_donnee, const long slowns) {
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
        index_actuel = bloc_libre(t, index_actuel, slowns);
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

            attendre(slowns);
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
        attendre(slowns);
        deverrouiller_bloc(t->fd, index_actuel);
        
        if (index_precedent != FIN_CHAINE) {
            verrouiller_bloc(t->fd, index_precedent, F_WRLCK);
            t->table[index_precedent].suivant = index_actuel;
            attendre(slowns);
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

void supprimer_objet_complet(struct hache_entree *table, uint32_t tete, const uint32_t b) {
    if (tete == BLOC_ERREUR || table[tete].longueur == BLOC_LIBRE) {
        return;
    }

    uint32_t courant = tete;
    uint32_t restant = table[tete].longueur;

    while (courant != FIN_CHAINE && restant > 0) {
        uint32_t suivant = table[courant].suivant;
        supprimer_bloc(table, courant, b);

        if (restant > ESPACE_MAX) {
            restant -= ESPACE_MAX;
        } else {
            restant = 0;
        }

        courant = suivant;
    }
}

int extraire_fichier_depuis_bloc(struct hache_entree *table, uint32_t index, char **objet_complet, char **contenu, size_t *taille_contenu, char *nom_fichier, size_t taille_nom, long *modif, int *droit) {
    (void)taille_nom;
    if (table[index].longueur == BLOC_LIBRE || table[index].type != TYPE_FICHIER) {
        return -1;
    }

    char *objet = recuperer_chaine_complete(table, index);
    if (objet == NULL) {
        return -1;
    }

    uint32_t longueur_totale = table[index].longueur;
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

void afficher_resume_fichier(struct hache_entree *table, uint32_t index) {
    char *objet = NULL;
    char *contenu = NULL;
    size_t taille_contenu = 0;
    char nom_fichier[256] = {0};
    long modif = 0;
    int droit = 0;

    if (extraire_fichier_depuis_bloc(table, index, &objet, &contenu, &taille_contenu, nom_fichier, sizeof(nom_fichier), &modif, &droit) < 0) {
        printf("[ALERTE] -> bloc %u : fichier corrompu ou illisible\n", index);
        return;
    }

    (void)contenu;
    printf("[INFO] -> bloc %u | nom: %s | taille: %zu | modif: %ld | droit: %d\n",
           index, nom_fichier, taille_contenu, modif, droit);

    free(objet);
}

int sauvegarder_fichier_lookup(struct hache_entree *table, uint32_t index, uint32_t numero) {
    char *objet = NULL;
    char *contenu = NULL;
    size_t taille_contenu = 0;
    char nom_fichier[256] = {0};
    long modif = 0;
    int droit = 0;

    if (extraire_fichier_depuis_bloc(table, index, &objet, &contenu, &taille_contenu, nom_fichier, sizeof(nom_fichier), &modif, &droit) < 0) {
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

    int fd = open(chemin, O_CREAT | O_WRONLY | O_TRUNC, (mode_t)(droit & 0777));
    if (fd < 0) {
        perror("[ERREUR] open (lookup save)");
        free(objet);
        return -1;
    }

    ssize_t ecrit = write(fd, contenu, taille_contenu);
    if (ecrit < 0 || (size_t)ecrit != taille_contenu) {
        perror("[ERREUR] write (lookup save)");
        close(fd);
        free(objet);
        return -1;
    }

    close(fd);
    printf("[INFO] Fichier sauvegarde: %s\n", chemin);
    free(objet);
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
int verrouiller_bloc(int fd, uint32_t index, short type_verrou) {
    struct flock lock;
    lock.l_type = type_verrou;
    lock.l_whence = SEEK_SET;
    lock.l_start = index * 1024;
    lock.l_len = 1024;

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("[ERREUR] fcntl verrouillage");
        return -1;
    }
    return 0;
}

int deverrouiller_bloc(int fd, uint32_t index) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = index * 1024;
    lock.l_len = 1024;

    if (fcntl(fd, F_SETLKW, &lock) == -1) {
        perror("[ERREUR] fcntl deverrouillage");
        return -1;
    }
    return 0;
}

int attendre(const long slowns) {
    if (slowns > 0) {
        struct timespec time;
        time.tv_sec = slowns / 1000000000L; 
        time.tv_nsec = slowns % 1000000000L;
        nanosleep(&time, NULL);
    }

    return 0;
}

void *insert_nouvelle_table(void *arg) {
    thread_ags *args = (thread_ags *)arg;

    for (uint32_t i = args->debut; i < args->fin; i++) {
        if (args->table[i].longueur == BLOC_LIBRE || args->premiere_donnee[i] == 1 || args->table[i].type == TYPE_NOM_FICHIER) {
            continue;
        }

        char *donnee = recuperer_chaine_complete(args->table, i);
        if (donnee == NULL) {
            args->erreur = BLOC_ERREUR;
            return NULL;
        }

        uint32_t index = handle_data(args->nom_nouvelle_table, donnee, args->table[i].longueur, args->t_nouveau, donnee, args->table[i].longueur, args->table[i].type, args->slowns);
        if (index == BLOC_ERREUR) {
            args->erreur = BLOC_ERREUR;
            free(donnee);
            return NULL;
        }

        args->index_nom_fichier[i] = index;
        free(donnee);
    }

    return NULL;
}

uint32_t init_extend_shrink(const char *nom_table, const uint32_t est_extend, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ) < 0) {
        return BLOC_ERREUR;
    }

    struct flock lock_global;
    lock_global.l_type = F_WRLCK;
    lock_global.l_whence = SEEK_SET;
    lock_global.l_start = 0;
    lock_global.l_len = 0;

    if (fcntl(t.fd, F_SETLK, &lock_global) == -1) {
        fprintf(stderr, "[ERREUR] Redimensionnement deja en cours par un autre processus !\n");
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
    if (open_table(nom_nouvelle_table, &t_nouveau, O_RDWR, PROT_READ | PROT_WRITE) < 0) {
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

    for (long i = 0; i < nb_threads; i++) {
        args[i].thread_id = i;
        args[i].table = t.table;
        args[i].nom_nouvelle_table = nom_nouvelle_table;
        args[i].t_nouveau = &t_nouveau;
        args[i].premiere_donnee = premiere_donnee;
        args[i].debut = i * taille;
        args[i].fin = (i == nb_threads - 1) ? t.b : (i + 1) * taille;
        args[i].b = t.b;
        args[i].slowns = slowns;
        args[i].erreur = 0;
        args[i].index_nom_fichier = index_nom_fichier;
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

    if (erreur != BLOC_ERREUR) {
        for (uint32_t i = 0; i < t.b; i++) {
            if (t.table[i].longueur != BLOC_LIBRE && t.table[i].type == TYPE_NOM_FICHIER && premiere_donnee[i] == 0) {
                char *nom_fichier = t.table[i].obj;
                uint32_t *versions = recuperer_versions_fichier(t.table, i);
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
                            if (handle_data(nom_table, nom_fichier, strlen(nom_fichier), &t_nouveau, nom_donnee, taille_nom + sizeof(uint32_t), TYPE_NOM_FICHIER, slowns) == BLOC_ERREUR) {
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