CC = gcc
CFLAGS = -Wall -g

all: debugger

debugger: debugger.c
	$(CC) $(CFLAGS) debugger.c -o debugger

clean:
	rm -f debugger
