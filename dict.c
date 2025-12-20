/* dict.c -- hash-table based dictionaries ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"

#define INIT_DICT_SIZE 8
#define REMAIN(n) (((n) * 2) / 3)
#define GROW(n) ((n) * 2)

/*
 * hashing
 */

#define FNV1A_HashStart 0x811c9dc5
#define FNV1A_HashIncr 0x01000193

HashFunction hashfunction = HaahrHash;

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

uint64_t
strhash2(const char *str1, const char *str2)
{
	switch(hashfunction) {
	default:
		unreachable();
		break;
	case HaahrHash:
		return haahr_strhash2(str1, str2);
	case FNV1AHash:
		return fnv1a_strhash2(str1, str2);
	case JenkinsOATHash:
		return jenkins_oat_strhash2(str1, str2);
	}
	unreachable();
	return 0;
}

uint64_t
strhash(const char *str)
{
	switch(hashfunction) {
	default:
		unreachable();
		break;
	case HaahrHash:
		return haahr_strhash2(str, nil);
	case FNV1AHash:
		return fnv1a_strhash2(str, nil);
	case JenkinsOATHash:
		return jenkins_oat_strhash2(str, nil);
	}
	unreachable();
	return 0;
}

/*
 * data structures and garbage collection
 */

DefineTag(Dict, static);

static inline size_t
bloomsize(size_t size)
{
	size_t res;

	if(size % 8 == 0)
		res = size / 8;
	else
		res = (size / 8) + 1;

	return res;
	if(res <= 2)
		return 2;
	return res / 2;
}

static inline uint64_t
bloominsert(Dict *dict, char *name)
{
	uint64_t fnv1a_hash = 0;
	size_t fnv1a_bit = 0;
	uint64_t haahr_hash = 0;
	size_t haahr_bit = 0;
	uint64_t jenkins_hash = 0;
	size_t jenkins_bit = 0;
	size_t bloomsz = bloomsize(dict->size);

	fnv1a_hash = fnv1a_strhash2(name, nil);
	fnv1a_bit = fnv1a_hash % bloomsz;
	haahr_hash = haahr_strhash2(name, nil);
	haahr_bit = haahr_hash % bloomsz;
	jenkins_hash = jenkins_oat_strhash2(name, nil);
	jenkins_bit = jenkins_hash % bloomsz;

	dict->bloom[fnv1a_bit / 8] |= 1 << (fnv1a_bit % 8);
	dict->bloom[haahr_bit / 8] |= 1 << (haahr_bit % 8);
	dict->bloom[jenkins_bit / 8] |= 1 << (jenkins_bit % 8);

	switch(hashfunction) {
	default:
		unreachable();
		break;
	case HaahrHash:
		return haahr_hash;
	case FNV1AHash:
		return fnv1a_hash;
	case JenkinsOATHash:
		return jenkins_hash;
	}
}

typedef struct BloomResult {
	Boolean exists;
	uint64_t hash;
} BloomResult;

static inline BloomResult
bloomcheck2(Dict *dict, const char *name1, const char *name2)
{
	BloomResult res = {FALSE, 0};
	uint64_t fnv1a_hash = 0;
	size_t fnv1a_bit = 0;
	uint64_t haahr_hash = 0;
	size_t haahr_bit = 0;
	uint64_t jenkins_hash = 0;
	size_t jenkins_bit = 0;
	size_t bloomsz = bloomsize(dict->size);

	fnv1a_hash = fnv1a_strhash2(name1, name2);
	fnv1a_bit = fnv1a_hash % bloomsz;
	haahr_hash = haahr_strhash2(name1, name2);
	haahr_bit = haahr_hash % bloomsz;
	jenkins_hash = jenkins_oat_strhash2(name1, name2);
	jenkins_bit = jenkins_hash % bloomsz;

	switch(hashfunction) {
	default:
		unreachable();
		break;
	case HaahrHash:
		res.hash = haahr_hash;
		break;
	case FNV1AHash:
		res.hash = fnv1a_hash;
		break;
	case JenkinsOATHash:
		res.hash = jenkins_hash;
		break;
	}

	if(((dict->bloom[fnv1a_bit / 8] & (1 << (fnv1a_bit % 8))) != 0) &&
	   ((dict->bloom[haahr_bit / 8] & (1 << (haahr_bit % 8))) != 0) &&
	   ((dict->bloom[jenkins_bit / 8] & (1 << (jenkins_bit % 8))) != 0))
		res.exists = TRUE;

	return res;
}

static inline BloomResult
bloomcheck(Dict *dict, const char *name)
{
	return bloomcheck2(dict, name, nil);
}

