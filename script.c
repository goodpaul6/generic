#ifdef __cplusplus
extern "C" {
#endif

#include "script.h"
#include "hashmap.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#define MAX_LEX_CHARS 256
#define STACK_SIZE 256
#define INIT_GC_THRESH 64
#define INVALID_VAR_DECL_INDEX -9999

typedef unsigned char word;

struct func_decl;

typedef struct
{
	int line;
	const char* file;
	int scope;
	struct func_decl* func;
} context_t;

typedef enum
{
	TAG_VOID,
	TAG_DYNAMIC,
	TAG_BOOL,
	TAG_CHAR,
	TAG_NUMBER,
	TAG_STRING,
	TAG_FUNC,
	TAG_ARRAY,
	TAG_NATIVE,
	NUM_BUILTIN_TAGS,
	
	TAG_STRUCT,
	TAG_UNKNOWN
} tag_t;

typedef struct type_tag
{
	context_t ctx;
	tag_t type;
	char defined;
	char finalized;
	
	union
	{
		struct
		{
			vector_t arg_types;
			struct type_tag* return_type;
		} func;
		
		struct type_tag* contained;
		
		struct
		{
			char is_union;
			char* name;
			size_t size;			// NOTE: size of struct in members
			vector_t using;
			vector_t members;
		} ds;
	};
} type_tag_t;

struct expr;
typedef struct
{
	char* name;
	int index;
	type_tag_t* type;
	struct expr* default_value;
} type_tag_member_t;

typedef enum
{
	DECL_FUNCTION,
	DECL_EXTERN
} func_decl_type_t;

typedef struct func_decl
{
	struct func_decl* parent;
	
	func_decl_type_t type;
	type_tag_t* tag;
	
	char* name;
	
	vector_t locals;
	vector_t args;
	
	int index;
	char has_return;
} func_decl_t;

typedef struct
{
	func_decl_t* parent;

	type_tag_t* tag;
	
	char* name;
	
	int scope;
	int index;
} var_decl_t;

enum
{
	TOK_ATOMIC,

	TOK_DIRECTIVE,
	
	TOK_USING,
	TOK_STATIC,
	
	TOK_NULL,
	
	TOK_NEW,
	TOK_STRUCT,
	TOK_UNION,
	TOK_EXTERN,
	
	TOK_CHAR,
	
	TOK_TRUE,
	TOK_FALSE,
	TOK_WHILE,
	TOK_FOR,
	TOK_ELSE,
	TOK_IF,
	TOK_RETURN,
	TOK_FUNC,
	TOK_VAR,
	TOK_READ,
	TOK_WRITE,
	TOK_LEN,
	TOK_IDENT,
	
	TOK_OPENSQUARE,
	TOK_CLOSESQUARE,
	
	TOK_OPENPAREN,
	TOK_CLOSEPAREN,
	
	TOK_OPENCURLY,
	TOK_CLOSECURLY,
	
	TOK_COLON,
	TOK_SEMICOLON,
	TOK_COMMA,
	TOK_DOT,
	
	TOK_NUMBER,
	TOK_STRING,
	
	TOK_NOT,
	TOK_ASSIGN,
	TOK_PLUS,
	TOK_MINUS,
	TOK_MUL,
	TOK_DIV,
	TOK_MOD,

	TOK_LT,
	TOK_GT,
	TOK_LTE,
	TOK_GTE,
	
	TOK_LAND,
	TOK_LOR,
	
	TOK_EQUALS,
	TOK_NOTEQUAL,
	
	TOK_EOF,
	
	NUM_TOKENS
};

typedef enum expr_type
{	
	EXP_NULL,
	
	EXP_STRUCT_DECL,
	EXP_STRUCT_NEW,
	
	EXP_COLON,
	EXP_DOT,
	
	EXP_VAR,
	EXP_ARRAY_INDEX,

	EXP_BOOL,
	EXP_CHAR,
	EXP_NUMBER,
	EXP_STRING,
	
	EXP_LEN,
	EXP_WRITE,
	
	EXP_UNARY,
	EXP_BINARY,
	
	EXP_CALL,
	
	EXP_PAREN,
	EXP_ARRAY_LITERAL,
	EXP_BLOCK,
	
	EXP_IF,
	EXP_WHILE,
	EXP_FOR,

	EXP_RETURN,
	
	EXP_EXTERN,
	EXP_EXTERN_LIST,

	EXP_FUNC,

	EXP_ATOMIC
} expr_type_t;

typedef struct expr
{
	// NOTE: Tells delete_expr not to delete any expressions this expression references (cause it's a shallow copy) 
	char is_shallow_copy;
	context_t ctx;
	expr_type_t type;
	type_tag_t* tag;
		
	union
	{
		// NOTE: EXP_COLON uses dotx	
		struct
		{
			struct expr* value;
			char* name;
		} dotx;
		
		type_tag_t* struct_tag;
		
		struct
		{
			type_tag_t* type;
			vector_t init;			// contains expr_t*
		} newx;
		
		char boolean_value;
		char code;
		
		struct
		{
			struct expr* array;
			struct expr* index;
		} array_index;
		
		struct
		{
			char* name;
			var_decl_t* decl;
		} varx;
		
		int number_index;
		int string_index;
	
		struct expr* write;
		struct expr* len;
		
		struct
		{
			struct expr* rhs;
			int op;
		} unaryx;
		
		struct
		{
			struct expr* lhs;
			struct expr* rhs;
			int op;
		} binx;
		
		struct
		{
			struct expr* func;
			vector_t args;
		} callx;
		
		struct expr* paren;
		vector_t block;
		
		struct
		{
			type_tag_t* contained;
			vector_t values;
		} array_literal;
		
		struct
		{
			struct expr* cond;
			struct expr* body;
			struct expr* alt;
		} ifx;
		
		struct
		{
			struct expr* cond;
			struct expr* body;
		} whilex;
		
		struct
		{
			struct expr* init;
			struct expr* cond;
			struct expr* step;
			struct expr* body;
		} forx;

		struct
		{
			func_decl_t* parent;
			struct expr* value;
		} retx;
		
		func_decl_t* extern_decl;
		vector_t extern_array;
		
		struct
		{
			func_decl_t* decl;
			struct expr* body;
		} funcx;

		struct expr* atomx;
	};
} expr_t;

static const char* g_token_names[NUM_TOKENS] = {
	"atomic",

	"#",
	
	"using",
	"static",
	
	"null",
	
	"new",
	"struct",
	"union",	
	
	"extern",
	
	"character",
	
	"true",
	"false",
	"while",
	"for",
	"else",
	"if",
	"return",
	"func",
	"var",
	"read",
	"write",
	"len",
	"identifier",
	
	"[",
	"]",
	
	"(",
	")",
	
	"{", 
	"}",
	
	":",
	";",
	",",
	".",
	
	"number",
	"string",
	
	"!",
	"=",
	"+",
	"-",
	"*",
	"%",

	"/",
	"<",
	">",
	"<=",
	">=",
	
	"&&",
	"||",
	
	"==",
	"!=",
	
	"eof"
};

static const char* g_value_types[NUM_VALUE_TYPES] = {
	"null",
	"bool",
	"char",
	"number",
	"string",
	"func",
	"array",
	"struct_instance",
	"native"
};

static const char* g_builtin_type_tags[NUM_BUILTIN_TAGS] = {
	"void",
	"dynamic",
	"bool",
	"char",
	"number",
	"string",
	"func",
	"array",
	"native"
};

static int g_line = 1;
static const char* g_file = "none";
static const char* g_code = NULL;
static char g_lexeme[MAX_LEX_CHARS];
static int g_cur_tok = 0;
static int g_scope = 0;
static func_decl_t* g_cur_func = NULL;
static char g_has_error = 0;
static int g_cur_module_index = 0;

static char g_warning_disabled[NUM_WARNINGS] = {0};
static const char* g_warning_name[NUM_WARNINGS] = {
	"dynamic_array_literal",
	"array_dynamic_to_specific",
	"call_dynamic"
};
static const char* g_warning_format[NUM_WARNINGS] = {
	"Contained type of array literal is not uniform across the literal; defaulting to dynamic.",
	"Treating array-dynamic as an array of a specified type; this is totally not type safe.",
	"Calling value of type dynamic; if you don't know whether this is a function, don't do it."
};

// NOTE: Used for file and line debug info optimization
static int g_last_compiled_line = 0;
static const char* g_last_compiled_file = NULL;

static void warn_c(context_t ctx, script_warning_t warning, ...)
{
	if(!g_warning_disabled[(int)warning])
	{
		fprintf(stderr, "\nWarning %s (%s:%i):\n", g_warning_name[(int)warning], ctx.file, ctx.line);
	
		va_list args;
		va_start(args, warning);
		vfprintf(stderr, g_warning_format[(int)warning], args);
		va_end(args);
		
		fprintf(stderr, "\n");
	}
}

static void warn_e(expr_t* exp, script_warning_t warning, ...)
{
	if(!g_warning_disabled[(int)warning])
	{
		fprintf(stderr, "\nWarning %s (%s:%i):\n", g_warning_name[(int)warning], exp->ctx.file, exp->ctx.line);
	
		va_list args;
		va_start(args, warning);
		vfprintf(stderr, g_warning_format[(int)warning], args);
		va_end(args);
		
		fprintf(stderr, "\n");
	}
}

static void error_exit(const char* fmt, ...)
{
	fprintf(stderr, "\nError:\n");
	
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	exit(1);
}

static void error_exit_p(const char* fmt, ...)
{
	fprintf(stderr, "\nError (%s:%i):\n", g_file, g_line);
	
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	exit(1);
}

static void error_defer_c(context_t ctx, const char* fmt, ...)
{
	fprintf(stderr, "\nError (%s:%i):\n", ctx.file, ctx.line);
	
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	g_has_error = 1;
}

static void error_defer_e(expr_t* exp, const char* fmt, ...)
{
	fprintf(stderr, "\nError (%s:%i):\n", exp->ctx.file, exp->ctx.line);
	
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	g_has_error = 1;
}

static void error_exit_c(context_t ctx, const char* fmt, ...)
{
	fprintf(stderr, "\nError (%s:%i):\n", ctx.file, ctx.line);
	
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	exit(1);
}

static void error_exit_e(expr_t* exp, const char* fmt, ...)
{
	fprintf(stderr, "\nError (%s:%i):\n", exp->ctx.file, exp->ctx.line);
	
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	exit(1);
}

static void write_value(script_value_t* val, char quote)
{
	if(!val)
	{
		printf("null");
		return;
	}
	
	switch(val->type)
	{
		case VAL_NULL: printf("null"); break;
		case VAL_BOOL: printf("%s", val->boolean ? "true" : "false"); break;
		case VAL_CHAR: printf("%c", val->code); break;
		case VAL_NUMBER: printf("%g", val->number); break;
		case VAL_STRING: quote ? printf("\"%s\"", val->string.data) : printf("%s", val->string.data); break;
		case VAL_FUNC: printf("func (extern: %s, index: %d)", val->function.is_extern ? "true" : "false", val->function.index); break;
		case VAL_ARRAY:
		{
			printf("[");
			for(int i = 0; i < val->array.length; ++i)
			{
				script_value_t* mem = vec_get_value(&val->array, i, script_value_t*);
				if (val == mem)
					printf("__self__");
				else
					write_value(mem, quote);
				if(i + 1 < val->array.length) printf(", ");
			}
			printf("]");
		} break;
		case VAL_STRUCT_INSTANCE:
		{
			printf("{ ");
			for(int i = 0; i < val->ds.members.length; ++i)
			{
				script_value_t* mem = vec_get_value(&val->ds.members, i, script_value_t*);
				if (val == mem)
					printf("__self__");
				else
					write_value(mem, quote);
				if(i + 1 < val->ds.members.length) printf(", ");
			}
			printf(" }");
		} break;
		case VAL_NATIVE:
		{
			printf("native 0x%lX", (unsigned long)val->nat.value);
		} break;
	}
}

static void print_stack_trace(script_t* script)
{
	fprintf(stderr, "Trace:\n");
	for(int i = script->call_records.length - 1; i >= 0; --i)
	{
		script_call_record_t* record = vec_get(&script->call_records, i);

		fprintf(stderr, "(%s, %d): ", record->file, record->line);
		
		if (record->function.is_extern)
			fprintf(stderr, "extern %s(", vec_get_value(&script->extern_names, record->function.index, char*));
		else
			fprintf(stderr, "%s(", vec_get_value(&script->function_names, record->function.index, char*));
		
		if (record->nargs > 0)
		{
			// NOTE: arguments started at exactly record->stackSize - record->nargs
			int args_start = record->stack_size - record->nargs;

			// NOTE: Print all arguments to this function
			// TODO: Have write_value take in a FILE* so we can route this
			// to stderr
			for (int i = args_start; i < args_start + record->nargs; ++i)
			{
				write_value(vec_get_value(&script->stack, i, script_value_t*), 1);
				if (i + 1 < args_start + record->nargs)
					fprintf(stderr, ", ");
			}
		}

		fprintf(stderr, ")\n");
	}
}

// NOTE: Searches through each module looking at it's start_pc and
// end_pc and seeing if it encloses the current pc
// DO NOT EXPECT THE RETURNED POINTER TO BE VALID AFTER
// MODULES ARE ADDED
static script_module_t* get_executing_module(script_t* script)
{
	for (int i = 0; i < script->modules.length; ++i)
	{
		script_module_t* m = vec_get(&script->modules, i);
		if (m->start_pc <= script->pc && m->end_pc >= script->pc)
			return m;
	}

	return NULL;
}

static func_decl_t* reference_function(script_t* script, const char* function_name);

static void debug_script(script_t* script)
{
	printf("\n");

	char cmdbuf[256];

	while (1)
	{
		fgets(cmdbuf, 256, stdin);
		
		// NOTE: Prints code in the vicinty of the error
		if (strcmp(cmdbuf, "list\n") == 0)
		{
			script_module_t* mod = get_executing_module(script);
			if (mod)
			{
				printf("distance: ");
				
				fgets(cmdbuf, 256, stdin);
				cmdbuf[strcspn(cmdbuf, "\n")] = '\0';

				char* code = mod->source_code;
				int line = 1;
				int minDist = atoi(cmdbuf);

				while (code && *code)
				{
					char* line_end = strchr(code, '\n');

					int dist = (int)labs(script->cur_line - line);
					if (dist < minDist)
					{
						if (dist == 0)
							printf("error ->");
						printf("%.*s\n", (int)(line_end - code), code);
					}

					code = line_end ? line_end + 1 : NULL;
					++line;
				}
			}
			else
				printf("Code being executed does not belong to any modules.\n");
		}
		else if (strcmp(cmdbuf, "local\n") == 0)
		{
			script_call_record_t* record;
			if (script->call_records.length <= 0)
			{
				printf("Not inside function.\n");
				continue;
			}

			record = vec_get(&script->call_records, script->call_records.length - 1);

			printf("local name: ");
			
			fgets(cmdbuf, 256, stdin);
			cmdbuf[strcspn(cmdbuf, "\n")] = '\0';

			const char* name = vec_get_value(&script->function_names, record->function.index, char*);
			
			func_decl_t* decl = reference_function(script, name);
			if (!decl)
			{
				printf("Unable to retrieve function data.\n");
				continue;
			}

			var_decl_t* local = NULL;
			for (int i = 0; i < decl->locals.length; ++i)
			{
				var_decl_t* l = vec_get_value(&decl->locals, i, var_decl_t*);
				if (strcmp(l->name, cmdbuf) == 0)
				{
					local = l;
					break;
				}
			}

			for (int i = 0; i < decl->args.length; ++i)
			{
				var_decl_t* l = vec_get_value(&decl->args, i, var_decl_t*);
				if (strcmp(l->name, cmdbuf) == 0)
				{
					local = l;
					break;
				}
			}

			if (!local)
			{
				printf("No local by that name.\n");
				continue;
			}

			printf("local offset = %d\n", local->index);
			printf("local value = ");

			script_value_t* val = vec_get_value(&script->stack, script->fp + local->index, script_value_t*);
			
			write_value(val, 1);
			printf("\n");
		}
		else if (strcmp(cmdbuf, "stack\n") == 0)
		{
			script_call_record_t* record;
			if (script->call_records.length <= 0)
				record = NULL;
			else
				record = vec_get(&script->call_records, script->call_records.length - 1);

			func_decl_t* decl = NULL;

			if (record)
				decl = reference_function(script, vec_get_value(&script->function_names, record->function.index, char*));

			int min = script->fp;
			if (record)
				min = script->fp - record->nargs;

			for (size_t stackIndex = script->stack.length - 1; stackIndex >= min; --stackIndex)
			{
				if (decl)
				{
					for (int i = 0; i < decl->locals.length; ++i)
					{
						var_decl_t* local = vec_get_value(&decl->locals, i, var_decl_t*);
						if (local->index == stackIndex - script->fp)
						{
							printf("%s = ", local->name);
							break;
						}
					}

					for (int i = 0; i < decl->args.length; ++i)
					{
						var_decl_t* arg = vec_get_value(&decl->args, i, var_decl_t*);
						if (arg->index == stackIndex - script->fp)
						{
							printf("%s = ", arg->name);
							break;
						}
					}
				}

				write_value(vec_get_value(&script->stack, stackIndex, script_value_t*), 1);
				printf("\n");
			}
		}
		else if (strcmp(cmdbuf, "stop\n") == 0)
			break;
	}
}

static void error_exit_script(script_t* script, const char* fmt, ...)
{
	fprintf(stderr, "\nError (%s:%i):\n", script->cur_file, script->cur_line);
	
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
	fprintf(stderr, "\n");
	print_stack_trace(script);
	
	debug_script(script);

	exit(1);
}

static void* emalloc(size_t size)
{
	void* mem = malloc(size);
	if(!mem) error_exit("Out of memory!\n");
	return mem;
}

static char* estrdup(const char* str)
{
	size_t length = strlen(str);
	char* cpy = emalloc(length + 1);
	strcpy(cpy, str);
	
	return cpy;
}

static void delete_expr(expr_t* exp)
{
	if(!exp->is_shallow_copy)
	{
		switch(exp->type)
		{
			case EXP_ATOMIC:
			{
				delete_expr(exp->atomx);
			} break;

			case EXP_VAR:
			{
				free(exp->varx.name);
			} break;
			
			case EXP_DOT: case EXP_COLON:
			{
				delete_expr(exp->dotx.value);
				free(exp->dotx.name);
			} break;
			
			case EXP_STRUCT_NEW:
			{
				for(int i = 0; i < exp->newx.init.length; ++i)
					delete_expr(vec_get_value(&exp->newx.init, i, expr_t*));
				vec_destroy(&exp->newx.init);
			} break;
			
			// NOTE: Nothing is destroyed here because the expressions will be deleted in the 
			// delete_type_tag function
			case EXP_STRUCT_DECL: break;
			
			case EXP_NULL:
			case EXP_EXTERN:
			case EXP_BOOL:
			case EXP_CHAR:
			case EXP_NUMBER:
			case EXP_STRING: break;

			case EXP_EXTERN_LIST:
			{
				for (int i = 0; i < exp->extern_array.length; ++i)
					delete_expr(vec_get_value(&exp->extern_array, i, expr_t*));
				vec_destroy(&exp->extern_array);
			} break;

			case EXP_PAREN: delete_expr(exp->paren); break;
			case EXP_BLOCK:
			{
				for(int i = 0; i < exp->block.length; ++i)
				{
					expr_t* e = vec_get_value(&exp->block, i, expr_t*);
					delete_expr(e);
				}
				vec_destroy(&exp->block);
			} break;
			
			case EXP_ARRAY_INDEX:
			{
				delete_expr(exp->array_index.array);
				delete_expr(exp->array_index.index);
			} break;
			
			case EXP_ARRAY_LITERAL:
			{
				for(int i = 0; i < exp->array_literal.values.length; ++i)
					delete_expr(vec_get_value(&exp->array_literal.values, i, expr_t*));
				vec_destroy(&exp->array_literal.values);
			} break;
			
			case EXP_LEN:
			{
				delete_expr(exp->len);
			} break;
			
			case EXP_UNARY:
			{
				delete_expr(exp->unaryx.rhs);
			} break;
			
			case EXP_BINARY:
			{
				delete_expr(exp->binx.lhs);
				delete_expr(exp->binx.rhs);
			} break;
			
			case EXP_CALL:
			{
				delete_expr(exp->callx.func);
				for(int i = 0; i < exp->callx.args.length; ++i)
					delete_expr(vec_get_value(&exp->callx.args, i, expr_t*));
				vec_destroy(&exp->callx.args);
			} break;
			
			case EXP_IF:
			{
				delete_expr(exp->ifx.cond);
				delete_expr(exp->ifx.body);
				if(exp->ifx.alt)
					delete_expr(exp->ifx.alt);
			} break;
			
			case EXP_WHILE:
			{
				delete_expr(exp->whilex.cond);
				delete_expr(exp->whilex.body);
			} break;

			case EXP_FOR:
			{
				delete_expr(exp->forx.init);
				delete_expr(exp->forx.cond);
				delete_expr(exp->forx.step);
				delete_expr(exp->forx.body);
			} break;
			
			case EXP_RETURN:
			{
				if(exp->retx.value)
					delete_expr(exp->retx.value);
			} break;
			
			case EXP_FUNC:
			{
				delete_expr(exp->funcx.body);
			} break;
			
			case EXP_WRITE:
			{
				delete_expr(exp->write);
			} break;
		}
	}
	
	free(exp);
}

static type_tag_t* create_type_tag(script_t* script, tag_t type)
{
	type_tag_t* tag = emalloc(sizeof(type_tag_t)); 
	
	tag->defined = 1;
	tag->finalized = 0;
	
	tag->ctx.file = g_file;
	tag->ctx.line = g_line;
	tag->ctx.scope = g_scope;
	
	tag->type = type;
	
	switch(type)
	{
		case TAG_FUNC:
		{
			vec_init(&tag->func.arg_types, sizeof(type_tag_t*));
			tag->func.return_type = NULL;
		} break;
		
		case TAG_ARRAY:
		{
			tag->contained = NULL;
		} break;
		
		case TAG_STRUCT:
		{
			tag->ds.name = NULL;
			tag->ds.is_union = 0;
			tag->ds.size = 0;
			vec_init(&tag->ds.using, sizeof(type_tag_t*));
			vec_init(&tag->ds.members, sizeof(type_tag_member_t));
		} break;
		
		default: break;
	}
	
	script_module_t* module = vec_get(&script->modules, g_cur_module_index);
	vec_push_back(&module->all_type_tags, &tag);

	return tag;
}

static type_tag_t* get_user_type_tag_from_name(script_t* script, const char* name)
{
	for (int i = 0; i < script->modules.length; ++i)
	{
		script_module_t* module = vec_get(&script->modules, i);
		var_decl_t* decl = map_get(&module->user_type_tags, name);
		if (decl)
			return decl->tag;
	}

	return NULL;
}

static type_tag_t* get_type_tag_from_name(script_t* script, const char* name)
{
	type_tag_t* user_tag = get_user_type_tag_from_name(script, name);
	if(user_tag) return user_tag;
	
	for(int i = 0; i < NUM_BUILTIN_TAGS; ++i)
	{
		if(strcmp(g_builtin_type_tags[i], name) == 0)
			return create_type_tag(script, (tag_t)i);
	}
	
	type_tag_t* potential_tag = create_type_tag(script, TAG_STRUCT);
	
	potential_tag->ds.name = estrdup(name);
	vec_init(&potential_tag->ds.members, sizeof(type_tag_member_t));
	potential_tag->defined = 0;
	
	script_module_t* module = vec_get(&script->modules, g_cur_module_index);
	map_set(&module->user_type_tags, name, potential_tag);
	
	return potential_tag;
}

static char is_type_tag(type_tag_t* tag, tag_t type)
{
	return tag->type == type || (tag->type != TAG_VOID && (tag->type == TAG_DYNAMIC));
}

static char compare_type_tags(type_tag_t* a, type_tag_t* b)
{
	if(a->type != TAG_VOID && b->type != TAG_VOID)
	{
		if(a->type == TAG_DYNAMIC || b->type == TAG_DYNAMIC)
			return 1;
	}
	
	if(a->type != b->type) return 0;
	
	switch(a->type)
	{
		case TAG_FUNC:
		{
			if(!compare_type_tags(a->func.return_type, b->func.return_type)) return 0;
			if(a->func.arg_types.length != b->func.arg_types.length) return 0;
			
			for(int i = 0; i < a->func.arg_types.length; ++i)
			{
				if(!compare_type_tags(vec_get_value(&a->func.arg_types, i, type_tag_t*), vec_get_value(&b->func.arg_types, i, type_tag_t*)))
					return 0;
			}
		} break;
		
		case TAG_ARRAY:
		{
			if(a->contained->type == TAG_DYNAMIC || b->contained->type == TAG_DYNAMIC)
			{
				context_t ctx;
				ctx.file = g_file;
				ctx.line = g_line;
				
				warn_c(ctx, WARN_ARRAY_DYNAMIC_TO_SPECIFIC);
			}
			
			if(!compare_type_tags(a->contained, b->contained)) return 0;
		} break;
		
		case TAG_STRUCT:
		{
			return strcmp(a->ds.name, b->ds.name) == 0;
		} break;
		
		default: break;
	}
	
	return 1;
}

static void check_all_tags_defined(script_t* script)
{
	script_module_t* module = vec_get(&script->modules, g_cur_module_index);
	for(int i = 0; i < module->all_type_tags.length; ++i)
	{
		type_tag_t* tag = vec_get_value(&module->all_type_tags, i, type_tag_t*);
		if(!tag->defined) error_defer_c(tag->ctx, "Use of undefined type '%s'\n", tag->ds.name);
	}
}

static void destroy_type_tag(void* vtag)
{
	type_tag_t* tag = vtag;
	switch(tag->type)
	{
		case TAG_FUNC:
		{	
			vec_destroy(&tag->func.arg_types);
		} break;
	
		case TAG_STRUCT:
		{
			free(tag->ds.name);
			
			for(int i = 0; i < tag->ds.members.length; ++i)
			{
				type_tag_member_t* mem = vec_get(&tag->ds.members, i);
				free(mem->name);
				if(mem->default_value)
					delete_expr(mem->default_value);
			}
				
			vec_destroy(&tag->ds.using);
			vec_destroy(&tag->ds.members);
		} break;
		
		default: break;
	}
}

static type_tag_t* define_struct_type(script_t* script, const char* name, char is_union)
{
	script_module_t* module = vec_get(&script->modules, g_cur_module_index);
	type_tag_t* tag = map_get(&module->user_type_tags, name);
	if(!tag)
	{
		tag = create_type_tag(script, TAG_STRUCT);
		tag->ds.name = estrdup(name);
		map_set(&module->user_type_tags, name, tag);
	}
	
	tag->ds.is_union = is_union;
	tag->defined = 1;
	
	return tag;
}

static void traverse_all_tags(void* vp_tag)
{
	type_tag_t* tag = *(type_tag_t**)vp_tag;
	
	destroy_type_tag(tag);
	free(tag);
}

static void delete_var_decl(void* vp)
{
	var_decl_t* decl = *(var_decl_t**)vp;
	free(decl->name);
	free(decl);
}

static void delete_func_decl(void* vp)
{
	func_decl_t* decl = *(func_decl_t**)vp;
	
	vec_traverse(&decl->locals, delete_var_decl);
	vec_traverse(&decl->args, delete_var_decl);
	
	vec_destroy(&decl->locals);
	vec_destroy(&decl->args);
	
	free(decl->name);
	free(decl);
}

static var_decl_t* reference_variable(script_t* script, const char* name);
static var_decl_t* declare_variable(script_t* script, const char* name, type_tag_t* tag)
{
	var_decl_t* decl = reference_variable(script, name);
	
	if(!decl)
	{
		decl = emalloc(sizeof(type_tag_t));
		
		decl->parent = g_cur_func;
		decl->tag = tag;
		decl->name = estrdup(name);
		decl->scope = g_scope;
		
		if(g_cur_func)
		{
			decl->index = g_cur_func->locals.length;
			vec_push_back(&g_cur_func->locals, &decl);
			return decl;
		}
	
		// NOTE: g_cur_module_index MUST have the right value rn
		script_module_t* module = vec_get(&script->modules, g_cur_module_index);

		// NOTE: The global index must be unique across modules
		int index = module->globals.length;
		for (int i = 0; i < g_cur_module_index; ++i)
		{
			script_module_t* prev_module = vec_get(&script->modules, i);
			index += prev_module->globals.length;
		}

		decl->index = index;
		vec_push_back(&module->globals, &decl);

		return decl;
	}
	else if (!g_cur_func)
		error_exit_p("Attempted to redeclare global variable '%s'\n", name);

	return decl;
}

static var_decl_t* declare_argument(const char* name, type_tag_t* tag)
{
	if(!g_cur_func) error_exit("Attempting to declare argument outside of function\n");
	
	var_decl_t* decl = emalloc(sizeof(var_decl_t));
	
	vec_push_back(&g_cur_func->tag->func.arg_types, &tag);
	
	decl->parent = g_cur_func;
	decl->tag = tag;
	decl->name = estrdup(name);
	decl->scope = g_scope;
	decl->index = INVALID_VAR_DECL_INDEX;
	
	vec_push_back(&g_cur_func->args, &decl);
	return decl;
}

// NOTE: This is to be called after all the arguments to a function
// are parsed (it will assign an index to them)
static void finalize_args()
{
	if (!g_cur_func) error_exit("Attempting to finalize arguments outside of function\n");

	for (int i = 0; i < g_cur_func->args.length; ++i)
	{
		var_decl_t* arg = vec_get_value(&g_cur_func->args, i, var_decl_t*);
		arg->index = -(int)g_cur_func->args.length + i;
	}
}

static void enter_scope()
{
	++g_scope;
}

static void exit_scope()
{
	if(g_cur_func)
	{		
		for(int i = 0; i < g_cur_func->locals.length; ++i)
		{
			var_decl_t* decl = vec_get_value(&g_cur_func->locals, i, var_decl_t*);
			if(decl->scope == g_scope) decl->scope = -1;	// NOTE: can no longer access this variable because we exited this scope
		}
	}
	--g_scope;
}

static var_decl_t* reference_variable(script_t* script, const char* name)
{
	if(g_cur_func)
	{
		for(int scope = g_scope; scope >= 0; --scope)
		{
			for(int i = 0; i < g_cur_func->locals.length; ++i)
			{
				var_decl_t* decl = vec_get_value(&g_cur_func->locals, i, var_decl_t*);
				if(decl->scope == scope && strcmp(decl->name, name) == 0) return decl;
			}
		}
		
		for(int i = 0; i < g_cur_func->args.length; ++i)
		{
			var_decl_t* decl = vec_get_value(&g_cur_func->args, i, var_decl_t*);
			if(strcmp(decl->name, name) == 0) return decl;
		}
	}

	for (int module_index = 0; module_index < script->modules.length; ++module_index)
	{
		script_module_t* module = vec_get(&script->modules, module_index);
		for (int i = 0; i < module->globals.length; ++i)
		{
			var_decl_t* decl = vec_get_value(&module->globals, i, var_decl_t*);
			if (strcmp(decl->name, name) == 0) return decl;
		}
	}
	
	return NULL;
}

static func_decl_t* declare_function(script_t* script, const char* name)
{
	script_module_t* module = vec_get(&script->modules, g_cur_module_index);
	func_decl_t* decl = emalloc(sizeof(func_decl_t));
	
	decl->parent = g_cur_func;
	decl->type = DECL_FUNCTION;
	decl->tag = create_type_tag(script, TAG_FUNC);
	decl->name = estrdup(name);
	
	vec_init(&decl->locals, sizeof(var_decl_t*));
	vec_init(&decl->args, sizeof(var_decl_t*));

	decl->index = script->function_names.length;
	
	char* dup_name = estrdup(name);
	vec_push_back(&script->function_names, &dup_name);
	
	int undef_pc = -1;
	vec_push_back(&script->function_pcs, &undef_pc);
	
	decl->has_return = 0;

	vec_push_back(&module->functions, &decl);
	return decl;
}

static int get_extern_index(script_t* script, const char* name)
{
	for(int i = 0; i < script->extern_names.length; ++i)
	{
		if(strcmp(vec_get_value(&script->extern_names, i, char*), name) == 0) 
			return i;
	}
	
	return -1;
}

static func_decl_t* declare_extern(script_t* script, const char* name)
{
	script_module_t* module = vec_get(&script->modules, g_cur_module_index);
	func_decl_t* decl = emalloc(sizeof(func_decl_t));
	
	decl->parent = g_cur_func;
	decl->type = DECL_EXTERN;
	decl->tag = create_type_tag(script, TAG_FUNC);
	decl->name = estrdup(name);
	
	vec_init(&decl->locals, sizeof(var_decl_t*));
	vec_init(&decl->args, sizeof(var_decl_t*));

	int index = get_extern_index(script, name);
	if(index < 0) error_exit_p("Attempted to declare unbound extern by name '%s'\n", name);
	decl->index = index;

	vec_push_back(&module->functions, &decl);
	return decl;
}

static func_decl_t* reference_function(script_t* script, const char* name)
{
	for (int module_index = 0; module_index < script->modules.length; ++module_index)
	{
		script_module_t* module = vec_get(&script->modules, module_index);
		for (int i = 0; i < module->functions.length; ++i)
		{
			func_decl_t* decl = vec_get_value(&module->functions, i, func_decl_t*);
			if (strcmp(decl->name, name) == 0)
				return decl;
		}
	}
	
	return NULL;
}

static void enter_function(func_decl_t* decl)
{
	g_cur_func = decl;
}

static void exit_function()
{
	if(!g_cur_func) error_exit("Attempted to exit function even though the parser never entered one to begin with\n");
	g_cur_func = g_cur_func->parent;
}

static int get_char()
{
	if(!g_code) return EOF;
	
	int c = *g_code;
	if(c == '\0') return EOF;
	else 
	{
		++g_code;
		return c;
	}
}

static int peek_char()
{
	if(!g_code) return EOF;
	
	int c = *(g_code + 1);
	if(c == '\0') return EOF;
	else return c;
}

static int get_token(char reset)
{
	static int last = ' ';
	
	if(reset)
	{
		last = ' ';
		return 0;
	}
	
	while(isspace(last))
	{
		if(last == '\n') ++g_line;
		last = get_char();
	}
	
	// TODO: check for buffer overflow	
	if(isalpha(last) || last == '_' || last == '#')
	{
		int i = 0;
		while(isalnum(last) || last == '_' || last == '#')
		{
			g_lexeme[i++] = last;
			last = get_char();
		}
		g_lexeme[i] = '\0';
		
		if(g_lexeme[0] == '#') return TOK_DIRECTIVE;
		
		if (strcmp(g_lexeme, "atomic") == 0) return TOK_ATOMIC;
		if(strcmp(g_lexeme, "using") == 0) return TOK_USING;
		if(strcmp(g_lexeme, "static") == 0) return TOK_STATIC;
		if(strcmp(g_lexeme, "null") == 0) return TOK_NULL;
		if(strcmp(g_lexeme, "new") == 0) return TOK_NEW;
		if(strcmp(g_lexeme, "union") == 0) return TOK_UNION;
		if(strcmp(g_lexeme, "struct") == 0) return TOK_STRUCT;
		if(strcmp(g_lexeme, "extern") == 0) return TOK_EXTERN;
		if(strcmp(g_lexeme, "true") == 0) return TOK_TRUE;
		if(strcmp(g_lexeme, "false") == 0) return TOK_FALSE;
		if(strcmp(g_lexeme, "while") == 0) return TOK_WHILE;
		if (strcmp(g_lexeme, "for") == 0) return TOK_FOR;
		if(strcmp(g_lexeme, "else") == 0) return TOK_ELSE;
		if(strcmp(g_lexeme, "if") == 0) return TOK_IF;
		if(strcmp(g_lexeme, "return") == 0) return TOK_RETURN;
		if(strcmp(g_lexeme, "func") == 0) return TOK_FUNC;
		if(strcmp(g_lexeme, "var") == 0) return TOK_VAR;
		if(strcmp(g_lexeme, "read") == 0) return TOK_READ;
		if(strcmp(g_lexeme, "write") == 0) return TOK_WRITE;
		if(strcmp(g_lexeme, "len") == 0) return TOK_LEN;
		
		return TOK_IDENT;
	}
	
	if(isdigit(last))
	{
		int i = 0;
		while(isdigit(last) || last == '-' || last == '.')
		{
			g_lexeme[i++] = last;
			last = get_char();
		}
		g_lexeme[i] = '\0';
		
		return TOK_NUMBER;
	}
	
	if(last == '"')
	{
		last = get_char();
		
		int i = 0;
		while(last != '"')
		{
			if (last == '\\')
			{
				last = get_char();
				switch (last)
				{
					case 'n': last = '\n'; break;
					case 't': last = '\t'; break;
					case '0': last = '\0'; break;
					case 'b': last = '\b'; break;
					case 'r': last = '\r'; break;
					case '\'': last = '\''; break;
					case '"': last = '"'; break;
					case '\\': last = '\\'; break;
					case '\n':
					case '\r':
					{
						while (isspace(last)) 
						{
							if (last == '\n') 
								++g_line;

							last = get_char();
						}
					} break;
					default:
						error_exit("invalid escape sequence char %c\n", last);
						break;
				}
			}

			g_lexeme[i++] = last;
			last = get_char();
		}
		last = get_char();
		
		g_lexeme[i] = '\0';
		
		return TOK_STRING;
	}
	
	if(last == '\'')
	{
		last = get_char();
		g_lexeme[0] = last;
		g_lexeme[1] = '\0';
		last = get_char();
		if(last != '\'') error_exit_p("Expected ' after %c\n", g_lexeme[0]);
		last = get_char();
		
		return TOK_CHAR; 
	}
	
	if(last == EOF)
		return TOK_EOF;
	
	int last_char = last;
	last = get_char();
	
	if(last_char == '!')
	{
		if(last == '=')
		{
			last = get_char();
			return TOK_NOTEQUAL;
		}
		
		return TOK_NOT;
	}
	
	if(last_char == '&' && last == '&')
	{
		last = get_char();
		return TOK_LAND;
	}
	
	if(last_char == '|' && last == '|')
	{
		last = get_char();
		return TOK_LOR;
	}
	
	if(last_char == '/' && last == '/')
	{
		last = get_char();
		while(last != '\n' || last == EOF) last = get_char();
		return get_token(0);
	}
	
	if(last_char == '.') return TOK_DOT;
	
	if(last_char == ';') return TOK_SEMICOLON;
	
	if(last_char == '[') return TOK_OPENSQUARE;
	if(last_char == ']') return TOK_CLOSESQUARE;
	
	if(last_char == '(') return TOK_OPENPAREN;
	if(last_char == ')') return TOK_CLOSEPAREN;
	
	if(last_char == '{') return TOK_OPENCURLY;
	if(last_char == '}') return TOK_CLOSECURLY;
	
	if(last_char == ':') return TOK_COLON;
	
	if(last_char == ',') return TOK_COMMA;
	
	if(last_char == '+') return TOK_PLUS;
	if(last_char == '-') return TOK_MINUS;
	if(last_char == '*') return TOK_MUL;
	if(last_char == '/') return TOK_DIV;
	if (last_char == '%') return TOK_MOD;
	
	// NOTE: order of checking these is important
	if(last_char == '=' && last == '=')
	{
		last = get_char();
		return TOK_EQUALS;
	}
	
	if(last_char == '<' && last == '=')
	{
		last = get_char();
		return TOK_LTE;
	}
	if(last_char == '>' && last == '=')
	{
		last = get_char();
		return TOK_GTE;
	}
	
	if(last_char == '=') return TOK_ASSIGN;
	if(last_char == '<') return TOK_LT;
	if(last_char == '>') return TOK_GT;
	
	error_exit_p("Unexpected character '%c'\n", last_char);
	return 0;
}

static int get_next_token()
{
	g_cur_tok = get_token(0);
	return g_cur_tok;
}

static int register_number(script_t* script, double number)
{
	for(int i = 0; i < script->numbers.length; ++i)
	{
		if(vec_get_value(&script->numbers, i, double) == number)
			return i;
	}
	
	vec_push_back(&script->numbers, &number);
	return script->numbers.length - 1;
}

static int register_string(script_t* script, const char* string)
{
	for(int i = 0; i < script->strings.length; ++i)
	{
		if(strcmp(vec_get_value(&script->strings, i, script_string_t).data, string) == 0)
			return i;
	}
	
	script_string_t str;
	str.length = strlen(string);
	str.data = emalloc(str.length + 1);
	strcpy(str.data, string);
	
	vec_push_back(&script->strings, &str);
	return script->strings.length - 1;
}

static expr_t* create_expr(expr_type_t type)
{
	expr_t* exp = emalloc(sizeof(expr_t));

	exp->is_shallow_copy = 0;
	exp->tag = NULL;
	exp->ctx.file = g_file;
	exp->ctx.line = g_line;
	exp->ctx.scope = g_scope;
	exp->ctx.func = g_cur_func;
	exp->type = type;
	
	return exp;
}

static expr_t* parse_expr(script_t* script);

static type_tag_t* parse_type_tag(script_t* script)
{
	type_tag_t* tag = get_type_tag_from_name(script, g_lexeme);
	if(!tag) error_exit_p("Invalid type tag '%s'\n", g_lexeme);
	get_next_token();

	switch(tag->type)
	{
		case TAG_FUNC: // parse argument types and return type
		{
			if(g_cur_tok != TOK_OPENPAREN) error_exit_p("Expected '(' after 'func' in type definition but received '%s'\n", g_token_names[g_cur_tok]);
			get_next_token();
			
			while(g_cur_tok != TOK_CLOSEPAREN)
			{
				type_tag_t* arg_tag = parse_type_tag(script);
				vec_push_back(&tag->func.arg_types, &arg_tag);
				if(g_cur_tok == TOK_COMMA) get_next_token();
				else if(g_cur_tok != TOK_CLOSEPAREN) error_exit_p("Expected ')' at the end of function argument list '%s'\n", g_token_names[g_cur_tok]);
			}
			get_next_token();
			
			if(g_cur_tok != TOK_MINUS) error_exit_p("Expected '-' after ')' but received '%s'\n", g_token_names[g_cur_tok]);
			get_next_token();
			
			tag->func.return_type = parse_type_tag(script);
		} break;
		
		case TAG_ARRAY: // parse contained type
		{
			if(g_cur_tok != TOK_MINUS) error_exit_p("Expected '-' after 'array' but received '%s'\n", g_token_names[g_cur_tok]);
			get_next_token();
			
			tag->contained = parse_type_tag(script);
		} break;
		
		default: break;
	}

	return tag;
}

static expr_t* parse_bool(script_t* script)
{
	expr_t* exp = create_expr(EXP_BOOL);
	if(g_cur_tok == TOK_TRUE) exp->boolean_value = 1;
	else exp->boolean_value = 0;
	get_next_token();
	
	return exp;
}

static expr_t* parse_char(script_t* script)
{
	expr_t* exp = create_expr(EXP_CHAR);
	exp->code = g_lexeme[0];
	get_next_token();
	
	return exp;
}

static expr_t* parse_number(script_t* script)
{
	expr_t* exp = create_expr(EXP_NUMBER);
	
	double number = strtod(g_lexeme, NULL);
	get_next_token();
	
	exp->number_index = register_number(script, number);
	
	return exp; 
}

static expr_t* parse_string(script_t* script)
{
	expr_t* exp = create_expr(EXP_STRING);
	
	exp->string_index = register_string(script, g_lexeme);
	
	get_next_token();
	
	return exp;
}

static expr_t* parse_ident(script_t* script)
{
	expr_t* exp = create_expr(EXP_VAR);
	
	exp->varx.name = estrdup(g_lexeme);
	exp->varx.decl = reference_variable(script, g_lexeme);
	
	get_next_token();
	
	return exp;
}

static expr_t* parse_var(script_t* script)
{
	get_next_token();
	if(g_cur_tok != TOK_IDENT) error_exit_p("Expected identifier after 'var'\n");
	
	expr_t* exp = create_expr(EXP_VAR);
	exp->varx.name = estrdup(g_lexeme);
	
	get_next_token();
			
	if (g_cur_tok != TOK_COLON)
	{
		// NOTE: Type not given so it remains null until resolved
		exp->varx.decl = declare_variable(script, exp->varx.name, create_type_tag(script, TAG_UNKNOWN));
		return exp;
	}

	get_next_token();
	
	type_tag_t* tag = parse_type_tag(script);
	exp->varx.decl = declare_variable(script, exp->varx.name, tag);

	return exp;
}

static expr_t* parse_write(script_t* script)
{
	expr_t* exp = create_expr(EXP_WRITE);
	get_next_token();
	
	exp->write = parse_expr(script);
	
	return exp;
}

static expr_t* parse_len(script_t* script)
{
	expr_t* exp = create_expr(EXP_LEN);
	get_next_token();
	
	exp->len = parse_expr(script);
	
	return exp;
}

static expr_t* parse_paren(script_t* script)
{
	expr_t* exp = create_expr(EXP_PAREN);
	
	get_next_token();
	exp->paren = parse_expr(script);
	
	if(g_cur_tok != TOK_CLOSEPAREN) error_exit_p("Expected ')' after '('\n");
	get_next_token();
	
	return exp;
}

static expr_t* parse_block(script_t* script)
{
	expr_t* exp = create_expr(EXP_BLOCK);
	get_next_token();
	
	vector_t block;
	vec_init(&block, sizeof(expr_t*));
	
	enter_scope();
	while(g_cur_tok != TOK_CLOSECURLY)
	{
		expr_t* e = parse_expr(script);
		vec_push_back(&block, &e);
	}
	exit_scope();
	
	get_next_token();
	
	exp->block = block;
	return exp;
}

static expr_t* parse_array_literal(script_t* script)
{
	expr_t* exp = create_expr(EXP_ARRAY_LITERAL);
	get_next_token();

	exp->array_literal.contained = NULL;
	
	vector_t array_values;
	vec_init(&array_values, sizeof(expr_t*));
	
	while(g_cur_tok != TOK_CLOSESQUARE)
	{
		expr_t* e = parse_expr(script);
		vec_push_back(&array_values, &e);
		if(g_cur_tok == TOK_COMMA) get_next_token();
		else if(g_cur_tok != TOK_CLOSESQUARE) error_exit_p("Unexpected '%s'\n", g_token_names[g_cur_tok]);
	}
	get_next_token();
	
	exp->array_literal.values = array_values;
	
	if(array_values.length == 0) 
	{
		if(g_cur_tok != TOK_COLON) error_exit_p("Expected ':' after empty array literal\n");
		get_next_token();
		
		exp->array_literal.contained = parse_type_tag(script);
	}
	
	return exp;
}

static expr_t* parse_return(script_t* script)
{
	if(!g_cur_func) error_exit_p("'return' can only be used inside a function body\n");
	expr_t* exp = create_expr(EXP_RETURN);
	exp->retx.parent = g_cur_func;
	
	g_cur_func->has_return = 1;
	
	if(get_next_token() != TOK_SEMICOLON)
		exp->retx.value = parse_expr(script);
	else
	{
		get_next_token();
		exp->retx.value = NULL;
	}
		
	return exp;
}

static expr_t* parse_if(script_t* script)
{
	expr_t* exp = create_expr(EXP_IF);
	get_next_token();
	
	exp->ifx.cond = parse_expr(script);
	exp->ifx.body = parse_expr(script);
	
	if(g_cur_tok == TOK_ELSE)
	{
		get_next_token();
		exp->ifx.alt = parse_expr(script);
	}
	else
		exp->ifx.alt = NULL;
		
	return exp;
}

static expr_t* parse_while(script_t* script)
{
	expr_t* exp = create_expr(EXP_WHILE);
	get_next_token();

	exp->whilex.cond = parse_expr(script);
	exp->whilex.body = parse_expr(script);
	
	return exp;
}

static expr_t* parse_for(script_t* script)
{
	expr_t* exp = create_expr(EXP_FOR);
	get_next_token();

	++g_scope;

	exp->forx.init = parse_expr(script);

	if (g_cur_tok != TOK_COMMA) error_exit_p("Expected ',' after for initializer\n");
	get_next_token();

	exp->forx.cond = parse_expr(script);
	
	if (g_cur_tok != TOK_COMMA) error_exit_p("Expected ',' after for condition\n");
	get_next_token();

	exp->forx.step = parse_expr(script);
	exp->forx.body = parse_expr(script);
	--g_scope;

	return exp;
}

static expr_t* parse_extern_decl(script_t* script)
{
	expr_t* exp = create_expr(EXP_EXTERN);

	char* name = estrdup(g_lexeme);

	get_next_token();

	exp->extern_decl = declare_extern(script, name);

	if (g_cur_tok != TOK_OPENPAREN) error_exit_p("Expected '(' after extern %s\n", name);
	get_next_token();

	while (g_cur_tok != TOK_CLOSEPAREN)
	{
		type_tag_t* arg_tag = parse_type_tag(script);
		vec_push_back(&exp->extern_decl->tag->func.arg_types, &arg_tag);

		if (g_cur_tok == TOK_COMMA) get_next_token();
		else if (g_cur_tok != TOK_CLOSEPAREN) error_exit_p("Unexpected token '%s'\n", g_token_names[g_cur_tok]);
	}
	get_next_token();

	if (g_cur_tok != TOK_COLON) error_exit_p("Expected ':' but received '%s'\n", g_token_names[g_cur_tok]);
	get_next_token();

	exp->extern_decl->tag->func.return_type = parse_type_tag(script);

	free(name);

	return exp;
}

static expr_t* parse_extern(script_t* script)
{
	get_next_token();
	
	if (g_cur_tok == TOK_OPENCURLY)
	{
		// NOTE: Extern list
		expr_t* exp = create_expr(EXP_EXTERN_LIST);
		
		exp->type = EXP_EXTERN_LIST;
		vec_init(&exp->extern_array, sizeof(expr_t*));

		get_next_token();

		while (g_cur_tok != TOK_CLOSECURLY)
		{
			expr_t* declExp = parse_extern_decl(script);
			vec_push_back(&exp->extern_array, &declExp);
		}

		get_next_token();

		return exp;
	}

	return parse_extern_decl(script);
}

static expr_t* parse_func(script_t* script)
{
	expr_t* exp = create_expr(EXP_FUNC);
	get_next_token();
	
	char* name = estrdup(g_lexeme);
	
	get_next_token();
	
	exp->funcx.decl = declare_function(script, name);
	enter_function(exp->funcx.decl);
	
	if(g_cur_tok != TOK_OPENPAREN) error_exit_p("Expected '(' after func %s\n", name);
	get_next_token();
	
	while(g_cur_tok != TOK_CLOSEPAREN)
	{
		if(g_cur_tok != TOK_IDENT) error_exit_p("Expected identifier but received %s\n", g_token_names[g_cur_tok]);
		char* name = estrdup(g_lexeme);
		
		get_next_token();
		if(g_cur_tok != TOK_COLON) error_exit_p("Expected ':' after '%s' in argument list\n", name);
		get_next_token();
		
		declare_argument(name, parse_type_tag(script));
		free(name);
		
		if(g_cur_tok == TOK_COMMA) get_next_token();
		else if(g_cur_tok != TOK_CLOSEPAREN) error_exit_p("Unexpected token '%s'\n", g_token_names[g_cur_tok]);
	}
	get_next_token();

	finalize_args();
	
	if(g_cur_tok != TOK_COLON) error_exit_p("Expected ':' after ')'\n");
	get_next_token();
	exp->funcx.decl->tag->func.return_type = parse_type_tag(script);
	
	exp->funcx.body = parse_expr(script);
	exit_function();
	
	if(exp->funcx.decl->tag->func.return_type->type != TAG_VOID && !exp->funcx.decl->has_return)
		error_exit_p("Non-void function missing return statement in body\n");
		
	// TODO: Check if the body has a top-level return statement
	// in order to assure that non-void functions ALWAYS return a value
	// (I mean, they'll return null when you don't so idk really)
	
	free(name);

	return exp;
}

static expr_t* parse_struct(script_t* script)
{
	expr_t* exp = create_expr(EXP_STRUCT_DECL);
	char is_union = g_cur_tok == TOK_UNION;
	
	get_next_token();
	// if(g_cur_tok != TOK_IDENT) error_exit_p("Expected identifier after 'struct' but received '%s'\n", g_token_names[g_cur_tok]);
	// type_tag_t* tag = define_struct_type(g_lexeme, is_union);
	
	if(g_cur_tok != TOK_IDENT) error_exit_p("Expected identifier after '%s'\n", is_union ? "union" : "struct");
	type_tag_t* tag = define_struct_type(script, g_lexeme, is_union);
	
	get_next_token();
	
	if(g_cur_tok != TOK_OPENCURLY) error_exit_p("Expected '{' after struct %s but received '%s'\n", tag->ds.name, g_token_names[g_cur_tok]);
	get_next_token();
	
	while(g_cur_tok != TOK_CLOSECURLY)
	{
		if(g_cur_tok == TOK_USING)
		{
			get_next_token();
			if(g_cur_tok != TOK_IDENT) error_exit_p("Expected identifier after 'using' but received '%s'\n", g_token_names[g_cur_tok]);
			
			type_tag_t* used_type = get_type_tag_from_name(script, g_lexeme);
			vec_push_back(&tag->ds.using, &used_type);
			
			get_next_token();
			continue;
		}
		
		char is_static = 0;
		
		if(g_cur_tok == TOK_STATIC)
		{
			is_static = 1;
			get_next_token();
		}
	
		if(g_cur_tok != TOK_IDENT) error_exit_p("Expected identifier but received '%s'\n", g_token_names[g_cur_tok]);
			
		char* name = estrdup(g_lexeme);
		get_next_token();
		
		if(g_cur_tok != TOK_COLON)
		{
			if(g_cur_tok != TOK_OPENPAREN) error_exit_p("Expected ':' or '(' but received '%s'\n", g_token_names[g_cur_tok]);
			get_next_token();
			
			size_t length = strlen(tag->ds.name) + strlen(name) + 2;
			char* buf = emalloc(length + 1);
			sprintf(buf, "%s_%s", tag->ds.name, name); 
			buf[length - 1] = '\0';
			
			func_decl_t* decl = declare_function(script, buf);
			enter_function(decl);
			
			if(!is_static)
				declare_argument("self", tag);
			
			// NOTE: Copied from parse_func
			while(g_cur_tok != TOK_CLOSEPAREN)
			{
				if(g_cur_tok != TOK_IDENT) error_exit_p("Expected identifier but received %s\n", g_token_names[g_cur_tok]);
				char* name = estrdup(g_lexeme);
				
				get_next_token();
				if(g_cur_tok != TOK_COLON) error_exit_p("Expected ':' after '%s' in argument list\n", name);
				get_next_token();
				
				declare_argument(name, parse_type_tag(script));
				free(name);
				
				if(g_cur_tok == TOK_COMMA) get_next_token();
				else if(g_cur_tok != TOK_CLOSEPAREN) error_exit_p("Unexpected token '%s'\n", g_token_names[g_cur_tok]);
			}
			get_next_token();

			finalize_args();
				
			if(g_cur_tok != TOK_COLON) error_exit_p("Expected ':' after ')'\n");
			get_next_token();
			decl->tag->func.return_type = parse_type_tag(script);
		
			free(buf);
			exit_function();
		}
		else
		{
			get_next_token();
		
			type_tag_member_t member;
			member.index = is_union ? 0 : tag->ds.members.length;
			member.name = name;
			member.type = parse_type_tag(script);
			member.default_value = NULL;
			
			if(is_union)
				tag->ds.size = 1;
			else
				tag->ds.size += 1;
			
			// NOTE: user is setting a default value for this shit
			if(g_cur_tok == TOK_ASSIGN)
			{
				get_next_token();
				member.default_value = parse_expr(script);
			}
		
			vec_push_back(&tag->ds.members, &member);
		}
	}
	get_next_token();
	
	exp->struct_tag = tag;
	
	return exp;
}

static expr_t* parse_null(script_t* script)
{
	expr_t* exp = create_expr(EXP_NULL);
	get_next_token();
	return exp;
}

// NOTE: Returns module index
static int add_module(script_t* script, const char* local_path, const char* module_name, const char* code)
{
	// NOTE: This'll slow down add module a bit (but who cares, how many modules could you possibly have :)
	if (local_path)
	{
		for (int i = 0; i < script->modules.length; ++i)
		{
			script_module_t* module = vec_get(&script->modules, i);
			if (module->local_path)
			{
				if (strcmp(module->local_path, local_path) == 0)
					return i;
			}
		}
	}
	
	script_module_t module;
	
	module.start_pc = -1;
	module.end_pc = -1;
	module.name = module_name ? estrdup(module_name) : NULL;
	module.local_path = local_path ? estrdup(local_path) : NULL;
	module.source_code = estrdup(code);
	module.parsed = 0;
	module.compiled = 0;
	
	vec_init(&module.referenced_modules, sizeof(int));

	vec_init(&module.expr_list, sizeof(expr_t*));
	vec_init(&module.compile_time_blocks, sizeof(expr_t*));
	
	vec_init(&module.globals, sizeof(var_decl_t*));
	vec_init(&module.functions, sizeof(func_decl_t*));
	vec_init(&module.all_type_tags, sizeof(type_tag_t*));
	map_init(&module.user_type_tags);

	vec_push_back(&script->modules, &module);
	return script->modules.length - 1;
}

static void apply_import_directive(script_t* script, const char* filename)
{
	script_module_t* module = (script_module_t*)vec_get(&script->modules, g_cur_module_index);
	char* dir = estrdup(module->local_path);
	char* sep = strrchr(dir, '/');
	if (sep)
		*(sep + 1) = 0;
	else
		*dir = 0;

	size_t dirlen = strlen(dir);
	size_t namelen = strlen(filename);
	char* path = emalloc(dirlen + namelen + 1);
	strcpy(path, dir);
	strcpy(path + dirlen, filename);

	FILE* file = fopen(path, "rb");
	if(file)
	{
		fseek(file, 0, SEEK_END);
		size_t length = ftell(file);
		fseek(file, 0, SEEK_SET);
		
		char* code = emalloc(length + 1);
		fread(code, 1, length, file);
		code[length] = '\0';
		
		int module_index = add_module(script, path, NULL, code);
		
		script_module_t* current_module = vec_get(&script->modules, g_cur_module_index);
		vec_push_back(&current_module->referenced_modules, &module_index);

		free(code);
		fclose(file);
	}
	else
		fprintf(stderr, "Unable to open file '%s' for importing\n", filename);

	free(path);
	free(dir);
}

static expr_t* parse_factor(script_t* script);
static expr_t* parse_directive(script_t* script)
{
	if(strcmp(g_lexeme, "#import") == 0)
	{
		get_next_token();
		
		if(g_cur_tok != TOK_STRING) error_exit_p("Expected string after '#import' but received '%s'\n", g_token_names[g_cur_tok]);
		
		apply_import_directive(script, g_lexeme);
		get_next_token();
	
		return parse_factor(script);
	}
	else if(strcmp(g_lexeme, "#on_compile") == 0)
	{
		get_next_token();
		
		expr_t* exp = parse_expr(script);
		
		script_module_t* cur_module = vec_get(&script->modules, g_cur_module_index);
		vec_push_back(&cur_module->compile_time_blocks, &exp);

		// NOTE: The compile time block is not a part of the runtime
		// program.
		return parse_factor(script);
	}
	else
		error_exit_p("Invalid directive '%s'\n", g_lexeme);
	return NULL;
}

static expr_t* parse_atomic(script_t* script)
{
	expr_t* exp = create_expr(EXP_ATOMIC);
	get_next_token();

	exp->atomx = parse_expr(script);
	return exp;
}

static expr_t* parse_factor(script_t* script)
{
	switch(g_cur_tok)
	{
		case TOK_ATOMIC: return parse_atomic(script);
		case TOK_DIRECTIVE: return parse_directive(script);
		case TOK_NULL: return parse_null(script);
		case TOK_TRUE:
		case TOK_FALSE: return parse_bool(script);
		case TOK_CHAR: return parse_char(script);
		case TOK_VAR: return parse_var(script);
		case TOK_IDENT: return parse_ident(script);
		case TOK_OPENPAREN: return parse_paren(script);
		case TOK_OPENCURLY: return parse_block(script);
		case TOK_IF: return parse_if(script);
		case TOK_WHILE: return parse_while(script);
		case TOK_FOR: return parse_for(script);
		case TOK_FUNC: return parse_func(script);
		case TOK_NUMBER: return parse_number(script);
		case TOK_STRING: return parse_string(script);
		case TOK_RETURN: return parse_return(script);
		case TOK_WRITE: return parse_write(script);
		case TOK_OPENSQUARE: return parse_array_literal(script);
		case TOK_EXTERN: return parse_extern(script);
		case TOK_LEN: return parse_len(script);
		case TOK_UNION:
		case TOK_STRUCT: return parse_struct(script);
		
		default:
			error_exit_p("Unexpected token '%s'\n", g_token_names[g_cur_tok]);
	}
	
	return NULL;
}

static int get_prec(int op)
{
	switch(op)
	{
		case TOK_ASSIGN: return 0;
		case TOK_LAND: case TOK_LOR: return 1;
		case TOK_LT: case TOK_GT: case TOK_LTE: case TOK_GTE: case TOK_EQUALS: case TOK_NOTEQUAL: return 2;
		case TOK_PLUS: case TOK_MINUS: return 3;
		case TOK_MUL: case TOK_DIV: case TOK_MOD: return 4;
	
		default:
			return -1;
	}
}

static expr_t* parse_expr(script_t* script);
static expr_t* parse_post(script_t* script, expr_t* pre)
{
	switch(g_cur_tok)
	{
		case TOK_DOT:
		{
			expr_t* exp = create_expr(EXP_DOT);
			get_next_token();
			
			if(g_cur_tok != TOK_IDENT) error_exit_p("Expected identifier after '.' but received '%s'\n", g_token_names[g_cur_tok]);
			
			exp->dotx.value = pre;
			exp->dotx.name = estrdup(g_lexeme);
			
			get_next_token();
			
			return parse_post(script, exp);
		} break;
		
		case TOK_COLON:
		{
			expr_t* exp = create_expr(EXP_COLON);
			get_next_token();
			
			if(g_cur_tok != TOK_IDENT) error_exit_p("Expected identifier after ':' but received '%s'\n", g_token_names[g_cur_tok]);
			
			exp->dotx.value = pre;
			exp->dotx.name = estrdup(g_lexeme);
			
			get_next_token();
			
			return parse_post(script, exp);
		} break;
		
		case TOK_OPENPAREN:
		{
			expr_t* exp = create_expr(EXP_CALL);
			exp->callx.func = pre;
			vec_init(&exp->callx.args, sizeof(expr_t*));
			
			if(pre->type == EXP_COLON)
			{
				expr_t* cpy = create_expr(pre->type);
				
				memcpy(cpy, pre->dotx.value, sizeof(expr_t));
				cpy->is_shallow_copy = 1;
				
				vec_push_back(&exp->callx.args, &cpy);
			}
			
			get_next_token();
			
			while(g_cur_tok != TOK_CLOSEPAREN)
			{
				expr_t* arg = parse_expr(script);
				vec_push_back(&exp->callx.args, &arg);
				
				if(g_cur_tok == TOK_COMMA) get_next_token();
				else if(g_cur_tok != TOK_CLOSEPAREN) error_exit_p("Expected ')' after '(' in function call expression but received '%s'\n", g_token_names[g_cur_tok]);
			}
			get_next_token();
			
			return parse_post(script, exp);
		} break;
		
		case TOK_OPENSQUARE:
		{
			expr_t* exp = create_expr(EXP_ARRAY_INDEX);
			exp->array_index.array = pre;
			get_next_token();
			
			exp->array_index.index = parse_expr(script);
			if(g_cur_tok != TOK_CLOSESQUARE) error_exit_p("Expected ']' after '[' in array index expression but received '%s'\n", g_token_names[g_cur_tok]);
			get_next_token();
			
			return parse_post(script, exp);
		} break;
		
		default:
			return pre;
	}
}

static expr_t* parse_unary(script_t* script)
{
	switch(g_cur_tok)
	{
		case TOK_NEW:
		{
			get_next_token();
			
			expr_t* exp = NULL;
			
			if(g_cur_tok == TOK_IDENT)
			{
				type_tag_t* tag = get_type_tag_from_name(script, g_lexeme);
				if(tag->type == TAG_STRUCT)
				{
					exp = create_expr(EXP_STRUCT_NEW);
					
					exp->newx.type = tag;
					vec_init(&exp->newx.init, sizeof(expr_t*));
					
					get_next_token();
					
					if(g_cur_tok == TOK_OPENCURLY)
					{
						get_next_token();
						while(g_cur_tok != TOK_CLOSECURLY)
						{
							expr_t* e = parse_expr(script);
							if(e->type != EXP_BINARY || e->binx.lhs->type != EXP_VAR || e->binx.op != TOK_ASSIGN)
								error_exit_p("Invalid initializer expression for %s\n", tag->ds.name);
							
							vec_push_back(&exp->newx.init, &e);
							if(g_cur_tok == TOK_COMMA) get_next_token();
						}
						get_next_token();
					}
					
					return exp;
				}
				
				// NOTE: http://xkcd.com/292/
				goto not_type;
			}
		
		not_type:
			exp = create_expr(EXP_VAR);
			exp->varx.name = estrdup("new");
			exp->varx.decl = reference_variable(script, "new");
			
			return parse_post(script, exp);
		} break;
		
		case TOK_NOT: case TOK_MINUS:
		{
			expr_t* exp = create_expr(EXP_UNARY);
			exp->unaryx.op = g_cur_tok;
			get_next_token();
			
			exp->unaryx.rhs = parse_expr(script);
			return parse_post(script, exp);
		} break;
		
		default:
			return parse_post(script, parse_factor(script));
	}
}

static expr_t* parse_bin_rhs(script_t* script, expr_t* lhs, int eprec)
{
	while(1)
	{
		int prec = get_prec(g_cur_tok);
	
		if(prec < eprec)
			return lhs;
	
		int op = g_cur_tok;
		
		get_next_token();
		
		expr_t* rhs = parse_unary(script);
		if(get_prec(g_cur_tok) > prec)
			rhs = parse_bin_rhs(script, rhs, prec + 1);
		
		expr_t* newLhs = create_expr(EXP_BINARY);
		
		newLhs->binx.lhs = lhs;
		newLhs->binx.rhs = rhs;
		newLhs->binx.op = op;
		
		lhs = newLhs;
	}
}

static expr_t* parse_expr(script_t* script)
{
	expr_t* e = parse_unary(script);
	return parse_bin_rhs(script, e, 0);
}

static void parse_program(script_t* script, vector_t* program)
{
	get_token(1);
	get_next_token();
	
	while(g_cur_tok != TOK_EOF)
	{
		expr_t* e = parse_expr(script);
		vec_push_back(program, &e);
	}
}

static void debug_expr(script_t* script, expr_t* exp)
{
	switch(exp->type)
	{
		case EXP_NULL:
		{
			printf("null");
		} break;
		
		case EXP_STRUCT_DECL:
		{
			printf("struct %s", exp->struct_tag->ds.name);
		} break;
		
		case EXP_STRUCT_NEW:
		{
			printf("new %s", exp->newx.type->ds.name);
		} break;
		
		case EXP_DOT: case EXP_COLON:
		{
			debug_expr(script, exp->dotx.value); printf(".%s", exp->dotx.name);
		} break;
		
		case EXP_VAR: printf("%s", exp->varx.name); break;
		case EXP_BOOL: printf("%s", exp->boolean_value ? "true" : "false"); break;
		case EXP_CHAR: printf("'%c'", exp->code); break;
		case EXP_NUMBER: printf("%g", vec_get_value(&script->numbers, exp->number_index, double)); break;
		case EXP_STRING: printf("\"%s\"", vec_get_value(&script->strings, exp->string_index, script_string_t).data); break;
		case EXP_PAREN: printf("( "); debug_expr(script, exp->paren); printf(" )"); break;
		
		case EXP_ARRAY_INDEX:
		{
			debug_expr(script, exp->array_index.array);
			printf("[");
			debug_expr(script, exp->array_index.index);
			printf("]");
		} break;
		
		case EXP_ARRAY_LITERAL:
		{
			printf("[");
			for(int i = 0; i < exp->array_literal.values.length; ++i)
			{
				debug_expr(script, vec_get_value(&exp->array_literal.values, i, expr_t*));
				if(i + 1 < exp->array_literal.values.length) printf(", ");
			}
			printf("]");
		} break;
		
		case EXP_BLOCK:
		{
			printf("{\n");
			for(int i = 0; i < exp->block.length; ++i)
			{
				expr_t* e = vec_get_value(&exp->block, i, expr_t*);
				debug_expr(script, e);
				printf("\n");
			}
			printf("}\n");
		} break;
		
		case EXP_FUNC:
		{
			printf("func %s(", exp->funcx.decl->name);
			for(int i = 0; i < exp->funcx.decl->args.length; ++i)
			{
				var_decl_t* decl = vec_get(&exp->funcx.decl->args, i);
				printf("%s", decl->name);
				if(i + 1 < exp->funcx.decl->args.length) printf(", ");
			} 
			printf(")\n{\n");
			
			debug_expr(script, exp->funcx.body);
			printf("\n}");
		} break;
		
		case EXP_RETURN:
		{
			if(exp->retx.value)
			{
				printf("return ");
				debug_expr(script, exp->retx.value);
			}
			else
				printf("return;");
		} break;
		
		case EXP_IF:
		{
			printf("if ");
			debug_expr(script, exp->ifx.cond);
			
			printf("\n{\n");
			debug_expr(script, exp->ifx.body);
			printf("\n}");
			
			if(exp->ifx.alt)
			{
				printf("\nelse ");
				debug_expr(script, exp->ifx.alt);
			}
		} break;
		
		case EXP_WHILE:
		{
			printf("while ");
			debug_expr(script, exp->whilex.cond);
			printf("\n{\n");
			debug_expr(script, exp->whilex.body);
			printf("\n}");
		} break;
		
		case EXP_CALL:
		{
			debug_expr(script, exp->callx.func);
			printf("(");
			for(int i = 0; i < exp->callx.args.length; ++i)
			{
				expr_t* e = vec_get_value(&exp->callx.args, i, expr_t*);
				debug_expr(script, e);
				if(i + 1 < exp->callx.args.length) printf(", ");
			}
			printf(")");
		} break;
		
		case EXP_EXTERN: printf("extern %s", exp->extern_decl->name); break;
		case EXP_EXTERN_LIST:
		{
			printf("extern {\n");
			for (size_t i = 0; i < exp->extern_array.length; ++i)
			{
				printf("\t");
				expr_t* e = vec_get_value(&exp->extern_array, i, expr_t*);
				debug_expr(script, e);
			}
			printf("}\n");
		} break;

		case EXP_UNARY: printf("%s", g_token_names[exp->unaryx.op]); debug_expr(script, exp->unaryx.rhs); break;
		case EXP_BINARY: printf("("); debug_expr(script, exp->binx.lhs); printf(" %s ", g_token_names[exp->binx.op]); debug_expr(script, exp->binx.rhs); printf(")"); break;
		case EXP_WRITE: printf("write "); debug_expr(script, exp->write); break;
		
		case EXP_LEN: printf("len "); debug_expr(script, exp->len); break;
	}
}

static inline void append_code(script_t* script, word w)
{
	vec_push_back(&script->code, &w);
}

static inline void append_int(script_t* script, int v)
{
	word* vp = (word*)(&v);
	for(int i = 0; i < sizeof(int) / sizeof(word); ++i)
		append_code(script, *vp++);
}

static inline void patch_int(script_t* script, int loc, int v)
{
	word* vp = (word*)(&v);
	for(int i = 0; i < sizeof(int) / sizeof(word); ++i)
		vec_set(&script->code, loc + i, vp++);
}

static void flatten_expr(vector_t* expr_list, expr_t* exp)
{
	vec_push_back(expr_list, &exp);

	switch (exp->type)
	{
		case EXP_NULL:
		case EXP_BOOL:
		case EXP_CHAR:
		case EXP_NUMBER:
		case EXP_STRING:
		case EXP_EXTERN:
		case EXP_VAR:
		case EXP_STRUCT_DECL:
			break;

		case EXP_EXTERN_LIST:
		{
			for (int i = 0; i < exp->extern_array.length; ++i)
				flatten_expr(expr_list, vec_get_value(&exp->extern_array, i, expr_t*));
		} break;

		case EXP_DOT:
		case EXP_COLON:
		{
			flatten_expr(expr_list, exp->dotx.value);
		} break;

		case EXP_STRUCT_NEW:
		{
			for (int i = 0; i < exp->newx.init.length; ++i)
				flatten_expr(expr_list, vec_get_value(&exp->newx.init, i, expr_t*));
		} break;

		case EXP_PAREN:
		{
			flatten_expr(expr_list, exp->paren);
		} break;

		case EXP_ARRAY_INDEX:
		{
			flatten_expr(expr_list, exp->array_index.array);
			flatten_expr(expr_list, exp->array_index.index);
		} break;

		case EXP_ARRAY_LITERAL:
		{
			for (int i = 0; i < exp->array_literal.values.length; ++i)
				flatten_expr(expr_list, vec_get_value(&exp->array_literal.values, i, expr_t*));
		} break;
		
		case EXP_UNARY:
		{
			flatten_expr(expr_list, exp->unaryx.rhs);
		} break;

		case EXP_BINARY:
		{
			flatten_expr(expr_list, exp->binx.lhs);
			flatten_expr(expr_list, exp->binx.rhs);
		} break;

		case EXP_CALL:
		{
			flatten_expr(expr_list, exp->callx.func);
	
			for (int i = 0; i < exp->callx.args.length; ++i)
				flatten_expr(expr_list, vec_get_value(&exp->callx.args, i, expr_t*));
		} break;

		case EXP_RETURN:
		{
			flatten_expr(expr_list, exp->retx.value);
		} break;

		case EXP_BLOCK:
		{
			for (int i = 0; i < exp->block.length; ++i)
				flatten_expr(expr_list, vec_get_value(&exp->block, i, expr_t*));
		} break;

		case EXP_FUNC:
		{
			flatten_expr(expr_list, exp->funcx.body);
		} break;

		case EXP_ATOMIC:
		{
			flatten_expr(expr_list, exp->atomx);
		} break;

		case EXP_IF:
		{
			flatten_expr(expr_list, exp->ifx.cond);
			flatten_expr(expr_list, exp->ifx.body);

			if (exp->ifx.alt)
				flatten_expr(expr_list, exp->ifx.alt);
		} break;

		case EXP_WHILE:
		{
			flatten_expr(expr_list, exp->whilex.cond);
			flatten_expr(expr_list, exp->whilex.body);
		} break;

		case EXP_FOR:
		{
			flatten_expr(expr_list, exp->forx.init);
			flatten_expr(expr_list, exp->forx.cond);
			flatten_expr(expr_list, exp->forx.step);
			flatten_expr(expr_list, exp->forx.body);
		} break;

		case EXP_WRITE:
		{
			flatten_expr(expr_list, exp->write);
		} break;

		case EXP_LEN:
		{
			flatten_expr(expr_list, exp->len);
		} break;

		default:
			assert(0);
			break;
	}
}

static void resolve_symbols(script_t* script, expr_t* exp)
{
	switch(exp->type)
	{
		case EXP_ATOMIC:
		{
			resolve_symbols(script, exp->atomx);
		} break;
		
		case EXP_DOT: case EXP_COLON:
		{
			resolve_symbols(script, exp->dotx.value);
		} break;
		
		case EXP_STRUCT_NEW:
		{
			for(int i = 0; i < exp->newx.init.length; ++i)
				resolve_symbols(script, vec_get_value(&exp->newx.init, i, expr_t*));
		} break;
		
		case EXP_STRUCT_DECL:
		{
			for(int i = 0; i < exp->struct_tag->ds.members.length; ++i)
			{
				type_tag_member_t* mem = vec_get(&exp->struct_tag->ds.members, i);
				if(mem->default_value)
					resolve_symbols(script, mem->default_value);
			}
		} break;
		
		case EXP_NULL:
		case EXP_EXTERN:
		case EXP_EXTERN_LIST:
		case EXP_NUMBER:
		case EXP_STRING:
		case EXP_CHAR:
		case EXP_BOOL: break;
	
		case EXP_VAR:
		{
			if(exp->varx.decl) return;
			exp->varx.decl = reference_variable(script, exp->varx.name);
			if(!exp->varx.decl) 
			{
				if(reference_function(script, exp->varx.name)) return;
				if(get_type_tag_from_name(script, exp->varx.name)) return;
				error_defer_e(exp, "Attempted to reference undeclared entity '%s'\n", exp->varx.name);
			}
		} break;
		
		case EXP_UNARY:
		{
			resolve_symbols(script, exp->unaryx.rhs);
		} break;
		
		case EXP_BINARY:
		{
			resolve_symbols(script, exp->binx.lhs);
			resolve_symbols(script, exp->binx.rhs);
		} break;
		
		case EXP_PAREN:
		{
			resolve_symbols(script, exp->paren);
		} break;
		
		case EXP_BLOCK:
		{
			for(int i = 0; i < exp->block.length; ++i)
				resolve_symbols(script, vec_get_value(&exp->block, i, expr_t*));
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			resolve_symbols(script, exp->array_index.array);
			resolve_symbols(script, exp->array_index.index);
		} break;
		
		case EXP_ARRAY_LITERAL:
		{
			for(int i = 0; i < exp->array_literal.values.length; ++i)
				resolve_symbols(script, vec_get_value(&exp->array_literal.values, i, expr_t*));
		} break;
		
		case EXP_IF:
		{
			resolve_symbols(script, exp->ifx.cond);
			resolve_symbols(script, exp->ifx.body);
			if(exp->ifx.alt) resolve_symbols(script, exp->ifx.alt);
		} break;
		
		case EXP_WHILE:
		{
			resolve_symbols(script, exp->whilex.cond);
			resolve_symbols(script, exp->whilex.body);
		} break;
		
		case EXP_FOR:
		{
			resolve_symbols(script, exp->forx.init);
			resolve_symbols(script, exp->forx.cond);
			resolve_symbols(script, exp->forx.step);
			resolve_symbols(script, exp->forx.body);
		} break;

		case EXP_RETURN:
		{
			if(exp->retx.value)
				resolve_symbols(script, exp->retx.value);
		} break;
		
		case EXP_CALL:
		{
			for(int i = 0; i < exp->callx.args.length; ++i)
				resolve_symbols(script, vec_get_value(&exp->callx.args, i, expr_t*));
			resolve_symbols(script, exp->callx.func);
		} break;
		
		case EXP_FUNC:
		{
			resolve_symbols(script, exp->funcx.body);
		} break;
		
		case EXP_WRITE:
		{
			resolve_symbols(script, exp->write);
		} break;
		
		case EXP_LEN:
		{
			resolve_symbols(script, exp->len);
		} break;
	}
}

static char are_assignment_types_valid(expr_t* lhs, expr_t* rhs)
{
	return compare_type_tags(lhs->tag, rhs->tag);
}

static void finalize_type(const char* key, void* v_tag, void* data)
{
	type_tag_t* tag = v_tag;
	if(tag->defined && !tag->finalized)
	{
		if(tag->type != TAG_STRUCT) error_exit_c(tag->ctx, "Attempted to use non-struct type in struct '%s'\n", tag->ds.name);
		
		for(int using_index = 0; using_index < tag->ds.using.length; ++using_index)
		{
			type_tag_t* using = vec_get_value(&tag->ds.using, using_index, type_tag_t*);
			
			for(int mem_id = 0; mem_id < using->ds.members.length; ++mem_id)
			{
				type_tag_member_t* mem = vec_get(&using->ds.members, mem_id);
				
				type_tag_member_t mem_copy;
				mem_copy.name = estrdup(mem->name);
				mem_copy.index = mem->index + tag->ds.size;
				mem_copy.type = mem->type;
			
				if(mem->default_value)
				{
					// NOTE: Making a shallow copy of the default_value expression
					mem_copy.default_value = emalloc(sizeof(expr_t));
					memcpy(mem_copy.default_value, mem->default_value, sizeof(expr_t));
					mem_copy.default_value->is_shallow_copy = 1;
				}
				else
					mem_copy.default_value = NULL;
				
				vec_push_back(&tag->ds.members, &mem_copy);
			}
			
			tag->ds.size += using->ds.size;
		}
		
		tag->finalized = 1;
	}
}

// NOTE: Called before 'resolve_symbols' but after 'check_all_tags_defined'
// NOTE: All the 'using' statements inside structs are evaluated
static void finalize_types(script_t* script)
{	
	for (int i = 0; i < script->modules.length; ++i)
	{
		script_module_t* module = vec_get(&script->modules, i);
		map_traverse(&module->user_type_tags, finalize_type, NULL);
	}
}

static func_decl_t* get_struct_member_function(script_t* script, const char* struct_name, const char* func_name)
{
	size_t la = strlen(struct_name);
	size_t length = la + strlen(func_name) + 2;
	type_tag_t* struct_tag = get_user_type_tag_from_name(script, struct_name);
	if (!struct_tag) return NULL;

	// NOTE: Stores (struct_name)_(func_name)
	// which is ultimately the name of the member function
	// in the global scope
	char* buf = emalloc(length);

	strcpy(buf, struct_name);
	buf[la] = '_';
	strcpy(buf + la + 1, func_name);

	func_decl_t* result = reference_function(script, buf);
	free(buf);

	if (result && (result->args.length < 1 || !compare_type_tags(vec_get_value(&result->args, 0, var_decl_t*)->tag, struct_tag)))
		return NULL;

	return result;
}

// NOTE: Recurses in search for an EXP_VAR
// in order to resolve a var_decl (which should exist because
// resolve_symbols has been called). 
static var_decl_t* get_root_var_decl(expr_t* exp)
{
	switch (exp->type)
	{
		case EXP_VAR:
		{
			return exp->varx.decl;
		} break;

		case EXP_CALL:
		{
			return get_root_var_decl(exp->callx.func);
		} break;

		case EXP_ARRAY_INDEX:
		{
			return get_root_var_decl(exp->array_index.array);
		} break;

		default:
			return NULL;
	}
}

static int get_struct_type_member_index(type_tag_t* tag, const char* name);
// TODO: Add type names to error messages
// NOTE: resolve_symbols before this
static void resolve_type_tags(script_t* script, void* vexp)
{
	expr_t* exp = vexp;
	
	// NOTE: Pathetic attempt at providing proper
	// line information for type warnings
	g_file = exp->ctx.file;
	g_line = exp->ctx.line;
	
	switch(exp->type)
	{
		case EXP_ATOMIC:
		{
			exp->tag = create_type_tag(script, TAG_VOID);
			resolve_type_tags(script, exp->atomx);
		} break;

		case EXP_DOT: case EXP_COLON:
		{
			resolve_type_tags(script, exp->dotx.value);
			if(exp->dotx.value->tag->type != TAG_STRUCT) error_defer_e(exp->dotx.value, "Attempted to access members in non-struct type\n");
			
			char found = 0;
			
			type_tag_t* tag = exp->dotx.value->tag;
			for(int i = 0; i < tag->ds.members.length; ++i)
			{
				type_tag_member_t* mem = vec_get(&tag->ds.members, i);
				if(strcmp(mem->name, exp->dotx.name) == 0)
				{
					exp->tag = mem->type;
					found = 1;
					
					if(exp->type == EXP_COLON) error_defer_e(exp, "Attempted to use ':' operator to access member value; use '.' instead\n");
					break;
				}
			}
			
			if (!found)
			{
				func_decl_t* decl = get_struct_member_function(script, exp->dotx.value->tag->ds.name, exp->dotx.name);
				if (decl) found = 1;
				if (exp->type == EXP_DOT) error_defer_e(exp, "Attempted to use '.' operator to access member function; use ':' instead\n");

				if (!found)
					error_exit_e(exp, "Attempted to access non-existent member '%s' in struct %s\n", exp->dotx.name, tag->ds.name);
				else
					exp->tag = decl->tag;
			}
		} break;
		
		case EXP_NULL:
		{
			exp->tag = create_type_tag(script, TAG_DYNAMIC);
		} break;
		
		case EXP_STRUCT_DECL:
		{
			exp->tag = create_type_tag(script, TAG_VOID);
			
			for(int i = 0; i < exp->struct_tag->ds.members.length; ++i)
			{
				type_tag_member_t* mem = vec_get(&exp->struct_tag->ds.members, i);
				if(mem->default_value)
				{	
					resolve_type_tags(script, mem->default_value);
					if(!compare_type_tags(mem->type, mem->default_value->tag))
						error_defer_e(mem->default_value, "Type of default value does not match type of member variable '%s'\n", mem->name);
				}
			}
		} break;
		
		case EXP_STRUCT_NEW:
		{
			exp->tag = exp->newx.type;
			
			for(int i = 0; i < exp->newx.init.length; ++i)
			{
				expr_t* init = vec_get_value(&exp->newx.init, i, expr_t*);
				
				int member_index = get_struct_type_member_index(exp->newx.type, init->binx.lhs->varx.name);
				resolve_type_tags(script, init->binx.rhs);
					
				type_tag_member_t* mem = vec_get(&exp->newx.type->ds.members, member_index);
				if(!compare_type_tags(init->binx.rhs->tag, mem->type))
					error_defer_e(init->binx.rhs, "Type of initializer does not match type of member '%s' being initialized\n", mem->name);
			}
		} break;
			
		case EXP_NUMBER:
		{
			exp->tag = create_type_tag(script, TAG_NUMBER);
		} break;
		
		case EXP_STRING:
		{
			exp->tag = create_type_tag(script, TAG_STRING);
		} break;
		
		case EXP_BOOL:
		{
			exp->tag = create_type_tag(script, TAG_BOOL);
		} break;
		
		case EXP_CHAR:
		{
			exp->tag = create_type_tag(script, TAG_CHAR);
		} break;
		
		case EXP_VAR:
		{
			if(!exp->varx.decl)
			{
				func_decl_t* decl = reference_function(script, exp->varx.name);
				if(decl) exp->tag = decl->tag;
				else error_exit_e(exp, "Referencing undeclared function/variable '%s'\n", exp->varx.name); 
			}
			else
				exp->tag = exp->varx.decl->tag;
		} break;
		
		case EXP_WRITE:
		{
			exp->tag = create_type_tag(script, TAG_VOID);
			resolve_type_tags(script, exp->write);
		} break;
		
		case EXP_UNARY:
		{
			resolve_type_tags(script, exp->unaryx.rhs);
			switch(exp->unaryx.op)
			{
				case TOK_NOT:
				{
					if(exp->unaryx.rhs->tag->type != TAG_BOOL)
						error_defer_e(exp->unaryx.rhs, "Attempted to use unary ! operator on non-boolean value\n");
					exp->tag = create_type_tag(script, TAG_BOOL);
				} break;
				
				case TOK_MINUS:
				{
					if(exp->unaryx.rhs->tag->type != TAG_NUMBER)
						error_defer_e(exp->unaryx.rhs, "Attempted to use unary - operator on non-numerical value\n");
					exp->tag = create_type_tag(script, TAG_NUMBER);
				} break;
				
				default:
					error_exit_e(exp, "No type resolution for %s operator\n", g_token_names[exp->unaryx.op]);
					break;
			}
		} break;
		
		case EXP_BINARY:
		{
			resolve_type_tags(script, exp->binx.lhs);
			resolve_type_tags(script, exp->binx.rhs);
		
			// NOTE: assignment operation lhs does not need to be
			// number
			if(exp->binx.op != TOK_ASSIGN)
			{ 
				if(!compare_type_tags(exp->binx.lhs->tag, exp->binx.rhs->tag) || !is_type_tag(exp->binx.lhs->tag, TAG_NUMBER))
				{	
					if(exp->binx.op != TOK_EQUALS && exp->binx.op != TOK_NOTEQUAL && 
						exp->binx.op != TOK_LAND && exp->binx.op != TOK_LOR)
						error_defer_e(exp->binx.lhs, "Invalid types in binary %s operation\n", g_token_names[exp->binx.op]);
				}
			}
			else
			{
				// NOTE: If the type of the lhs is unknown, set it to the type of the rhs
				// (since this is an assignment, after all)
				if (exp->binx.lhs->tag->type == TAG_UNKNOWN)
				{
					exp->binx.lhs->tag = exp->binx.rhs->tag;

					var_decl_t* decl = get_root_var_decl(exp->binx.lhs);
					if(decl)
						decl->tag = exp->binx.lhs->tag;
				}
				else if (!are_assignment_types_valid(exp->binx.lhs, exp->binx.rhs))
					error_defer_e(exp->binx.lhs, "Type of lhs in assignment operation does not match the type of rhs\n");
			}

			switch(exp->binx.op)
			{
				case TOK_PLUS:
				case TOK_MINUS:
				case TOK_MUL:
				case TOK_DIV: 
				case TOK_MOD: exp->tag = create_type_tag(script, TAG_NUMBER); break;
				
				case TOK_LTE:
				case TOK_GTE:
				case TOK_LT:
				case TOK_GT:
				case TOK_EQUALS:
				case TOK_NOTEQUAL:
				case TOK_LAND:
				case TOK_LOR: exp->tag = create_type_tag(script, TAG_BOOL); break;
				
				default: exp->tag = create_type_tag(script, TAG_VOID); break;
			}
		} break;
		
		case EXP_CALL:
		{
			resolve_type_tags(script, exp->callx.func);
			if(exp->callx.func->tag->type == TAG_DYNAMIC)
				warn_e(exp, WARN_CALL_DYNAMIC);
			
			if(exp->callx.func->tag->type != TAG_FUNC && exp->callx.func->tag->type != TAG_DYNAMIC)
				error_defer_e(exp, "Attempting to call something that is not a function\n");
			
			if(exp->callx.func->tag->type != TAG_DYNAMIC && exp->callx.args.length != exp->callx.func->tag->func.arg_types.length)
				error_defer_e(exp, "Passed %d argument(s) into function expecting %d argument(s)\n", 
				exp->callx.args.length, exp->callx.func->tag->func.arg_types.length);
			else
			{
				for(int i = 0; i < exp->callx.args.length; ++i)
				{
					expr_t* arg = vec_get_value(&exp->callx.args, i, expr_t*);
					resolve_type_tags(script, arg);
					
					if(exp->callx.func->tag->type != TAG_DYNAMIC)
					{
						type_tag_t* expected_tag = vec_get_value(&exp->callx.func->tag->func.arg_types, i, type_tag_t*);
						if(!compare_type_tags(arg->tag, expected_tag)) error_defer_e(arg, "Type of argument %i does not match expected type\n", i + 1);
					}
				}
			}
			
			exp->tag = exp->callx.func->tag->func.return_type;
		} break;
		
		case EXP_PAREN:
		{
			resolve_type_tags(script, exp->paren);
			exp->tag = exp->paren->tag;
		} break;
		
		case EXP_BLOCK:
		{
			exp->tag = create_type_tag(script, TAG_VOID);
			for(int i = 0; i < exp->block.length; ++i)
			{
				expr_t* e = vec_get_value(&exp->block, i, expr_t*);
				resolve_type_tags(script, e);
			}
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			resolve_type_tags(script, exp->array_index.array);
			resolve_type_tags(script, exp->array_index.index);

			if(!is_type_tag(exp->array_index.index->tag, TAG_NUMBER))
				error_defer_e(exp->array_index.index, "Attempting to index array with non-number value\n");
			
			switch(exp->array_index.array->tag->type)
			{
				case TAG_STRING: exp->tag = create_type_tag(script, TAG_CHAR); break;
				case TAG_ARRAY: exp->tag = exp->array_index.array->tag->contained; break;
				default:
					error_defer_e(exp->array_index.array, "Attempting to index non-indexable type\n");
					break;
			}
		} break;
		
		case EXP_EXTERN:
		case EXP_EXTERN_LIST: break;
		
		case EXP_ARRAY_LITERAL:
		{
			for(int i = 0; i < exp->array_literal.values.length; ++i)
				resolve_type_tags(script, vec_get_value(&exp->array_literal.values, i, expr_t*));
			
			type_tag_t* tag = create_type_tag(script, TAG_ARRAY);
			if(exp->array_literal.contained) tag->contained = exp->array_literal.contained;
			else
			{
				expr_t* first = vec_get_value(&exp->array_literal.values, 0, expr_t*);
				tag->contained = first->tag;
				
				for(int i = 1; i < exp->array_literal.values.length; ++i)
				{
					expr_t* e = vec_get_value(&exp->array_literal.values, i, expr_t*);
					if(!compare_type_tags(e->tag, tag->contained))
					{
						warn_e(e, WARN_DYNAMIC_ARRAY_LITERAL);
						tag = create_type_tag(script, TAG_DYNAMIC);			// NOTE: okay, so it must be a dynamic literal
						break;										// NOTE: also, we don't care about the types anymore
					}
					// error_defer_e(e, "Array literal value type does not match the array's contained type\n");
				}
			}
			
			exp->tag = tag;
		} break;
		
		case EXP_LEN:
		{
			exp->tag = create_type_tag(script, TAG_NUMBER);
			resolve_type_tags(script, exp->len);
			
			if(exp->len->tag->type != TAG_STRING && exp->len->tag->type != TAG_ARRAY)
				error_defer_e(exp->len, "Attempted to find the length of non-measurable type\n");
		} break;
		
		case EXP_IF:
		{
			exp->tag = create_type_tag(script, TAG_VOID);
			resolve_type_tags(script, exp->ifx.cond);
			
			if(!is_type_tag(exp->ifx.cond->tag, TAG_BOOL))
				error_defer_e(exp->ifx.cond, "Condition does not evaluate to a boolean value\n");
			
			resolve_type_tags(script, exp->ifx.body);
			if(exp->ifx.alt)
				resolve_type_tags(script, exp->ifx.alt);
		} break;
		
		case EXP_WHILE:
		{
			exp->tag = create_type_tag(script, TAG_VOID);
			resolve_type_tags(script, exp->whilex.cond);
			
			if(!is_type_tag(exp->whilex.cond->tag, TAG_BOOL)) 
				error_defer_e(exp->whilex.cond, "Condition does not evaluate to a boolean value\n");
			
			resolve_type_tags(script, exp->whilex.body);
		} break;

		case EXP_FOR:
		{
			exp->tag = create_type_tag(script, TAG_VOID);

			resolve_type_tags(script, exp->forx.init);
			resolve_type_tags(script, exp->forx.cond);
			resolve_type_tags(script, exp->forx.step);

			if (!is_type_tag(exp->forx.cond->tag, TAG_BOOL))
				error_defer_e(exp->forx.cond, "Condition does not evaluate to a boolean value\n");

			resolve_type_tags(script, exp->forx.body);
		} break;
		
		case EXP_RETURN:
		{
			exp->tag = create_type_tag(script, TAG_VOID);
			resolve_type_tags(script, exp->retx.value);
	
			if(!compare_type_tags(exp->retx.value->tag, exp->retx.parent->tag->func.return_type))
				error_defer_e(exp, "Returned value's type does not match the return type of the enclosing function.\n");
		} break;
		
		case EXP_FUNC:
		{
			exp->tag = create_type_tag(script, TAG_VOID);
			resolve_type_tags(script, exp->funcx.body);
		} break;
	}
}

static void compile_value_expr(script_t* script, expr_t* exp);
static void compile_call(script_t* script, expr_t* exp)
{
	int nargs = exp->callx.args.length;

	if (exp->callx.func->type == EXP_COLON)
	{
		compile_value_expr(script, exp->callx.func->dotx.value);
		nargs += 1;
	}

	for(int i = 0; i < exp->callx.args.length; ++i)
	{
		expr_t* e = vec_get_value(&exp->callx.args, i, expr_t*);
		compile_value_expr(script, e);
	}

	compile_value_expr(script, exp->callx.func);
	
	append_code(script, OP_CALL);
	append_code(script, nargs);
}

static int get_struct_type_member_index(type_tag_t* tag, const char* name)
{
	// NOTE: In unions, all members are located at the same index (0)
	if(tag->ds.is_union)
		return 0;

	for(int i = 0; i < tag->ds.members.length; ++i)
	{
		type_tag_member_t* mem = vec_get(&tag->ds.members, i);
		if(strcmp(mem->name, name) == 0)
			return mem->index;
	}
	
	return -1;
}

static void compile_file_line_info(script_t* script, expr_t* exp, char ignore_last)
{
	if (exp->ctx.file)
	{
		if (ignore_last || !g_last_compiled_file || strcmp(g_last_compiled_file, exp->ctx.file) != 0)
		{
			append_code(script, OP_FILE);
			append_int(script, register_string(script, exp->ctx.file));
		}
		g_last_compiled_file = exp->ctx.file;
	}

	if(ignore_last || g_last_compiled_line != exp->ctx.line)
	{
		append_code(script, OP_LINE);
		append_int(script, exp->ctx.line);
	}
	g_last_compiled_line = exp->ctx.line;	
}

static void compile_value_expr(script_t* script, expr_t* exp)
{
	compile_file_line_info(script, exp, 0);
	
	switch(exp->type)
	{
		case EXP_NULL:
		{
			append_code(script, OP_PUSH_NULL);
		} break;
		
		case EXP_STRUCT_NEW:
		{
			char* initialized_members = emalloc(exp->newx.type->ds.members.length);
			memset(initialized_members, 0, exp->newx.type->ds.members.length);
			
			int num_init = 0;
			for(int i = exp->newx.init.length - 1; i >= 0; --i)
			{
				expr_t* init = vec_get_value(&exp->newx.init, i, expr_t*);
				int index = get_struct_type_member_index(exp->newx.type, init->binx.lhs->varx.name);
				
				if(index < 0) error_exit_e(init, "Attempted to initialize non-existent member '%s' in struct %s instantiation\n", 
							  init->binx.lhs->varx.name, exp->newx.type->ds.name);
							  
				initialized_members[index] = 1;
				++num_init;
				
				append_code(script, OP_PUSH_NUMBER);
				append_int(script, register_number(script, index));
				
				compile_value_expr(script, init->binx.rhs);
			}
			
			// NOTE: iterate over all the member types, check if they've been
			// initialized; if they haven't, and they have a default initializer
			// then use that
			for(int i = exp->newx.type->ds.members.length - 1; i >= 0; --i)
			{
				if(!initialized_members[i])
				{
					type_tag_member_t* mem = vec_get(&exp->newx.type->ds.members, i);
					if(mem->default_value)
					{
						append_code(script, OP_PUSH_NUMBER);
						append_int(script, register_number(script, i));
						
						compile_value_expr(script, mem->default_value);
						++num_init;
					}
				}
			}
			
			append_code(script, OP_PUSH_STRUCT);
			// NOTE: Unions only have 1 member location
			if(exp->newx.type->ds.is_union)
				append_int(script, 1);
			else
				append_int(script, exp->newx.type->ds.members.length);
			append_int(script, num_init);
			
			free(initialized_members);
		} break;
		
		case EXP_DOT:
		{
			int index = get_struct_type_member_index(exp->dotx.value->tag, exp->dotx.name);
			if(index < 0) error_exit_e(exp, "Attempted to access non-existent member '%s' in struct %s\n", exp->dotx.name, exp->dotx.value->tag->ds.name);
			
			compile_value_expr(script, exp->dotx.value);
			append_code(script, OP_STRUCT_GET);
			append_int(script, index);		
		} break;

		case EXP_COLON:
		{
			func_decl_t* decl = get_struct_member_function(script, exp->dotx.value->tag->ds.name, exp->dotx.name);
			if (!decl)
				error_exit_e(exp, "Value of type '%s' has no member function '%s'\n", exp->dotx.value->tag->ds.name, exp->dotx.name);

			append_code(script, OP_PUSH_FUNC);
			append_int(script, decl->index);
		} break;
			
		case EXP_BOOL:
		{
			if(exp->boolean_value) append_code(script, OP_PUSH_TRUE);
			else append_code(script, OP_PUSH_FALSE);
		} break;
		
		case EXP_CHAR:
		{
			append_code(script, OP_PUSH_CHAR);
			append_int(script, exp->code);
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			compile_value_expr(script, exp->array_index.index);
			compile_value_expr(script, exp->array_index.array);
			if(exp->array_index.array->tag->type == TAG_STRING)
				append_code(script, OP_STRING_GET);
			else
				append_code(script, OP_ARRAY_GET);
		} break;
		
		case EXP_ARRAY_LITERAL:
		{
			for(int i = 0; i < exp->array_literal.values.length; ++i)
			{
				expr_t* e = vec_get_value(&exp->array_literal.values, i, expr_t*);
				compile_value_expr(script, e);
			}
			append_code(script, OP_PUSH_ARRAY_BLOCK);
			append_int(script, (int)exp->array_literal.values.length);
		} break;
		
		case EXP_LEN:
		{
			compile_value_expr(script, exp->len);
			if(exp->len->tag->type == TAG_STRING) append_code(script, OP_STRING_LEN);
			else if(exp->len->tag->type == TAG_ARRAY) append_code(script, OP_ARRAY_LEN);
		} break;
		
		case EXP_VAR:
		{
			if(exp->varx.decl)
			{
				if(exp->varx.decl->parent)
					append_code(script, OP_GETLOCAL);
				else
					append_code(script, OP_GET);
					
				append_int(script, exp->varx.decl->index);
			}
			else
			{
				// NOTE: This assumes resolve_symbols checked for a function reference existing
				func_decl_t* decl = reference_function(script, exp->varx.name);
				
				// TODO: Switch here
				if(decl->type == DECL_FUNCTION) append_code(script, OP_PUSH_FUNC);
				else if (decl->type == DECL_EXTERN) append_code(script, OP_PUSH_EXTERN_FUNC);
				else error_exit_p("Invalid function declaration type\n");
				
				append_int(script, decl->index);
			}
		} break;
		
		case EXP_EXTERN:
		case EXP_EXTERN_LIST: break;
		
		case EXP_NUMBER:
		{
			append_code(script, OP_PUSH_NUMBER);
			append_int(script, exp->number_index);
		} break;
		
		case EXP_STRING:
		{
			append_code(script, OP_PUSH_STRING);
			append_int(script, exp->string_index);
		} break;
	
		case EXP_PAREN:
		{
			compile_value_expr(script, exp->paren);
		} break;
		
		case EXP_UNARY:
		{
			compile_value_expr(script, exp->unaryx.rhs);
			switch(exp->unaryx.op)
			{
				case TOK_MINUS: append_code(script, OP_NEG); break;
				case TOK_NOT: append_code(script, OP_NOT); break;
				// NOTE: perhaps error here
			}
		} break;
		
		case EXP_BINARY:
		{
			compile_value_expr(script, exp->binx.rhs);
			compile_value_expr(script, exp->binx.lhs);
			
			switch(exp->binx.op)
			{
				case TOK_PLUS: append_code(script, OP_ADD); break;
				case TOK_MINUS: append_code(script, OP_SUB); break;
				case TOK_MUL: append_code(script, OP_MUL); break;
				case TOK_DIV: append_code(script, OP_DIV); break;
				case TOK_MOD: append_code(script, OP_MOD); break;

				case TOK_LT: append_code(script, OP_LT); break;
				case TOK_GT: append_code(script, OP_GT); break;
				case TOK_LTE: append_code(script, OP_LTE); break;
				case TOK_GTE: append_code(script, OP_GTE); break;
				
				case TOK_LAND: append_code(script, OP_LAND); break;
				case TOK_LOR: append_code(script, OP_LOR); break;
				
				case TOK_EQUALS: append_code(script, OP_EQU); break;
				case TOK_NOTEQUAL: append_code(script, OP_EQU); append_code(script, OP_NOT); break;
				
				default: break;
			}
		} break;
		
		case EXP_CALL:
		{
			compile_call(script, exp);
			append_code(script, OP_PUSH_RETVAL);
			
			// NOTE: because branching
			compile_file_line_info(script, exp, 1);
		} break;
		
		default:
			error_exit_e(exp, "Non-value expression used in value context\n");
			break;
	}
}

static void compile_assign(script_t* script, expr_t* exp)
{
	switch(exp->type)
	{
		case EXP_VAR:
		{
			if(exp->varx.decl)
			{
				if(exp->varx.decl->parent)
					append_code(script, OP_SETLOCAL);
				else
					append_code(script, OP_SET);
				append_int(script, exp->varx.decl->index);
			}
			else
				error_exit_e(exp, "Attempted to assign to unassignable value\n");
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			compile_value_expr(script, exp->array_index.index);
			compile_value_expr(script, exp->array_index.array);
			append_code(script, OP_ARRAY_SET);
		} break;
		
		case EXP_DOT:
		{
			int index = get_struct_type_member_index(exp->dotx.value->tag, exp->dotx.name);
			if (index < 0) error_exit_e(exp, "Attempted to access non-existent member '%s' in struct '%s'\n", exp->dotx.name, exp->dotx.value->tag->ds.name);
			
			compile_value_expr(script, exp->dotx.value);
			append_code(script, OP_STRUCT_SET);
			append_int(script, index);
		} break;
		
		default: 
			error_exit_e(exp, "Invalid left-hand-side in assignment expression\n");
			break;
	};
}

static void compile_expr(script_t* script, expr_t* exp)
{
	compile_file_line_info(script, exp, 0);
	
	switch(exp->type)
	{
		case EXP_ATOMIC:
		{
			append_code(script, OP_ATOMIC_ENABLE);
			compile_expr(script, exp->atomx);
			append_code(script, OP_ATOMIC_DISABLE);
		} break;

		case EXP_STRUCT_DECL:
		case EXP_EXTERN:
		case EXP_EXTERN_LIST:
		case EXP_VAR:
		{
			// Do nothing for these
		} break;
		
		case EXP_BLOCK:
		{
			for(int i = 0; i < exp->block.length; ++i)
			{
				expr_t* e = vec_get_value(&exp->block, i, expr_t*);
				compile_expr(script, e);
			}
		} break;
		
		case EXP_RETURN:
		{
			if(!exp->retx.value)
				append_code(script, OP_RETURN);
			else
			{
				compile_value_expr(script, exp->retx.value);
				append_code(script, OP_RETURN_VALUE);
			}
		} break;
		
		case EXP_IF:
		{
			compile_value_expr(script, exp->ifx.cond);
			
			append_code(script, OP_GOTOZ);
			int loc = script->code.length;
			append_int(script, 0);
			
			compile_expr(script, exp->ifx.body);
			
			append_code(script, OP_GOTO);
			int exitLoc = script->code.length;
			append_int(script, 0);
			
			patch_int(script, loc, script->code.length);
			if(exp->ifx.alt)
				compile_expr(script, exp->ifx.alt);
			
			patch_int(script, exitLoc, script->code.length);
		} break;
		
		case EXP_WHILE:
		{
			int jump = script->code.length;
			compile_value_expr(script, exp->whilex.cond);
			
			append_code(script, OP_GOTOZ);
			int loc = script->code.length;
			append_int(script, 0);
			
			compile_expr(script, exp->whilex.body);
			append_code(script, OP_GOTO);
			append_int(script, jump);
			
			patch_int(script, loc, script->code.length);
		} break;

		case EXP_FOR:
		{
			compile_expr(script, exp->forx.init);
			int jump = script->code.length;
			compile_value_expr(script, exp->forx.cond);

			append_code(script, OP_GOTOZ);
			int loc = script->code.length;
			append_int(script, 0);

			compile_expr(script, exp->forx.body);
			compile_expr(script, exp->forx.step);

			append_code(script, OP_GOTO);
			append_int(script, jump);

			patch_int(script, loc, script->code.length);
		} break;
		
		case EXP_FUNC:
		{
			append_code(script, OP_GOTO);
			int loc = script->code.length;
			append_int(script, 0);
			
			vec_set(&script->function_pcs, exp->funcx.decl->index, &script->code.length);
			
			for(int i = 0; i < exp->funcx.decl->locals.length; ++i)
				append_code(script, OP_PUSH_NULL);
			
			compile_expr(script, exp->funcx.body);
			
			append_code(script, OP_RETURN);
			patch_int(script, loc, script->code.length);
		} break;
		
		case EXP_BINARY:
		{
			if(exp->binx.op != TOK_ASSIGN)
				error_exit_e(exp, "Value expression used in non-value context\n");
			
			compile_value_expr(script, exp->binx.rhs);
			compile_assign(script, exp->binx.lhs);
		} break;
		
		case EXP_CALL:
		{
			compile_call(script, exp);
			
			// NOTE: because branching
			compile_file_line_info(script, exp, 1);
		} break;
		
		case EXP_WRITE:
		{
			compile_value_expr(script, exp->write);
			append_code(script, OP_WRITE);
		} break;
		
		default:
			error_exit_e(exp, "Value expression used in non-value context\n");
	}
}

static void allocate_globals(script_t* script)
{
	static script_value_t null_val;
	null_val.type = VAL_NULL;

	script_value_t* pv = &null_val;

	int num_globals = 0;
	for (int i = 0; i < script->modules.length; ++i)
	{
		script_module_t* module = vec_get(&script->modules, i);
		num_globals += module->globals.length;
	}

	vec_resize(&script->globals, num_globals, &pv);
}

void script_bind_extern(script_t* script, const char* name, script_extern_t ext)
{
	char* name_copy = estrdup(name);
	vec_push_back(&script->extern_names, &name_copy);
	vec_push_back(&script->externs, &ext);
}

// DEFAULT EXTERNS

// NOTE: Fancy compile-time externs
#define EXT_CHECK_IF_CT(name) if(g_cur_module_index < 0) error_exit_script(script, "Attempted to call compile-time function '" name "' at runtime\n");

static void ext_add_module(script_t* script, vector_t* args)
{
	script_value_t* name_val = script_get_arg(args, 0);
	script_value_t* code_val = script_get_arg(args, 1);
	
	const char* name = name_val->string.data;
	const char* code = code_val->string.data;
	
	script_parse_code(script, code, NULL, name);
	script_push_number(script, script->modules.length - 1);
	script_return_top(script);
}

static void ext_load_module(script_t* script, vector_t* args)
{
	script_value_t* name_val = script_get_arg(args, 0);
	script_value_t* path_val = script_get_arg(args, 1);

	const char* name = name_val->string.data;
	const char* path = path_val->string.data;

	script_load_parse_file(script, path, name);
	script_push_number(script, script->modules.length - 1);
	script_return_top(script);
}

static void compile_module(script_t* script, script_module_t* module);
static void ext_compile_module(script_t* script, vector_t* args)
{
	append_code(script, OP_HALT);
	
	script_value_t* idx_val = script_get_arg(args, 0);
	int module_index = (int)idx_val->number;
	
	script_module_t* module = vec_get(&script->modules, module_index);
	compile_module(script, module);
}

static void execute_cycle(script_t* script);
static void ext_run_module(script_t* script, vector_t* args)
{
	script_value_t* idx_val = script_get_arg(args, 0);
	int module_index = (int)idx_val->number;
	
	script_module_t* module = vec_get(&script->modules, module_index);

	if(!module->compiled)
		error_exit_script(script, "Attempting to run an uncompiled module '%s'\n", module->name);

	int pc = script->pc;
	script->pc = module->start_pc;
	
	while(script->pc >= 0 && script->pc < module->end_pc)
		execute_cycle(script);
	
	script->pc = pc;
}

static void ext_get_current_module_index(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("get_current_module_index");
	script_push_number(script, g_cur_module_index);
}

static void ext_get_module_source_code(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("get_module_source_code");
	
	script_value_t* val = script_get_arg(args, 0);
	int module_index = (int)val->number;
	
	script_module_t* module = vec_get(&script->modules, module_index);
	script_push_cstr(script, module->source_code);
	script_return_top(script);
}

static script_value_t* new_value(script_t* script, script_value_type_t type);
static void ext_get_module_expr_list(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("get_module_expr_list");
	
	script_value_t* val = script_get_arg(args, 0);
	script_value_t* flatten_val = script_get_arg(args, 1);

	int module_index = (int)val->number;
	char flatten = flatten_val->boolean;

	script_module_t* module = vec_get(&script->modules, module_index);
	vector_t expr_list;

	vec_init(&expr_list, sizeof(script_value_t*));	

	for(int i = 0; i < module->expr_list.length; ++i)
	{
		if (flatten)
		{
			vector_t flat;
			vec_init(&flat, sizeof(expr_t*));

			flatten_expr(&flat, vec_get_value(&module->expr_list, i, expr_t*));

			for (int i = 0; i < flat.length; ++i)
			{
				script_value_t* exp_val = new_value(script, VAL_NATIVE);

				exp_val->nat.value = vec_get_value(&flat, i, expr_t*);
				exp_val->nat.on_mark = exp_val->nat.on_delete = NULL;

				vec_push_back(&expr_list, &exp_val);
			}

			vec_destroy(&flat);
		}
		else
		{
			// TODO: make script_create_native, et al and use those instead
			script_value_t* exp_val = new_value(script, VAL_NATIVE);

			exp_val->nat.value = vec_get_value(&module->expr_list, i, expr_t*);
			exp_val->nat.on_mark = exp_val->nat.on_delete = NULL;

			vec_push_back(&expr_list, &exp_val);
		}
	}

	script_push_premade_array(script, expr_list);
	script_return_top(script);
}

// TODO: Have line info be passed into this function
static void ext_parse_code(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("parse_code");
	
	script_module_t* module = vec_get(&script->modules, g_cur_module_index);
	script_value_t* code_val = script_get_arg(args, 0);
	
	const char* code = code_val->string.data;

	g_file = "parse_code";
	g_code = code;
	g_line = 1;

	vector_t expr_list;
	vec_init(&expr_list, sizeof(expr_t*));

	parse_program(script, &expr_list);

	vector_t expr_nat_list;
	vec_init(&expr_nat_list, sizeof(script_value_t*));

	for (int i = 0; i < expr_list.length; ++i)
	{
		expr_t* exp = vec_get_value(&expr_list, i, expr_t*);

		script_value_t* val = new_value(script, VAL_NATIVE);
		
		val->nat.on_mark = NULL;
		val->nat.on_delete = NULL;
		
		val->nat.value = exp;

		vec_push_back(&expr_nat_list, &val);
	}

	vec_destroy(&expr_list);

	script_push_premade_array(script, expr_nat_list);
	script_return_top(script);
}

static void ext_get_expr_kind(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("get_expr_kind");
	
	script_value_t* exp_val = script_get_arg(args, 0);
	expr_t* exp = exp_val->nat.value;
	
	script_push_number(script, exp->type);
	script_return_top(script);
}

static void ext_make_num_expr(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("make_num_expr");
	
	script_value_t* val = script_get_arg(args, 0);
	double number = val->number;
	
	expr_t* exp = create_expr(EXP_NUMBER);
	exp->number_index = register_number(script, number);
	
	script_push_native(script, exp, NULL, NULL);
	script_return_top(script);
}

static void ext_make_string_expr(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("make_num_expr");
	
	script_value_t* val = script_get_arg(args, 0);
	const char* string = val->string.data;
	
	expr_t* exp = create_expr(EXP_STRING);
	exp->string_index = register_string(script, string);
	
	script_push_native(script, exp, NULL, NULL);
	script_return_top(script);
}

static void ext_make_write_expr(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("make_write_expr");
	
	script_value_t* val = script_get_arg(args, 0);
	
	expr_t* exp = create_expr(EXP_WRITE);
	exp->write = val->nat.value;
	
	script_push_native(script, exp, NULL, NULL);
	script_return_top(script); 
}

static void ext_create_bool_type(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("create_bool_type");
	
	script_push_native(script, create_type_tag(script, TAG_BOOL), NULL, NULL);
	script_return_top(script);
}

static void ext_create_char_type(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("create_char_type");
	
	script_push_native(script, create_type_tag(script, TAG_CHAR), NULL, NULL);
	script_return_top(script);
}

static void ext_create_number_type(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("create_number_type");
	
	script_push_native(script, create_type_tag(script, TAG_NUMBER), NULL, NULL);
	script_return_top(script);
}

static void ext_create_string_type(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("create_string_type");
	
	script_push_native(script, create_type_tag(script, TAG_STRING), NULL, NULL);
	script_return_top(script);
}

static void ext_create_array_type(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("create_array_type");
	
	script_value_t* contained_val = script_get_arg(args, 0);
	type_tag_t* contained = contained_val->nat.value;
	
	type_tag_t* array = create_type_tag(script, TAG_ARRAY);
	array->contained = contained;
	
	script_push_native(script, array, NULL, NULL);
	script_return_top(script);
}

static void ext_create_function_type(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("create_function_type");
	
	script_value_t* return_type_val = script_get_arg(args, 0);
	script_value_t* arg_types_val = script_get_arg(args, 1);
	
	type_tag_t* tag = create_type_tag(script, TAG_FUNC);
	tag->func.return_type = return_type_val->nat.value;
	for(int i = 0; i < arg_types_val->array.length; ++i)
	{
		script_value_t* arg_type_val = vec_get_value(&arg_types_val->array, i, script_value_t*);
		type_tag_t* arg_tag = arg_type_val->nat.value;
		
		vec_push_back(&tag->func.arg_types, &arg_tag);
	}
	
	script_push_native(script, tag, NULL, NULL);
	script_return_top(script);
}

static void ext_create_native_type(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("create_native_type");
		
	script_push_native(script, create_type_tag(script, TAG_NATIVE), NULL, NULL);
	script_return_top(script);
}

static void ext_create_dynamic_type(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("create_dynamic_type");
	
	script_push_native(script, create_type_tag(script, TAG_DYNAMIC), NULL, NULL);
	script_return_top(script);
}

static void ext_create_void_type(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("create_void_type");
	
	script_push_native(script, create_type_tag(script, TAG_VOID), NULL, NULL);
	script_return_top(script);
}

static void ext_reference_func(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("reference_func");
	
	script_value_t* name_val = script_get_arg(args, 0);
	const char* name = name_val->string.data;
	
	func_decl_t* decl = reference_function(script, name);
	if(decl)
		script_push_native(script, decl, NULL, NULL);
	else
		script_push_null(script);
	script_return_top(script);
}

static void ext_get_func_decl_name(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("get_func_decl_name");
	
	script_value_t* decl_val = script_get_arg(args, 0);
	func_decl_t* decl = decl_val->nat.value;
	
	script_push_cstr(script, decl->name);
	script_return_top(script);
}

static void ext_declare_variable(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("declare_variable");
	
	script_value_t* name_val = script_get_arg(args, 0);
	script_value_t* type_val = script_get_arg(args, 1);
	script_value_t* func_decl_val = script_get_arg(args, 2);
	script_value_t* scope_val = script_get_arg(args, 3);
	
	const char* name = name_val->string.data;
	type_tag_t* tag = type_val->nat.value;
	func_decl_t* decl = NULL;
	int scope = 0;
	
	if(func_decl_val->type != VAL_NULL)
	{
		decl = func_decl_val->nat.value;
		scope = (int)scope_val->number;
	}

	g_cur_func = decl;
	g_scope = scope;
	
	script_push_native(script, declare_variable(script, name, tag), NULL, NULL);
	script_return_top(script);
}

static void ext_reference_variable(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("reference_variable");
	
	script_value_t* name_val = script_get_arg(args, 0);
	script_value_t* func_decl_val = script_get_arg(args, 1);
	script_value_t* scope_val = script_get_arg(args, 2);
	
	const char* name = name_val->string.data;
	func_decl_t* decl = NULL;
	int scope = 0;
	
	if(func_decl_val->type != VAL_NULL)
	{
		decl = func_decl_val->nat.value;
		scope = (int)scope_val->number;
	}

	g_cur_func = decl;
	g_scope = scope;
	
	var_decl_t* vdecl = reference_variable(script, name);
	if(!vdecl)
		error_exit_script(script, "Attempted to reference non-existent variable '%s'\n", name);
	else
		script_push_native(script, vdecl, NULL, NULL);
	script_return_top(script);
}

static void ext_make_var_expr(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("make_var_expr");
	
	script_value_t* decl_val = script_get_arg(args, 0);
	var_decl_t* decl = decl_val->nat.value;
	
	expr_t* exp = create_expr(EXP_VAR);
	
	exp->varx.decl = decl;
	exp->varx.name = estrdup(decl->name);
	
	script_push_native(script, exp, NULL, NULL);
	script_return_top(script);
}

static void ext_make_undeclared_var_expr(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("make_undeclared_var_expr");
	
	script_value_t* name_val = script_get_arg(args, 0);
	expr_t* exp = create_expr(EXP_VAR);
	
	exp->varx.decl = NULL;
	exp->varx.name = estrdup(name_val->string.data);
	
	script_push_native(script, exp, NULL, NULL);
	script_return_top(script);
}

static void ext_make_bin_expr(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("make_bin_expr");
	
	script_value_t* lhs_val = script_get_arg(args, 0);
	script_value_t* rhs_val = script_get_arg(args, 1);
	script_value_t* op_val = script_get_arg(args, 2);

	expr_t* lhs = lhs_val->nat.value;
	expr_t* rhs = rhs_val->nat.value;
	const char* op = op_val->string.data;
	
	expr_t* exp = create_expr(EXP_BINARY);
	exp->binx.lhs = lhs;
	exp->binx.rhs = rhs;
	
	if(strcmp(op, "=") == 0) exp->binx.op = TOK_ASSIGN;
	else error_exit_script(script, "Invalid operator '%s' given to 'make_bin_expr'\n", op);
	
	script_push_native(script, exp, NULL, NULL);
	script_return_top(script);
}

static void ext_make_call_expr(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("make_call_expr");
	
	script_value_t* func_val = script_get_arg(args, 0);
	script_value_t* args_val = script_get_arg(args, 1);
	
	expr_t* exp = create_expr(EXP_CALL);
	vec_init(&exp->callx.args, sizeof(expr_t*));
	exp->callx.func = func_val->nat.value;
	
	for(int i = 0; i < args_val->array.length; ++i)
	{
		script_value_t* arg_exp_val = vec_get_value(&args_val->array, i, script_value_t*);
		vec_push_back(&exp->callx.args, &arg_exp_val->nat.value);
	}
	
	script_push_native(script, exp, NULL, NULL);
	script_return_top(script);
}

static void ext_make_array_index_expr(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("make_array_index_expr");
	
	script_value_t* array_exp_val = script_get_arg(args, 0);
	script_value_t* index_exp_val = script_get_arg(args, 1);
	
	expr_t* exp = create_expr(EXP_ARRAY_INDEX);
	exp->array_index.array = array_exp_val->nat.value;
	exp->array_index.index = index_exp_val->nat.value;
	
	script_push_native(script, exp, NULL, NULL);
	script_return_top(script);
}

static void ext_get_func_expr_decl(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("get_func_expr_decl");
	
	script_value_t* exp_val = script_get_arg(args, 0);
	expr_t* exp = exp_val->nat.value;
	
	if(exp->type != EXP_FUNC) error_exit_script(script, "Passed non-func expression into 'get_func_expr_decl'\n");
	
	script_push_native(script, exp->funcx.decl, NULL, NULL);
	script_return_top(script);
}

static void ext_add_expr_to_module(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("add_expr_to_module");
	
	script_value_t* mod_val = script_get_arg(args, 0);
	script_value_t* exp_val = script_get_arg(args, 1);
	
	int module_index = (int)mod_val->number;
	expr_t* exp = exp_val->nat.value;
	
	script_module_t* module = vec_get(&script->modules, module_index);
	vec_push_back(&module->expr_list, &exp);
}

static void ext_insert_expr_into_module(script_t* script, vector_t* args)
{
	EXT_CHECK_IF_CT("insert_expr_into_module");
	
	script_value_t* mod_val = script_get_arg(args, 0);
	script_value_t* exp_val = script_get_arg(args, 1);
	script_value_t* loc_val = script_get_arg(args, 2);
	
	int module_index = (int)mod_val->number;
	expr_t* exp = exp_val->nat.value;
	int location = (int)loc_val->number;
	
	script_module_t* module = vec_get(&script->modules, module_index);
	vec_insert(&module->expr_list, &exp, location);
}

static void ext_make_array_of_length(script_t* script, vector_t* args)
{
	script_value_t* val = script_get_arg(args, 0);
	int length = (int)val->number;
	
	vector_t array;
	vec_init(&array, sizeof(script_value_t*));
	script_value_t* null_value = NULL;
	vec_resize(&array, length, &null_value);
	
	script_push_premade_array(script, array);
	script_return_top(script);
}

static void ext_array_push(script_t* script, vector_t* args)
{
	script_value_t* array_val = script_get_arg(args, 0);
	script_value_t* value_val = script_get_arg(args, 1);

	vector_t* array = &array_val->array;
	
	vec_push_back(array, &value_val);
}

static void push_value(script_t* script, script_value_t* value);

static void ext_array_pop(script_t* script, vector_t* args)
{
	script_value_t* array_val = script_get_arg(args, 0);
	script_value_t* value;

	vector_t* array = &array_val->array;

	vec_pop_back(array, &value);

	push_value(script, value);
	script_return_top(script);
}

static void ext_char_to_number(script_t* script, vector_t* args)
{
	script_value_t* val = script_get_arg(args, 0);
	script_push_number(script, val->code);
	script_return_top(script);
}

static void ext_number_to_char(script_t* script, vector_t* args)
{
	script_value_t* val = vec_get_value(args, 0, script_value_t*);
	script_push_char(script, (char)val->number);
	script_return_top(script);
}

static void ext_number_to_string(script_t* script, vector_t* args)
{
	script_value_t* val = script_get_arg(args, 0);
	
	static char buf[256]; // TODO: don't use a magic number here
	sprintf(buf, "%g", val->number);
	
	script_push_cstr(script, buf);
	script_return_top(script);
}

static void ext_string_to_number(script_t* script, vector_t* args)
{
	script_value_t* val = script_get_arg(args, 0);
	script_push_number(script, strtod(val->string.data, NULL));
	script_return_top(script);
}

static void ext_read_char(script_t* script, vector_t* args)
{
	script_push_char(script, getc(stdin));
	script_return_top(script);
}

static void ext_print_char(script_t* script, vector_t* args)
{
	script_value_t* val = script_get_arg(args, 0);
	putchar(val->code);
}

static void ext_floor(script_t* script, vector_t* args)
{
	script_value_t* val = script_get_arg(args, 0);
	script_push_number(script, (long)(val->number));
	script_return_top(script);
}

static void ext_ceil(script_t* script, vector_t* args)
{
	script_value_t* val = script_get_arg(args, 0);
	script_push_number(script, (long)(val->number) + 1);
	script_return_top(script);
}

static void delete_u8_buffer(void* pbuf)
{
	vector_t* buf = pbuf;
	vec_destroy(buf);
	free(buf);
}

static void ext_create_u8_buffer(script_t* script, vector_t* args)
{
	vector_t* buf = emalloc(sizeof(vector_t));
	vec_init(buf, sizeof(uint8_t));
	script_push_native(script, buf, NULL, delete_u8_buffer);
	script_return_top(script); 
}

static void ext_u8_buffer_clear(script_t* script, vector_t* args)
{
	script_native_t* nat = &script_get_arg(args, 0)->nat;
	vec_clear(nat->value);
}

static void ext_u8_buffer_length(script_t* script, vector_t* args)
{
	script_native_t* nat = &script_get_arg(args, 0)->nat;
	script_push_number(script, ((vector_t*)nat->value)->length);
	script_return_top(script);
}

static void ext_u8_buffer_push(script_t* script, vector_t* args)
{
	script_native_t* nat = &script_get_arg(args, 0)->nat;
	script_value_t* val = script_get_arg(args, 1);
	
	uint8_t value = (uint8_t)val->number;
	
	vec_push_back(nat->value, &value);
}

static void ext_u8_buffer_pop(script_t* script, vector_t* args)
{
	script_native_t* nat = &script_get_arg(args, 0)->nat;
	uint8_t value;
	vec_pop_back(nat->value, &value);
	
	script_push_number(script, value);
	script_return_top(script);
}

static void ext_u8_buffer_to_string(script_t* script, vector_t* args)
{
	script_native_t* nat = &script_get_arg(args, 0)->nat;
	vector_t* buf = nat->value;
	unsigned char null_terminator = '\0';
	vec_push_back(buf, &null_terminator);
	script_string_t str = { buf->length, (char*)buf->data };
	
	script_push_string(script, str);
	script_return_top(script);
}

// END OF DEFAULT EXTERNS

static void bind_default_externs(script_t* script)
{
	script_bind_extern(script, "add_module", ext_add_module);
	script_bind_extern(script, "load_module", ext_load_module);
	script_bind_extern(script, "compile_module", ext_compile_module);
	script_bind_extern(script, "run_module", ext_run_module);
	
	script_bind_extern(script, "get_current_module_index", ext_get_current_module_index);
	script_bind_extern(script, "get_module_source_code", ext_get_module_source_code);
	script_bind_extern(script, "get_module_expr_list", ext_get_module_expr_list);
	
	script_bind_extern(script, "get_expr_kind", ext_get_expr_kind);
	
	script_bind_extern(script, "create_bool_type", ext_create_bool_type);
	script_bind_extern(script, "create_char_type", ext_create_char_type);
	script_bind_extern(script, "create_number_type", ext_create_number_type);
	script_bind_extern(script, "create_string_type", ext_create_string_type);
	script_bind_extern(script, "create_function_type", ext_create_function_type);
	script_bind_extern(script, "create_array_type", ext_create_array_type);
	script_bind_extern(script, "create_native_type", ext_create_native_type);
	script_bind_extern(script, "create_dynamic_type", ext_create_dynamic_type);
	script_bind_extern(script, "create_void_type", ext_create_void_type);
	
	script_bind_extern(script, "reference_function", ext_reference_func);
	script_bind_extern(script, "get_func_decl_name", ext_get_func_decl_name);
	script_bind_extern(script, "declare_variable", ext_declare_variable);
	script_bind_extern(script, "reference_variable", ext_reference_variable);
	
	script_bind_extern(script, "parse_code", ext_parse_code);

	script_bind_extern(script, "make_num_expr", ext_make_num_expr);
	script_bind_extern(script, "make_string_expr", ext_make_string_expr);
	script_bind_extern(script, "make_var_expr", ext_make_var_expr);
	script_bind_extern(script, "make_undeclared_var_expr", ext_make_undeclared_var_expr);
	script_bind_extern(script, "make_write_expr", ext_make_write_expr);
	script_bind_extern(script, "make_bin_expr", ext_make_bin_expr);
	script_bind_extern(script, "make_call_expr", ext_make_call_expr);
	script_bind_extern(script, "make_array_index_expr", ext_make_array_index_expr);
	
	script_bind_extern(script, "get_func_expr_decl", ext_get_func_expr_decl);
	
	script_bind_extern(script, "add_expr_to_module", ext_add_expr_to_module);
	script_bind_extern(script, "insert_expr_into_module", ext_insert_expr_into_module);
	
	script_bind_extern(script, "make_array_of_length", ext_make_array_of_length);
	script_bind_extern(script, "array_push", ext_array_push);
	script_bind_extern(script, "array_pop", ext_array_pop);
	
	script_bind_extern(script, "char_to_number", ext_char_to_number);
	script_bind_extern(script, "number_to_char", ext_number_to_char);
	script_bind_extern(script, "number_to_string", ext_number_to_string);
	script_bind_extern(script, "string_to_number", ext_string_to_number);
	
	script_bind_extern(script, "read_char", ext_read_char);
	script_bind_extern(script, "print_char", ext_print_char);
	
	script_bind_extern(script, "floor", ext_floor);
	script_bind_extern(script, "ceil", ext_ceil);
	
	script_bind_extern(script, "create_u8_buffer", ext_create_u8_buffer);
	script_bind_extern(script, "u8_buffer_clear", ext_u8_buffer_clear);
	script_bind_extern(script, "u8_buffer_length", ext_u8_buffer_length);
	script_bind_extern(script, "u8_buffer_push", ext_u8_buffer_push);
	script_bind_extern(script, "u8_buffer_pop", ext_u8_buffer_pop);
	script_bind_extern(script, "u8_buffer_to_string", ext_u8_buffer_to_string);
}

static void add_heap_block(script_t* script)
{
	script_heap_block_t* block = emalloc(sizeof(script_heap_block_t));
	
	block->next = script->heap_head;
	block->num_free = SCRIPT_HEAP_BLOCK_SIZE;
	for (int i = 0; i < block->num_free; ++i)
	{
		block->free_indices[i] = block->num_free - i - 1;

		block->values[i].block = block;
		block->values[i].block_index = i;
	}

	script->heap_head = block;
}

void script_init(script_t* script)
{
	g_line = 1;
	g_file = "none";

	script->atomic_depth = 0;
	
	script->userdata = NULL;
	script->in_extern = 0;
	
	script->cur_file = "unknown";
	script->cur_line = 0;
	
	script->heap_head = NULL;
	add_heap_block(script);

	script->gc_head = NULL;
	script->ret_val = NULL;
	
	script->num_objects = 0;
	script->max_objects_until_gc = INIT_GC_THRESH;
	script->pc = -1;
	script->fp = 0;
	
	script->indir_depth = 0;

	vec_init(&script->call_records, sizeof(script_call_record_t));

	vec_init(&script->globals, sizeof(script_value_t*));
	
	vec_init(&script->stack, sizeof(script_value_t*));
	vec_reserve(&script->stack, STACK_SIZE);

	vec_init(&script->indir, sizeof(int));
	
	vec_init(&script->code, sizeof(word));
	
	vec_init(&script->numbers, sizeof(double));
	vec_init(&script->strings, sizeof(script_string_t));

	vec_init(&script->extern_names, sizeof(char*));
	vec_init(&script->externs, sizeof(script_extern_t));
	
	vec_init(&script->function_names, sizeof(char*));
	vec_init(&script->function_pcs, sizeof(int));

	vec_init(&script->modules, sizeof(script_module_t));

	bind_default_externs(script);
}

#if 0
// NOTE:
// This doesn't work because I don't know how to relocate pointers after 
// resizing the memory pool
// eventually, I'd like to put in some smart-handle system to do that sort
// of thing

	static void* heap_alloc(script_t* script, size_t size)
	{
		// NOTE: size of allocation is stored behind
		// the used memory; the size of the memory allocated
		// is also aligned to 4-byte boundaries
		size_t aligned_size = (size + 3) & ~0x03;
		size_t total_size = sizeof(size_t) + aligned_size;
		
		if(script->heap_used + total_size >= script->heap_max) return NULL;
		
		unsigned char* ptr = &script->heap[script->heap_used];
		*(size_t*)ptr = aligned_size;
		ptr += sizeof(size_t);
		
		script->heap_used += total_size;
		script->heap += total_size;
		
		return ptr;
	}

	static void heap_free(script_t* script, void* ptr)
	{
		unsigned char* cp = ptr;
		
		cp -= sizeof(size_t);
		
		size_t alloc_size = *(size_t*)cp;
		size_t off = (size_t)(cp - (script->heap - script->heap_used));
		size_t total_size = sizeof(size_t) + alloc_size;
		
		memmove(&script->heap[off], &script->heap[off + total_size], (script->heap_used - off - total_size));
		
		script->heap_used -= total_size;
	}
#endif

static void delete_value(script_t* script, script_value_t* val)
{	
	switch(val->type)
	{
		case VAL_NULL:
		case VAL_BOOL:
		case VAL_CHAR:
		case VAL_NUMBER:
		case VAL_FUNC: break;
		
		case VAL_STRING: free(val->string.data); val->string.data = NULL; break;
		case VAL_ARRAY: vec_destroy(&val->array); break;
		case VAL_STRUCT_INSTANCE: vec_destroy(&val->ds.members); break;
		case VAL_NATIVE: if(val->nat.on_delete) val->nat.on_delete(val->nat.value); break;
	}
	
	val->block->free_indices[val->block->num_free++] = val->block_index;
}

static void destroy_all_values(script_t* script)
{
	script_heap_block_t* block = script->heap_head;
	while (block)
	{
		script_heap_block_t* next = block->next;
		free(block);
		block = next;
	}

	script->gc_head = NULL;
}

static void destroy_module(void* p_module)
{
	script_module_t* module = p_module;

	vec_destroy(&module->referenced_modules);

	free(module->local_path);
	free(module->name);
	free(module->source_code);

	for(int i = 0; i < module->expr_list.length; ++i)
		delete_expr(vec_get_value(&module->expr_list, i, expr_t*));
	vec_destroy(&module->expr_list);

	vec_destroy(&module->compile_time_blocks);

	vec_destroy(&module->globals);
	vec_destroy(&module->functions);

	vec_traverse(&module->all_type_tags, destroy_type_tag);

	vec_destroy(&module->all_type_tags);
	map_destroy(&module->user_type_tags);
}

void script_reset(script_t* script)
{	
	vec_traverse(&script->modules, destroy_module);
	vec_clear(&script->modules);
	
	destroy_all_values(script);
	
	script->heap_head = NULL;
	add_heap_block(script);

	script->in_extern = 0;
	
	script->num_objects = 0;
	script->max_objects_until_gc = INIT_GC_THRESH;
	
	script->pc = -1;
	script->fp = 0;

	script->gc_head = NULL;
	script->ret_val = NULL;

	script->indir_depth = 0;

	vec_clear(&script->globals);
	
	vec_clear(&script->stack);
	vec_clear(&script->indir);
	
	vec_clear(&script->code);
	
	vec_clear(&script->function_names);
	vec_clear(&script->function_pcs);
}

static int read_int(script_t* script)
{
	int value = 0;
	word* vp = (word*)(&value);
	
	for(int i = 0; i < sizeof(int) / sizeof(word); ++i)
		*vp++ = vec_get_value(&script->code, script->pc++, word);
	
	return value;
}

static void mark(script_value_t* value)
{
	if(!value) return;
	if(value->marked) return;
	
	value->marked = 1;
	
	if(value->type == VAL_ARRAY)
	{
		for(int i = 0; i < value->array.length; ++i)
		{	
			script_value_t* v = vec_get_value(&value->array, i, script_value_t*); 
			mark(v);
		}
	}
	else if(value->type == VAL_STRUCT_INSTANCE)
	{
		for(int i = 0; i < value->ds.members.length; ++i)
		{
			script_value_t* v = vec_get_value(&value->ds.members, i, script_value_t*); 
			mark(v);
		}
	}
	else if(value->type == VAL_NATIVE)
	{
		if(value->nat.on_mark)
			value->nat.on_mark(value->nat.value);
	}
}

static void mark_all(script_t* script)
{
	if(script->ret_val)
		mark(script->ret_val);
	for(int i = 0; i < script->stack.length; ++i)
	{
		script_value_t* val = vec_get_value(&script->stack, i, script_value_t*);
		mark(val);
	}
	
	for(int i = 0; i < script->globals.length; ++i)
	{
		script_value_t* val = vec_get_value(&script->globals, i, script_value_t*);
		mark(val);
	}
}

static void sweep(script_t* script)
{
	script_value_t** val = &script->gc_head;
	
	while(*val)
	{
		if(!(*val)->marked)
		{
			script_value_t* unreached = *val;
			*val = unreached->next;
			--script->num_objects;
			delete_value(script, unreached);
		}
		else
		{
			(*val)->marked = 0;
			val = &(*val)->next;
		}
	}
}

static void collect_garbage(script_t* script)
{
	mark_all(script);
	sweep(script);
	script->max_objects_until_gc = script->num_objects * 2;
}

static script_value_t* get_heap_value(script_t* script)
{
	script_heap_block_t* block = script->heap_head;
	while(block)
	{
		if (block->num_free > 0)
		{
			int index = block->free_indices[--block->num_free];
			return &block->values[index];
		}

		block = block->next;
	}

	add_heap_block(script);
	return get_heap_value(script);
}

static script_value_t* new_value(script_t* script, script_value_type_t type)
{
	if(!script->in_extern && script->num_objects >= script->max_objects_until_gc) collect_garbage(script); 
	
	script_value_t* val = get_heap_value(script);
	
	val->marked = 0;
	val->type = type;
	
	val->next = script->gc_head;
	script->gc_head = val;
	
	++script->num_objects;
	
	return val;
}

static void push_value(script_t* script, script_value_t* val)
{
	if(script->stack.length + 1 >= script->stack.capacity) error_exit_script(script, "Stack overflow!\n");
	vec_push_back(&script->stack, &val);
}

static script_value_t* pop_value(script_t* script)
{
	if(script->stack.length == 0) error_exit_script(script, "Stack underflow\n");
	
	script_value_t* val;
	vec_pop_back(&script->stack, &val);
	
	return val;
}

void script_set_arg(script_t* script, int index, int nargs)
{
	if (script->fp == 0 || script->pc < 0) return;
	script_value_t* val = pop_value(script);
	vec_set(&script->stack, script->fp + (index - nargs), &val);
}

void script_push_bool(script_t* script, char bv)
{
	// NOTE: Singleton values
	// which never get gc'd
	static script_value_t true_val = { 0 };
	static script_value_t false_val = { 0 };

	true_val.type = VAL_BOOL;
	true_val.boolean = 1;
	
	false_val.type = VAL_BOOL;
	false_val.boolean = 0;

	push_value(script, bv ? &true_val : &false_val);
}

char script_pop_bool(script_t* script)
{	
	script_value_t* val = pop_value(script);
	if(val->type != VAL_BOOL)
		error_exit_script(script, "Expected bool but received %s\n", g_value_types[val->type]);

	return val->boolean;
}

void script_push_char(script_t* script, char code)
{
	script_value_t* val = new_value(script, VAL_CHAR);
	val->code = code;
	push_value(script, val);
}

char script_pop_char(script_t* script)
{
	script_value_t* val = pop_value(script);
	if(val->type != VAL_CHAR)
		error_exit_script(script, "Expected char but received %s\n", g_value_types[val->type]);

	return val->code;
}

void script_push_number(script_t* script, double number)
{
	script_value_t* val = new_value(script, VAL_NUMBER);
	val->number = number;
	push_value(script, val);
}

double script_pop_number(script_t* script)
{
	script_value_t* val = pop_value(script);
	if(val->type != VAL_NUMBER)
		error_exit_script(script, "Expected number but received %s\n", g_value_types[val->type]);
	
	return val->number;
}

void script_push_cstr(script_t* script, const char* string)
{
	script_value_t* val = new_value(script, VAL_STRING);
	val->string.length = strlen(string);
	val->string.data = emalloc(val->string.length + 1);
	strcpy(val->string.data, string);
	
	push_value(script, val);
}

void script_push_string(script_t* script, script_string_t string)
{
	script_value_t* val = new_value(script, VAL_STRING);
	val->string.length = string.length;
	val->string.data = emalloc(string.length + 1);
	strcpy(val->string.data, string.data);
	
	push_value(script, val);
}

script_string_t script_pop_string(script_t* script)
{
	script_value_t* val = pop_value(script);
	if (val->type != VAL_STRING)
		error_exit_script(script, "Expected string but received %s\n", g_value_types[val->type]);
	return val->string;
}

void script_push_array(script_t* script, size_t length)
{
	script_value_t* val = new_value(script, VAL_ARRAY);
	vec_init(&val->array, sizeof(script_value_t*));
	vec_reserve(&val->array, length);
	push_value(script, val);
}

void script_push_premade_array(script_t* script, vector_t array)
{
	script_value_t* val = new_value(script, VAL_ARRAY);
	val->array = array;
	push_value(script, val);
}

vector_t* script_pop_array(script_t* script)
{
	script_value_t* val = pop_value(script);
	if(val->type != VAL_ARRAY) error_exit_script(script, "Expected array but received %s\n", g_value_types[val->type]);
	return &val->array;
}

static void push_func(script_t* script, char is_extern, int index)
{
	script_value_t* val = new_value(script, VAL_FUNC);
	val->function.is_extern = is_extern;
	val->function.index = index;
	push_value(script, val);
}

static script_function_t pop_func(script_t* script)
{
	script_value_t* val = pop_value(script);
	if(val->type != VAL_FUNC) error_exit_script(script, "Expected function but received %s\n", g_value_types[val->type]);
	return val->function;
}


void script_push_native(script_t* script, void* value, script_native_callback_t on_mark, script_native_callback_t on_delete)
{
	script_value_t* val = new_value(script, VAL_NATIVE);
	val->nat.value = value;
	val->nat.on_mark = on_mark;
	val->nat.on_delete = on_delete;
	push_value(script, val);
}

script_native_t* script_pop_native(script_t* script)
{
	script_value_t* val = pop_value(script);
	if(val->type != VAL_NATIVE) error_exit_script(script, "Expected native but received %s\n", g_value_types[val->type]);
	return &val->nat;
}

script_value_t* script_get_arg(vector_t* args, int index)
{
	return vec_get_value(args, index, script_value_t*);
}

static char is_value_null(script_value_t* val)
{
	return val->type == VAL_NULL;
}

void script_push_null(script_t* script)
{
	static script_value_t null_value;
	null_value.type = VAL_NULL;
	push_value(script, &null_value);
}

void script_return_top(script_t* script)
{
	script->ret_val = pop_value(script);
}

static void push_struct(script_t* script, vector_t members)
{
	script_value_t* val = new_value(script, VAL_STRUCT_INSTANCE);
	val->ds.members = members;
	push_value(script, val);
}

static script_struct_t* pop_struct(script_t* script)
{
	script_value_t* val = pop_value(script);
	if(val->type != VAL_STRUCT_INSTANCE) error_exit_script(script, "Expected struct but received %s\n", g_value_types[val->type]);
	return &val->ds;
}

// TODO: Check for overflows/underflows here (esp. the s
static void push_stack_frame(script_t* script, word nargs)
{
	int i_nargs = (int)nargs;

	vec_push_back(&script->indir, &i_nargs);
	vec_push_back(&script->indir, &script->fp);
	vec_push_back(&script->indir, &script->pc);
	
	script->fp = script->stack.length;
	++script->indir_depth;
}

static void push_call_record(script_t* script, script_function_t function, word nargs)
{
	script_call_record_t record;

	record.stack_size = script->stack.length;
	record.pc = script->pc - 1;
	record.fp = script->fp;
	record.nargs = nargs;
	record.function = function;
	record.file = script->cur_file;
	record.line = script->cur_line;

	vec_push_back(&script->call_records, &record);
}

static void pop_call_record(script_t* script)
{
	script_call_record_t record;

	if(script->call_records.length > 0)
		vec_pop_back(&script->call_records, &record);
}

static void pop_stack_frame(script_t* script)
{
	if (script->indir_depth <= 0)
	{
		script->pc = -1;
		vec_clear(&script->stack);
		return;
	}
	
	// NOTE: Remove local values
	vec_resize(&script->stack, script->fp, NULL);

	// NOTE: Reset pc and fp
	vec_pop_back(&script->indir, &script->pc);
	vec_pop_back(&script->indir, &script->fp);
	
	int nargs;
	vec_pop_back(&script->indir, &nargs);

	// NOTE: Remove arguments
	vec_resize(&script->stack, script->stack.length - nargs, NULL);

	--script->indir_depth;
}

static int read_int_at(script_t* script, int pc)
{
	int value = 0;
	word* vp = (word*)(&value);
	
	for(int i = 0; i < sizeof(int) / sizeof(word); ++i)
		*vp++ = vec_get_value(&script->code, pc + i, word);
		
	return value;
}

static void disassemble(script_t* script, FILE* out)
{
	int pc = 0;
	
	const char* file = "unknown";
	int line = 0;
	
	while(pc < script->code.length)
	{
		word code = vec_get_value(&script->code, pc, word);
		++pc;
		
		if(code != OP_FILE && code != OP_LINE)
			fprintf(out, "%d (%s:%i): ", pc, file, line);

		switch(code)
		{
			case OP_PUSH_NULL: fprintf(out, "push_null\n"); break;
			
			case OP_PUSH_TRUE: fprintf(out, "push_true\n"); break;
			case OP_PUSH_FALSE: fprintf(out, "push_false\n"); break;
			
			case OP_PUSH_CHAR:
			{
				int c = read_int_at(script, pc);
				fprintf(out, "push_char '%c'\n", c);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_PUSH_NUMBER:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "push_number %g\n", vec_get_value(&script->numbers, index, double));
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_PUSH_STRING:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "push_string '%s'\n", vec_get_value(&script->strings, index, script_string_t).data);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_PUSH_FUNC:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "push_func %d (pc = %d)\n", index, vec_get_value(&script->function_pcs, index, int));
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_PUSH_EXTERN_FUNC:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "push_extern_func %s (id=%d)\n", vec_get_value(&script->extern_names, index, char*), index);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_PUSH_ARRAY:
			{
				fprintf(out, "push_array\n");
			} break;
			
			case OP_PUSH_ARRAY_BLOCK:
			{
				int length = read_int_at(script, pc);
				fprintf(out, "push_array_block %d\n", length);
				pc += sizeof(int) / sizeof(word);
			} break;
		
			case OP_PUSH_RETVAL:
			{
				fprintf(out, "push_retval\n");
			} break;

			case OP_PUSH_STRUCT:
			{
				int nmem = read_int_at(script, pc);
				pc += sizeof(int) / sizeof(word);
				
				int ninit = read_int_at(script, pc);
				pc += sizeof(int) / sizeof(word);
				
				fprintf(out, "push_struct num_members=%d num_init=%d\n", nmem, ninit);
			} break;
			
			case OP_STRING_LEN:
			{
				fprintf(out, "string_len\n");
			} break;
			
			case OP_ARRAY_LEN:
			{
				fprintf(out, "array_len\n");
			} break;
			
			case OP_STRING_GET:
			{
				fprintf(out, "string_get\n");
			} break;
			
			case OP_ARRAY_GET:
			{
				fprintf(out, "array_get\n");
			} break;
			
			case OP_ARRAY_SET:
			{
				fprintf(out, "array_set\n");
			} break;
			
			case OP_STRUCT_GET:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "struct_get %d\n", index);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_STRUCT_SET:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "struct_set %d\n", index);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_ADD: fprintf(out, "add\n"); break;
			case OP_SUB: fprintf(out, "sub\n"); break;
			case OP_MUL: fprintf(out, "mul\n"); break;
			case OP_DIV: fprintf(out, "div\n"); break;
			case OP_MOD: fprintf(out, "mod\n"); break;

			case OP_LT: fprintf(out, "lt\n"); break;
			case OP_GT: fprintf(out, "gt\n"); break;
			case OP_LTE: fprintf(out, "lte\n"); break;
			case OP_GTE: fprintf(out, "gte\n"); break;
			
			case OP_LAND: fprintf(out, "land\n"); break;
			case OP_LOR: fprintf(out, "lor\n"); break;
			
			case OP_NEG: fprintf(out, "neg\n"); break;
			case OP_NOT: fprintf(out, "not\n"); break;
			
			case OP_EQU: fprintf(out, "equ\n"); break;
			
			case OP_READ: fprintf(out, "read\n"); break;
			case OP_WRITE: fprintf(out, "write\n"); break;
		
			case OP_SET:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "set %d\n", index);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_GET:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "get %d\n", index);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_SETLOCAL:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "setlocal %d\n", index);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_GETLOCAL:
			{
				int index = read_int_at(script, pc);
				fprintf(out, "getlocal %d\n", index);
				pc += sizeof(int) / sizeof(word);
			} break;
		
			case OP_GOTO:
			{
				int g = read_int_at(script, pc);
				fprintf(out, "goto %d\n", g);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_GOTOZ:
			{
				int g = read_int_at(script, pc);
				fprintf(out, "gotoz %d\n", g);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_CALL:
			{
				word nargs = vec_get_value(&script->code, pc++, word);
				fprintf(out, "call nargs=%d\n", nargs);
			} break;
			
			case OP_RETURN:
			{
				fprintf(out, "return\n");
			} break;
			
			case OP_RETURN_VALUE:
			{
				fprintf(out, "return_value\n");
			} break;
			
			case OP_FILE:
			{
				int index = read_int_at(script, pc);
				const char* str = vec_get_value(&script->strings, index, script_string_t).data;
				pc += sizeof(int) / sizeof(word);
				
				file = str;
			} break;
			
			case OP_LINE:
			{
				int line_no = read_int_at(script, pc);
				pc += sizeof(int) / sizeof(word);
				line = line_no;
			} break;
			
			case OP_ATOMIC_ENABLE:
			{
				fprintf(out, "atomic_enable\n");
			} break;
			
			case OP_ATOMIC_DISABLE:
			{
				fprintf(out, "atomic_disable\n");
			} break;

			case OP_HALT:
			{
				fprintf(out, "halt\n");
			} break;
		}
	}
}

