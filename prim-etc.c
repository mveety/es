/* prim-etc.c -- miscellaneous primitives ($Revision: 1.2 $) */

#include <stdlib.h>
#define	REQUIRE_PWD	1

#include "es.h"
#include "prim.h"

PRIM(result) {
	return list;
}

PRIM(echo) {
	const char *eol = "\n";
	if (list != NULL) {
		if (termeq(list->term, "-n")) {
			eol = "";
			list = list->next;
		} else if (termeq(list->term, "--"))
			list = list->next;
	}
	print("%L%s", list, " ", eol);
	return list_true;
}

PRIM(count) {
	return mklist(mkstr(str("%d", length(list))), NULL);
}

PRIM(setnoexport) {
	Ref(List *, lp, list);
	setnoexport(lp);
	RefReturn(lp);
}

PRIM(version) {
	return mklist(mkstr((char *) version), NULL);
}

PRIM(buildstring) {
	return mklist(mkstr((char *) buildstring), NULL);
}

PRIM(exec) {
	return eval(list, NULL, evalflags | eval_inchild);
}

PRIM(dot) {
	int c, fd;
	Push zero, star;
	volatile int runflags = (evalflags & eval_inchild);
	const char * const usage = ". [-einvxL] file [arg ...]";

	esoptbegin(list, "$&dot", usage);
	while ((c = esopt("einvxL")) != EOF)
		switch (c) {
		case 'e':	runflags |= eval_exitonfalse;	break;
		case 'i':	runflags |= run_interactive;	break;
		case 'n':	runflags |= run_noexec;		break;
		case 'v':	runflags |= run_echoinput;	break;
		case 'x':	runflags |= run_printcmds;	break;
		case 'L':	runflags |= run_lisptrees; break;
		}

	Ref(List *, result, NULL);
	Ref(List *, lp, esoptend());
	if (lp == NULL)
		fail("$&dot", "usage: %s", usage);

	Ref(char *, file, getstr(lp->term));
	lp = lp->next;
	fd = eopen(file, oOpen);
	if (fd == -1)
		fail("$&dot", "%s: %s", file, esstrerror(errno));

	varpush(&star, "*", lp);
	varpush(&zero, "0", mklist(mkstr(file), NULL));

	result = runfd(fd, file, runflags);

	varpop(&zero);
	varpop(&star);
	RefEnd2(file, lp);
	RefReturn(result);
}

PRIM(flatten) {
	char *sep;
	if (list == NULL)
		fail("$&flatten", "usage: %%flatten separator [args ...]");
	Ref(List *, lp, list);
	sep = getstr(lp->term);
	lp = mklist(mkstr(str("%L", lp->next, sep)), NULL);
	RefReturn(lp);
}

PRIM(whatis) {
	/* the logic in here is duplicated in eval() */
	if (list == NULL || list->next != NULL)
		fail("$&whatis", "usage: $&whatis program");
	Ref(Term *, term, list->term);
	if (getclosure(term) == NULL) {
		List *fn;
		Ref(char *, prog, getstr(term));
		assert(prog != NULL);
		fn = varlookup2("fn-", prog, binding);
		if (fn != NULL)
			list = fn;
		else {
			if (isabsolute(prog)) {
				char *error = checkexecutable(prog);
				if (error != NULL)
					fail("$&whatis", "%s: %s", prog, error);
			} else
				list = pathsearch(term);
		}
		RefEnd(prog);
	}
	RefEnd(term);
	return list;
}

PRIM(split) {
	char *sep;
	if (list == NULL)
		fail("$&split", "usage: %%split separator [args ...]");
	Ref(List *, lp, list);
	sep = getstr(lp->term);
	lp = fsplit(sep, lp->next, TRUE);
	RefReturn(lp);
}

PRIM(fsplit) {
	char *sep;
	if (list == NULL)
		fail("$&fsplit", "usage: %%fsplit separator [args ...]");
	Ref(List *, lp, list);
	sep = getstr(lp->term);
	lp = fsplit(sep, lp->next, FALSE);
	RefReturn(lp);
}

