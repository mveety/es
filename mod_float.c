#include "es.h"
#include "prim.h"
#include <stdlib.h>
#include "float_util.h"
LIBNAME(mod_float);

List *
prim_addf(List *list, Binding *binding, int evalflags)
{
	double a, b, res;
	List *result; Root r_result;

	a = listtof(list, "$&addf", 1);
	b = listtof(list->next, "$&addf", 2);

	res = a + b;

	gcref(&r_result, (void **)&result);
	result = floattolist(res, "$&addf");
	gcrderef(&r_result);

	return result;
}

List *
prim_subf(List *list, Binding *binding, int evalflags)
{
	double a, b, res;
	List *result; Root r_result;

	a = listtof(list, "$&subf", 1);
	b = listtof(list->next, "$&subf", 2);

	res = a - b;

	gcref(&r_result, (void **)&result);
	result = floattolist(res, "$&subf");
	gcrderef(&r_result);

	return result;
}

List *
prim_mulf(List *list, Binding *binding, int evalflags)
{
	double a, b, res;
	List *result; Root r_result;

	a = listtof(list, "$&mulf", 1);
	b = listtof(list->next, "$&mulf", 2);

	res = a * b;

	gcref(&r_result, (void **)&result);
	result = floattolist(res, "$&mulf");
	gcrderef(&r_result);

	return result;
}

List *
prim_divf(List *list, Binding *binding, int evalflags)
{
	double a, b, res;
	List *result; Root r_result;

	a = listtof(list, "$&divf", 1);
	b = listtof(list->next, "$&divf", 2);

	if(b == 0)
		fail("$&divf", "divide by zero");
	res = a / b;

	gcref(&r_result, (void **)&result);
	result = floattolist(res, "$&divf");
	gcrderef(&r_result);

	return result;
}

List *
prim_feq(List *list, Binding *binding, int evalflags)
{
	char *str = nil;
	char *endptr = nil;
	double a, b;

	if(list == nil || list->next == nil)
		return list_false;

	errno = 0;
	str = getstr(list->term);
	a = strtod(str, &endptr);
	if(a == 0 && str == endptr)
		return list_false;
	if(a == 0 && errno == ERANGE)
		return list_false;

	str = endptr = nil;
	errno = 0;
	str = getstr(list->next->term);
	b = strtod(str, &endptr);
	if(b == 0 && str == endptr)
		return list_false;
	if(b == 0 && errno == ERANGE)
		return list_false;

	if(a == b)
		return list_true;
	return list_false;
}

List *
prim_fgt(List *list, Binding *binding, int evalflags)
{
	char *str = nil;
	char *endptr = nil;
	double a, b;

	if(list == nil || list->next == nil)
		return list_false;

	errno = 0;
	str = getstr(list->term);
	a = strtod(str, &endptr);
	if(a == 0 && str == endptr)
		return list_false;
	if(a == 0 && errno == ERANGE)
		return list_false;

	str = endptr = nil;
	errno = 0;
	str = getstr(list->next->term);
	b = strtod(str, &endptr);
	if(b == 0 && str == endptr)
		return list_false;
	if(b == 0 && errno == ERANGE)
		return list_false;

	if(a > b)
		return list_true;
	return list_false;
}

List *
prim_fgte(List *list, Binding *binding, int evalflags)
{
	char *str = nil;
	char *endptr = nil;
	double a, b;

	if(list == nil || list->next == nil)
		return list_false;

	errno = 0;
	str = getstr(list->term);
	a = strtod(str, &endptr);
	if(a == 0 && str == endptr)
		return list_false;
	if(a == 0 && errno == ERANGE)
		return list_false;

	str = endptr = nil;
	errno = 0;
	str = getstr(list->next->term);
	b = strtod(str, &endptr);
	if(b == 0 && str == endptr)
		return list_false;
	if(b == 0 && errno == ERANGE)
		return list_false;

	if(a >= b)
		return list_true;
	return list_false;
}

List *
prim_flt(List *list, Binding *binding, int evalflags)
{
	char *str = nil;
	char *endptr = nil;
	double a, b;

	if(list == nil || list->next == nil)
		return list_false;

	errno = 0;
	str = getstr(list->term);
	a = strtod(str, &endptr);
	if(a == 0 && str == endptr)
		return list_false;
	if(a == 0 && errno == ERANGE)
		return list_false;

	str = endptr = nil;
	errno = 0;
	str = getstr(list->next->term);
	b = strtod(str, &endptr);
	if(b == 0 && str == endptr)
		return list_false;
	if(b == 0 && errno == ERANGE)
		return list_false;

	if(a < b)
		return list_true;
	return list_false;
}

List *
prim_flte(List *list, Binding *binding, int evalflags)
{
	char *str = nil;
	char *endptr = nil;
	double a, b;

	if(list == nil || list->next == nil)
		return list_false;

	errno = 0;
	str = getstr(list->term);
	a = strtod(str, &endptr);
	if(a == 0 && str == endptr)
		return list_false;
	if(a == 0 && errno == ERANGE)
		return list_false;

	str = endptr = nil;
	errno = 0;
	str = getstr(list->next->term);
	b = strtod(str, &endptr);
	if(b == 0 && str == endptr)
		return list_false;
	if(b == 0 && errno == ERANGE)
		return list_false;

	if(a <= b)
		return list_true;
	return list_false;
}


DYNPRIMS() = {
	{"addf", &prim_addf}, {"subf", &prim_subf}, {"mulf", &prim_mulf},
	{"divf", &prim_divf}, {"feq", &prim_feq},	{"fgt", &prim_fgt},
	{"fgte", &prim_fgte}, {"flt", &prim_flt},	{"flte", &prim_flte},
};
DYNPRIMSLEN();
