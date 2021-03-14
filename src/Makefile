CC=gcc
CFLAGS=-W -Wall `pkg-config --cflags libwebsockets` -ggdb
LDFLAGS=-lm -lpthread `pkg-config --libs libwebsockets`
EXEC=ananke-srv
SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

all: $(EXEC)

ananke-srv: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

*.o: *.c
	$(CC) -o $@ -c $< $(CLFAGS)

.PHONY: clean mrproper

clean: 
	rm -rf *.o

mrproper:
	rm -rf $(EXEC)

