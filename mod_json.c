#include "es.h"
#include "prim.h"
#include "stdenv.h"
#include <cjson/cJSON.h>
#include <cjson/cJSON_Utils.h>
LIBRARY(mod_json);

enum {
	JsonChild = 1 << 0,

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

double
termtof(Term *term, char *funname, int arg)
{
	char *str = nil;
	char *endptr = nil;
	double res;

	errno = 0;

	str = getstr(term);
	res = strtod(str, &endptr);
	if(res == 0 && str == endptr)
		fail(funname, "invalid input: $%d = %s", arg, str);
	if(res == 0 && errno == ERANGE)
		fail(funname, "conversion overflow: $%d = %s", arg, str);

	return res;
}

int64_t
termtoint(Term *term, char *funname, int arg)
{
	int64_t res;

	errno = 0;
	res = strtoll(getstr(term), NULL, 10);
	if(res == 0) {
		switch(errno) {
		case EINVAL:
			fail(funname, str("invalid input: $%d = '%s'", arg, getstr(term)));
			break;
		case ERANGE:
			fail(funname, str("conversion overflow: $%d = '%s'", arg, getstr(term)));
			break;
		}
	}

	return res;
}

List *
floattolist(double num, char *funname)
{
	char temp[256];
	int printlen;
	List *result = nil; Root r_result;

	gcref(&r_result, (void **)&result);

	printlen = snprintf(&temp[0], sizeof(temp), "%g", num);
	if(printlen >= (int)sizeof(temp))
		fail(funname, "result conversion failed (temp string too short)");
	result = mklist(mkstr(str("%s", temp)), nil);

	gcrderef(&r_result);

	return result;
}

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
create_json_object(int type, char *string, double number, Boolean bool)
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
	unreachable();
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

int
dyn_onload(void)
{
	if(define_type("json", &json_deallocate, &json_refdeps) < 0)
		return -1;
	return 0;
}

int
dyn_onunload(void)
{
	undefine_type("json");
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
	free(str);

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
	free(str);

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

	if(cJSON_IsObject(json(parent)->data)) {
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

PRIM(json_getobject){
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	Object *parent;
	Object *child;
	int type = 0;
	int index = 0;

	if(list == nil)
		fail("$&json_getdata", "missing argument");
	if(!is_json_object(list->term))
		fail("$&json_getdata", "argument needs to be a json object");

	gcref(&r_lp, (void **)&lp);
	gcref(&r_res, (void **)&res);
	lp = list;

	parent = getobject(lp->term);

	type = get_json_object_type(parent);
	if(type != JTArray && type != JTObject)
		fail("$&json_getobject", "must be an object or array");

	if(lp->next == nil)
		switch(type) {
		default:
			unreachable();
		case JTObject:
			fail("$&json_getobject", "missing key");
		case JTArray:
			fail("$&json_getobject", "missing index");
		}

	switch(type) {
	default:
		unreachable();
	case JTObject:
		child = get_json_object(parent, getstr(lp->next->term), 0);
		break;
	case JTArray:
		if((index = (int)termtoint(lp->next->term, "$&json_getobject", 2)) < 0)
			fail("$&json_getobject", "invalid index");
		child = get_json_object(parent, nil, index);
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

PRIM(json_getdata){
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;
	Object *obj;
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

DYNPRIMS() = {
	DX(json_dumpobject),
	DX(json_decode),
	DX(json_encode),
	DX(json_encode_formatted),
	DX(json_create),
	DX(json_addto),
	DX(json_gettype),
	DX(json_getobject),
	DX(json_getdata),

	PRIMSEND,
};
