#include "cmd.h"

void make_cmd(const char *nom_table, uint32_t b) {
    if (b == 0) {
        fprintf(stderr, "[ERREUR] make: b ne peut pas etre egal a 0.\n");
        exit(EXIT_FAILURE);
    }

    int fd = open(nom_table, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("[ERREUR] open (make)");
        exit(EXIT_FAILURE);
    }
    
    size_t taille = b * sizeof(struct hache_entree);
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

    munmap(table, taille);
    close(fd);
}

// TODO: afficher le nombre de chaine, fichiers, objets dans la table
// Structure de la table pas encore adapté pour cela (manque un uint32_t type)
void free_cmd(const char *nom_table) {
    int fd = open(nom_table, O_RDONLY);
    if (fd < 0) {
        perror("[ERREUR] open (free)");
        exit(EXIT_FAILURE);
    }

    uint32_t b = get_b(fd);
    size_t taille = b * sizeof(struct hache_entree);
    struct hache_entree *table = mmap(NULL, taille, PROT_READ, MAP_SHARED, fd, 0);
    if (table == MAP_FAILED) {
        perror("[ERREUR] mmap (free)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    uint32_t bloc_libre = 0;
    for (uint32_t i = 0; i < b; i++) {
        if (table[i].longueur == BLOC_LIBRE) {
            bloc_libre++;
        }
    }

    printf("[INFO] Blocs libres: %u\n", bloc_libre);
    printf("[INFO] Blocs occupes: %u\n", b - bloc_libre);
    printf("[INFO] Taux de remplissage: %.2f%% \n", ((double)(b - bloc_libre) / (double)b) * 100.0);

    munmap(table, taille);
    close(fd);
}

int insertstring_cmd(const char *nom_table, const char *chaine) {
    int fd = open(nom_table, O_RDWR);
    if (fd < 0) {
        perror("[ERREUR] open (insertstring)");
        return -1;
    }

    uint32_t b = get_b(fd);
    size_t taille = b * sizeof(struct hache_entree);
    struct hache_entree *table = mmap(NULL, taille, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (table == MAP_FAILED) {
        perror("[ERREUR] mmap (insertstring)");
        close(fd);
        return -1;
    }

    size_t taille_chaine = strlen(chaine);
    int bloc = recherche_bloc(nom_table, table, chaine, taille_chaine, b);
    if (bloc == -2) {
        fprintf(stderr, "[ERREUR] insertstring: la table est remplie.\n");
        munmap(table, taille);
        close(fd);
        return -1;
    } else if (bloc != -1) {
        fprintf(stderr, "[ALERTE] insertstring: la chaine existe deja dans la table.\n");
        munmap(table, taille);
        close(fd);
        return -1;
    }

    if (bloc_suffisant(table, taille_chaine, b) < 0) {
        fprintf(stderr, "[ERREUR] insertstring: la table n'a pas assez de place pour inserer toutes les donnees.\n");
        munmap(table, taille);
        close(fd);
        return -1;
    }

    size_t offset = 0;
    uint32_t index_actuel;
    uint32_t index_precedent = FIN_CHAINE;
    uint32_t index = h(nom_table, chaine, taille_chaine, b);
    while (taille_chaine > 0) {
        size_t taille_actuel;
        if (taille_chaine > ESPACE_MAX) {
            taille_actuel = ESPACE_MAX;
        } else {
            taille_actuel = taille_chaine;
        }
        
        index_actuel = bloc_libre(table, b, index);
        if (index_actuel == BLOC_ERREUR) {
            fprintf(stderr, "[ERREUR] insertstring: la table n'a pas assez de place pour inserer les donnees.\n");
            munmap(table, taille);
            close(fd);
            return -1;
        }

        table[index_actuel].premier = index;
        table[index_actuel].suivant = FIN_CHAINE;
        if (index_precedent != FIN_CHAINE) {
            table[index_precedent].suivant = index_actuel;
        }

        table[index_actuel].longueur = taille_chaine;
        memcpy(table[index_actuel].obj, chaine + offset, taille_actuel);

        taille_chaine -= taille_actuel;
        offset += taille_actuel;
        index_precedent = index_actuel;
    }

    munmap(table, taille);
    close(fd);
    return 0;
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