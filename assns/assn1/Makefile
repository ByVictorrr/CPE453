CC = gcc
CFLAGS = -g
OBJ =  malloc.o

all : $(OBJ) 
	ar r malloc.a $(OBJ) 

malloc.o : malloc.c 
	$(CC) $(CFLAGS) -c malloc.c