PRIM(var) {
	Term *term;
	if (list == NULL)
		return NULL;
	Ref(List *, rest, list->next);
	Ref(char *, name, getstr(list->term));
	Ref(List *, defn, varlookup(name, NULL));
	rest = prim_var(rest, NULL, evalflags);
	term = mkstr(str("%S = %#L", name, defn, " "));
	list = mklist(term, rest);
	RefEnd3(defn, name, rest);
	return list;
}

PRIM(sethistory) {
	if (list == NULL) {
		sethistory(NULL);
		return NULL;
	}
	Ref(List *, lp, list);
	sethistory(getstr(lp->term));
	RefReturn(lp);
}
#if READLINE

PRIM(addhistory) {
	if (list == NULL)
		fail("$&addhistory", "usage: $&addhistory [string]");
	add_history(getstr(list->term));
	return NULL;
}

PRIM(clearhistory) {
	clear_history();
	return NULL;
}

#endif

PRIM(getlast) {
	if(lastcmd == NULL)
		return mklist(mkstr((char *) ""), NULL);
	return mklist(mkstr((char *) lastcmd), NULL);
}

PRIM(parse) {
	List *result;
	Tree *tree;
	Ref(char *, prompt1, NULL);
	Ref(char *, prompt2, NULL);
	Ref(List *, lp, list);
	if (lp != NULL) {
		prompt1 = getstr(lp->term);
		if ((lp = lp->next) != NULL)
			prompt2 = getstr(lp->term);
	}
	RefEnd(lp);
	tree = parse(prompt1, prompt2);
	result = (tree == NULL)
		   ? NULL
		   : mklist(mkterm(NULL, mkclosure(mk(nThunk, tree), NULL)),
			    NULL);
	RefEnd2(prompt2, prompt1);
	return result;
}

PRIM(exitonfalse) {
	return eval(list, NULL, evalflags | eval_exitonfalse);
}

PRIM(batchloop) {
	Ref(List *, result, list_true);
	Ref(List *, dispatch, NULL);

	SIGCHK();

	ExceptionHandler

		for (;;) {
			List *parser, *cmd;
			parser = varlookup("fn-%parse", NULL);
			cmd = (parser == NULL)
					? prim("parse", NULL, NULL, 0)
					: eval(parser, NULL, 0);
			SIGCHK();
			dispatch = varlookup("fn-%dispatch", NULL);
			if (cmd != NULL) {
				if (dispatch != NULL)
					cmd = append(dispatch, cmd);
				result = eval(cmd, NULL, evalflags);
				SIGCHK();
			}
		}

	CatchException (e)

		if (!termeq(e->term, "eof"))
			throw(e);
		RefEnd(dispatch);
		if (result == list_true)
			result = list_true;
		RefReturn(result);

	EndExceptionHandler
}

PRIM(collect) {
	gc();
	return list_true;
}

PRIM(home) {
	struct passwd *pw;
	if (list == NULL)
		return varlookup("home", NULL);
	if (list->next != NULL)
		fail("$&home", "usage: %%home [user]");
	pw = getpwnam(getstr(list->term));
	return (pw == NULL) ? NULL : mklist(mkstr(gcdup(pw->pw_dir)), NULL);
}

PRIM(vars) {
	return listvars(FALSE);
}

PRIM(internals) {
	return listvars(TRUE);
}

PRIM(isinteractive) {
	return isinteractive() ? list_true : list_false;
}

PRIM(noreturn) {
	if (list == NULL)
		fail("$&noreturn", "usage: $&noreturn lambda args ...");
	Ref(List *, lp, list);
	Ref(Closure *, closure, getclosure(lp->term));
	if (closure == NULL || closure->tree->kind != nLambda)
		fail("$&noreturn", "$&noreturn: %E is not a lambda", lp->term);
	Ref(Tree *, tree, closure->tree);
	Ref(Binding *, context, bindargs(tree->u[0].p, lp->next, closure->binding));
	lp = walk(tree->u[1].p, context, evalflags);
	RefEnd3(context, tree, closure);
	RefReturn(lp);
}

