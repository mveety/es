#include "editor.h"
#include "es.h"
#include "gc.h"
#include "prim.h"

extern Region *regions;
extern int gc_after;
extern int gc_sort_after_n;
extern int gc_coalesce_after_n;
extern uint32_t gc_oldage;
extern int gc_oldsweep_after;
extern EditorState *editor;

PRIM(version) {
	return mklist(mkstr((char *)version), NULL);
}

PRIM(buildstring) {
	return mklist(mkstr((char *)buildstring), NULL);
}


PRIM(addhistory) {
	if(list == NULL)
		fail("$&addhistory", "usage: $&addhistory [string]");
	history_add(editor, getstr(list->term));
	return NULL;
}

PRIM(addhistorylist) {
	Root r_list;
	List *lp; Root r_lp;

	if(list == NULL)
		fail("$&addhistorylist", "usage: $&addhistorylist [list of strings]");

	gcref(&r_list, (void **)&list);
	gcref(&r_lp, (void **)&lp);

	for(lp = list; lp != NULL; lp = lp->next)
		history_add(editor, getstr(lp->term));

	gcrderef(&r_lp);
	gcrderef(&r_list);

	return list_true;
}

PRIM(clearhistory) {
	history_clear(editor);
	return NULL;
}

PRIM(sethistory) {
	if(list == NULL) {
		sethistory(NULL);
		return NULL;
	}
	Ref(List *, lp, list);
	sethistory(getstr(lp->term));
	RefReturn(lp);
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
	if(start == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&range", str("invalid input: $1 = '%s'", getstr(list->term)));
			break;
		case ERANGE:
			fail("$&range", str("conversion overflow: $1 = '%s'", getstr(list->term)));
			break;
		}
	}

	end = (int)strtol(getstr(list->next->term), NULL, 10);
	if(start == 0) {
		switch(errno) {
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

	gcref(&r_res, (void **)&res);

	gcdisable();
	for(i = end; i >= start; i--)
		res = mklist(mkstr(str("%d", i)), res);
	gcenable();

	gcderef(&r_res, (void **)&res);
	return res;
}

PRIM(reverse) {
	List *l = NULL;
	List *res = NULL; Root r_res;

	if(list == NULL)
		return NULL;

	gcref(&r_res, (void **)&res);

	for(l = list; l != NULL; l = l->next)
		res = mklist(l->term, res);

	gcderef(&r_res, (void **)&res);
	return res;
}

PRIM(reverse_noalloc) {
	if(list == nil)
		return nil;

	return reverse(list);
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

	curtime = ((int64_t)ts.tv_sec) * 1000000000LL + ts.tv_nsec;
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

	gcref(&r_lp, (void **)&lp);
	gcref(&r_term, (void **)&term);
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

	switch(list->term->tag) {
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

	gcref(&r_res, (void **)&res);

	memset(&stats, 0, sizeof(GcStats));
	if(gctype == NewGc) {
		gc_getstats(&stats);
		res = mklist(mkstr(str("%d", stats.gcblocked)), res);
		if(stats.array_sort == TRUE) {
			if(stats.use_size)
				res = mklist(mkstr(str("array_sort+size")), res);
			else
				res = mklist(mkstr(str("array_sort")), res);
		} else
			res = mklist(mkstr(str("list_sort")), res);
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
		res = mklist(mkstr(str("%d", stats.gcblocked)), res);
		res = mklist(mkstr(str("%lud", stats.ngcs)), res);
		res = mklist(mkstr(str("%lud", stats.allocations)), res);
		res = mklist(mkstr(str("%lud", stats.nallocs)), res);
		res = mklist(mkstr(str("%lud", stats.total_used)), res);
		res = mklist(mkstr(str("%lud", stats.total_free)), res);
		res = mklist(mkstr(str("old")), res);
	}

	gcderef(&r_res, (void **)&res);
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

	gcref(&r_res, (void **)&res);

	for(r = regions; r != NULL; r = r->next) {
		res = mklist(mkstr(str("%lud", r->size)), res);
		res = mklist(mkstr(str("%ulx", r->start)), res);
	}

	gcderef(&r_res, (void **)&res);
	return res;
}

PRIM(gctuning) {
	List *res = NULL; Root r_res;
	int v = 0;

	if(list == NULL) {
		gcref(&r_res, (void **)&res);
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
	if(v == 0) {
		switch(errno) {
		case EINVAL:
			fail("$&gctuning", str("invalid input: $2 = '%s'", getstr(list->next->term)));
			break;
		case ERANGE:
			fail("$&gctuning", str("conversion overflow: $2 = '%s'", getstr(list->next->term)));
			break;
		}
	}

	gcdisable();
	if(termeq(list->term, "gcafter")) {
		gc_after = v;
		gcenable();
		return list_true;
	}
	if(termeq(list->term, "sortaftern")) {
		gc_sort_after_n = v;
		gcenable();
		return list_true;
	}
	if(termeq(list->term, "coalesceaftern")) {
		gc_coalesce_after_n = v;
		gcenable();
		return list_true;
	}
	if(termeq(list->term, "oldage")) {
		gc_oldage = v;
		gcenable();
		return list_true;
	}
	if(termeq(list->term, "oldsweepafter")) {
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

	gcref(&r_result, (void **)&result);
	gcref(&r_lp, (void **)&lp);
	gcref(&r_tree, (void **)&tree);
	gcref(&r_str, (void **)&str);

	lp = list;
	str = getstr(lp->term);
	if(str == NULL) {
		gcrderef(&r_str);
		gcrderef(&r_tree);
		gcrderef(&r_lp);
		gcrderef(&r_result);
		fail("$&parsestring", "invalid term");
	}
	tree = parsestring((const char *)str);
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

	gcref(&r_term, (void **)&term);
	gcref(&r_res, (void **)&res);
	gcref(&r_name, (void **)&name);
	gcref(&r_defn, (void **)&defn);

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

	gcref(&r_lp, (void **)&lp);
	gcref(&r_subject, (void **)&subject);
	gcref(&r_pattern, (void **)&pattern);

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
	gcref(&r_lp, (void **)&lp);
	gcref(&r_subject, (void **)&subject);
	gcref(&r_pattern, (void **)&pattern);
	gcref(&r_st_substrs, (void **)&status.substrs);

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

PRIM(sortlist) {
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;

	if(list == nil)
		return nil;

	gcref(&r_lp, (void **)&lp);
	gcref(&r_res, (void **)&res);
	lp = list;

	res = sortlist(lp);

	gcrderef(&r_res);
	gcrderef(&r_lp);

	return res;
}

PRIM(mapkey) {
	List *lp = nil; Root r_lp;
	int res = 0;

	if(list == nil)
		fail("$&mapkey", "missing argument");
	if(list->next == nil)
		fail("$&mapkey", "missing argument");
	if(!editor->initialized) {
		if(isinteractive())
			fail("$&mapkey", "using fallback editor");
		else
			return nil;
	}

	gcref(&r_lp, (void **)&lp);
	lp = list;

	if(hasprefix(getstr(lp->next->term), "%esmle:")) {
		res = bind_base_function(getstr(lp->term), getstr(lp->next->term));
		if(res == -1)
			fail("$&mapkey", "keyname %s is not valid", getstr(lp->term));
		if(res == -2)
			fail("$&mapkey", "editor function %s not found", getstr(lp->next->term));
	} else {
		if(varlookup2("fn-", getstr(lp->next->term), nil) == nil)
			fail("$&mapkey", "function %s not found", getstr(lp->next->term));

		if(bind_es_function(getstr(lp->term), getstr(lp->next->term)) < 0)
			fail("$&mapkey", "keyname %s is not valid", getstr(lp->term));
	}
	gcrderef(&r_lp);
	return nil;
}

PRIM(unmapkey) {
	List *lp = nil; Root r_lp;

	if(list == nil)
		fail("$&unmapkey", "missing argument");

	if(!editor->initialized) {
		if(isinteractive())
			fail("$&unmapkey", "using fallback editor");
		else
			return nil;
	}

	gcref(&r_lp, (void **)&lp);
	lp = list;

	switch(unbind_es_function(getstr(lp->term))) {
	default:
		unreachable();
		break;
	case -1:
		fail("$&unmapkey", "keyname %s is not valid", getstr(lp->term));
		break;
	case -2:
		fail("$&unmapkey", "key value out of range (key = %d)", name2key(getstr(lp->term)));
		break;
	case -3:
		fail("$&unmapkey", "key %s is not bound to a function", getstr(lp->term));
		break;
	case 0:
		break;
	}

	gcrderef(&r_lp);
	return nil;
}

PRIM(mapaskey) {
	List *lp = nil; Root r_lp;

	if(list == nil)
		fail("$&mapaskey", "missing argument");
	if(list->next == nil)
		fail("$&mapaskey", "missing argument");

	if(!editor->initialized) {
		if(isinteractive())
			fail("$&mapaskey", "using fallback editor");
		else
			return nil;
	}

	gcref(&r_lp, (void **)&lp);
	lp = list;

	switch(map_as_key(getstr(lp->term), getstr(lp->next->term))) {
	default:
		unreachable();
		break;
	case -1:
		fail("$&mapaskey", "keyname %s is not valid", getstr(lp->term));
		break;
	case -2:
		fail("$&mapaskey", "keyname %s is not valid", getstr(lp->term));
		break;
	case -3:
		fail("$&mapaskey", "key value out of range (key = %d)", name2key(getstr(lp->next->term)));
		break;
	case 0:
		break;
	}

	gcrderef(&r_lp);
	return nil;
}

PRIM(clearkey) {
	if(list == nil)
		fail("$&clearkey", "missing argument");

	if(!editor->initialized) {
		if(isinteractive())
			fail("$&clearkey", "using fallback editor");
		else
			return nil;
	}

	if(clear_key_mapping(getstr(list->term)) < 0)
		fail("$&clearkey", "keyname %s is not valid", getstr(list->term));
	return nil;
}

PRIM(getkeymap) {
	List *res = nil; Root r_res;
	EditorFunction fn;
	int i = 0;

	if(!editor->initialized) {
		if(isinteractive())
			fail("$&getkeymap", "using fallback editor");
		else
			return nil;
	}

	gcref(&r_res, (void **)&res);

	for(i = 127; i > 0; i--) {
		fn = mapping2edfn(editor->keymap->base_keys[i]);
		if(fn == FnInvalid)
			continue;
		if(fn == FnEsFunction) {
			res = mklist(mkstr(str("%s", editor->keymap->base_keys[i].aux)), res);
		} else {
			res = mklist(mkstr(str("%s", edfn2name(fn))), res);
		}
		res = mklist(mkstr(str("%s", key2name(i))), res);
	}

	for(i = (ExtendedKeys - 1); i >= 0; i--) {
		fn = mapping2edfn(editor->keymap->ext_keys[i]);
		if(fn == FnInvalid)
			continue;
		if(fn == FnEsFunction) {
			res = mklist(mkstr(str("%s", editor->keymap->ext_keys[i].aux)), res);
		} else {
			res = mklist(mkstr(str("%s", edfn2name(fn))), res);
		}
		res = mklist(mkstr(str("%s", key2name(i + ExtKeyOffset))), res);
	}

	gcrderef(&r_res);
	return res;
}

PRIM(editormatchbraces) {
	if(list == nil)
		fail("$&editormatchbraces", "missing argument");
	if(list->next != nil)
		fail("$&editormatchbraces", "list too long");

	if(termeq(list->term, "true"))
		editor->match_braces = 1;
	else
		editor->match_braces = 0;

	return list;
}

PRIM(esmlegetterm) {
	if(editor->initialized)
		return mklist(mkstr(str("%s", editor->term)), nil);
	return mklist(mkstr(str("unknown")), nil);
}

PRIM(esmlegetwordstart) {
	if(!editor->initialized) {
		if(isinteractive())
			fail("$&esmlegetwordstart", "using fallback editor");
		else
			return mklist(mkstr(str("unknown")), nil);
	}
	return mklist(mkstr(str("%s", get_word_start())), nil);
}

PRIM(esmlesetwordstart) {
	List *lp = nil; Root r_lp;
	List *res = nil; Root r_res;

	if(!editor->initialized) {
		if(isinteractive())
			fail("$&esmlesetwordstart", "using fallback editor");
		else
			return list_false;
	}
	if(list == nil)
		fail("$&esmlesetwordstart", "missing argument");
	gcref(&r_lp, (void **)&lp);
	gcref(&r_res, (void **)&res);

	lp = list;

	if(configure_word_start(getstr(list->term)) < 0)
		fail("$&esmlesetwordstart", "invalid argument: %s", getstr(lp->term));
	res = mklist(mkstr(str("%s", getstr(lp->term))), nil);

	gcrderef(&r_res);
	gcrderef(&r_lp);
	return res;
}

PRIM(esmlesethighlight) {
	List *lp = nil; Root r_lp;

	if(!editor->initialized) {
		if(isinteractive())
			fail("$&esmlesethighlight", "using fallback editor");
		else
			return list_false;
	}
	if(list == nil) {
		set_highlight_formatting(editor, nil);
		return mklist(mkstr(str("")), nil);
	}

	gcref(&r_lp, (void **)&lp);
	lp = list;
	if(lp->term->kind != tkString)
		fail("$&esmlesethighlight", "argument must be a string");
	set_highlight_formatting(editor, getstr(lp->term));
	gcrderef(&r_lp);
	return mklist(mkstr(str("%#S", getstr(lp->term))), nil);
}

PRIM(esmlegethighlight) {
	List *res = nil; Root r_res;

	if(!editor->initialized) {
		if(isinteractive())
			fail("$&esmlegethighlight", "using fallback editor");
		else
			return list_false;
	}

	if(editor->highlight_formatting == nil)
		return mklist(mkstr(str("")), nil);

	gcref(&r_res, (void **)&res);
	if(list != nil && termeq(list->term, "-r"))
		res = mklist(mkstr(str("%s", editor->highlight_formatting)), nil);
	else
		res = mklist(mkstr(str("%#S", editor->highlight_formatting)), nil);

	gcrderef(&r_res);
	return res;
}

PRIM(fmt) {
	List *res = nil; Root r_res;

	if(list == nil)
		fail("$&fmt", "missing argument");

	gcref(&r_res, (void **)&res);

	res = mklist(mkstr(str("%#S", getstr(list->term))), nil);

	gcrderef(&r_res);

	return res;
}

Dict *
initprims_mv(Dict *primdict)
{
	X(version);
	X(buildstring);
	X(addhistory);
	X(addhistorylist);
	X(clearhistory);
	X(sethistory);
	X(getevaldepth);
	X(range);
	X(reverse);
	X(reverse_noalloc);
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
	X(sortlist);
	X(mapkey);
	X(unmapkey);
	X(mapaskey);
	X(clearkey);
	X(getkeymap);
	X(editormatchbraces);
	X(esmlegetterm);
	X(esmlegetwordstart);
	X(esmlesetwordstart);
	X(esmlesethighlight);
	X(esmlegethighlight);
	X(fmt);

	return primdict;
}
