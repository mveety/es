#include "es.h"
#include "prim.h"
#include "gc.h"
#include <stdio.h>
LIBNAME(mod_hello);

List *
hellotest(List *list, Binding *binding, int evalflags)
{
	used(list);
	used(binding);
	used(&evalflags);

	dprintf(1, "hello from mod_hello.so!\n");

	return nil;
}

int
dyn_onload(void)
{
	dprintf(2, "mod_hello's dyn_onload was called!\n");
	return 0;
}

int
dyn_onunload(void)
{
	dprintf(2, "mod_hello's dyn_onunload was called!\n");
	return 0;
}

DYNPRIMS() = {
	{"hellotest", &hellotest},
};
DYNPRIMSLEN();
