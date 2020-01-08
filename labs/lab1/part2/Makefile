CC = gcc
CFLAGS = -Wall -ansi -pedantic -g -lm -w
MAIN = pipeit
OBJ =  pipeit.o


all: $(MAIN)

$(MAIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(MAIN) $(OBJ)	
pipeit.o: pipeit.c
	$(CC) $(CFLAGS) -c pipeit.c

clean:
	rm *.o $(MAIN)
