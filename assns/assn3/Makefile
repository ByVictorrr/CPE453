 
CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -Wall -g
   
dine: dine.o
	$(CC) $(CFLAGS) -lpthread -o $@ dine.o

dine.o: dine.c
	$(CC) $(CFLAGS) -c -o dine.o dine.c

clean:
	rm *.o dine