CC = gcc
LDFLAGS = -lfcgi
CFLAGS = -g -ansi -pedantic -Wall

all: clog

clog: 
	$(CC) $(CFLAGS) -o clog.out clog.c
clean:
	@rm clog.out