static char compare_values(script_value_t* a, script_value_t* b)
{
	if(a->type != b->type) return 0;
	switch(a->type)
	{
		case VAL_NATIVE: return a->nat.value == b->nat.value;
		case VAL_BOOL: return a->boolean == b->boolean;
		case VAL_CHAR: return a->code == b->code;
		case VAL_NUMBER: return a->number == b->number;
		case VAL_STRING: return strcmp(a->string.data, b->string.data) == 0;
		case VAL_NULL: return 1;
		case VAL_FUNC: return (a->function.is_extern == b->function.is_extern) && (a->function.index == b->function.index);
		case VAL_ARRAY:
		{
			int min_index = (int)(a->array.length < b->array.length ? a->array.length : b->array.length);
			for(int i = 0; i < min_index; ++i)
			{
				if(!compare_values(vec_get_value(&a->array, i, script_value_t*), vec_get_value(&b->array, i, script_value_t*)))
					return 0;
			}
			
			return 1;
		} break;
		case VAL_STRUCT_INSTANCE:
		{
			if(a->ds.members.length != b->ds.members.length) return 0;
			for(int i = 0; i < a->ds.members.length; ++i)
			{
				if(!compare_values(vec_get_value(&a->ds.members, i, script_value_t*), vec_get_value(&b->ds.members, i, script_value_t*)))
					return 0;
			}
			
			return 1;
		} break;
	}
	
	return 0;
}

