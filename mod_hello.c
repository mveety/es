#include "es.h"
#include "prim.h"
#include "gc.h"

char *dynprims[] = {"hellotest"};
size_t dynprimslen = (sizeof(dynprims)/sizeof(char*));
extern List *list_true;

List*
hellotest(List *list, Binding *binding, int evalflags)
{
	used(list);
	used(binding);
	used(&evalflags);

	dprintf(1, "hello from mod_hello.so!\n");

	return nil;
}

