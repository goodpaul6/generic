#ifndef SCRIPT_H
#define SCRIPT_H

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
	char* local_path;
	char parsed;
	vector_t expr_list;
} script_module_t;

typedef struct script_debug_env
{
	vector_t breakpoints;
	vector_t break_stack;
} script_debug_env_t;

typedef struct script_debug_breakpoint
{
	int pc; 			// NOTE: if this is known ofc (not of user-level use), -1 usually
	char* file;	// NOTE: If this is null, then any file works
	int line;			// NOTE: If this is -1, then no break occurs
} script_debug_breakpoint_t;

typedef struct
{
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

void script_compile(script_t* script);
void script_dissassemble(script_t* script, FILE* out);
void script_run(script_t* script);
void script_debug(script_t* script, script_debug_env_t* env);

char script_get_function_by_name(script_t* script, const char* name, script_function_t* function);
void script_call_function(script_t* script, script_function_t function, int nargs);

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

#endif
