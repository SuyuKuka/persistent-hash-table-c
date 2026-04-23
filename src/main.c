#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"

char *concatener_args(int argc, char *argv[]) {
    size_t taille_chaine = 0;
    for (int i = 4; i < argc; i++) {
        taille_chaine += strlen(argv[i]) + 1;
    }

    char *chaine = malloc(taille_chaine);
    if (chaine == NULL) {
        perror("[ERREUR] malloc");
        return NULL;
    }

    chaine[0] = '\0';
    for (int i = 4; i < argc; i++) {
        strcat(chaine, argv[i]);
        if (i < argc - 1) {
            strcat(chaine, " ");
        }
    }

    return chaine;
}

int handle_make(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "[ERREUR] Utilisation: %s make table slowns b\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (make_cmd(argv[2], atoi(argv[4])) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_free(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "[ERREUR] Utilisation: %s free table slowns\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (free_cmd(argv[2]) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_insertstring(int argc, char *argv[], const long slowns) {
    if (argc < 5) {
        fprintf(stderr, "[ERREUR] Utilisation: %s insertstring table slowns chaine\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *chaine = concatener_args(argc, argv);
    if (chaine == NULL) {
        return EXIT_FAILURE;
    }
    
    if (insertstring_cmd(argv[2], chaine, slowns) < 0) {
        free(chaine);
        return EXIT_FAILURE;
    }

    free(chaine);
    return EXIT_SUCCESS;
}

int handle_insertfile(int argc, char *argv[], const long slowns) {
    if (argc != 5) {
        fprintf(stderr, "[ERREUR] Utilisation: %s insertfile table slowns nom_fichier\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (insertfile_cmd(argv[2], argv[4], slowns) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_lookup(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    if (lookup_cmd() < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_deletehash(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "[ERREUR] Utilisation: %s deletehash table slowns hashcode\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (deletehash_cmd(argv[2], atoi(argv[4])) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_delete(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "[ERREUR] Utilisation: %s delete table slowns chaine\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *chaine = concatener_args(argc, argv);
    if (chaine == NULL) {
        return EXIT_FAILURE;
    }

    if (delete_cmd(argv[2], chaine) < 0) {
        free(chaine);
        return EXIT_FAILURE;
    }

    free(chaine);
    return EXIT_SUCCESS;
}

int handle_check(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "[ERREUR] Utilisation: %s check table slowns\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (check_cmd(argv[2]) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_extend(int argc, char *argv[], const long slowns) {
    if (argc != 4) {
        fprintf(stderr, "[ERREUR] Utilisation: %s extend table slowns\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (extend_cmd(argv[2], slowns) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_shrink(int argc, char *argv[], const long slowns) {
    if (argc != 4) {
        fprintf(stderr, "[ERREUR] Utilisation: %s shrink table slowns\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (shrink_cmd(argv[2], slowns) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int handle_ajust(int argc, char *argv[], const long slowns) {
    if (argc != 4) {
        fprintf(stderr, "[ERREUR] Utilisation: %s ajust table slowns\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (ajust_cmd(argv[2], slowns) < 0) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "[ERREUR] Utilisation: %s commande table slowns [autres parametres]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *cmd = argv[1];
    int handle;
    char *endptr;
    long slowns = strtol(argv[3], &endptr, 10);
    if (argv[3] == endptr || *endptr != '\0') {
        fprintf(stderr, "[ERREUR] slows doit etre un nombre.\n");
        return EXIT_FAILURE;
    }

    if (slowns < 0) {
        fprintf(stderr, "[ERREUR] slowns doit etre positif.\n");
        return EXIT_FAILURE;
    }

    if (strcmp(cmd, "make") == 0) {
        handle = handle_make(argc, argv);
    } else if (strcmp(cmd, "free") == 0) {
        handle = handle_free(argc, argv);
    } else if (strcmp(cmd, "insertstring") == 0) {
        handle = handle_insertstring(argc, argv, slowns);
    } else if (strcmp(cmd, "insertfile") == 0) {
        handle = handle_insertfile(argc, argv, slowns);
    } else if (strcmp(cmd, "lookup") == 0) {
        handle = handle_lookup(argc, argv);
    } else if (strcmp(cmd, "deletehash") == 0) {
        handle = handle_deletehash(argc, argv);
    } else if (strcmp(cmd, "delete") == 0) {
        handle = handle_delete(argc, argv);
    } else if (strcmp(cmd, "check") == 0) {
        handle = handle_check(argc, argv);
    } else if (strcmp(cmd, "extend") == 0) {
        handle = handle_extend(argc, argv, slowns);
    } else if (strcmp(cmd, "shrink") == 0) {
        handle = handle_shrink(argc, argv, slowns);
    } else if (strcmp(cmd, "ajust") == 0) {
        handle = handle_ajust(argc, argv, slowns);
    } else {
        fprintf(stderr, "[ERREUR] Commande %s inconnue.\n", cmd);
        return EXIT_FAILURE;
    }

    if (print_bloc(argv[2]) < 0) {
        return EXIT_FAILURE;
    }
    
    return handle;
}