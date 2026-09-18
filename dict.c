/* dict.c -- hash-table based dictionaries ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"
#include "stdenv.h"

#define INIT_DICT_SIZE 8
#define REMAIN(n) (((n) * 2) / 3)
#define GROW(n) ((n) * 2)

/*
 * hashing
 */

#define FNV1A_HashStart 0x811c9dc5
#define FNV1A_HashIncr 0x01000193

HashFunction hashfunction = HaahrHash;
DictHash nilhash = {0, 0, 0};
DictStats dictstats = (DictStats){
	.maxsize = 0,
	.totalsize = 0,
	.nputs = 0,
	.nlookups = 0,
	.avgcompares = 0,
	.totalcompares = 0,
	.bloomfail = 0,
	.hashfail = 0,
	.strcmpfail = 0,
};

uint32_t
fnv1a_strhash2(const char *s1, const char *s2)
{
	size_t i;
	uint32_t hash = FNV1A_HashStart;

	if(s1 != nil)
		for(i = 0; s1[i] != '\0'; i++) {
			hash ^= (uint8_t)s1[i];
			hash *= FNV1A_HashIncr;
		}

	if(s2 != nil)
		for(i = 0; s2[i] != '\0'; i++) {
			hash ^= (uint8_t)s2[i];
			hash *= FNV1A_HashIncr;
		}

	return hash;
}

/* strhash2 -- the (probably too slow) haahr hash function */
static unsigned long
haahr_strhash2(const char *str1, const char *str2)
{

#define ADVANCE()                          \
	{                                      \
		if((c = *s++) == '\0') {           \
			if(str2 == NULL)               \
				break;                     \
			else {                         \
				s = (unsigned char *)str2; \
				str2 = NULL;               \
				if((c = *s++) == '\0')     \
					break;                 \
			}                              \
		}                                  \
	}

	int c;
	unsigned long n = 0;
	unsigned char *s = (unsigned char *)str1;
	assert(str1 != NULL);
	while(1) {
		ADVANCE();
		n += (c << 17) ^ (c << 11) ^ (c << 5) ^ (c >> 1);
		ADVANCE();
		n ^= (c << 14) + (c << 7) + (c << 4) + c;
		ADVANCE();
		n ^= (~c << 11) | ((c << 3) ^ (c >> 1));
		ADVANCE();
		n -= (c << 16) | (c << 9) | (c << 2) | (c & 3);
	}
	return n;
}

uint32_t
jenkins_oat_strhash2(const char *s1, const char *s2)
{
	size_t i = 0;
	uint32_t res = 0;

	if(s1 != nil)
		for(i = 0; s1[i] != '\0'; i++) {
			res += s1[i];
			res += (res << 10);
			res ^= (res >> 6);
		}

	if(s2 != nil)
		for(i = 0; s2[i] != '\0'; i++) {
			res += s2[i];
			res += (res << 10);
			res ^= (res >> 6);
		}

	return res;
}

/* interface */

DictHash*
strhash2(DictHash *dh, const char *str1, const char *str2)
{
	dh->haahr = haahr_strhash2(str1, str2);
	dh->fnv1a = fnv1a_strhash2(str1, str2);
	dh->jenkins = jenkins_oat_strhash2(str1, str2);

	return dh;
}

DictHash*
strhash(DictHash *dh, const char *str)
{
	return strhash2(dh, str, nil);
}

/*
 * data structures and garbage collection
 */

Boolean
hash_compare(DictHash *dh1, DictHash *dh2)
{
	if(dh1->jenkins != dh2->jenkins)
		goto fail;
	if(dh1->fnv1a != dh2->fnv1a)
		goto fail;
	if(dh1->haahr != dh2->haahr)
		goto fail;
	return TRUE;
fail:
	dictstats.hashfail++;
	return FALSE;
}

static uint64_t
gethashi(DictHash *dh)
{
	switch(hashfunction){
	default:
		unreachable();
		return 0;
	case HaahrHash:
		return dh->haahr;
	case FNV1AHash:
		return dh->fnv1a;
	case JenkinsOATHash:
		return dh->jenkins;
	}
}

DefineTag(Dict, static);

static inline size_t
bloomsize(size_t size)
{
	size_t res;

	if(size % 8 == 0)
		res = (size*2) / 8;
	else
		res = ((size*2) / 8) + 1;

	return res;
}

static inline DictHash *
bloominsert(DictHash *dh, Dict *dict, char *name)
{
	size_t fnv1a_bit = 0;
	size_t haahr_bit = 0;
	size_t jenkins_bit = 0;
	size_t bloomsz = bloomsize(dict->size);

	dh = strhash(dh, name);
	fnv1a_bit = dh->fnv1a % bloomsz;
	haahr_bit = dh->haahr % bloomsz;
	jenkins_bit = dh->jenkins % bloomsz;

	dict->bloom[fnv1a_bit / 8] |= 1 << (fnv1a_bit % 8);
	dict->bloom[haahr_bit / 8] |= 1 << (haahr_bit % 8);
	dict->bloom[jenkins_bit / 8] |= 1 << (jenkins_bit % 8);

	return dh;
}

