/* eval.c -- evaluation of lists and trees ($Revision: 1.2 $) */

#include "es.h"
/* #include "eval.h" */

unsigned long evaldepth = 0, maxevaldepth = MAXmaxevaldepth;

void
failexec(char *file, List *args)
{
	List *fn;
	int olderror;
	List *list; Root r_list;
	Root r_file;

	assert(gcisblocked());
	fn = varlookup("fn-%exec-failure", NULL);
	if (fn != NULL) {
		olderror = errno;
		list = append(fn, mklist(mkstr(file), args));
		gcref(&r_list, (void**)&list);
		gcref(&r_file, (void**)&file);
		gcenable();
		gcderef(&r_file, (void**)&file);
		eval(list, NULL, 0);
		gcderef(&r_list, (void**)&list);
		errno = olderror;
	}
	eprint("%s: %s\n", file, esstrerror(errno));
	exit(1);
}

/* forkexec -- fork (if necessary) and exec */
List*
forkexec(char *file, List *list, Boolean inchild)
{
	int pid, status;
	Vector *env;

	gcdisable();
	env = mkenv();
	pid = efork(!inchild, FALSE);
	if (pid == 0) {
		execve(file, vectorize(list)->vector, env->vector);
		failexec(file, list);
	}
	gcenable();
	status = ewaitfor(pid);
	if ((status & 0xff) == 0) {
		sigint_newline = FALSE;
		SIGCHK();
		sigint_newline = TRUE;
	} else
		SIGCHK();
	printstatus(0, status);
	return mklist(mkterm(mkstatus(status), NULL), NULL);
}

/* assign -- bind a list of values to a list of variables */
List*
assign(Tree *varform, Tree *valueform0, Binding *binding0)
{
	List *value;
	List *result; Root r_result;
	Tree *valueform; Root r_valueform;
	Binding *binding; Root r_binding;
	List *vars; Root r_vars;
	List *values; Root r_values;
	char *name; Root r_name;

	result = NULL;
	gcref(&r_result, (void**)&result);

	valueform = valueform0;
	gcref(&r_valueform, (void**)&valueform);

	binding = binding0;
	gcref(&r_binding, (void**)&binding);

	vars = glom(varform, binding, FALSE);
	gcref(&r_vars, (void**)&vars);

	if (vars == NULL)
		fail("es:assign", "null variable name");

	values = glom(valueform, binding, TRUE);
	gcref(&r_values, (void**)&values);
	result = values;

	for (; vars != NULL; vars = vars->next) {
		name = getstr(vars->term);
		gcref(&r_name, (void**)&name);
		if (values == NULL)
			value = NULL;
		else if (vars->next == NULL || values->next == NULL) {
			value = values;
			values = NULL;
		} else {
			value = mklist(values->term, NULL);
			values = values->next;
		}
		vardef(name, binding, value);
		gcderef(&r_name, (void**)&name);
	}

	gcderef(&r_values, (void**)&values);
	gcderef(&r_vars, (void**)&vars);
	gcderef(&r_binding, (void**)&binding);
	gcderef(&r_valueform, (void**)&valueform);
	gcderef(&r_result, (void**)&result);
	return result;
}

/* letbindings -- create a new Binding containing let-bound variables */
Binding*
letbindings1(Tree *defn0, Binding *outer0,
		Binding *context0, int evalflags, int letstar)
{
	Binding *binding; Root r_binding;
	Binding *context; Root r_context;
	Tree *defn; Root r_defn;
	Tree *assign; Root r_assign;
	List *vars; Root r_vars;
	List *values; Root r_values;
	char *name; Root r_name;
	List *value;

	binding = outer0;
	gcref(&r_binding, (void**)&binding);
	context = context0;
	gcref(&r_context, (void**)&context);
	defn = defn0;
	gcref(&r_defn, (void**)&defn);

	for (; defn != NULL; defn = defn->u[1].p) {
		assert(defn->kind == nList);
		if (defn->u[0].p == NULL)
			continue;

		assign = defn->u[0].p;
		gcref(&r_assign, (void**)&assign);
		assert(assign->kind == nAssign);
		vars = glom(assign->u[0].p, context, FALSE);
		gcref(&r_vars, (void**)&vars);
		values = glom(assign->u[1].p, context, TRUE);
		gcref(&r_values, (void**)&values);

		if (vars == NULL)
			fail("es:let", "null variable name");

		for (; vars != NULL; vars = vars->next) {
			name = getstr(vars->term);
			gcref(&r_name, (void**)&name);
			if (values == NULL)
				value = NULL;
			else if (vars->next == NULL || values->next == NULL) {
				value = values;
				values = NULL;
			} else {
				value = mklist(values->term, NULL);
				values = values->next;
			}
			binding = mkbinding(name, value, binding);
			if(letstar)
				context = mkbinding(name, value, binding);
			gcrderef(&r_name);
		}
		gcrderef(&r_values);
		gcrderef(&r_vars);
		gcrderef(&r_assign);
	}
	gcrderef(&r_defn);
	gcrderef(&r_context);
	gcrderef(&r_binding);
	return binding;
}