static void execute_cycle(script_t* script)
{
	if(script->pc < 0) return;
	if (script->pc >= script->code.length)
	{
		script->pc = -1;
		return;
	}

	word code = vec_get_value(&script->code, script->pc, word);
	++script->pc;
	
	switch(code)
	{
		case OP_PUSH_NULL:
		{
			script_push_null(script);
		} break;
		
		case OP_PUSH_TRUE:
		{
			script_push_bool(script, 1);
		} break;
		
		case OP_PUSH_FALSE:
		{
			script_push_bool(script, 0);
		} break;
		
		case OP_PUSH_CHAR:
		{
			char c = (char)read_int(script);
			script_push_char(script, c);
		} break;
		
		case OP_PUSH_NUMBER:
		{
			int index = read_int(script);
			script_push_number(script, vec_get_value(&script->numbers, index, double));
		} break;
		
		case OP_PUSH_STRING:
		{
			int index = read_int(script);
			script_push_string(script, vec_get_value(&script->strings, index, script_string_t));
		} break;
		
		case OP_PUSH_FUNC:
		{
			int index = read_int(script);
			push_func(script, 0, index);
		} break;
		
		case OP_PUSH_EXTERN_FUNC:
		{
			int index = read_int(script);
			push_func(script, 1, index);
		} break;
	
		case OP_PUSH_ARRAY:
		{
			size_t length = (size_t)script_pop_number(script);
			script_push_array(script, length);
		} break;
		
		case OP_PUSH_ARRAY_BLOCK:
		{
			int length = read_int(script);
			vector_t array;

			vec_init(&array, sizeof(script_value_t*));
			vec_copy_region(&array, &script->stack, 0, script->stack.length - length, length);
			script->stack.length -= length;
			
			script_push_premade_array(script, array);
		} break;
				
		case OP_PUSH_RETVAL:
		{
			if(script->ret_val)
				push_value(script, script->ret_val);
			else
				script_push_null(script);
		} break;
		
		case OP_PUSH_STRUCT:
		{
			int length = read_int(script);
			int n_init = read_int(script);
			
			vector_t members;
			vec_init(&members, sizeof(script_value_t*));
			
			// initialize all members to null
			script_value_t* init_value = NULL;
			vec_resize(&members, length, &init_value);
			
			for(int i = 0; i < n_init; ++i)
			{
				script_value_t* val = pop_value(script);
				int index = (int)script_pop_number(script);
				
				vec_set(&members, index, &val);
			}
			
			// NOTE: not destroying members because 
			// the data should not be destroyed
			push_struct(script, members);
		} break;
		
		case OP_STRING_LEN:
		{
			script_string_t string = script_pop_string(script);
			script_push_number(script, string.length);
		} break;
		
		case OP_ARRAY_LEN:
		{
			vector_t* array = script_pop_array(script);
			script_push_number(script, array->length);
		} break;
		
		case OP_STRING_GET:
		{
			script_string_t string = script_pop_string(script);
			int index = (int)script_pop_number(script);
			
			if(index < 0 || index >= string.length) error_exit_script(script, "String index out of bounds\n");
			
			script_push_char(script, string.data[index]);
		} break;
		
		case OP_ARRAY_GET:
		{
			vector_t* array = script_pop_array(script);
			int index = (int)script_pop_number(script);
			
			script_value_t* val = vec_get_value(array, index, script_value_t*);
			if(!val) script_push_null(script);
			else push_value(script, val);
		} break;
		
		case OP_ARRAY_SET:
		{
			vector_t* array = script_pop_array(script);
			int index = (int)script_pop_number(script);
			script_value_t* value = pop_value(script);
			
			vec_set(array, index, &value);
		} break;
		
		case OP_STRUCT_GET:
		{
			int index = read_int(script);
			script_struct_t* s = pop_struct(script);
			
			script_value_t* val = vec_get_value(&s->members, index, script_value_t*);
			if(!val) script_push_null(script);
			else push_value(script, val);
		} break;
		
		case OP_STRUCT_SET:
		{
			int index = read_int(script);
			script_struct_t* s = pop_struct(script);
			script_value_t* val = pop_value(script);
			
			vec_set(&s->members, index, &val);
		} break;
		
		#define BOP_TYPE(name, op, type) case name: { type a = (type)script_pop_number(script), b = (type)script_pop_number(script); script_push_number(script, a op b); } break;
		#define BOP(name, op) BOP_TYPE(name, op, double)
		
		#define BOP_REL(name, op) case name: { double a = script_pop_number(script), b = script_pop_number(script); script_push_bool(script, a op b); } break;
		
		BOP(OP_ADD, +)
		BOP(OP_SUB, -)
		BOP(OP_MUL, *)
		BOP(OP_DIV, /)
		BOP_TYPE(OP_MOD, %, int)

		BOP_REL(OP_LT, <)
		BOP_REL(OP_GT, >)
		BOP_REL(OP_LTE, <=)
		BOP_REL(OP_GTE, >=)
		
		#undef BOP_TYPE
		#undef BOP
		
		case OP_LAND:
		{
			script_push_bool(script, script_pop_bool(script) && script_pop_bool(script));
		} break;
		
		case OP_LOR:
		{
			script_push_bool(script, script_pop_bool(script) || script_pop_bool(script));
		} break;
		
		case OP_NEG:
		{
			script_push_number(script, -script_pop_number(script));
		} break;
		
		case OP_NOT:
		{
			script_push_bool(script, !script_pop_bool(script));
		} break;

		// TODO: this should be specialized (OP_NUMBER_EQU, OP_STRING_EQU, OP_FUNC_EQU, OP_ARRAY_EQU)
		case OP_EQU:
		{
			script_value_t* a = pop_value(script);
			script_value_t* b = pop_value(script);
			
			script_push_bool(script, compare_values(a, b));
		} break;

		case OP_READ:
		{
			vector_t buf;
			int c = getchar();
			
			vec_init(&buf, sizeof(char));
			
			while(c != '\n')
			{
				vec_push_back(&buf, &c);
				c = getchar();
			}
			
			// NOTE: not destroying vector because that will free the memory
			// of the string
			script_string_t str = { buf.length, (char*)buf.data };
			script_push_string(script, str);
		} break;
		
		case OP_WRITE:
		{
			write_value(pop_value(script), 0);
			printf("\n");
		} break;
		
		case OP_GOTO:
		{
			int pc = read_int(script);
			script->pc = pc;
		} break;
		
		case OP_GOTOZ:
		{
			int pc = read_int(script);
			char cond = script_pop_bool(script);
			if(cond == 0)
				script->pc = pc;
		} break;
		
		case OP_SET:
		{
			int index = read_int(script);
			script_value_t* val = pop_value(script);
			vec_set(&script->globals, index, &val);
		} break;
		
		case OP_GET:
		{
			int index = read_int(script);
			push_value(script, vec_get_value(&script->globals, index, script_value_t*));
		} break;
		
		case OP_SETLOCAL:
		{
			int index = read_int(script);
			script_value_t* val = pop_value(script);
			vec_set(&script->stack, script->fp + index, &val);
		} break;
		
		case OP_GETLOCAL:
		{
			int index = read_int(script);
			script_value_t* val = vec_get_value(&script->stack, script->fp + index, script_value_t*);
			push_value(script, val);
		} break;
		
		case OP_CALL:
		{
			word nargs = vec_get_value(&script->code, script->pc++, word);
			script_function_t function = pop_func(script);
			
			if(function.is_extern)
			{
				int new_stack_length = script->stack.length - nargs;
				
				vector_t args;
				vec_init(&args, sizeof(script_value_t*));
				
				args.data = nargs > 0 ? vec_get(&script->stack, script->stack.length - nargs) : NULL;
				args.capacity = args.length = nargs;
				
				push_call_record(script, function, nargs);

				script->in_extern = 1;
				vec_get_value(&script->externs, function.index, script_extern_t)(script, &args);
				script->in_extern = 0;

				pop_call_record(script);

				script->stack.length = new_stack_length;
			}
			else
			{
				push_stack_frame(script, nargs);
				push_call_record(script, function, nargs);
				script->pc = vec_get_value(&script->function_pcs, function.index, int);
			}
		} break;
		
		case OP_RETURN:
		{
			script->ret_val = NULL;
			pop_stack_frame(script);
			pop_call_record(script);
		} break;
		
		case OP_RETURN_VALUE:
		{
			script->ret_val = pop_value(script);
			pop_stack_frame(script);
			pop_call_record(script);
		} break;

		case OP_FILE:
		{
			int index = read_int(script);
			script->cur_file = vec_get_value(&script->strings, index, script_string_t).data; 
		} break;
		
		case OP_LINE:
		{
			int line = read_int(script);
			script->cur_line = line;
		} break;

		case OP_ATOMIC_ENABLE:
		{
			++script->atomic_depth;
		} break;

		case OP_ATOMIC_DISABLE:
		{
			--script->atomic_depth;
			if (script->atomic_depth < 0)
				script->atomic_depth = 0;
		} break;

		case OP_HALT:
		{
			script->pc = -1;
		} break;
	}

	if(script->pc >= script->code.length)
		script->pc = -1;
}

