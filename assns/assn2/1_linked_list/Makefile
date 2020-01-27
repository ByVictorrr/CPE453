CC = gcc
CFLAGS = -Wall -g 
LD = gcc
LDFLAGS = -Wall -g -L.
   
main: lwp.o magic64.o tests/26/26_test.o tests/26/random.o scheduler.o
	$(CC) $(CFLAGS) -o $@ lwp.o magic64.o 26_test.o random.o scheduler.o


liblwp.so: lwp.o magic64.o scheduler.o 
	$(CC) $(CFLAGS) -fPIC -shared -o $@ lwp.o magic64.o scheduler.o

lwp.o: lwp.c lwp.h scheduler.h
	$(CC) $(CFLAGS) -fpic -c -o lwp.o lwp.c

scheduler.o: scheduler.h lwp.h
	$(CC) $(CFLAGS) -fpic -c -o scheduler.o scheduler.c
#fiveprocsmain.o: fiveprocsmain.c
	#$(CC) $(CFLAGS) -fpic -c -o fiveprocsmain.o fiveprocsmain.c

tests/26/26_test.o: lwp.h tests/26/schedulers.h 
	$(CC) $(CFLAGS) -fpic -c -o  26_test.o tests/26/26_test.c 

tests/26/random.o: 	lwp.h tests/26/schedulers.h 
	$(CC) $(CFLAGS) -fpic -c -o  random.o tests/26/random.c



magic64.o: magic64.S
	$(CC) $(CFLAGS) -fpic -c -o magic64.o magic64.S

#numbersmain.o: 	numbersmain.c
	#$(CC) $(CFLAGS) -fpic -c -o numbersmain.o numbersmain.c

clean:
	rm *.o main *.so *.a core*
