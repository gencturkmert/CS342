CC = gcc
CFLAGS = -g

all: primeP

primeP: primeP.o LinkedList.o
	$(CC) $(CFLAGS) -o primeP primeP.o LinkedList.o

primeP.o: primeP.c LinkedList.h
	$(CC) $(CFLAGS) -c primeP.c

LinkedList.o: LinkedList.c LinkedList.h
	$(CC) $(CFLAGS) -c LinkedList.c

clean:
	rm -f primeP primeP.o LinkedList.o
