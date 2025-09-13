#include "es.h"
#include "prim.h"
#include "gc.h"

List *
hellotest(List *list, Binding *binding, int evalflags)
{
	used(list);
	used(binding);
	used(&evalflags);

	dprintf(1, "hello from mod_hello.so!\n");

	return nil;
}

DYNPRIMS() = {
	{"hellotest", &hellotest},
};
DYNPRIMSLEN();
