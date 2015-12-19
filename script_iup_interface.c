#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "script.h"
#include "iup.h"

typedef struct
{
	script_t* script;
	script_function_t function;
} cb_data_t;

static void error_exit(const char* fmt, ...)
{
	fprintf(stderr, "\nError:\n");
	
	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	
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


static void ext_iup_label(script_t* script, vector_t* args)
{
	script_string_t name = script_get_arg(args, 0)->string;
	Ihandle* label = IupLabel(estrdup(name.data));
	script_push_native(script, label, NULL, NULL);
	script_return_top(script);
}

static void ext_iup_button(script_t* script, vector_t* args)
{
	script_string_t name = script_get_arg(args, 0)->string;
	Ihandle* button = IupButton(estrdup(name.data), NULL);
	script_push_native(script, button, NULL, NULL);
	script_return_top(script);
}

static void ext_iup_vbox(script_t* script, vector_t* args)
{
	vector_t* array = &script_get_arg(args, 0)->array;
	vector_t handle_array;
	
	vec_init(&handle_array, sizeof(Ihandle*));
	vec_reserve(&handle_array, array->length + 1);
	
	for(int i = 0; i < array->length; ++i)
	{
		script_value_t* val = vec_get_value(array, i, script_value_t*);
		vec_push_back(&handle_array, &val->nat.value);
	}
	Ihandle* null_handle = NULL;
	vec_push_back(&handle_array, &null_handle); 
	
	Ihandle* vbox = IupVboxv((Ihandle**)handle_array.data);
	vec_destroy(&handle_array);
	
	script_push_native(script, vbox, NULL, NULL);
	script_return_top(script);
}

static void ext_iup_dialog(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	Ihandle* dlg = IupDialog(handle);
	
	script_push_native(script, dlg, NULL, NULL);
	script_return_top(script);
}

static void ext_iup_set_attribute(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	script_string_t attr_name = script_get_arg(args, 1)->string;
	script_string_t attr_value = script_get_arg(args, 2)->string;
	
	IupSetAttribute(handle, attr_name.data, attr_value.data);
}

static int iup_callback(Ihandle* self)
{
	cb_data_t* data = (cb_data_t*)IupGetAttribute(self, "CBDATA");
	
	script_push_native(data->script, self, NULL, NULL);
	script_call_function(data->script, data->function, 1);
	
	if(!data->script->ret_val) error_exit("IUP Callback must return a value\n");
	
	return (int)data->script->ret_val->number;
}

static void ext_iup_set_callback(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	script_string_t cb_name = script_get_arg(args, 1)->string;
	script_function_t cb_func = script_get_arg(args, 2)->function;
	
	// TODO: add an extern to free this data
	cb_data_t* data = emalloc(sizeof(cb_data_t));
	data->script = script;
	data->function = cb_func;
	
	IupSetAttribute(handle, "CBDATA", (char*)data);
	IupSetCallback(handle, cb_name.data, (Icallback)iup_callback);
}

static void ext_iup_show_xy(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	int x = (int)script_get_arg(args, 1)->number;
	int y = (int)script_get_arg(args, 2)->number;
	
	IupShowXY(handle, x, y);
}

static void ext_iup_main_loop(script_t* script, vector_t* args)
{
	IupMainLoop();
}

static void ext_iup_close(script_t* script, vector_t* args)
{
	IupClose();
}

void script_bind_iup(script_t* script, int argc, char** argv)
{
	IupOpen(&argc, &argv);
	
	script_bind_extern(script, "iup_label", ext_iup_label);
	script_bind_extern(script, "iup_button", ext_iup_button);
	script_bind_extern(script, "iup_vbox", ext_iup_vbox);
	script_bind_extern(script, "iup_dialog", ext_iup_dialog);
	script_bind_extern(script, "iup_set_attribute", ext_iup_set_attribute);
	script_bind_extern(script, "iup_set_callback", ext_iup_set_callback);
	script_bind_extern(script, "iup_show_xy", ext_iup_show_xy);
	script_bind_extern(script, "iup_main_loop", ext_iup_main_loop);
	script_bind_extern(script, "iup_close", ext_iup_close);
}
