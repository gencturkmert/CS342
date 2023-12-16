all: libvsfs.a create_format app

libvsfs.a: vsfs.o shared_defs.o
	ar -cvq libvsfs.a vsfs.o shared_defs.o
	ranlib libvsfs.a

create_format: create_format.c libvsfs.a
	gcc -Wall -o create_format create_format.c -L. -lvsfs

app: app.c libvsfs.a
	gcc -Wall -o app app.c -L. -lvsfs

vsfs.o: vsfs.c shared_defs.h
	gcc -Wall -c vsfs.c

shared_defs.o: shared_defs.c shared_defs.h
	gcc -Wall -c shared_defs.c

clean:
	rm -f *.o *~ libvsfs.a app create_format
