#include "es.h"
#include "prim.h"
#include "stdenv.h"
#include "float_util.h"
#include <cjson/cJSON.h>
#include <cjson/cJSON_Utils.h>

enum {
	JsonChild = 1 << 0,

	JTBadType = -1,
	JTString = 1,
	JTNumber = 2,
	JTObject = 3,
	JTArray = 4,
	JTBoolean = 5,
	JTNull = 6,

	JOpGet = 0,
	JOpDetach = 1,
};

typedef struct {
	Object *parent;
	cJSON *data;
} json_data;

json_data *
json(Object *obj)
{
	return (json_data *)obj->data;
}

int
json_deallocate(Object *obj)
{
	if(!(obj->flags & JsonChild))
		cJSON_Delete(json(obj)->data);
	return 0;
}

int
json_refdeps(Object *obj)
{
	if(obj->flags & JsonChild) {
		assert(json(obj)->parent);
		refobject(json(obj)->parent);
	}
	return 0;
}

char *
json_stringify(Object *obj)
{
	if(!(obj->sysflags & ObjectInitialized))
		return nil;

	return cJSON_PrintUnformatted(json(obj)->data);
}

int
is_json_object(Term *term)
{
	Object *obj;

	if(term == nil)
		return 0;
	if((obj = getobject(term)) == nil)
		return 0;
	if(!object_is_type(obj, "json"))
		return 0;
	return 1;
}

Object *
create_json_data(void)
{
	Object *o;

	o = allocate_object("json", sizeof(json_data));
	assert(o);
	json(o)->data = nil;
	o->sysflags |= ObjectFreeWhenNoRefs;
	o->sysflags |= ObjectGcManaged;
	o->flags = 0;

	return o;
}

Object *
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

Result
json_objectify(char *str)
{
	Object *obj = nil;

	obj = decode_json(str);
	if(!obj)
		return result(nil, ObjectifyInvalidObjectFormat);
	return result(obj, ObjectifyOk);
}

char *
encode_json(Object *obj)
{
	if(!(obj->sysflags & ObjectInitialized))
		return nil;

	return cJSON_PrintUnformatted(json(obj)->data);
}

char *
encode_json_formatted(Object *obj)
{
	if(!(obj->sysflags & ObjectInitialized))
		return nil;

	return cJSON_Print(json(obj)->data);
}

