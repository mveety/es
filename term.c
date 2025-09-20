/* term.c -- operations on terms ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"
#include "stdenv.h"

DefineTag(Term, static);

Term *
mkterm1(char *str, Closure *closure, Dict *dict)
{
	Term *term = nil; Root r_term;

	assert(str != nil || closure != nil || dict != nil);
	gcdisable();
	term = gcnew(Term);
	gcref(&r_term, (void **)&term);
	if(str != nil)
		*term = (Term){tkString, ttNone, str, nil, nil};
	else if(closure != nil)
		*term = (Term){tkClosure, ttNone, nil, closure, nil};
	else if(dict != nil)
		*term = (Term){tkDict, ttNone, nil, nil, dict};
	gcenable();
	gcderef(&r_term, (void **)&term);
	return term;
}

Term *
mkterm(char *str, Closure *closure)
{
	return mkterm1(str, closure, nil);
}

Term *
mkstr(char *str)
{
	Term *term;
	char *string = nil; Root r_string;

	string = str;
	gcref(&r_string, (void **)&string);
	term = mkterm(string, nil);
	gcderef(&r_string, (void **)&string);
	return term;
}

Term *
mkdictterm(Dict *d)
{
	if(!d)
		return mkterm1(nil, nil, mkdict());
	return mkterm1(nil, nil, d);
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

Closure *
getclosure(Term *term)
{
	Term *tp = nil; Root r_tp;
	Tree *np = nil; Root r_np;

	if(term->kind == tkDict)
		return nil;
	if(term->closure == nil) {
		assert(term->str != nil);
		if(isfunction(term->str)) {
			gcref(&r_tp, (void **)&tp);
			gcref(&r_np, (void **)&np);
			tp = term;
			np = parsestring(term->str);
			if(np == nil) {
				gcrderef(&r_np);
				gcrderef(&r_tp);
				return nil;
			}
			tp->closure = extractbindings(np);
			tp->str = nil;
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
	AssocArgs *args = nil; Root r_args_result;
	List *data = nil; Root r_data;

	args = vargs;
	gcref(&r_args_result, (void **)&args->result);
	gcref(&r_data, (void **)&data);
	data = vdata;

	if(args->result == nil)
		args->result = str("%s => %V", name, data, " ");
	else
		args->result = str("%s; %s => %V", args->result, name, data, " ");

	gcrderef(&r_data);
	gcrderef(&r_args_result);
}

char *
getregex(Term *term)
{
	if(term->kind == tkRegex)
		return term->str;
	else
		return getstr(term);
}

char *
getstr(Term *term)
{
	Term *tp = nil; Root r_tp;
	AssocArgs args; Root r_args_result;
	Dict *d = nil; Root r_d;
	char *res = nil; Root r_res;

	if(term->kind == tkString && term->str == nil) {
		/* TODO: This is wrong, but I still need to hunt down places where
		 * strings are defined improperly
		 */
		gcref(&r_tp, (void **)&tp);
		tp = term;
		if(term->closure != nil) {
			tp->str = str("%C", tp->closure);
			tp->kind = tkClosure;
		} else if(term->dict != nil) {
			tp->str = str("%V", tp->dict);
			tp->kind = tkDict;
		}
		gcrderef(&r_tp);
		term = tp;
	}
	switch(term->kind) {
	default:
		unreachable();
	case tkString:
		return term->str;
	case tkRegex:
		return str("%%re(%#S)", term->str);
	case tkClosure:
		assert(term->closure != nil);
		return str("%C", term->closure);
	case tkDict:
		assert(term->dict != nil);
		args.result = nil;
		gcref(&r_args_result, (void **)&args.result);
		gcref(&r_tp, (void **)&tp);
		gcref(&r_d, (void **)&r_d);
		gcref(&r_res, (void **)&r_res);

		d = term->dict;
		dictforall(d, assocfmt, &args);
		if(args.result == nil)
			res = str("%%dict()");
		else
			res = str("%%dict(%s)", args.result);

		gcrderef(&r_res);
		gcrderef(&r_d);
		gcrderef(&r_tp);
		gcrderef(&r_args_result);
		return res;
	}
	unreachable();
	return nil;
}

Dict *
getdict(Term *term)
{
	Term *t = nil; Root r_t;
	Tree *parsed = nil; Root r_parsed;
	List *glommed = nil; Root r_glommed;
	Dict *dict = nil; Root r_dict;

	switch(term->kind) {
	default:
		unreachable();
	case tkDict:
		return term->dict;
	case tkString:
		assert(term->str != nil);
		if(!isadict(term->str))
			return nil;
		gcref(&r_t, (void **)&t);
		gcref(&r_parsed, (void **)&parsed);
		gcref(&r_glommed, (void **)&glommed);
		gcref(&r_dict, (void **)&dict);

		t = term;
		parsed = parsestring(t->str);
		if(!parsed)
			goto done;
		glommed = glom(parsed, nil, FALSE);
		if(glommed == nil || glommed->next != nil)
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
	case tkClosure:
	case tkRegex:
		return nil;
	}
	return nil;
}

Term *
termcat(Term *t1, Term *t2)
{
	Term *term = nil; Root r_term;
	char *str1 = nil; Root r_str1;
	char *str2 = nil; Root r_str2;

	if(t1 == nil)
		return t2;
	if(t2 == nil)
		return t1;

	gcref(&r_term, (void **)&term);
	str1 = getstr(t1);
	gcref(&r_str1, (void **)&str1);
	str2 = getstr(t2);
	gcref(&r_str2, (void **)&str2);

	term = mkstr(str("%s%s", str1, str2));

	gcderef(&r_str2, (void **)&str2);
	gcderef(&r_str1, (void **)&str1);
	gcderef(&r_term, (void **)&term);
	return term;
}

void *
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

	t = (Term *)p;
	gc_set_mark(header(p));
	gcmark(t->closure);
	gcmark(t->str);
	gcmark(t->dict);
}

extern Boolean
termeq(Term *term, const char *s)
{
	assert(term != nil);
	if(term->str == nil)
		return FALSE;
	return streq(term->str, s);
}

extern Boolean
isclosure(Term *term)
{
	assert(term != nil);
	return term->closure != nil;
}

Boolean
isdict(Term *term)
{
	assert(term != nil);
	if(term->dict == nil && !isadict(getstr(term)))
		return FALSE;
	return TRUE;
}
