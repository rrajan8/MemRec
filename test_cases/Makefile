CC = g++
CFLAGS  = -std=c++11 -Wall -g -I.
LDLIBS ?= ../rvm.a

all:	abort truncate basic multi-abort multi

abort:	abort.c

multi-abort:	multi-abort.c

truncate:	truncate.c

basic:	basic.c

multi:	multi.c

clean:
	rm -f multi abort truncate basic multi multi-abort
	