#include "es.h"
#include "gc.h"

/* simple allocator */

typedef struct BlockArray {
	Block **blocks;
	size_t len;
} BlockArray;

Region *regions;

Block *usedlist = nil;
Block *freelist = nil;
Block *oldlist = nil;
size_t nfrees = 0;
size_t nallocs = 0;
size_t bytesfree = 0;
size_t bytesused = 0;
size_t allocations = 0;
size_t nregions = 0;
size_t nsort = 0;
size_t ncoalesce = 0;
volatile int rangc;
volatile int ms_gc_blocked;
volatile size_t ngcs = 0;
volatile int nsortgc = 0;
volatile int ncoalescegc = 0;
volatile int noldsweep = 30;
/* tuning */
int gc_after = 30000;
int gc_sort_after_n = 1;
int gc_coalesce_after_n = 1;
int reserve_blocks = 1;
uint32_t gc_oldage = 5;
int gc_oldsweep_after = 15;
Boolean generational = FALSE;
uint64_t nsortsn = 0;
Boolean use_array_sort = FALSE;
Boolean use_list_sz = FALSE;
size_t triedgcs = 0;
// size_t gen = 0;

size_t blocksize = MIN_minspace;

/* this is the smallest sensible size of a block (probably) */
const size_t minsize = sizeof(Block) + sizeof(Header) + sizeof(void *);

Block *
create_block(size_t sz)
{
	Block *b;
	Region *r;

	r = ealloc(sizeof(Region));
	b = ealloc(sz);
	b->size = sz;
	b->next = nil;
	b->prev = nil;
	b->age = 0;
	r->start = (size_t)b;
	r->size = sz;
	r->next = regions;
	regions = r;
	nregions++;
	return b;
}

void *
block_pointer(Block *b)
{
	return (void *)(((char *)b) + sizeof(Block));
}

Block *
pointer_block(void *p)
{
	return (Block *)(((char *)p) - sizeof(Block));
}

