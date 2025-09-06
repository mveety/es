/* term.c -- operations on terms ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"
#include "stdenv.h"

DefineTag(Term, static);

Term*
mkterm1(char *str, Closure *closure, Dict *dict)
{
	Term *term = NULL; Root r_term;

	assert(str != NULL || closure != NULL || dict != NULL);
	gcdisable();
	term = gcnew(Term);
	gcref(&r_term, (void**)&term);
	if(str != NULL)
		*term = (Term){tkString, ttNone, str, NULL, NULL};
	else if(closure != NULL)
		*term = (Term){tkClosure, ttNone, NULL, closure, NULL};
	else if(dict != NULL)
		*term = (Term){tkDict, ttNone, NULL, NULL, dict};
	gcenable();
	gcderef(&r_term, (void**)&term);
	return term;
}

Term*
mkterm(char *str, Closure *closure)
{
	return mkterm1(str, closure, NULL);
}

Term*
mkstr(char *str)
{
	Term *term;
	char *string = NULL; Root r_string;

	string = str;
	gcref(&r_string, (void**)&string);
	term = mkterm(string, NULL);
	gcderef(&r_string, (void**)&string);
	return term;
}

Term*
mkdictterm(Dict *d)
{
	if(!d)
		return mkterm1(NULL, NULL, mkdict());
	return mkterm1(NULL, NULL, d);
}

int
isfunction(char *s)
{
	if((s[0] == '{' || s[0] == '@') && s[strlen(s) - 1] == '}')
		return 1;
	if(s[0] == '$' && s[1] == '&')
		return 1;
	if(hasprefix(s, "%closure"))
		return 1;
	return 0;
}

int
isadict(char *s)
{
	if(hasprefix(s, "%dict"))
		return 1;
	return 0;
}

Closure*
getclosure(Term *term)
{
	Term *tp = NULL; Root r_tp;
	Tree *np = NULL; Root r_np;

	if(term->kind == tkDict)
		return NULL;
	if (term->closure == NULL) {
		assert(term->str != NULL);
		if(isfunction(term->str)){
			gcref(&r_tp, (void**)&tp);
			gcref(&r_np, (void**)&np);
			tp = term;
			np = parsestring(term->str);
			if (np == NULL) {
				gcrderef(&r_np);
				gcrderef(&r_tp);
				return NULL;
			}
			tp->closure = extractbindings(np);
			tp->str = NULL;
			term = tp;
			gcrderef(&r_np);
			gcrderef(&r_tp);
		}
	}
	return term->closure;
}

typedef struct {
	char *result;
} AssocArgs;

void
assocfmt(void *vargs, char *name, void *vdata)
{
	AssocArgs *args = NULL; Root r_args_result;
	List *data = NULL; Root r_data;

	args = vargs;
	gcref(&r_args_result, (void**)&args->result);
	gcref(&r_data, (void**)&data);
	data = vdata;

	if(args->result == NULL)
		args->result = str("%s => %#L", name, data, " ");
	else
		args->result = str("%s; %s => %#L", args->result, name, data, " ");

	gcrderef(&r_data);
	gcrderef(&r_args_result);
}

char*
getregex(Term *term)
{
	if(term->kind == tkRegex)
		return term->str;
	else
		return getstr(term);
}

char*
getstr(Term *term)
{
	Term *tp = NULL; Root r_tp;
	AssocArgs args; Root r_args_result;
	Dict *d = NULL; Root r_d;
	char *res = NULL; Root r_res;

	if(term->kind == tkString && term->str == NULL){
		/* TODO: This is wrong, but I still need to hunt down places where
		 * strings are defined improperly
		 */
		gcref(&r_tp, (void**)&tp);
		tp = term;
		if(term->closure != NULL){
			tp->str = str("%C", tp->closure);
			tp->kind = tkClosure;
		} else if(term->dict != NULL){
			tp->str = str("%V", tp->dict);
			tp->kind = tkDict;
		}
		gcrderef(&r_tp);
		term = tp;
	}
	switch(term->kind) {
	case tkString:
			return term->str;
	case tkRegex:
			return str("%%re(%#S)", term->str);
	case tkClosure:
		assert(term->closure != NULL);
		return str("%C", term->closure);
	case tkDict:
		assert(term->dict != NULL);
		args.result = NULL;
		gcref(&r_args_result, (void**)&args.result);
		gcref(&r_tp, (void**)&tp);
		gcref(&r_d, (void**)&r_d);
		gcref(&r_res, (void**)&r_res);

		d = term->dict;
		dictforall(d, assocfmt, &args);
		if(args.result == NULL)
			res = str("%%dict()");
		else
			res = str("%%dict(%s)", args.result);

		gcrderef(&r_res);
		gcrderef(&r_d);
		gcrderef(&r_tp);
		gcrderef(&r_args_result);
		return res;
	}
	return NULL;
}

