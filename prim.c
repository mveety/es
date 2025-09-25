/* prim.c -- primitives and primitive dispatching ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "prim.h"

static Dict *prims;

extern List *
prim(char *s, List *list, Binding *binding, int evalflags)
{
	List *(*p)(List *, Binding *, int);
	p = (List * (*)(List *, Binding *, int)) dictget(prims, s);
	if(p == NULL)
		fail("es:prim", "unknown primitive: %s", s);
	else
		return (*p)(list, binding, evalflags);
}

PRIM(primitives) {
	List *primlist = nil; Root r_primlist;

	gcref(&r_primlist, (void **)&primlist);
	dictforall(prims, addtolist, &primlist);
	primlist = sortlist(primlist);
	gcrderef(&r_primlist);

	return primlist;
}

extern void
initprims(void)
{
	prims = mkdict();
	globalroot(&prims);

	prims = initprims_controlflow(prims);
	prims = initprims_io(prims);
	prims = initprims_etc(prims);
	prims = initprims_sys(prims);
	prims = initprims_proc(prims);
	prims = initprims_access(prims);
	prims = initprims_math(prims);
	prims = initprims_mv(prims);
	prims = initprims_dict(prims);
#ifdef DYNAMIC_LIBRARIES
	prims = initprims_dynlib(prims);
#endif

#define primdict prims
	X(primitives);
}

void
add_prim(char *name, List *(*primfn)(List *, Binding *, int))
{
	char *gcname = nil; Root r_gcname;

	gcref(&r_gcname, (void **)&gcname);
	gcdisable();

	gcname = str("%s", name);
	prims = dictput(prims, gcname, primfn);

	gcenable();
	gcrderef(&r_gcname);
}

void
remove_prim(char *name)
{
	gcdisable();
	prims = dictput(prims, name, nil);
	gcenable();
}
