#include "script.h"
#include "script_iup_interface.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char* argv[])
{
	script_t script;
	
	script_init(&script);
	
	script_bind_iup(&script, argc, argv);
	script_load_run_file(&script, "test.txt");
	
	script_destroy(&script);

	return 0;
}
