#include "es.h"
#include "prim.h"
#include "gc.h"
#include <dlfcn.h>
#include <stdio.h>

typedef struct DynamicLibrary DynamicLibrary;

struct DynamicLibrary {
	char *fname;
	char *name;
	void *handle;
	Primitive *prims;
	size_t *primslen;
	DynamicLibrary *next;
};

DynamicLibrary *loaded_libs;
Boolean dynlib_verbose = FALSE;
Boolean dynlib_fail_cancel = TRUE;

DynamicLibrary *
create_library(char *fname, char *errstr, size_t errstrlen)
{
	DynamicLibrary *lib;
	char *dlerrstr;

	memset(errstr, 0, errstrlen);
	lib = ealloc(sizeof(DynamicLibrary));
	lib->fname = strdup(fname);
	lib->handle = dlopen(fname, RTLD_NOW);
	if(!lib->handle) {
		dlerrstr = dlerror();
		if(errstr)
			strncpy(errstr, dlerrstr, errstrlen);
		free(lib->fname);
		free(lib);
		return nil;
	}

	lib->name = dlsym(lib->handle, "dynlibname");
	lib->prims = dlsym(lib->handle, "dynprims");
	lib->primslen = dlsym(lib->handle, "dynprimslen");

	if(!lib->name || !lib->prims || !lib->primslen) {
		if(dlclose(lib->handle)) {
			dlerrstr = dlerror();
			if(errstr)
				strncpy(errstr, dlerrstr, errstrlen);
		}
		free(lib->fname);
		free(lib);
		return nil;
	}

	return lib;
}

int
destroy_library(DynamicLibrary *lib, char *errstr, size_t errstrlen)
{
	char *dlerrstr;
	int res = 0;

	res = dlclose(lib->handle);
	if(res) {
		dlerrstr = dlerror();
		strncpy(errstr, dlerrstr, errstrlen);
	}
	free(lib->fname);
	free(lib);
	return res;
}

void
dump_prims_lib(DynamicLibrary *lib)
{
	size_t i = 0;

	dprintf(2, "Library %s (%s)\n", lib->name, lib->fname);
	for(i = 0; i < *lib->primslen; i++)
		dprintf(2, "\t%lu: primitive %s\n", i, lib->prims[i].name);
}

void
dump_dynamic_prims(void)
{
	DynamicLibrary *lib;

	for(lib = loaded_libs; lib != nil; lib = lib->next)
		dump_prims_lib(lib);
}

int
load_prims_lib(DynamicLibrary *lib)
{
	size_t i = 0;
	int res = 0;

	for(i = 0; i < *lib->primslen; i++)
		add_prim(lib->prims[i].name, lib->prims[i].symbol);

	return res;
}

int
unload_prims_lib(DynamicLibrary *lib)
{
	Primitive *prim;
	size_t i = 0;

	for(i = 0; i < *lib->primslen; i++) {
		prim = &lib->prims[i];
		if(dynlib_verbose)
			dprintf(2, "unloading %s from %s...", prim->name, lib->name);
		remove_prim(prim->name);
		if(dynlib_verbose)
			dprintf(2, "done.\n");
	}

	return 0;
}

int
lib_add_to_list(DynamicLibrary *lib)
{
	lib->next = loaded_libs;
	loaded_libs = lib;
	return 0;
}

DynamicLibrary *
lib_remove_from_list(char *fname)
{
	DynamicLibrary *last = nil;
	DynamicLibrary *cur = nil;
	DynamicLibrary *next = nil;
	DynamicLibrary *result = nil;

	cur = loaded_libs;
	while(1) {
		if(strcmp(cur->fname, fname) == 0) {
			result = cur;
			if(cur == loaded_libs)
				loaded_libs = loaded_libs->next;
			if(last)
				last->next = next;
			result->next = nil;
			return result;
		}
		if(next == nil)
			return nil;
		last = cur;
		cur = next;
		next = next->next;
	}
	return nil;
}

