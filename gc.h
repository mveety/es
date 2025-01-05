/* gc.h -- garbage collector interface for es ($Revision: 1.1.1.1 $) */

/* see also es.h for more generally applicable definitions */

#define	ALIGN(n)	(((n) + sizeof (void *) - 1) &~ (sizeof (void *) - 1))
#define	MIN_minspace	1024*1024

extern Root *globalrootlist;
extern Root *exceptionrootlist;


/*
 * tags
 */
/* typedef for Tag is in es.h */
struct Tag {
	void *(*copy)(void *);
	size_t (*scan)(void *);
	void (*mark)(void *);
	long magic;
	char *typename;
};

enum {
	GcForward = 1<<0,
	GcUsed = 1<<1,
};

typedef struct Header Header;
struct Header {
	unsigned int flags;
	Tag *tag;
	void *forward;
};

typedef struct GcStats GcStats;
struct GcStats {
	size_t total_free;
	size_t real_free;
	size_t total_used;
	size_t real_used;
	size_t free_blocks;
	size_t used_blocks;
};

extern Tag StringTag;

enum {TAGMAGIC = 0xDefaced};
#define	DefineTag(t, storage) \
	static void *CONCAT(t,Copy)(void *); \
	static size_t CONCAT(t,Scan)(void *); \
	static void CONCAT(t, Mark)(void *); \
	storage Tag CONCAT(t,Tag) = { CONCAT(t,Copy), CONCAT(t,Scan), CONCAT(t, Mark), TAGMAGIC, STRING(t) }

extern Header *header(void *p);

/* old collecter */
extern void old_gcenable(void);
extern void old_gcdisable(void);
extern void old_gcreserve(size_t minfree);
extern Boolean old_gcisblocked(void);
extern void old_gc(void);

/* mark sweep */
extern void gcmark(void *p);
extern void gc_set_mark(Header *h);
extern void gc_unset_mark(Header *h);
extern void gc_markrootlist(Root *r);
extern void gc_getstats(GcStats *stats);
extern void gc_print_stats(GcStats *stats);
extern void ms_gc(void);
extern void *gcallocate(size_t sz, Tag *t);
/*
 * allocation
 */

extern void *gcalloc(size_t, Tag *);

typedef struct Buffer Buffer;
struct Buffer {
	size_t len;
	size_t current;
	char str[1];
};

extern Buffer *openbuffer(size_t minsize);
extern Buffer *expandbuffer(Buffer *buf, size_t minsize);
extern Buffer *bufncat(Buffer *buf, const char *s, size_t len);
extern Buffer *bufcat(Buffer *buf, const char *s);
extern Buffer *bufputc(Buffer *buf, char c);
extern char *sealbuffer(Buffer *buf);
extern char *sealcountedbuffer(Buffer *buf);
extern void freebuffer(Buffer *buf);

extern void *forward(void *p);
