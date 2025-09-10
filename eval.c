/* eval.c -- evaluation of lists and trees ($Revision: 1.2 $) */

#include <es.h>
#include <regex.h>
/* #include <eval.h> */

unsigned long evaldepth = 0, maxevaldepth = MAXmaxevaldepth;
extern Boolean verbose_match;

void
failexec(char *file, List *args)
{
	List *fn;
	int olderror;
	List *list = nil; Root r_list;
	Root r_file;

	assert(gcisblocked());
	fn = varlookup("fn-%exec-failure", nil);
	if (fn != nil) {
		olderror = errno;
		list = append(fn, mklist(mkstr(file), args));
		gcref(&r_list, (void**)&list);
		gcref(&r_file, (void**)&file);
		gcenable();
		gcderef(&r_file, (void**)&file);
		eval(list, nil, 0);
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
	return mklist(mkterm(mkstatus(status), nil), nil);
}

List*
dictdestructassign(Tree *vardict0, Tree *valueform0, Binding *binding0)
{
	Tree *vardict = vardict0; Root r_vardict;
	Tree *valueform = valueform0; Root r_valueform;
	Binding *binding = binding0; Root r_binding;

	Tree *dictform = nil; Root r_dictform;
	Tree *assoc = nil; Root r_assoc;
	List *valuelist = nil; Root r_valuelist;
	Dict *valuedict = nil; Root r_valuedict;
	List *namelist = nil; Root r_namelist;
	List *varlist = nil; Root r_varlist;
	List *dictdata = nil; Root r_dictdata;

	gcref(&r_vardict, (void**)&vardict);
	gcref(&r_valueform, (void**)&valueform);
	gcref(&r_binding, (void**)&binding);

	gcref(&r_dictform, (void**)&dictform);
	gcref(&r_assoc, (void**)&assoc);
	gcref(&r_valuelist, (void**)&valuelist);
	gcref(&r_valuedict, (void**)&valuedict);
	gcref(&r_namelist, (void**)&namelist);
	gcref(&r_varlist, (void**)&varlist);
	gcref(&r_dictdata, (void**)&dictdata);

	dictform = vardict->u[0].p;

	valuelist = glom(valueform, binding, TRUE);
	if(valuelist == nil)
		fail("es:dictassign", "null values");
	if(valuelist->term->kind != tkDict)
		fail("es:dictassign", "rhs value not a dict");
	if(valuelist->next != nil)
		fail("es:dictassign", "too many rhs elements");
	valuedict = getdict(valuelist->term);

	for(dictform = vardict->u[0].p; dictform != nil; dictform = dictform->u[1].p){
		assoc = dictform->u[0].p;
		assert(assoc != nil && assoc->kind == nAssoc);
		if(assoc->u[1].p == nil)
			continue;
		namelist = glom(assoc->u[0].p, binding, FALSE);
		varlist = glom(assoc->u[1].p, binding, TRUE);
		if(namelist == nil || varlist == nil)
			continue;
		if(namelist->next != nil)
			fail("es:dictassign", "element name should not be a list");
		dictdata = (List*)dictget(valuedict, getstr(namelist->term));
		if(dictdata == nil)
			fail("es:dictassign", "element %s is empty", getstr(namelist->term));
		for(; varlist != nil; varlist = varlist->next){
			if(varlist->next == nil) {
				if(!termeq(varlist->term, "_"))
					vardef(getstr(varlist->term), binding, dictdata);
			} else {
				if(dictdata == nil) {
					if(!termeq(varlist->term, "_"))
						vardef(getstr(varlist->term), binding, nil);
				} else {
					if(!termeq(varlist->term, "_"))
						vardef(getstr(varlist->term), binding, mklist(dictdata->term, nil));
					dictdata = dictdata->next;
				}
			}
		}
	}

	gcrderef(&r_dictdata);
	gcrderef(&r_varlist);
	gcrderef(&r_namelist);
	gcrderef(&r_valuedict);
	gcrderef(&r_valuelist);
	gcrderef(&r_assoc);
	gcrderef(&r_dictform);

	gcrderef(&r_binding);
	gcrderef(&r_valueform);
	gcrderef(&r_vardict);

	return valuelist;
}

/* assign -- bind a list of values to a list of variables */
List*
assign(Tree *varform, Tree *valueform0, Binding *binding0)
{
	List *value;
	List *result = nil; Root r_result;
	Tree *valueform = nil; Root r_valueform;
	Binding *binding = nil; Root r_binding;
	List *vars = nil; Root r_vars;
	List *values = nil; Root r_values;
	char *name = nil; Root r_name;

	if(varform->kind == nDict)
		return dictdestructassign(varform, valueform0, binding0);

	result = nil;
	gcref(&r_result, (void**)&result);

	valueform = valueform0;
	gcref(&r_valueform, (void**)&valueform);

	binding = binding0;
	gcref(&r_binding, (void**)&binding);

	vars = glom(varform, binding, FALSE);
	gcref(&r_vars, (void**)&vars);

	if (vars == nil)
		fail("es:assign", "null variable name");

	values = glom(valueform, binding, TRUE);
	gcref(&r_values, (void**)&values);
	result = values;

	for (; vars != nil; vars = vars->next) {
		name = getstr(vars->term);
		gcref(&r_name, (void**)&name);
		if (values == nil)
			value = nil;
		else if (vars->next == nil || values->next == nil) {
			value = values;
			values = nil;
		} else {
			value = mklist(values->term, nil);
			values = values->next;
		}
		if(!termeq(vars->term, "_"))
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
letbindings1(Tree *defn0, Binding *outer0, Binding *context0, int evalflags, int letstar)
{
	Binding *binding = nil; Root r_binding;
	Binding *context = nil; Root r_context;
	Tree *defn = nil; Root r_defn;
	Tree *assign = nil; Root r_assign;
	List *vars = nil; Root r_vars;
	List *values = nil; Root r_values;
	char *name = nil; Root r_name;
	List *value;

	binding = outer0;
	gcref(&r_binding, (void**)&binding);
	context = context0;
	gcref(&r_context, (void**)&context);
	defn = defn0;
	gcref(&r_defn, (void**)&defn);

	for (; defn != nil; defn = defn->u[1].p) {
		assert(defn->kind == nList);
		if (defn->u[0].p == nil)
			continue;

		assign = defn->u[0].p;
		gcref(&r_assign, (void**)&assign);
		assert(assign->kind == nAssign);
		vars = glom(assign->u[0].p, context, FALSE);
		gcref(&r_vars, (void**)&vars);
		values = glom(assign->u[1].p, context, TRUE);
		gcref(&r_values, (void**)&values);

		if (vars == nil)
			fail("es:let", "null variable name");

		for (; vars != nil; vars = vars->next) {
			name = getstr(vars->term);
			gcref(&r_name, (void**)&name);
			if (values == nil)
				value = nil;
			else if (vars->next == nil || values->next == nil) {
				value = values;
				values = nil;
			} else {
				value = mklist(values->term, nil);
				values = values->next;
			}
			if(!termeq(vars->term, "_"))
				binding = mkbinding(name, value, binding);
			if(letstar && !termeq(vars->term, "_"))
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
	List *result = nil; Root r_result;
	Tree *body = nil; Root r_body;
	Binding *dynamic = nil; Root r_dynamic;
	Binding *lexical = nil; Root r_lexical;

	if (dynamic0 == nil)
		return walk(body0, lexical0, evalflags);
	else {
		result = nil;
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
			Boolean underscore = FALSE;
			Ref(Binding *, bp, outer);
			Ref(Binding *, lp, looping);
			Ref(Binding *, sequence, NULL);
			for (; lp != NULL; lp = lp->next) {
				underscore = FALSE;
				if(strcmp(lp->name, "_") == 0)
					underscore = TRUE;
				Ref(List *, value, NULL);
				if (lp->defn != &MULTIPLE)
					sequence = lp;
				assert(sequence != NULL);
				if (sequence->defn != NULL) {
					if(underscore == FALSE)
						value = mklist(sequence->defn->term, NULL);
					sequence->defn = sequence->defn->next;
					allnull = FALSE;
				}
				if(underscore == FALSE)
					bp = mkbinding(lp->name, value, bp);
				RefEnd(value);
			}
			RefEnd2(sequence, lp);
			if (allnull) {
				RefPop(bp);
				break;
			}

			ExceptionHandler
				result = walk(body, bp, evalflags & eval_exitonfalse);
			CatchException (e)
				if(!termeq(e->term, "continue"))
					throw(e);
			EndExceptionHandler

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
	Binding *bp = binding; Root r_bp;
	Tree *patternform = patternform0; Root r_patternform;
	List *subject = nil; Root r_subject;
	List *sp = nil; Root r_sp;
	List *pattern = nil; Root r_pattern;
	List *lp = nil; Root r_lp;
	List *pattern1 = nil; Root r_pattern1;
	Boolean result = FALSE;
	Boolean hasregex = FALSE;
	StrList *quote = nil; Root r_quote;
	StrList *quote1 = nil; Root r_quote1;
	StrList *ql = nil; Root r_ql;
	StrList *ql1 = nil; Root r_ql1;
	StrList *tmp = nil; Root r_tmp;
	RegexStatus status;
	char errstr[128];

	gcref(&r_bp, (void**)&bp);
	gcref(&r_patternform, (void**)&patternform);
	gcref(&r_subject, (void**)&subject);
	gcref(&r_sp, (void**)&sp);
	gcref(&r_pattern, (void**)&pattern);
	gcref(&r_lp, (void**)&lp);
	gcref(&r_pattern1, (void**)&pattern1);
	gcref(&r_quote, (void**)&quote);
	gcref(&r_quote1, (void**)&quote1);
	gcref(&r_ql, (void**)&ql);
	gcref(&r_ql1, (void**)&ql1);
	gcref(&r_tmp, (void**)&tmp);

	subject = glom(subjectform0, bp, TRUE);
	pattern = glom2(patternform, bp, &quote);

	for(lp = pattern, ql = quote; lp != nil; lp = lp->next, ql = ql->next){
		if(lp->term->kind == tkRegex){
			hasregex = TRUE;
			for(sp = subject; sp != nil; sp = sp->next){
				memset(errstr, 0, sizeof(errstr)); /* I should probably not set errstr on a match fail */
				status = (RegexStatus){ReNil, FALSE, 0, 0, nil, 0, &errstr[0], sizeof(errstr)};
				regexmatch(&status, sp->term, lp->term);
				assert(status.type == ReMatch);

				if(status.compcode)
					fail("es:rematch", "compilation error: %s", errstr);
				if(status.matchcode != 0 && status.matchcode != REG_NOMATCH)
					fail("es:rematch", "match error: %s", errstr);

				if(status.matched == TRUE){
					result = status.matched;
					goto done;
				}
			}
		} else {
			pattern1 = mklist(lp->term, pattern1);
			if(quote1 == nil){
				quote1 = mkstrlist(ql->str, nil);
				ql1 = quote1;
			} else {
				tmp = mkstrlist(ql->str, nil);
				assert(tmp != nil); /* problems with gcc's optimizer? */
				ql1->next = tmp;
				ql1 = ql1->next;
				assert(ql1 != nil);
			}
		}
	}

	pattern1 = reverse(pattern1);
	if(hasregex){
		if(verbose_match)
			eprint("quote = [%Z], quote1 = [%Z], subject = (%L), pattern1 = (%L)\n",
				quote, " ", quote1, ", ", subject, " ", pattern1, " ");
	} else
		quote1 = quote;
	result = listmatch(subject, pattern1, quote1);

done:
	gcrderef(&r_tmp);
	gcrderef(&r_ql1);
	gcrderef(&r_ql);
	gcrderef(&r_quote1);
	gcrderef(&r_quote);
	gcrderef(&r_pattern1);
	gcrderef(&r_lp);
	gcrderef(&r_pattern);
	gcrderef(&r_sp);
	gcrderef(&r_subject);
	gcrderef(&r_patternform);
	gcrderef(&r_bp);

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
eval(List *list0, Binding *binding0, int flags)
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

/* eval1 -- evaluate a term, producing a list */
List*
eval1(Term *term, int flags)
{
	return eval(mklist(term, NULL), NULL, flags);
}

