TARGET   = a.out
CC       = gcc
LD       = gcc
CCFLAGS  = -pedantic -Wall -fpic -g -lm
LDFLAGS  = -Wall -g -L. -lm
SOURCES  = $(wildcard *.c)
INCLUDES = $(wildcard *.h)
OBJECTS  = $(SOURCES:.c=.o)


LS_OBJECTS  = minls.o types.o parser.o path.o shared.o
GET_OBJECTS  = minget.o types.o parser.o

TARGETS = minls minget


all: $(TARGETS)

minls: $(LS_OBJECTS)
	$(CC) -o $@ $(LDFLAGS) $(LS_OBJECTS)

#minget: $(GET_OBJECTS)
#	$(CC) -o $@ $(LDFLAGS) $(GET_OBJECTS)


#minls.o: minls.c
#$(CC) $(CCFLAGS) -fpic -c -o $@ minls.c

#minget.o: minget.c
#$(CC) $(CCFLAGS) -fpic -c -o $@ minget.c

#minfs.o: minfs.c
#$(CC) $(CCFLAGS) -fpic -c -o $@ minfs.c

$(OBJECTS): $(SOURCES) $(INCLUDES)
	$(CC) -c $(CCFLAGS) $(SOURCES)

gdb: minls
	gdb -tui --args minls Images/SmallBlocks--1k /DirOne/FileOne

clean:
	rm -f $(TARGET) $(TARGETS) $(OBJECTS)
