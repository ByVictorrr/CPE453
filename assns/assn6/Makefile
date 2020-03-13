CC       = gcc
CCFLAGS  = -Wall -ansi -pedantic -g

TARGETS = hello

all: $(TARGETS)

hello: hello.o
	$(CC) -g -o hello hello.o

hello.o: hello.c
	$(CC) -c $(CCFLAGS) -c -o hello.o hello.c
clean:
	rm -f hello.o *~ TAGS
test: all
	@echo "Testing hello..."
	@./hello
	@echo "done."


