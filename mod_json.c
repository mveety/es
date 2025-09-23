#include "es.h"
#include "prim.h"
#include <cjson/cJSON.h>
#include <cjson/cJSON_Utils.h>
LIBRARY(mod_json);

typedef struct {
	cJSON *data;
} json_data;

json_data*
json(Object *obj)
{
	return (json_data*)obj->data;
}

int
json_deallocate(Object *obj)
{
	cJSON_Delete(json(obj)->data);
	return 0;
}

Object*
create_json_data(void)
{
	Object *o;

	o = allocate_object("json", sizeof(json_data));
	json(o)->data = nil;
	o->sysflags |= ObjectFreeWhenNoRefs;
	o->sysflags |= ObjectGcManaged;

	return o;
}

Object*
decode_json(char *str)
{
	Object *obj;

	obj = create_json_data();
	json(obj)->data = cJSON_Parse(str);
	if(!(json(obj)->data))
		return nil;

	obj->sysflags |= ObjectInitialized;
	return obj;
}

char*
encode_json(Object *obj)
{
	if(!(obj->sysflags & ObjectInitialized))
		return nil;
	
	return cJSON_PrintUnformatted(json(obj)->data);
}

char*
encode_json_formatted(Object *obj)
{
	if(!(obj->sysflags & ObjectInitialized))
		return nil;

	return cJSON_Print(json(obj)->data);
}

int
dyn_onload(void)
{
	if(define_type("json", &json_deallocate) < 0)
		return -1;
	return 0;
}

int
dyn_onunload(void)
{
	undefine_type("json");
	return 0;
}

PRIM(decode_json) {
	List *res = nil; Root r_res;
	char *str = nil;
	Object *obj;

	if(list == nil)
		fail("$&decode_json", "missing argument");

	gcref(&r_res, (void**)&res);
	gcdisable();

	str = getstr(list->term);
	obj = decode_json(str);
	if(!obj)
		goto fail;

	res = mklist(mkobject(obj), nil);
	gcenable();
	gcrderef(&r_res);
	return res;

fail:
	gcenable();
	gcrderef(&r_res);
	fail("$&decode_json", "error decoding json data");
	return nil;
}

PRIM(encode_json) {
	List *args = nil; Root r_args;
	List *res = nil; Root r_res;
	char *str = nil;
	Object *obj;

	if(list == nil)
		fail("$&encode_json", "missing argument");
	if(!object_is_type(getobject(list->term), "json"))
		fail("$&encode_json", "requires a json object as an argument");

	gcref(&r_args, (void**)&args);
	gcref(&r_res, (void**)&res);
	args = list;
	obj = getobject(args->term);

	str = encode_json(obj);
	if(!str)
		goto fail;
	
	res = mklist(mkstr(gcdup(str)), nil);
	free(str);

fail:
	gcrderef(&r_res);
	gcrderef(&r_args);
	return res;
}

PRIM(encode_json_formatted) {
	List *args = nil; Root r_args;
	List *res = nil; Root r_res;
	char *str = nil;
	Object *obj;

	if(list == nil)
		fail("$&encode_json", "missing argument");
	if(!object_is_type(getobject(list->term), "json"))
		fail("$&encode_json", "requires a json object as an argument");

	gcref(&r_args, (void**)&args);
	gcref(&r_res, (void**)&res);
	args = list;
	obj = getobject(args->term);

	str = encode_json_formatted(obj);
	if(!str)
		goto fail;
	
	res = mklist(mkstr(gcdup(str)), nil);
	free(str);

fail:
	gcrderef(&r_res);
	gcrderef(&r_args);
	return res;
}

DYNPRIMS() = {
	DX(decode_json),
	DX(encode_json),
	DX(encode_json_formatted),
	PRIMSEND,
};

