/* glom.c -- walk parse tree to produce list ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"


extern char DEAD[]; // from dict.c
Dict *dictlistconcat(Dict*, List*, Boolean);
Dict *dictdictconcat(Dict*, Dict*);

int
isvaliddict(List *list)
{
	if(list == NULL)
		return 0;
	if(list->next == NULL && list->term->kind == tkDict)
		return 1;
	return 0;
}

/* concat -- cartesion cross product concatenation */
extern List *concat(List *list1, List *list2) {
	List **p, *result = NULL; Root r_result;
	Dict *dict = NULL; Root r_dict;
	Dict *dictsrc = NULL; Root r_dictsrc;
	List *list = NULL; Root r_list;
	Boolean before = FALSE;

	if(isvaliddict(list1) && isvaliddict(list2)){
		gcref(&r_result, (void**)&result);
		gcref(&r_dict, (void**)&dict);
		gcref(&r_dictsrc, (void**)&dictsrc);

		dict = getdict(list1->term);
		dictsrc = getdict(list2->term);

		dict = dictdictconcat(dict, dictsrc);
		result = mklist(mkdictterm(dict), NULL);

		gcrderef(&r_dictsrc);
		gcrderef(&r_dict);
		gcrderef(&r_result);

		return result;

	}
	if(isvaliddict(list1) || isvaliddict(list2)){
		gcref(&r_result, (void**)&result);
		gcref(&r_dict, (void**)&dict);
		gcref(&r_list, (void**)&list);

		if(isvaliddict(list1)){
			dict = getdict(list1->term);
			list = list2;
			before = FALSE;
		} else {
			dict = getdict(list2->term);
			list = list1;
			before = TRUE;
		}
		dict = dictlistconcat(dict, list, before);
		result = mklist(mkdictterm(dict), NULL);

		gcrderef(&r_list);
		gcrderef(&r_dict);
		gcrderef(&r_result);

		return result;
	}
	gcdisable();
	for (p = &result; list1 != NULL; list1 = list1->next) {
		List *lp;
		for (lp = list2; lp != NULL; lp = lp->next) {
			List *np = mklist(termcat(list1->term, lp->term), NULL);
			*p = np;
			p = &np->next;
		}
	}

	Ref(List *, list, result);
	gcenable();
	RefReturn(list);
}

Dict*
dictlistconcat(Dict *darg, List *larg, Boolean before)
{
	int i;
	Dict *dict = NULL; Root r_dict;
	Dict *odict = NULL; Root r_odict;
	List *list = NULL; Root r_list;
	List *dictelem = NULL; Root r_dictelem;
	List *odictelem = NULL; Root r_odictelem;

	gcref(&r_dict, (void**)&dict);
	gcref(&r_odict, (void**)&odict);
	gcref(&r_list, (void**)&list);
	gcref(&r_dictelem, (void**)&dictelem);
	gcref(&r_odictelem, (void**)&odictelem);


	dict = mkdict();
	odict = darg;
	list = larg;

	for(i = 0; i < odict->size; i++){
		if(odict->table[i].name == NULL || odict->table[i].name == DEAD)
			continue;
		odictelem = (List*)odict->table[i].value;
		if(before)
			dictelem = concat(list, odictelem);
		else
			dictelem = concat(odictelem, list);
		dict = dictput(dict, odict->table[i].name, (void*)dictelem);
	}

	gcrderef(&r_odictelem);
	gcrderef(&r_dictelem);
	gcrderef(&r_list);
	gcrderef(&r_odict);
	gcrderef(&r_dict);

	return dict;
}

Dict*
dictdictconcat(Dict *desta, Dict *srca)
{
	Dict *odest = NULL; Root r_odest;
	Dict *dest = NULL; Root r_dest;
	Dict *src = NULL; Root r_src;

	gcref(&r_odest, (void**)&dest);
	gcref(&r_dest, (void**)&dest);
	gcref(&r_src, (void**)&src);

	odest = desta;
	src = srca;
	dest = dictcopy(odest);
	dest = dictappend(dest, src, TRUE);

	gcrderef(&r_src);
	gcrderef(&r_dest);
	gcrderef(&r_odest);

	return dest;
}