void script_load_parse_file(script_t* script, const char* filename, const char* module_name)
{
	FILE* in = fopen(filename, "rb");
	
	if (!in)
		error_exit("Failed to open file '%s'\n", filename);
	
	script_parse_file(script, in, filename, module_name);

	fclose(in);
}

void script_parse_file(script_t* script, FILE* in, const char* local_path, const char* module_name)
{
	fseek(in, 0, SEEK_END);
	size_t length = ftell(in);
	fseek(in, 0, SEEK_SET);
	
	char* str = emalloc(length + 1);
	fread(str, sizeof(char), length, in);
	str[length] = '\0';
	
	rewind(in);
	
	script_parse_code(script, str, local_path, module_name);
	
	free(str);
}

void script_parse_code(script_t* script, const char* code, const char* local_path, const char* module_name)
{
	g_file = local_path;
	g_code = code;
	g_line = 1;
	
	// NOTE: adding the current module in and parsing it
	int current_module_index = add_module(script, local_path, module_name, code);

	script_module_t* module = vec_get(&script->modules, current_module_index);
	
	if (!module->parsed)
	{
		g_cur_module_index = current_module_index;
			
		parse_program(script, &module->expr_list);
		module->parsed = 1;
	}

	// NOTE: parse any unparsed modules
	for(int i = 0; i < script->modules.length; ++i)
	{
		script_module_t* module = vec_get(&script->modules, i);
		
		if(!module->parsed)
		{
			g_file = module->local_path;
			g_line = 1;
			g_code = module->source_code;
			g_cur_module_index = i;
			
			// NOTE: new modules *might* result
			// in a relocation of the module values (i.e a vec_realloc call could result in the 
			// vec.data being moved somewhere else leaving this "module" pointer invalid)
			
			// NOTE: this should not be 'destroyed' because
			// it is being copied into module->expr_list 
			vector_t expr_list;
			vec_init(&expr_list, sizeof(expr_t*));
			
			parse_program(script, &expr_list);
			
			// NOTE: the module pointer must be reset for above reason
			module = vec_get(&script->modules, i);
			module->expr_list = expr_list;
			module->parsed = 1;
		}
	}
	
	if(g_has_error) error_exit("Found errors in script code. Stopping compilation\n");
}

