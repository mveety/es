/* this helps manage non-gc allocations from modules */

#include "es.h"
#include "prim.h"
#include "gc.h"
#include <stdint.h>
#include <string.h>

typedef struct {
	char *name;
	int (*deallocate)(Object *); /* called before an object is freed */
	int (*refdeps)(Object *);
	char *(*stringify)(Object *);
	int (*onfork)(Object *);
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
	newtype->stringify = nil;

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

int
define_stringifier(char *name, char *(*stringify)(Object *))
{
	int index;
	Typedef *type;

	assert(name);

	assert((index = gettypeindex(name)) >= 0);
	type = typedefs[index];

	type->stringify = stringify;
	return 0;
}

int
define_onfork_callback(char *name, int (*onfork)(Object *))
{
	int index;
	Typedef *type;

	assert(name);

	assert((index = gettypeindex(name)) >= 0);
	type = typedefs[index];

	type->onfork = onfork;
	return 0;
}
int
undefine_type(char *name)
{
	int32_t index;
	Typedef *oldtype;
	size_t i = 0;

	if((index = gettypeindex(name)) < 0)
		return 0;

	assert(typedefs[index]);

	for(i = 0; i < objectssz; i++) {
		if(!objects[i])
			continue;
		if(objects[i]->type == index)
			return ObjectErrorTypeInUse;
	}

	oldtype = typedefs[index];
	typedefs[index] = nil;
	free(oldtype->name);
	free(oldtype);

	return 0;
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

	assert(objtype = typedefs[obj->type]);

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

char *
stringify(Object *obj)
{
	Typedef *objtype;

	assert(obj);
	assert(obj->type < (int32_t)typessz);
	assert(objects[obj->id] != nil);
	assert(objects[obj->id] == obj);

	assert(objtype = typedefs[obj->type]);

	if(objtype->stringify)
		return objtype->stringify(obj);
	return nil;
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

void
deallocate_all_objects(void)
{
	int32_t i = 0;
	Typedef *objtype = nil;

	if(objects == nil)
		return;
	for(i = 0; i < (int32_t)objectssz; i++, objtype = nil) {
		if(!objects[i])
			continue;

		assert(objects[i]->type < (int32_t)typessz);
		assert(objtype = typedefs[objects[i]->type]);

		if(objtype->deallocate && objects[i]->sysflags & ObjectInitialized)
			objtype->deallocate(objects[i]);
		free(objects[i]);
		objects[i] = nil;
	}
}

void
all_objects_onfork_ops(void)
{
	int32_t i = 0;
	Typedef *objtype = nil;

	if(objects == nil)
		return;

	for(i = 0; i < (int32_t)objectssz; i++, objtype = nil) {
		if(!objects[i])
			continue;

		assert(objects[i]->type < (int32_t)typessz);
		assert(objtype = typedefs[objects[i]->type]);

		if(objects[i]->sysflags & (ObjectInitialized | ObjectCallbackOnFork)){
			if(objtype->onfork)
				if(objtype->onfork(objects[i]))
					dprintf(2, "es:objects: %s onfork callback failed", objtype->name);
		}

		if(objects[i]->sysflags & (ObjectInitialized | ObjectCloseOnFork)) {
			if(objtype->deallocate)
				objtype->deallocate(objects[i]);
			free(objects[i]);
			objects[i] = nil;
		}
	}
}