Binding*
letbindings(Tree *defn, Binding *outer, Binding *context, int evalflags)
{
	return letbindings1(defn, outer, context, evalflags, 0);
}

Binding*
letsbindings(Tree *defn, Binding *outer, Binding *context, int evalflags)
{
	return letbindings1(defn, outer, context, evalflags, 1);
}

/* localbind -- recursively convert a Bindings list into dynamic binding */
List*
localbind(Binding *dynamic0, Binding *lexical0, Tree *body0, int evalflags)
{
	Push p;
	List *result; Root r_result;
	Tree *body; Root r_body;
	Binding *dynamic; Root r_dynamic;
	Binding *lexical; Root r_lexical;

	if (dynamic0 == NULL)
		return walk(body0, lexical0, evalflags);
	else {
		result = NULL;
		gcref(&r_result, (void**)&result);
		body = body0;
		gcref(&r_body, (void**)&body);
		dynamic = dynamic0;
		gcref(&r_dynamic, (void**)&dynamic);
		lexical = lexical0;
		gcref(&r_lexical, (void**)&lexical);

		varpush(&p, dynamic->name, dynamic->defn);
		result = localbind(dynamic->next, lexical, body, evalflags);
		varpop(&p);

		gcrderef(&r_lexical);
		gcrderef(&r_dynamic);
		gcrderef(&r_body);
		gcrderef(&r_result);
		return result;
	}
}
	
/* local -- build, recursively, one layer of local assignment */
List*
local(Tree *defn, Tree *body0, Binding *bindings0, int evalflags)
{
	Ref(List *, result, NULL);
	Ref(Tree *, body, body0);
	Ref(Binding *, bindings, bindings0);
	Ref(Binding *, dynamic,
	    reversebindings(letbindings(defn, NULL, bindings, evalflags)));

	result = localbind(dynamic, bindings, body, evalflags);

	RefEnd3(dynamic, bindings, body);
	RefReturn(result);
}

/* forloop -- evaluate a for loop */
List*
forloop(Tree *defn0, Tree *body0, Binding *binding, int evalflags)
{
	static List MULTIPLE = { NULL, NULL };

	Ref(List *, result, list_true);
	Ref(Binding *, outer, binding);
	Ref(Binding *, looping, NULL);
	Ref(Tree *, body, body0);

	Ref(Tree *, defn, defn0);
	for (; defn != NULL; defn = defn->u[1].p) {
		assert(defn->kind == nList);
		if (defn->u[0].p == NULL)
			continue;
		Ref(Tree *, assign, defn->u[0].p);
		assert(assign->kind == nAssign);
		Ref(List *, vars, glom(assign->u[0].p, outer, FALSE));
		Ref(List *, list, glom(assign->u[1].p, outer, TRUE));
		if (vars == NULL)
			fail("es:for", "null variable name");
		for (; vars != NULL; vars = vars->next) {
			char *var = getstr(vars->term);
			looping = mkbinding(var, list, looping);
			list = &MULTIPLE;
		}
		RefEnd3(list, vars, assign);
		SIGCHK();
	}
	looping = reversebindings(looping);
	RefEnd(defn);

	ExceptionHandler

		for (;;) {
			Boolean allnull = TRUE;
			Ref(Binding *, bp, outer);
			Ref(Binding *, lp, looping);
			Ref(Binding *, sequence, NULL);
			for (; lp != NULL; lp = lp->next) {
				Ref(List *, value, NULL);
				if (lp->defn != &MULTIPLE)
					sequence = lp;
				assert(sequence != NULL);
				if (sequence->defn != NULL) {
					value = mklist(sequence->defn->term,
						       NULL);
					sequence->defn = sequence->defn->next;
					allnull = FALSE;
				}
				bp = mkbinding(lp->name, value, bp);
				RefEnd(value);
			}
			RefEnd2(sequence, lp);
			if (allnull) {
				RefPop(bp);
				break;
			}
			result = walk(body, bp, evalflags & eval_exitonfalse);
			RefEnd(bp);
			SIGCHK();
		}

	CatchException (e)

		if (!termeq(e->term, "break"))
			throw(e);
		result = e->next;

	EndExceptionHandler

	RefEnd3(body, looping, outer);
	RefReturn(result);
}

