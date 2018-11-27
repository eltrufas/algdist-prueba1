CC=gcc


all: par seq

par: src/par.c
	$(CC)  -pthread -o bin/par src/par.c

seq: src/seq.c
	$(CC) -pthread -o bin/seq src/seq.c

