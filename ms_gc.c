#include "es.h"
#include "gc.h"

#define nil ((void*)0)

/* simple allocator */

typedef struct Region Region;
typedef struct Block Block;

struct Region {
	size_t start;
	size_t size;
	Block *startp;
	Region *next;
};

struct Block {
//	size_t intype;
//	size_t alloc;
	Block *prev;
	Block *next;
	size_t size; /* includes the size of this header */
//	Header *h;
};

Region *regions;

Block *usedlist;
Block *freelist;
size_t nfrees = 0;
size_t nallocs = 0;
size_t bytesfree = 0;
size_t bytesused = 0;
size_t allocations = 0;
volatile int rangc;
volatile int ms_gc_blocked;
volatile size_t ngcs = 0;
volatile int nsortgc = 0;
volatile int ncoalescegc = 0;
/* tuning */
int gc_after = 5000;
int gc_sort_after_n = 100;
int gc_coalesce_after_n = 100;
// size_t gen = 0;

size_t blocksize = MIN_minspace;

/* this is the smallest sensible size of a block (probably) */
const size_t minsize = sizeof(Block)+sizeof(Header)+sizeof(void*);

Block*
create_block(size_t sz)
{
	Block *b;
	Region *r;

	r = ealloc(sizeof(Region));
	b = ealloc(sz);
	b->size = sz;
	b->next = nil;
	b->prev = nil;
	r->start = (size_t)b;
	r->size = sz;
	r->next = regions;
	regions = r;
	return b;
}

void*
block_pointer(Block *b)
{
	return (void*)(((char*)b)+sizeof(Block));
}

Block*
pointer_block(void *p)
{
	return (Block*)(((char*)p)-sizeof(Block));
}

size_t
checklist2(Block *list, int print, void *b)
{
	Block *p;
	size_t i;

	if(list == nil)
		return 0;
	if(list->prev == nil && list->next == nil)
		return 1;

	print = 0;
	for(i = 0, p = nil; list != nil; i++, list = list->next){
		assert(p == list->prev);
		assert(p != list);
		assert(list->next != list);
		if(print){
			if(b){
				dprintf(2, "%lu: list->prev = %p, list = %p, list->next = %p, p = %p, b = %p\n",
						i, list->prev, list, list->next, p, b);

			} else {
				dprintf(2, "%lu: list->prev = %p, list = %p, list->next = %p, p = %p\n",
						i, list->prev, list, list->next, p);
			}
		}
		p = list;
	}
	return i;
}

size_t
checklist1(Block *list, int print)
{
	return checklist2(list, print, nil);
}

size_t
checklist(Block *list)
{
	return checklist1(list, 0);
}

Block* /* cheat a little */
sort_pivot(Block *list, size_t len)
{
	size_t i;

	for(i = 0; i < len/2 && list->next != nil; i++, list = list->next)
		;
	return list;
}


Block*
sort_list(Block *list, size_t len)
{
	Block *pivot;
	Block *p = nil, *n = nil, *l = nil;
	Block *left = nil, *right = nil;
	size_t leftlen = 0, rightlen = 0;
	Block *head;

	if(!list)
		return nil;
	if(!list->next)
		return list;

	pivot = sort_pivot(list, len);
	if(pivot->prev)
		pivot->prev->next = pivot->next;
	if(pivot->next)
	pivot->next->prev = pivot->prev;
	p = list;
	pivot->next = pivot->prev = nil;

	while(p){
		n = p;
		p = p->next;
		n->prev = n->next = nil;
		if(n < pivot){
			n->next = left;
			if(left)
				left->prev = n;
			left = n;
			leftlen++;
		} else {
			n->next = right;
			if(right)
				right->prev = n;
			right = n;
			rightlen++;
		}
	}

	left = sort_list(left, leftlen);
	right = sort_list(right, rightlen);

	head = left;
	l = left;
	if(left)
		for(; l->next != nil; l = l->next)
			;
	if(l)
		l->next = pivot;
	pivot->prev = l;
	pivot->next = right;
	if(right)
		right->prev = pivot;
	if(head)
		return head;
	else
		return pivot;
}