void script_disable_warning(script_warning_t warning, char disabled)
{
	g_warning_disabled[(int)warning] = disabled;
}

static void compile_module(script_t* script, script_module_t* module)
{
	char symbol_error = 0;
	
	if(!module->compiled)
	{
		module->start_pc = script->code.length;

		for (int pass = 0; pass <= 1; ++pass)
		{
			// NOTE: Skip first pass if there is no compile
			// time code
			if (module->compile_time_blocks.length == 0)
				pass = 1;

			check_all_tags_defined(script);
			finalize_types(script);

			// NOTE: We must compile all referenced modules so that this 
			// module can access their code
			for (int i = 0; i < module->referenced_modules.length; ++i)
			{
				script_module_t* ref_module = vec_get(&script->modules, vec_get_value(&module->referenced_modules, i, int));

				// NOTE: This will compile its referenced modules too
				compile_module(script, ref_module);
			}

			// NOTE: Pass to resolve symbols and types
			for (int expr_index = 0; expr_index < module->expr_list.length; ++expr_index)
			{
				expr_t* node = vec_get_value(&module->expr_list, expr_index, expr_t*);

				char prev_error = g_has_error;
				resolve_symbols(script, node);
				symbol_error = prev_error ? 0 : g_has_error;
				if (!symbol_error) resolve_type_tags(script, node);
			}

			if (g_has_error) error_exit("Errors in script code. Stopping...\n");

			for (int expr_index = 0; expr_index < module->expr_list.length; ++expr_index)
			{
				expr_t* node = vec_get_value(&module->expr_list, expr_index, expr_t*);
				compile_expr(script, node);
			}
			
			// NOTE: The first pass is the compile-time execution pass
			if (pass == 0)
			{
				int ct_code_start = script->code.length;

				for (int i = 0; i < module->compile_time_blocks.length; ++i)
				{
					expr_t* exp = vec_get_value(&module->compile_time_blocks, i, expr_t*);

					char prev_error = g_has_error;
					resolve_symbols(script, exp);
					symbol_error = prev_error ? 0 : g_has_error;
					if (!symbol_error) resolve_type_tags(script, exp);

					//debug_expr(script, node);
					//printf("\n");

					if (!g_has_error)
						compile_expr(script, exp);
					else
						error_exit("Found errors in compile-time code. Stopping compilation\n");
				}

				// NOTE: Must stop after all compile time code is executed
				append_code(script, OP_HALT);

				// NOTE: setup script for compile-time execution
				vec_clear(&script->stack);
				allocate_globals(script);

				printf("Executing compile-time code...\n");

				script->pc = module->start_pc;
				while (script->pc >= 0)
					execute_cycle(script);

				printf("Finished compile-time execution.\n");

				// NOTE: Reset the script code so it doesn't include the compile time code
				vec_resize(&script->code, ct_code_start, NULL);
			}
		}

		module->end_pc = script->code.length;
		module->compiled = 1;
	}
}

