CC=gcc

all: main.o openmp.o

main.o: main.c
	$(CC) main.c -pthread -o main.o

openmp.o: openmp.c
	$(CC) openmp.c -pthread -fopenmp -o openmp.o

clean:
	rm main.o openmp.o
