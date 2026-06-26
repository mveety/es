#include "es.h"
#include "prim.h"
#include <math.h>

int64_t (*listtoint)(List *, char *, int);
double (*listtof)(List *, char *, int);
List *(*floattolist)(double, char *);

int
math_onload(void)
{
	DynamicLibrary *mod_float;
	union {
		void *ptr;
		int64_t (*fptr)(List *, char *, int);
	} listtoint_union;
	union {
		void *ptr;
		double (*fptr)(List *, char *, int);
	} listtof_union;
	union {
		void *ptr;
		List *(*fptr)(double, char *);
	} floattolist_union;

	mod_float = dynlib("mod_float");
	if(!mod_float)
		return ErrorModuleNotLoaded;

	if(!(listtoint_union.ptr = dynsymbol(mod_float, "listtoint")))
		return ErrorModuleMissingSymbol;
	if(!(listtof_union.ptr = dynsymbol(mod_float, "listtof")))
		return ErrorModuleMissingSymbol;
	if(!(floattolist_union.ptr = dynsymbol(mod_float, "floattolist")))
		return ErrorModuleMissingSymbol;

	listtoint = listtoint_union.fptr;
	listtof = listtof_union.fptr;
	floattolist = floattolist_union.fptr;

	return 0;
}

PRIM(cbrt) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&cbrt", 1);
	res = cbrt(a);
	result = floattolist(res, "$&cbrt");

	deref(result);

	return result;
}

PRIM(sqrt) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&sqrt", 1);
	res = sqrt(a);
	result = floattolist(res, "$&sqrt");

	deref(result);

	return result;
}

PRIM(hypot) {
	double a, b, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&hypot", 1);
	b = listtof(list->next, "$&hypot", 2);
	res = hypot(a, b);
	result = floattolist(res, "$&hypot");

	deref(result);

	return result;
}

PRIM(sin) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&sin", 1);
	res = sin(a);
	result = floattolist(res, "$&sin");

	deref(result);

	return result;
}

PRIM(asin) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&asin", 1);
	res = asin(a);
	result = floattolist(res, "$&asin");

	deref(result);

	return result;
}

PRIM(sinh) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&sinh", 1);
	res = sinh(a);
	result = floattolist(res, "$&sinh");

	deref(result);

	return result;
}

PRIM(asinh) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&asinh", 1);
	res = asinh(a);
	result = floattolist(res, "$&asinh");

	deref(result);

	return result;
}


PRIM(cos) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&cos", 1);
	res = cos(a);
	result = floattolist(res, "$&cos");

	deref(result);

	return result;
}

PRIM(acos) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&acos", 1);
	res = acos(a);
	result = floattolist(res, "$&acos");

	deref(result);

	return result;
}

PRIM(cosh) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&cosh", 1);
	res = cosh(a);
	result = floattolist(res, "$&cosh");

	deref(result);

	return result;
}

PRIM(acosh) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&acosh", 1);
	res = acosh(a);
	result = floattolist(res, "$&acosh");

	deref(result);

	return result;
}

PRIM(tan) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&tan", 1);
	res = tan(a);
	result = floattolist(res, "$&tan");

	deref(result);

	return result;
}

PRIM(atan) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&atan", 1);
	res = atan(a);
	result = floattolist(res, "$&atan");

	deref(result);

	return result;
}

PRIM(tanh) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&tanh", 1);
	res = tanh(a);
	result = floattolist(res, "$&tanh");

	deref(result);

	return result;
}

PRIM(atanh) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&atanh", 1);
	res = atanh(a);
	result = floattolist(res, "$&atanh");

	deref(result);

	return result;
}

PRIM(ceil) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&ceil", 1);
	res = ceil(a);
	result = floattolist(res, "$&ceil");

	deref(result);

	return result;
}

PRIM(floor) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&floor", 1);
	res = floor(a);
	result = floattolist(res, "$&floor");

	deref(result);

	return result;
}

PRIM(log) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&log", 1);
	res = log(a);
	result = floattolist(res, "$&log");

	deref(result);

	return result;
}

PRIM(exp) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&exp", 1);
	res = exp(a);
	result = floattolist(res, "$&exp");

	deref(result);

	return result;
}

PRIM(log10) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&log10", 1);
	res = log10(a);
	result = floattolist(res, "$&log10");

	deref(result);

	return result;
}

PRIM(log2) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&log2", 1);
	res = log2(a);
	result = floattolist(res, "$&log2");

	deref(result);

	return result;
}

PRIM(exp2) {
	double a, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&exp2", 1);
	res = exp2(a);
	result = floattolist(res, "$&exp2");

	deref(result);

	return result;
}

PRIM(pow) {
	double a, b, res;
	List *result = nil;

	ref(result);

	a = listtof(list, "$&pow", 1);
	b = listtof(list->next, "$&pow", 2);
	res = pow(a, b);
	result = floattolist(res, "$&pow");

	deref(result);

	return result;
}

List *
next(List *list)
{
	if(list != nil)
		return list->next;
	return nil;
}

PRIM(intpow) {
	int64_t base, exp;
	int64_t res = 1;

	exp = listtoint(next(list), "$&intpow", 2);

	if(exp < 0)
		fail("$&intpow", "only positive exponents are valid");
	if(exp == 0)
		return mklist(mkstr(str("1")), nil);

	base = listtoint(list, "$&intpow", 1);

	if(base == 1)
		return mklist(mkstr(str("1")), nil);
	if(base == 0)
		return mklist(mkstr(str("0")), nil);

	while(exp > 0) {
		if(exp % 2)
			res *= base;
		base *= base;
		exp /= 2;
	}

	return mklist(mkstr(str("%ld", res)), nil);
}

MODULE(mod_math, &math_onload, nil,
	DX(cbrt), DX(sqrt),	 DX(hypot), DX(sin),  DX(asin), DX(sinh),	DX(asinh), DX(cos),	  DX(acos),
	DX(cosh), DX(acosh), DX(tan),	DX(atan), DX(tanh), DX(atanh),	DX(ceil),  DX(floor), DX(log),
	DX(exp),  DX(log10), DX(log2),	DX(exp2), DX(pow),	DX(intpow),
);