/* qcat -- concatenate two quote flag terms */
static char *qcat(const char *q1, const char *q2, Term *t1, Term *t2) {
	size_t len1, len2;
	char *result, *s;

	assert(gcisblocked());

	if (q1 == QUOTED && q2 == QUOTED)
		return QUOTED;
	if (q1 == UNQUOTED && q2 == UNQUOTED)
		return UNQUOTED;

	len1 = (q1 == QUOTED || q1 == UNQUOTED) ? strlen(getstr(t1)) : strlen(q1);
	len2 = (q2 == QUOTED || q2 == UNQUOTED) ? strlen(getstr(t2)) : strlen(q2);
	result = s = gcalloc(len1 + len2 + 1, tString);

	if (q1 == QUOTED)
		memset(s, 'q', len1);
	else if (q1 == UNQUOTED)
		memset(s, 'r', len1);
	else
		memcpy(s, q1, len1);
	s += len1;
	if (q2 == QUOTED)
		memset(s, 'q', len2);
	else if (q2 == UNQUOTED)
		memset(s, 'r', len2);
	else
		memcpy(s, q2, len2);
	s += len2;
	*s = '\0';

	return result;
}

/* qconcat -- cartesion cross product concatenation; also produces a quote list */
static List *qconcat(List *list1, List *list2, StrList *ql1, StrList *ql2, StrList **quotep) {
	List **p, *result = NULL;
	StrList **qp;

	gcdisable();
	for (p = &result, qp = quotep; list1 != NULL; list1 = list1->next, ql1 = ql1->next) {
		List *lp;
		StrList *qlp;
		for (lp = list2, qlp = ql2; lp != NULL; lp = lp->next, qlp = qlp->next) {
			List *np;
			StrList *nq;
			np = mklist(termcat(list1->term, lp->term), NULL);
			*p = np;
			p = &np->next;
			nq = mkstrlist(qcat(ql1->str, qlp->str, list1->term, lp->term), NULL);
			*qp = nq;
			qp = &nq->next;
		}
	}

	Ref(List *, list, result);
	gcenable();
	RefReturn(list);
}

/* subscript -- variable subscripting */
static List *subscript(List *list, List *subs) {
	int lo, hi, len, counter;
	List *result, **prevp, *current;

	gcdisable();

	result = NULL;
	prevp = &result;
	len = length(list);
	current = list;
	counter = 1;

	if (subs != NULL && streq(getstr(subs->term), "...")) {
		lo = 1;
goto mid_range;
	}

	while (subs != NULL) {
		lo = atoi(getstr(subs->term));
		if (lo < 1) {
			Ref(char *, bad, getstr(subs->term));
			gcenable();
			fail("es:subscript", "bad subscript: %s", bad);
			RefEnd(bad);
		}
		subs = subs->next;
		if (subs != NULL && streq(getstr(subs->term), "...")) {
		mid_range:
			subs = subs->next;
			if (subs == NULL)
				hi = len;
			else {
				hi = atoi(getstr(subs->term));
				if (hi < 1) {
					Ref(char *, bad, getstr(subs->term));
					gcenable();
					fail("es:subscript", "bad subscript: %s", bad);
					RefEnd(bad);
				}
				if (hi > len)
					hi = len;
				subs = subs->next;
			}
		} else
			hi = lo;
		if (lo > len)
			continue;
		if (counter > lo) {
			current = list;
			counter = 1;
		}
		for (; counter < lo; counter++, current = current->next)
			;
		for (; counter <= hi; counter++, current = current->next) {
			*prevp = mklist(current->term, NULL);
			prevp = &(*prevp)->next;
		}
	}

	Ref(List *, r, result);
	gcenable();
	RefReturn(r);
}

