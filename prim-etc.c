/* prim-etc.c -- miscellaneous primitives ($Revision: 1.2 $) */

#include "es.h"
#include "prim.h"

PRIM(result) {
	return list;
}

PRIM(echo) {
	const char *eol = "\n";
	if(list != NULL) {
		if(termeq(list->term, "-n")) {
			eol = "";
			list = list->next;
		} else if(termeq(list->term, "--"))
			list = list->next;
	}
	print("%L%s", list, " ", eol);
	return list_true;
}

PRIM(count) {
	if(list != NULL && list->next == NULL && list->term->kind == tkDict)
		return prim("dictsize", list, binding, evalflags);
	return mklist(mkstr(str("%d", length(list))), NULL);
}

PRIM(listcount) {
	return mklist(mkstr(str("%d", length(list))), nil);
}

PRIM(setnoexport) {
	Ref(List *, lp, list);
	setnoexport(lp);
	RefReturn(lp);
}

PRIM(exec) {
	return eval(list, NULL, evalflags | eval_inchild);
}

PRIM(dot) {
	int c, fd;
	Push zero, star;
	volatile int runflags = (evalflags & eval_inchild);
	const char *const usage = ". [-einvxLX] file [arg ...]";

	esoptbegin(list, "$&dot", usage);
	while((c = esopt("einvxLX")) != EOF)
		switch(c) {
		case 'e':
			runflags |= eval_exitonfalse;
			break;
		case 'i':
			runflags |= run_interactive;
			break;
		case 'n':
			runflags |= run_noexec;
			break;
		case 'v':
			runflags |= run_echoinput;
			break;
		case 'x':
			runflags |= run_printcmds;
			break;
		case 'L':
			runflags |= run_lisptrees;
			break;
		case 'X':
			runflags |= run_noforkexec;
			break;
		}

	Ref(List *, result, NULL);
	Ref(List *, lp, esoptend());
	if(lp == NULL)
		fail("$&dot", "usage: %s", usage);

	Ref(char *, file, getstr(lp->term));
	lp = lp->next;
	fd = eopen(file, oOpen);
	if(fd == -1)
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
	if(list == NULL)
		fail("$&flatten", "usage: %%flatten separator [args ...]");
	Ref(List *, lp, list);
	sep = getstr(lp->term);
	lp = mklist(mkstr(str("%L", lp->next, sep)), NULL);
	RefReturn(lp);
}

