CC=gcc


all: par seq parv2 parv3

par: src/par.c
	$(CC)  -pthread -o bin/par src/par.c

seq: src/seq.c
	$(CC) -pthread -o bin/seq src/seq.c
parv2: src/parv2.c
	$(CC)  -pthread -o bin/parv2 src/parv2.c
parv3: src/parv3.c
	$(CC)  -pthread -o bin/parv3 src/parv3.c

