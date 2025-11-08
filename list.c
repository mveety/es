/* list.c -- operations on lists ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"

/*
 * allocation and garbage collector support
 */

DefineTag(List, static);

extern List *
mklist(Term *term, List *next)
{
	gcdisable();
	assert(term != NULL);
	Ref(List *, list, gcnew(List));
	list->term = term;
	list->next = next;
	gcenable();
	RefReturn(list);
}

static void *
ListCopy(void *op)
{
	void *np = gcnew(List);
	memcpy(np, op, sizeof(List));
	return np;
}

static size_t
ListScan(void *p)
{
	List *list = p;
	list->term = forward(list->term);
	list->next = forward(list->next);
	return sizeof(List);
}

void
ListMark(void *p)
{
	List *l;

	l = (List *)p;
	gc_set_mark(header(p));
	assert((void *)l != (void *)l->term && l != l->next);
	gcmark(l->term);
	gcmark(l->next);
}

/*
 * basic list manipulations
 */

/* reverse -- destructively reverse a list */
extern List *
reverse(List *list)
{
	List *prev, *next;
	if(list == NULL)
		return NULL;
	gcdisable();
	prev = NULL;
	do {
		next = list->next;
		list->next = prev;
		prev = list;
	} while((list = next) != NULL);
	gcenable();
	return prev;
}

/* append -- merge two lists, non-destructively */
extern List *
really_old_append(List *head, List *tail)
{
	List *lp, **prevp;
	Ref(List *, hp, head);
	Ref(List *, tp, tail);
	gcreserve(40 * sizeof(List));
	gcdisable();
	head = hp;
	tail = tp;
	RefEnd2(tp, hp);

	for(prevp = &lp; head != NULL; head = head->next) {
		List *np = mklist(head->term, NULL);
		*prevp = np;
		prevp = &np->next;
	}
	*prevp = tail;

	Ref(List *, result, lp);
	gcenable();
	RefReturn(result);
}

void
append_start(AppendContext *ctx)
{
	ctx->result = nil;
	ctx->rlp = nil;
	ctx->started = 1;
	gcref(&ctx->r_result, (void **)&ctx->result);
	gcref(&ctx->r_rlp, (void **)&ctx->rlp);
}

List *
append_end(AppendContext *ctx)
{
	gcrderef(&ctx->r_rlp);
	gcrderef(&ctx->r_result);
	return ctx->result;
}

int
partial_append(AppendContext *ctx, List *list0)
{
	List *list = nil; Root r_list;
	List *lp = nil; Root r_lp;

	if(!ctx->started)
		return -1;

	gcref(&r_list, (void **)&list);
	gcref(&r_lp, (void **)&lp);

	list = list0;
	gcdisable();
	for(lp = list; lp != nil; lp = lp->next) {
		if(ctx->result == nil) {
			ctx->result = mklist(lp->term, nil);
			ctx->rlp = ctx->result;
		} else {
			ctx->rlp->next = mklist(lp->term, nil);
			ctx->rlp = ctx->rlp->next;
		}
	}
	gcenable();

	gcrderef(&r_lp);
	gcrderef(&r_list);
	return 0;
}

List *
using_partial_append(List *head0, List *tail0)
{
	List *head = nil; Root r_head;
	List *tail = nil; Root r_tail;
	List *result = nil; Root r_result;
	AppendContext ctx;

	gcref(&r_head, (void **)&head);
	gcref(&r_tail, (void **)&tail);
	gcref(&r_result, (void **)&result);

	head = head0;
	tail = tail0;
	append_start(&ctx);
	partial_append(&ctx, head);
	partial_append(&ctx, tail);
	result = append_end(&ctx);

	gcrderef(&r_result);
	gcrderef(&r_tail);
	gcrderef(&r_head);

	return result;
}

extern size_t old_ngcs;

List *
append(List *head0, List *tail0)
{
	List *head = nil; Root r_head;
	List *tail = nil; Root r_tail;
	List *result = nil; Root r_result;
	List *lp = nil; Root r_lp;
	List *rp = nil; Root r_rp;
	List *tmp = nil; Root r_tmp;
	Term *term = nil; Root r_term;
	volatile size_t startgc = 0;

	if(head0 == nil)
		return tail0;

	gcref(&r_head, (void **)&head);
	gcref(&r_tail, (void **)&tail);
	gcref(&r_result, (void **)&result);
	gcref(&r_lp, (void **)&lp);
	gcref(&r_rp, (void **)&rp);
	gcref(&r_tmp, (void **)&tmp);
	gcref(&r_term, (void **)&term);

	head = head0;
	tail = tail0;

	if(gctype == OldGc)
		startgc = old_ngcs;
	used(&startgc);

	for(lp = head; lp != nil; lp = lp->next) {
		term = lp->term;
		tmp = mklist(term, nil);
		if(result == nil) {
			result = tmp;
			rp = result;
			continue;
		}
		rp->next = tmp;
		rp = rp->next;
	}

	rp->next = tail;

	gcrderef(&r_term);
	gcrderef(&r_tmp);
	gcrderef(&r_rp);
	gcrderef(&r_lp);
	gcrderef(&r_result);
	gcrderef(&r_tail);
	gcrderef(&r_head);
	return result;
}

/* listcopy -- make a copy of a list */
extern List *
listcopy(List *list)
{
	return append(list, NULL);
}

/* length -- lenth of a list */
extern int
length(List *list)
{
	int len = 0;
	for(; list != NULL; list = list->next)
		++len;
	return len;
}

/* listify -- turn an argc/argv vector into a list */
extern List *
listify(int argc, char **argv)
{
	List *list = nil; Root r_list;
	int i;
	char *s;

	gcref(&r_list, (void **)&list);

	for(i = 1; i <= argc; i++) {
		s = argv[argc - i];
		list = mklist(mkstr(s), list);
	}

	gcrderef(&r_list);
	return list;
}

/* nth -- return nth element of a list, indexed from 1 */
extern Term *
nth(List *list, int n)
{
	assert(n > 0);
	for(; list != NULL; list = list->next) {
		assert(list->term != NULL);
		if(--n == 0)
			return list->term;
	}
	return NULL;
}

/* sortlist */
List *
sortlist(List *list)
{
	Vector *v = nil; Root r_v;
	List *l = nil; Root r_l;
	List *lp = nil; Root r_lp;

	gcref(&r_v, (void **)&v);
	gcref(&r_l, (void **)&l);
	gcref(&r_lp, (void **)&lp);
	l = list;

	if(length(list) > 1) {
		v = vectorize(l);
		sortvector(v);
		gcdisable();
		lp = listify(v->count, v->vector);
		gcenable();
	} else
		lp = list;

	gcrderef(&r_lp);
	gcrderef(&r_l);
	gcrderef(&r_v);
	return lp;
}
