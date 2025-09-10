/* except.c -- exception mechanism ($Revision: 1.1.1.1 $) */

#include <es.h>
#include <print.h>
#include <stdenv.h>

/* globals */
Handler *tophandler = NULL;
Handler *roothandler = NULL;
List *exception = NULL;
Push *pushlist = NULL;
Boolean debug_exceptions = FALSE;

/* pophandler -- remove a handler */
extern void pophandler(Handler *handler) {
	assert(tophandler == handler);
	assert(handler->rootlist == rootlist);
	tophandler = handler->up;
}

/* throw -- raise an exception */
extern noreturn throw(List *e) {
	Handler *handler = tophandler;
	Root exceptroot;

	assert(!gcisblocked());
	assert(e != NULL);
	assert(handler != NULL);
	tophandler = handler->up;

	exceptionroot(&exceptroot, &e);
	while (pushlist != handler->pushlist) {
		rootlist = &pushlist->defnroot;
		varpop(pushlist);
	}
	exceptionunroot();
	evaldepth = handler->evaldepth;

	if(assertions == TRUE){
		for (; rootlist != handler->rootlist; rootlist = rootlist->next)
			assert(rootlist != NULL);
	} else {
		rootlist = handler->rootlist;
	}

	exception = e;
	longjmp(handler->label, 1);
	NOTREACHED;
}

extern noreturn
fail(const char *from, const char *fmt, ...)
{
	char *s;
	va_list args;

	va_start(args, fmt);
	s = strv(fmt, args);
	va_end(args);

	gcdisable();
	Ref(List *, e, mklist(mkstr("error"),
				mklist(mkstr((char *) from),
					mklist(mkstr(s), NULL))));
	while(gcisblocked())
		gcenable();
	throw(e);
	RefEnd(e);
}

/* newchildcatcher -- remove the current handler chain for a new child */
extern void newchildcatcher(void) {
	tophandler = roothandler;
}

/* raised -- print exceptions as we climb the exception stack */
extern List *raised(List *e) {
	if(debug_exceptions == TRUE)
		eprint("raised (sp @ %x) %L\n", &e, e, " ");
	return e;
}