PRIM(catch_noreturn) {
	if (list == NULL)
		fail("$&catch_noreturn", "usage: $&catch_noreturn lambda args ...");
	Ref(List *, lp, list);
	Ref(Closure *, closure, getclosure(lp->term));
	if (closure == NULL || closure->tree->kind != nLambda)
		fail("$&catch_noreturn", "$&catch_noreturn: %E is not a lambda", lp->term);
	Ref(Tree *, tree, closure->tree);
	Ref(Binding *, context, bindargs(tree->u[0].p, lp->next, closure->binding));
	lp = walk(tree->u[1].p, context, evalflags);
	RefEnd3(context, tree, closure);
	RefReturn(lp);
}


PRIM(setmaxevaldepth) {
	char *s;
	long n;
	if (list == NULL) {
		maxevaldepth = MAXmaxevaldepth;
		return NULL;
	}
	if (list->next != NULL)
		fail("$&setmaxevaldepth", "usage: $&setmaxevaldepth [limit]");
	Ref(List *, lp, list);
	n = strtol(getstr(lp->term), &s, 0);
	if (n < 0 || (s != NULL && *s != '\0'))
		fail("$&setmaxevaldepth", "max-eval-depth must be set to a positive integer");
	if (n < MINmaxevaldepth)
		n = (n == 0) ? MAXmaxevaldepth : MINmaxevaldepth;
	maxevaldepth = n;
	RefReturn(lp);
}

PRIM(getevaldepth) {
	return mklist(mkstr(str("%d", evaldepth)), NULL);
}

#if READLINE
PRIM(resetterminal) {
	resetterminal = TRUE;
	return list_true;
}
#endif

