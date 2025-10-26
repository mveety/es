#include "es.h"
#include "gc.h"
#include "input.h"

#define ARENASIZE (2 * 1024 * 1024)

Arena *
newarena(size_t size)
{
	Arena *newa = nil;

	newa = ealloc(sizeof(Arena));
	newa->ptr = ealloc(size);
	newa->cur = newa->ptr;
	newa->size = size;
	newa->remain = size;
	newa->next = nil;

	return newa;
}

Arena *
extend_arena(Arena *arena, size_t size)
{
	Arena *nexta = nil;
	Arena *cur = nil;

	nexta = newarena(size);
	if(arena == nil)
		return nexta;

	for(cur = arena; cur->next != nil; cur = cur->next)
		;
	;

	cur->next = nexta;

	return nexta;
}

Arena *
next_free_arena(Arena *arena, size_t nbytes)
{
	Arena *cur = nil;

	if(arena == nil)
		return nil;

	for(cur = arena; cur != nil; cur = cur->next)
		if(cur->remain > nbytes)
			return cur;

	return nil;
}

void *
arena_allocate(Arena *arena, size_t nbytes)
{
	Arena *cur = nil;
	size_t nextsize = 0;
	void *ptr = nil;

	if(nbytes >= arena->size)
		nextsize = arena->size + (2 * nbytes);
	else
		nextsize = arena->size;

	cur = next_free_arena(arena, nbytes);
	if(cur == nil)
		cur = extend_arena(arena, nextsize);

	ptr = cur->cur;
	cur->cur += nbytes;
	cur->remain -= nbytes;

	return ptr;
}

int
isinarena(Arena *arena, void *ptr)
{
	Arena *cur = nil;

	if(arena == nil)
		return 0;
	for(cur = arena; cur != nil; cur = cur->next)
		if(ptr >= cur->ptr && ptr < (cur->ptr + cur->size))
			return 1;

	return 0;
}

int
arena_destroy(Arena *arena)
{
	Arena *cur = nil;
	Arena *prev = nil;

	if(arena == nil)
		return 0;

	cur = arena;
	do {
		prev = cur;
		cur = cur->next;

		efree(prev->ptr);
		efree(prev);
	} while(cur != nil);

	return 0;
}

size_t
arena_size(Arena *arena)
{
	Arena *cur = nil;
	size_t nbytes = 0;

	if(arena == nil)
		return 0;

	for(cur = arena; cur != nil; cur = cur->next)
		nbytes += cur->size;

	return nbytes;
}

size_t
arena_used(Arena *arena)
{
	Arena *cur = nil;
	size_t nbytes = 0;

	if(arena == nil)
		return 0;

	for(cur = arena; cur != nil; cur = cur->next)
		nbytes += cur->size - cur->remain;

	return nbytes;
}

void *
aalloc(size_t sz, int tag)
{
	Header *nb;
	size_t realsz;

	gettag(tag);

	assert(sz > 0);

	if(input->arena == nil)
		input->arena = newarena(ARENASIZE);

	realsz = ALIGN(sz + sizeof(Header));
	nb = arena_allocate(input->arena, realsz);

	nb->flags = 0;
	nb->tag = tag;
	nb->forward = nil;
	nb->size = sz;
	nb->refs = 1;

	return (void *)(((char *)nb) + sizeof(Header));
}

char *
andup(const char *str, size_t n)
{
	char *res;

	res = aalloc((n + 1) * sizeof(char), tString);
	memcpy(res, str, n);
	res[n] = 0;
	assert(strlen(res) == n);

	return res;
}

char *
adup(const char *str)
{
	return andup(str, strlen(str));
}

void *
aseal(void *ptr)
{
	void *nptr = nil;
	Tag *tag = nil;
	size_t maxused = 0;

	if(ptr == nil)
		return nil;

	assert(input->arena);

	maxused = arena_used(input->arena);
	gcreserve(maxused);

	nptr = forward(ptr);
	assert(nptr);
	tag = gettag(header(nptr)->tag);
	assert(tag);
	assert(tag->magic == TAGMAGIC);
	tag->scan(nptr);

	arena_destroy(input->arena);
	input->arena = nil;

	return nptr;
}

char *
asealbuffer(Buffer *buf)
{
	char *str = nil;

	str = adup(buf->str);
	efree(buf);
	return str;
}

char *
asealcountedbuffer(Buffer *buf)
{
	char *str = nil;

	str = andup(buf->str, buf->current);
	efree(buf);
	return str;
}