/* matchpattern -- does the text match a pattern? */
List*
matchpattern(Tree *subjectform0, Tree *patternform0, Binding *binding)
{
	Boolean result;
	List *pattern;
	StrList *quote = NULL;
	Ref(Binding *, bp, binding);
	Ref(Tree *, patternform, patternform0);
	Ref(List *, subject, glom(subjectform0, bp, TRUE));
	pattern = glom2(patternform, bp, &quote);
	result = listmatch(subject, pattern, quote);
	RefEnd3(subject, patternform, bp);
	return result ? list_true : list_false;
}

/* extractpattern -- Like matchpattern, but returns matches */
List*
extractpattern(Tree *subjectform0, Tree *patternform0, Binding *binding)
{
	List *pattern;
	StrList *quote = NULL;
	Ref(List *, result, NULL);
	Ref(Binding *, bp, binding);
	Ref(Tree *, patternform, patternform0);
	Ref(List *, subject, glom(subjectform0, bp, TRUE));
	pattern = glom2(patternform, bp, &quote);
	result = (List *) extractmatches(subject, pattern, quote);
	RefEnd3(subject, patternform, bp);
	RefReturn(result);
}

/* walk -- walk through a tree, evaluating nodes */
inline List*
walk(Tree *tree0, Binding *binding0, int flags)
{
	Tree *volatile tree = tree0;
	Binding *volatile binding = binding0;

	SIGCHK();

top:
	if (tree == NULL)
		return list_true;

	switch (tree->kind) {

	    case nConcat: case nList: case nQword: case nVar: case nVarsub:
	    case nWord: case nThunk: case nLambda: case nCall: case nPrim: {
		List *list;
		Ref(Binding *, bp, binding);
		list = glom(tree, binding, TRUE);
		binding = bp;
		RefEnd(bp);
		return eval(list, binding, flags);
	    }

	    case nAssign:
		return assign(tree->u[0].p, tree->u[1].p, binding);

	    case nLet: case nClosure: case nLets:
		Ref(Tree *, body, tree->u[1].p);
		if(tree->kind == nLets)
			binding = letsbindings(tree->u[0].p, binding, binding, flags);
		else
			binding = letbindings(tree->u[0].p, binding, binding, flags);
		tree = body;
		RefEnd(body);
		goto top;

	    case nLocal:
		return local(tree->u[0].p, tree->u[1].p, binding, flags);

	    case nFor:
		return forloop(tree->u[0].p, tree->u[1].p, binding, flags);
	
	    case nMatch:
		return matchpattern(tree->u[0].p, tree->u[1].p, binding);

	    case nExtract:
		return extractpattern(tree->u[0].p, tree->u[1].p, binding);

	    default:
		panic("walk: bad node kind %s", treekind(tree));

	}
	NOTREACHED;
}

/* bindargs -- bind an argument list to the parameters of a lambda */
Binding*
bindargs(Tree *params, List *args, Binding *binding)
{
	Tree *param;
	List *value;

	if (params == NULL)
		return mkbinding("*", args, binding);

	gcdisable();

	for (; params != NULL; params = params->u[1].p) {
		assert(params->kind == nList);
		param = params->u[0].p;
		assert(param->kind == nWord || param->kind == nQword);
		if (args == NULL)
			value = NULL;
		else if (params->u[1].p == NULL || args->next == NULL) {
			value = args;
			args = NULL;
		} else {
			value = mklist(args->term, NULL);
			args = args->next;
		}
		binding = mkbinding(param->u[0].s, value, binding);
	}

	Ref(Binding *, result, binding);
	gcenable();
	RefReturn(result);
}