static Dict *
mkdict0(size_t size)
{
	size_t len = offsetof(Dict, table[size]);
	Dict *dict = gcalloc(len, tDict);
	memzero(dict, len);
	dict->readonly = 0;
	dict->size = size;
	dict->remain = REMAIN(size);
	dict->bloom = gcmalloc(bloomsize(size));
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

static Assoc *
get(Dict *dict, const char *name)
{
	Assoc *ap;
	uint64_t hash = 0;
	uint64_t mask = dict->size - 1;
	BloomResult bloomres = {FALSE, 0};
	Root r_dict;

	gcref(&r_dict, (void **)&dict);
	if(dict->size > 100) {
		bloomres = bloomcheck(dict, name);
		if(bloomres.exists == FALSE)
			goto fail;
		hash = bloomres.hash;
	} else
		hash = strhash(name);

	for(; (ap = &dict->table[hash & mask])->name != NULL; hash++)
		if(ap->name != DEAD && streq(name, ap->name)) {
			gcrderef(&r_dict);
			return ap;
		}
fail:
	gcrderef(&r_dict);
	return nil;
}

static void putwrapper(void *, char *, void *); // shut up clang

static Dict *
put(Dict *dict, char *name, void *value)
{
	uint64_t n, mask;
	Assoc *ap;
	Dict *old = NULL; Root r_old;
	char *np = NULL; Root r_np;
	void *vp = NULL; Root r_vp;
	Dict *new;

	assert(get(dict, name) == NULL);
	assert(value != NULL);
	assert(!dict->readonly);

	if(dict->remain <= 1) {
		old = dict;
		gcref(&r_old, (void **)&old);
		np = name;
		gcref(&r_np, (void **)&np);
		vp = value;
		gcref(&r_vp, (void **)&vp);

		new = mkdict0(GROW(old->size));
		dictforall(old, &putwrapper, new);
		dict = new;
		name = np;
		value = vp;

		gcderef(&r_vp, (void **)&vp);
		gcderef(&r_np, (void **)&np);
		gcderef(&r_old, (void **)&old);
	}

	n = bloominsert(dict, name);
	mask = dict->size - 1;
	for(; (ap = &dict->table[n & mask])->name != DEAD; n++)
		if(ap->name == NULL) {
			--dict->remain;
			break;
		}

	ap->name = name;
	ap->value = value;
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
	Assoc *ap = get(dict, name);
	if(ap == NULL)
		return NULL;
	return ap->value;
}

extern Dict *
dictput(Dict *dict, char *name, void *value)
{
	Assoc *ap = nil;

	assert(!dict->readonly);
	ap = get(dict, name);
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
	Dict *dict = NULL; Root r_dict;
	void *argp = NULL; Root r_argp;

	dict = dp;
	gcref(&r_dict, (void **)&dict);
	argp = arg;
	gcref(&r_argp, (void **)&argp);
	for(i = 0; i < dict->size; i++) {
		Assoc *ap = &dict->table[i];
		if(ap->name != NULL && ap->name != DEAD)
			(*proc)(argp, ap->name, ap->value);
	}
	gcderef(&r_argp, (void **)&argp);
	gcderef(&r_dict, (void **)&dict);
}

/* dictget2 -- look up the catenation of two names (such a hack!) */
extern void *
dictget2(Dict *dict, const char *name1, const char *name2)
{
	Assoc *ap;
	uint64_t hash = 0;
	uint64_t mask = dict->size - 1;
	BloomResult bloomres = {FALSE, 0};

	if(dict->size > 100) {
		bloomres = bloomcheck2(dict, name1, name2);
		if(bloomres.exists == FALSE)
			return nil;
		hash = bloomres.hash;
	} else
		hash = strhash2(name1, name2);

	for(; (ap = &dict->table[hash & mask])->name != NULL; hash++)
		if(ap->name != DEAD && streq2(ap->name, name1, name2))
			return ap->value;
	return nil;
}

Dict *
dictcopy(Dict *oda)
{
	int i;
	Dict *odict = NULL; Root r_odict;
	Dict *dict = NULL; Root r_dict;

	gcref(&r_odict, (void **)&odict);
	gcref(&r_dict, (void **)&dict);

	odict = oda;
	dict = mkdict();

	for(i = 0; i < odict->size; i++) {
		if(odict->table[i].name == NULL || odict->table[i].name == DEAD)
			continue;
		dict = dictput(dict, odict->table[i].name, (void *)odict->table[i].value);
	}

	gcrderef(&r_dict);
	gcrderef(&r_odict);

	return dict;
}

Dict *
dictappend(Dict *desta, Dict *srca, Boolean overwrite)
{
	int i;
	Dict *dest = NULL; Root r_dest;
	Dict *src = NULL; Root r_src;

	gcref(&r_dest, (void **)&dest);
	gcref(&r_src, (void **)&src);

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

	gcrderef(&r_src);
	gcrderef(&r_dest);

	return dest;
}

Dict *
parsedict(Tree *tree0, Binding *binding0, int flags)
{
	Tree *tree = nil; Root r_tree;
	Binding *binding = nil; Root r_binding;
	Tree *inner = nil; Root r_inner;
	Dict *dict = nil; Root r_dict;
	Tree *assoc = nil; Root r_assoc;
	List *name = nil; Root r_name;
	List *value = nil; Root r_value;
	char *namestr = nil; Root r_namestr;

	gcref(&r_tree, (void **)&tree);
	gcref(&r_binding, (void **)&binding);
	gcref(&r_inner, (void **)&inner);
	gcref(&r_dict, (void **)&dict);
	gcref(&r_assoc, (void **)&assoc);
	gcref(&r_name, (void **)&name);
	gcref(&r_value, (void **)&value);
	gcref(&r_namestr, (void **)&namestr);

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

	gcrderef(&r_namestr);
	gcrderef(&r_value);
	gcrderef(&r_name);
	gcrderef(&r_assoc);
	gcrderef(&r_dict);
	gcrderef(&r_inner);
	gcrderef(&r_binding);
	gcrderef(&r_tree);

	return dict;
}
