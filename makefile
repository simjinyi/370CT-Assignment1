CC=gcc

main.o: main.c
	$(CC) main.c -pthread -o main.o

clean:
	rm main.o