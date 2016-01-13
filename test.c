#include "script.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char* argv[])
{
	script_t script;
	
	script_init(&script);
	
	script_load_run_file(&script, "test.txt");
	
	script_destroy(&script);

	return 0;
}
