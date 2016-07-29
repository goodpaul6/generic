#include "script.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "%s [execution amount]\n", argv[0]);
		return 1;
	}
	
	script_t script;
	
	script_init(&script);
	
	script_disable_warning(WARN_DYNAMIC_ARRAY_LITERAL, 1);
	script_disable_warning(WARN_ARRAY_DYNAMIC_TO_SPECIFIC, 1);
	
	script_load_parse_file(&script, "test.txt");
	script_compile(&script);
	
	for(int i = 2; i < argc; ++i)
	{
		if(strcmp(argv[i], "-dis") == 0)
			script_dissassemble(&script, stdout);
	}
	for(int i = 0; i < strtol(argv[1], NULL, 10); ++i)
		script_run(&script);
	script_destroy(&script);

	return 0;
}