/* glom1 -- glom when we don't need to produce a quote list */
static List *glom1(Tree *tree, Binding *binding) {
	Ref(List *, result, NULL);
	Ref(List *, tail, NULL);
	Ref(Tree *, tp, tree);
	Ref(Binding *, bp, binding);

	/* for nVar */
	List *var = NULL; Root r_var;

	/* for nVarsub */
	char *namestr = NULL; Root r_namestr;
	char *subnamestr = NULL; Root r_subnamestr;
	List *sub = NULL; Root r_sub;
	List *templist = NULL; Root r_templist;
	List *li = NULL; Root r_li;

	/* for nConcat */
	List *l = NULL; Root r_l;
	List *r = NULL; Root r_r;

	/* for nDict */
	Tree *inner = NULL; Root r_inner;
	Dict *dict = NULL; Root r_dict;
	Tree *assoc = NULL; Root r_assoc;
	List *name = NULL; Root r_name;
	List *value = NULL; Root r_value;
	/* char *namestr = NULL; Root r_namestr; */

	assert(!gcisblocked());

	while (tp != NULL) {
		Ref(List *, list, NULL);

		switch (tp->kind) {
		default:
			fail("es:glom", "glom1: bad node kind %s: %T", treekind(tree), tree);
			break;
		case nQword:
			list = mklist(mkterm(tp->u[0].s, NULL), NULL);
			tp = NULL;
			break;
		case nWord:
			list = mklist(mkterm(tp->u[0].s, NULL), NULL);
			tp = NULL;
			break;
		case nRegex:
			list = glom1(tp->u[0].p, binding);
			tp = NULL;
			break;
		case nThunk:
		case nLambda:
			list = mklist(mkterm(NULL, mkclosure(tp, bp)), NULL);
			tp = NULL;
			break;
		case nPrim:
			list = mklist(mkterm(NULL, mkclosure(tp, NULL)), NULL);
			tp = NULL;
			break;
		case nVar:
			gcref(&r_var, (void**)&var);

			var = glom1(tp->u[0].p, bp);
			tp = NULL;
			for (; var != NULL; var = var->next) {
				list = listcopy(varlookup(getstr(var->term), bp));
				if (list != NULL) {
					if (result == NULL)
						tail = result = list;
					else
						tail->next = list;
					for (; tail->next != NULL; tail = tail->next)
						;
				}
				list = NULL;
			}

			gcrderef(&r_var);
			break;
		case nVarsub:
			gcref(&r_namestr, (void**)&r_namestr);
			gcref(&r_sub, (void**)&sub);
			gcref(&r_templist, (void**)&templist);

			templist = glom1(tp->u[0].p, bp);
			if (templist == NULL)
				fail("es:glom", "null variable name in subscript");
			if (templist->next != NULL)
				fail("es:glom", "multi-word variable name in subscript");
			namestr = getstr(templist->term);
			templist = varlookup(namestr, bp);
			sub = glom1(tp->u[1].p, bp);
			if(templist != NULL && templist->term->kind == tkDict && templist->next == NULL){
				gcref(&r_dict, (void**)&dict);
				gcref(&r_subnamestr, (void**)&subnamestr);
				gcref(&r_l, (void**)&l);
				gcref(&r_li, (void**)&li);

				dict = getdict(templist->term);
				for(li = sub; li != NULL; li = li->next){
					subnamestr = getstr(li->term);
					l = dictget(dict, subnamestr);
					if(!l)
						fail("es:glom", "element '%s' is empty", subnamestr);
					list = append(list, l);
				}

				gcrderef(&r_li);
				gcrderef(&r_l);
				gcrderef(&r_subnamestr);
				gcrderef(&r_dict);
			} else {
				list = subscript(templist, sub);
			}

			tp = NULL;

			gcrderef(&r_templist);
			gcrderef(&r_sub);
			gcrderef(&r_namestr);
			break;
		case nCall:
			list = listcopy(walk(tp->u[0].p, bp, 0));
			tp = NULL;
			break;
		case nList:
			list = glom1(tp->u[0].p, bp);
			tp = tp->u[1].p;
			break;
		case nConcat:
			gcref(&r_l, (void**)&l);
			gcref(&r_r, (void**)&r);

			l = glom1(tp->u[0].p, bp);
			r = glom1(tp->u[1].p, bp);
			tp = NULL;
			list = concat(l, r);

			gcrderef(&r_r);
			gcrderef(&r_l);
			break;
		case nDict:

			gcref(&r_inner, (void**)&inner);
			gcref(&r_dict, (void**)&dict);
			gcref(&r_assoc, (void**)&assoc);
			gcref(&r_name, (void**)&name);
			gcref(&r_value, (void**)&value);
			gcref(&r_namestr, (void**)&namestr);

			dict = mkdict();
			for(inner = tp->u[0].p; inner != NULL; inner = inner->u[1].p){
				assoc = inner->u[0].p;
				assert(assoc->kind = nAssoc);
				name = glom1(assoc->u[0].p, bp);
				value = glom(assoc->u[1].p, bp, TRUE);
				if(name == NULL)
					continue;
				namestr = getstr(name->term);
				dict = dictput(dict, namestr, value);
			}

			list = mklist(mkdictterm(dict), NULL);
			tp = NULL;

			gcrderef(&r_namestr);
			gcrderef(&r_value);
			gcrderef(&r_name);
			gcrderef(&r_assoc);
			gcrderef(&r_dict);
			gcrderef(&r_inner);
			break;
		}

		if (list != NULL) {
			if (result == NULL)
				tail = result = list;
			else
				tail->next = list;
			for (; tail->next != NULL; tail = tail->next)
				;
		}
		RefEnd(list);
	}

	RefEnd3(bp, tp, tail);
	RefReturn(result);
}

