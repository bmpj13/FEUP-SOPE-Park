CC = gcc
CFLAGS  = -Wall -D_REENTRANT
LIBS = -lpthread
OBJECTS_GER_DEP = src/gerador.c src/utils.c src/utils.h src/common.h
OBJECTS_PAR_DEP = src/parque.c src/utils.c src/utils.h src/common.h
OBJECTS_GER = src/gerador.c src/utils.c
OBJECTS_PAR = src/parque.c src/utils.c
	
all: bin/gerador bin/parque 
	
bin/gerador:	$(OBJECTS_GER_DEP)
		$(CC) $(CFLAGS) $(OBJECTS_GER) -o bin/gerador $(LIBS)

bin/parque:	$(OBJECTS_PAR_DEP)
		$(CC) $(CFLAGS) $(OBJECTS_PAR) -o bin/parque $(LIBS)



