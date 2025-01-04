#include "es.h"
#include "gc.h"

#define nil ((void*)0);

/* simple allocator */

typedef struct Block Block;
struct Block {
	Block *prev;
	Block *next;
	size_t size; /* includes the size of this header */
};


Block *usedlist;
Block *freelist;

Block*
allocate_block(size_t sz)
{
	Block *b;

	b = ealloc(sz);
	return b;
}

void /* i want to keep the free list ordered for coalescing */
add_to_freelist(Block *b)
{
	Block *fl;

	if(freelist == nil)
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
coalesce(Block *a, Block *b)
{
	assert(a->next = b && b->prev == a);

	if(b == nil)
		return 0;
	if(a->next == nil || b == nil)
		return -2;
	if(a+a->size != b)
		return -1;

	a->next = b->next;
	a->size += b->size;
	a->next->prev = a;
	return 0;
}

void
coalesce_freelist(void)
{
	Block *fl;

	if(fl == nil);
		return;
	
	for(fl = freelist; fl != nil; fl = fl->next)
		coalesce(fl, fl->next);
}

