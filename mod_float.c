#include "es.h"
#include "prim.h"
#include <stdlib.h>
LIBNAME(mod_float);

float
termtof(Term *term, char *funname, int arg)
{
	char *str = nil;
	char *endptr = nil;
	float res;

	errno = 0;

	str = getstr(term);
	res = strtof(str, &endptr);
	if(res == 0 && str == endptr)
		fail(funname, "invalid input: $%d = %s", arg, str);
	if(res == 0 && errno == ERANGE)
		fail(funname, "conversion overflow: $%d = %s", arg, str);

	return res;
}

List *
prim_addf(List *list, Binding *binding, int evalflags)
{
	float a, b, res;
	char temp[256];
	int printlen;
	List *result; Root r_result;

	if(list == nil || list->next == nil)
		fail("$&addf", "missing arguments");

	memset(temp, 0, sizeof(temp));

	a = termtof(list->term, "$&addf", 1);
	b = termtof(list->next->term, "$&addf", 2);

	res = a + b;
	printlen = snprintf(&temp[0], sizeof(temp), "%f", res);
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
	float a, b, res;
	char temp[256];
	int printlen;
	List *result; Root r_result;

	if(list == nil || list->next == nil)
		fail("$&subf", "missing arguments");

	memset(temp, 0, sizeof(temp));

	a = termtof(list->term, "$&subf", 1);
	b = termtof(list->next->term, "$&subf", 2);

	res = a - b;
	printlen = snprintf(&temp[0], sizeof(temp), "%f", res);
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
	float a, b, res;
	char temp[256];
	int printlen;
	List *result; Root r_result;

	if(list == nil || list->next == nil)
		fail("$&mulf", "missing arguments");

	memset(temp, 0, sizeof(temp));

	a = termtof(list->term, "$&mulf", 1);
	b = termtof(list->next->term, "$&mulf", 2);

	res = a * b;
	printlen = snprintf(&temp[0], sizeof(temp), "%f", res);
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
	float a, b, res;
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
	printlen = snprintf(&temp[0], sizeof(temp), "%f", res);
	if(printlen >= (int)sizeof(temp))
		fail("$&divf", "result conversion failed (temp string too short)");

	gcref(&r_result, (void **)&result);
	result = mklist(mkstr(str("%s", temp)), nil);
	gcrderef(&r_result);

	return result;
}


DYNPRIMS() = {
	{"addf", &prim_addf},
	{"subf", &prim_subf},
	{"mulf", &prim_mulf},
	{"divf", &prim_divf},
};
DYNPRIMSLEN();