void script_compile(script_t* script)
{
	// NOTE: modules are compiled in reverse order
	// because that's how the dependencies work out
	// ex.
	/* in file a:
	 * #import "b"
	 * some_function_in_b()
	 * 
	 * in file b:
	 * var important_global_variable : something = something
	 * 
	 * func some_function_in_b() { do_something(important_global_variable); }
	 * 
	 * ^ that will likely crash if b code is run after a code cause the 'important_global_variable' is uninitialized
	 */ 
	// NOTE: This automatically compiles dependencies
	compile_module(script, vec_get(&script->modules, 0));
	
	if(g_has_error) error_exit("Found errors in script code. Stopping compilation\n");
}

void script_dissassemble(script_t* script, FILE* out)
{
	disassemble(script, out);
}

void script_run(script_t* script)
{
	allocate_globals(script);
	vec_clear(&script->stack);
	
	// NOTE: making sure that compile-time externs
	// cannot be called at this point
	g_cur_module_index = -1;
	
	script->pc = 0;
	while(script->pc >= 0)
		execute_cycle(script);
}

void script_start(script_t* script)
{
	allocate_globals(script);
	vec_clear(&script->stack);
	
	g_cur_module_index = -1;

	script->pc = 0;

	// NOTE: All global code needs to be run at startup
	// because otherwise globals remain uninitialized
	// etc
	while (script->pc >= 0)
		execute_cycle(script);
	
	script->pc = 0;
}

