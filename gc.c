/* gc.c -- copying garbage collector for es ($Revision: 1.2 $) */

#define	GARBAGE_COLLECTOR	1	/* for es.h */

#include "es.h"
#include "gc.h"

#define	ALIGN(n)	(((n) + sizeof (void *) - 1) &~ (sizeof (void *) - 1))

typedef struct Space Space;
struct Space {
	char *current, *bot, *top;
	Space *next;
};

#define	SPACESIZE(sp)	(((sp)->top - (sp)->bot))
#define	SPACEFREE(sp)	(((sp)->top - (sp)->current))
#define	SPACEUSED(sp)	(((sp)->current - (sp)->bot))
#define	INSPACE(p, sp)	((sp)->bot <= (char *) (p) && (char *) (p) < (sp)->top)


#if GCPROTECT
#define	NSPACES		10
#endif

#if HAVE_SYSCONF
# ifndef _SC_PAGESIZE
#  undef HAVE_SYSCONF
#  define HAVE_SYSCONF 0
# endif
#endif


/* globals */
Root *rootlist;
int gcblocked = 0;
Tag StringTag;

/* own variables */
static Space *new, *old;
#if GCPROTECT
static Space *spaces;
#endif
static size_t minspace = MIN_minspace;	/* minimum number of bytes in a new space */


/*
 * debugging
 */

#define	VERBOSE(p)	STMT(if (gcverbose) eprint p)

/*
 * GCPROTECT
 *	to use the GCPROTECT option, you must provide the following functions
 *		initmmu
 *		take
 *		release
 *		invalidate
 *		revalidate
 *	for your operating system
 */

#if GCPROTECT
#if __MACH__

/* mach versions of mmu operations */

#include <mach.h>
#include <mach_error.h>

#define	PAGEROUND(n)	((n) + vm_page_size - 1) &~ (vm_page_size - 1)

/* initmmu -- initialization for memory management calls */
static void initmmu(void) {
}

/* take -- allocate memory for a space */
static void *take(size_t n) {
	vm_address_t addr;
	kern_return_t error = vm_allocate(task_self(), &addr, n, TRUE);
	if (error != KERN_SUCCESS) {
		mach_error("vm_allocate", error);
		exit(1);
	}
	memset((void *) addr, 0xC9, n);
	return (void *) addr;
}

/* release -- deallocate a range of memory */
static void release(void *p, size_t n) {
	kern_return_t error = vm_deallocate(task_self(), (vm_address_t) p, n);
	if (error != KERN_SUCCESS) {
		mach_error("vm_deallocate", error);
		exit(1);
	}
}

/* invalidate -- disable access to a range of memory */
static void invalidate(void *p, size_t n) {
	kern_return_t error = vm_protect(task_self(), (vm_address_t) p, n, FALSE, 0);
	if (error != KERN_SUCCESS) {
		mach_error("vm_protect 0", error);
		exit(1);
	}
}

/* revalidate -- enable access to a range of memory */
static void revalidate(void *p, size_t n) {
	kern_return_t error =
		vm_protect(task_self(), (vm_address_t) p, n, FALSE, VM_PROT_READ|VM_PROT_WRITE);
	if (error != KERN_SUCCESS) {
		mach_error("vm_protect VM_PROT_READ|VM_PROT_WRITE", error);
		exit(1);
	}
	memset(p, 0x4F, n);
}

#else /* !__MACH__ */

/* sunos-derived mmap(2) version of mmu operations */

#include <sys/mman.h>

static int pagesize;
#define	PAGEROUND(n)	((n) + pagesize - 1) &~ (pagesize - 1)

