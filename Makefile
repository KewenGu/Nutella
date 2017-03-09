all: nutella

nutella: nutella.o modules.o msock.o
	gcc -g -Wall nutella.o modules.o msock.o -o nutella -lcrypt

nutella.o: nutella.c nutella.h msock.h
	gcc -g -Wall -c nutella.c

modules.o: modules.c nutella.h msock.h
	gcc -g -Wall -c modules.c

msock.o: msock.c msock.h
	gcc -g -Wall -c msock.c

clean:
	rm -f *.o nutella
