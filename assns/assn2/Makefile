CC = gcc
CFLAGS = -Wall -g 
LD = gcc
LDFLAGS = -Wall -g -L.
   
main: lwp.o magic64.o
	$(CC) $(CFLAGS) -o $@ lwp.o magic64.o 

liblwp.so: lwp.o magic64.o 
	$(CC) $(CFLAGS) -fPIC -shared -o $@ lwp.o magic64.o

lwp.o: lwp.c
	$(CC) $(CFLAGS) -fpic -c -o lwp.o lwp.c

magic64.o: magic64.S
	$(CC) $(CFLAGS) -fpic -c -o magic64.o magic64.S

numbersmain.o: 	numbersmain.c
	$(CC) $(CFLAGS) -fpic -c -o numbersmain.o numbersmain.c

clean:
	rm *.o main
