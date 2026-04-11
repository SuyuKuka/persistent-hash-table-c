#include "cmd.h"

void make_cmd(const char *nom_table, uint32_t b) {
    if (b == 0) {
        fprintf(stderr, "make: b ne peut pas etre egal a 0.\n");
        exit(EXIT_FAILURE);
    }
    
    int fd;
    size_t taille = b * sizeof(struct hache_entree);

    fd = open(nom_table, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open (make)");
        exit(EXIT_FAILURE);
    }
    
    if (ftruncate(fd, taille) < 0) {
        perror("ftruncate (make)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    struct hache_entree *table = mmap(NULL, taille, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (table == MAP_FAILED) {
        perror("mmap (make)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 0; i < b; i++) {
        table[i].premier = i;
        table[i].suivant = FIN_CHAINE;
        table[i].longueur = BLOC_LIBRE;
    }

    if (munmap(table, taille) == -1) {
        perror("munmap (make)");
    }

    close(fd);
}

void free_cmd() {
    
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