typedef struct BloomResult {
	Boolean exists;
	DictHash hash;
} BloomResult;

static inline BloomResult*
bloomcheck2(BloomResult *br, Dict *dict, const char *name1, const char *name2)
{
	size_t fnv1a_bit = 0;
	size_t haahr_bit = 0;
	size_t jenkins_bit = 0;
	size_t bloomsz = bloomsize(dict->size);
	DictHash *dh = nil;

	*br = (BloomResult){FALSE, nilhash};
	dh = strhash2(&br->hash, name1, name2);

	fnv1a_bit = dh->fnv1a % bloomsz;
	haahr_bit = dh->haahr % bloomsz;
	jenkins_bit = dh->jenkins % bloomsz;


	if(((dict->bloom[fnv1a_bit / 8] & (1 << (fnv1a_bit % 8))) != 0) &&
	   ((dict->bloom[haahr_bit / 8] & (1 << (haahr_bit % 8))) != 0) &&
	   ((dict->bloom[jenkins_bit / 8] & (1 << (jenkins_bit % 8))) != 0))
		br->exists = TRUE;

	return br;
}

/*static inline BloomResult*
bloomcheck(BloomResult *br, Dict *dict, const char *name)
{
	return bloomcheck2(br, dict, name, nil);
}*/

static Dict *
mkdict0(size_t size)
{
	size_t len = offsetof(Dict, table[size]);
	Dict *dict = nil; Root r_dict;

	gcref(&r_dict, (void **)&dict);
	gcdisable();
	dict = gcalloc(len, tDict);
	memzero(dict, len);
	dict->readonly = 0;
	dict->size = size;
	dict->remain = REMAIN(size);
	dict->bloom = gcmalloc(bloomsize(size));
	if((uint64_t)dict->size > dictstats.maxsize)
		dictstats.maxsize = dict->size;
	dictstats.totalsize += dict->size;
	dictstats.ndicts++;
	gcenable();
	gcrderef(&r_dict);
	return dict;
}

static void *
DictCopy(void *op)
{
	Dict *dict = op;
	size_t len = offsetof(Dict, table[dict->size]);
	void *np = gcalloc(len, tDict);
	memcpy(np, op, len);
	return np;
}

static size_t
DictScan(void *p)
{
	Dict *dict = p;
	int i;
	dict->bloom = forward(dict->bloom);
	for(i = 0; i < dict->size; i++) {
		Assoc *ap = &dict->table[i];
		ap->name = forward(ap->name);
		ap->value = forward(ap->value);
	}
	return offsetof(Dict, table[dict->size]);
}

void
DictMark(void *p)
{
	Dict *d;
	Assoc *a;
	int i;

	d = (Dict *)p;
	gc_set_mark(header(p));

	gcmark(d->bloom);
	for(i = 0; i < d->size; i++) {
		a = &d->table[i];
		gcmark(a->name);
		gcmark(a->value);
	}
}

/*
 * private operations
 */

char *DEAD = "%%DEAD%%";

static inline void
update_get_stats(uint64_t compares, uint64_t failed)
{
	dictstats.avgcompares = ((dictstats.avgcompares*dictstats.nlookups)+compares)/(dictstats.nlookups+1);
	dictstats.totalcompares += compares;
	dictstats.nlookups++;
	dictstats.failed_lookups += failed;
}

static Boolean
dictstreq2(const char *s, const char *t1, const char *t2)
{
	if(streq2(s, t1, t2))
		return TRUE;
	dictstats.strcmpfail++;
	return FALSE;
}

static Assoc *
get2(Dict *dict, const char *name1, const char *name2)
{
	Assoc *ap;
	uint64_t hash = 0;
	uint64_t mask = dict->size - 1;
	BloomResult bloomres = {FALSE, nilhash};
	DictHash *dh = nil;
	BloomResult *br = nil;
	uint64_t compares = 1;
	uint64_t failed = 0;

	ref(dict);
	dh = &bloomres.hash;
	br = bloomcheck2(&bloomres, dict, name1, name2);
	if(br->exists == FALSE){
		dictstats.bloomfail++;
		goto fail;
	}
	hash = gethashi(dh);

	for(; (ap = &dict->table[hash & mask])->name != NULL; hash++, compares++)
		if(ap->name != DEAD && hash_compare(dh, &ap->hash) && dictstreq2(ap->name, name1, name2)) {
			update_get_stats(compares, 0);
			deref(dict);
			return ap;
		}
fail:
	update_get_stats(br->exists == TRUE ? compares : 0, 1);
	deref(dict);
	return nil;
}

static void putwrapper(void *, char *, void *); // shut up clang

