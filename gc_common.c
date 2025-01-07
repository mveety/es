#define	GARBAGE_COLLECTOR	1	/* for es.h */

#include "es.h"
#include "gc.h"

#define nil ((void*)0)

extern Tag ClosureTag;
extern Tag BindingTag;
extern Tag DictTag;
extern Tag StringTag;
extern Tag ListTag;
extern Tag StrListTag;
extern Tag TermTag;
extern Tag Tree1Tag;
extern Tag Tree2Tag;
extern Tag VarTag;
extern Tag VectorTag;

Root *globalrootlist;
Root *exceptionrootlist;
Root *rootlist;
int gcblocked = 0;
int gctype = OldGc;
Tag *tags[12];

/* globalroot -- add an external to the list of global roots */
extern void globalroot(void *addr) {
	Root *root;

	if(assertions == TRUE) {
		for (root = globalrootlist; root != NULL; root = root->next)
			assert(root->p != addr);
	}

	root = ealloc(sizeof (Root));
	root->p = addr;
	if(globalrootlist)
		globalrootlist->prev = root;
	root->next = globalrootlist;
	globalrootlist = root;
}

extern void
exceptionroot(Root *root, List **e)
{
	Root *r;

	if(assertions == TRUE) {
		for(r = exceptionrootlist; r != NULL; r = r->next)
			assert(r->p != (void**)e);
	}

	root->p = (void**)e;
	if(exceptionrootlist)
		exceptionrootlist->prev = root;
	root->next = exceptionrootlist;
	exceptionrootlist = root;
}

extern void
exceptionunroot(void)
{
	assert(exceptionrootlist != NULL);
	exceptionrootlist = exceptionrootlist->next;
	if(exceptionrootlist)
		exceptionrootlist->prev = NULL;
}

Tag*
gettag(int t)
{
	assert(t >= tNil && (size_t)t < sizeof(tags));
	assert(tags[t] == nil || tags[t]->magic == TAGMAGIC);
	return tags[t];
}

Header*
header(void *p)
{
	void *v;

	v = (void*)(((char*)p)-sizeof(Header));
	return (Header*)v;
}

void
gcref(Root *r, void **p)
{
	r->p = p;
	if(rootlist)
		rootlist->prev = r;
	r->next = rootlist;
	rootlist = r;
}

void
gcderef(Root *r, void **p)
{
	assert(r == rootlist);
	assert(r->p == p);

	rootlist = rootlist->next;
	if(rootlist)
		rootlist->prev = NULL;
	r->next = NULL;
	r->prev = NULL;
}

void
initgc(void)
{
	tags[tNil] = nil;
	tags[tClosure] = &ClosureTag;
	tags[tBinding] = &BindingTag;
	tags[tDict] = &DictTag;
	tags[tString] = &StringTag;
	tags[tList] = &ListTag;
	tags[tStrList] = &StrListTag;
	tags[tTerm] = &TermTag;
	tags[tTree1] = &Tree1Tag;
	tags[tTree2] = &Tree2Tag;
	tags[tVar] = &VarTag;
	tags[tVector] = &VectorTag;

	if(gctype == NewGc)
		ms_initgc();
	else
		old_initgc();
}

void
gcrderef(Root *r)
{
	gcderef(r, r->p);
}

void
gc(void)
{
	if(gctype == NewGc)
		ms_gc();
	else
		old_gc();
}

void
gcenable(void)
{
	if(gctype == NewGc)
		ms_gcenable();
	else
		old_gcenable();
}

void
gcdisable(void)
{
	if(gctype == NewGc)
		ms_gcdisable();
	else
		old_gcdisable();
}

void
gcreserve(size_t sz)
{
	if(gctype == NewGc)
		return;
	old_gcreserve(sz);
}

Boolean
gcisblocked(void)
{
	if(gctype == NewGc)
		return ms_gcisblocked();
	return old_gcisblocked();
}

void*
gcalloc(size_t sz, int t)
{
	if(gctype == NewGc)
		return ms_gcallocate(sz, t);
	return old_gcallocate(sz, t);
}

/*
 * allocation of large, contiguous buffers for large object creation
 *	see the use of this in str().  note that this region may not
 *	contain pointers or '\0' until after sealbuffer() has been called.
 */

extern Buffer *openbuffer(size_t minsize) {
	Buffer *buf;
	if (minsize < 500)
		minsize = 500;
	buf = ealloc(offsetof(Buffer, str[minsize]));
	buf->len = minsize;
	buf->current = 0;
	return buf;
}

extern Buffer *expandbuffer(Buffer *buf, size_t minsize) {
	buf->len += (minsize > buf->len) ? minsize : buf->len;
	buf = erealloc(buf, offsetof(Buffer, str[buf->len]));
	return buf;
}

extern char *sealbuffer(Buffer *buf) {
	char *s = gcdup(buf->str);
	efree(buf);
	return s;
}

extern char *sealcountedbuffer(Buffer *buf) {
	char *s = gcndup(buf->str, buf->current);
	efree(buf);
	return s;
}

