 
CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -Wall -g
   
main: dine.o
	$(CC) $(CFLAGS) -lpthread -o $@ dine.o

dine.o: dine.c
	$(CC) $(CFLAGS) -fpic -c -o dine.o dine.c

clean:
	rm *.o main