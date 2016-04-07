#ifndef SCRIPT_H
#define SCRIPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#include "vector.h"
#include "hashmap.h"

typedef enum script_op
{
	OP_PUSH_NULL,
	OP_PUSH_TRUE,
	OP_PUSH_FALSE,
	OP_PUSH_CHAR,
	OP_PUSH_NUMBER,
	OP_PUSH_STRING,
	OP_PUSH_FUNC,
	OP_PUSH_EXTERN_FUNC,
	OP_PUSH_ARRAY,
	OP_PUSH_ARRAY_BLOCK,
	OP_PUSH_RETVAL,
	OP_PUSH_STRUCT,
	
	OP_STRING_LEN,
	OP_ARRAY_LEN,
	
	OP_STRING_GET,
	
	OP_ARRAY_GET,
	OP_ARRAY_SET,
	
	OP_STRUCT_GET,
	OP_STRUCT_SET,
	
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_LT,
	OP_GT,
	OP_LTE,
	OP_GTE,
	
	OP_LAND,
	OP_LOR,
	
	OP_NEG,
	OP_NOT,

	OP_EQU,
	
	OP_READ,
	OP_WRITE,
	
	OP_GOTO,
	OP_GOTOZ,
	
	OP_SET,
	OP_GET,
	
	OP_SETLOCAL,
	OP_GETLOCAL,
	
	OP_CALL,
	
	OP_RETURN,
	OP_RETURN_VALUE,
	
	OP_FILE,
	OP_LINE,
	
	OP_HALT
} script_op_t;

typedef enum
{
	WARN_DYNAMIC_ARRAY_LITERAL,
	WARN_ARRAY_DYNAMIC_TO_SPECIFIC,
	WARN_CALL_DYNAMIC,
	NUM_WARNINGS
} script_warning_t;

typedef enum script_value_type
{
	VAL_NULL,
	VAL_BOOL,
	VAL_CHAR,
	VAL_NUMBER,
	VAL_STRING,
	VAL_FUNC,
	VAL_ARRAY,
	VAL_STRUCT_INSTANCE,
	VAL_NATIVE,
	NUM_VALUE_TYPES
} script_value_type_t;

typedef struct script_string
{
	unsigned int length;
	char* data;
} script_string_t;

typedef struct script_function
{
	char is_extern;
	int index;
} script_function_t;

typedef struct script_struct
{
	// TODO: add some extra info for reflection-y stuff?
	// member names, type names, etc?
	vector_t members;
} script_struct_t;

typedef void (*script_native_callback_t)(void*);

typedef struct script_native
{
	void* value;
	script_native_callback_t on_mark;
	script_native_callback_t on_delete;
} script_native_t;

typedef struct script_value
{
	script_value_type_t type;
	struct script_value* next;
	char marked;
	
	union
	{
		char boolean;
		char code;
		double number;
		script_string_t string;
		script_function_t function;
		vector_t array;
		script_struct_t ds;
		script_native_t nat;
	};
} script_value_t;

typedef struct script_module
{
	size_t start_pc;				// NOTE: when modules are compiled, their starting pc in code is written here
	size_t end_pc;					// NOTE: when modules are compiled, their last pc in code is written here
	char* source_code;
	char* local_path;
	char parsed;
	char compiled;
	vector_t expr_list;
	vector_t compile_time_funcs;	// NOTE: array of expr_t's which should be executed when the module is compiled

	vector_t all_type_tags;			// NOTE: array of type_tag_t's
	hashmap_t user_type_tags;		// NOTE: hashmap of char* to type_tag_t's for struct tags
	vector_t functions;				// NOTE: array of func_decl_t's
	vector_t globals;				// NOTE: array of var_decl_t's
} script_module_t;

typedef struct script_debug_env
{
	vector_t breakpoints;
	vector_t break_stack;
} script_debug_env_t;

typedef struct script_debug_breakpoint
{
	int pc; 			// NOTE: if this is known ofc (not of user-level use), -1 usually
	char* file;			// NOTE: If this is null, then any file works
	int line;			// NOTE: If this is -1, then no break occurs
} script_debug_breakpoint_t;