void
add_to_freelist(Block *b)
{
	size_t len;

	nfrees++;
	bytesfree += b->size;

	if(assertions)
		len = checklist(freelist);
	b->next = freelist;
	if(freelist)
		freelist->prev = b;
	freelist = b;
	if(assertions)
		assert(len+1 == checklist(freelist));
}

void
add_to_usedlist(Block *b)
{
	size_t len;

	nallocs++;
	bytesused += b->size;

	if(assertions)
		len = checklist(usedlist);
	b->next = usedlist;
	if(usedlist)
		usedlist->prev = b;
	usedlist = b;
	if(assertions)
		assert(len+1 == checklist(usedlist));
}

int
coalesce1(Block *a, Block *b)
{
	if(b == nil)
		return 0;

	assert(a->next == b && b->prev == a);

	if(b == nil)
		return -1;
	if(a->next == nil || b == nil)
		return -2;
	if(gcverbose)
		dprintf(2, "coalesce: a = %p, a->size = %lx, a+a->size = %lx, b = %p\n",
				a, a->size, (size_t)((char*)a)+a->size, b);
	if(((char*)a)+a->size != (void*)b)
		return -3;

	a->next = b->next;
	a->size += b->size;
	if(a->next)
		a->next->prev = a;
	nfrees--;
	return 0;
}

void*
coalesce(Block *a, Block *b)
{
	if(coalesce1(a, b) == 0 && b != nil && gcverbose)
		dprintf(2, ">>> coalescing blocks %p and %p\n", a, b);
	return a;
}

Block*
coalesce_list(Block *list)
{
	Block *l;

	if(!list)
		return nil;

	l = list;
	while(l != nil){
		l = coalesce(l, l->next);
		if(l->prev)
			coalesce(l->prev, l);
		l = l->next;
	}
	return list;
}

Block*
allocate2(size_t sz)
{
	Block *fl, *prev;
	size_t realsize;
	Block *new;
	char *p;
	size_t i;

	realsize = sizeof(Block) + sz;

	prev = nil;
	for(fl = freelist; fl != nil; prev = fl, fl = fl->next){
		if(fl->size >= realsize){
			if(fl->size == realsize || fl->size-realsize < minsize){
				new = fl;
				if(prev)
					prev->next = new->next;
				if(new->next)
					new->next->prev = prev;
				if(fl == freelist)
					freelist = new->next;
				nfrees--;
				break;
			}
			fl->size -= realsize;
			new = (void*)(((char*)fl) + fl->size);
			new->size = realsize;
			break;
		}
	}

	if(fl == nil)
		return nil;

	new->next = nil;
	new->prev = nil;
	bytesfree -= realsize;
	p = (void*)(((char*)new)+sizeof(Block));
	if(assertions){
		for(i = 0; i < realsize-sizeof(Block); i++)
			p[i] = 0;
	}
	return new;
}

void*
allocate1(size_t sz)
{
	Block *b;

	if(!(b = allocate2(sz)))
		return nil;

	add_to_usedlist(b);
	if(assertions)
		checklist(usedlist);

	return block_pointer(b);
}

size_t /* includes block headers */
usage_stats(Block *list)
{
	size_t stats;

	stats = 0;
	if(!list)
		return 0;

	for(; list != nil; list = list->next)
		stats += list->size;

	return stats;
}

size_t /* omits block headers */
real_usage_stats(Block *list)
{
	size_t stats;

	stats = 0;

	if(!list)
		return 0;

	for(; list != nil; list = list->next)
		stats += (list->size - sizeof(Block));

	return stats;
}

size_t /* number of blocks */
nblocks_stats(Block *list)
{
	size_t stats;

	stats = 0;
	if(!list)
		return 0;

	for(; list != nil; list = list->next)
		stats++;

	return stats;
}