PRIM(whatis) {
	/* the logic in here is duplicated in eval() */
	if(list == NULL || list->next != NULL)
		fail("$&whatis", "usage: $&whatis program");
	Ref(Term *, term, list->term);
	if(getclosure(term) == NULL) {
		List *fn;
		Ref(char *, prog, getstr(term));
		assert(prog != NULL);
		fn = varlookup2("fn-", prog, binding);
		if(fn != NULL)
			list = fn;
		else {
			if(isabsolute(prog)) {
				char *error = checkexecutable(prog);
				if(error != NULL)
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
	if(list == NULL)
		fail("$&split", "usage: %%split separator [args ...]");
	Ref(List *, lp, list);
	sep = getstr(lp->term);
	lp = fsplit(sep, lp->next, TRUE);
	RefReturn(lp);
}

PRIM(fsplit) {
	char *sep;
	if(list == NULL)
		fail("$&fsplit", "usage: %%fsplit separator [args ...]");
	Ref(List *, lp, list);
	sep = getstr(lp->term);
	lp = fsplit(sep, lp->next, FALSE);
	RefReturn(lp);
}

PRIM(var) {
	Term *term;
	if(list == NULL)
		return NULL;
	Ref(List *, rest, list->next);
	Ref(char *, name, getstr(list->term));
	Ref(List *, defn, varlookup(name, NULL));
	rest = prim_var(rest, NULL, evalflags);
	term = mkstr(str("%S = %V", name, defn, " "));
	list = mklist(term, rest);
	RefEnd3(defn, name, rest);
	return list;
}

PRIM(parse) {
	List *result;
	Tree *tree;
	Ref(char *, prompt1, NULL);
	Ref(char *, prompt2, NULL);
	Ref(List *, lp, list);
	if(lp != NULL) {
		prompt1 = getstr(lp->term);
		if((lp = lp->next) != NULL)
			prompt2 = getstr(lp->term);
	}
	RefEnd(lp);
	tree = parse(prompt1, prompt2);
	result =
		(tree == NULL) ? NULL : mklist(mkterm(NULL, mkclosure(gcmk(nThunk, tree), NULL)), NULL);
	RefEnd2(prompt2, prompt1);
	return result;
}

PRIM(exitonfalse) {
	return eval(list, NULL, evalflags | eval_exitonfalse);
}

PRIM(batchloop) {
	Ref(List *, result, list_true);
	Ref(List *, dispatch, NULL);

	sigchk();

	ExceptionHandler {
		for(;;) {
			List *parser, *cmd;
			parser = varlookup("fn-%parse", NULL);
			cmd = (parser == NULL) ? prim("parse", NULL, NULL, 0) : eval(parser, NULL, 0);
			sigchk();
			dispatch = varlookup("fn-%dispatch", NULL);
			if(cmd != NULL) {
				if(dispatch != NULL)
					cmd = append(dispatch, cmd);
				result = eval(cmd, NULL, evalflags);
				sigchk();
			}
		}
	} CatchException (e) {
		if(!termeq(e->term, "eof"))
			throw(e);
		RefEnd(dispatch);
		if(result == list_true)
			result = list_true;
		RefReturn(result);
	} EndExceptionHandler;
}

PRIM(collect) {
	gc();
	return list_true;
}

PRIM(home) {
	struct passwd *pw;
	if(list == NULL)
		return varlookup("home", NULL);
	if(list->next != NULL)
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
	if(list == NULL)
		fail("$&noreturn", "usage: $&noreturn lambda args ...");
	Ref(List *, lp, list);
	Ref(Closure *, closure, getclosure(lp->term));
	if(closure == NULL || closure->tree->kind != nLambda)
		fail("$&noreturn", "$&noreturn: %E is not a lambda", lp->term);
	Ref(Tree *, tree, closure->tree);
	Ref(Binding *, context, bindargs(tree->u[0].p, lp->next, closure->binding));
	lp = walk(tree->u[1].p, context, evalflags);
	RefEnd3(context, tree, closure);
	RefReturn(lp);
}

PRIM(catch_noreturn) {
	if(list == NULL)
		fail("$&catch_noreturn", "usage: $&catch_noreturn lambda args ...");
	Ref(List *, lp, list);
	Ref(Closure *, closure, getclosure(lp->term));
	if(closure == NULL || closure->tree->kind != nLambda)
		fail("$&catch_noreturn", "$&catch_noreturn: %E is not a lambda", lp->term);
	Ref(Tree *, tree, closure->tree);
	Ref(Binding *, context, bindargs(tree->u[0].p, lp->next, closure->binding));
	lp = walk(tree->u[1].p, context, evalflags);
	RefEnd3(context, tree, closure);
	RefReturn(lp);
}

PRIM(withbindings) {
	Binding *ctx = nil; Root r_ctx;
	Binding *bp = nil; Root r_bp;
	List *lp = nil; Root r_lp;
	Closure *fun = nil; Root r_fun;
	Tree *funbody = nil; Root r_funbody;

	if(list == nil)
		fail("$&withbindings", "usage: $&withbindings lambda args...");

	gcref(&r_lp, (void **)&lp);
	gcref(&r_ctx, (void **)&ctx);
	gcref(&r_bp, (void **)&bp);
	gcref(&r_fun, (void **)&fun);
	gcref(&r_funbody, (void **)&funbody);

	lp = list;
	if(!(fun = getclosure(lp->term)))
		fail("$&withbindings", "$&withbindings: %E is not a lambda", lp->term);

	funbody = fun->tree;
	ctx = binding;
	for(bp = fun->binding; bp != nil; bp = bp->next)
		ctx = mkbinding(bp->name, bp->defn, ctx);

	ctx = bindargs(funbody->u[0].p, lp->next, ctx);

	ExceptionHandler {
		lp = walk(funbody->u[1].p, ctx, evalflags);
	} CatchException (e) {
		if(termeq(e->term, "return"))
			lp = e->next;
		else
			throw(e);
	} EndExceptionHandler;

	gcrderef(&r_funbody);
	gcrderef(&r_fun);
	gcrderef(&r_bp);
	gcrderef(&r_ctx);
	gcrderef(&r_lp);

	return lp;
}


PRIM(setmaxevaldepth) {
	char *s;
	long n;
	if(list == NULL) {
		maxevaldepth = MAXmaxevaldepth;
		return NULL;
	}
	if(list->next != NULL)
		fail("$&setmaxevaldepth", "usage: $&setmaxevaldepth [limit]");
	Ref(List *, lp, list);
	n = strtol(getstr(lp->term), &s, 0);
	if(n < 0 || (s != NULL && *s != '\0'))
		fail("$&setmaxevaldepth", "max-eval-depth must be set to a positive integer");
	if(n < MINmaxevaldepth)
		n = (n == 0) ? MAXmaxevaldepth : MINmaxevaldepth;
	maxevaldepth = n;
	RefReturn(lp);
}

PRIM(resetterminal) {
	resetterminal = TRUE;
	return list_true;
}

/*
 * initialization
 */

Primitive prim_etc[] = {
	DX(echo),		   DX(count),
	DX(listcount),	   DX(exec),
	DX(dot),		   DX(flatten),
	DX(whatis),		   DX(split),
	DX(fsplit),		   DX(var),
	DX(parse),		   DX(batchloop),
	DX(collect),	   DX(home),
	DX(setnoexport),   DX(vars),
	DX(internals),	   DX(result),
	DX(isinteractive), DX(exitonfalse),
	DX(noreturn),	   DX(catch_noreturn),
	DX(withbindings),  DX(setmaxevaldepth),
	DX(resetterminal),
};

extern Dict *
initprims_etc(Dict *primdict)
{
	size_t i = 0;

	for(i = 0; i < nelem(prim_etc); i++)
		primdict = dictput(primdict, prim_etc[i].name, (void *)prim_etc[i].symbol);

	return primdict;
}
