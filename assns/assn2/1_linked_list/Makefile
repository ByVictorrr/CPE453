CC = gcc
CFLAGS = -Wall -g 
LD = gcc
LDFLAGS = -Wall -g -L.
   
main: lwp.o magic64.o scheduler.o tests/14/fiveprocsmain.o
	$(CC) $(CFLAGS) -o $@ lwp.o magic64.o scheduler.o fiveprocsmain.o


liblwp.so: lwp.o magic64.o scheduler.o 
	$(CC) $(CFLAGS) -fPIC -shared -o $@ lwp.o magic64.o scheduler.o

lwp.o: lwp.c lwp.h scheduler.h
	$(CC) $(CFLAGS) -fpic -c -o lwp.o lwp.c

scheduler.o: scheduler.h lwp.h
	$(CC) $(CFLAGS) -fpic -c -o scheduler.o scheduler.c

magic64.o: magic64.S
	$(CC) $(CFLAGS) -fpic -c -o magic64.o magic64.S

# Tests BELOW THIS POINT
tests/14/fiveprocsmain.o: tests/14/fiveprocsmain.c lwp.h
	$(CC) $(CFLAGS) -fpic -c -o fiveprocsmain.o tests/14/fiveprocsmain.c

clean:
	rm *.o main *.so *.a core*
