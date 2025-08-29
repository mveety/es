#include "es.h"
#include "prim.h"

PRIM(dictnew) {
	return mklist(mkdictterm(NULL), NULL);
}

PRIM(dictget) {
	List *res = NULL; Root r_res;
	Dict *d = NULL; Root r_d;
	char *name;

	if(!list || !list->next)
		fail("$&dictget", "missing arguments");

	res = NULL;
	gcref(&r_res, (void**)&res);

	d = getdict(list->term);
	if(!d)
		fail("$&dictget", "term not valid dict");
	gcref(&r_d, (void**)&d);
	name = getstr(list->next->term);

	res = dictget(d, name);
	if(!res)
		fail("$&dictget", "element '%s' is empty", name);

	gcrderef(&r_d);
	gcrderef(&r_res);
	return res;
}

PRIM(dictput) {
	Dict *d = NULL; Root r_d;
	char *name;
	List *v = NULL; Root r_v;

	if(!list || !list->next || !list->next->next)
		fail("$&dictput", "missing arguments");

	d = getdict(list->term);
	if(!d)
		fail("$&dictput", "term not valid dict");
	gcref(&r_d, (void**)&d);

	name = getstr(list->next->term);
	v = list->next->next;
	gcref(&r_v, (void**)&v);

d = dictput(d, name, v);

	gcrderef(&r_v);
	gcrderef(&r_d);
	return mklist(mkdictterm(d), NULL);
}

PRIM(dictremove) {
	Dict *d = NULL; Root r_d;
	char *name;

	if(!list || !list->next)
		fail("$&dictremove", "missing arguments");

	d = getdict(list->term);
	if(!d)
		fail("$&dictremove", "term not valid dict");
	gcref(&r_d, (void**)&d);

	name = getstr(list->next->term);

	d = dictput(d, name, NULL);

	gcrderef(&r_d);
	return mklist(mkdictterm(d), NULL);
}

typedef struct {
	Term *function;
	Binding *binding;
	int evalflags;
} DictForAllArgs;

void
dicteval(void *vdfaargs, char *name, void *vdata)
{
	DictForAllArgs *dfaargs;
	List *data = NULL; Root r_data;
	List *res = NULL; Root r_res;
	List *args = NULL; Root r_args;

	gcref(&r_data, (void**)&data);
	gcref(&r_args, (void**)&args);
	gcref(&r_res, (void**)&res);

	dfaargs = vdfaargs;
	data = vdata;
	args = mklist(mkstr(name), data);

	ExceptionHandler

		res = prim("noreturn", mklist(dfaargs->function, args), dfaargs->binding, dfaargs->evalflags);

	CatchException (e)

		if (!termeq(e->term, "continue"))
			throw(e);

	EndExceptionHandler

	gcrderef(&r_res);
	gcrderef(&r_args);
	gcrderef(&r_data);
}

void
dictsize(void *vsz, char *name, void *vdata)
{
	unsigned int *sz;

	sz = vsz;
	*sz = *sz + 1;
}

PRIM(dictforall) {
	DictForAllArgs args = {NULL, NULL, 0};
	List *lp = NULL; Root r_lp;
	Dict *d = NULL; Root r_dict;
	Root r_function;
	Root r_binding;

	if(!list || !list->next)
		fail("$&dictforall", "missing arguments");

	gcdisable();
	lp = list;
	gcref(&r_lp, (void**)&lp);
	d = getdict(lp->term);
	if(!d)
		fail("$&dictforall", "term not valid dict");
	gcref(&r_dict, (void**)&d);
	lp = lp->next;

	gcref(&r_binding, (void**)&args.binding);
	gcref(&r_function, (void**)&args.function);
	args = (DictForAllArgs){
		.function = lp->term,
		.evalflags = evalflags,
		.binding = binding,
	};
	gcenable();

	ExceptionHandler

		dictforall(d, &dicteval, &args);

	CatchException (e)

		if(!termeq(e->term, "break"))
			throw(e);

	EndExceptionHandler

	/* varpop(&dfafn); */
	gcrderef(&r_function);
	gcrderef(&r_binding);
	gcrderef(&r_dict);
	gcrderef(&r_lp);

	return list_true;
}

PRIM(dictsize) {
	List *lp = NULL; Root r_lp;
	Dict *d = NULL; Root r_d;
	unsigned int sz = 0;

	if(!list)
		fail("$&dictsize", "missing argument");

	lp = list;
	gcref(&r_lp, (void**)&lp);
	d = getdict(lp->term);
	if(!d)
		fail("$&dictforall", "term not valid dict");
	gcref(&r_d, (void**)&d);

	dictforall(d, &dictsize, &sz);

	gcrderef(&r_d);
	gcrderef(&r_lp);

	return mklist(mkstr(str("%ud", sz)), NULL);
}

PRIM(termtypeof) {
	if(!list)
		fail("$&termtypeof", "missing argument");

	switch(list->term->kind){
	case tkString:
		return mklist(mkstr(str("string")), NULL);
	case tkClosure:
		return mklist(mkstr(str("closure")), NULL);
	case tkDict:
		return mklist(mkstr(str("dict")), NULL);
	default:
		fail("$&termtypeof", "invalid term type!");
		break;
	}
	return list_false;
}

Dict*
initprims_dict(Dict *primdict)
{
	X(dictnew);
	X(dictget);
	X(dictput);
	X(dictremove);
	X(dictforall);
	X(dictsize);
	X(termtypeof);
	return primdict;
}


