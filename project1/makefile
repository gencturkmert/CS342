CC = gcc
CFLAGS = -Wall -Werror

all: primeP primeT

primeP: LinkedList.c primeP.c
	$(CC) $(CFLAGS) -o primeP LinkedList.c primeP.c

primeT: LinkedList.c primeT.c
	$(CC) $(CFLAGS) -o primeT LinkedList.c primeT.c

clean:
	rm -f primeP primeT

.PHONY: all clean
