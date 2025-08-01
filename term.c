/* term.c -- operations on terms ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"

DefineTag(Term, static);

Term*
mkterm(char *str, Closure *closure)
{
	Term *term; Root r_term;

	assert(str != NULL || closure != NULL);
	gcdisable();
	term = gcnew(Term);
	gcref(&r_term, (void**)&term);
	if(str != NULL)
		*term = (Term){tkString, str, NULL, NULL};
	else if(closure != NULL)
		*term = (Term){tkClosure, NULL, closure, NULL};
	gcenable();
	gcderef(&r_term, (void**)&term);
	return term;
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

extern Closure *getclosure(Term *term) {
	if (term->closure == NULL) {
		char *s = term->str;
		assert(s != NULL);
		if (
			((*s == '{' || *s == '@') && s[strlen(s) - 1] == '}')
			|| (*s == '$' && s[1] == '&')
			|| hasprefix(s, "%closure")
		) {
			Ref(Term *, tp, term);
			Ref(Tree *, np, parsestring(s));
			if (np == NULL) {
				RefPop2(np, tp);
				return NULL;
			}
			tp->closure = extractbindings(np);
			tp->str = NULL;
			term = tp;
			RefEnd2(np, tp);
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
			gcderef(&r_tp, (void**)&tp);
		}
		return term->str;
	case tkClosure:
		assert(term->closure != NULL);
		return str("%C", term->closure);
	case tkDict:
		assert(term->dict != NULL);
		return str("dict%p", term->dict);
	}
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