/* take -- allocate memory for a space */
static void *take(size_t n) {
	caddr_t addr;
#ifdef MAP_ANONYMOUS
	addr = mmap(0, n, PROT_READ|PROT_WRITE,	MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
#else
	static int devzero = -1;
	if (devzero == -1)
		devzero = eopen("/dev/zero", oOpen);
	addr = mmap(0, n, PROT_READ|PROT_WRITE, MAP_PRIVATE, devzero, 0);
#endif
	if (addr == (caddr_t) -1)
		panic("mmap: %s", esstrerror(errno));
	memset(addr, 0xA5, n);
	return addr;
}

/* release -- deallocate a range of memory */
static void release(void *p, size_t n) {
	if (munmap(p, n) == -1)
		panic("munmap: %s", esstrerror(errno));
}

/* invalidate -- disable access to a range of memory */
static void invalidate(void *p, size_t n) {
	if (mprotect(p, n, PROT_NONE) == -1)
		panic("mprotect(PROT_NONE): %s", esstrerror(errno));
}

/* revalidate -- enable access to a range of memory */
static void revalidate(void *p, size_t n) {
	if (mprotect(p, n, PROT_READ|PROT_WRITE) == -1)
		panic("mprotect(PROT_READ|PROT_WRITE): %s", esstrerror(errno));
}

/* initmmu -- initialization for memory management calls */
static void initmmu(void) {
#if HAVE_SYSCONF
	pagesize = sysconf(_SC_PAGESIZE);
#else
	pagesize = getpagesize();
#endif
}

#endif	/* !__MACH__ */
#endif	/* GCPROTECT */


/*
 * ``half'' space management
 */

#if GCPROTECT

/* mkspace -- create a new ``half'' space in debugging mode */
static Space *mkspace(Space *space, Space *next) {
	assert(space == NULL || (&spaces[0] <= space && space < &spaces[NSPACES]));

	if (space != NULL) {
		Space *sp;
		if (space->bot == NULL)
			sp = NULL;
		else if (SPACESIZE(space) < minspace)
			sp = space;
		else {
			sp = space->next;
			revalidate(space->bot, SPACESIZE(space));
		}
		while (sp != NULL) {
			Space *tail = sp->next;
			release(sp->bot, SPACESIZE(sp));
			if (&spaces[0] <= space && space < &spaces[NSPACES])
				sp->bot = NULL;
			else
				efree(sp);
			sp = tail;
		}
	}

	if (space == NULL) {
		space = ealloc(sizeof (Space));
		memzero(space, sizeof (Space));
	}
	if (space->bot == NULL) {
		size_t n = PAGEROUND(minspace);
		space->bot = take(n);
		space->top = space->bot + n / (sizeof (*space->bot));
	}

	space->next = next;
	space->current = space->bot;

	return space;
}
#define	newspace(next)		mkspace(NULL, next)

#else	/* !GCPROTECT */

/* newspace -- create a new ``half'' space */
static Space *newspace(Space *next) {
	size_t n = ALIGN(minspace);
	Space *space = ealloc(sizeof (Space) + n);
	space->bot = (void *) &space[1];
	space->top = (void *) (((char *) space->bot) + n);
	space->current = space->bot;
	space->next = next;
	return space;
}

#endif	/* !GCPROTECT */

/* deprecate -- take a space and invalidate it */
static void deprecate(Space *space) {
#if GCPROTECT
	Space *base;
	assert(space != NULL);
	for (base = space; base->next != NULL; base = base->next)
		;
	assert(&spaces[0] <= base && base < &spaces[NSPACES]);
	for (;;) {
		invalidate(space->bot, SPACESIZE(space));
		if (space == base)
			break;
		else {
			Space *next = space->next;
			space->next = base->next;
			base->next = space;
			space = next;
		}
	}
#else
	while (space != NULL) {
		Space *old = space;
		space = space->next;
		efree(old);
	}

#endif
}

/* isinspace -- does an object lie inside a given Space? */
extern Boolean isinspace(Space *space, void *p) {
	for (; space != NULL; space = space->next)
		if (INSPACE(p, space)) {
		 	assert((char *) p < space->current);
		 	return TRUE;
		}
	return FALSE;
}

/*
 * root list building and scanning
 */

/* forward -- forward an individual pointer from old space */
extern void*
forward(void *p)
{
	Header *h, *nh;
	void *np;

	if(!isinspace(old, p)){
		VERBOSE(("GC %8ux : <<not in old space>>\n", p));
		return p;
	}

	h = header(p);
	assert(h->tag != NULL);
	assert(h->tag->magic == TAGMAGIC);

	if(h->forward){
		np = h->forward;
		nh = header(np);
		VERBOSE(("%s	-> %8ux (followed)\n", nh->tag->typename, np));
	} else {
		np = (h->tag->copy)(p);
		nh = header(np);
		h->forward = np;
		nh->forward = NULL;
		VERBOSE(("%s	-> %8ux (forwarded)\n", nh->tag->typename, np));
	}

	assert(nh->tag->magic == TAGMAGIC);
	return np;
}


/* scanroots -- scan a rootlist */
static void scanroots(Root *rootlist) {
	Root *root;
	for (root = rootlist; root != NULL; root = root->next) {
		VERBOSE(("GC root at %8lx: %8lx\n", root->p, *root->p));
		*root->p = forward(*root->p);
	}
}

/* scanspace -- scan new space until it is up to date */
static void
scanspace(void)
{
	Space *sp, *scanned, *front;
	char *scan;
	Header *h;

	scanned = NULL;
	for(;;){
		front = new;
		for(sp = new; sp != scanned; sp = sp->next){
			scan = sp->bot;
			while(scan < sp->current){
				h = (Header*)scan;
				assert(h->tag->magic == TAGMAGIC);
				scan += sizeof(Header);
				VERBOSE(("GC %8ux : %s	scan\n", scan, h->tag->typename));
				scan += ALIGN((h->tag->scan)(scan));
			}
		}
		if(new == front)
			break;
		scanned = front;
	}
}

/*
 * the garbage collector public interface
 */

/* gcenable -- enable collections */
extern void old_gcenable(void) {
	assert(gcblocked > 0);
	--gcblocked;
	if (!gcblocked && new->next != NULL)
		gc();
}

/* gcdisable -- disable collections */
extern void old_gcdisable(void) {
	assert(gcblocked >= 0);
	++gcblocked;
}

/* gcreserve -- provoke a collection if there's not a certain amount of space around */
extern void old_gcreserve(size_t minfree) {
	if (SPACEFREE(new) < (int)minfree) {
		if (minspace < minfree)
			minspace = minfree;
		old_gc();
	}
#if GCALWAYS
	else
		old_gc();
#endif
}

/* gcisblocked -- is collection disabled? */
extern Boolean old_gcisblocked(void) {
	assert(gcblocked >= 0);
	return gcblocked != 0;
}

/* gc -- actually do a garbage collection */
extern void old_gc(void) {
		size_t livedata;
		Space *space;
		size_t olddata;

	do {
		olddata = 0;
		
		if (gcinfo)
			for (space = new; space != NULL; space = space->next)
				olddata += SPACEUSED(space);
		
		assert(gcblocked >= 0);
		if (gcblocked > 0)
			return;
		++gcblocked;

		assert(new != NULL);
		assert(old == NULL);
		old = new;
#if GCPROTECT
		for (; new->next != NULL; new = new->next)
			;
		if (++new >= &spaces[NSPACES])
			new = &spaces[0];
		new = mkspace(new, NULL);
#else
		new = newspace(NULL);
#endif
		VERBOSE(("\nGC collection starting\n"));
		/* gc_markrootlist(rootlist); */
		if(gcverbose == TRUE){
			for (space = old; space != NULL; space = space->next)
				VERBOSE(("GC old space = %ux ... %ux\n", space->bot, space->current));
		}
		VERBOSE(("GC new space = %ux ... %ux\n", new->bot, new->top));
		VERBOSE(("GC scanning root list\n"));
		scanroots(rootlist);
		VERBOSE(("GC scanning global root list\n"));
		scanroots(globalrootlist);
		VERBOSE(("GC scanning exception root list\n"));
		scanroots(exceptionrootlist);
		VERBOSE(("GC scanning new space\n"));
		scanspace();
		VERBOSE(("GC collection done\n\n"));

		deprecate(old);
		old = NULL;

		for (livedata = 0, space = new; space != NULL; space = space->next)
			livedata += SPACEUSED(space);

		if (gcinfo)
			eprint(
				"[GC: old %8d  live %8d  min %8d  (pid %5d)]\n",
				olddata, livedata, minspace, getpid()
			);

		if (minspace < livedata * 2)
			minspace = livedata * 4;
		else if (minspace > livedata * 12 && minspace > (MIN_minspace * 2))
			minspace /= 2;

		--gcblocked;
	} while (new->next != NULL);
}

/* initgc -- initialize the garbage collector */
extern void initgc(void) {
#if GCPROTECT
	initmmu();
	spaces = ealloc(NSPACES * sizeof (Space));
	memzero(spaces, NSPACES * sizeof (Space));
	new = mkspace(&spaces[0], NULL);
#else
	new = newspace(NULL);
#endif
	old = NULL;
}

/*
 * allocation
 */

/* gcalloc -- allocate an object in new space */
extern void* /* use the same logic. that's solid */
gcalloc(size_t nbytes, Tag *tag)
{
	Header *hp;
	char *np;
	char *p;
	size_t n;

#if GCALWAYS
	gc();
#endif

	assert(tag == NULL || tag->magic == TAGMAGIC);

	n = ALIGN(nbytes + sizeof(Header));
	for(;;){
		hp = (void*)new->current;
		np = ((char*)hp) + n;
		if(np <= new->top){
			new->current = np;
			p = ((char*)hp)+sizeof(Header);
			hp->tag = tag;
			hp->forward = NULL;
			return (void*)p;
		}
		if(minspace < n)
			minspace = n;
		if(gcblocked)
			new = newspace(new);
		else
			gc();
	}
}

/*
 * strings
 */

#define notstatic
DefineTag(String, notstatic);

extern char *gcndup(const char *s, size_t n) {
	char *ns;

	gcdisable();
	ns = gcalloc((n + 1) * sizeof (char), &StringTag);
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
	char *np = gcalloc(n, &StringTag);
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

static char *tree1name(NodeKind k) {
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

static char *tree2name(NodeKind k) {
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


static size_t dump(Tag *t, void *p) {
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
		print("str = %ux  next = %ux\n", l->str, l->next);
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

extern void memdump(void) {
	Space *sp;
	for (sp = new; sp != NULL; sp = sp->next) {
		char *scan = sp->bot;
		while (scan < sp->current) {
			Tag *tag = *(Tag **) scan;
			assert(tag->magic == TAGMAGIC);
			scan += sizeof (Tag *);
			scan += ALIGN(dump(tag, scan));
		}
	}
}

