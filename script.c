#include "script.h"
#include "hashmap.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#define MAX_LEX_CHARS 256
#define STACK_SIZE 256
#define INIT_GC_THRESH 8

typedef unsigned char word;

typedef struct
{
	int line;
	const char* file;
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
	TAG_TRAIT
} tag_t;

typedef struct type_tag
{
	context_t ctx;
	tag_t type;
	char defined;
	
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
			vector_t members;
			vector_t functions;
		} ds;
	};
} type_tag_t;

struct expr;
typedef struct
{
	char* name;
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
	TOK_DIR_IMPORT,
	
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
	
	EXP_RETURN,
	
	EXP_EXTERN,
	EXP_FUNC
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
			func_decl_t* parent;
			struct expr* value;
		} retx;
		
		func_decl_t* extern_decl;
		
		struct
		{
			func_decl_t* decl;
			struct expr* body;
		} funcx;
	};
} expr_t;

typedef struct
{
	char* name;
	func_decl_t* decl;
	expr_t* body;
} type_mem_fn_t;

static const char* g_token_names[NUM_TOKENS] = {
	"#import",
	
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
static vector_t g_globals;
static vector_t g_functions;
static int g_num_functions = 0;
static func_decl_t* g_cur_func = NULL;
static hashmap_t g_user_type_tags;
static vector_t g_all_type_tags;
static char g_has_error = 0;

// NOTE: Used for file and line debug info optimization
static int g_last_compiled_line = 0;
static const char* g_last_compiled_file = NULL;

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

static void write_value(script_value_t* val)
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
		case VAL_STRING: printf("%s", val->string.data); break;
		case VAL_FUNC: printf("func (extern: %s, index: %d)", val->function.is_extern ? "true" : "false", val->function.index); break;
		case VAL_ARRAY:
		{
			printf("[");
			for(int i = 0; i < val->array.length; ++i)
			{
				write_value(vec_get_value(&val->array, i, script_value_t*));
				if(i + 1 < val->array.length) printf(", ");
			}
			printf("]");
		} break;
		case VAL_STRUCT_INSTANCE:
		{
			printf("{ ");
			for(int i = 0; i < val->ds.members.length; ++i)
			{
				write_value(vec_get_value(&val->ds.members, i, script_value_t*));
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
	fprintf(stderr, "Stack:\n");
	for(int i = 0; i < script->stack.length; ++i)
	{
		write_value(vec_get_value(&script->stack, i, script_value_t*));
		printf("\n");
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

static void init_type_tags()
{
	map_init(&g_user_type_tags);
	vec_init(&g_all_type_tags, sizeof(type_tag_t*));
}

static type_tag_t* create_type_tag(tag_t type)
{
	type_tag_t* tag = emalloc(sizeof(type_tag_t)); 
	
	tag->defined = 1;
	
	tag->ctx.file = g_file;
	tag->ctx.line = g_line;
	
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
			vec_init(&tag->ds.members, sizeof(type_tag_member_t));
			vec_init(&tag->ds.functions, sizeof(type_mem_fn_t));
		} break;
		
		default: break;
	}
	
	vec_push_back(&g_all_type_tags, &tag);
	return tag;
}

static type_tag_t* get_type_tag_from_name(const char* name)
{
	type_tag_t* user_tag = map_get(&g_user_type_tags, name);
	if(user_tag) return user_tag;
	
	for(int i = 0; i < NUM_BUILTIN_TAGS; ++i)
	{
		if(strcmp(g_builtin_type_tags[i], name) == 0)
			return create_type_tag((tag_t)i);
	}
	
	type_tag_t* potential_tag = create_type_tag(TAG_STRUCT);
	
	potential_tag->ds.name = estrdup(name);
	vec_init(&potential_tag->ds.members, sizeof(type_tag_member_t));
	potential_tag->defined = 0;
	
	map_set(&g_user_type_tags, name, potential_tag);
	
	return potential_tag;
}

static char is_type_tag(type_tag_t* tag, tag_t type)
{
	return tag->type == type || (tag->type != TAG_VOID && tag->type == TAG_DYNAMIC);
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

static void check_all_tags_defined()
{
	for(int i = 0; i < g_all_type_tags.length; ++i)
	{
		type_tag_t* tag = vec_get_value(&g_all_type_tags, i, type_tag_t*);
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
			}
			
			for(int i = 0; i < tag->ds.functions.length; ++i)
			{
				type_mem_fn_t* mem = vec_get(&tag->ds.functions, i);
				free(mem->name);
			}
				
			vec_destroy(&tag->ds.members);
			vec_destroy(&tag->ds.functions);
		} break;
		
		default: break;
	}
}

static type_tag_t* define_struct_type(const char* name, char is_union)
{
	type_tag_t* tag = map_get(&g_user_type_tags, name);
	if(!tag)
	{
		tag = create_type_tag(TAG_STRUCT);
		tag->ds.name = estrdup(name);
		map_set(&g_user_type_tags, name, tag);
	}
	
	tag->ds.is_union = is_union;
	tag->defined = 1;
	
	return tag;
}

static void reset_type_tags()
{
	map_clear(&g_user_type_tags);
}

static void traverse_all_tags(void* vp_tag)
{
	type_tag_t* tag = *(type_tag_t**)vp_tag;
	
	destroy_type_tag(tag);
	free(tag);
}

static void destroy_type_tags()
{
	map_destroy(&g_user_type_tags);
	
	vec_traverse(&g_all_type_tags, traverse_all_tags);
	vec_destroy(&g_all_type_tags);
}

static void init_symbols()
{
	vec_init(&g_globals, sizeof(var_decl_t*));
	vec_init(&g_functions, sizeof(func_decl_t*));
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

static void destroy_symbol_data()
{
	vec_traverse(&g_globals, delete_var_decl);
	vec_traverse(&g_functions, delete_func_decl);
}

static void destroy_symbols()
{
	destroy_symbol_data();
	
	vec_destroy(&g_globals);
	vec_destroy(&g_functions); 
}

static void reset_symbols()
{
	destroy_symbol_data();
	
	vec_clear(&g_globals);
	vec_clear(&g_functions);
}

static var_decl_t* declare_variable(const char* name, type_tag_t* tag)
{
	var_decl_t* decl = emalloc(sizeof(type_tag_t));
	
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
	
	decl->index = g_globals.length;
	
	vec_push_back(&g_globals, &decl);
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
	decl->index = -(g_cur_func->args.length + 1);
	
	vec_push_back(&g_cur_func->args, &decl);
	return decl;
}

static var_decl_t* reference_variable(const char* name)
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

	for(int i = 0; i < g_globals.length; ++i)
	{
		var_decl_t* decl = vec_get_value(&g_globals, i, var_decl_t*);
		if(strcmp(decl->name, name) == 0) return decl;
	}
	
	return NULL;
}

static func_decl_t* declare_function(const char* name)
{
	func_decl_t* decl = emalloc(sizeof(func_decl_t));
	
	decl->parent = g_cur_func;
	decl->type = DECL_FUNCTION;
	decl->tag = create_type_tag(TAG_FUNC);
	decl->name = estrdup(name);
	
	vec_init(&decl->locals, sizeof(var_decl_t*));
	vec_init(&decl->args, sizeof(var_decl_t*));

	decl->index = g_num_functions++;
	decl->has_return = 0;

	vec_push_back(&g_functions, &decl);
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
	func_decl_t* decl = emalloc(sizeof(func_decl_t));
	
	decl->parent = g_cur_func;
	decl->type = DECL_EXTERN;
	decl->tag = create_type_tag(TAG_FUNC);
	decl->name = estrdup(name);
	
	vec_init(&decl->locals, sizeof(var_decl_t*));
	vec_init(&decl->args, sizeof(var_decl_t*));

	int index = get_extern_index(script, name);
	if(index < 0) error_exit_p("Attempted to declare unbound extern by name '%s'\n", name);
	decl->index = index;

	vec_push_back(&g_functions, &decl);
	return decl;
}

static func_decl_t* reference_function(const char* name)
{
	for(int i = 0; i < g_functions.length; ++i)
	{
		func_decl_t* decl = vec_get_value(&g_functions, i, func_decl_t*);
		if(strcmp(decl->name,  name) == 0)
			return decl;
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
		
		if(strcmp(g_lexeme, "#import") == 0) return TOK_DIR_IMPORT;
		
		if(strcmp(g_lexeme, "static") == 0) return TOK_STATIC;
		if(strcmp(g_lexeme, "null") == 0) return TOK_NULL;
		if(strcmp(g_lexeme, "new") == 0) return TOK_NEW;
		if(strcmp(g_lexeme, "union") == 0) return TOK_UNION;
		if(strcmp(g_lexeme, "struct") == 0) return TOK_STRUCT;
		if(strcmp(g_lexeme, "extern") == 0) return TOK_EXTERN;
		if(strcmp(g_lexeme, "true") == 0) return TOK_TRUE;
		if(strcmp(g_lexeme, "false") == 0) return TOK_FALSE;
		if(strcmp(g_lexeme, "while") == 0) return TOK_WHILE;
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
	exp->type = type;
	
	return exp;
}

static expr_t* parse_expr(script_t* script);

static type_tag_t* parse_type_tag(script_t* script)
{
	type_tag_t* tag = get_type_tag_from_name(g_lexeme);
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
	exp->varx.decl = reference_variable(g_lexeme);
	
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
			
	if(g_cur_tok != TOK_COLON) error_exit_p("Expected ':' after '%s'\n", exp->varx.name);
	get_next_token();
	
	type_tag_t* tag = parse_type_tag(script);
	exp->varx.decl = declare_variable(exp->varx.name, tag);

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
	
	++g_scope;
	while(g_cur_tok != TOK_CLOSECURLY)
	{
		expr_t* e = parse_expr(script);
		vec_push_back(&block, &e);
	}
	--g_scope;
	
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

static expr_t* parse_extern(script_t* script)
{
	expr_t* exp = create_expr(EXP_EXTERN);
	get_next_token();
	
	char* name = estrdup(g_lexeme);
	
	get_next_token();
	
	exp->extern_decl = declare_extern(script, name);
	
	if(g_cur_tok != TOK_OPENPAREN) error_exit_p("Expected '(' after extern %s\n", name);
	get_next_token();
	
	while(g_cur_tok != TOK_CLOSEPAREN)
	{
		type_tag_t* arg_tag = parse_type_tag(script);
		vec_push_back(&exp->extern_decl->tag->func.arg_types, &arg_tag);
		
		if(g_cur_tok == TOK_COMMA) get_next_token();
		else if(g_cur_tok != TOK_CLOSEPAREN) error_exit_p("Unexpected token '%s'\n", g_token_names[g_cur_tok]);
	}
	get_next_token();
	
	if(g_cur_tok != TOK_COLON) error_exit_p("Expected ':' but received '%s'\n", g_token_names[g_cur_tok]);
	get_next_token();
	
	exp->extern_decl->tag->func.return_type = parse_type_tag(script);
	
	free(name);
	
	return exp;
}

static expr_t* parse_func(script_t* script)
{
	expr_t* exp = create_expr(EXP_FUNC);
	get_next_token();
	
	char* name = estrdup(g_lexeme);
	
	get_next_token();
	
	exp->funcx.decl = declare_function(name);
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
	type_tag_t* tag = define_struct_type(g_lexeme, is_union);
	
	get_next_token();
	
	if(g_cur_tok != TOK_OPENCURLY) error_exit_p("Expected '{' after struct %s but received '%s'\n", tag->ds.name, g_token_names[g_cur_tok]);
	get_next_token();
	
	while(g_cur_tok != TOK_CLOSECURLY)
	{
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
			char buf[length];
			sprintf(buf, "%s_%s", tag->ds.name, name); 
			buf[length - 1] = '\0';
			
			func_decl_t* decl = declare_function(buf);
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
				
			if(g_cur_tok != TOK_COLON) error_exit_p("Expected ':' after ')'\n");
			get_next_token();
			decl->tag->func.return_type = parse_type_tag(script);
		
			type_mem_fn_t mem;
			mem.name = name;
			mem.decl = decl;
			mem.body = parse_expr(script);
			
			vec_push_back(&tag->ds.functions, &mem);
			
			exit_function();
		}
		else
		{
			get_next_token();
		
			type_tag_member_t member;
			member.name = name;
			member.type = parse_type_tag(script);
			member.default_value = NULL;
			
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

static void add_module(script_t* script, const char* local_path)
{
	// NOTE: This'll slow down add module a bit (but who cares, how many modules could you possibly have :)
	for(int i = 0; i < script->modules.length; ++i)
	{
		script_module_t* module = vec_get(&script->modules, i);
		if(strcmp(module->local_path, local_path) == 0)
			return;
	}
	
	script_module_t module;
	
	module.local_path = estrdup(local_path);
	module.parsed = 0;
	vec_init(&module.expr_list, sizeof(expr_t*));
	
	vec_push_back(&script->modules, &module);
}

static void apply_import_directive(script_t* script, const char* filename)
{
	add_module(script, filename);
}

static expr_t* parse_factor(script_t* script)
{
	switch(g_cur_tok)
	{
		case TOK_DIR_IMPORT:
		{
			get_next_token();
			if(g_cur_tok != TOK_STRING) error_exit_p("Expected string after '#import' but received '%s'\n", g_token_names[g_cur_tok]);
			
			apply_import_directive(script, g_lexeme);
			
			get_next_token();
			return parse_factor(script);
		} break;
	
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
		case TOK_MUL: case TOK_DIV: return 4;
	
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
				type_tag_t* tag = get_type_tag_from_name(g_lexeme);
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
				
				// NOTE: A velociraptor attacked me out of nowhere when I compiled this
				// http://xkcd.com/292/
				goto not_type;
			}
		
		not_type:
			exp = create_expr(EXP_VAR);
			exp->varx.name = estrdup("new");
			exp->varx.decl = reference_variable("new");
			
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
		
		case EXP_UNARY: printf("%s", g_token_names[exp->unaryx.op]); debug_expr(script, exp->unaryx.rhs); break;
		case EXP_BINARY: printf("("); debug_expr(script, exp->binx.lhs); printf(" %s ", g_token_names[exp->binx.op]); debug_expr(script, exp->binx.rhs); printf(")"); break;
		case EXP_WRITE: printf("write "); debug_expr(script, exp->write); break;
		
		case EXP_LEN: printf("len "); debug_expr(script, exp->len); break;
	}
}

static void delete_expr(expr_t* exp)
{
	if(!exp->is_shallow_copy)
	{
		switch(exp->type)
		{
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
			
			case EXP_STRUCT_DECL:
			{
				for(int i = 0; i < exp->struct_tag->ds.functions.length; ++i)
				{
					type_mem_fn_t* mem = vec_get(&exp->struct_tag->ds.functions, i);
					delete_expr(mem->body);
				}
			} break;
			
			case EXP_NULL:
			case EXP_EXTERN:
			case EXP_BOOL:
			case EXP_CHAR:
			case EXP_NUMBER:
			case EXP_STRING: break;
			
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

static void resolve_symbols(expr_t* exp)
{
	switch(exp->type)
	{
		case EXP_DOT: case EXP_COLON:
		{
			resolve_symbols(exp->dotx.value);
		} break;
		
		case EXP_STRUCT_NEW:
		{
			for(int i = 0; i < exp->newx.init.length; ++i)
				resolve_symbols(vec_get_value(&exp->newx.init, i, expr_t*));
		} break;
		
		case EXP_STRUCT_DECL:
		{
			for(int i = 0; i < exp->struct_tag->ds.members.length; ++i)
			{
				type_tag_member_t* mem = vec_get(&exp->struct_tag->ds.members, i);
				if(mem->default_value)
					resolve_symbols(mem->default_value);
			}
			
			for(int i = 0; i < exp->struct_tag->ds.functions.length; ++i)
			{
				type_mem_fn_t* mem = vec_get(&exp->struct_tag->ds.functions, i);
				resolve_symbols(mem->body);
			}
		} break;
		
		case EXP_NULL:
		case EXP_EXTERN:
		case EXP_NUMBER:
		case EXP_STRING:
		case EXP_CHAR:
		case EXP_BOOL: break;
	
		case EXP_VAR:
		{
			if(exp->varx.decl) return;
			exp->varx.decl = reference_variable(exp->varx.name);
			if(!exp->varx.decl) 
			{
				if(reference_function(exp->varx.name)) return;
				if(get_type_tag_from_name(exp->varx.name)) return;
				error_defer_e(exp, "Attempted to reference undeclared thing '%s'\n", exp->varx.name);
			}
		} break;
		
		case EXP_UNARY:
		{
			resolve_symbols(exp->unaryx.rhs);
		} break;
		
		case EXP_BINARY:
		{
			resolve_symbols(exp->binx.lhs);
			resolve_symbols(exp->binx.rhs);
		} break;
		
		case EXP_PAREN:
		{
			resolve_symbols(exp->paren);
		} break;
		
		case EXP_BLOCK:
		{
			for(int i = 0; i < exp->block.length; ++i)
				resolve_symbols(vec_get_value(&exp->block, i, expr_t*));
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			resolve_symbols(exp->array_index.array);
			resolve_symbols(exp->array_index.index);
		} break;
		
		case EXP_ARRAY_LITERAL:
		{
			for(int i = 0; i < exp->array_literal.values.length; ++i)
				resolve_symbols(vec_get_value(&exp->array_literal.values, i, expr_t*));
		} break;
		
		case EXP_IF:
		{
			resolve_symbols(exp->ifx.cond);
			resolve_symbols(exp->ifx.body);
			if(exp->ifx.alt) resolve_symbols(exp->ifx.alt);
		} break;
		
		case EXP_WHILE:
		{
			resolve_symbols(exp->whilex.cond);
			resolve_symbols(exp->whilex.body);
		} break;
		
		case EXP_RETURN:
		{
			if(exp->retx.value)
				resolve_symbols(exp->retx.value);
		} break;
		
		case EXP_CALL:
		{
			for(int i = 0; i < exp->callx.args.length; ++i)
				resolve_symbols(vec_get_value(&exp->callx.args, i, expr_t*));
			resolve_symbols(exp->callx.func);
		} break;
		
		case EXP_FUNC:
		{
			resolve_symbols(exp->funcx.body);
		} break;
		
		case EXP_WRITE:
		{
			resolve_symbols(exp->write);
		} break;
		
		case EXP_LEN:
		{
			resolve_symbols(exp->len);
		} break;
	}
}

static char are_assignment_types_valid(expr_t* lhs, expr_t* rhs)
{
	return compare_type_tags(lhs->tag, rhs->tag);
	/*
	switch(lhs->type)
	{
		case EXP_VAR:
		{
			return compare_type_tags(lhs->tag, rhs->tag);
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			return compare_type_tags(lhs->tag->contained, rhs->tag);
		} break;
		
		default: break;
	}
	
	return 1;*/
}

// TODO: Add type names to error messages
// NOTE: resolve_symbols before this
static void resolve_type_tags(void* vexp)
{
	expr_t* exp = vexp;
	
	switch(exp->type)
	{
		case EXP_DOT: case EXP_COLON:
		{
			resolve_type_tags(exp->dotx.value);
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
			
			for(int i = 0; i < tag->ds.functions.length; ++i)
			{
				type_mem_fn_t* mem = vec_get(&tag->ds.functions, i);
				if(strcmp(mem->name, exp->dotx.name) == 0)
				{
					exp->tag = mem->decl->tag;
					found = 1;
					
					if(exp->type == EXP_DOT) error_defer_e(exp, "Attempted to use '.' operator to access member function; use ':' instead\n");
					break;
				}
			}
			
			if(!found)
			{
				error_defer_e(exp, "Attempted to access non-existent member '%s' in struct %s\n", exp->dotx.name, tag->ds.name);
				exp->tag = create_type_tag(TAG_VOID); // prevent crashes
			}
		} break;
		
		case EXP_NULL:
		{
			exp->tag = create_type_tag(TAG_DYNAMIC);
		} break;
		
		case EXP_STRUCT_DECL:
		{
			exp->tag = create_type_tag(TAG_VOID);
			
			for(int i = 0; i < exp->struct_tag->ds.members.length; ++i)
			{
				type_tag_member_t* mem = vec_get(&exp->struct_tag->ds.members, i);
				if(mem->default_value)
				{	
					resolve_type_tags(mem->default_value);
					if(!compare_type_tags(mem->type, mem->default_value->tag))
						error_defer_e(mem->default_value, "Type of default value does not match type of member variable\n");
				}
			}
			
			for(int i = 0; i < exp->struct_tag->ds.functions.length; ++i)
			{
				type_mem_fn_t* mem = vec_get(&exp->struct_tag->ds.functions, i);
				resolve_type_tags(mem->body);
			}
		} break;
		
		case EXP_STRUCT_NEW:
		{
			exp->tag = exp->newx.type;
		} break;
			
		case EXP_NUMBER:
		{
			exp->tag = create_type_tag(TAG_NUMBER);
		} break;
		
		case EXP_STRING:
		{
			exp->tag = create_type_tag(TAG_STRING);
		} break;
		
		case EXP_BOOL:
		{
			exp->tag = create_type_tag(TAG_BOOL);
		} break;
		
		case EXP_CHAR:
		{
			exp->tag = create_type_tag(TAG_CHAR);
		} break;
		
		case EXP_VAR:
		{
			if(!exp->varx.decl)
			{
				func_decl_t* decl = reference_function(exp->varx.name);
				if(decl) exp->tag = decl->tag;
				else error_exit_e(vexp, "Referencing undeclared function/variable %s\n", exp->varx.name); 
			}
			else
				exp->tag = exp->varx.decl->tag;
		} break;
		
		case EXP_WRITE:
		{
			exp->tag = create_type_tag(TAG_VOID);
			resolve_type_tags(exp->write);
		} break;
		
		case EXP_UNARY:
		{
			resolve_type_tags(exp->unaryx.rhs);
			switch(exp->unaryx.op)
			{
				case TOK_NOT:
				{
					if(exp->unaryx.rhs->tag->type != TAG_BOOL)
						error_defer_e(exp->unaryx.rhs, "Attempted to use unary ! operator on non-boolean value\n");
					exp->tag = create_type_tag(TAG_BOOL);
				} break;
				
				case TOK_MINUS:
				{
					if(exp->unaryx.rhs->tag->type != TAG_NUMBER)
						error_defer_e(exp->unaryx.rhs, "Attempted to use unary - operator on non-numerical value\n");
					exp->tag = create_type_tag(TAG_NUMBER);
				} break;
				
				default:
					error_exit_e(exp, "No type resolution for %s operator\n", g_token_names[exp->unaryx.op]);
					break;
			}
		} break;
		
		case EXP_BINARY:
		{
			resolve_type_tags(exp->binx.lhs);
			resolve_type_tags(exp->binx.rhs);
		
			// NOTE: assignment operation lhs does not need to be
			// number
			if(exp->binx.op != TOK_ASSIGN)
			{ 
				if(!compare_type_tags(exp->binx.lhs->tag, exp->binx.rhs->tag) || exp->binx.lhs->tag->type != TAG_NUMBER)
				{	
					if(exp->binx.op != TOK_EQUALS && exp->binx.op != TOK_NOTEQUAL && 
						exp->binx.op != TOK_LAND && exp->binx.op != TOK_LOR)
						error_defer_e(exp->binx.lhs, "Invalid types in binary %s operation\n", g_token_names[exp->binx.op]);
				}
			}
			else if(!are_assignment_types_valid(exp->binx.lhs, exp->binx.rhs))
				error_defer_e(exp->binx.lhs, "Type of lhs in assignment operation does not match the type of rhs\n");
		
			switch(exp->binx.op)
			{
				case TOK_PLUS:
				case TOK_MINUS:
				case TOK_MUL:
				case TOK_DIV: exp->tag = create_type_tag(TAG_NUMBER); break;
				
				case TOK_LTE:
				case TOK_GTE:
				case TOK_LT:
				case TOK_GT:
				case TOK_EQUALS:
				case TOK_NOTEQUAL:
				case TOK_LAND:
				case TOK_LOR: exp->tag = create_type_tag(TAG_BOOL); break;
				
				default: exp->tag = create_type_tag(TAG_VOID); break;
			}
		} break;
		
		case EXP_CALL:
		{
			resolve_type_tags(exp->callx.func);
			if(exp->callx.func->tag->type != TAG_FUNC)
				error_defer_e(exp, "Attempting to call something that is not a function\n");
			
			if(exp->callx.args.length != exp->callx.func->tag->func.arg_types.length)
				error_defer_e(exp, "Passed %d argument(s) into function expecting %d argument(s)\n", 
				exp->callx.args.length, exp->callx.func->tag->func.arg_types.length);
			else
			{
				for(int i = 0; i < exp->callx.args.length; ++i)
				{
					expr_t* arg = vec_get_value(&exp->callx.args, i, expr_t*);
					resolve_type_tags(arg);
				
					type_tag_t* expected_tag = vec_get_value(&exp->callx.func->tag->func.arg_types, i, type_tag_t*);
					if(!compare_type_tags(arg->tag, expected_tag)) error_defer_e(arg, "Type of argument %i does not match expected type\n", i + 1);
				}
			}
			
			exp->tag = exp->callx.func->tag->func.return_type;
		} break;
		
		case EXP_PAREN:
		{
			resolve_type_tags(exp->paren);
			exp->tag = exp->paren->tag;
		} break;
		
		case EXP_BLOCK:
		{
			exp->tag = create_type_tag(TAG_VOID);
			for(int i = 0; i < exp->block.length; ++i)
			{
				expr_t* e = vec_get_value(&exp->block, i, expr_t*);
				resolve_type_tags(e);
			}
		} break;
		
		case EXP_ARRAY_INDEX:
		{
			resolve_type_tags(exp->array_index.array);
			resolve_type_tags(exp->array_index.index);
			
			if(!is_type_tag(exp->array_index.index->tag, TAG_NUMBER))
				error_defer_e(exp->array_index.index, "Attempting to index array with non-number value\n");
			
			switch(exp->array_index.array->tag->type)
			{
				case TAG_STRING: exp->tag = create_type_tag(TAG_CHAR); break;
				case TAG_ARRAY: exp->tag = exp->array_index.array->tag->contained; break;
				default:
					error_defer_e(exp->array_index.array, "Attempting to index non-indexable type\n");
					break;
			}
		} break;
		
		case EXP_EXTERN: break;
		
		case EXP_ARRAY_LITERAL:
		{
			for(int i = 0; i < exp->array_literal.values.length; ++i)
				resolve_type_tags(vec_get_value(&exp->array_literal.values, i, expr_t*));
			
			type_tag_t* tag = create_type_tag(TAG_ARRAY);
			if(exp->array_literal.contained) tag->contained = exp->array_literal.contained;
			else
			{
				expr_t* first = vec_get_value(&exp->array_literal.values, 0, expr_t*);
				tag->contained = first->tag;
				
				for(int i = 1; i < exp->array_literal.values.length; ++i)
				{
					expr_t* e = vec_get_value(&exp->array_literal.values, i, expr_t*);
					if(!compare_type_tags(e->tag, tag->contained))
						tag = create_type_tag(TAG_DYNAMIC);			// okay, so it must be a dynamic literal
					// error_defer_e(e, "Array literal value type does not match the array's contained type\n");
				}
			}
			
			exp->tag = tag;
		} break;
		
		case EXP_LEN:
		{
			exp->tag = create_type_tag(TAG_NUMBER);
			resolve_type_tags(exp->len);
			
			if(exp->len->tag->type != TAG_STRING && exp->len->tag->type != TAG_ARRAY)
				error_defer_e(exp->len, "Attempted to find the length of non-measurable type\n");
		} break;
		
		case EXP_IF:
		{
			exp->tag = create_type_tag(TAG_VOID);
			resolve_type_tags(exp->ifx.cond);
			
			if(!is_type_tag(exp->ifx.cond->tag, TAG_BOOL))
				error_defer_e(exp->ifx.cond, "Condition does not evaluate to a boolean value\n");
			
			resolve_type_tags(exp->ifx.body);
			if(exp->ifx.alt)
				resolve_type_tags(exp->ifx.alt);
		} break;
		
		case EXP_WHILE:
		{
			exp->tag = create_type_tag(TAG_VOID);
			resolve_type_tags(exp->whilex.cond);
			
			if(!is_type_tag(exp->whilex.cond->tag, TAG_BOOL)) 
				error_defer_e(exp->whilex.cond, "Condition does not evaluate to a boolean value\n");
			
			resolve_type_tags(exp->whilex.body);
		} break;
		
		case EXP_RETURN:
		{
			exp->tag = create_type_tag(TAG_VOID);
			resolve_type_tags(exp->retx.value);
	
			if(!compare_type_tags(exp->retx.value->tag, exp->retx.parent->tag->func.return_type))
				error_defer_e(exp, "Returned value's type does not match the return type of the enclosing function.\n");
		} break;
		
		case EXP_FUNC:
		{
			exp->tag = create_type_tag(TAG_VOID);
			resolve_type_tags(exp->funcx.body);
		} break;
	}
}

static void compile_value_expr(script_t* script, expr_t* exp);
static void compile_call(script_t* script, expr_t* exp)
{
	for(int i = exp->callx.args.length - 1; i >= 0; --i)
	{
		expr_t* e = vec_get_value(&exp->callx.args, i, expr_t*);
		compile_value_expr(script, e);
	}		

	compile_value_expr(script, exp->callx.func);
	append_code(script, OP_CALL);
	append_code(script, exp->callx.args.length);
}

static int get_struct_type_member_index(type_tag_t* tag, const char* name, char* is_function)
{
	for(int i = 0; i < tag->ds.members.length; ++i)
	{
		type_tag_member_t* mem = vec_get(&tag->ds.members, i);
		if(strcmp(mem->name, name) == 0)
		{
			*is_function = 0;
			// NOTE: In unions, all members are located at the same index (0)
			if(tag->ds.is_union)
				return 0;
				
			return i;
		}
	}
	
	for(int i = 0; i < tag->ds.functions.length; ++i)
	{
		type_mem_fn_t* mem = vec_get(&tag->ds.functions, i);
		if(strcmp(mem->name, name) == 0)
		{
			*is_function = 1;
			return i;
		}
	}
	
	return -1;
}

static void compile_file_line_info(script_t* script, expr_t* exp, char ignore_last)
{
	if(ignore_last || !g_last_compiled_file || strcmp(g_last_compiled_file, exp->ctx.file) != 0)
	{
		append_code(script, OP_FILE);
		append_int(script, register_string(script, exp->ctx.file));
	}
	g_last_compiled_file = exp->ctx.file;
	
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
			// NOTE: VLA keeping track of which members have been initialized
			char initialized_members[exp->newx.type->ds.members.length];
			memset(initialized_members, 0, exp->newx.type->ds.members.length);
			
			int num_init = 0;
			for(int i = exp->newx.init.length - 1; i >= 0; --i)
			{
				expr_t* init = vec_get_value(&exp->newx.init, i, expr_t*);
				char is_function = 0;
				int index = get_struct_type_member_index(exp->newx.type, init->binx.lhs->varx.name, &is_function);
				
				if(index < 0) error_exit_e(init, "Attempted to initialize non-existent member '%s' in struct %s instantiation\n", 
							  init->binx.lhs->varx.name, exp->newx.type->ds.name);
				if(is_function) error_exit_e(init, "Attempted to initialize member function '%s' in struct %s instantiation\n",
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
		} break;
		
		case EXP_DOT:
		{
			char is_function = 0;
			int index = get_struct_type_member_index(exp->dotx.value->tag, exp->dotx.name, &is_function);
			if(index < 0) error_exit_e(exp, "Attempted to access non-existent member '%s' in struct %s\n", exp->dotx.name, exp->dotx.value->tag->ds.name);
			
			if(!is_function)
			{
				compile_value_expr(script, exp->dotx.value);
				append_code(script, OP_STRUCT_GET);
				append_int(script, index);
			}
			else
				error_exit_e(exp, "Attempted to access member function with '.' operator; use ':' operator instead\n");
		} break;

		case EXP_COLON:
		{
			char is_function = 0;
			int index = get_struct_type_member_index(exp->dotx.value->tag, exp->dotx.name, &is_function);
			if(index < 0) error_exit_e(exp, "Attempted to access non-existent member '%s' in struct %s\n", exp->dotx.name, exp->dotx.value->tag->ds.name);
			
			if(is_function)
			{
				append_code(script, OP_PUSH_FUNC);
				type_mem_fn_t* mem = vec_get(&exp->dotx.value->tag->ds.functions, index);
				append_int(script, mem->decl->index);
			}
			else
				error_exit_e(exp, "Attempted to access member value with ':' operator; use '.' operator instead\n");
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
			for(int i = exp->array_literal.values.length - 1; i >= 0; --i)
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
				func_decl_t* decl = reference_function(exp->varx.name);
				
				// TODO: Switch here
				if(decl->type == DECL_FUNCTION) append_code(script, OP_PUSH_FUNC);
				else if (decl->type == DECL_EXTERN) append_code(script, OP_PUSH_EXTERN_FUNC);
				else error_exit_p("Invalid function declaration type\n");
				
				append_int(script, decl->index);
			}
		} break;
		
		case EXP_EXTERN: break;
		
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
			char is_function = 0;
			int index = get_struct_type_member_index(exp->dotx.value->tag, exp->dotx.name, &is_function);
			if(index < 0) error_exit_e(exp, "Attempted to access non-existent member '%s' in struct %s\n", exp->dotx.name, exp->dotx.value->tag->ds.name);
			if(is_function) error_exit_e(exp, "Attempted to assign to member function '%s' of struct %s\n", exp->dotx.name, exp->dotx.value->tag->ds.name);
			
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
		case EXP_STRUCT_DECL:
		{
			for(int i = 0; i < exp->struct_tag->ds.functions.length; ++i)
			{
				type_mem_fn_t* mem = vec_get(&exp->struct_tag->ds.functions, i);
				
				// NOTE: Copied from case EXP_FUNC
				append_code(script, OP_GOTO);
				int loc = script->code.length;
				append_int(script, 0);
				
				vec_push_back(&script->function_pcs, &script->code.length);
				
				for(int i = 0; i < mem->decl->locals.length; ++i)
					append_code(script, OP_PUSH_NULL);
				
				compile_expr(script, mem->body);
				
				append_code(script, OP_RETURN);
				patch_int(script, loc, script->code.length);
			}
		} break;
		
		case EXP_EXTERN:
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
		
		case EXP_FUNC:
		{
			append_code(script, OP_GOTO);
			int loc = script->code.length;
			append_int(script, 0);
			
			char* name = estrdup(exp->funcx.decl->name);
			vec_push_back(&script->function_names, &name);
			vec_push_back(&script->function_pcs, &script->code.length);
			
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
	script_value_t* val = NULL;
	vec_resize(&script->globals, g_globals.length, &val);
}

void script_bind_extern(script_t* script, const char* name, script_extern_t ext)
{
	char* name_copy = estrdup(name);
	vec_push_back(&script->extern_names, &name_copy);
	vec_push_back(&script->externs, &ext);
}

// DEFAULT EXTERNS

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
	script_push_number(script, floor(val->number));
	script_return_top(script);
}

static void ext_ceil(script_t* script, vector_t* args)
{
	script_value_t* val = script_get_arg(args, 0);
	script_push_number(script, ceil(val->number));
	script_return_top(script);
}

static void delete_u8_buffer(void* pbuf)
{
	vector_t* buf = pbuf;
	vec_destroy(buf);
	free(buf);
}

static void ext_make_u8_buffer(script_t* script, vector_t* args)
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
	script_bind_extern(script, "make_array_of_length", ext_make_array_of_length);
	
	script_bind_extern(script, "char_to_number", ext_char_to_number);
	script_bind_extern(script, "number_to_char", ext_number_to_char);
	script_bind_extern(script, "number_to_string", ext_number_to_string);
	script_bind_extern(script, "string_to_number", ext_string_to_number);
	
	script_bind_extern(script, "read_char", ext_read_char);
	script_bind_extern(script, "print_char", ext_print_char);
	
	script_bind_extern(script, "floor", ext_floor);
	script_bind_extern(script, "ceil", ext_ceil);
	
	script_bind_extern(script, "make_u8_buffer", ext_make_u8_buffer);
	script_bind_extern(script, "u8_buffer_clear", ext_u8_buffer_clear);
	script_bind_extern(script, "u8_buffer_length", ext_u8_buffer_length);
	script_bind_extern(script, "u8_buffer_push", ext_u8_buffer_push);
	script_bind_extern(script, "u8_buffer_pop", ext_u8_buffer_pop);
	script_bind_extern(script, "u8_buffer_to_string", ext_u8_buffer_to_string);
}

void script_init(script_t* script)
{
	g_line = 1;
	g_file = "none";

	init_type_tags();
	init_symbols();

	script->in_extern = 0;
	
	script->cur_file = "unknown";
	script->cur_line = 0;
	
	script->gc_head = NULL;
	script->ret_val = NULL;
	
	script->num_objects = 0;
	script->max_objects_until_gc = INIT_GC_THRESH;
	script->pc = -1;
	script->fp = 0;
	
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
		
		case VAL_STRING: free(val->string.data); break;
		case VAL_ARRAY: vec_destroy(&val->array); break;
		case VAL_STRUCT_INSTANCE: vec_destroy(&val->ds.members); break;
		case VAL_NATIVE: if(val->nat.on_delete) val->nat.on_delete(val->nat.value); break;
	}
	
	free(val);
}

static void destroy_all_values(script_t* script)
{
	while(script->gc_head)
	{
		script_value_t* next = script->gc_head->next;
		delete_value(script, script->gc_head);
		script->gc_head = next;
	}
}

static void reset_script(script_t* script)
{	
	destroy_all_values(script);
	
	script->in_extern = 0;
	
	script->num_objects = 0;
	script->max_objects_until_gc = INIT_GC_THRESH;
	
	script->pc = -1;
	script->fp = 0;

	script->ret_val = NULL;

	vec_clear(&script->globals);
	
	vec_clear(&script->stack);
	vec_clear(&script->indir);
	
	vec_clear(&script->code);
	
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

static script_value_t* new_value(script_t* script, script_value_type_t type)
{
	if(!script->in_extern && script->num_objects >= script->max_objects_until_gc) collect_garbage(script);

	script_value_t* val = emalloc(sizeof(script_value_t));
	
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

void script_push_bool(script_t* script, char bv)
{
	script_value_t* val = new_value(script, VAL_BOOL);
	val->boolean = bv;
	push_value(script, val);
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
	return vec_get_value(args, args->length - index - 1, script_value_t*);
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
}

static void pop_stack_frame(script_t* script)
{
	// resize the stack without resetting the values
	vec_resize(&script->stack, script->fp, NULL);
	
	vec_pop_back(&script->indir, &script->pc);
	vec_pop_back(&script->indir, &script->fp);
	
	int nargs;
	vec_pop_back(&script->indir, &nargs);
	
	vec_resize(&script->stack, script->stack.length - nargs, NULL);
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
	
	while(pc < script->code.length)
	{
		fprintf(out, "%d: ", pc);
		
		word code = vec_get_value(&script->code, pc, word);
		++pc;
		
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
				fprintf(out, "file '%s'\n", vec_get_value(&script->strings, index, script_string_t).data);
				pc += sizeof(int) / sizeof(word);
			} break;
			
			case OP_LINE:
			{
				int line_no = read_int_at(script, pc);
				fprintf(out, "line %d\n", line_no);
				pc += sizeof(int) / sizeof(word);
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
			vec_resize(&array, length, NULL);
			
			for(int i = 0; i < length; ++i)
			{
				script_value_t* val = pop_value(script);
				vec_set(&array, i, &val);
			}
			
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
			write_value(pop_value(script));
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
				
				// NOTE: copies argument pointers over to the args vector
				vec_copy_region(&args, &script->stack, 0, script->stack.length - nargs, nargs);
				
				script->in_extern = 1;
				vec_get_value(&script->externs, function.index, script_extern_t)(script, &args);
				script->in_extern = 0;
				
				vec_destroy(&args);
				
				script->stack.length = new_stack_length;
			}
			else
			{
				push_stack_frame(script, nargs);
				script->pc = vec_get_value(&script->function_pcs, function.index, int);
			}
		} break;
		
		case OP_RETURN:
		{
			script->ret_val = NULL;
			pop_stack_frame(script);
		} break;
		
		case OP_RETURN_VALUE:
		{
			script->ret_val = pop_value(script);
			pop_stack_frame(script);
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

		case OP_HALT:
		{
			script->pc = -1;
		} break;
	}
}

void script_load_run_file(script_t* script, const char* filename)
{
	FILE* in = fopen(filename, "rb");
	
	if(!in)
		error_exit("Failed to open file '%s'\n", filename);
	
	g_file = filename;
	add_module(script, filename);
	
	script_run_file(script, in);

	fclose(in);
}

void script_run_file(script_t* script, FILE* in)
{
	fseek(in, 0, SEEK_END);
	size_t length = ftell(in);
	fseek(in, 0, SEEK_SET);
	
	char* str = emalloc(length + 1);
	fread(str, sizeof(char), length, in);
	str[length] = '\0';
	
	rewind(in);
	
	script_run_code(script, str);
	
	free(str);
}

void script_run_code(script_t* script, const char* code)
{
	g_code = code;
	g_line = 1;
	
	reset_type_tags();
	reset_symbols();
	
	reset_script(script);
	
	// NOTE: if it exists, the module from which the code is sourced is going to be parsed
	// otherwise, a dummy module is created
	if(script->modules.length == 0)
		add_module(script, "__dummy_module");
		
	script_module_t* module = vec_get(&script->modules, 0);
	parse_program(script, &module->expr_list);
	module->parsed = 1;
	
	// NOTE: parse any unparsed modules
	for(int i = 0; i < script->modules.length; ++i)
	{
		script_module_t* module = vec_get(&script->modules, i);
		
		if(!module->parsed)
		{
			FILE* file = fopen(module->local_path, "rb");
			if(!file)
			{
				fprintf(stderr, "Failed to open file '%s' for reading\n", module->local_path);
				fprintf(stderr, "Skipping module: %s\n", module->local_path);
				continue;
			}
			
			vector_t codebuf;
			vec_init(&codebuf, sizeof(char));
			
			fseek(file, 0, SEEK_END);
			size_t length = ftell(file);
			fseek(file, 0, SEEK_SET);
			
			vec_resize(&codebuf, length + 1, NULL);
			
			fread(codebuf.data, 1, length, file);			
			fclose(file);
			
			codebuf.data[length] = '\0';
		
			g_file = module->local_path;
			g_line = 1;
			g_code = (char*)codebuf.data;
			
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
	
	check_all_tags_defined();
	if(g_has_error) error_exit("Found errors in script code. Stopping compilation\n");

	char symbol_error = 0;
	
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
	for(int i = script->modules.length - 1; i >= 0; --i)
	{
		script_module_t* module = vec_get(&script->modules, i);
		for(int expr_index = 0; expr_index < module->expr_list.length; ++expr_index)
		{
			expr_t* node = vec_get_value(&module->expr_list, expr_index, expr_t*);
		
			char prev_error = g_has_error;
			resolve_symbols(node);
			symbol_error = prev_error ? 0 : g_has_error;
			if(!symbol_error) resolve_type_tags(node);
		
			//debug_expr(script, node);
			//printf("\n");
		
			if(!g_has_error) compile_expr(script, node);
			delete_expr(node);
		}
		
		// NOTE: module->expr_list is no longer valid after this point
		vec_destroy(&module->expr_list);
	}
	
	if(g_has_error) error_exit("Found errors in script code. Stopping compilation\n");
	
	append_code(script, OP_HALT);
	disassemble(script, stdout);

	allocate_globals(script);
	
	script->pc = 0;
	while(script->pc >= 0)
		execute_cycle(script);
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
	int fp = script->fp;
	push_stack_frame(script, (word)nargs);
	script->pc = vec_get_value(&script->function_pcs, function.index, int);
	
	while(script->fp > fp)
		execute_cycle(script);
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

static void destroy_modules(void* p_module)
{
	script_module_t* module = p_module;
	free(module->local_path);
}

void script_destroy(script_t* script)
{
	destroy_type_tags();
	destroy_symbols();
	
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
	
	vec_traverse(&script->modules, destroy_modules);
	vec_destroy(&script->modules);
}