/* pathsearch -- evaluate fn %pathsearch + some argument */
List*
pathsearch(Term *term)
{
	List *search, *list;

	search = varlookup("fn-%pathsearch", NULL);
	if (search == NULL)
		fail("es:pathsearch", "%E: fn %%pathsearch undefined", term);

	list = mklist(term, NULL);
	return eval(append(search, list), NULL, 0);
}

/* eval -- evaluate a list, producing a list */
List*
old_eval(List *list0, Binding *binding0, int flags)
{
	Closure *volatile cp;
	List *fn;
	char *error, *binname;

	if (++evaldepth >= maxevaldepth)
		fail("es:eval", "max-eval-depth exceeded");

	Ref(List *, list, list0);
	Ref(Binding *, binding, binding0);
	Ref(char *, funcname, NULL);

restart:
	if (list == NULL) {
		RefPop3(funcname, binding, list);
		--evaldepth;
		return list_true;
	}

	assert(list->term != NULL);

	if ((cp = getclosure(list->term)) != NULL) {
		switch (cp->tree->kind) {
		case nPrim:
			assert(cp->binding == NULL);
			list = prim(cp->tree->u[0].s, list->next, binding, flags);
			break;
		case nThunk:
			list = walk(cp->tree->u[0].p, cp->binding, flags);
			break;
		case nLambda:
			ExceptionHandler

				Push p;
				Ref(Tree *, tree, cp->tree);
				Ref(Binding *, context,
					       bindargs(tree->u[0].p,
							list->next,
							cp->binding));
				if (funcname != NULL)
					varpush(&p, "0",
						    mklist(mkterm(funcname,
								  NULL),
							   NULL));
				list = walk(tree->u[1].p, context, flags);
				if (funcname != NULL)
					varpop(&p);
				RefEnd2(context, tree);
	
			CatchException (e)

				if (termeq(e->term, "return")) {
					list = e->next;
					goto done;
				}
				throw(e);

			EndExceptionHandler
			break;
		case nList:
			list = glom(cp->tree, cp->binding, TRUE);
			list = append(list, list->next);
			goto restart;
		case nConcat:
			Ref(Tree *, t, cp->tree);
			while(t->kind == nConcat)
				t = t->u[0].p;
			if(t->kind == nPrim)
				fail("es:eval", "invalid primitive: %T", cp->tree);
			RefEnd(t);
			break;
		default:
			panic("eval: bad closure node kind %s", treekind(cp->tree));
		}
		goto done;
	}

	/* the logic here is duplicated in $&whatis */

	Ref(char *, name, getstr(list->term));
	fn = varlookup2("fn-", name, binding);
	if (fn != NULL) {
		funcname = name;
		list = append(fn, list->next);
		RefPop(name);
		goto restart;
	}
	if (isabsolute(name)) {
		error = checkexecutable(name);
		if (error != NULL)
			fail("$&whatis", "%s: %s", name, error);
		list = forkexec(name, list, flags & eval_inchild);
		RefPop(name);
		goto done;
	}
	RefEnd(name);

	fn = pathsearch(list->term);
	if (fn != NULL && fn->next == NULL
	    && (cp = getclosure(fn->term)) == NULL) {
		binname = getstr(fn->term);
		list = forkexec(binname, list, flags & eval_inchild);
		goto done;
	}

	list = append(fn, list->next);
	goto restart;

done:
	--evaldepth;
	if ((flags & eval_exitonfalse) && !istrue(list))
		exit(exitstatus(list));
	RefEnd2(funcname, binding);
	RefReturn(list);
}

List*
eval(List *list, Binding *binding, int flags)
{
	return old_eval(list, binding, flags);
}


/* eval1 -- evaluate a term, producing a list */
List*
eval1(Term *term, int flags)
{
	return eval(mklist(term, NULL), NULL, flags);
}

