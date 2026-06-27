#include "es.h"
#include "prim.h"

PRIM(extractbinding){
	List *result = nil;
	Dict *d = nil;
	Closure *fn = nil;
	Binding *cbind = nil;

	if(!list)
		fail("$&extractbinding", "missing argument");

	ref(list);
	ref(result);
	ref(d);
	ref(fn);
	ref(cbind);

	if((fn = getclosure(list->term)) == nil)
		fail("$&extractbinding", "argument must be a closure");

	d = mkdict();

	for(cbind = fn->binding; cbind != nil; cbind = cbind->next)
		d = dictput(d, cbind->name, cbind->defn);

	result = mklist(mkdictterm(d), nil);

	deref(cbind);
	deref(fn);
	deref(d);
	deref(result);
	deref(list);

	return result;
}

PRIM(linkbinding){
	List *result = nil;
	Closure *fn1 = nil;
	Closure *fn2 = nil;

	if(!list)
		fail("$&linkbinding", "missing argument $1");
	if(!list->next)
		fail("$&linkbinding", "missing argument $2");

	ref(list);
	ref(result);

	if((fn1 = getclosure(list->term)) == nil)
		fail("$&linkbinding", "$1 must be a closure");
	if((fn2 = getclosure(list->next->term)) == nil)
		fail("$&linkbinding", "$2 must be a closure");

	fn2->binding = fn1->binding;
	result = mklist(mkterm(nil, fn2), nil);

	deref(result);
	deref(list);

	return result;
}

PRIM(clonebinding){
	List *result = nil;
	Closure *fn = nil;
	Binding *newbind = nil;

	if(!list)
		fail("$&clonebinding", "missing argument");

	ref(list);
	ref(result);
	ref(fn);
	ref(newbind);

	if((fn = getclosure(list->term)) == nil)
		fail("$&clonebinding", "argument must be a closure");

	newbind = clonebindings(fn->binding);
	fn->binding = newbind;
	result = mklist(mkterm(nil, fn), nil);

	deref(newbind);
	deref(fn);
	deref(result);
	deref(list);

	return result;
}

MODULE(mod_macro, nil, nil,
	DX(extractbinding),
	DX(linkbinding),
	DX(clonebinding),
);