typedef struct
{
	// NOTE: This is a pointer you can use to store your own structures
	// for access in external function calls 
	// set it like: script->userdata = my_userdata;
	void* userdata;

	char in_extern;
	int pc, fp;
	
	// NOTE:
	// cur_line = line info for current instruction pointer
	// cur_file = file name for current instruction pointer
	int cur_line;
	const char* cur_file;
	
	script_value_t* free_list;
	script_value_t* gc_head;
	script_value_t* ret_val;
	
	int num_objects;
	int max_objects_until_gc;

	vector_t globals;
	
	// NOTE:
	// Number of stack frames (indir_depth)
	int indir_depth;

	vector_t stack;
	vector_t indir;
	
	vector_t code;
	
	vector_t numbers;
	vector_t strings;
	
	vector_t extern_names;
	vector_t externs;
	
	vector_t function_names;
	vector_t function_pcs;
	
	vector_t modules;
} script_t;

typedef void (*script_extern_t)(script_t* script, vector_t* args);

void script_init(script_t* script);

void script_bind_extern(script_t* script, const char* name, script_extern_t ext);

void script_reset(script_t* script);

void script_load_parse_file(script_t* script, const char* filename);
void script_parse_file(script_t* script, FILE* in, const char* module_name);
void script_parse_code(script_t* script, const char* code, const char* module_name);

void script_disable_warning(script_warning_t warning, char disabled);

void script_compile(script_t* script);
void script_dissassemble(script_t* script, FILE* out);
void script_run(script_t* script);

void script_start(script_t* script);
void script_execute_cycle(script_t* script);
void script_stop(script_t* script);

void script_debug(script_t* script, script_debug_env_t* env);

char script_get_function_by_name(script_t* script, const char* name, script_function_t* function);
void script_call_function(script_t* script, script_function_t function, int nargs);
void script_goto_function(script_t* script, script_function_t function, int nargs);

#define script_call_function_cycles(script, function, cycles, nargs, ...) \
do \
{ \
	static bool inFunction = false; \
	static int startDepth = 0; \
	static script_value_t* retVal = NULL; \
	(script)->ret_val = retVal; \
	if(!inFunction) \
	{ \
		startDepth = (script)->indir_depth; \
		__VA_ARGS__ \
		script_goto_function((script), (function), (nargs)); \
		inFunction = true; \
	} \
	for(int i = 0; i < (int)(cycles); ++i) \
	{ \
		script_execute_cycle((script)); \
		if ((script)->indir_depth <= startDepth || (script)->pc < 0) \
		{ \
			inFunction = false; \
			break; \
		} \
	} \
	retVal = (script)->ret_val; \
} while(0)

// NOTE: (YOU HAVE TO BE INSIDE A SCRIPT FUNCTION FOR THIS TO WORK) 
// Sets argument (index, 0 is first argument) of current function to the value
// on the top of the stack given the number of args (nargs)
void script_set_arg(script_t* script, int index, int nargs);

void script_push_bool(script_t* script, char bv);
char script_pop_bool(script_t* script);

void script_push_char(script_t* script, char code);
char script_pop_char(script_t* script);

void script_push_number(script_t* script, double number);
double script_pop_number(script_t* script);

void script_push_cstr(script_t* script, const char* string); 
void script_push_string(script_t* script, script_string_t string);
script_string_t script_pop_string(script_t* script);

void script_push_array(script_t* script, size_t length);
void script_push_premade_array(script_t* script, vector_t array);
vector_t* script_pop_array(script_t* script);

void script_push_native(script_t* script, void* value, script_native_callback_t on_mark, script_native_callback_t on_delete);
script_native_t* script_pop_native(script_t* script);

script_value_t* script_get_arg(vector_t* args, int index);

void script_push_null(script_t* script);

void script_return_top(script_t* script);

void script_destroy(script_t* script);

#ifdef __cplusplus
}
#endif

#endif