size_t
checklist2(Block *list, int print, void *b)
{
	Block *p;
	size_t i = 0;

	if(list == nil)
		return 0;
	if(list->prev == nil && list->next == nil)
		return 1;

	print = 0;
	for(i = 0, p = nil; list != nil; i++, list = list->next) {
		assert(p == list->prev);
		assert(p != list);
		assert(list->next != list);
		if(print) {
			if(b)
				dprintf(2, "%lu: list->prev = %p, list = %p, list->next = %p, p = %p, b = %p\n", i,
						list->prev, list, list->next, p, b);
			else
				dprintf(2, "%lu: list->prev = %p, list = %p, list->next = %p, p = %p\n", i,
						list->prev, list, list->next, p);
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
list_rating(Block *list)
{
	Block *l;
	int64_t sortrating = 0;

	for(l = list; l != nil; l = l->next) {
		if(l->prev == nil)
			continue;
		if(l > l->prev)
			sortrating++;
		else
			sortrating--;
	}
	nsortsn++;
	dprintf(2, "%lu: sortrating = %ld (listlen = %lu)\n", nsortsn, sortrating, nblocks_stats(list));
}

BlockArray *
list_arrayify_sz(Block *list, size_t len)
{
	BlockArray *res;
	size_t i;

	res = ealloc(sizeof(BlockArray) + (sizeof(Block *) * len));
	res->len = len;
	res->blocks = (void *)(((char *)res) + sizeof(BlockArray));

	for(i = 0; list != nil && i < res->len; list = list->next, i++)
		res->blocks[i] = list;

	return res;
}

BlockArray *
list_arrayify(Block *list)
{
	return list_arrayify_sz(list, nblocks_stats(list));
}

Block *
array_listify(BlockArray *array)
{
	Block *res = nil;
	Block *p = nil;
	size_t i = 0;

	for(i = 0; i < array->len; i++) {
		if(res == nil) {
			res = array->blocks[i];
			assert(res != nil);
			res->next = nil;
			res->prev = nil;
			p = res;
		} else {
			p->next = array->blocks[i];
			p->next->prev = p;
			p->next->next = nil;
			p = p->next;
		}
	}

	efree(array);
	return res;
}

int
block_compare(const void *a, const void *b)
{
	Block **blocka;
	Block **blockb;

	blocka = (Block **)a;
	blockb = (Block **)b;

	if(*blocka < *blockb)
		return -1;
	if(*blocka > *blockb)
		return 1;
	// if(*blocka == *blockb)
	return 0;
}

Block *
l2a_sortlist_sz(Block *list, size_t len)
{
	BlockArray *array;
	Block *result;

	array = list_arrayify_sz(list, len);
	qsort(array->blocks, array->len, sizeof(Block *), &block_compare);
	result = array_listify(array);

	return result;
}

Block *
l2a_sortlist(Block *list)
{
	BlockArray *array;
	Block *result;

	array = list_arrayify(list);
	qsort(array->blocks, array->len, sizeof(Block *), &block_compare);
	result = array_listify(array);

	return result;
}

Block * /* cheat a little */
sort_pivot(Block *list, size_t len)
{
	size_t i;

	for(i = 0; i <= len / 3 && list->next != nil; i++, list = list->next)
		;
	return list;
}

Block *
l_sort_list(Block *list, size_t len)
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

	while(p) {
		n = p;
		p = p->next;
		n->prev = n->next = nil;
		if(n < pivot) {
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

	left = l_sort_list(left, leftlen);
	right = l_sort_list(right, rightlen);

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

	if(assertions) {
		for(l = head ? head : pivot; l != nil; l = l->next) {
			if(l->prev == nil)
				continue;
			assert(l->prev < l);
		}
	}

	if(head)
		return head;
	else
		return pivot;
}

Block *
sort_list(Block *list, size_t len)
{
	if(use_array_sort) {
		if(use_list_sz)
			return l2a_sortlist_sz(list, len);
		else
			return l2a_sortlist(list);
	}
	return l_sort_list(list, len);
}

void
add_to_freelist(Block *b)
{
	size_t len = 0;

	nfrees++;
	bytesfree += b->size;
	used(&len);

	if(gcverbose)
		dprintf(2, ">>> adding %p to freelist\n", b);
	if(assertions)
		len = checklist(freelist);
	b->next = freelist;
	if(freelist)
		freelist->prev = b;
	freelist = b;
	if(assertions)
		assert(len + 1 == checklist(freelist));
}

void
add_to_usedlist(Block *b)
{
	size_t len = 0;

	nallocs++;
	bytesused += b->size;
	used(&len);

	if(assertions)
		len = checklist(usedlist);
	b->next = usedlist;
	if(usedlist)
		usedlist->prev = b;
	usedlist = b;
	if(assertions)
		assert(len + 1 == checklist(usedlist));
}

void
add_to_oldlist(Block *b)
{
	size_t len = 0;

	used(&len);
	if(assertions) {
		len = checklist(oldlist);
		dprintf(2, "len = %lu\n", len);
	}
	b->next = oldlist;
	if(oldlist)
		oldlist->prev = b;
	oldlist = b;
	if(assertions)
		assert(len + 1 == checklist(oldlist));
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
		dprintf(2, "coalesce: a = %p, a->size = %x, a+a->size = %lx, b = %p, b->size = %x\n", a,
				a->size, (size_t)((char *)a) + a->size, b, b->size);
	if(((char *)a) + a->size != (void *)b)
		return -3;

	a->next = b->next;
	a->size += b->size;
	if(a->next)
		a->next->prev = a;
	nfrees--;
	return 0;
}

void *
coalesce(Block *a, Block *b)
{
	if(coalesce1(a, b) == 0 && b != nil && gcverbose)
		dprintf(2, ">>> coalescing blocks %p and %p\n", a, b);
	return a;
}

Block *
coalesce_list(Block *list)
{
	Block *l;

	if(!list)
		return nil;

	l = list;
	while(l != nil) {
		l = coalesce(l, l->next);
		if(l->prev)
			coalesce(l->prev, l);
		l = l->next;
	}
	return list;
}

Block *
allocate2(size_t sz)
{
	Block *fl, *prev;
	size_t realsize;
	Block *new;
	char *p;
	size_t i;

	realsize = sizeof(Block) + sz;

	prev = nil;
	for(fl = freelist; fl != nil; prev = fl, fl = fl->next) {
		if(fl->size >= realsize) {
			if(fl->size == realsize || fl->size - realsize < minsize) {
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
			new = (void *)(((char *)fl) + fl->size);
			new->size = realsize;
			break;
		}
	}

	if(fl == nil)
		return nil;

	new->next = nil;
	new->prev = nil;
	new->age = 0;
	bytesfree -= realsize;
	p = (void *)(((char *)new) + sizeof(Block));
	if(assertions) {
		for(i = 0; i < realsize - sizeof(Block); i++)
			p[i] = 0;
	}
	return new;
}

void *
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

void
gc_getstats(GcStats *stats)
{
	stats->total_free = usage_stats(freelist);
	stats->real_free = real_usage_stats(freelist);
	stats->total_used = usage_stats(usedlist) + usage_stats(oldlist);
	stats->real_used = real_usage_stats(usedlist) + real_usage_stats(oldlist);
	stats->free_blocks = nblocks_stats(freelist);
	stats->used_blocks = nblocks_stats(usedlist);
	stats->old_blocks = nblocks_stats(oldlist);
	stats->nfrees = nfrees;
	stats->nallocs = nallocs;
	stats->allocations = allocations;
	stats->ngcs = ngcs;
	stats->sort_after_n = gc_sort_after_n;
	stats->nsortgc = nsortgc;
	stats->coalesce_after = gc_coalesce_after_n;
	stats->ncoalescegc = ncoalescegc;
	stats->gc_after = gc_after;
	stats->nregions = nregions;
	stats->nsort = nsort;
	stats->ncoalesce = ncoalesce;
	stats->blocksz = blocksize;
	stats->oldage = gc_oldage;
	stats->generational = generational;
	stats->gc_oldsweep_after = gc_oldsweep_after;
	stats->array_sort = use_array_sort;
	stats->use_size = use_list_sz;
	stats->gcblocked = gcblocked;
}

int
ismanaged(void *p)
{
	Region *r;

	for(r = regions; r != nil; r = r->next)
		if(r->start <= (size_t)p && (r->start + r->size) > (size_t)p)
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

	if(!ismanaged(p)) {
		if(gcverbose)
			dprintf(2, ">>> not marking unmanaged ptr %p\n", p);
		return;
	}

	h = header(p);

	if(h->tag == tNil) {
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

	for(; r != NULL; r = r->next) {
		if(ismanaged((void *)r)) {
			h = header((void *)r);
			gc_set_mark(h);
		}
		p = *(r->p);
		gcmark(p);
	}
}

typedef struct {
	Block *list;
	Block *olist;
	size_t frees;
} SweepData;

SweepData
sweeplist(Block *list, Block *oolist, Boolean gensort)
{
	Block *new_list = nil, *cur, *next;
	Block *new_list_head = nil;
	Block *olist = oolist;
	Header *h;
	size_t frees;
	SweepData res;

	if(assertions)
		checklist(list);
	cur = list;
	frees = 0;
	for(;;) {
		if(cur == nil)
			break;
		next = cur->next;
		if(next)
			next->prev = nil;
		h = (Header *)block_pointer(cur);
		if(h->flags & GcUsed) {
			cur->age++;
			if(cur->age > gc_oldage && generational && gensort) {
				cur->prev = nil;
				cur->next = nil;
				cur->next = olist;
				if(olist)
					olist->prev = cur;
				olist = cur;
			} else {
				if(gcverbose)
					dprintf(2, ">>> unmarking %p\n", h);
				gc_unset_mark(h);
				cur->prev = new_list;
				cur->next = nil;
				if(new_list_head == nil)
					new_list_head = cur;
				else
					new_list->next = cur;
				new_list = cur;
			}
		} else {
			if(gcverbose)
				dprintf(2, ">>> deallocating %p\n", cur);
			bytesused -= cur->size;
			cur->next = nil;
			cur->prev = nil;
			add_to_freelist(cur);
			frees++;
			nallocs--;
		}
		cur = next;
	}
	res.list = new_list_head;
	res.olist = olist;
	res.frees = frees;
	if(assertions)
		checklist(res.list);
	if(assertions)
		checklist(res.olist);
	return res;
}

size_t
gcsweep(void)
{
	SweepData d;

	d = sweeplist(usedlist, oldlist, TRUE);
	usedlist = d.list;
	oldlist = d.olist;

	return d.frees;
}

size_t
gcoldsweep(void)
{
	SweepData d;

	if(!generational)
		return 0;
	d = sweeplist(oldlist, nil, FALSE);
	assert(d.olist == nil);
	oldlist = d.list;
	return d.frees;
}

void
gc_print_stats(GcStats *stats)
{
	dprintf(2, "tfree = %lu, rfree = %lu\n", stats->total_free, stats->real_free);
	dprintf(2, "tused = %lu, rused = %lu\n", stats->total_used, stats->real_used);
	dprintf(2, "free blocks = %lu, used blocks = %lu\n", stats->free_blocks, stats->used_blocks);
	dprintf(2, "old blocks = %lu\n", stats->old_blocks);
	dprintf(2, "nfrees = %lu, nallocs = %lu\n", stats->nfrees, stats->nallocs);
	dprintf(2, "allocations since last gc = %lu\n", stats->allocations);
	dprintf(2, "number of gc = %lu\n", stats->ngcs);
	dprintf(2, "gc_sort_after_n = %d, nsortgc = %d\n", stats->sort_after_n, stats->nsortgc);
	dprintf(2, "gc_coalesce_after_n = %d, ncoalescegc = %d\n", stats->coalesce_after,
			stats->ncoalescegc);
	dprintf(2, "gc_after = %d\n", stats->gc_after);
	dprintf(2, "nregions = %lu\n", stats->nregions);
	dprintf(2, "nsort = %lu\n", stats->nsort);
	dprintf(2, "ncoalesce = %lu\n", stats->ncoalesce);
	dprintf(2, "blocksz = %lu\n", stats->blocksz);
}

void
ms_initgc(void)
{
	GcStats stats;
	Block *b;
	int i;

	if(gcverbose || gcinfo)
		dprintf(2, "Starting mark/sweep GC\n");
	if((gcverbose || gcinfo) && generational == TRUE)
		dprintf(2, "GC is generational: oldage = %u\n", gc_oldage);

	rangc = 0;
	for(i = 0; i < reserve_blocks; i++) {
		b = create_block(blocksize);
		add_to_freelist(b);
	}

	if(gcverbose || gcinfo) {
		gc_getstats(&stats);
		gc_print_stats(&stats);
	}
}

/*void
ms_gc(Boolean full, Boolean inalloc)
{
	GcStats starting, ending;

	* set things to their defaults if it's weird *
	if(gc_after < 1)
		gc_after = 100;
	if(gc_sort_after_n < 1)
		gc_sort_after_n = 5;
	if(gc_coalesce_after_n < 1)
		gc_coalesce_after_n = 5;

	if(allocations < (size_t)gc_after && !(full || inalloc))
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

	if(nsortgc >= gc_sort_after_n || full) {
		if(gcverbose)
			dprintf(2, ">> Sorting freelist\n");
		// list_rating(freelist);
		freelist = sort_list(freelist, nfrees);
		nsortgc = 0;
		nsort++;
	}
	if(ncoalescegc >= gc_coalesce_after_n || full) {
		if(gcverbose)
			dprintf(2, ">> Coalescing blocks\n");
		freelist = coalesce_list(freelist);
		ncoalescegc = 0;
		ncoalesce++;
	}
	if(noldsweep >= gc_oldsweep_after || full) {
		if(gcverbose)
			dprintf(2, ">> Sweeping geriatric blocks\n");
		gcoldsweep();
		noldsweep = 0;
	}

	ngcs++;
	nsortgc++;
	ncoalescegc++;
	noldsweep++;

	if(gcverbose || gcinfo) {
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
}*/

void
ms_gc(Boolean full, Boolean inalloc)
{
	if(allocations < (size_t)gc_after && !full)
		return;

	if(gcblocked > 0) {
		triedgcs++;
		return;
	}

	derefallobjects();
	gc_markrootlist(rootlist);
	gc_markrootlist(globalrootlist);
	gc_markrootlist(exceptionrootlist);

	gcsweep();
	dealloc_unrefed_objects();

	if(nsortgc > gc_sort_after_n) {
		freelist = sort_list(freelist, nfrees);
		nsortgc = 0;
	} else
		nsortgc++;

	if(ncoalescegc > gc_coalesce_after_n) {
		freelist = coalesce_list(freelist);
		ncoalescegc = 0;
	} else
		ncoalescegc++;

	if(noldsweep >= gc_oldsweep_after) {
		gcoldsweep();
		noldsweep = 0;
	} else
		noldsweep++;

	ngcs++;
	allocations = 0;
	rangc = 1;
	triedgcs = 0;
}

void *
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

	ms_gc(TRUE, TRUE);
	nb = allocate1(realsz);
	if(nb)
		goto done;

	ms_gc(TRUE, TRUE);
	nb = allocate1(realsz);
	if(nb)
		goto done;

	while(realsz + sizeof(Block) >= blocksize)
		blocksize *= 2;
	b = create_block(blocksize);
	add_to_freelist(b);
	nb = allocate1(realsz);

	assert(nb != nil);
done:
	nb->flags = 0;
	nb->tag = tag;
	nb->forward = nil;
	nb->size = sz;
	nb->refs = 1;
	allocations++;
	return (void *)(((char *)nb) + sizeof(Header));
}

void
gc_set_mark(Header *h)
{
	if(!ismanaged((void *)h))
		return;
	h->flags |= GcUsed;
}

void
gc_unset_mark(Header *h)
{
	if(!ismanaged((void *)h))
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

	for(l = usedlist; l != nil; l = l->next) {
		h = (Header *)block_pointer(l);
		p = (void *)(((char *)h) + sizeof(Header));
		t = gettag(h->tag);
		dump(t, p);
	}
}

void
ms_gcenable(void)
{
	assert(gcblocked > 0);
	gcblocked--;
	if(!gcblocked && triedgcs > 0)
		ms_gc(FALSE, FALSE);
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