static Dict *
put(Dict *dict, char *name, void *value)
{
	uint64_t n, mask;
	Assoc *ap;
	Dict *old = nil;
	char *np = nil;
	void *vp = nil;
	Dict *new = nil;
	DictHash dicthash;

	assert(get2(dict, name, nil) == nil);
	assert(value != nil);
	assert(!dict->readonly);

	if(dict->remain <= 1) {
		ref(old);
		ref(np);
		ref(vp);
		ref(new);
		old = dict;
		np = name;
		vp = value;

		dictstats.totalsize -= old->size;
		dictstats.ndicts--;
		new = mkdict0(GROW(old->size));
		dictforall(old, &putwrapper, new);
		dict = new;
		name = np;
		value = vp;

		deref(new);
		deref(vp);
		deref(np);
		deref(old);
	}

	n = gethashi(bloominsert(&dicthash, dict, name));
	mask = dict->size - 1;
	for(; (ap = &dict->table[n & mask])->name != DEAD; n++)
		if(ap->name == nil) {
			--dict->remain;
			break;
		}

	ap->name = name;
	ap->hash = dicthash;
	ap->value = value;
	dictstats.nputs++;
	return dict;
}

static void
putwrapper(void *a, char *b, void *c)
{
	put(a, b, c);
}

static void
rm(Dict *dict, Assoc *ap)
{
	unsigned long n, mask;
	assert(dict->table <= ap && ap < &dict->table[dict->size]);

	ap->name = DEAD;
	ap->value = NULL;
	n = ap - dict->table;
	mask = dict->size - 1;
	for(n++; (ap = &dict->table[n & mask])->name == DEAD; n++)
		;
	if(ap->name != NULL)
		return;
	for(n--; (ap = &dict->table[n & mask])->name == DEAD; n--) {
		ap->name = NULL;
		++dict->remain;
	}
}

/*
 * exported functions
 */

extern Dict *
mkdict(void)
{
	return mkdict0(INIT_DICT_SIZE);
}

extern void *
dictget(Dict *dict, const char *name)
{
	Assoc *ap = get2(dict, name, nil);
	if(ap == NULL)
		return NULL;
	return ap->value;
}

extern Dict *
dictput(Dict *dict, char *name, void *value)
{
	Assoc *ap = nil;

	assert(!dict->readonly);
	ap = get2(dict, name, nil);
	if(value != NULL)
		if(ap == NULL)
			dict = put(dict, name, value);
		else
			ap->value = value;
	else if(ap != NULL)
		rm(dict, ap);
	return dict;
}

extern void
dictforall(Dict *dp, void (*proc)(void *, char *, void *), void *arg)
{
	int i;
	Dict *dict = nil;
	void *argp = nil;

	dict = dp;
	ref(dict);
	argp = arg;
	ref(argp);
	for(i = 0; i < dict->size; i++) {
		Assoc *ap = &dict->table[i];
		if(ap->name != NULL && ap->name != DEAD)
			(*proc)(argp, ap->name, ap->value);
	}
	deref(argp);
	deref(dict);
}

/* dictget2 -- look up the catenation of two names (such a hack!) */
extern void *
dictget2(Dict *dict, const char *name1, const char *name2)
{
	Assoc *ap = get2(dict, name1, name2);
	if(ap == NULL)
		return NULL;
	return ap->value;
}

Dict *
dictcopy(Dict *oda)
{
	int i;
	Dict *odict = nil;
	Dict *dict = nil;

	ref(odict);
	ref(dict);

	odict = oda;
	dict = mkdict();

	for(i = 0; i < odict->size; i++) {
		if(odict->table[i].name == NULL || odict->table[i].name == DEAD)
			continue;
		dict = dictput(dict, odict->table[i].name, (void *)odict->table[i].value);
	}

	deref(dict);
	deref(odict);

	return dict;
}

Dict *
dictappend(Dict *desta, Dict *srca, Boolean overwrite)
{
	int i;
	Dict *dest = nil;
	Dict *src = nil;

	ref(dest);
	ref(src);

	dest = desta;
	src = srca;

	for(i = 0; i < src->size; i++) {
		if(src->table[i].name == NULL || src->table[i].name == DEAD)
			continue;
		if(dictget(dest, src->table[i].name))
			if(!overwrite)
				continue;
		dest = dictput(dest, src->table[i].name, src->table[i].value);
	}

	deref(src);
	deref(dest);

	return dest;
}

Dict *
parsedict(Tree *tree0, Binding *binding0, int flags)
{
	Tree *tree = nil;
	Binding *binding = nil;
	Tree *inner = nil;
	Dict *dict = nil;
	Tree *assoc = nil;
	List *name = nil;
	List *value = nil;
	char *namestr = nil;

	ref(tree);
	ref(binding);
	ref(inner);
	ref(dict);
	ref(assoc);
	ref(name);
	ref(value);
	ref(namestr);

	tree = tree0;
	binding = binding0;
	dict = mkdict();
	for(inner = tree->u[0].p; inner != nil; inner = inner->u[1].p) {
		assoc = inner->u[0].p;
		assert(assoc->kind = nAssoc);
		name = glom1(assoc->u[0].p, binding, flags);
		value = glom(assoc->u[1].p, binding, flags, TRUE);
		if(name == nil)
			continue;
		namestr = getstr(name->term);
		dict = dictput(dict, namestr, value);
	}

	deref(namestr);
	deref(value);
	deref(name);
	deref(assoc);
	deref(dict);
	deref(inner);
	deref(binding);
	deref(tree);

	return dict;
}
