#include "script.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <windows.h>

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "%s [execution amount]\n", argv[0]);
		return 1;
	}
	
	script_t script;
	
	script_init(&script);
	
	script_load_parse_file(&script, "test.txt");
	script_compile(&script);
	
	script_dissassemble(&script, stdout);
	
	LARGE_INTEGER start, end, msec;
	LARGE_INTEGER freq;
	
	QueryPerformanceFrequency(&freq);
	
	QueryPerformanceCounter(&start);
	for(int i = 0; i < strtol(argv[1], NULL, 10); ++i)
	{	
		
		script_run(&script);
	}
	
	QueryPerformanceCounter(&end);
	
	msec.QuadPart = end.QuadPart - start.QuadPart;
	
	msec.QuadPart *= 1000000;
	msec.QuadPart /= freq.QuadPart;
	
	printf("It took %ul us to run\n", msec.QuadPart);
	
	script_destroy(&script);

	return 0;
}
