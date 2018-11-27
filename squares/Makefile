CC=gcc


all: par-squares-g-v2 par-squares-v3 seq-squares

par-squares-g-v2: src/par-squares-g-v2.c
	$(CC) -lm -pthread -o bin/par-squares-g-v2 src/par-squares-g-v2.c

par-squares-v3: src/par-squares-v3.c
	$(CC) -lm -pthread -o bin/par-squares-v3 src/par-squares-v3.c

seq-squares: src/seq-squares.c
	$(CC) -lm -o bin/seq-squares src/seq-squares.c
