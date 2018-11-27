#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

void usage(char *binary) {
    printf("usage: %s mat_size\n", binary);
    puts("\tmat_size: Dimension of the (square) matrix to generate");
}

int main(int argc, char **argv) {
    uint32_t dim, i, n, b;

    srand(time(0));

    if (argc == 2) {
        dim = atoi(argv[1]);
        
        printf("%u\n", dim);
        printf("%u\n", dim);

        n = dim * dim;
        for (i = 0; i < n; i++) {
            b = rand() % 24;
            b = b >= 16 ? 0 : b;
            printf("%u\n", b);
        }

    } else {
        usage(argv[0]);
    }

    return 0;
}
