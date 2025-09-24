#include "es.h"
#include "prim.h"
#include "float_util.h"

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

double
listtof(List *list, char *funname, int arg)
{
	if(list == nil)
		fail(funname, "missing argument $%d", arg);

	return termtof(list->term, funname, arg);
}

List *
floattolist(double num, char *funname)
{
	char temp[256];
	int printlen;
	List *result = nil; Root r_result;

	gcref(&r_result, (void **)&result);

	printlen = snprintf(&temp[0], sizeof(temp), "%.8g", num);
	if(printlen >= (int)sizeof(temp))
		fail(funname, "result conversion failed (temp string too short)");
	result = mklist(mkstr(str("%s", temp)), nil);

	gcrderef(&r_result);

	return result;
}

int64_t
termtoint(Term *term, char *funname, int arg)
{
	int64_t res;

	errno = 0;
	res = strtoll(getstr(term), NULL, 10);
	if(res == 0) {
		switch(errno) {
		case EINVAL:
			fail(funname, str("invalid input: $%d = '%s'", arg, getstr(term)));
			break;
		case ERANGE:
			fail(funname, str("conversion overflow: $%d = '%s'", arg, getstr(term)));
			break;
		}
	}

	return res;
}

int64_t
listtoint(List *list, char *funname, int arg)
{
	if(list == nil)
		fail(funname, "missing argument $%d", arg);

	return termtoint(list->term, funname, arg);
}
