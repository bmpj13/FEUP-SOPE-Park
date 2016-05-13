CC = gcc
CFLAGS  = -Wall -lpthread -D_REENTRANT -lrt
OBJECTS = gerador.o parque.o utils.o
OBJECTS_GER = utils.o gerador.o
OBJECTS_PAR = utils.o parque.o
	
all: utils.o gerador.o parque.o bin/gerador bin/parque clean

utils.o:	src/utils.c src/utils.h
		cc -c src/utils.c $(CFLAGS)
		
gerador.o:	src/gerador.c src/utils.h src/common.h
		cc -c src/gerador.c $(CFLAGS)
		
parque.o:	src/parque.c src/utils.h src/common.h
		cc -c src/parque.c $(CFLAGS)
		
	
bin/gerador:	$(OBJECTS_GER)
		$(CC) $(OBJECTS_GER) -o bin/gerador $(CFLAGS)

bin/parque:	$(OBJECTS_PAR)
		$(CC) $(OBJECTS_PAR) -o bin/parque $(CFLAGS)

.PHONY: clean
clean :
	-rm *.o