/* glom2 -- glom and produce a quoting list */
extern List *glom2(Tree *tree, Binding *binding, StrList **quotep) {
	Ref(List *, result, NULL);
	Ref(List *, tail, NULL);
	Ref(StrList *, qtail, NULL);
	Ref(Tree *, tp, tree);
	Ref(Binding *, bp, binding);

	assert(!gcisblocked());
	assert(quotep != NULL);

	/*
	 * this loop covers only the cases where we might produce some
	 * unquoted (raw) values.  all other cases are handled in glom1
	 * and we just add quoted word flags to them.
	 */

	while (tp != NULL) {
		Ref(List *, list, NULL);
		Ref(StrList *, qlist, NULL);

		switch (tp->kind) {
		case nWord:
			list = mklist(mkterm(tp->u[0].s, NULL), NULL);
			qlist = mkstrlist(UNQUOTED, NULL);
			tp = NULL;
			break;
		case nList:
			list = glom2(tp->u[0].p, bp, &qlist);
			tp = tp->u[1].p;
			break;
		case nConcat:
			Ref(List *, l, NULL);
			Ref(List *, r, NULL);
			Ref(StrList *, ql, NULL);
			Ref(StrList *, qr, NULL);
			l = glom2(tp->u[0].p, bp, &ql);
			r = glom2(tp->u[1].p, bp, &qr);
			if(isvaliddict(l) || isvaliddict(r)) {
				list = concat(l, r);
				qlist = mkstrlist(UNQUOTED, NULL);
			} else {
				list = qconcat(l, r, ql, qr, &qlist);
			}
			RefEnd4(qr, ql, r, l);
			tp = NULL;
			break;
		default:
			list = glom1(tp, bp);
			Ref(List *, lp, list);
			for (; lp != NULL; lp = lp->next)
				qlist = mkstrlist(QUOTED, qlist);
			RefEnd(lp);
			tp = NULL;
			break;
		}

		if (list != NULL) {
			if (result == NULL) {
				assert(*quotep == NULL);
				result = tail = list;
				*quotep = qtail = qlist;
			} else {
				assert(*quotep != NULL);
				tail->next = list;
				qtail->next = qlist;
			}
			for (; tail->next != NULL; tail = tail->next, qtail = qtail->next)
				;
			assert(qtail->next == NULL);
		}
		RefEnd2(qlist, list);
	}

	RefEnd4(bp, tp, qtail, tail);
	RefReturn(result);
}

/* glom -- top level glom dispatching */
extern List *glom(Tree *tree, Binding *binding, Boolean globit) {
	if (globit) {
		Ref(List *, list, NULL);
		Ref(StrList *, quote, NULL);
		list = glom2(tree, binding, &quote);
		list = glob(list, quote);
		RefEnd(quote);
		RefReturn(list);
	} else
		return glom1(tree, binding);
}