void script_execute_cycle(script_t* script)
{
	execute_cycle(script);
}

void script_stop(script_t* script)
{
	script->pc = -1;
	vec_clear(&script->stack);
}

static void read_console_input(char* buf, size_t length)
{
	fgets(buf, length, stdin);
	char* new_line = strrchr(buf, '\n');
	if(new_line)
		*new_line = '\0';
}

static void output_current_line(script_t* script)
{
	script_module_t* module = NULL;
	for(int i = 0; i < script->modules.length; ++i)
	{
		script_module_t* m = vec_get(&script->modules, i);
		if(script->pc >= m->start_pc && script->pc < m->end_pc)
		{
			module = m;
			break;
		}
	}
	
	if(module)
	{
		size_t length = strlen(module->source_code);
		int line = 1;
		int offset = -1;
		for(int i = 0; i < length; ++i)
		{
			if(line == script->cur_line)
			{
				offset = i;
				break;
			}
			if(module->source_code[i] == '\n') ++line;
		}
		
		if(offset >= 0)
		{
			printf("(%s:%d): ", script->cur_file, script->cur_line);
			char* source = &module->source_code[offset];
			while(*source != '\n')
			{
				putc(*source, stdout);
				++source;
			}
			putc('\n', stdout);
		}
		else
			fprintf(stderr, "Unable to find line %d in source code\n", script->cur_line);
	}
	else
		fprintf(stderr, "Unable to locate module for current pc %d\n", script->pc);
}

