#ifndef SCRIPT_H
#define SCRIPT_H

#include <stdio.h>

#include "vector.h"

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
	
	OP_STRING_LEN,
	OP_ARRAY_LEN,
	
	OP_STRING_GET,
	
	OP_ARRAY_GET,
	OP_ARRAY_SET,
	
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_LT,
	OP_GT,
	OP_LTE,
	OP_GTE,
	
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
	};
} script_value_t;

typedef struct
{
	char in_extern;
	int pc, fp;
	
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
	
	vector_t function_pcs;
} script_t;

typedef void (*script_extern_t)(script_t* script, vector_t* args);

void script_init(script_t* script);

void script_bind_extern(script_t* script, const char* name, script_extern_t ext);

void script_load_run_file(script_t* script, const char* filename);
void script_run_file(script_t* script, FILE* in);
void script_run_code(script_t* script, const char* code);

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

void script_push_null(script_t* script);

void script_return_top(script_t* script);

void script_destroy(script_t* script);

#endif
