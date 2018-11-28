#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define SILENT 0
#define VERBOSE 1

struct SetNode {
    char val;
    struct SetNode *next;
};

struct Set {
    size_t size;
    struct SetNode *first;
    struct Set *next;
};

void usage(char **argv) {
    printf("Usage: %s n\n", argv[0]);
    exit(1);
}

// Factorial de n
int fact(int n){
    return n > 0 ? n * fact(n - 1) : 1;
};

// Calcula combinaciones de n en k
uint32_t ncr(uint32_t n, uint32_t k) {
    int result, i;
    if (k > n) {
        return 0;
    }

    if (k * 2 > n) {
        k = n-k;
    }

    if (k == 0) {
        return 1;
    }

    result = n;
    for (i = 2; i <= k; i++) {
        result *= (n - i + 1);
        result /= i;
    }
    return result;
}

struct SetNode *get_next_node(struct SetNode *nodes, size_t *next_idx) {
    struct SetNode *node = nodes + *next_idx;
    (*next_idx)++;

    return node;
}

void push_node_front(struct Set *set, struct SetNode *val) {
    val->next = set->first;
    set->first = val;
    set->size++;
}

void push_set_front(struct Set **setlist, struct Set *set) {
    set->next = (*setlist);
    *setlist = set;
}

void print_set(struct Set *set) {
    struct SetNode *p;
    printf("{ ");
    p = set->first;
    while (p) {
        printf("%d ", p->val);
        p = p->next;
    }
    printf("}\n");
}

int cmp_sets(struct Set *a, struct Set *b) {
    struct SetNode *p, *q;

    if (a->size < b->size) {
        return -1;
    }
    
    if (a->size > b->size) {
        return 1;
    }

    p = a->first;
    q = b->first;

    while (p && q) {
        if (p->val < q->val) {
            return -1;
        }
        if (p->val > q->val) {
            return 1;
        }

        p = p->next;
        q = q->next;
    }

    return 0;
}

void print_holes(size_t *holes, size_t n) {
    size_t i;
    printf("Hole sizes: ");
    for (i = 0; i < n; i++) {
        printf("%lu ", holes[i]);
    }
    printf("\n");
}

int main(int argc, char** argv) {
    uint32_t n, mode;
    uint32_t n_sets;
    size_t i, k, set_size, l;
    size_t total_nodes;
    size_t next_node;
    size_t *hole_sizes;
    struct SetNode *nodes, *p, *q;
    struct Set *sets, *sets_ordered, *r;
    struct Set input_set;
    struct SetNode *input_set_nodes;
    struct Set **sets_per_size;
    long t0, t1;
    clock_t c0, c1;
    struct timeval timecheck;

    if (argc != 3) {
        usage(argv);
    }

    if (strcmp(argv[1],"-S") == 0) {
        mode = SILENT;
    }

    if (strcmp(argv[1],"-V") == 0) {
        mode = VERBOSE;
    }

    n = atoi(argv[2]);
    n_sets = 1 << n;

    // Inicializar la lista de entrada al reves
    input_set_nodes = calloc(n_sets, sizeof(struct Set*));
    for (i = 0; i < n - 1; i++) {
        input_set_nodes[i].val = i + 1;
        input_set_nodes[i + 1].next = input_set_nodes + i;
    }
    input_set_nodes[n - 1].val = n;

    input_set.size = n;
    input_set.first = input_set_nodes + n - 1;

    if (mode == VERBOSE) {
        printf("Conjunto de entrada: ");
        print_set(&input_set);
    }

    // Inicializar el arreglo de conjuntos
    sets = calloc(n_sets, sizeof(struct Set));
    sets_ordered = calloc(n_sets, sizeof(struct Set));

    // Calcular el numero total de nodos que se necesitaran
    total_nodes = 0;
    for (i = 0; i <= n; i++) {
        total_nodes += ncr(n, i) * (i * 10);
    }

    // Alocamos todos los nodos que vamos a ocupar
    next_node = 0;
    nodes = calloc(total_nodes, sizeof(struct SetNode));

    // Creamos una arreglo de conjuntos de conjuto agrupados por tamano
    sets_per_size = calloc(n + 1, sizeof(struct Set*));
    hole_sizes = calloc(n + 1, sizeof(size_t));

    c0 = clock();
    // Este metodo para obtener el tiempo de pared tiene mucha mejor
    // resoluci'on que time
    // https://stackoverflow.com/a/45769714
    gettimeofday(&timecheck, 0);
    t0 = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;

    // Generamos los conjuntos fuera de orden
    for (i = 0; i < n_sets; i++) {
        p = input_set.first;
        k = i;

        sets[i].size = 0;
        sets[i].first = 0;
        // Consideramos el bit n para indicar si el n-esimo elemento
        // del conjunto de entrada est'a en el subconjunto 
        while (p) {
            if (k & 1) {
                q = calloc(1, sizeof(struct SetNode));
                q->val = p->val;
                push_node_front(sets + i, q);
            }
            p = p->next;
            k >>= 1;
        }

        set_size = sets[i].size;
        push_set_front(sets_per_size + set_size, sets + i);
    }


    k = 0;
    for (i = 0; i <= n; i++) {
        l = k;
        r = sets_per_size[i];
        while (r) {
            sets_ordered[k] = *r;
            k += 1;
            r = r->next;
        }
    }

    gettimeofday(&timecheck, 0);
    t1 = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;
    c1 = clock();

    if (mode == VERBOSE) {
        for (i = 0; i < n_sets; i++) {
            print_set(sets_ordered + i);
        }
    }

    printf ("wall time: %f\n", ((double)(t1 - t0)) / 1000);
    printf ("cpu time: %f\n", (float) (c1 - c0)/CLOCKS_PER_SEC);

    return 0;
}


