#include "cmd.h"

uint32_t get_b(int fd) {
    struct stat st;
    uint32_t b;

    if (fstat(fd, &st) == -1) {
        perror("[ERREUR] fstat (get_b)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (st.st_size % sizeof(struct hache_entree) != 0) {
        fprintf(stderr, "[ERREUR] get_b: la table est corrompue.\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    b = st.st_size / sizeof(struct hache_entree);
    if (b == 0) {
        fprintf(stderr, "[ERREUR] get_b: la table est vide.\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return b;
}

void make_cmd(const char *nom_table, uint32_t b) {
    if (b == 0) {
        fprintf(stderr, "[ERREUR] make: b ne peut pas etre egal a 0.\n");
        exit(EXIT_FAILURE);
    }

    int fd;
    size_t taille = b * sizeof(struct hache_entree);

    fd = open(nom_table, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("[ERREUR] open (make)");
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(fd, taille) < 0) {
        perror("[ERREUR] ftruncate (make)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    struct hache_entree *table = mmap(NULL, taille, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (table == MAP_FAILED) {
        perror("[ERREUR] mmap (make)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < b; i++) {
        table[i].premier = i;
        table[i].suivant = FIN_CHAINE;
        table[i].longueur = BLOC_LIBRE;
    }

    if (munmap(table, taille) == -1) {
        perror("[ERREUR] munmap (make)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

// TODO: afficher le nombre de chaine, fichiers, objets dans la table
// Structure de la table pas encore adapté pour cela (manque un uint32_t type)
void free_cmd(const char *nom_table) {
    int fd;
    size_t taille;
    uint32_t b;
    uint32_t bloc_libre = 0;

    fd = open(nom_table, O_RDONLY);
    if (fd < 0) {
        perror("[ERREUR] open (free)");
        exit(EXIT_FAILURE);
    }

    b = get_b(fd);
    taille = b * sizeof(struct hache_entree);
    struct hache_entree *table = mmap(NULL, taille, PROT_READ, MAP_SHARED, fd, 0);
    if (table == MAP_FAILED) {
        perror("[ERREUR] mmap (free)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < b; i++) {
        if (table[i].longueur == BLOC_LIBRE) {
            bloc_libre++;
        }
    }

    printf("[INFO] Blocs libres: %u\n", bloc_libre);
    printf("[INFO] Blocs occupes: %u\n", b - bloc_libre);
    printf("[INFO] Taux de remplissage: %.2f%% \n", ((double)(b - bloc_libre) / (double)b) * 100.0);

    if (munmap(table, taille) == -1) {
        perror("[ERREUR] munmap (free)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

void insertstring_cmd() {

}

void insertfile_cmd() {

}

void lookup_cmd() {

}

void deletehash_cmd() {
    
}

void delete_cmd() {
    
}

void check_cmd() {
    
}

void extend_cmd() {
    
}

void shrink_cmd() {
    
}

void ajust_cmd(){
    
}