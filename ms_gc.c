#include "es.h"
#include "gc.h"

#define nil ((void*)0)

/* simple allocator */

typedef struct Block Block;
struct Block {
	Block *prev;
	Block *next;
	size_t size; /* includes the size of this header */
};

Block *usedlist;
Block *freelist;

/* this is the smallest sensible size of a block (probably) */
const size_t minsize = sizeof(Block)+sizeof(Header)+sizeof(void*);

Block*
create_block(size_t sz)
{
	Block *b;

	b = ealloc(sz);
	return b;
}

void /* i want to keep the free list ordered for coalescing */
add_to_freelist(Block *b)
{
	Block *fl;

	if(!freelist)
		freelist = b;

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

	if(!freelist);
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
		cb = b + realsize;
		cb->size = b->size + realsize;
		cb->next = b->next;
		cb->prev = b;
		b->next = cb;
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
deallocate1(void *p, Block **list)
{
	Block *b, *prev, *next;

	b = pointer_block(p);
	next = b->next;
	prev = b->prev;

	if(b == *list){
		*list = (*list)->next;
		if(*list)
			(*list)->prev = nil;
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