void
gc_getstats(GcStats *stats)
{
	stats->total_free = usage_stats(freelist);
	stats->real_free = real_usage_stats(freelist);
	stats->total_used = usage_stats(usedlist);
	stats->real_used = real_usage_stats(usedlist);
	stats->free_blocks = nblocks_stats(freelist);
	stats->used_blocks = nblocks_stats(usedlist);
	stats->nfrees = nfrees;
	stats->nallocs = nallocs;
	stats->allocations = allocations;
	stats->ngcs = ngcs;
	stats->sort_after_n = gc_sort_after_n;
	stats->nsortgc = nsortgc;
	stats->coalesce_after = gc_coalesce_after_n;
	stats->ncoalescegc = ncoalescegc;
	stats->gc_after = gc_after;
}

int
ismanaged(void *p)
{
	Region *r;

	for(r = regions; r != nil; r = r->next)
		if(r->start <= (size_t)p && (r->start+r->size) > (size_t)p)
			return 1;
	return 0;
}

Boolean
gc_istracked(void *p)
{
	if(ismanaged(p))
		return TRUE;
	return FALSE;
}

/* mark sweep garbage collector */

void
gcmark(void *p)
{
	Header *h;
	Tag *t;

	if(p == NULL)
		return;

	if(!ismanaged(p)){
		if(gcverbose)
			dprintf(2, ">>> not marking unmanaged ptr %p\n", p);
		return;
	}

	h = header(p);
	
	if(h->tag == tNil){
		if(gcverbose)
			dprintf(2, ">>> marking NULL %p\n", p);
		gc_set_mark(h);
		return;
	}

	t = gettag(h->tag);
	if(gcverbose)
		dprintf(2, ">>>> marking %s %p\n", t->typename, p);
	(t->mark)(p);
}

void
gc_markrootlist(Root *r)
{
	void *p;
	Header *h;

	for(; r != NULL; r = r->next){
		if(ismanaged((void*)r)){
			h = header((void*)r);
			gc_set_mark(h);
		}
		p = *(r->p);
		gcmark(p);
	}
}

size_t
gcsweep(void)
{
	Block *new_usedlist = nil, *cur, *next;
	Block *new_usedlist_head = nil;
	Header *h;
	size_t frees;
	
	if(assertions)
		checklist(usedlist);
	cur = usedlist;
	frees = 0;
	for(;;){
		if(cur == nil)
			break;
		next = cur->next;
		if(next)
			next->prev = nil;
		h = (Header*)block_pointer(cur);
		if(h->flags & GcUsed){
			if(gcverbose)
				dprintf(2, ">>> unmarking %p\n", h);
			gc_unset_mark(h);
			cur->prev = new_usedlist;
			cur->next = nil;
			if(new_usedlist_head == nil)
				new_usedlist_head = cur;
			else
				new_usedlist->next = cur;
			new_usedlist = cur;
		} else {
			if(gcverbose)
				dprintf(2, ">>> deallocating %p\n", cur);
			bytesused -= cur->size;
			cur->next = nil;
			cur->prev = nil;
			add_to_freelist(cur);
			//cur = coalesce(cur, cur->next);
			//if(cur->prev)
			//	coalesce(cur->prev, cur);
			frees++;
			nallocs--;
		}
		cur = next;
	}
	usedlist = new_usedlist_head;
	if(assertions)
		checklist(usedlist);
	return frees;
}

void
gc_print_stats(GcStats *stats)
{
	dprintf(2, "tfree = %lu, rfree = %lu\n", stats->total_free, stats->real_free);
	dprintf(2, "tused = %lu, rused = %lu\n", stats->total_used, stats->real_used);
	dprintf(2, "free blocks = %lu, used blocks = %lu\n", stats->free_blocks, stats->used_blocks);
	dprintf(2, "nfrees = %lu, nallocs = %lu\n", stats->nfrees, stats->nallocs);
	dprintf(2, "allocations since last gc = %lu\n", stats->allocations);
	dprintf(2, "number of gc = %lu\n", stats->ngcs);
	dprintf(2, "gc_sort_after_n = %d, nsortgc = %d\n", stats->sort_after_n, stats->nsortgc);
	dprintf(2, "gc_coalesce_after_n = %d, ncoalescegc = %d\n",
			stats->coalesce_after, stats->ncoalescegc);
	dprintf(2, "gc_after = %d\n", stats->gc_after);

}

