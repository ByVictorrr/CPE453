CC = gcc
CFLAGS = -Wall -g 
LD = gcc
LDFLAGS = -Wall -g -L.
   
main: lwp.o magic64.o tests/17_test.o
	$(CC) $(CFLAGS) -o $@ lwp.o magic64.o 17_test.o

#libPLN.a: lwp.c 
#	gcc -c lwp.c
#	ar r libPLN.a lwp.o
#	rm lwp.o


liblwp.so: lwp.o magic64.o 
	$(CC) $(CFLAGS) -fPIC -shared -o $@ lwp.o magic64.o

lwp.o: lwp.c lwp.h
	$(CC) $(CFLAGS) -fpic -c -o lwp.o lwp.c

#fiveprocsmain.o: fiveprocsmain.c
	#$(CC) $(CFLAGS) -fpic -c -o fiveprocsmain.o fiveprocsmain.c

tests/17_test.o: tests/17_test.c lwp.h
	$(CC) $(CFLAGS) -fpic -c -o  17_test.o tests/17_test.c



magic64.o: magic64.S
	$(CC) $(CFLAGS) -fpic -c -o magic64.o magic64.S

#numbersmain.o: 	numbersmain.c
	#$(CC) $(CFLAGS) -fpic -c -o numbersmain.o numbersmain.c

clean:
	rm *.o main *.so *.a core*
