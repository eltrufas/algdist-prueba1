CC=clang


all: generate-stars stars-row stars-col stars-row-par stars-col-par

generate-stars: src/generate-stars.c
	$(CC) -o bin/generate-stars src/generate-stars.c

stars-row: src/stars.c
	$(CC) -o bin/stars-row src/stars.c

stars-col: src/stars-column.c
	$(CC) -o bin/stars-col src/stars-column.c

stars-row-par: src/stars-par.c
	$(CC) -pthread -o bin/stars-row-par src/stars-par.c

stars-col-par: src/stars-column-par.c
	$(CC) -pthread -o bin/stars-col-par src/stars-column-par.c

