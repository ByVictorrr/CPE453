CC = gcc
CFLAGS = -Wall -g 
LD = gcc
LDFLAGS = -Wall -g -L.
   
main: lwp.o magic64.o scheduler.o tests/28/prog.o tests/28/handsoff.o
	$(CC) $(CFLAGS) -o $@ lwp.o magic64.o scheduler.o prog.o handsoff.o
	#prog.o handsoff.o

liblwp.so: lwp.o magic64.o scheduler.o 
	$(CC) $(CFLAGS) -fPIC -shared -o $@ lwp.o magic64.o scheduler.o

lwp.o: lwp.c lwp.h scheduler.h
	$(CC) $(CFLAGS) -fpic -c -o lwp.o lwp.c

scheduler.o: scheduler.h lwp.h
	$(CC) $(CFLAGS) -fpic -c -o scheduler.o scheduler.c

magic64.o: magic64.S
	$(CC) $(CFLAGS) -fpic -c -o magic64.o magic64.S

# Tests BELOW THIS POINT

tests/28/prog.o: tests/28/prog.c lwp.h	
	$(CC) $(CFLAGS) -fpic -c -o prog.o tests/28/prog.c

tests/28/handsoff.o: tests/28/handsoff.h tests/28/handsoff.c lwp.h 
	$(CC) $(CFLAGS) -fpic -c -o handsoff.o tests/28/handsoff.c


#tests/14/fiveprocsmain.o: tests/14/fiveprocsmain.c lwp.h
	#$(CC) $(CFLAGS) -fpic -c -o fiveprocsmain.o tests/14/fiveprocsmain.c


#tests/22/rr.o: tests/22/rr.c lwp.h tests/22/rr.h
	#$(CC) $(CFLAGS) -fpic -c -o rr.o tests/22/rr.c

#tests/22/main.o: tests/22/main.c lwp.h tests/22/rr.h
	#$(CC) $(CFLAGS) -fpic -c -o main.o tests/22/main.c



clean:
	rm *.o main *.so *.a core* DETAILS*