Dict*
getdict(Term *term)
{
	Term *t = NULL; Root r_t;
	Tree *parsed = NULL; Root r_parsed;
	List *glommed = NULL; Root r_glommed;
	Dict *dict = NULL; Root r_dict;

	switch(term->kind){
	case tkDict:
		return term->dict;
	case tkString:
		assert(term->str != NULL);
		if(!isadict(term->str))
			return NULL;
		gcref(&r_t, (void**)&t);
		gcref(&r_parsed, (void**)&parsed);
		gcref(&r_glommed, (void**)&glommed);
		gcref(&r_dict, (void**)&dict);

		t = term;
		parsed = parsestring(t->str);
		if(!parsed)
			goto done;
		glommed = glom(parsed, NULL, FALSE);
		if(glommed == NULL || glommed->next != NULL)
			goto done;
		dict = getdict(glommed->term);
		t->kind = tkDict;
		t->dict = dict;
done:
		gcrderef(&r_dict);
		gcrderef(&r_glommed);
		gcrderef(&r_parsed);
		gcrderef(&r_t);
		return dict;
	default:
		return NULL;
	}
	return NULL;
}

Term*
termcat(Term *t1, Term *t2)
{
	Term *term = NULL; Root r_term;
	char *str1 = NULL; Root r_str1;
	char *str2 = NULL; Root r_str2;

	if (t1 == NULL)
		return t2;
	if (t2 == NULL)
		return t1;

	gcref(&r_term, (void**)&term);
	str1 = getstr(t1);
	gcref(&r_str1, (void**)&str1);
	str2 = getstr(t2);
	gcref(&r_str2, (void**)&str2);

	term = mkstr(str("%s%s", str1, str2));

	gcderef(&r_str2, (void**)&str2);
	gcderef(&r_str1, (void**)&str1);
	gcderef(&r_term, (void**)&term);
	return term;
}


void*
TermCopy(void *op)
{
	void *np;

	np = gcnew(Term);
	memcpy(np, op, sizeof(Term));
	return np;
}

size_t
TermScan(void *p)
{
	Term *term;

	term = p;
	term->closure = forward(term->closure);
	term->str = forward(term->str);
	term->dict = forward(term->dict);
	return sizeof(Term);
}

void
TermMark(void *p)
{
	Term *t;

	t = (Term*)p;
	gc_set_mark(header(p));
	gcmark(t->closure);
	gcmark(t->str);
	gcmark(t->dict);
}

extern Boolean termeq(Term *term, const char *s) {
	assert(term != NULL);
	if (term->str == NULL)
		return FALSE;
	return streq(term->str, s);
}

extern Boolean isclosure(Term *term) {
	assert(term != NULL);
	return term->closure != NULL;
}

Boolean
isdict(Term *term)
{
	assert(term != NULL);
	if(term->dict == NULL && !isadict(getstr(term)))
		return FALSE;
	return TRUE;
}

