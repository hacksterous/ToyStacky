CC = gcc
CFLAGS = -Os -Wall -Wextra

SRC = linenoise.c utf8.c
OBJ = $(SRC:.c=.o)
INC = linenoise.h utf8.h

all: toystacky

toystacky: bigint.c linenoise.h toystacky.c bigint.h utf8.c linenoise.c utf8.h
	$(CC) -Wall -Os utf8.c linenoise.c bigint.c toystacky.c -o $@ -lm

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f toystacky toystacky.o linenoise.o bigint.o utf8.o
