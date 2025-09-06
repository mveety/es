/* vec.c -- argv[] and envp[] vectors ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"

DefineTag(Vector, static);

extern Vector *mkvector(int n) {
	int i;
	Vector *v = gcalloc(offsetof(Vector, vector[n + 1]), tVector);
	v->alloclen = n;
	v->count = 0;
	for (i = 0; i <= n; i++)
		v->vector[i] = NULL;
	return v;
}

static void *VectorCopy(void *ov) {
	size_t n = offsetof(Vector, vector[((Vector *) ov)->alloclen + 1]);
	void *nv = gcalloc(n, tVector);
	memcpy(nv, ov, n);
	return nv;
}

static size_t VectorScan(void *p) {
	Vector *v = p;
	int i, n = v->count;
	for (i = 0; i <= n; i++)
		v->vector[i] = forward(v->vector[i]);
	return offsetof(Vector, vector[v->alloclen + 1]);
}

void
VectorMark(void *p)
{
	Vector *v;
	int i;

	v = (Vector*)p;
	gc_set_mark(header(p));
	for(i = 0; i <= v->count; i++)
		gcmark(v->vector[i]);
}

extern Vector *vectorize(List *list) {
	Vector *v = nil; Root r_v;
	List *lp = nil; Root r_lp;
	char *s = nil; Root r_s;
	int i, n;

	gcref(&r_v, (void**)&v);
	gcref(&r_lp, (void**)&lp);
	gcref(&r_s, (void**)&s);
	lp = list;
	n = length(lp);
	v = mkvector(n);
	v->count = n;

	/* dprintf(2, "vectorize start\n"); */
	for (i = 0; lp != NULL; lp = lp->next, i++) {
		assert(lp->term->kind != tkRegex);
		s = getstr(lp->term); /* must evaluate before v->vector[i] */
		/* dprintf(2, "s = %p, ", s);
		dprintf(2, "*s = \"%s\"\n", s); */
		assert(s != nil);
		v->vector[i] = s;
	}
	/* dprintf(2, "vectorize end\n"); */
	gcrderef(&r_s);
	gcrderef(&r_lp);
	gcrderef(&r_v);

	return v;
}

/* qstrcmp -- a strcmp wrapper for qsort */
extern int qstrcmp(const void *s1, const void *s2) {
	return strcmp(*(const char **)s1, *(const char **)s2);
}

/* sortvector */
extern void sortvector(Vector *v) {
	assert(v->vector[v->count] == NULL);
	qsort(v->vector, v->count, sizeof (char *), qstrcmp);
}
