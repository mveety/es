/* this helps manage non-gc allocations from modules */

#include "es.h"
#include "prim.h"
#include "gc.h"
#include <string.h>

typedef struct {
	char *name;
	int (*deallocate)(Object *); /* called before an object is freed */
	int (*refdeps)(Object *);
} Typedef;

size_t typessz = 0;
Typedef **typedefs = nil;
size_t objectssz = 0;
Object **objects = nil;

void
init_typedefs(void)
{
	assert(typessz == 0);
	assert(typedefs == nil);

	typessz = 4;
	typedefs = ealloc(sizeof(Typedef *) * typessz);
}

void
grow_typedefs(void)
{
	size_t newsize = 0;
	Typedef **oldtypedefs = nil;

	assert(typedefs);

	newsize = typessz * 2;
	oldtypedefs = typedefs;
	typedefs = ealloc(sizeof(Typedef *) * newsize);
	memcpy(typedefs, oldtypedefs, typessz * sizeof(Typedef *));
	free(oldtypedefs);
	typessz = newsize;
}

void
init_objects(void)
{
	assert(objectssz == 0);
	assert(objects == nil);

	objectssz = 4;
	objects = ealloc(sizeof(Object *) * objectssz);
}

void
grow_objects(void)
{
	size_t newsize = 0;
	Object **oldobjects = nil;

	assert(objects);

	newsize = objectssz * 2;
	oldobjects = objects;
	objects = ealloc(sizeof(Object *) * newsize);
	memcpy(objects, oldobjects, objectssz * sizeof(Object *));
	free(oldobjects);
	objectssz = newsize;
}

int32_t
gettypeindex(char *name)
{
	int32_t i;

	assert(name);
	for(i = 0; i < (int32_t)typessz; i++) {
		if(typedefs[i] == nil)
			continue;
		if(streq(typedefs[i]->name, name))
			return i;
	}

	return -1;
}

char *
gettypename(int32_t index)
{
	if(index >= (int32_t)typessz)
		unreachable();
	if(typedefs[index] == nil)
		unreachable();

	return typedefs[index]->name;
}

int32_t
define_type(char *name, int (*deallocate)(Object *), int (*refdeps)(Object *))
{
	Typedef *newtype;
	int32_t typen;
	int32_t i;

	if((typen = gettypeindex(name)) >= 0)
		return typen;

	newtype = ealloc(sizeof(Typedef));

	newtype->name = strdup(name);
	newtype->deallocate = deallocate;
	newtype->refdeps = refdeps;

	for(i = 0; i < (int32_t)typessz; i++)
		if(typedefs[i] == nil) {
			typedefs[i] = newtype;
			return i;
		}

	grow_typedefs();

	for(i = 0; i < (int32_t)typessz; i++)
		if(typedefs[i] == nil) {
			typedefs[i] = newtype;
			return i;
		}

	unreachable();
}

void
undefine_type(char *name)
{
	int32_t index;
	Typedef *oldtype;

	if((index = gettypeindex(name)) < 0)
		return;

	assert(typedefs[index]);

	oldtype = typedefs[index];
	typedefs[index] = nil;
	free(oldtype->name);
	free(oldtype);
}

Object *
allocate_object(char *type, size_t size)
{
	int32_t typeidx;
	Object *newobject;
	int32_t i;

	if((typeidx = gettypeindex(type)) < 0)
		unreachable();

	newobject = ealloc(sizeof(Object) + size);
	newobject->type = typeidx;
	newobject->size = size;
	newobject->data = ((char *)newobject) + sizeof(Object);

	for(i = 0; i < (int32_t)objectssz; i++)
		if(objects[i] == nil) {
			objects[i] = newobject;
			newobject->id = i;
			return newobject;
		}

	grow_objects();

	for(i = 0; i < (int32_t)objectssz; i++)
		if(objects[i] == nil) {
			objects[i] = newobject;
			newobject->id = i;
			return newobject;
		}

	unreachable();
}

void
deallocate_object(Object *obj)
{
	Typedef *objtype;

	assert(obj);
	assert(obj->type < (int32_t)typessz);
	assert(objects[obj->id] != nil);
	assert(objects[obj->id] == obj);

	objtype = typedefs[obj->type];

	if(objtype->deallocate && obj->sysflags & ObjectInitialized)
		if(objtype->deallocate(obj) != 0)
			return;
	objects[obj->id] = nil;
	free(obj);
}

void
assert_type(Object *obj, char *name)
{
	int32_t typeindex;

	typeindex = gettypeindex(name);
	assert(typeindex >= 0);
	assert(obj);
	assert(obj->type == typeindex);
}

int
object_is_type(Object *obj, char *name)
{
	int32_t typeindex;

	typeindex = gettypeindex(name);
	assert(typeindex >= 0);

	if(obj == nil)
		return 0;
	if(obj->type != typeindex)
		return 0;
	return 1;
}

void
refobject(Object *obj)
{
	Typedef *objtype;

	assert(obj);
	obj->refcount++;
	objtype = typedefs[obj->type];
	if(objtype->refdeps)
		objtype->refdeps(obj);
}

void
derefobject(Object *obj)
{
	assert(obj);
	if(obj->refcount > 0)
		obj->refcount--;
}

int64_t
object_refs(Object *obj)
{
	assert(obj);
	return obj->refcount;
}

/* to gc objects:
 * 1. call derefallobjects
 * 2. when the marker/sweeper hits an object term the object is ref'd
 * 3. call dealloc_unrefed_objects
 */

void
derefallobjects(void)
{
	int32_t i;

	for(i = 0; i < (int32_t)objectssz; i++)
		if(objects[i] != nil && objects[i]->sysflags & ObjectGcManaged)
			objects[i]->refcount = 0;
}

void
dealloc_unrefed_objects(void)
{
	int32_t i;

	for(i = 0; i < (int32_t)objectssz; i++)
		if(objects[i] != nil && objects[i]->refcount == 0 &&
		   objects[i]->sysflags & (ObjectFreeWhenNoRefs | ObjectGcManaged))
			deallocate_object(objects[i]);
}

void
gcmanageobj(Object *obj)
{
	assert(obj);
	assert(obj->type < (int32_t)typessz);
	assert(objects[obj->id] != nil);
	assert(objects[obj->id] == obj);

	obj->sysflags |= ObjectGcManaged;
	obj->sysflags |= ObjectFreeWhenNoRefs;
}

void
gcunmanageobj(Object *obj)
{
	assert(obj);
	assert(obj->type < (int32_t)typessz);
	assert(objects[obj->id] != nil);
	assert(objects[obj->id] == obj);

	obj->sysflags &= ~ObjectGcManaged;
	obj->sysflags &= ~ObjectFreeWhenNoRefs;
}