void script_debug(script_t* script, script_debug_env_t* env)
{
	vec_init(&env->breakpoints, sizeof(script_debug_breakpoint_t));
	vec_init(&env->break_stack, sizeof(int));
	
	printf("Debugging script...\nFollowing modules loaded:\n");
	for(int i = 0; i < script->modules.length; ++i)
	{
		script_module_t* module = vec_get(&script->modules, i);
		printf("%s\n", module->local_path);
	}
	
	char running = 1;
	char script_running = 0;
	
	while(running)
	{
		char input[256];

	on_break:
		if(script_running)
			output_current_line(script);
	
		read_console_input(input, 256);
		
		if(strcmp(input, "exit") == 0)
			running = 0;
		else if(strcmp(input, "run") == 0)
		{
			if(!script_running)
				printf("Running...\n");
			else
				printf("Restarting...\n");
			
			script_running = 1;
		
			allocate_globals(script);
			vec_clear(&script->stack);
			script->pc = 0;
		
			while(script->pc >= 0)
			{
				int break_index = -1;
				for(int i = 0; i < env->breakpoints.length; ++i)
				{
					script_debug_breakpoint_t* bp = vec_get(&env->breakpoints, i);
					if((bp->line == script->cur_line && strcmp(bp->file, script->cur_file) == 0) ||
						bp->pc == script->pc)
					{
						break_index = i;
						break;
					}
				}
				
				if(break_index >= 0)
				{
					script_debug_breakpoint_t* bp = vec_get(&env->breakpoints, break_index);
					printf("Hit breakpoint %d (%s:%d)\n", break_index, bp->file, bp->line);
					vec_push_back(&env->break_stack, &script->pc);
					goto on_break;
				}
				
			on_continue:
				execute_cycle(script);
			}
			
			script_running = 0;
		}
		else if(strcmp(input, "continue") == 0)
		{
			if(env->break_stack.length > 0)
			{
				int pc;
				vec_pop_back(&env->break_stack, &pc); 
				script->pc = pc;
				
				goto on_continue;
			}
			else
				fprintf(stderr, "Cannot continue; no breakpoint encountered\n");
		}
		else if(strcmp(input, "dis") == 0)
			script_dissassemble(script, stdout);
		else if(strcmp(input, "step") == 0)
		{
			if(script_running && script->pc >= 0 && script->pc < script->code.length)
			{
				int start_line = script->cur_line;
				const char* start_file = script->cur_file;
				while(script->cur_line == start_line && strcmp(script->cur_file, start_file))
					execute_cycle(script);
			}
			else
				fprintf(stderr, "Cannot step; script is not running\n");
		}
		else if(strcmp(input, "break_pc") == 0)
		{
			script_debug_breakpoint_t bp;
			bp.pc = -1;
			bp.file = NULL;
			bp.line = -1;
			
			printf("pc: ");
			read_console_input(input, 256);
		
			if(input[0])
			{
				bp.pc = strtol(input, NULL, 10);		
				printf("Setting breakpoint at pc %d\n", bp.pc);
				vec_push_back(&env->breakpoints, &bp);
			}
		}
		else if(strcmp(input, "break") == 0)
		{
			script_debug_breakpoint_t bp;
			bp.pc = -1;
			bp.file = NULL;
			bp.line = -1;
			
			printf("file: ");
			read_console_input(input, 256);
			
			if(input[0])
				bp.file = estrdup(input);
			printf("line: ");
			read_console_input(input, 256);
			
			if(input[0])
				bp.line = strtol(input, NULL, 10);
			
			printf("Setting breakpoint at %s:%i\n", bp.file, bp.line);
			vec_push_back(&env->breakpoints, &bp);
		}
	}
	
	vec_destroy(&env->breakpoints);
	vec_destroy(&env->break_stack);
}

char script_get_function_by_name(script_t* script, const char* name, script_function_t* function)
{
	for(int i = 0; i < script->function_names.length; ++i)
	{
		char* func_name = vec_get_value(&script->function_names, i, char*);
		if(strcmp(name, func_name) == 0)
		{
			function->index = i;
			function->is_extern = 0;
			return 1;
		}
	}
	
	for(int i = 0; i < script->extern_names.length; ++i)
	{
		char* extern_name = vec_get_value(&script->extern_names, i, char*);
		if(strcmp(name, extern_name) == 0)
		{
			function->index = i;
			function->is_extern = 1;
			return 1;
		}
	}
	
	return 0;
}

void script_call_function(script_t* script, script_function_t function, int nargs)
{
	int depth = script->indir_depth;
	push_stack_frame(script, (word)nargs);
	script->pc = vec_get_value(&script->function_pcs, function.index, int);
	
	while(script->indir_depth > depth && script->pc >= 0)
		execute_cycle(script);
}

void script_goto_function(script_t * script, script_function_t function, int nargs)
{
	push_stack_frame(script, (word)nargs);
	script->pc = vec_get_value(&script->function_pcs, function.index, int);
}

static void destroy_string(void* p_str)
{
	script_string_t* str = p_str;
	free(str->data);
	str->length = 0;
}

static void destroy_cstring(void* p_str)
{
	char* str = *(char**)p_str;
	free(str);
}

void script_destroy(script_t* script)
{
	vec_traverse(&script->modules, destroy_module);
	vec_destroy(&script->modules);
	
	destroy_all_values(script);
	
	vec_destroy(&script->globals);
	
	vec_destroy(&script->code);
	
	vec_destroy(&script->stack);
	vec_destroy(&script->indir);
	
	vec_destroy(&script->numbers);
	
	vec_traverse(&script->strings, destroy_string);
	vec_destroy(&script->strings);
	
	vec_traverse(&script->extern_names, destroy_cstring);
	vec_destroy(&script->extern_names);
	
	vec_destroy(&script->externs);
	
	vec_traverse(&script->function_names, destroy_cstring);
	vec_destroy(&script->function_names);
	
	vec_destroy(&script->function_pcs);
}

#ifdef __cplusplus
}
#endif
