CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

# Target 'all' should build the executable
.PHONY: all
all: nyuenc

# The executable 'nyuenc' depends on 'nyuenc.o'
nyuenc: nyuenc.o
	$(CC) $(CFLAGS) nyuenc.o -o nyuenc

# The object file 'nyuenc.o' depends on 'nyuenc.c'
nyuenc.o: nyuenc.c
	$(CC) $(CFLAGS) -c nyuenc.c

# Clean the build directory
.PHONY: clean
clean:
	rm -f *.o nyuenc
