#include "es.h"
#include "prim.h"
LIBRARY(mod_objutil);

/* from objects.c */
/* these aren't normally used but are needed for this mod */
typedef struct {
	char *name;
	int (*deallocate)(Object *); /* called before an object is freed */
	int (*refdeps)(Object *);
} Typedef;

extern size_t typessz;
extern Typedef **typedefs;
extern size_t objectssz;
extern Object **objects;

PRIM(objects_dumptypes) {
	List *result = nil; Root r_result;
	size_t i = 0;

	gcref(&r_result, (void **)&result);

	for(i = 0; i < typessz; i++) {
		if(typedefs[i] == nil)
			continue;
		result = mklist(mkstr(str("type %d: %s", (int32_t)i, typedefs[i]->name)), result);
	}
	result = reverse(result);

	gcrderef(&r_result);
	return result;
}

PRIM(printobjects) {
	size_t i;
	char *typename = nil;

	for(i = 0; i < objectssz; i++) {
		if(objects[i] == nil)
			continue;
		typename = gettypename(objects[i]->type);
		dprintf(2, "object %lu: type=%s(%d), id=%d, sysflags=%x, size=%lu, refs=%lu\n", i, typename,
				objects[i]->type, objects[i]->id, objects[i]->sysflags, objects[i]->size,
				objects[i]->refcount);
		typename = nil;
	}

	return nil;
}

PRIM(printobjectstats) {
	size_t nobjs = 0;
	size_t nobjssz = 0;
	size_t nobjssz_total = 0;
	size_t ntypes = 0;
	size_t i;

	for(i = 0; i < typessz; i++){
		if(typedefs[i])
			ntypes++;
	}

	for(i = 0; i < objectssz; i++){
		if(objects[i]){
			nobjs++;
			nobjssz += objects[i]->size;
		}
	}
	nobjssz_total = nobjssz + (nobjs*sizeof(Object));

	dprintf(2, "%lu objects in %lu slots using %lu bytes (%lu total)\n",
			nobjs, objectssz, nobjssz, nobjssz_total);
	dprintf(2, "%lu types in %lu slots\n", ntypes, typessz);

	return nil;
}

DYNPRIMS() = {
	DX(objects_dumptypes),
	DX(printobjects),
	DX(printobjectstats),
	PRIMSEND,
};

