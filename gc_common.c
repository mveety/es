#define	GARBAGE_COLLECTOR	1	/* for es.h */

#include "es.h"
#include "gc.h"


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
gcrderef(Root *r)
{
	gcderef(r, r->p);
}

void
gc_mark(void *p)
{
	Header *h;

	h = header(p);
	assert(h->tag->magic = TAGMAGIC);
	h->flags |= GcUsed;
}

void
gc_unmark(void *p)
{
	Header *h;

	h = header(h);
	assert(h->tag->magic = TAGMAGIC);
	h->flags &= ~GcUsed;
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


