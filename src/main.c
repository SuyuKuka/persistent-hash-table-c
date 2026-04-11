#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"

int handle_make(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "[ERREUR] Utilisation: %s make table slowns b\n", argv[0]);
        return EXIT_FAILURE;
    }

    make_cmd(argv[2], atoi(argv[4]));
    return EXIT_SUCCESS;
}

int handle_free(int argc, char *argv[]) {
    handle_free(argc, argv);
    if (argc != 4) {
        fprintf(stderr, "[ERREUR] Utilisation: %s free table slowns\n", argv[0]);
        return EXIT_FAILURE;
    }

    free_cmd(argv[2]);
    return EXIT_SUCCESS;
}

int handle_insertstring(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "[ERREUR] Utilisation: %s insertstring table slowns chaine\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t taille_chaine = 0;
    for (int i = 4; i < argc; i++) {
        taille_chaine += strlen(argv[i]) + 1;
    }

    char *chaine = malloc(taille_chaine);
    if (chaine == NULL) {
        perror("[ERREUR] malloc");
        return EXIT_FAILURE;
    }
    
    chaine[0] = '\0';
    for (int i = 4; i < argc; i++) {
        strcat(chaine, argv[i]);
        if (i < argc - 1) {
            strcat(chaine, " ");
        }
    }

    if (insertstring_cmd(argv[2], chaine) == -1) {
        free(chaine);
        return EXIT_FAILURE;
    }
    free(chaine);
    return EXIT_SUCCESS;
}

int handle_insertfile(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "[ERREUR] Utilisation: %s insertfile table slowns nom_fichier\n", argv[0]);
        return EXIT_FAILURE;
    }

    insertfile_cmd(argv[2], argv[4]);
    return EXIT_SUCCESS;
}

int handle_lookup(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    lookup_cmd();
    return EXIT_SUCCESS;
}

int handle_deletehash(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    deletehash_cmd();
    return EXIT_SUCCESS;
}

int handle_delete(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    delete_cmd();
    return EXIT_SUCCESS;
}

int handle_check(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    check_cmd();
    return EXIT_SUCCESS;
}

int handle_extend(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    extend_cmd();
    return EXIT_SUCCESS;
}

int handle_shrink(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    shrink_cmd();
    return EXIT_SUCCESS;
}

int handle_ajust(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    ajust_cmd();
    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "[ERREUR] Utilisation: %s commande table slowns [autres parametres]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *cmd = argv[1];
    int handle;
    if (strcmp(cmd, "make") == 0) {
        handle = handle_make(argc, argv);
    } else if (strcmp(cmd, "free") == 0) {
        handle = handle_free(argc, argv);
    } else if (strcmp(cmd, "insertstring") == 0) {
        handle = handle_insertstring(argc, argv);
    } else if (strcmp(cmd, "insertfile") == 0) {
        handle = handle_insertfile(argc, argv);
    } else if (strcmp(cmd, "lookup") == 0) {
        handle = handle_lookup(argc, argv);
    } else if (strcmp(cmd, "deletehash") == 0) {
        handle = handle_deletehash(argc, argv);
    } else if (strcmp(cmd, "delete") == 0) {
        handle = handle_delete(argc, argv);
    } else if (strcmp(cmd, "check") == 0) {
        handle = handle_check(argc, argv);
    } else if (strcmp(cmd, "extend") == 0) {
        handle = handle_extend(argc, argv);
    } else if (strcmp(cmd, "shrink") == 0) {
        handle = handle_shrink(argc, argv);
    } else if (strcmp(cmd, "ajust") == 0) {
        handle = handle_ajust(argc, argv);
    } else {
        fprintf(stderr, "[ERREUR] Commande %s inconnue.\n", cmd);
        return EXIT_FAILURE;
    }

    print_bloc(argv[2]);
    return handle;
}