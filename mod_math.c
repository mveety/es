#include "es.h"
#include "prim.h"
#include <math.h>
#include "float_util.h"

LIBNAME(mod_math);

PRIM(cbrt) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&cbrt", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&cbrt", 1);
	res = cbrt(a);
	result = floattolist(res, "$&cbrt");

	gcrderef(&r_result);
	
	return result;
}


PRIM(sqrt) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&sqrt", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&sqrt", 1);
	res = sqrt(a);
	result = floattolist(res, "$&sqrt");

	gcrderef(&r_result);
	
	return result;
}

PRIM(hypot) {
	double a, b, res;
	List *result = nil; Root r_result;

	if(list == nil || list->next == nil)
		fail("$&hypot", "missing arguments");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&hypot", 1);
	b = termtof(list->next->term, "$&hypot", 2);
	res = hypot(a, b);
	result = floattolist(res, "$&hypot");

	gcrderef(&r_result);

	return result;
}

PRIM(sine) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&sine", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&sine", 1);
	res = sin(a);
	result = floattolist(res, "$&sine");

	gcrderef(&r_result);
	
	return result;
}

PRIM(cosine) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&cosine", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&cosine", 1);
	res = cos(a);
	result = floattolist(res, "$&cosine");

	gcrderef(&r_result);
	
	return result;
}

PRIM(tangent) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&tangent", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&tangent", 1);
	res = tan(a);
	result = floattolist(res, "$&tangent");

	gcrderef(&r_result);
	
	return result;
}


PRIM(ceil) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&ceil", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&ceil", 1);
	res = ceil(a);
	result = floattolist(res, "$&ceil");

	gcrderef(&r_result);
	
	return result;
}

PRIM(floor) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&floor", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&floor", 1);
	res = floor(a);
	result = floattolist(res, "$&floor");

	gcrderef(&r_result);
	
	return result;
}

PRIM(log) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&log", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&log", 1);
	res = log(a);
	result = floattolist(res, "$&log");

	gcrderef(&r_result);
	
	return result;
}

PRIM(log10) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&log10", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&log10", 1);
	res = log10(a);
	result = floattolist(res, "$&log10");

	gcrderef(&r_result);
	
	return result;
}

PRIM(log2) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&log2", "missing argument");

	gcref(&r_result, (void**)&result);

	a = termtof(list->term, "$&log2", 1);
	res = log2(a);
	result = floattolist(res, "$&log2");

	gcrderef(&r_result);
	
	return result;
}

DYNPRIMS() = {
	DX(cbrt),
	DX(sqrt),
	DX(hypot),
	DX(sine),
	DX(cosine),
	DX(tangent),
	DX(ceil),
	DX(floor),
	DX(log),
	DX(log10),
	DX(log2),
};
DYNPRIMSLEN();