Object *
create_json_object(int type, char *string, double number, Boolean b)
{
	Object *newobj = nil;

	newobj = create_json_data();
	switch(type) {
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
		if(b)
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

Object *
add_to_json_object(Object *parent, Object *child, char *label)
{
	assert(parent);
	assert(child);
	if(cJSON_IsArray(json(parent)->data)) {
		cJSON_AddItemToArray(json(parent)->data, json(child)->data);
		child->flags |= JsonChild;
		json(child)->parent = parent;
		return parent;
	}
	if(cJSON_IsObject(json(parent)->data)) {
		assert(label);
		cJSON_AddItemToObject(json(parent)->data, label, json(child)->data);
		child->flags |= JsonChild;
		json(child)->parent = parent;
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
typename2int(char *typename)
{
	if(streq(typename, "string"))
		return JTString;
	if(streq(typename, "number"))
		return JTNumber;
	if(streq(typename, "object"))
		return JTObject;
	if(streq(typename, "array"))
		return JTArray;
	if(streq(typename, "boolean"))
		return JTBoolean;
	if(streq(typename, "null"))
		return JTNull;
	return JTBadType;
}

char *
int2typename(int type)
{
	switch(type) {
	default:
		unreachable();
		break;
	case JTString:
		return "string";
	case JTNumber:
		return "number";
	case JTObject:
		return "object";
	case JTArray:
		return "array";
	case JTBoolean:
		return "boolean";
	case JTNull:
		return "null";
	}
	unreachable();
	return 0;
}

Object *
get_json_object(Object *obj, char *key, int index)
{
	Object *child;
	cJSON *json_object;

	if(cJSON_IsObject(json(obj)->data)) {
		json_object = cJSON_GetObjectItemCaseSensitive(json(obj)->data, key);
		if(!json_object)
			return nil;
	} else if(cJSON_IsArray(json(obj)->data)) {
		json_object = cJSON_GetArrayItem(json(obj)->data, index);
		if(!json_object)
			return nil;
	} else {
		unreachable();
	}
	child = create_json_data();
	json(child)->data = json_object;
	child->sysflags |= ObjectInitialized;
	child->flags |= JsonChild;
	json(child)->parent = obj;
	return child;
}

Object *
detach_json_object(Object *obj, char *key, int index)
{
	Object *child;
	cJSON *json_object;

	if(cJSON_IsObject(json(obj)->data)) {
		json_object = cJSON_DetachItemFromObjectCaseSensitive(json(obj)->data, key);
		if(!json_object)
			return nil;
	} else if(cJSON_IsArray(json(obj)->data)) {
		json_object = cJSON_DetachItemFromArray(json(obj)->data, index);
		if(!json_object)
			return nil;
	} else {
		unreachable();
	}

	child = create_json_data();
	json(child)->data = json_object;
	child->sysflags |= ObjectInitialized;
	return child;
}

int
json_onload(void)
{
	if(define_type("json", &json_deallocate, &json_refdeps) < 0)
		return ErrorModuleNotLoaded;
	define_stringifier("json", &json_stringify);
	define_objectifier("json", &json_objectify);
	return 0;
}

int
json_onunload(void)
{
	if(undefine_type("json"))
		return ErrorModuleInUse;
	return 0;
}

PRIM(json_dumpobject){
	Object *obj = nil;
	char *typename = nil;

	if(list == nil)
		fail("$&json_dumpobject", "missing argument");
	if(!is_json_object(list->term))
		fail("$&json_dumpobject", "requires a json object as an argument");

	obj = getobject(list->term);

	typename = gettypename(obj->type);
	dprintf(2, "object %d: type=%s(%d), id=%d, sysflags=%x, size=%lu, refs=%lu\n", obj->id,
			typename, obj->type, obj->id, obj->sysflags, obj->size, obj->refcount);
	dprintf(2, "\tflags=%x, json_data.parent=%p, json_data.data=%p\n", obj->flags,
			json(obj)->parent, json(obj)->data);
	if(json(obj)->parent)
		dprintf(2, "parent: obj%d: type=%s(%d), id=%d, sysflags=%x, size=%lu, refs=%lu\n",
				json(obj)->parent->id, gettypename(json(obj)->parent->type),
				json(obj)->parent->type, json(obj)->parent->id, json(obj)->parent->sysflags,
				json(obj)->parent->size, json(obj)->parent->refcount);

	return nil;
}


PRIM(json_decode) {
	List *res = nil; Root r_res;
	char *str = nil;
	Object *obj;

	if(list == nil)
		fail("$&json_decode", "missing argument");

	gcref(&r_res, (void **)&res);
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
	fail("$&json_decode", "error decoding json data");
	return nil;
}

PRIM(json_encode) {
	List *args = nil; Root r_args;
	List *res = nil; Root r_res;
	char *str = nil;
	Object *obj;

	if(list == nil)
		fail("$&json_encode", "missing argument");
	if(!is_json_object(list->term))
		fail("$&json_encode", "requires a json object as an argument");

	gcref(&r_args, (void **)&args);
	gcref(&r_res, (void **)&res);
	args = list;
	obj = getobject(args->term);

	str = encode_json(obj);
	if(!str)
		goto fail;

	res = mklist(mkstr(gcdup(str)), nil);
	efree(str);

fail:
	gcrderef(&r_res);
	gcrderef(&r_args);
	return res;
}

PRIM(json_encode_formatted) {
	List *args = nil; Root r_args;
	List *res = nil; Root r_res;
	char *str = nil;
	Object *obj;

	if(list == nil)
		fail("$&json_encode_formatted", "missing argument");
	if(!is_json_object(list->term))
		fail("$&json_encode_formatted", "requires a json object as an argument");

	gcref(&r_args, (void **)&args);
	gcref(&r_res, (void **)&res);
	args = list;
	obj = getobject(args->term);

	str = encode_json_formatted(obj);
	if(!str)
		goto fail;

	res = mklist(mkstr(gcdup(str)), nil);
	efree(str);

fail:
	gcrderef(&r_res);
	gcrderef(&r_args);
	return res;
}

PRIM(json_create) {
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	double number;
	Object *resobj;
	int type;

	if(list == nil)
		fail("$&json_create", "missing type");

	gcref(&r_lp, (void **)&lp);
	gcref(&r_res, (void **)&res);

	lp = list;

	switch((type = typename2int(getstr(lp->term)))) {
	default:
		unreachable();
		break;
	case JTBadType:
		fail("$&json_create", "invalid type: %s", getstr(lp->term));
	case JTString:
		if(lp->next == nil)
			fail("$&json_create", "missing data");
		resobj = create_json_object(JTString, getstr(lp->next->term), 0, FALSE);
		break;
	case JTNumber:
		if(lp->next == nil)
			fail("$&json_create", "missing data");
		number = termtof(lp->next->term, "$&json_create_object", 2);
		resobj = create_json_object(JTNumber, nil, number, FALSE);
		break;
	case JTObject:
	case JTArray:
	case JTNull:
		resobj = create_json_object(type, nil, 0, FALSE);
		break;
	case JTBoolean:
		if(lp->next == nil)
			fail("$&json_create", "missing data");
		if(termeq(lp->next->term, "true") || termeq(lp->next->term, "0"))
			resobj = create_json_object(JTBoolean, nil, 0, TRUE);
		else
			resobj = create_json_object(JTBoolean, nil, 0, FALSE);
		break;
	}

	res = mklist(mkobject(resobj), nil);

	gcrderef(&r_res);
	gcrderef(&r_lp);
	return res;
}

PRIM(json_addto){
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	char *name = nil; Root r_name;
	int type;
	Object *parent;
	Object *newparent;
	Object *child;

	if(list == nil)
		fail("$&json_addto", "missing parent object");
	if(!is_json_object(list->term))
		fail("$&json_addto", "$1 must be a json object");
	if(list->next == nil)
		fail("$&json_addto", "missing child object");
	if(!is_json_object(list->next->term))
		fail("$&json_addto", "$2 must be a json object");

	gcref(&r_lp, (void **)&lp);
	gcref(&r_res, (void **)&res);
	gcref(&r_name, (void **)&name);
	lp = list;

	parent = getobject(list->term);
	child = getobject(list->next->term);

	type = get_json_object_type(parent);
	if(type != JTArray && type != JTObject)
		fail("$&json_addto", "parent must be an object or array");

	if(type == JTObject) {
		if(lp->next->next == nil)
			fail("$&json_addto", "$3 must have an identifier");
		name = getstr(lp->next->next->term);
	}

	newparent = add_to_json_object(parent, child, name);
	if(!newparent)
		goto fail;

	res = mklist(mkobject(newparent), nil);

fail:
	gcrderef(&r_name);
	gcrderef(&r_res);
	gcrderef(&r_lp);
	return res;
}

PRIM(json_gettype){
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	int type;
	Object *obj;

	if(list == nil)
		fail("$&json_gettype", "missing argument");
	if(!is_json_object(list->term))
		fail("$&json_gettype", "argument needs to be a json object");

	gcref(&r_lp, (void **)&lp);
	gcref(&r_res, (void **)&res);

	obj = getobject(list->term);
	type = get_json_object_type(obj);

	if(type == JTArray)
		res = mklist(mkstr(str("%d", cJSON_GetArraySize(json(obj)->data))), nil);
	res = mklist(mkstr(str("%s", int2typename(type))), res);

	gcrderef(&r_res);
	gcrderef(&r_lp);
	return res;
}

List *
get_or_detach_object(int op, List *list, Binding *bindings, int evalflags)
{
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	Object *parent = nil;
	Object *child = nil;
	int type = 0;
	int index = 0;
	char *fnname = nil;

	switch(op) {
	default:
		unreachable();
	case JOpGet:
		fnname = "$&json_getobject";
		break;
	case JOpDetach:
		fnname = "$&json_detachobject";
		break;
	}

	if(list == nil)
		fail(fnname, "missing argument");
	if(!is_json_object(list->term))
		fail(fnname, "argument needs to be a json object");

	gcref(&r_lp, (void **)&lp);
	gcref(&r_res, (void **)&res);
	lp = list;

	parent = getobject(lp->term);

	type = get_json_object_type(parent);
	if(type != JTArray && type != JTObject)
		fail(fnname, "parent must be an object or array");

	if(lp->next == nil)
		switch(type) {
		default:
			unreachable();
		case JTObject:
			fail(fnname, "missing key");
		case JTArray:
			fail(fnname, "missing index");
		}

	switch(type) {
	default:
		unreachable();
	case JTObject:
		if(op == JOpGet)
			child = get_json_object(parent, getstr(lp->next->term), 0);
		else if(op == JOpDetach)
			child = detach_json_object(parent, getstr(lp->next->term), 0);
		else
			unreachable();
		break;
	case JTArray:
		if((index = (int)termtoint(lp->next->term, fnname, 2)) < 0)
			fail(fnname, "invalid index");
		if(op == JOpGet)
			child = get_json_object(parent, nil, index);
		else if(op == JOpDetach)
			child = detach_json_object(parent, nil, index);
		else
			unreachable();
		break;
	}

	if(!child)
		goto fail;

	res = mklist(mkobject(child), nil);

fail:
	gcrderef(&r_res);
	gcrderef(&r_lp);
	return res;
}

PRIM(json_getobject){
	return get_or_detach_object(JOpGet, list, binding, evalflags);
}

PRIM(json_detachobject){
	return get_or_detach_object(JOpDetach, list, binding, evalflags);
}

PRIM(json_getdata){
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	Object *obj = nil;
	int type;

	if(list == nil)
		fail("$&json_getdata", "missing argument");
	if(!is_json_object(list->term))
		fail("$&json_getdata", "argument needs to be a json object");

	gcref(&r_lp, (void **)&lp);
	gcref(&r_res, (void **)&res);
	lp = list;

	obj = getobject(lp->term);
	type = get_json_object_type(obj);

	if(type == JTObject || type == JTArray)
		fail("$&json_getdata", "type must not be object or array");

	switch(type) {
	default:
		unreachable();
	case JTString:
		res = mklist(mkstr(str("%s", cJSON_GetStringValue(json(obj)->data))), nil);
		break;
	case JTNumber:
		res = floattolist(cJSON_GetNumberValue(json(obj)->data), "$&json_getdata");
		break;
	case JTNull:
		res = nil;
		break;
	case JTBoolean:
		if(cJSON_IsTrue(json(obj)->data))
			res = list_true;
		else
			res = list_false;
		break;
	}

	gcrderef(&r_res);
	gcrderef(&r_lp);
	return res;
}

List *
cJSON_WalkObjectForNames(cJSON *children)
{
	if(!children)
		return nil;

	return mklist(mkstr(str("%s", children->string)), cJSON_WalkObjectForNames(children->next));
}

List *
cJSON_GetObjectItemNames(cJSON *object)
{
	List *res = nil; Root r_res;

	assert(object);

	if(!cJSON_IsObject(object))
		return nil;

	gcref(&r_res, (void **)&res);
	gc();
	gcdisable();

	res = cJSON_WalkObjectForNames(object->child);

	gcenable();
	gc();
	gcrderef(&r_res);

	return res;
}

PRIM(json_getobjectnames) {
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	Object *obj;

	if(list == nil)
		fail("$&json_getobjectnames", "missing argument");
	if(!is_json_object(list->term))
		fail("$&json_getobjectnames", "argument needs to be a json object");
	if(get_json_object_type(getobject(list->term)) != JTObject)
		fail("$&json_getobjectnames", "object must be a json object");

	gcref(&r_lp, (void **)&lp);
	gcref(&r_res, (void **)&res);

	lp = list;
	obj = getobject(lp->term);

	res = cJSON_GetObjectItemNames(json(obj)->data);

	gcrderef(&r_res);
	gcrderef(&r_lp);
	return res;
}

MODULE(mod_json) {
	DX(json_dumpobject),   DX(json_decode),	 DX(json_encode),		  DX(json_encode_formatted),
	DX(json_create),	   DX(json_addto),	 DX(json_gettype),		  DX(json_getobject),
	DX(json_detachobject), DX(json_getdata), DX(json_getobjectnames),
	PRIMSEND,
} ENDMODULE(mod_json, &json_onload, &json_onunload)
