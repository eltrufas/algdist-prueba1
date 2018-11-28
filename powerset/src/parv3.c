/*
 * parv3.c: Calcula el conjunto potencia del conjunto con los numeros
 * naturales de 1..n usando k hilos.
 *
 * Esta versi'on escribe en la memoria compartida por lotes de conjuntos
 * para reducir la cantidad de alocaciones de memoria aun mas. El tamano del
 * lote se determina por la constante BATCH_SIZE.
 *
 * Autor: Rafael A. Castillo 
 *
 * Santiago de Chile: 2018-11-28
 */
 
#include <sys/types.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include <string.h>

#define SILENT 0
#define VERBOSE 1

// Define que tan seguido se escribe en memoria compartida
#define BATCH_SIZE 80

struct SetNode {
    char val;
    struct SetNode *next;
};

struct Set {
    size_t id;
    size_t size;
    struct SetNode *first;
    struct Set *next;
};

struct ThreadMsg {
    size_t id;
    size_t n_threads;
    size_t n_sets;
    struct Set *sets;
    struct Set **sets_per_size;
    struct Set *input_set;
    struct SetNode *temp_buf;
    struct NodeAlloc *alloc;
    uint32_t opmode;
};

struct SortMsg {
    size_t id;
    size_t n_threads;
    size_t n;
    struct Set* sets;
    uint32_t opmode;
    // Arreglo con todos los resultados de la generacion de conjuntos
    struct ThreadMsg *work;
};

int cmp_sets(struct Set *a, struct Set *b);
void insertion_sort(struct Set *sets, size_t start, size_t end);

/**
 * Alocador de memoria por bloque sincronizado.
 * Solo puede darte un nodo, no puede liberarlo.
 * No importa en este caso.
 *
 * Esto realmente no es necesario, solo lo hice
 * para evitar llamadas a calloc.
 */
struct NodeAlloc {
    struct SetNode *nodes;
    size_t next_idx;
    pthread_mutex_t lock;
};

/**
 * Crea un nuevo alocador de memoria con cap bloques de memoria.
 */
struct NodeAlloc init_node_alloc(size_t cap) {
    struct NodeAlloc alloc;
    alloc.nodes = calloc(cap, sizeof(struct SetNode));
    if (pthread_mutex_init(&alloc.lock, 0) != 0) {
        printf("\n mutex init failed\n");
        exit(1);
    }

    return alloc;
}

void destroy_node_alloc(struct NodeAlloc *alloc) {
    pthread_mutex_destroy(&alloc->lock);
    free(alloc->nodes);
}

/**
 * Regresa un nodo del alocador
 */
struct SetNode *alloc_node(struct NodeAlloc *alloc) {
    struct SetNode *node;
    pthread_mutex_lock(&alloc->lock);

    node = alloc->nodes + alloc->next_idx;
    alloc->next_idx++;

    pthread_mutex_unlock(&alloc->lock);

    return node;
}

/**
 * Pide espacio para n nodos al alocador
 */
struct SetNode *alloc_n(struct NodeAlloc *alloc, size_t n) {
    struct SetNode *node;
    pthread_mutex_lock(&alloc->lock);

    node = alloc->nodes + alloc->next_idx;
    alloc->next_idx += n;

    pthread_mutex_unlock(&alloc->lock);

    return node;
}

/**
 * Mueve los nodos de la lista del conjunto
 * a otra locacion de memoria
 */
void move_set(struct Set *set, struct SetNode *space) {
    struct SetNode *p;
    size_t i;

    if (!set->size) {
        return;
    }

    p = set->first;

    i = 0;
    while (p) {
        space[i].val = p->val;
        space[i].next = space + i + 1;
        i++;
        p = p->next;
    }

    space[i-1].next = 0;
    set->first = space;
}

/**
 * Mueve n conjuntos del buffer de memoria temporal a la memoria
 * del alocador
 */
void commit_batch(struct NodeAlloc *alloc, struct Set *sets, size_t n_sets) {
    size_t total_size, i, k;
    struct SetNode *final_nodes;
    
    // Calculamos la cantidad total de nodos de todos los conjuntos
    total_size = 0;
    for (i = 0; i < n_sets; i++) {
        total_size += sets[i].size;
    }

    // Solicitamos toda la memoria que vamos a necesitar
    final_nodes = alloc_n(alloc, total_size);

    // Movemos los nodos a memoria mas permanente.
    k = 0;
    for (i = 0; i < n_sets; i++) {
        move_set(sets + i, final_nodes + k);
        k += sets[i].size;
    }
}

