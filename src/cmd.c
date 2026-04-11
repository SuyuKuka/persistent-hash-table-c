#include "cmd.h"

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

void make_cmd(const char *nom_table, const uint32_t b) {
    if (b == 0) {
        fprintf(stderr, "[ERREUR] make: b ne peut pas etre egal a 0.\n");
        exit(EXIT_FAILURE);
    }

    int fd = open(nom_table, O_CREAT | O_RDWR | O_TRUNC, 0666);
    if (fd < 0) {
        perror("[ERREUR] open (make)");
        exit(EXIT_FAILURE);
    }
    
    size_t taille_table = b * sizeof(struct hache_entree);
    if (ftruncate(fd, taille_table) < 0) {
        perror("[ERREUR] ftruncate (make)");
        close(fd);
        exit(EXIT_FAILURE);
    }

    struct hache_entree *table = mmap(NULL, taille_table, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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

    munmap(table, taille_table);
    close(fd);
}

// TODO: afficher le nombre de chaine, fichiers, objets dans la table
// Structure de la table pas encore adapté pour cela (manque un uint32_t type)
void free_cmd(const char *nom_table) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDONLY, PROT_READ) < 0) {
        close_table(&t);
        exit(EXIT_FAILURE);
    }

    uint32_t bloc_libre = 0;
    for (uint32_t i = 0; i < t.b; i++) {
        if (t.table[i].longueur == BLOC_LIBRE) {
            bloc_libre++;
        }
    }

    printf("[INFO] Blocs libres: %u\n", bloc_libre);
    printf("[INFO] Blocs occupes: %u\n", t.b - bloc_libre);
    printf("[INFO] Taux de remplissage: %.2f%% \n", ((double)(t.b - bloc_libre) / (double)t.b) * 100.0);

    close_table(&t);
}

int insertstring_cmd(const char *nom_table, const char *chaine) {
    struct table_entree t;
    if (open_table(nom_table, &t, O_RDWR, PROT_READ | PROT_WRITE) < 0) {
        close_table(&t);
        exit(EXIT_FAILURE);
    }

    size_t taille_chaine = strlen(chaine);
    int bloc = recherche_bloc(nom_table, t.table, chaine, taille_chaine, t.b);
    if (bloc == -2) {
        fprintf(stderr, "[ERREUR] insertstring: la table est remplie.\n");
        close_table(&t);
        return -1;
    } else if (bloc != -1) {
        fprintf(stderr, "[ALERTE] insertstring: la chaine existe deja dans la table.\n");
        close_table(&t);
        return -1;
    }

    if (bloc_suffisant(t.table, taille_chaine, t.b) < 0) {
        fprintf(stderr, "[ERREUR] insertstring: la table n'a pas assez de blocs pour inserer toutes les donnees.\n");
        close_table(&t);
        return -1;
    }

    size_t offset = 0;
    uint32_t index_actuel;
    uint32_t index_precedent = FIN_CHAINE;
    uint32_t index = h(nom_table, chaine, taille_chaine, t.b);
    while (taille_chaine > 0) {
        size_t taille_actuel;
        if (taille_chaine > ESPACE_MAX) {
            taille_actuel = ESPACE_MAX;
        } else {
            taille_actuel = taille_chaine;
        }
        
        index_actuel = bloc_libre(t.table, t.b, index);
        t.table[index_actuel].premier = index;
        t.table[index_actuel].suivant = FIN_CHAINE;
        t.table[index_actuel].longueur = taille_chaine;
        memcpy(t.table[index_actuel].obj, chaine + offset, taille_actuel);
        
        if (index_precedent != FIN_CHAINE) {
            t.table[index_precedent].suivant = index_actuel;
        }

        taille_chaine -= taille_actuel;
        offset += taille_actuel;
        index_precedent = index_actuel;
    }

    close_table(&t);
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