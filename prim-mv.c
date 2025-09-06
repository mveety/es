#include "es.h"
#include "gc.h"
#include "prim.h"
#include <regex.h>

extern Region *regions;
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

PRIM(addhistorylist) {
	Root r_list;
	List *lp; Root r_lp;

	if(list == NULL)
		fail("$&addhistorylist", "usage: $&addhistorylist [list of strings]");

	gcref(&r_list, (void**)&list);
	gcref(&r_lp, (void**)&lp);

	for(lp = list; lp != NULL; lp = lp->next)
		add_history(getstr(lp->term));

	gcrderef(&r_lp);
	gcrderef(&r_list);

	return list_true;
}

PRIM(clearhistory) {
	clear_history();
	return NULL;
}

PRIM(rlconf) {
	char *s;
	int r = -1;

	if(list == NULL)
		fail("$&rlconf", "usage: $&rlconf [readline inputrc string]");

	gcdisable();
	s = getstr(list->term);
	r = es_rl_parse_and_bind(s);
	gcenable();

	return mklist(mkstr(str("%d", r)), NULL);
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

PRIM(unixtimens) {
	int64_t curtime;
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);

	curtime = ((int64_t)ts.tv_sec)*1000000000LL + ts.tv_nsec;
	return mklist(mkstr(str("%ld", curtime)), NULL);
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
	else if(strcmp(tagname, "box") == 0)
		term->tag = ttBox;
	else if(strcmp(tagname, "none") == 0)
		term->tag = ttNone;
	else if(strcmp(tagname, "regex") == 0)
		term->tag = ttRegex;
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
	default:
		fail("$&gettermtag", "invalid tag %d", list->term->tag);
		return list_false;
	case ttNone:
		return mklist(mkstr(str("none")), NULL);
	case ttError:
		return mklist(mkstr(str("error")), NULL);
	case ttBox:
		return mklist(mkstr(str("box")), NULL);
	case ttRegex:
		return mklist(mkstr(str("regex")), NULL);
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

PRIM(gcstats) {
	List *res = NULL; Root r_res;
	GcStats stats;

	gcref(&r_res, (void**)&res);

	if(gctype == NewGc){
		gc_getstats(&stats);
		res = mklist(mkstr(str("%d", stats.gc_oldsweep_after)), res);
		res = mklist(mkstr(str("%lud", stats.oldage)), res);
		res = mklist(mkstr(str("%lud", stats.old_blocks)), res);
		res = mklist(mkstr(str("%lud", stats.blocksz)), res);
		res = mklist(mkstr(str("%lud", stats.ncoalesce)), res);
		res = mklist(mkstr(str("%lud", stats.nsort)), res);
		res = mklist(mkstr(str("%lud", stats.nregions)), res);
		res = mklist(mkstr(str("%d", stats.gc_after)), res);
		res = mklist(mkstr(str("%d", stats.ncoalescegc)), res);
		res = mklist(mkstr(str("%d", stats.coalesce_after)), res);
		res = mklist(mkstr(str("%d", stats.nsortgc)), res);
		res = mklist(mkstr(str("%d", stats.sort_after_n)), res);
		res = mklist(mkstr(str("%lud", stats.ngcs)), res);
		res = mklist(mkstr(str("%lud", stats.allocations)), res);
		res = mklist(mkstr(str("%lud", stats.nallocs)), res);
		res = mklist(mkstr(str("%lud", stats.nfrees)), res);
		res = mklist(mkstr(str("%lud", stats.used_blocks)), res);
		res = mklist(mkstr(str("%lud", stats.free_blocks)), res);
		res = mklist(mkstr(str("%lud", stats.real_used)), res);
		res = mklist(mkstr(str("%lud", stats.total_used)), res);
		res = mklist(mkstr(str("%lud", stats.real_free)), res);
		res = mklist(mkstr(str("%lud", stats.total_free)), res);
		if(stats.generational == TRUE)
			res = mklist(mkstr(str("generational")), res);
		else
			res = mklist(mkstr(str("new")), res);
	} else if(gctype == OldGc) {
		old_getstats(&stats);
		res = mklist(mkstr(str("%lud", stats.ngcs)), res);
		res = mklist(mkstr(str("%lud", stats.allocations)), res);
		res = mklist(mkstr(str("%lud", stats.nallocs)), res);
		res = mklist(mkstr(str("%lud", stats.total_used)), res);
		res = mklist(mkstr(str("%lud", stats.total_free)), res);
		res = mklist(mkstr(str("old")), res);
	}

	gcderef(&r_res, (void**)&res);
	return res;
}

PRIM(gc) {
	gc();
	return NULL;
}

PRIM(dumpregions) {
	List *res = NULL; Root r_res;
	Region *r;

	if(gctype == OldGc)
		fail("$&dumpregions", "using old gc");

	gcref(&r_res, (void**)&res);

	for(r = regions; r != NULL; r = r->next){
		res = mklist(mkstr(str("%lud", r->size)), res);
		res = mklist(mkstr(str("%ulx", r->start)), res);
	}

	gcderef(&r_res, (void**)&res);
	return res;
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

PRIM(parsestring) {
	List *result = NULL; Root r_result;
	List *lp = NULL; Root r_lp;
	Tree *tree = NULL; Root r_tree;
	char *str = NULL; Root r_str;

	if(list == NULL)
		fail("$&parsestring", "missing argument");

	gcref(&r_result, (void**)&result);
	gcref(&r_lp, (void**)&lp);
	gcref(&r_tree, (void**)&tree);
	gcref(&r_str, (void**)&str);

	lp = list;
	str = getstr(lp->term);
	if(str == NULL){
		gcrderef(&r_str);
		gcrderef(&r_tree);
		gcrderef(&r_lp);
		gcrderef(&r_result);
		fail("$&parsestring", "invalid term");
	}
	tree = parsestring((const char*)str);
	if(tree == NULL)
		goto done;
	result = mklist(mkterm(NULL, mkclosure(mk(nThunk, tree), NULL)), NULL);

done:
	gcrderef(&r_str);
	gcrderef(&r_tree);
	gcrderef(&r_lp);
	gcrderef(&r_result);
	return result;
}

PRIM(fmtvar) {
	Term *term = NULL; Root r_term;
	List *res = NULL; Root r_res;
	char *name = NULL; Root r_name;
	List *defn = NULL; Root r_defn;

	if(list == NULL)
		fail("$&fmtvar", "missing var");

	gcref(&r_term, (void**)&term);
	gcref(&r_res, (void**)&res);
	gcref(&r_name, (void**)&name);
	gcref(&r_defn, (void**)&defn);

	name = getstr(list->term);
	defn = varlookup(name, binding);
	term = mkstr(str("%V", defn, " "));
	res = mklist(term, NULL);

	gcrderef(&r_defn);
	gcrderef(&r_name);
	gcrderef(&r_res);
	gcrderef(&r_term);

	return res;
}

PRIM(setditto) {
	if(list == NULL)
		fail("$&setditto", "missing term");

	gcdisable();
	setnextlastcmd(getstr(list->term));
	gcenable();

	return list_true;
}

PRIM(getditto) {
	char *s;

	s = getnextlastcmd();
	if(!s)
		return mklist(mkstr(str("")), NULL);
	return mklist(mkstr(str("%s", s, " ")), NULL);
}

PRIM(isalist) {
	if(list != NULL && list->next != NULL)
		return list_true;
	return list_false;
}

PRIM(rematch) {
	RegexStatus status;
	char errstr[128];
	List *lp = list; Root r_lp;
	Term *subject = nil; Root r_subject;
	Term *pattern = nil; Root r_pattern;

	if(list == NULL || list->next == NULL)
		fail("$&rematch", "missing arguments");

	gcref(&r_lp, (void**)&lp);
	gcref(&r_subject, (void**)&subject);
	gcref(&r_pattern, (void**)&pattern);

	memset(errstr, 0, sizeof(errstr));
	status = (RegexStatus){ReNil, FALSE, 0, 0, nil, 0, &errstr[0], sizeof(errstr)};
	subject = lp->term;
	pattern = lp->next->term;

	regexmatch(&status, subject, pattern);
	assert(status.type == ReMatch);

	if(status.compcode)
		fail("$&rematch", "compilation error: %s", errstr);

	if(status.matchcode != 0 && status.matchcode != REG_NOMATCH)
		fail("$&rematch", "match error: %s", errstr);

	gcrderef(&r_pattern);
	gcrderef(&r_subject);
	gcrderef(&r_lp);

	if(status.matched == TRUE)
		return list_true;
	return list_false;
}

PRIM(reextract) {
	RegexStatus status;
	char errstr[128];
	List *lp = list; Root r_lp;
	Term *subject = nil; Root r_subject;
	Term *pattern = nil; Root r_pattern;
	Root r_st_substrs;

	if(list == NULL || list->next == NULL)
		fail("$&rematch", "missing arguments");

	status = (RegexStatus){ReNil, FALSE, 0, 0, nil, 0, &errstr[0], sizeof(errstr)};
	gcref(&r_lp, (void**)&lp);
	gcref(&r_subject, (void**)&subject);
	gcref(&r_pattern, (void**)&pattern);
	gcref(&r_st_substrs, (void**)&status.substrs);

	memset(errstr, 0, sizeof(errstr));
	subject = lp->term;
	pattern = lp->next->term;

	regexextract(&status, subject, pattern);
	assert(status.type == ReExtract);

	if(status.compcode)
		fail("$&reextract", "compilation error: %s", errstr);

	if(status.matchcode != 0 && status.matchcode != REG_NOMATCH)
		fail("$&reextract", "match error: %s", errstr);

	gcrderef(&r_st_substrs);
	gcrderef(&r_pattern);
	gcrderef(&r_subject);
	gcrderef(&r_lp);

	if(status.matched == TRUE)
		return status.substrs;
	return nil;
}


Dict*
initprims_mv(Dict *primdict)
{
	X(version);
	X(buildstring);
#if READLINE
	X(addhistory);
	X(addhistorylist);
	X(clearhistory);
	X(rlconf);
#endif
	X(sethistory);
	X(getlast);
	X(getevaldepth);
	X(range);
	X(reverse);
	X(unixtime);
	X(unixtimens);
	X(getrunflags);
	X(setrunflags);
	X(settermtag);
	X(gettermtag);
	X(varhide);
	X(varunhide);
	X(varishidden);
	X(gcstats);
	X(gc);
	X(dumpregions);
	X(gctuning);
	X(parsestring);
	X(fmtvar);
	X(setditto);
	X(getditto);
	X(isalist);
	X(rematch);
	X(reextract);

	return primdict;
}

