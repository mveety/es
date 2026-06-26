/* term.c -- operations on terms ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"
#include "stdenv.h"

DefineTag(Term, static);

Term *
mkterm1(char *str, Closure *closure, Dict *dict, Object *obj)
{
	Term *term = nil;

	assert(str != nil || closure != nil || dict != nil || obj != nil);
	gcdisable();
	term = gcnew(Term);
	ref(term);
	if(str != nil)
		*term = (Term){tkString, ttNone, {.str = str}};
	else if(closure != nil)
		*term = (Term){tkClosure, ttNone, {.closure = closure}};
	else if(dict != nil)
		*term = (Term){tkDict, ttNone, {.dict = dict}};
	else if(obj != nil)
		*term = (Term){tkObject, ttNone, {.obj = obj}};
	else
		unreachable();
	gcenable();
	deref(term);
	return term;
}

Term *
mkterm(char *str, Closure *closure)
{
	return mkterm1(str, closure, nil, nil);
}

Term *
mkstr(char *str)
{
	Term *term;
	char *string = nil;

	string = str;
	ref(string);
	term = mkterm(string, nil);
	deref(string);
	return term;
}

Term *
mkobject(Object *obj)
{
	refobject(obj);
	return mkterm1(nil, nil, nil, obj);
}

Term *
mkdictterm(Dict *d)
{
	if(!d)
		return mkterm1(nil, nil, mkdict(), nil);
	return mkterm1(nil, nil, d, nil);
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
	Term *tp = nil;
	Tree *np = nil;
	char *tmp = nil;

	assert(term->ptr);
	switch(term->kind) {
	default:
		// should be unreachable();
		return nil;
	case tkString:
		if(isfunction(term->str)) {
			ref(tp);
			ref(np);
			ref(tmp);
			tp = term;
			tmp = gcdup(tp->str);
			np = parsestring(tmp);
			if(np == nil) {
				deref(np);
				deref(tp);
				return nil;
			}
			tp->closure = extractbindings(np);
			tp->kind = tkClosure;
			term = tp;
			deref(tmp);
			deref(np);
			deref(tp);
		} else
			return nil;
		fallthrough;
	case tkClosure:
		return term->closure;
	}
}

typedef struct {
	char *result;
} AssocArgs;

void
assocfmt(void *vargs, char *name, void *vdata)
{
	AssocArgs *args = nil; Root r_args_result;
	List *data = nil;

	args = vargs;
	gcref(&r_args_result, (void **)&args->result);
	ref(data);
	data = vdata;

	if(args->result == nil)
		args->result = str("%s => %#V", name, data, " ");
	else
		args->result = str("%s; %s => %#V", args->result, name, data, " ");

	deref(data);
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
	Term *tp = nil; 
	AssocArgs args; Root r_args_result;
	Dict *d = nil;
	char *res = nil;
	char *objstr = nil;
	char *tmp;

	assert(term->ptr);
	switch(term->kind) {
	default:
		unreachable();
	case tkString:
		return term->str;
	case tkRegex:
		return str("%%re(%#S)", term->str);
	case tkClosure:
		return str("%C", term->closure);
	case tkDict:
		args.result = nil;
		gcref(&r_args_result, (void **)&args.result);
		ref(tp);
		ref(d);
		ref(res);

		d = term->dict;
		dictforall(d, assocfmt, &args);
		if(args.result == nil)
			res = str("%%dict()");
		else
			res = str("%%dict(%s)", args.result);

		deref(res);
		deref(d);
		deref(tp);
		gcrderef(&r_args_result);
		return res;
	case tkObject:
		if((objstr = stringify(term->obj))) {
			tmp = str("%%obj:%s('%s')", gettypename(term->obj->type), objstr);
			efree(objstr);
			return tmp;
		}
		return str("%%obj:%s", gettypename(term->obj->type));
	}
	unreachable();
	return nil;
}

Dict *
getdict(Term *term)
{
	Term *t = nil;
	Tree *parsed = nil;
	List *glommed = nil;
	Dict *dict = nil;

	switch(term->kind) {
	default:
		unreachable();
	case tkDict:
		return term->dict;
	case tkString:
		assert(term->str != nil);
		if(!isadict(term->str))
			return nil;
		ref(t);
		ref(parsed);
		ref(glommed);
		ref(dict);

		t = term;
		parsed = parsestring(t->str);
		if(!parsed)
			goto done; // might need to pass getdict the runflags
		glommed = glom(parsed, nil, FALSE, 0);
		if(glommed == nil || glommed->next != nil)
			goto done;
		dict = getdict(glommed->term);
		t->kind = tkDict;
		t->dict = dict;
done:
		deref(dict);
		deref(glommed);
		deref(parsed);
		deref(t);
		return dict;
	case tkClosure:
	case tkRegex:
	case tkObject:
		return nil;
	}
	return nil;
}

Object *
getobject(Term *term)
{
	Result res;

	if(!term)
		return nil;
	switch(term->kind) {
	default:
		unreachable();
	case tkObject:
		return term->obj;
	case tkString:
		res = objectify(nil, getstr(term));
		switch(status(res)) {
		default:
			if(res.errstr)
				fail("es:objectify", "%s", res.errstr);
			fail("es:objectify", "other error: %d", res.status);
			break;
		case ObjectifyOk:
			return ok_obj(res);
		case ObjectifyUnknownError:
			fail("es:objectify", "unknown error");
			break;
		case ObjectifyInvalidType:
			fail("es:objectify", "invalid type");
			break;
		case ObjectifyInvalidObjectFormat:
			fail("es:objectify", "invalid object format");
		case ObjectifyInvalidFormat:
			fail("es:objectify", "invalid format");
			break;
		}
		break;
	case tkClosure:
	case tkRegex:
		return nil;
	}
	unreachable();
	return nil;
}

Term *
termcat(Term *t1, Term *t2)
{
	Term *term = nil;
	char *str1 = nil;
	char *str2 = nil;

	if(t1 == nil)
		return t2;
	if(t2 == nil)
		return t1;

	ref(term);
	ref(str1);
	ref(str2);

	str1 = getstr(t1);
	str2 = getstr(t2);
	term = mkstr(str("%s%s", str1, str2));

	deref(str2);
	deref(str1);
	deref(term);
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
	switch(term->kind) {
	default:
		unreachable();
	case tkClosure:
		term->closure = forward(term->closure);
		break;
	case tkString:
	case tkRegex:
		term->str = forward(term->str);
		break;
	case tkDict:
		term->dict = forward(term->dict);
		break;
	case tkObject:
		if(term->obj->sysflags & ObjectGcManaged)
			refobject(term->obj);
		break;
	}
	return sizeof(Term);
}

void
TermMark(void *p)
{
	Term *t;

	t = (Term *)p;
	gc_set_mark(header(p));
	switch(t->kind) {
	default:
		unreachable();
	case tkString:
	case tkRegex:
	case tkDict:
	case tkClosure:
		gcmark(t->ptr);
		break;
	case tkObject:
		if(t->obj->sysflags & ObjectGcManaged)
			refobject(t->obj);
		break;
	}
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
	return term->kind == tkClosure;
}

Boolean
isdict(Term *term)
{
	assert(term != nil);
	if(term->dict == nil && !isadict(getstr(term)))
		return FALSE;
	return TRUE;
}
