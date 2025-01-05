#include "es.h"
#include "gc.h"

#define nil ((void*)0)

/* simple allocator */

typedef struct Region Region;
typedef struct Block Block;

struct Region {
	size_t start;
	size_t size;
	Region *next;
};

struct Block {
	Block *prev;
	Block *next;
	size_t size; /* includes the size of this header */
};

Region *regions;

Block *usedlist;
Block *freelist;
size_t nfrees = 0;
size_t nallocs = 0;
size_t bytesfree = 0;
size_t bytesused = 0;
size_t allocations = 0;

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

void /* i want to keep the free list ordered for coalescing */
add_to_freelist(Block *b)
{
	Block *fl;

	nfrees++;
	bytesfree += b->size;

	if(!freelist){
		b->next = nil;
		b->prev = nil;
		freelist = b;
		return;
	}

	for(fl = freelist; fl->next != nil; fl = fl->next){
		if(fl->next > b)
			break;
	}
	b->next = fl->next;
	if(fl->next != nil)
		fl->next->prev = b;
	fl->next = b;
	b->prev = fl;
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
	if(a+a->size != b)
		return -3;

	a->next = b->next;
	a->size += b->size;
	a->next->prev = a;
	return 0;
}

void*
coalesce(Block *a, Block *b)
{
	coalesce1(a, b);
	return a;
}

void
coalesce_freelist(void)
{
	Block *fl;

	if(!freelist)
		return;
	
	for(fl = freelist; fl != nil; fl = fl->next)
		coalesce(fl, fl->next);
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

Block*
allocate2(size_t sz)
{
	Block *b, *cb, *fl;
	Block *ahead, *behind;
	size_t realsize;
	
	realsize = sizeof(Block) + sz;
	for(fl = freelist; fl != nil; fl = fl->next)
		if(fl->size >= realsize)
			break;

	if(fl == nil)
		return nil;

	b = fl;
	if(b->size > realsize && b->size-realsize > minsize){
		cb = (void*)(((char*)b) + realsize);
		cb->size = b->size - realsize;
		cb->next = b->next;
		cb->prev = b;
		b->next = cb;
		b->size = realsize;
	}
	ahead = b->next;
	behind = b->prev;
	if(b == freelist){
		freelist = ahead;
		if(ahead)
			ahead->prev = nil;
	} else {
		if(ahead)
			ahead->prev = behind;
		behind->next = ahead;
	}

	b->next = nil;
	b->prev = nil;
	nallocs++;
	bytesfree -= realsize;
	bytesused += realsize;
	return b;
}

void*
allocate1(size_t sz)
{
	Block *b;

	if(!(b = allocate2(sz)))
		return nil;
	b->next = usedlist;
	if(usedlist)
		usedlist->prev = b;
	usedlist = b;

	return block_pointer(b);
}

void
deallocate1(void *p)
{
	Block *b, *prev, *next;

	b = pointer_block(p);
	next = b->next;
	prev = b->prev;
	bytesused -= b->size;

	if(b == usedlist){
		usedlist = usedlist->next;
		if(usedlist)
			usedlist->prev = nil;
	} else {
		if(next)
			next->prev = prev;
		if(prev)
			prev->next = next;
	}

	add_to_freelist(b);
	coalesce(b, b->next);
	if(b->prev)
		b = coalesce(b->prev, b);
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
	assert(h->tag == NULL || h->tag->magic == TAGMAGIC);
	
	if(h->tag == NULL){
		if(gcverbose)
			dprintf(2, ">>> marking NULL %p\n", p);
		gc_set_mark(h);
		return;
	}

	t = h->tag;
	if(gcverbose)
		dprintf(2, ">>>> marking %s %p\n", t->typename, p);
	(t->mark)(p);
}

void
gc_markrootlist(Root *r)
{
	void *p;

	for(; r != NULL; r = r->next){
		p = *(r->p);
		gcmark(p);
	}
}

size_t
gcsweep(void)
{
	Block *l;
	Header *h;
	size_t frees;

	for(frees = 0, l = usedlist; l != nil; l = l->next){
		h = (Header*)block_pointer(l);
		if(h->flags & GcUsed){
			dprintf(2, ">>> unmarking %p\n", h);
			gc_unset_mark(h);
			continue;
		}
		dprintf(2, ">>> deallocating %p\n", h);
		deallocate1((void*)h);
		frees++;
	}
	return frees;
}

void
gc_print_stats(GcStats *stats)
{
	dprintf(2, "tfree = %lu, rfree = %lu\n", stats->total_free, stats->real_free);
	dprintf(2, "tused = %lu, rused = %lu\n", stats->total_used, stats->real_used);
	dprintf(2, "free blocks = %lu, used blocks = %lu\n", stats->free_blocks, stats->used_blocks);
}

void
ms_initgc(void)
{
	GcStats stats;
	Block *b;

	if(gcverbose || gcinfo)
		dprintf(2, "Starting mark/sweep GC\n");

	b = create_block(blocksize);
	add_to_freelist(b);

	if(gcverbose || gcinfo){
		gc_getstats(&stats);
		gc_print_stats(&stats);
	}
}

void
ms_gc(void)
{
	GcStats starting, ending;

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
	allocations = 0;

	if(gcverbose || gcinfo){
		gc_getstats(&ending);
		dprintf(2, "Starting stats:\n");
		gc_print_stats(&starting);
		dprintf(2, "Ending stats:\n");
		gc_print_stats(&ending);
	}

	if(gcverbose)
		dprintf(2, "GC finished\n");
}

void*
ms_gcallocate(size_t sz, Tag *t)
{
	Block *b;
	Header *nb;
	size_t realsz;

	assert(t == NULL || t->magic == TAGMAGIC);

	realsz = ALIGN(sz + sizeof(Header));
	nb = allocate1(realsz);
	if(nb)
		goto done;
	if(allocations > 100){
		ms_gc();
		nb = allocate1(realsz);
		if(nb)
			goto done;
	}
	while(realsz+sizeof(Block) >= blocksize)
		blocksize *= 2;
	b = create_block(blocksize);
	add_to_freelist(b);
	nb = allocate1(realsz);
	assert(nb != nil);
done:
	nb->tag = t;
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
	void *p;

	for(l = usedlist; l != nil; l = l->next){
		h = (Header*)block_pointer(l);
		p = (void*)(((char*)h)+sizeof(Header));
		dump(h->tag, p);
	}
}

void
ms_gcenable(void)
{
	assert(gcblocked > 0);
	gcblocked--;
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

