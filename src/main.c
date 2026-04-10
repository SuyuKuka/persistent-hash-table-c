#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        fprintf(stderr, "[ERREUR] Utilisation: %s commande table slowns [autres parametres]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *cmd = argv[1];
    char *table = argv[2];

    if (strcmp(cmd, "make") == 0) {
        if (argc != 5) {
            fprintf(stderr, "[ERREUR] Utilisation: %s make table slowns b\n", argv[0]);
            return EXIT_FAILURE;
        }

        make_cmd(table, atoi(argv[4]));
    } else if (strcmp(cmd, "free") == 0) {
        if (argc != 4) {
            fprintf(stderr, "[ERREUR] Utilisation: %s free table slowns\n", argv[0]);
            return EXIT_FAILURE;
        }

        free_cmd(table);
    } else if (strcmp(cmd, "insertstring") == 0) {
        insertstring_cmd();
    } else if (strcmp(cmd, "insertfile") == 0) {
        insertfile_cmd();
    } else if (strcmp(cmd, "lookup") == 0) {
        lookup_cmd();
    } else if (strcmp(cmd, "deletehash") == 0) {
        deletehash_cmd();
    } else if (strcmp(cmd, "delete") == 0) {
        delete_cmd();
    } else if (strcmp(cmd, "check") == 0) {
        check_cmd();
    } else if (strcmp(cmd, "extend") == 0) {
        extend_cmd();
    } else if (strcmp(cmd, "shrink") == 0) {
        shrink_cmd();
    } else {
        fprintf(stderr, "[ERREUR] Commande %s inconnue.\n", cmd);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}