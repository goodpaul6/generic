all: *.c
	gcc test.c script.c vector.c hashmap.c script_iup_interface.c -std=c99 -o test -g -Wall -Iinclude -Llib -liup -lgdi32 -lcomdlg32 -lcomctl32 -luuid -loleaut32 -lole32
