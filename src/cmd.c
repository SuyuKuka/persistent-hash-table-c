#include "cmd.h"
#include <sys/stat.h>
#include <pthread.h>


int make_cmd(const char *nom_table, const uint32_t b) {
    if (b == 0) {
        fprintf(stderr, "[ERREUR] make: b ne peut pas etre egal a 0.\n");
        return -1;
    }

    int fd = open(nom_table, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("[ERREUR] open (make)");
        return -1;
    }

    struct flock lock_global;
    lock_global.l_type = F_WRLCK;
    lock_global.l_whence = SEEK_SET;
    lock_global.l_start = 0;
    lock_global.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock_global) == -1) {
        fprintf(stderr, "[ERREUR] make: le fichier est deja en cours d'utilisation.\n");
        close(fd);
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

int free_cmd(const char *nom_table, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ, slowns) < 0) {
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
        verrouiller_bloc(t.fd, i, F_RDLCK);
        uint32_t longueur_actuelle = t.table[i].longueur;
        uint32_t type_actuel = t.table[i].type;
        attendre(slowns);
        deverrouiller_bloc(t.fd, i);

        if (longueur_actuelle == BLOC_LIBRE) {
            bloc_libre++;
            continue;
        }
        
        if (premiere_donnee[i] == 0) {
            if (type_actuel == TYPE_CHAINE) {
                nb_chaine++;
            } else if (type_actuel == TYPE_FICHIER) {
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

int insertstring_cmd(const char *nom_table, const char *chaine, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE, slowns) < 0) {
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

int insertfile_cmd(const char *nom_table, const char *nom_fichier, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE, slowns) < 0) {
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

    uint32_t bloc_necessaire = bloc_suffisant_fichier(nom_table, &t, nom_fichier);
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

int lookup_cmd(const char *nom_table, const char *chaine, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ, slowns) < 0) {
        return -1;
    }

    int trouve = 0;
    size_t taille = strlen(chaine);

    uint32_t bloc_str = recherche_bloc(nom_table, &t, chaine, taille, TYPE_CHAINE);
    if (bloc_str != BLOC_ERREUR) {
        printf("[INFO] La chaine existe dans la table (bloc %u).\n", bloc_str);
        trouve = 1;
    }

    uint32_t bloc_nom = recherche_bloc(nom_table, &t, chaine, taille, TYPE_NOM_FICHIER);
    if (bloc_nom != BLOC_ERREUR) {
        printf("[INFO] Des fichiers portant le nom \"%s\" existent.\n", chaine);

        uint32_t *versions = recuperer_versions_fichier(&t, bloc_nom);
        if (versions == NULL) {
            close_table(&t);
            return -1;
        }

        verrouiller_bloc(t.fd, bloc_nom, F_RDLCK);
        uint32_t longueur_nom = t.table[bloc_nom].longueur;
        size_t taille = strlen(t.table[bloc_nom].obj);
        attendre(slowns);
        deverrouiller_bloc(t.fd, bloc_nom);

        uint32_t nb_elements = (longueur_nom - (taille + 1)) / sizeof(uint32_t);
        printf("[INFO] Nombre de versions associees : %u\n", nb_elements);
        printf("\nVersions disponibles :\n");
        for (uint32_t k = 0; k < nb_elements; k++) {
            printf("  [%u] ", k);

            if (versions[k] >= t.b) {
                printf("[ALERTE] index corrompu: %u\n", versions[k]);
                continue;
            }

            verrouiller_bloc(t.fd, versions[k], F_RDLCK);
            uint32_t longueur_version = t.table[versions[k]].longueur;
            uint32_t type_version = t.table[versions[k]].type;
            attendre(slowns);
            deverrouiller_bloc(t.fd, versions[k]);
            if (longueur_version == BLOC_LIBRE || type_version != TYPE_FICHIER) {
                printf("[ALERTE] bloc %u ne pointe pas vers un fichier valide\n", versions[k]);
                continue;
            }

            afficher_resume_fichier(&t, versions[k]);
        }

        printf("\nChoisissez une action pour ces fichiers :\n");
        printf("  [n] juste notifier\n");
        printf("  [a] afficher le contenu\n");
        printf("  [s] sauvegarder les fichiers\n");
        printf("Votre choix: ");

        char choix[32];
        if (fgets(choix, sizeof(choix), stdin) != NULL) {
            if (choix[0] == 'a' || choix[0] == 'A' || choix[0] == 's' || choix[0] == 'S') {
                if (nb_elements == 1) {
                    uint32_t k = 0;
                    verrouiller_bloc(t.fd, versions[k], F_RDLCK);
                    uint32_t longueur_version = t.table[versions[k]].longueur;
                    uint32_t type_version = t.table[versions[k]].type;
                    attendre(slowns);
                    deverrouiller_bloc(t.fd, versions[k]);

                    if (versions[k] >= t.b || longueur_version == BLOC_LIBRE || type_version != TYPE_FICHIER) {
                        fprintf(stderr, "[ERREUR] version invalide ou corrompue.\n");
                        free(versions);
                        close_table(&t);
                        return -1;
                    }

                    if (choix[0] == 'a' || choix[0] == 'A') {
                        char *objet = NULL;
                        char *contenu = NULL;
                        size_t taille_contenu = 0;
                        char nom_fichier[256] = {0};
                        long modif = 0;
                        int droit = 0;

                        if (extraire_fichier_depuis_bloc(&t, versions[k], &objet, &contenu, &taille_contenu, nom_fichier, &modif, &droit) == 0) {
                            printf("\n===== CONTENU DU FICHIER %s version %u (bloc %u) =====\n", nom_fichier, k, versions[k]);
                            fwrite(contenu, 1, taille_contenu, stdout);
                            printf("\n===== FIN DU FICHIER %s version %u =====\n", nom_fichier, k);
                            free(objet);
                        }
                    } else {
                        if (sauvegarder_fichier_lookup(&t, versions[k], k) < 0) {
                            free(versions);
                            close_table(&t);
                            return -1;
                        }
                    }
                } else {
                    printf("\nVoulez-vous traiter :\n");
                    printf("  [u] une seule version\n");
                    printf("  [t] toutes les versions\n");
                    printf("Votre choix: ");

                    char choix_version[32];
                    if (fgets(choix_version, sizeof(choix_version), stdin) == NULL) {
                        free(versions);
                        close_table(&t);
                        return -1;
                    }

                    if (choix_version[0] == 'u' || choix_version[0] == 'U') {
                        uint32_t k;
                        if (lire_numero_version(nb_elements, &k) < 0) {
                            free(versions);
                            close_table(&t);
                            return -1;
                        }

                        verrouiller_bloc(t.fd, versions[k], F_RDLCK);
                        uint32_t longueur_version = t.table[versions[k]].longueur;
                        uint32_t type_version = t.table[versions[k]].type;
                        attendre(slowns);
                        deverrouiller_bloc(t.fd, versions[k]);
                        if (versions[k] >= t.b || longueur_version == BLOC_LIBRE || type_version != TYPE_FICHIER) {
                            fprintf(stderr, "[ERREUR] version invalide ou corrompue.\n");
                            free(versions);
                            close_table(&t);
                            return -1;
                        }

                        if (choix[0] == 'a' || choix[0] == 'A') {
                            char *objet = NULL;
                            char *contenu = NULL;
                            size_t taille_contenu = 0;
                            char nom_fichier[256] = {0};
                            long modif = 0;
                            int droit = 0;

                            if (extraire_fichier_depuis_bloc(&t, versions[k], &objet, &contenu, &taille_contenu, nom_fichier, &modif, &droit) == 0) {
                                printf("\n===== CONTENU DU FICHIER %s version %u (bloc %u) =====\n",
                                    nom_fichier, k, versions[k]);
                                fwrite(contenu, 1, taille_contenu, stdout);
                                printf("\n===== FIN DU FICHIER %s version %u =====\n", nom_fichier, k);
                                free(objet);
                            }
                        } else {
                            if (sauvegarder_fichier_lookup(&t, versions[k], k) < 0) {
                                free(versions);
                                close_table(&t);
                                return -1;
                            }
                        }
                    } else if (choix_version[0] == 't' || choix_version[0] == 'T') {
                        for (uint32_t k = 0; k < nb_elements; k++) {
                            if (versions[k] >= t.b) {
                                continue;
                            }

                            verrouiller_bloc(t.fd, versions[k], F_RDLCK);
                            uint32_t longueur_version = t.table[versions[k]].longueur;
                            uint32_t type_version = t.table[versions[k]].type;
                            attendre(slowns);
                            deverrouiller_bloc(t.fd, versions[k]);
                            if (longueur_version == BLOC_LIBRE || type_version != TYPE_FICHIER) {
                                continue;
                            }

                            if (choix[0] == 'a' || choix[0] == 'A') {
                                char *objet = NULL;
                                char *contenu = NULL;
                                size_t taille_contenu = 0;
                                char nom_fichier[256] = {0};
                                long modif = 0;
                                int droit = 0;

                                if (extraire_fichier_depuis_bloc(&t, versions[k], &objet, &contenu, &taille_contenu, nom_fichier, &modif, &droit) == 0) {
                                    printf("\n===== CONTENU DU FICHIER %s version %u (bloc %u) =====\n",
                                        nom_fichier, k, versions[k]);
                                    fwrite(contenu, 1, taille_contenu, stdout);
                                    printf("\n===== FIN DU FICHIER %s version %u =====\n", nom_fichier, k);
                                    free(objet);
                                }
                            } else {
                                if (sauvegarder_fichier_lookup(&t, versions[k], k) < 0) {
                                    free(versions);
                                    close_table(&t);
                                    return -1;
                                }
                            }
                        }
                    } else {
                        printf("[INFO] Choix invalide, notification uniquement.\n");
                    }
                }
            }else {
                printf("[INFO] Des fichiers portant ce nom existent.\n");
            }
        }

        free(versions);
        trouve = 1;
    }

    if (!trouve) {
        printf("[INFO] Aucune chaine ou fichier correspondant a \"%s\" n'a ete trouve.\n", chaine);
    }

    close_table(&t);
    return 0;
}

int deletehash_cmd(const char *nom_table, const uint32_t hashcode, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE, slowns) < 0) {
        return -1;
    }

    if (t.b <= hashcode) {
        fprintf(stderr, "[ERREUR] deletehash: le hashcode depasse la taille de la table.\n");
        close_table(&t);
        return -1;
    }

    verrouiller_bloc(t.fd, hashcode, F_RDLCK);
    uint32_t premier_hash = t.table[hashcode].premier;
    uint32_t longueur_hash = t.table[hashcode].longueur;
    attendre(slowns);
    deverrouiller_bloc(t.fd, hashcode);

    if ((premier_hash == hashcode && longueur_hash == BLOC_LIBRE) || (premier_hash == BLOC_LIBRE)) {
        close_table(&t);
        return 0;
    }

    uint32_t index = premier_hash;
    uint32_t bloc_actuel = index;
    while (bloc_actuel != FIN_CHAINE) {
        verrouiller_bloc(t.fd, bloc_actuel, F_RDLCK);
        uint32_t type_actuel = t.table[bloc_actuel].type;
        uint32_t precedent_actuel = t.table[bloc_actuel].precedent;
        uint32_t suivant_actuel = t.table[bloc_actuel].suivant;

        int est_premier_fichier = 1;
        if (type_actuel == TYPE_FICHIER) {
            if (strncmp(t.table[bloc_actuel].obj, "nom:", 4) == 0) {
                est_premier_fichier = 0;
            }
        }
        attendre(slowns);
        deverrouiller_bloc(t.fd, bloc_actuel);

        if (type_actuel == TYPE_FICHIER) {
            if (est_premier_fichier == 0) {
                supprimer_versions_fichier(nom_table, &t, bloc_actuel);
            }
        } else if (type_actuel == TYPE_NOM_FICHIER) {
            int premier_nom_fichier = 1;
            
            if (precedent_actuel == FIN_CHAINE) {
                premier_nom_fichier = 0;
            } else {
                verrouiller_bloc(t.fd, precedent_actuel, F_RDLCK);
                uint32_t type_precedent = t.table[precedent_actuel].type;
                attendre(slowns);
                deverrouiller_bloc(t.fd, precedent_actuel);
                
                if (type_precedent != TYPE_NOM_FICHIER) {
                    premier_nom_fichier = 0;
                }
            }
            
            if (premier_nom_fichier == 0) {
                supprimer_nom_fichier(&t, bloc_actuel);
            }
        }

        bloc_actuel = suivant_actuel;
    }

    supprimer_par_derriere(&t, index, 0);

    if (deplacer_bloc(nom_table, &t) == BLOC_ERREUR) {
        close_table(&t);
        return -1;
    }

    close_table(&t);
    return 0;
}

