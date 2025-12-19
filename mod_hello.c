#include "es.h"
#include "prim.h"

int
hello_deallocate(Object *object)
{
	used(object);

	// dprintf(2, "hellotype's deallocate was called!\n");
	return 0;
}

int
hello_onfork(Object *object)
{
	used(object);

	// dprintf(2, "hellotype's onfork callback was called!\n");
	return 0;
}

char *
teststring_stringify(Object *obj)
{
	return (char *)obj->data;
}

Result
teststring_objectify(char *str)
{
	Object *newobj;
	size_t strsize;

	strsize = strlen(str);
	newobj = allocate_object("teststring", strsize + 5);
	strcpy(newobj->data, str);
	newobj->sysflags |= ObjectInitialized;
	newobj->sysflags |= ObjectGcManaged;
	newobj->sysflags |= ObjectFreeWhenNoRefs;

	return result_obj(newobj, ObjectifyOk);
}

int
hello_onload(void)
{
	dprintf(2, "mod_hello's dyn_onload was called!\n");
	if(define_type("hellotype", &hello_deallocate, nil) < 0) {
		dprintf(2, "unable to define hellotype!\n");
		return -1;
	}
	if(define_type("teststring", nil, nil) < 0) {
		dprintf(2, "unable to define teststring");
		return -1;
	}
	define_stringifier("teststring", teststring_stringify);
	define_objectifier("teststring", teststring_objectify);
	define_onfork_callback("hellotype", &hello_onfork);
	return 0;
}

int
hello_onunload(void)
{
	dprintf(2, "mod_hello's dyn_onunload was called!\n");
	undefine_type("hellotype");
	return 0;
}

List *
hellotest(List *list, Binding *binding, int evalflags)
{
	used(list);
	used(binding);
	used(&evalflags);

	dprintf(1, "hello from mod_hello.so!\n");

	return nil;
}

PRIM(make_helloobject){
	Object *helloobject = nil;

	helloobject = allocate_object("hellotype", sizeof(int64_t));
	if(!helloobject)
		fail("$&make_helloobject", "unable to make a hellotype object");

	return mklist(mkobject(helloobject), nil);
}

PRIM(object_gcmanage) {
	Object *obj;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&object_gcmanage", "missing argument");
	if(list->term->kind != tkObject)
		fail("$&object_gcmanage", "argument must be an object");

	gcref(&r_result, (void **)&result);
	obj = getobject(list->term);
	refobject(obj);
	obj->sysflags |= ObjectGcManaged;
	result = mklist(mkobject(obj), nil);
	gcrderef(&r_result);
	derefobject(obj);
	return result;
}

PRIM(object_freeable) {
	Object *obj;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&object_freeable", "missing argument");

	gcref(&r_result, (void **)&result);
	if((obj = getobject(list->term)) == nil)
		fail("$&object_freeable", "argument must be an object");
	obj->sysflags |= ObjectFreeWhenNoRefs;
	result = mklist(mkobject(obj), nil);
	gcrderef(&r_result);
	return result;
}

PRIM(object_initialize) {
	Object *obj;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&object_gcmanage", "missing argument");
	if(list->term->kind != tkObject)
		fail("$&object_gcmanage", "argument must be an object");

	gcref(&r_result, (void **)&result);
	obj = getobject(list->term);
	refobject(obj);
	obj->sysflags |= ObjectInitialized;
	result = mklist(mkobject(obj), nil);
	gcrderef(&r_result);
	derefobject(obj);
	return result;
}

PRIM(object_closeonfork){
	Object *obj = nil;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&object_closeonfork", "missing argument");
	if(list->term->kind != tkObject)
		fail("$&object_closeonfork", "argument must be an object");

	gcref(&r_result, (void **)&result);
	obj = getobject(list->term);
	refobject(obj);
	obj->sysflags |= ObjectCloseOnFork;
	result = mklist(mkobject(obj), nil);
	gcrderef(&r_result);
	derefobject(obj);
	return result;
}

PRIM(object_onforkcallback){
	Object *obj = nil;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&object_onforkcallback", "missing argument");
	if(list->term->kind != tkObject)
		fail("$&object_onforkcallback", "argument must be an object");

	gcref(&r_result, (void **)&result);
	obj = getobject(list->term);
	refobject(obj);
	obj->sysflags |= ObjectCallbackOnFork;
	result = mklist(mkobject(obj), nil);
	gcrderef(&r_result);
	derefobject(obj);
	return result;
}

PRIM(dump_bindings) {
	Binding *bp = nil; Root r_bp;

	gcref(&r_bp, (void **)&bp);

	if(binding == nil)
		eprint("no bindings\n");
	else
		for(bp = binding; bp != nil; bp = bp->next)
			eprint("binding %s = %V\n", bp->name, bp->defn, " ");

	gcrderef(&r_bp);

	return list_true;
}

MODULE(mod_hello)
	{"hellotest", &hellotest}, DX(make_helloobject),  DX(object_gcmanage),
	DX(object_freeable),	   DX(object_initialize), DX(object_closeonfork),
	DX(object_onforkcallback), DX(dump_bindings),
	PRIMSEND,
ENDMODULE(mod_hello, &hello_onload, &hello_onunload);
