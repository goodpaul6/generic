all: *.c
	gcc test.c script.c vector.c hashmap.c -std=c99 -o test -g -Wall -Iinclude -Llib