PRIM(add) {
	int a = 0;
	int b = 0;
	int res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&add", "missing arguments");

	a = (int)strtol(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&add", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&add", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = (int)strtol(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&add", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&add", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	res = a + b;
	return mklist(mkstr(str("%d", res)), NULL);
}

PRIM(sub) {
	int a = 0;
	int b = 0;
	int res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&sub", "missing arguments");

	a = (int)strtol(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&sub", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&sub", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = (int)strtol(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&sub", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&sub", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	res = a - b;
	return mklist(mkstr(str("%d", res)), NULL);
}

PRIM(mul) {
	int a = 0;
	int b = 0;
	int res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&mul", "missing arguments");

	a = (int)strtol(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&mul", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&mul", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = (int)strtol(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&mul", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&mul", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	res = a * b;
	return mklist(mkstr(str("%d", res)), NULL);
}

PRIM(div) {
	int a = 0;
	int b = 0;
	int res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&div", "missing arguments");

	a = (int)strtol(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&div", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&div", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = (int)strtol(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&div", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&div", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if (b == 0)
		fail("$&div", "divide by zero");
	res = a / b;
	return mklist(mkstr(str("%d", res)), NULL);
}

PRIM(mod) {
	int a = 0;
	int b = 0;
	int res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&mod", "missing arguments");

	a = (int)strtol(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
			fail("$&mod", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&mod", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	b = (int)strtol(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
			fail("$&mod", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&mod", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if (b == 0)
		fail("$&mod", "divide by zero");
	res = a % b;
	return mklist(mkstr(str("%d", res)), NULL);
}

PRIM(eq) {
	int a = 0;
	int b = 0;

	if(list == NULL)
		return list_false;
	if (list->next == NULL)
		return list_false;

	a = (int)strtol(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	b = (int)strtol(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	if (a == b)
		return list_true;
	return list_false;
}

PRIM(gt) {
	int a = 0;
	int b = 0;

	if (list == NULL)
		return list_false;
	if (list->next == NULL)
		return list_false;

	a = (int)strtol(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	b = (int)strtol(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	}

	if (a > b)
		return list_true;
	return list_false;
}

PRIM(lt) {
	int a = 0;
	int b = 0;

	if (list == NULL)
		return list_false;
	if (list->next == NULL)
		return list_false;

	a = (int)strtol(getstr(list->term), NULL, 10);
	if(a == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	} 

	b = (int)strtol(getstr(list->next->term), NULL, 10);
	if(b == 0){
		switch(errno){
		case EINVAL:
		case ERANGE:
			return list_false;
		}
	} 

	if (a < b)
		return list_true;
	return list_false;
}

PRIM(tobase) {
	int base, num;
	char *s, *se;
	List *res; Root r_res;

	if(list == NULL || list->next == NULL)
		fail("$&tobase", "missing arguments");

	base = (int)strtol(getstr(list->term), NULL, 10);
	if(base == 0){
		switch(errno){
		case EINVAL:
			fail("$&tobase", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&tobase", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}

	}

	num = (int)strtol(getstr(list->next->term), NULL, 10);
	if(num == 0){
		switch(errno){
		case EINVAL:
			fail("$&tobase", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&tobase", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if(base < 2)
		fail("$&tobase", "base < 2");
	if(base > 16)
		fail("$&tobase", "base > 16");

	res = list;
	gcref(&r_res, (void**)&res);

	gcdisable();

	if(num == 0){
		gcenable();
		gcrderef(&r_res);
		return mklist(mkstr(str("0")), NULL);
	}

	s = ealloc(256);
	se = &s[254];
	memset(s, 0, 256);

	while(num) {
		*--se = "0123456789abcdef"[num%base];
		num = num/base;
		if(se == s) {
			free(s);
			gcenable();
			fail("$&tobase", "overflow");
		}
	}

	res = mklist(mkstr(str("%s", se)), NULL);
	free(s);

	gcenable();
	gcrderef(&r_res);
	return res;
}

PRIM(frombase) {
	int base, num;

	if(list == NULL || list->next == NULL)
		fail("$&frombase", "missing arguments");

	base = (int)strtol(getstr(list->term), NULL, 10);
	if(base == 0){
		switch(errno){
		case EINVAL:
			fail("$&frombase", str("invalid input: $1 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&frombase", str("conversion overflow: $1 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	if(base < 2)
		fail("&frombase", "base < 2");
	if(base > 16)
		fail("&frombase", "base > 16");

	num = (int)strtol(getstr(list->next->term), NULL, base);
	if(num == 0){
		switch(errno){
		case EINVAL:
			fail("$&frombase", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&frombase", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}
	return mklist(mkstr(str("%d", num)), NULL);
}

PRIM(range) {
	int start, end;
	int i;
	List *res = NULL; Root r_res;

	if(list == NULL || list->next == NULL)
		fail("$&range", "missing arguments");

	start = (int)strtol(getstr(list->term), NULL, 10);
	if(start == 0){
		switch(errno){
		case EINVAL:
			fail("$&range", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&range", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	end = (int)strtol(getstr(list->next->term), NULL, 10);
	if(start == 0){
		switch(errno){
		case EINVAL:
			fail("$&range", str("invalid input: $2 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&range", str("conversion overflow: $2 = '%s'", getstr(list->term)));
			break;
		}
	}

	if(start > end)
		fail("$&range", "start > end");

	gcref(&r_res, (void**)&res);

	for(i = end; i >= start; i--)
		res = mklist(mkstr(str("%d", i)), res);

	gcderef(&r_res, (void**)&res);
	return res;
}

PRIM(reverse) {
	List *l = NULL;
	List *res = NULL; Root r_res;

	if(list == NULL)
		return NULL;

	gcref(&r_res, (void**)&res);

	for(l = list; l != NULL; l = l->next)
		res = mklist(l->term, res);

	gcderef(&r_res, (void**)&res);
	return res;
}

PRIM(unixtime) {
	unsigned long curtime;

	curtime = time(NULL);
	return mklist(mkstr(str("%lud", curtime)), NULL);
}

/*
 * initialization
 */

extern Dict *initprims_etc(Dict *primdict) {
	X(echo);
	X(count);
	X(version);
	X(buildstring);
	X(exec);
	X(dot);
	X(flatten);
	X(whatis);
	X(sethistory);
	X(getlast);
	X(split);
	X(fsplit);
	X(var);
	X(parse);
	X(batchloop);
	X(collect);
	X(home);
	X(setnoexport);
	X(vars);
	X(internals);
	X(result);
	X(isinteractive);
	X(exitonfalse);
	X(noreturn);
	X(catch_noreturn);
	X(setmaxevaldepth);
	X(getevaldepth);
#if READLINE
	X(resetterminal);
	X(addhistory);
	X(clearhistory);
#endif
	/* math and numerical functions */
	X(add);
	X(sub);
	X(mul);
	X(div);
	X(mod);
	X(eq);
	X(gt);
	X(lt);
	X(tobase);
	X(frombase);

	X(range);
	X(reverse);
	X(unixtime);
	return primdict;
}

