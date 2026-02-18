/* prim.c -- primitives and primitive dispatching ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "prim.h"

static Dict *prims;
int primassert = 0;
int dumpprims = 0;

typedef struct {
	char *searched_prim;
	int i;
} dumpprim_arg;

static void
dumpprim(void *v, char *key, void *value)
{
	dumpprim_arg *arg;

	arg = v;
	if(streq(key, arg->searched_prim))
		dprintf(2, "-> ");
	dprintf(2, "%d:%s(%p)\n", arg->i, key, value);
	arg->i++;
}

extern List *
prim(char *s, List *list, Binding *binding, int evalflags)
{
	List *(*p)(List *, Binding *, int);
	dumpprim_arg arg = {.searched_prim = s, .i = 0};

	p = (List * (*)(List *, Binding *, int)) dictget(prims, s);
	if(p == NULL){
		if(primassert){
			if(dumpprims){
				dprintf(2, "primitives: ");
				dictforall(prims, &dumpprim, &arg);
				dprintf(2, "\n%d primitives\n", arg.i);
			}
			panic("unknown primitive: %s", s);
		}
		fail("es:prim", "unknown primitive: %s", s);
	} else
		return (*p)(list, binding, evalflags);
}

extern Dict *
primitives(void)
{
	return prims;
}

PRIM(primitives) {
	List *primlist = nil; Root r_primlist;

	gcref(&r_primlist, (void **)&primlist);
	dictforall(prims, addtolist, &primlist);
	primlist = sortlist(primlist);
	gcrderef(&r_primlist);

	return primlist;
}

Primitive prim_prim[] = {
	DX(primitives),
};

extern void
initprims(void)
{
	size_t i = 0;

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

	for(i = 0; i < nelem(prim_prim); i++)
		prims = dictput(prims, prim_prim[i].name, (void *)prim_prim[i].symbol);
}

void
add_prim(char *name, List *(*primfn)(List *, Binding *, int))
{
	char *gcname = nil;

	if(name == nil || primfn == nil)
		return;

	gcdisable();

	gcname = str("%s", name);
	prims = dictput(prims, gcname, primfn);

	gcenable();
}

void
remove_prim(char *name)
{
	gcdisable();
	prims = dictput(prims, name, nil);
	gcenable();
}