void
ms_initgc(void)
{
	GcStats stats;
	Block *b;

	if(gcverbose || gcinfo)
		dprintf(2, "Starting mark/sweep GC\n");

	rangc = 0;
	b = create_block(blocksize);
	add_to_freelist(b);

	if(gcverbose || gcinfo){
		gc_getstats(&stats);
		gc_print_stats(&stats);
	}
}

void
ms_gc(Boolean full)
{
	GcStats starting, ending;

	/* set things to their defaults if it's weird */
	if(gc_after < 1)
		gc_after = 5000;
	if(gc_sort_after_n < 1)
		gc_sort_after_n = 100;
	if(gc_coalesce_after_n < 1)
		gc_coalesce_after_n = 100;

	if(allocations < (size_t)gc_after && !full)
		return;

	if(gcblocked > 0)
		return;
	if(gcverbose)
		dprintf(2, "GC starting\n");
	
	if(gcverbose || gcinfo)
		gc_getstats(&starting);

	if(gcverbose)
		dprintf(2, ">> Marking\n");
	if(gcverbose)
		dprintf(2, ">>> gc_markrootlist(rootlist)\n");
	gc_markrootlist(rootlist);
	if(gcverbose)
		dprintf(2, ">>> gc_markrootlist(globalrootlist)\n");
	gc_markrootlist(globalrootlist);
	if(gcverbose)
		dprintf(2, ">>> gc_markrootlist(exceptionrootlist)\n");
	gc_markrootlist(exceptionrootlist);

	if(gcverbose)
		dprintf(2, ">> Sweeping\n");
	gcsweep();
	if(nsortgc >= gc_sort_after_n || full){
		freelist = sort_list(freelist, nfrees);
		nsortgc = 0;
	}
	if(ncoalescegc >= gc_coalesce_after_n || full){
		freelist = coalesce_list(freelist);
		ncoalescegc = 0;
	}

	ngcs++;
	nsortgc++;
	ncoalescegc++;

	if(gcverbose || gcinfo){
		gc_getstats(&ending);
		dprintf(2, "Starting stats:\n");
		gc_print_stats(&starting);
		dprintf(2, "Ending stats:\n");
		gc_print_stats(&ending);
	}

	allocations = 0;
	rangc = 1;
	if(gcverbose)
		dprintf(2, "GC finished\n");
}

void*
ms_gcallocate(size_t sz, int tag)
{
	Block *b;
	Header *nb;
	size_t realsz;

	gettag(tag);

	realsz = ALIGN(sz + sizeof(Header));
	nb = allocate1(realsz);
	if(nb)
		goto done;

	ms_gc(TRUE);
	nb = allocate1(realsz);
	if(nb)
		goto done;

	while(realsz+sizeof(Block) >= blocksize)
		blocksize *= 2;
	b = create_block(blocksize);
	add_to_freelist(b);
	nb = allocate1(realsz);

	assert(nb != nil);
done:
	nb->flags = 0;
	nb->tag = tag;
	nb->refcount = 0;
	nb->forward = nil;
	allocations++;
	return (void*)(((char*)nb)+sizeof(Header));
}

void
gc_set_mark(Header *h)
{
	if(!ismanaged((void*)h))
		return;
	h->flags |= GcUsed;
}

void
gc_unset_mark(Header *h)
{
	if(!ismanaged((void*)h))
		return;
	h->flags &= ~GcUsed;
}

void
gc_memdump(void)
{
	Block *l;
	Header *h;
	Tag *t;
	void *p;

	for(l = usedlist; l != nil; l = l->next){
		h = (Header*)block_pointer(l);
		p = (void*)(((char*)h)+sizeof(Header));
		t = gettag(h->tag);
		dump(t, p);
	}
}

void
ms_gcenable(void)
{
	assert(gcblocked > 0);
	gcblocked--;
	ms_gc(FALSE);
}

void
ms_gcdisable(void)
{
	assert(gcblocked >= 0);
	gcblocked++;
}

Boolean
ms_gcisblocked(void)
{
	assert(gcblocked >= 0);
	return gcblocked != 0;
}

