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
	size_t intype;
	size_t alloc;
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
volatile int rangc;
size_t gen = 0;

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
	Header *h;

	if(list == nil)
		return 0;
	if(list->prev == nil && list->next == nil)
		return 1;

	print = 0;
	for(i = 0, p = nil; list != nil; i++, list = list->next){
		assert(list->intype > 0);
		assert(p == list->prev);
		assert(p != list);
		assert(list->next != list);
		h = (Header*)block_pointer(list);
		assert(h->tag == NULL || h->tag->magic == TAGMAGIC);
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

void /* i want to keep the free list ordered for coalescing */
add_to_freelist(Block *b)
{
	Block *fl;
	Block *prev;

	nfrees++;
	bytesfree += b->size;

	b->next = nil;
	b->prev = nil;
	if(!freelist){
		freelist = b;
		return;
	}

	if(b < freelist){
		b->next = freelist;
		freelist->prev = b;
		freelist = b;
		return;
	}

	fl = usedlist;
	prev = fl->prev;
	while(b > fl && fl != nil){
		prev = fl;
		fl = fl->next;
	}

	if(fl == nil){
		fl = prev;
		b->prev = prev;
		b->next = nil;
		prev->next = b;
	} else { /* thanks a mistake from 9fans */
		b->prev = prev;
		b->next = prev->next;
		prev->next = b;
		if(b->next != nil)
			b->next->prev = b;
	}

	assert(b->prev != b && b->next != b);
}

void /* keeping the used list ordered will help with coalescing on free */
add_to_usedlist(Block *b)
{
	Block *ul, *prev;
	size_t len, i;

	nallocs++;
	bytesused += b->size;

	b->prev = nil;
	b->next = nil;
	b->intype = 0;
	b->alloc = gen++;

	len = checklist(usedlist);
	if(usedlist == nil){
//		dprintf(2, "initial commit to used list\n");
		b->intype = 1;
		usedlist = b;
		return;
	}

	if(b < usedlist){
//		dprintf(2, "head insert: b = %p, usedlist = %p, ul->prev = %p, ul->next = %p\n",
//					b, usedlist, usedlist->prev, usedlist->next);
		b->next = usedlist;
		usedlist->prev = b;
		usedlist = b;
		b->intype = 2;
		assert(len+1 == checklist(usedlist));
		return;
	}

	i = 0;
	ul = usedlist;
	prev = ul->prev;
	while(b > ul && ul != nil){
		prev = ul;
		ul = ul->next;
		i++;
	}

	b->prev = prev;
	b->next = prev->next;
	prev->next = b;
	if(b->next)
		b->next->prev = b;
	b->intype = 4;
	/*
	if(ul == nil){
//		dprintf(2, "tail insert(%zu): b = %p, ul = %p, ul->prev = %p, ul->next = %p\n",
//					i, b, prev, prev->prev, prev->next);
		b->prev = prev;
		b->next = nil;
		prev->next = b;
		b->intype = 3;
	} else { /* thanks a mistake from 9fans *
//		dprintf(2, "mid insert(%zu): b = %p, ul = %p, ul->prev = %p, ul->next = %p\n",
//					i, b, ul, ul->prev, ul->next);
		b->prev = prev;
		b->next = prev->next;
		prev->next = b;
		if(b->next != nil)
			b->next->prev = b;
		b->intype = 4;
	}*/

	assert(len+1 == checklist(usedlist));
	assert(b->prev != nil);
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
	return 0;
}

void*
coalesce(Block *a, Block *b)
{
	if(coalesce1(a, b) == 0 && b != nil && gcverbose)
		dprintf(2, ">>> coalescing blocks %p and %p\n", a, b);
	return a;
}

void
coalesce_freelist(void)
{
	Block *fl;

	if(!freelist)
		return;
	
	for(fl = freelist; fl != nil; fl = fl->next)
		fl = coalesce(fl, fl->next);
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
	bytesfree -= realsize;
	return b;
}

void*
allocate1(size_t sz)
{
	Block *b;

	if(!(b = allocate2(sz)))
		return nil;

	add_to_usedlist(b);
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
	Block *new_usedlist, *cur, *next;
	Block *new_usedlist_head;
	Header *h;
	size_t frees;
	
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
			if(!new_usedlist)
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
			cur = coalesce(cur, cur->next);
			if(cur->prev)
			coalesce(cur->prev, cur);
			frees++;
			nallocs--;
		}
		cur = next;
	}
	usedlist = new_usedlist_head;
	checklist(usedlist);
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

	rangc = 0;
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
		dprintf(2, "nfrees = %lu, nallocs = %lu\n", nfrees, nallocs);
	}

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
	Tag *t;

	t = gettag(tag);
	assert(t == NULL || t->magic == TAGMAGIC);

	realsz = ALIGN(sz + sizeof(Header));
	nb = allocate1(realsz);
	if(nb)
		goto done;

	ms_gc();
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