extern Buffer *bufncat(Buffer *buf, const char *s, size_t len) {
	while (buf->current + len >= buf->len)
		buf = expandbuffer(buf, buf->current + len - buf->len);
	memcpy(buf->str + buf->current, s, len);
	buf->current += len;
	return buf;
}

extern Buffer *bufcat(Buffer *buf, const char *s) {
	return bufncat(buf, s, strlen(s));
}

extern Buffer *bufputc(Buffer *buf, char c) {
	return bufncat(buf, &c, 1);
}

extern void freebuffer(Buffer *buf) {
	efree(buf);
}

/*
 * strings
 */

#define notstatic
DefineTag(String, notstatic);

extern char *gcndup(const char *s, size_t n) {
	char *ns;

	gcdisable();
	ns = gcalloc((n + 1) * sizeof (char), tString);
	memcpy(ns, s, n);
	ns[n] = '\0';
	assert(strlen(ns) == n);

	Ref(char *, result, ns);
	gcenable();
	RefReturn(result);
}

extern char *gcdup(const char *s) {
	return gcndup(s, strlen(s));
}

static void *StringCopy(void *op) {
	size_t n = strlen(op) + 1;
	char *np = gcalloc(n, tString);
	memcpy(np, op, n);
	return np;
}

static size_t StringScan(void *p) {
	return strlen(p) + 1;
}

static void
StringMark(void *p) {
	Header *h;

	h = header(p);
	gc_set_mark(h);
}


/*
 * memdump -- print out all of gc space, as best as possible
 */

char *tree1name(NodeKind k) {
	switch(k) {
	default:	panic("tree1name: bad node kind %d", k);
	case nPrim:	return "Prim";
	case nQword:	return "Qword";
	case nCall:	return "Call";
	case nThunk:	return "Thunk";
	case nVar:	return "Var";
	case nWord:	return "Word";
	}
}

char *tree2name(NodeKind k) {
	switch(k) {
	default:	panic("tree2name: bad node kind %d", k);
	case nAssign:	return "Assign";
	case nConcat:	return "Concat";
	case nClosure:	return "Closure";
	case nFor:	return "For";
	case nLambda:	return "Lambda";
	case nLet:	return "Let";
	case nLets: return "Lets";
	case nList:	return "List";
	case nLocal:	return "Local";
	case nMatch:	return "Match";
	case nExtract:	return "Extract";
	case nVarsub:	return "Varsub";
	}
}

/* having these here violates every data hiding rule in the book */

	typedef struct {
		char *name;
		void *value;
	} Assoc;
	struct Dict {
		int size, remain;
		Assoc table[1];		/* variable length */
	};

#include "var.h"
#include "term.h"


size_t dump(Tag *t, void *p) {
	char *s = t->typename;
	print("%8ux %s\t", p, s);

	if (streq(s, "String")) {
		print("%s\n", p);
		return strlen(p) + 1;
	}

	if (streq(s, "Term")) {
		Term *t = p;
		print("str = %ux  closure = %ux\n", t->str, t->closure);
		return sizeof (Term);
	}

	if (streq(s, "List")) {
		List *l = p;
		print("term = %ux  next = %ux\n", l->term, l->next);
		return sizeof (List);
	}

	if (streq(s, "StrList")) {
		StrList *l = p;
		print("str = %ux  next= %ux\n", l->str, l->next);
		return sizeof (StrList);
	}

	if (streq(s, "Closure")) {
		Closure *c = p;
		print("tree = %ux  binding = %ux\n", c->tree, c->binding);
		return sizeof (Closure);
	}

	if (streq(s, "Binding")) {
		Binding *b = p;
		print("name = %ux  defn = %ux  next = %ux\n", b->name, b->defn, b->next);
		return sizeof (Binding);
	}

	if (streq(s, "Var")) {
		Var *v = p;
		print("defn = %ux  env = %ux  flags = %d\n",
		      v->defn, v->env, v->flags);
		return sizeof (Var);
	}

	if (streq(s, "Tree1")) {
		Tree *t = p;
		print("%s	%ux\n", tree1name(t->kind), t->u[0].p);
		return offsetof(Tree, u[1]);
	}

	if (streq(s, "Tree2")) {
		Tree *t = p;
		print("%s	%ux  %ux\n", tree2name(t->kind), t->u[0].p, t->u[1].p);
		return offsetof(Tree, u[2]);
	}

	if (streq(s, "Vector")) {
		Vector *v = p;
		int i;
		print("alloclen = %d  count = %d [", v->alloclen, v->count);
		for (i = 0; i <= v->alloclen; i++)
			print("%s%ux", i == 0 ? "" : " ", v->vector[i]);
		print("]\n");
		return offsetof(Vector, vector[v->alloclen + 1]);
	}

	if (streq(s, "Dict")) {
		Dict *d = p;
		int i;
		print("size = %d  remain = %d\n", d->size, d->remain);
		for (i = 0; i < d->size; i++)
			print("\tname = %ux  value = %ux\n",
			      d->table[i].name, d->table[i].value);
		return offsetof(Dict, table[d->size]);
	}

	print("<<unknown>>\n");
	return 0;
}

void
memdump(void)
{
	if(gctype == NewGc)
		gc_memdump();
	else
		old_memdump();
}

