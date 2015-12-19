#include <stdlib.h>
#include "script.h"
#include "iup.h"

static void ext_iup_label(script_t* script, vector_t* args)
{
	script_string_t name = script_get_arg(args, 0)->string;
	Ihandle* label = IupLabel(name.data);
	script_push_native(script, label, NULL, NULL);
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
	script_bind_extern(script, "iup_vbox", ext_iup_vbox);
	script_bind_extern(script, "iup_dialog", ext_iup_dialog);
	script_bind_extern(script, "iup_set_attribute", ext_iup_set_attribute);
	script_bind_extern(script, "iup_show_xy", ext_iup_show_xy);
	script_bind_extern(script, "iup_main_loop", ext_iup_main_loop);
	script_bind_extern(script, "iup_close", ext_iup_close);
}
