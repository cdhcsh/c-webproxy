CC = gcc
CFLAGS = -O2 -I ..

all: adder

adder: adder.c
	$(CC) $(CFLAGS) -o adder adder.c ../csapp.o

clean:
	rm -f adder *~
