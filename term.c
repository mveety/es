/* term.c -- operations on terms ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"
#include "stdenv.h"

DefineTag(Term, static);

Term*
mkterm1(char *str, Closure *closure, Dict *dict)
{
	Term *term; Root r_term;

	assert(str != NULL || closure != NULL || dict != NULL);
	gcdisable();
	term = gcnew(Term);
	gcref(&r_term, (void**)&term);
	if(str != NULL)
		*term = (Term){tkString, str, NULL, NULL};
	else if(closure != NULL)
		*term = (Term){tkClosure, NULL, closure, NULL};
	else if(dict != NULL)
		*term = (Term){tkDict, NULL, NULL, dict};
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
	char *string; Root r_string;

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

Closure*
getclosure(Term *term)
{
	Term *tp; Root r_tp;
	Tree *np; Root r_np;

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

char*
getstr(Term *term)
{
	Term *tp; Root r_tp;

	switch(term->kind) {
	case tkString:
		/* TODO: This is wrong, but I still need to hunt down places where
		 * strings are defined improperly
		 */
		if(term->str == NULL){
			tp = term;
			gcref(&r_tp, (void**)&tp);
			tp->str = str("%C", term->closure);
			tp->kind = tkClosure;
			gcderef(&r_tp, (void**)&tp);
		}
		return term->str;
	case tkClosure:
		assert(term->closure != NULL);
		return str("%C", term->closure);
	case tkDict:
		assert(term->dict != NULL);
		return str("dict(%ulx)", term->dict);
	}
	return NULL;
}

Dict*
getdict(Term *term)
{
	if(term->kind == tkDict)
		return term->dict;
	return NULL;
}

Term*
termcat(Term *t1, Term *t2)
{
	Term *term; Root r_term;
	char *str1; Root r_str1;
	char *str2; Root r_str2;

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
	if(t->kind == tkString || t->kind == tkClosure) {
		gcmark(t->closure);
		gcmark(t->str);
	} else if(t->kind == tkDict) {
		gcmark(t->dict);
	}
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
