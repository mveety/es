#include <stdlib.h>
#include "es.h"
#include "prim.h"

PRIM(version) {
	return mklist(mkstr((char *) version), NULL);
}

PRIM(buildstring) {
	return mklist(mkstr((char *) buildstring), NULL);
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

PRIM(sethistory) {
	if (list == NULL) {
		sethistory(NULL);
		return NULL;
	}
	Ref(List *, lp, list);
	sethistory(getstr(lp->term));
	RefReturn(lp);
}

PRIM(getlast) {
	if(lastcmd == NULL)
		return mklist(mkstr((char *) ""), NULL);
	return mklist(mkstr((char *) lastcmd), NULL);
}

PRIM(getevaldepth) {
	return mklist(mkstr(str("%d", evaldepth)), NULL);
}

PRIM(add) {
	int a = 0;
	int b = 0;
	int res = 0;

	if (list == NULL || list->next == NULL)
		fail("$&add", "missing arguments");
	errno = 0;

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
	errno = 0;

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
	errno = 0;

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
	errno = 0;

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
	errno = 0;

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
	errno = 0;

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
	errno = 0;

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
	errno = 0;

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
	List *res = NULL; Root r_res;

	if(list == NULL || list->next == NULL)
		fail("$&tobase", "missing arguments");
	errno = 0;

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
	errno = 0;

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
	errno = 0;

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

PRIM(getrunflags) {
	char rf[9];

	memset(&rf[0], 0, sizeof(rf));
	getrunflags(&rf[0], sizeof(rf));

	return mklist(mkstr(str("%s", rf)), NULL);
}

PRIM(setrunflags) {
	char *s;
	size_t slen;

	if(list == NULL)
		fail("$&setrunflags", "missing arguement");

	s = getstr(list->term);
	slen = strlen(s);

	if(setrunflags(s, slen) < 0)
		return list_false;

	return list_true;
}

PRIM(settermtag) {
	char *tagname = NULL;
	List *lp = NULL; Root r_lp;
	Term *term = NULL; Root r_term;

	if(list == NULL || list->next == NULL)
		fail("$&settermtag", "missing arguments");

	gcref(&r_lp, (void**)&lp);
	gcref(&r_term, (void**)&term);
	lp = list;
	gcdisable();
	tagname = getstr(lp->term);
	term = lp->next->term;

	if(strcmp(tagname, "error") == 0)
		term->tag = ttError;
	else if(strcmp(tagname, "none") == 0)
		term->tag = ttNone;
	else {
		gcenable();
		gcrderef(&r_term);
		gcrderef(&r_lp);
		fail("$&settermtag", "invalid tag type");
	}

	gcenable();
	gcrderef(&r_term);
	gcrderef(&r_lp);

	return list_true;
}

PRIM(gettermtag) {
	if(list == NULL)
		fail("$&gettermtag", "missing argument");

	switch(list->term->tag){
	case ttNone:
		return mklist(mkstr(str("none")), NULL);
	case ttError:
		return mklist(mkstr(str("error")), NULL);
	default:
		fail("$&gettermtag", "invalid tag %d", list->term->tag);
		return list_false;
	}
}

PRIM(varhide) {
	Var *v = NULL;

	if(list == NULL)
		fail("$&varhide", "missing argument");

	gcdisable();
	v = varobjlookup(getstr(list->term));
	if(!v) {
		gcenable();
		fail("$&varhide", "var not found: %s", getstr(list->term));
		return list_false;
	}
	varhide(v);
	gcenable();
	return list_true;
}

PRIM(varunhide) {
	Var *v = NULL;

	if(list == NULL)
		fail("$&varunhide", "missing argument");

	gcdisable();
	v = varobjlookup(getstr(list->term));
	if(!v) {
		gcenable();
		fail("$&varhide", "var not found: %s", getstr(list->term));
		return list_false;
	}
	varunhide(v);
	gcenable();
	return list_true;
}

PRIM(varishidden) {
	Var *v;
	Boolean st;

	if(list == NULL)
		fail("$&varishidden", "missing argument");

	gcdisable();
	v = varobjlookup(getstr(list->term));
	if(!v) {
		gcenable();
		return list_false;
	}
	st = varishidden(v);
	gcenable();

	if(st == TRUE)
		return list_true;
	return list_false;
}

Dict*
initprims_mv(Dict *primdict)
{
	X(version);
	X(buildstring);
#if READLINE
	X(addhistory);
	X(clearhistory);
#endif
	X(sethistory);
	X(getlast);
	X(getevaldepth);
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
	X(getrunflags);
	X(setrunflags);
	X(settermtag);
	X(gettermtag);
	X(varhide);
	X(varunhide);
	X(varishidden);

	return primdict;
}

