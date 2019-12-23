#This is the makefile for prime-finder
CFLAGS =-std=gnu89 -Wpedantic


build: src/prime-finder.c
	gcc $(CFLAGS) -o bin/prime-finder src/prime-finder.c -pthread -lm

debug: src/prime-finder.c
	gcc $(CFLAGS) -g -fsanitize=address -o bin/prime-finder src/prime-finder.c -pthread -lm