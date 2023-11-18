CC = gcc
CFLAGS = -Wall -g

all: serverk clientk

serverk: serverk.o hash_table.o
	$(CC) $(CFLAGS) -o serverk serverk.o hash_table.o

clientk: clientk.o queue.o
	$(CC) $(CFLAGS) -o clientk clientk.o queue.o

serverk.o: serverk.c shared_defs.h hash_table.h
	$(CC) $(CFLAGS) -c serverk.c

hash_table.o: hash_table.c hash_table.h
	$(CC) $(CFLAGS) -c hash_table.c

clientk.o: clientk.c shared_defs.h queue.h
	$(CC) $(CFLAGS) -c clientk.c

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

clean:
	rm -f serverk clientk *.o
