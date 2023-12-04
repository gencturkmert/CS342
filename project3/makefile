CC = gcc
CFLAGS = -Wall -Wextra -std=c99

memsim: memsim.c ClockList.c
	$(CC) $(CFLAGS) -o memsim memsim.c ClockList.c -lm

.PHONY: clean
clean:
	rm -f memsim
