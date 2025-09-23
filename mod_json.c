#include "es.h"
#include "prim.h"
#include <cjson/cJSON.h>
#include <cjson/cJSON_Utils.h>
LIBRARY(mod_json);

enum {
	JsonChild = 1<<0,

	JTString = 1,
	JTNumber = 2,
	JTObject = 3,
	JTArray = 4,
	JTBoolean = 5,
	JTNull = 6,
};

typedef struct {
	Object *parent;
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
	if(!(obj->flags & JsonChild))
		cJSON_Delete(json(obj)->data);
	return 0;
}

int
is_json_object(Term *term)
{
	if(term == nil)
		return 0;
	if(term->kind != tkObject)
		return 0;
	if(!object_is_type(getobject(term), "json"))
		return 0;
	return 1;
}

Object*
create_json_data(void)
{
	Object *o;

	o = allocate_object("json", sizeof(json_data));
	assert(o); /* must never fail */
	json(o)->data = nil;
	o->sysflags |= ObjectFreeWhenNoRefs;
	o->sysflags |= ObjectGcManaged;
	o->flags = 0;

	return o;
}

Object*
decode_json(char *str)
{
	Object *obj;

	obj = create_json_data();
	json(obj)->data = cJSON_Parse(str);
	json(obj)->parent = nil;
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

Object*
create_json_object(int type, char *string, double number, Boolean bool)
{
	Object *newobj = nil;

	newobj = create_json_data();
	switch(type){
	default:
		unreachable();
		return nil;
	case JTString:
		json(newobj)->data = cJSON_CreateString(string);
		break;
	case JTNumber:
		json(newobj)->data = cJSON_CreateNumber(number);
		break;
	case JTObject:
		json(newobj)->data = cJSON_CreateObject();
		break;
	case JTArray:
		json(newobj)->data = cJSON_CreateArray();
		break;
	case JTBoolean:
		if(bool)
			json(newobj)->data = cJSON_CreateTrue();
		else
			json(newobj)->data = cJSON_CreateFalse();
		break;
	case JTNull:
		json(newobj)->data = cJSON_CreateNull();
		break;
	}
	newobj->sysflags |= ObjectInitialized;
	return newobj;
}

Object*
add_to_json_object(Object *parent, Object *child, char *label)
{
	assert(parent);
	assert(child);
	if(cJSON_IsArray(json(parent)->data)){
		cJSON_AddItemToArray(json(parent)->data, json(child)->data);
		child->flags |= JsonChild;
		return parent;
	}
	if(cJSON_IsObject(json(parent)->data)){
		assert(label);
		cJSON_AddItemToObject(json(parent)->data, label, json(child)->data);
		child->flags |= JsonChild;
		return parent;
	}
	return nil;
}

int
get_json_object_type(Object *obj)
{
	if(cJSON_IsString(json(obj)->data))
		return JTString;
	if(cJSON_IsNumber(json(obj)->data))
		return JTNumber;
	if(cJSON_IsObject(json(obj)->data))
		return JTObject;
	if(cJSON_IsArray(json(obj)->data))
		return JTArray;
	if(cJSON_IsBool(json(obj)->data))
		return JTBoolean;
	if(cJSON_IsNull(json(obj)->data))
		return JTNull;
	unreachable();
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
	if(!is_json_object(list->term))
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
		fail("$&encode_json_formatted", "missing argument");
	if(!is_json_object(list->term))
		fail("$&encode_json_formatted", "requires a json object as an argument");

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

