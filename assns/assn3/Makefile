 
CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -Wall -g -L.
   
main: dine.o
	$(CC) $(CFLAGS) -o $@ dine.o

dine.o: dine.c
	$(CC) $(CFLAGS) -lpthread  -fpic -c -o dine.o dine.c

clean:
	rm *.o main