DynamicLibrary*
lib_find_fname(char *fname)
{
	DynamicLibrary *lib;

	for(lib = loaded_libs; lib != nil; lib = lib->next)
		if(streq(lib->fname, fname))
			return lib;
	return nil;
}

DynamicLibrary*
lib_find_name(char *name)
{
	DynamicLibrary *lib;

	for(lib = loaded_libs; lib != nil; lib = lib->next)
		if(streq(lib->name, name))
			return lib;
	return nil;
}

int
open_library(char *fname, char *errstr, size_t errstrlen)
{
	DynamicLibrary *lib;

	lib = create_library(fname, errstr, errstrlen);
	if(!lib)
		return -1;

	if(lib_find_name(lib->name) != nil){
		snprintf(errstr, errstrlen, "library %s already loaded", lib->name);
		destroy_library(lib, nil, 0);
		return -1;
	}

	if(dynlib_verbose)
		dump_prims_lib(lib);

	lib_add_to_list(lib);
	return load_prims_lib(lib);
}

int
close_library(char *fname, char *errstr, size_t errstrlen)
{
	DynamicLibrary *lib;

	lib = lib_remove_from_list(fname);
	if(!lib)
		return -1;
	unload_prims_lib(lib);
	return destroy_library(lib, errstr, errstrlen);
}

PRIM(listdynlibs) {
	List *res = nil; Root r_res;
	DynamicLibrary *lib;

	if(loaded_libs == nil)
		return nil;

	gcref(&r_res, (void **)&res);

	for(lib = loaded_libs; lib != nil; lib = lib->next)
		res = mklist(mkstr(str("%s", lib->name)), res);

	gcrderef(&r_res);
	return res;
}

PRIM(dynlibprims) {
	List *res = nil; Root r_res;
	char *name = nil; Root r_name;
	DynamicLibrary *lib;
	size_t i;

	if(list == nil)
		fail("$&libraryprims", "missing args");

	if(loaded_libs == nil)
		fail("$&libraryprims", "library %s not loaded", getstr(list->term));

	gcref(&r_res, (void **)&res);
	gcref(&r_name, (void **)&name);

	name = getstr(list->term);
	for(lib = loaded_libs; lib != nil; lib = lib->next) {
		if(streq(lib->name, name)) {
			for(i = 0; i < *lib->primslen; i++)
				res = mklist(mkstr(str("%s", lib->prims[i])), res);
			goto done;
		}
	}
	fail("$&libraryprims", "library %s not loaded", name);

done:
	gcrderef(&r_name);
	gcrderef(&r_res);
	return res;
}

PRIM(dynlibfile) {
	List *res = nil; Root r_res;
	char *name = nil; Root r_name;
	DynamicLibrary *lib;

	if(list == nil)
		fail("$&dynlibfile", "missing args");

	if(loaded_libs == nil)
		fail("$&dynlibfile", "library %s not loaded", getstr(list->term));

	gcref(&r_res, (void**)&res);
	gcref(&r_name, (void**)&name);

	name = getstr(list->term);
	lib = lib_find_name(name);
	if(lib == nil)
		fail("$&dynlibfile", "library %s not loaded", name);

	res = mklist(mkstr(str("%s", lib->fname)), res);

	gcrderef(&r_name);
	gcrderef(&r_res);

	return res;
}

PRIM(opendynlib) {
	char *fname; Root r_fname;
	char errstr[256];
	DynamicLibrary *testlib;

	if(list == nil)
		fail("$&openlibrary", "missing arg");

	gcref(&r_fname, (void **)&fname);

	fname = getstr(list->term);

	testlib = lib_find_fname(fname);
	if(testlib != nil)
		fail("$&openlibrary", "unable to open library %s: already open", testlib->name);

	if(open_library(fname, &errstr[0], sizeof(errstr)))
		fail("$&openlibrary", "unable to open library %s: %s", fname, errstr);

	gcrderef(&r_fname);

	return list_true;
}

Dict *
initprims_dynlib(Dict *primdict)
{
	loaded_libs = nil;

	X(listdynlibs);
	X(dynlibprims);
	X(dynlibfile);
	X(opendynlib);

	return primdict;
}
