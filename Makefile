CC = gcc
LDFLAGS = -lfcgi
CFLAGS = -ansi -pedantic -Wall -Werror

all: clog

clog: 
	$(CC) $(CFLAGS) -o clog.out clog.c $(LDFLAGS)
clean:
	@rm clog.out
