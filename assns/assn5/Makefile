TARGET   = a.out
CC       = gcc
LD       = gcc
CCFLAGS  = -pedantic -Wall -Werror -fpic
LDFLAGS  = -Wall -g -L. -lm
SOURCES  = $(wildcard *.c)
INCLUDES = $(wildcard *.h)
OBJECTS  = $(SOURCES:.c=.o)


LS_OBJECTS  = minls.o minfs.o
GET_OBJECTS  = minget.o minfs.o



all: minls minget

minls: $(LS_OBJECTS)
	$(CC) -o $@ $(LDFLAGS) $(LS_OBJECTS)

minget: $(GET_OBJECTS)
	$(CC) -o $@ $(LDFLAGS) $(GET_OBJECTS)


#minls.o: minls.c
#$(CC) $(CCFLAGS) -fpic -c -o $@ minls.c

#minget.o: minget.c
#$(CC) $(CCFLAGS) -fpic -c -o $@ minget.c

#minfs.o: minfs.c
#$(CC) $(CCFLAGS) -fpic -c -o $@ minfs.c

$(OBJECTS): $(SOURCES) $(INCLUDES)
	$(CC) -c $(CCFLAGS) $(SOURCES)

clean:
	rm -f $(TARGET) $(OBJECTS)
