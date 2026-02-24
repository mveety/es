#include "es.h"
#include "prim.h"
#include <sys/stat.h>

/* from objects.c */
/* these aren't normally used but are needed for this mod */
typedef struct {
	char *name;
	int (*deallocate)(Object *); /* called before an object is freed */
	int (*refdeps)(Object *);
	char *(*stringify)(Object *);
	int (*onfork)(Object *);
	Result (*objectify)(char *);
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

PRIM(printtypes) {
	size_t i;
	int nocallbacks = 1;

	for(i = 0; i < typessz; i++, nocallbacks = 1) {
		if(typedefs[i] == nil)
			continue;
		dprintf(2, "type %lu: %s: ", i, typedefs[i]->name);
		if(typedefs[i]->deallocate) {
			dprintf(2, "deallocate ");
			nocallbacks = 0;
		}
		if(typedefs[i]->refdeps) {
			dprintf(2, "refdeps ");
			nocallbacks = 0;
		}
		if(typedefs[i]->stringify) {
			dprintf(2, "stringify ");
			nocallbacks = 0;
		}
		if(typedefs[i]->onfork) {
			dprintf(2, "onfork ");
			nocallbacks = 0;
		}
		if(typedefs[i]->objectify) {
			dprintf(2, "objectify");
			nocallbacks = 0;
		}
		if(nocallbacks)
			dprintf(2, "no callbacks");
		dprintf(2, "\n");
	}

	return nil;
}

PRIM(printobjects) {
	size_t i;
	Typedef *objtype;
	int nocallbacks = 1;

	for(i = 0; i < objectssz; i++, objtype = nil) {
		if(objects[i] == nil)
			continue;
		objtype = typedefs[objects[i]->type];
		dprintf(2, "object %lu: type=%s(%d), id=%d, sysflags=%x, size=%lu, refs=%lu\n", i,
				objtype->name, objects[i]->type, objects[i]->id, objects[i]->sysflags,
				objects[i]->size, objects[i]->refcount);
		dprintf(2, "    ");
		if(objtype->deallocate) {
			dprintf(2, "deallocate ");
			nocallbacks = 0;
		}
		if(objtype->refdeps) {
			dprintf(2, "refdeps ");
			nocallbacks = 0;
		}
		if(objtype->stringify) {
			dprintf(2, "stringify ");
			nocallbacks = 0;
		}
		if(objtype->onfork) {
			dprintf(2, "onfork ");
			nocallbacks = 0;
		}
		if(objtype->objectify) {
			dprintf(2, "objectify");
			nocallbacks = 0;
		}
		if(nocallbacks)
			dprintf(2, "no callbacks");
		dprintf(2, "\n");
	}

	return nil;
}

PRIM(printobjectstats) {
	size_t nobjs = 0;
	size_t nobjssz = 0;
	size_t nobjssz_total = 0;
	size_t ntypes = 0;
	size_t i;

	for(i = 0; i < typessz; i++) {
		if(typedefs[i])
			ntypes++;
	}

	for(i = 0; i < objectssz; i++) {
		if(objects[i]) {
			nobjs++;
			nobjssz += objects[i]->size;
		}
	}
	nobjssz_total = nobjssz + (nobjs * sizeof(Object));

	dprintf(2, "%lu objects in %lu slots using %lu bytes (%lu total)\n", nobjs, objectssz, nobjssz,
			nobjssz_total);
	dprintf(2, "%lu types in %lu slots\n", ntypes, typessz);

	return nil;
}

PRIM(objectify) {
	Object *obj = nil;

	if(list == nil)
		fail("$&objectify", "missing argument");
	if((obj = getobject(list->term)) == nil)
		fail("$&objectify", "not an object");

	return mklist(mkobject(obj), nil);
}

MODULE(mod_objutil, nil, nil,
	DX(objects_dumptypes), DX(printtypes), DX(printobjects),
	DX(printobjectstats),  DX(objectify),
);
