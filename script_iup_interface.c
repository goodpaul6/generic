#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "script.h"
#include "iup.h"

enum
{
	CB_SELF,
	NUM_CB
};

typedef struct
{
	int index;
	script_t* script;
	script_function_t functions[NUM_CB];
} cb_data_t;

static vector_t g_all_cb_data;

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

static int iup_self_callback(Ihandle* self)
{
	cb_data_t* data = (cb_data_t*)IupGetAttribute(self, "CBDATA");
	
	script_push_native(data->script, self, NULL, NULL);
	script_call_function(data->script, data->functions[CB_SELF], 1);
	
	return (int)data->script->ret_val->number;
}

static Icallback g_callbacks[NUM_CB] = {
	(Icallback)iup_self_callback	
};

static cb_data_t* create_cb_data(script_t* script)
{
	cb_data_t* data = emalloc(sizeof(cb_data_t));
	
	data->index = g_all_cb_data.length;
	data->script = script;
	
	vec_push_back(&g_all_cb_data, &data);

	return data;
}

static void iup_handle_free(void* vhandle)
{
	/*Ihandle* handle = vhandle;
	cb_data_t* data = (cb_data_t*)IupGetAttribute(handle, "CBDATA");
	
	if(data)
	{
		// TODO: maybe don't do this slow operation moving all the indices back
		for(int i = data->index + 1; i < g_all_cb_data.length; ++i)
		{
			cb_data_t* d = vec_get_value(&g_all_cb_data, i, cb_data_t*);
			d->index -= 1;
		}
	
		vec_remove(&g_all_cb_data, data->index);
		free(data);
		printf("freeing handle\n");
	}*/
}

static void ext_iup_label(script_t* script, vector_t* args)
{
	script_string_t name = script_get_arg(args, 0)->string;
	Ihandle* label = IupLabel(name.data);
	script_push_native(script, label, NULL, iup_handle_free);
	script_return_top(script);
}

static void ext_iup_button(script_t* script, vector_t* args)
{
	script_string_t name = script_get_arg(args, 0)->string;
	Ihandle* button = IupButton(name.data, NULL);
	script_push_native(script, button, NULL, iup_handle_free);
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
	
	script_push_native(script, vbox, NULL, iup_handle_free);
	script_return_top(script);
}

static void ext_iup_append(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	Ihandle* new_child = script_get_arg(args, 1)->nat.value;
	
	IupAppend(handle, new_child);
}

static void ext_iup_dialog(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	Ihandle* dlg = IupDialog(handle);
	
	script_push_native(script, dlg, NULL, iup_handle_free);
	script_return_top(script);
}

static void ext_iup_set_attribute(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	script_string_t attr_name = script_get_arg(args, 1)->string;
	script_string_t attr_value = script_get_arg(args, 2)->string;
	
	IupSetAttribute(handle, attr_name.data, estrdup(attr_value.data));
}

static void ext_iup_set_callback(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	script_string_t cb_name = script_get_arg(args, 1)->string;
	script_function_t cb_func = script_get_arg(args, 2)->function;
	int cb_type = (int)script_get_arg(args, 3)->number;
	
	// TODO: check if cb_type is valid
	cb_data_t* data = create_cb_data(script);
	data->functions[cb_type] = cb_func;
	
	IupSetAttribute(handle, "CBDATA", (char*)data);
	IupSetCallback(handle, cb_name.data, g_callbacks[cb_type]);
}

static void ext_iup_map(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	IupMap(handle);
}

static void ext_iup_show_xy(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	int x = (int)script_get_arg(args, 1)->number;
	int y = (int)script_get_arg(args, 2)->number;
	
	IupShowXY(handle, x, y);
}

static void ext_iup_refresh(script_t* script, vector_t* args)
{
	Ihandle* handle = script_get_arg(args, 0)->nat.value;
	IupRefresh(handle);
}

static void ext_iup_main_loop(script_t* script, vector_t* args)
{
	IupMainLoop();
}

static void ext_iup_close(script_t* script, vector_t* args)
{
	IupClose();
	vec_destroy(&g_all_cb_data);
}

void script_bind_iup(script_t* script, int argc, char** argv)
{
	vec_init(&g_all_cb_data, sizeof(cb_data_t*));
	
	IupOpen(&argc, &argv);
	
	script_bind_extern(script, "iup_label", ext_iup_label);
	script_bind_extern(script, "iup_button", ext_iup_button);
	script_bind_extern(script, "iup_vbox", ext_iup_vbox);
	script_bind_extern(script, "iup_append", ext_iup_append);
	script_bind_extern(script, "iup_dialog", ext_iup_dialog);
	script_bind_extern(script, "iup_set_attribute", ext_iup_set_attribute);
	script_bind_extern(script, "iup_set_callback", ext_iup_set_callback);
	script_bind_extern(script, "iup_map", ext_iup_map);
	script_bind_extern(script, "iup_show_xy", ext_iup_show_xy);
	script_bind_extern(script, "iup_refresh", ext_iup_refresh);
	script_bind_extern(script, "iup_main_loop", ext_iup_main_loop);
	script_bind_extern(script, "iup_close", ext_iup_close);
}
