#include "es.h"
#include "prim.h"

PRIM(dictnew) {
	return mklist(mkdictterm(NULL), NULL);
}

PRIM(dictget) {
	List *res; Root r_res;
	Dict *d; Root r_d;
	char *name;

	if(!list || !list->next)
		fail("$&dictget", "missing arguments");

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
	Dict *d; Root r_d;
	char *name;
	List *v; Root r_v;

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

Dict*
initprims_dict(Dict *primdict)
{
	X(dictnew);
	X(dictget);
	X(dictput);
	return primdict;
}


