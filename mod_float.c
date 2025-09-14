#include "es.h"
#include "prim.h"
#include <stdlib.h>
LIBNAME(mod_float);

double
termtof(Term *term, char *funname, int arg)
{
	char *str = nil;
	char *endptr = nil;
	double res;

	errno = 0;

	str = getstr(term);
	res = strtod(str, &endptr);
	if(res == 0 && str == endptr)
		fail(funname, "invalid input: $%d = %s", arg, str);
	if(res == 0 && errno == ERANGE)
		fail(funname, "conversion overflow: $%d = %s", arg, str);

	return res;
}

List *
prim_addf(List *list, Binding *binding, int evalflags)
{
	double a, b, res;
	char temp[256];
	int printlen;
	List *result; Root r_result;

	if(list == nil || list->next == nil)
		fail("$&addf", "missing arguments");

	memset(temp, 0, sizeof(temp));

	a = termtof(list->term, "$&addf", 1);
	b = termtof(list->next->term, "$&addf", 2);

	res = a + b;
	printlen = snprintf(&temp[0], sizeof(temp), "%g", res);
	if(printlen >= (int)sizeof(temp))
		fail("$&addf", "result conversion failed (temp string too short)");

	gcref(&r_result, (void **)&result);
	result = mklist(mkstr(str("%s", temp)), nil);
	gcrderef(&r_result);

	return result;
}

List *
prim_subf(List *list, Binding *binding, int evalflags)
{
	double a, b, res;
	char temp[256];
	int printlen;
	List *result; Root r_result;

	if(list == nil || list->next == nil)
		fail("$&subf", "missing arguments");

	memset(temp, 0, sizeof(temp));

	a = termtof(list->term, "$&subf", 1);
	b = termtof(list->next->term, "$&subf", 2);

	res = a - b;
	printlen = snprintf(&temp[0], sizeof(temp), "%g", res);
	if(printlen >= (int)sizeof(temp))
		fail("$&subf", "result conversion failed (temp string too short)");

	gcref(&r_result, (void **)&result);
	result = mklist(mkstr(str("%s", temp)), nil);
	gcrderef(&r_result);

	return result;
}

List *
prim_mulf(List *list, Binding *binding, int evalflags)
{
	double a, b, res;
	char temp[256];
	int printlen;
	List *result; Root r_result;

	if(list == nil || list->next == nil)
		fail("$&mulf", "missing arguments");

	memset(temp, 0, sizeof(temp));

	a = termtof(list->term, "$&mulf", 1);
	b = termtof(list->next->term, "$&mulf", 2);

	res = a * b;
	printlen = snprintf(&temp[0], sizeof(temp), "%g", res);
	if(printlen >= (int)sizeof(temp))
		fail("$&mulf", "result conversion failed (temp string too short)");

	gcref(&r_result, (void **)&result);
	result = mklist(mkstr(str("%s", temp)), nil);
	gcrderef(&r_result);

	return result;
}

List *
prim_divf(List *list, Binding *binding, int evalflags)
{
	double a, b, res;
	char temp[256];
	int printlen;
	List *result; Root r_result;

	if(list == nil || list->next == nil)
		fail("$&divf", "missing arguments");

	memset(temp, 0, sizeof(temp));

	a = termtof(list->term, "$&divf", 1);
	b = termtof(list->next->term, "$&divf", 2);

	if(b == 0)
		fail("$&divf", "divide by zero");
	res = a / b;
	printlen = snprintf(&temp[0], sizeof(temp), "%g", res);
	if(printlen >= (int)sizeof(temp))
		fail("$&divf", "result conversion failed (temp string too short)");

	gcref(&r_result, (void **)&result);
	result = mklist(mkstr(str("%s", temp)), nil);
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