int delete_cmd(const char *nom_table, const char *chaine, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE, slowns) < 0) {
        return -1;
    }

    int trouve = 0;
    size_t taille = strlen(chaine);

    uint32_t bloc_nom = recherche_bloc(nom_table, &t, chaine, taille, TYPE_NOM_FICHIER);
    if (bloc_nom != BLOC_ERREUR) {
        if (supprimer_nom_fichier(&t, bloc_nom) == BLOC_ERREUR) {
            close_table(&t);
            return -1;
        }

        supprimer_par_derriere(&t, bloc_nom, 1);
        trouve = 1;
    }

    uint32_t bloc_str = recherche_bloc(nom_table, &t, chaine, taille, TYPE_CHAINE);
    if (bloc_str != BLOC_ERREUR) {
        supprimer_par_derriere(&t, bloc_str, 1);
        trouve = 1;
    }

    if (!trouve) {
        fprintf(stderr, "[ERREUR] delete: entree introuvable.\n");
        close_table(&t);
        return -1;
    }

    if (deplacer_bloc(nom_table, &t) == BLOC_ERREUR) {
        close_table(&t);
        return -1;
    }

    close_table(&t);
    return 0;
}

int check_cmd(const char *nom_table, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ, slowns) < 0) {
        return -1;
    }

    uint32_t *premiere_donnee = recuperer_premiere_donnee(&t);
    if (premiere_donnee == NULL) {
        close_table(&t);
        return -1;
    }

    for (uint32_t i = 0; i < t.b; i++) {
        verrouiller_bloc(t.fd, i, F_RDLCK);
        uint32_t longueur_i = t.table[i].longueur;
        uint32_t type_i = t.table[i].type;
        char nom_fichier[256] = {0};
        if (longueur_i != BLOC_LIBRE && type_i == TYPE_NOM_FICHIER) {
            strncpy(nom_fichier, t.table[i].obj, 255);
        }
        attendre(slowns);
        deverrouiller_bloc(t.fd, i);

        if (longueur_i == BLOC_LIBRE) continue;
        if (premiere_donnee[i] == 1) continue;

        if (type_i != TYPE_NOM_FICHIER) {
            char *chaine = recuperer_chaine_complete(&t, i);
            uint32_t hash = h(nom_table, chaine, t.table[i].longueur, t.b);
            if (hash == BLOC_ERREUR) {
                free(premiere_donnee);
                free(chaine);
                close_table(&t);
                return BLOC_ERREUR;
            }

            free(chaine);

            verrouiller_bloc(t.fd, hash, F_RDLCK);
            uint32_t index = t.table[hash].premier;
            attendre(slowns);
            deverrouiller_bloc(t.fd, hash);
            int verif = 0;
            while (index != FIN_CHAINE) {
                if (index == i) {
                    verif = 1; 
                    break;
                }

                verrouiller_bloc(t.fd, index, F_RDLCK);
                uint32_t suivant = t.table[index].suivant;
                attendre(slowns);
                deverrouiller_bloc(t.fd, index);
                
                index = suivant;
            }

            if (verif == 0) {
                fprintf(stderr, "[ERREUR] check: le bloc %u est corrompu, le hash du contenu est différent de premier.\n", i);
                free(premiere_donnee);
                close_table(&t);
                return -1;
            }
        } else {
            uint32_t hash_nom_fichier = h(nom_table, nom_fichier, strlen(nom_fichier), t.b);
            if (hash_nom_fichier == BLOC_ERREUR) {
                free(premiere_donnee);
                close_table(&t);
                return BLOC_ERREUR;
            }

            verrouiller_bloc(t.fd, hash_nom_fichier, F_RDLCK);
            uint32_t index = t.table[hash_nom_fichier].premier;
            attendre(slowns);
            deverrouiller_bloc(t.fd, hash_nom_fichier);
            int verif = 0;
            while (index != FIN_CHAINE) {
                if (index == i) {
                    verif = 1;
                    break;
                }
                
                verrouiller_bloc(t.fd, index, F_RDLCK);
                uint32_t suivant = t.table[index].suivant;
                attendre(slowns);
                deverrouiller_bloc(t.fd, index);
                
                index = suivant;
            }

            if (verif == 0) {
                fprintf(stderr, "[ERREUR] check: le bloc %u est corrompu (%s): il n'est pas lié à la boîte aux lettres %u.\n", i, nom_fichier, hash_nom_fichier);
                free(premiere_donnee);
                close_table(&t);
                return -1;
            }

            uint32_t offset = strlen(nom_fichier) + 1;
            if (t.table[i].obj[offset - 1] != '\0') {
                fprintf(stderr, "[ERREUR] check: le bloc %u est corrompu: \\0 n'a pas ete trouve.\n", i);
                free(premiere_donnee);
                close_table(&t);
                return -1;
            }

            uint32_t *versions = recuperer_versions_fichier(&t, i);
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

                verrouiller_bloc(t.fd, versions[k], F_RDLCK);
                uint32_t longueur_version = t.table[versions[k]].longueur;
                uint32_t type_version = t.table[versions[k]].type;
                char obj_version[256] = {0};
                if (longueur_version != BLOC_LIBRE && type_version == TYPE_FICHIER) {
                    strncpy(obj_version, t.table[versions[k]].obj, 255);
                }
                attendre(slowns);
                deverrouiller_bloc(t.fd, versions[k]);
                if (longueur_version == BLOC_LIBRE || type_version != TYPE_FICHIER) {
                    fprintf(stderr, "[ERREUR] check: le bloc %u est corrompu (%s): l'index ne pointe pas vers le bon fichier.\n", versions[k], nom_fichier);
                    free(versions);
                    free(premiere_donnee);
                    close_table(&t);
                    return -1;
                }

                if (memcmp(obj_version + strlen("nom:"), nom_fichier, strlen(nom_fichier)) != 0) {
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

int extend_cmd(const char *nom_table, const long slowns) {
    if (init_extend_shrink(nom_table, 1, slowns) == BLOC_ERREUR) {
        return -1;
    }

    return 0;
}

int shrink_cmd(const char *nom_table, const long slowns) {
    if (init_extend_shrink(nom_table, 0, slowns) == BLOC_ERREUR) {
        return -1;
    }
    
    return 0;
}

int ajust_cmd(const char *nom_table, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ, slowns) < 0) {
        return -1;
    }

    uint32_t bloc_libre = 0;
    for (uint32_t i = 0; i < t.b; i++) {
        verrouiller_bloc(t.fd, i, F_RDLCK);
        uint32_t longueur_i = t.table[i].longueur;
        attendre(slowns);
        deverrouiller_bloc(t.fd, i);
        if (longueur_i == BLOC_LIBRE) {
            bloc_libre++;
        }
    }
    
    close_table(&t);
    double taux_remplissage = ((double)(t.b - bloc_libre) / (double)t.b) * 100.0;
    if (taux_remplissage > 70.0) {
        if (extend_cmd(nom_table, slowns) < 0) {
            return -1;
        }
    } else if (taux_remplissage < 10.0) {
        if (shrink_cmd(nom_table, slowns) < 0) {
            return -1;
        }
    } 

    return 0;
}

int insertgrosfichier_cmd(const char *nom_table, const char *nom_fichier, const long slowns) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE, slowns) < 0) {
        return -1;
    }

    int fd = open(nom_fichier, O_RDONLY);
    if (fd < 0) {
        perror("[ERREUR] open (insertgrosfichier)");
        close_table(&t);
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("[ERREUR] fstat (insertgrosfichier)");
        close(fd);
        close_table(&t);
        return -1;
    }

    if (st.st_size == 0) {
        close(fd);
        close_table(&t);
        return 0;
    }

    char *contenu = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (contenu == MAP_FAILED) {
        perror("[ERREUR] mmap (insertgrosfichier)");
        close(fd);
        close_table(&t);
        return -1;
    }
    /*
    size_t blocs_necessaires = 0;
    size_t pos = 0;
    while (pos < (size_t)st.st_size) {
        size_t debut_ligne = pos;

        while (pos < (size_t)st.st_size && contenu[pos] != '\n') {
            pos++;
        }

        size_t taille_ligne = pos - debut_ligne;
        if (taille_ligne > 0) {
            blocs_necessaires += (taille_ligne + ESPACE_MAX - 1) / ESPACE_MAX;
        }

        if (pos < (size_t)st.st_size && contenu[pos] == '\n') {
            pos++;
        }
    }

    size_t blocs_libres = 0;
    for (uint32_t i = 0; i < t.b; i++) {
        verrouiller_bloc(t.fd, i, F_RDLCK);
        if (t.table[i].longueur == BLOC_LIBRE) {
            blocs_libres++;
        }
        attendre(slowns);
        deverrouiller_bloc(t.fd, i);
    }

    if (blocs_libres < blocs_necessaires) {
        fprintf(stderr, "[ERREUR] insertgrosfichier: table trop petite (%zu blocs libres, %zu necessaires).\n", blocs_libres, blocs_necessaires);
        munmap(contenu, st.st_size);
        close(fd);
        close_table(&t);
        return -1;
    }*/

    long nb_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (nb_threads <= 0) {
        nb_threads = 1;
    }

    pthread_t threads[nb_threads];
    thread_gros_fichier_args args[nb_threads];
    pthread_mutex_t mutex_insert;
    pthread_mutex_init(&mutex_insert, NULL);

    size_t taille_zone = (size_t)st.st_size / (size_t)nb_threads;
    uint32_t erreur = 0;

    for (long i = 0; i < nb_threads; i++) {
        args[i].thread_id = (int)i;
        args[i].nom_table = nom_table;
        args[i].t = &t;
        args[i].contenu = contenu;
        args[i].taille_fichier = (size_t)st.st_size;
        args[i].debut = (size_t)i * taille_zone;
        args[i].fin = (i == nb_threads - 1) ? (size_t)st.st_size : (size_t)(i + 1) * taille_zone;
        args[i].mutex_insert = &mutex_insert;
        args[i].erreur = 0;

        if (pthread_create(&threads[i], NULL, insert_lignes_gros_fichier, &args[i]) != 0) {
            perror("[ERREUR] pthread_create (insertgrosfichier)");
            erreur = BLOC_ERREUR;
            nb_threads = i;
            break;
        }
    }

    for (long i = 0; i < nb_threads; i++) {
        pthread_join(threads[i], NULL);
        if (args[i].erreur == BLOC_ERREUR) {
            erreur = BLOC_ERREUR;
        }
    }

    pthread_mutex_destroy(&mutex_insert);
    munmap(contenu, st.st_size);
    close(fd);
    close_table(&t);

    return erreur == BLOC_ERREUR ? -1 : 0;
}
