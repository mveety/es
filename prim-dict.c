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

typedef struct {
	Term *fn;
	Binding *binding;
	int evalflags;
} DictForAllArgs;

void
dicteval(void *vdfaargs, char *name, void *vdata)
{
	DictForAllArgs *dfaargs;
	Term *fn = NULL; Root r_fn;
	List *data = NULL; Root r_data;
	List *res = NULL; Root r_res;
	List *args = NULL; Root r_args;

	dfaargs = vdfaargs;
	fn = dfaargs->fn;
	gcref(&r_fn, (void**)&fn);
	data = vdata;
	gcref(&r_data, (void**)&data);
	args = NULL;
	gcref(&r_args, (void**)&args);
	res = NULL;
	gcref(&r_res, (void**)&res);

	args = mklist(fn, mklist(mkstr(name), data));

	res = eval(args, dfaargs->binding, dfaargs->evalflags & ~run_interactive);

	gcrderef(&r_res);
	gcrderef(&r_args);
	gcrderef(&r_data);
	gcrderef(&r_fn);
}

PRIM(dictforall) {
	DictForAllArgs args;
	Term *fn = NULL; Root r_fn;
	Dict *d = NULL; Root r_dict;

	if(!list || !list->next)
		fail("$&dictforall", "missing arguments");

	d = getdict(list->term);
	if(!d)
		fail("$&dictforall", "term not valid dict");
	gcref(&r_dict, (void**)&d);
	fn = list->next->term;
	gcref(&r_fn, (void**)&fn);

	args = (DictForAllArgs){
		.fn = fn,
		.evalflags = (evalflags & eval_exitonfalse),
		.binding = binding,
	};

	dictforall(d, &dicteval, &args);

	gcrderef(&r_fn);
	gcrderef(&r_dict);

	return list_true;
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
	X(dictforall);
	X(termtypeof);
	return primdict;
}


