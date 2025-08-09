#include "es.h"
#include "prim.h"

extern int gc_after;
extern int gc_sort_after_n;
extern int gc_coalesce_after_n;
extern uint32_t gc_oldage;
extern int gc_oldsweep_after;

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

PRIM(gctuning) {
	List *res = NULL; Root r_res;
	int v = 0;

	if (list == NULL) {
		gcref(&r_res, (void**)&res);
		res = mklist(mkstr(str("%d", gc_oldsweep_after)), res);
		res = mklist(mkstr(str("%ud", gc_oldage)), res);
		res = mklist(mkstr(str("%d", gc_coalesce_after_n)), res);
		res = mklist(mkstr(str("%d", gc_sort_after_n)), res);
		res = mklist(mkstr(str("%d", gc_after)), res);
		gcrderef(&r_res);
		return res;
	}
	if(list->next == NULL)
		fail("$&gctuning", "missing arguments");

	errno = 0;
	v = (int)strtol(getstr(list->next->term), NULL, 10);
	if(v == 0){
		switch(errno){
		case EINVAL:
			fail("$&gctuning", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&gctuning", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	gcdisable();
	if(termeq(list->term, "gcafter")){
		gc_after = v;
		gcenable();
		return list_true;
	}
	if(termeq(list->term, "sortaftern")){
		gc_sort_after_n = v;
		gcenable();
		return list_true;
	}
	if(termeq(list->term, "coalesceaftern")){
		gc_coalesce_after_n = v;
		gcenable();
		return list_true;
	}
	if(termeq(list->term, "oldage")){
		gc_oldage = v;
		gcenable();
		return list_true;
	}
	if(termeq(list->term, "oldsweepafter")){
		gc_oldsweep_after = v;
		gcenable();
		return list_true;
	}

	gcenable();
	fail("$&gctuning", str("invalid paramter: '%s'\n", getstr(list->term)));
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
	X(gctuning);

	return primdict;
}

