/* gc.c -- copying garbage collector for es ($Revision: 1.2 $) */

#define	GARBAGE_COLLECTOR	1	/* for es.h */

#include <es.h>
#include <gc.h>

#define	ALIGN(n)	(((n) + sizeof (void *) - 1) &~ (sizeof (void *) - 1))

typedef struct Space Space;
struct Space {
	char *current, *bot, *top;
	Space *next;
};

#define	SPACESIZE(sp)	((size_t)((sp)->top - (sp)->bot))
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

/* own variables */
static Space *new, *old;
#if GCPROTECT
static Space *spaces;
#endif
static size_t minspace = MIN_minspace;	/* minimum number of bytes in a new space */

/* accounting */
size_t old_nallocs = 0;
size_t old_allocations = 0;
size_t old_ngcs = 0;

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

Boolean
old_istracked(void *p)
{
	return isinspace(new, p);
}

/*
 * root list building and scanning
 */

/* forward -- forward an individual pointer from old space */
extern void*
forward(void *p)
{
	Header *h, *nh;
	Tag *tag;
	void *np;

	if(!isinspace(old, p)){
		VERBOSE(("GC %8ux : <<not in old space>>\n", p));
		return p;
	}

	h = header(p);
	tag = gettag(h->tag);
	assert(tag != NULL);
	old_nallocs++;

	if(h->forward){
		np = h->forward;
		nh = header(np);
		VERBOSE(("%s	-> %8ux (followed)\n", tag->typename, np));
	} else {
		np = (tag->copy)(p);
		nh = header(np);
		h->forward = np;
		nh->forward = NULL;
		VERBOSE(("%s	-> %8ux (forwarded)\n", tag->typename, np));
	}

	/* this is probably not needed now */
	assert(gettag(header(np)->tag)->magic == TAGMAGIC);
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
	Tag *t;

	scanned = NULL;
	for(;;){
		front = new;
		for(sp = new; sp != scanned; sp = sp->next){
			scan = sp->bot;
			while(scan < sp->current){
				h = (Header*)scan;
				t = gettag(h->tag);
				scan += sizeof(Header);
				VERBOSE(("GC %8ux : %s	scan\n", scan, gettag(h->tag)->typename));
				scan += ALIGN((t->scan)(scan));
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

		old_nallocs = 0;
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
	old_allocations = 0;
	old_ngcs++;
}

/* initgc -- initialize the garbage collector */
extern void old_initgc(void) {
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
old_gcallocate(size_t nbytes, int t)
{
	Header *hp;
	char *np;
	char *p;
	size_t n;

#if GCALWAYS
	gc();
#endif

	gettag(t);
	n = ALIGN(nbytes + sizeof(Header));
	for(;;){
		hp = (void*)new->current;
		np = ((char*)hp) + n;
		if(np <= new->top){
			new->current = np;
			p = ((char*)hp)+sizeof(Header);
			hp->flags = 0;
			hp->tag = t;
			hp->forward = NULL;
			hp->size = nbytes;
			hp->refs = 1;
			old_nallocs++;
			old_allocations++;
			return (void*)p;
		}
		if(minspace < n)
			minspace = n;
		if(gcblocked)
			new = newspace(new);
		else
			old_gc();
	}
}

void
old_getstats(GcStats *stats)
{
	size_t total_used, total_free;
	char *current, *top, *bottom;

	current = (char*)((void*)new->current);
	top = (char*)((void*)new->top);
	bottom = (char*)((void*)new->bot);
	total_used = current - bottom;
	total_free = top - current;

	stats->total_free = total_free;
	stats->total_used = total_used;
	stats->nallocs = old_nallocs;
	stats->allocations = old_allocations;
	stats->ngcs = old_ngcs;
}

extern void old_memdump(void) {
/*	Space *sp;
	Tag *tag;

	for (sp = new; sp != NULL; sp = sp->next) {
		char *scan = sp->bot;
		while (scan < sp->current) {
			tag = gettag(((Header*)scan)->tag);
			assert(tag->magic == TAGMAGIC);
			scan += sizeof (Tag *);
			scan += ALIGN(dump(tag, scan));
		}
	}
*/
	return; /* stubbed out */
}