/**
 * Imprime un conjunto a stdout
 */
void print_set(struct Set *set) {
    struct SetNode *p;
    printf("{ ");
    p = set->first;
    while (p) {
        printf("%d ", p->val);
        p = p->next;
    }
    printf("} %lu %lu\n", set->size, set->id);
}


void usage(char **argv) {
    printf("Usage: %s [-S/-V] n k\n", argv[0]);
    exit(1);
}

// Calcula combinaciones de n en k
size_t  ncr(size_t n, size_t k) {
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

/**
 * Inserta un elemento al principio de un conjunto
 */
void push_node_front(struct Set *set, struct SetNode *val) {
    val->next = set->first;
    set->first = val;
    set->size++;
}

/**
 * Inserta un conjunto al principio de una lista de conjuntos
 */
void push_set_front(struct Set **setlist, struct Set *set) {
    set->next = (*setlist);
    *setlist = set;
}

void merge(struct Set *sets, size_t start, size_t mid, size_t end, struct Set *dest);
void merge_split(struct Set *sets, size_t start, size_t end, struct Set *dest);
void merge_sort(struct Set *sets, size_t start, size_t end);

void *thread_work(void *d) {
    size_t i, k, set_size, work_size, end_idx, temp_idx, batch_idx;
    struct SetNode *p, *q;
    struct ThreadMsg msg;

    msg = *(struct ThreadMsg*) d;

    work_size = msg.n_sets / msg.n_threads;
    end_idx = (msg.id + 1) * work_size;
    if (end_idx >= msg.n_sets) {
        end_idx = msg.n_sets;
    }

    batch_idx = msg.id * work_size;

    for (i = msg.id * work_size; i < end_idx; i++) {
        p = msg.input_set->first;
        k = i;

        if (msg.opmode == VERBOSE) {
            printf("[THREAD %lu] getting set %lu\n", msg.id, i);
        }

        temp_idx = 0;

        msg.sets[i].size = 0;
        msg.sets[i].first = 0;
        msg.sets[i].id = i;
        // Consideramos el bit n para indicar si el n-esimo elemento
        // del conjunto de entrada est'a en el subconjunto 
        while (p) {
            if (k & 1) {
                // Construimos los conjuntos en un buffer de memoria temporal
                // Para reducir cuantas veces pedimos memoria.
                q = msg.temp_buf + temp_idx++;
                q->val = p->val;
                push_node_front(msg.sets + i, q);
            }
            p = p->next;
            k >>= 1;
        }

        // Una vez que hayamos construido cierta cantidad de conjuntos
        // en nuestro buffer temporal, 
        if (i - batch_idx >= BATCH_SIZE) {
            commit_batch(msg.alloc, msg.sets + batch_idx, BATCH_SIZE);
            batch_idx += BATCH_SIZE;
        }

        set_size = msg.sets[i].size;
        push_set_front(msg.sets_per_size + set_size, msg.sets + i);
    }

    commit_batch(msg.alloc, msg.sets + batch_idx, i - batch_idx);

    pthread_exit(0);
}

/**
 * Funcion a correr en un hilo que ordena una porcion del conjunto
 */
void *thread_sort(void *d) {
    size_t i, j, k, m;
    struct Set *r;
    struct SortMsg msg;
    msg = *(struct SortMsg*) d;

    if (msg.opmode == VERBOSE) {
        printf("[THREAD %lu] Starting\n", msg.id);
    }

    k = 0;
    for (i = 0; i <= msg.n; i++) {
        // Este calculo distribuye los tamaños entre hilos lo mejor que se puede
        if (i % msg.n_threads == msg.id) {
            m = k;
            if (msg.opmode == VERBOSE) {
                printf("[THREAD %lu] Sorting %lu with k=%lu\n", msg.id, i, m);
            }
            for (j = 0; j < msg.n_threads; j++) {
                r = msg.work[j].sets_per_size[i];
                while(r) {
                    msg.sets[k++] = *r;
                    r = r->next;
                }
            }
            merge_sort(msg.sets, m, k);
        } else {
            k += ncr(msg.n, i);
        }

    }

    pthread_exit(0);
}

/* Las siguientes tres funciones son una implementaci'on vieja de merge sort */

void merge(struct Set *sets, size_t start, size_t mid, size_t end, struct Set *dest) {
    size_t i, j, k;

    i = start;
    j = mid;

    for (k = start; k < end; k++) {
        if (i < mid && (j >= end || sets[i].id > sets[j].id)) {
            dest[k] = sets[i];
            i++;
        } else {
            dest[k] = sets[j];
            j++;
        }
    }
}

void merge_split(struct Set *sets, size_t start, size_t end, struct Set *dest) {
    size_t mid, i;
    if (end - start < 2) {
        return;
    }

    mid = start + (end - start) / 2;

    merge_split(dest, start, mid, sets);
    merge_split(dest, mid, end, sets);


    merge(sets, start, mid, end, dest);

}

void merge_sort(struct Set *sets, size_t start, size_t end) {
    struct Set *temp;
    size_t i;
    sets = sets + start;
    end -= start;
    start = 0;

    if (end < 2) {
        return;
    }

    temp = calloc(end - start, sizeof(struct Set));
    for (i = 0; i < end; i++) {
        temp[i] = sets[i];
    }

    merge_split(temp, start, end, sets);

    free(temp);
}

int main(int argc, char** argv) {
    uint32_t n, mode;
    size_t n_sets;
    size_t i, k;
    void *exit_status;
    size_t total_nodes;
    pthread_t *threads;
    pthread_attr_t attribute;
    struct Set *sets, *sets_ordered;
    struct Set input_set;
    struct SetNode *input_set_nodes;
    struct NodeAlloc alloc;
    struct ThreadMsg *msgs;
    struct SortMsg *sort_msgs;
    long t0, t1;
    clock_t c0, c1;
    struct timeval timecheck;

    if (argc != 4) {
        usage(argv);
    }

    if (strcmp(argv[1],"-S") == 0) {
        mode = SILENT;
    }

    if (strcmp(argv[1],"-V") == 0) {
        mode = VERBOSE;
    }

    n = atoi(argv[2]);
    k = atoi(argv[3]);
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

    // Inicializamos los hilos
    threads = calloc(k,sizeof(pthread_t));

    // Inicializamos los mensajes para generar conjuntos
    msgs = calloc(k, sizeof(struct ThreadMsg));
    for (i = 0; i < k; i++) {
        msgs[i].sets_per_size = calloc(n + 1, sizeof(struct Set*));
        msgs[i].temp_buf = calloc(n * BATCH_SIZE, sizeof(struct SetNode));
    }

    // Inicializamos los mensajes para ordenar
    sort_msgs = calloc(k, sizeof(struct SortMsg)); 

    // Inicializamos nuestro alocador de nodos sincronziado
    alloc = init_node_alloc(total_nodes);

    c0 = clock();
    // Este metodo para obtener el tiempo de pared tiene mucha mejor
    // resoluci'on que time
    // https://stackoverflow.com/a/45769714
    gettimeofday(&timecheck, 0);
    t0 = (long)timecheck.tv_sec * 1000 + (long)timecheck.tv_usec / 1000;

    pthread_attr_init(&attribute);
    pthread_attr_setdetachstate(&attribute, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < k; i++) {
        msgs[i].id = i;
        msgs[i].n_threads = k;
        msgs[i].n_sets = n_sets;
        msgs[i].sets = sets;
        msgs[i].input_set = &input_set;
        msgs[i].alloc = &alloc;
        msgs[i].opmode = mode;

        pthread_create(threads + i,&attribute, thread_work, (void*) (msgs + i));
    }
    
    for (i = 0; i < k; i = i + 1) {
        pthread_join(threads[i], &exit_status);
    }

    if (mode == VERBOSE) {
        for (i = 0; i < n_sets; i++) {
            print_set(sets + i);
        }
    }

    for (i = 0; i < k; i++) {
        sort_msgs[i].id = i;
        sort_msgs[i].work = msgs;
        sort_msgs[i].n_threads = k;
        sort_msgs[i].n = n;
        sort_msgs[i].sets = sets_ordered;
        sort_msgs[i].opmode = mode;

        pthread_create(
            threads + i,
            &attribute,
            thread_sort,
            (void*) (sort_msgs + i)
        );
    }

    for (i = 0; i < k; i = i + 1) {
        pthread_join(threads[i], &exit_status);
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

    destroy_node_alloc(&alloc);

    return 0;
}

