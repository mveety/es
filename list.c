/* list.c -- operations on lists ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"

/*
 * allocation and garbage collector support
 */

DefineTag(List, static);

extern List *mklist(Term *term, List *next) {
	gcdisable();
	assert(term != NULL);
	Ref(List *, list, gcnew(List));
	list->term = term;
	list->next = next;
	gcenable();
	RefReturn(list);
}

static void *ListCopy(void *op) {
	void *np = gcnew(List);
	memcpy(np, op, sizeof (List));
	return np;
}

static size_t ListScan(void *p) {
	List *list = p;
	list->term = forward(list->term);
	list->next = forward(list->next);
	return sizeof (List);
}

void
ListMark(void *p)
{
	List *l;

	l = (List*)p;
	gc_set_mark(header(p));
	assert((void*)l != (void*)l->term && l != l->next);
	gcmark(l->term);
	gcmark(l->next);
}

/*
 * basic list manipulations
 */

/* reverse -- destructively reverse a list */
extern List *reverse(List *list) {
	List *prev, *next;
	if (list == NULL)
		return NULL;
	prev = NULL;
	do {
		next = list->next;
		list->next = prev;
		prev = list;
	} while ((list = next) != NULL);
	return prev;
}

/* append -- merge two lists, non-destructively */
extern List *append(List *head, List *tail) {
	List *lp, **prevp;
	Ref(List *, hp, head);
	Ref(List *, tp, tail);
	gcreserve(40 * sizeof (List));
	gcdisable();
	head = hp;
	tail = tp;
	RefEnd2(tp, hp);

	for (prevp = &lp; head != NULL; head = head->next) {
		List *np = mklist(head->term, NULL);
		*prevp = np;
		prevp = &np->next;
	}
	*prevp = tail;

	Ref(List *, result, lp);
	gcenable();
	RefReturn(result);
}

/* listcopy -- make a copy of a list */
extern List *listcopy(List *list) {
	return append(list, NULL);
}

/* length -- lenth of a list */
extern int length(List *list) {
	int len = 0;
	for (; list != NULL; list = list->next)
		++len;
	return len;
}

/* listify -- turn an argc/argv vector into a list */
extern List *listify(int argc, char **argv) {
	Ref(List *, list, NULL);
	while (argc > 0) {
		Term *term = mkstr(argv[--argc]);
		list = mklist(term, list);
	}
	RefReturn(list);
}

/* nth -- return nth element of a list, indexed from 1 */
extern Term *nth(List *list, int n) {
	assert(n > 0);
	for (; list != NULL; list = list->next) {
		assert(list->term != NULL);
		if (--n == 0)
			return list->term;
	}
	return NULL;
}

/* sortlist */
List*
sortlist(List *list) {
	Vector *v = nil; Root r_v;
	List *l = nil; Root r_l;
	List *lp = nil; Root r_lp;

	gcref(&r_v, (void**)&v);
	gcref(&r_l, (void**)&l);
	gcref(&r_lp, (void**)&lp);
	l = list;

	if (length(list) > 1) {
		v = vectorize(l);
		sortvector(v);
		// gcdisable();
		lp = listify(v->count, v->vector);
		// gcenable();
	}

	gcrderef(&r_lp);
	gcrderef(&r_l);
	gcrderef(&r_v);
	return lp;
}
