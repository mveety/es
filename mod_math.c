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

	gcref(&r_result, (void **)&result);

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

	gcref(&r_result, (void **)&result);

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

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&hypot", 1);
	b = termtof(list->next->term, "$&hypot", 2);
	res = hypot(a, b);
	result = floattolist(res, "$&hypot");

	gcrderef(&r_result);

	return result;
}

PRIM(sin) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&sin", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&sin", 1);
	res = sin(a);
	result = floattolist(res, "$&sin");

	gcrderef(&r_result);

	return result;
}

PRIM(asin) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&asin", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&asin", 1);
	res = asin(a);
	result = floattolist(res, "$&asin");

	gcrderef(&r_result);

	return result;
}

PRIM(sinh) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&sinh", "missinhg argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&sinh", 1);
	res = sinh(a);
	result = floattolist(res, "$&sinh");

	gcrderef(&r_result);

	return result;
}

PRIM(asinh) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&asinh", "missinhg argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&asinh", 1);
	res = asinh(a);
	result = floattolist(res, "$&asinh");

	gcrderef(&r_result);

	return result;
}


PRIM(cos) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&cos", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&cos", 1);
	res = cos(a);
	result = floattolist(res, "$&cos");

	gcrderef(&r_result);

	return result;
}

PRIM(acos) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&acos", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&acos", 1);
	res = acos(a);
	result = floattolist(res, "$&acos");

	gcrderef(&r_result);

	return result;
}

PRIM(cosh) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&cosh", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&cosh", 1);
	res = cosh(a);
	result = floattolist(res, "$&cosh");

	gcrderef(&r_result);

	return result;
}

PRIM(acosh) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&acosh", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&acosh", 1);
	res = acosh(a);
	result = floattolist(res, "$&acosh");

	gcrderef(&r_result);

	return result;
}

PRIM(tan) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&tan", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&tan", 1);
	res = tan(a);
	result = floattolist(res, "$&tan");

	gcrderef(&r_result);

	return result;
}

PRIM(atan) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&atan", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&atan", 1);
	res = atan(a);
	result = floattolist(res, "$&atan");

	gcrderef(&r_result);

	return result;
}

PRIM(tanh) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&tanh", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&tanh", 1);
	res = tanh(a);
	result = floattolist(res, "$&tanh");

	gcrderef(&r_result);

	return result;
}

PRIM(atanh) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&atanh", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&atanh", 1);
	res = atanh(a);
	result = floattolist(res, "$&atanh");

	gcrderef(&r_result);

	return result;
}

PRIM(ceil) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&ceil", "missing argument");

	gcref(&r_result, (void **)&result);

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

	gcref(&r_result, (void **)&result);

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

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&log", 1);
	res = log(a);
	result = floattolist(res, "$&log");

	gcrderef(&r_result);

	return result;
}

PRIM(exp) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&exp", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&exp", 1);
	res = exp(a);
	result = floattolist(res, "$&exp");

	gcrderef(&r_result);

	return result;
}

PRIM(log10) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&log10", "missing argument");

	gcref(&r_result, (void **)&result);

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

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&log2", 1);
	res = log2(a);
	result = floattolist(res, "$&log2");

	gcrderef(&r_result);

	return result;
}

PRIM(exp2) {
	double a, res;
	List *result = nil; Root r_result;

	if(list == nil)
		fail("$&exp2", "missing argument");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&exp2", 1);
	res = exp2(a);
	result = floattolist(res, "$&exp2");

	gcrderef(&r_result);

	return result;
}

PRIM(pow) {
	double a, b, res;
	List *result = nil; Root r_result;

	if(list == nil || list->next == nil)
		fail("$&pow", "missing arguments");

	gcref(&r_result, (void **)&result);

	a = termtof(list->term, "$&pow", 1);
	b = termtof(list->next->term, "$&pow", 2);
	res = pow(a, b);
	result = floattolist(res, "$&pow");

	gcrderef(&r_result);

	return result;
}

PRIM(intpow) {
	int64_t base, exp;
	int64_t res = 1;

	if(list == nil || list->next == nil)
		fail("$&intpow", "missing arguments");

	errno = 0;

	exp = strtoll(getstr(list->next->term), NULL, 10);
	if(exp == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&intpow", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&intpow", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if(exp < 0)
		fail("$&intpow", "only positive exponents are valid");
	if(exp == 0)
		return mklist(mkstr(str("1")), nil);

	base = strtoll(getstr(list->term), NULL, 10);
	if(base == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&intpow", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&intpow", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	while(exp > 0) {
		if(exp % 2)
			res *= base;
		base *= base;
		exp /= 2;
	}

	return mklist(mkstr(str("%ld", res)), nil);
}

DYNPRIMS() = {
	DX(cbrt),  DX(sqrt), DX(hypot), DX(sin),   DX(asin), DX(sinh), DX(asinh), DX(cos),
	DX(acos),  DX(cosh), DX(acosh), DX(tan),   DX(atan), DX(tanh), DX(atanh), DX(ceil),
	DX(floor), DX(log),	 DX(exp),	DX(log10), DX(log2), DX(exp2), DX(pow),	  DX(intpow),
};
DYNPRIMSLEN();
