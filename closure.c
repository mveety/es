/* closure.c -- operations on bindings, closures, lambdas, and thunks ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "gc.h"

/*
 * Closure garbage collection support
 */

DefineTag(Closure, static);

extern Closure *
mkclosure(Tree *tree, Binding *binding)
{
	Closure *closure = NULL; Root r_closure;

	gcdisable();

	closure = gcnew(Closure);
	gcref(&r_closure, (void **)&closure);

	closure->tree = tree;
	closure->binding = binding;
	gcenable();

	gcderef(&r_closure, (void **)&closure);
	return closure;
}

static void *
ClosureCopy(void *op)
{
	void *np = gcnew(Closure);
	memcpy(np, op, sizeof(Closure));
	return np;
}

static size_t
ClosureScan(void *p)
{
	Closure *closure = p;
	closure->tree = forward(closure->tree);
	closure->binding = forward(closure->binding);
	return sizeof(Closure);
}

void
ClosureMark(void *p)
{
	Closure *c;

	c = (Closure *)p;
	gc_set_mark(header(p));
	gcmark(c->tree);
	gcmark(c->binding);
}

/* revtree -- destructively reverse a list stored in a tree */
static Tree *
revtree(Tree *tree)
{
	Tree *prev, *next;
	if(tree == NULL)
		return NULL;
	prev = NULL;
	do {
		assert(tree->kind == nList);
		next = tree->u[1].p;
		tree->u[1].p = prev;
		prev = tree;
	} while((tree = next) != NULL);
	return prev;
}

typedef struct Chain Chain;
struct Chain {
	Closure *closure;
	Chain *next;
};
static Chain *chain = NULL;

static Binding *
extract(Tree *tree0, Binding *bindings0)
{
	Tree *tree = nil; Root r_tree;
	Binding *bindings = nil; Root r_bindings;
	Tree *defn = nil; Root r_defn;
	List *list = nil; Root r_list;
	Tree *name = nil; Root r_name;
	Term *term = nil; Root r_term;
	Tree *word = nil; Root r_word;
	NodeKind k;
	char *prim = nil;

	gcref(&r_tree, (void **)&tree);
	gcref(&r_bindings, (void **)&bindings);
	gcref(&r_defn, (void **)&defn);
	gcref(&r_list, (void **)&list);
	gcref(&r_name, (void **)&name);
	gcref(&r_term, (void **)&term);
	gcref(&r_word, (void **)&word);

	assert(gcisblocked());
	tree = tree0;
	bindings = bindings0;

	for(; tree != NULL; tree = tree->u[1].p) {
		defn = tree->u[0].p;
		assert(tree->kind == nList);
		if(defn != NULL) {
			list = NULL;
			name = defn->u[0].p;
			assert(name->kind == nWord || name->kind == nQword);
			defn = revtree(defn->u[1].p);
			for(; defn != NULL; defn = defn->u[1].p) {
				term = NULL;
				word = defn->u[0].p;
				k = word->kind;
				prim = nil;
				assert(defn->kind == nList);
				assert(k == nWord || k == nQword || k == nPrim || k == nThunk || k == nDict);
				switch(k) {
				case nPrim:
					prim = word->u[0].s;
					if(streq(prim, "nestedbinding")) {
						int i, count = 0;
						Chain *cp;
						if((defn = defn->u[1].p) == NULL || defn->u[0].p->kind != nWord ||
						   (count = (atoi(defn->u[0].p->u[0].s))) < 0) {
							fail("$&parse", "improper use of $&nestedbinding");
							unreachable();
						}
						for(cp = chain, i = 0;; cp = cp->next, i++) {
							if(cp == NULL) {
								fail("$&parse", "bad count in $&nestedbinding: %d", count);
								unreachable();
							}
							if(i == count)
								break;
						}
						term = mkterm(NULL, cp->closure);
					} else {
						fail("$&parse", "bad unquoted primitive in %%closure: $&%s", prim);
						unreachable();
					}
					break;
				case nThunk:
					term = mkterm(nil, mkclosure(word, bindings));
					break;
				case nDict: // not great.
					gcenable();
					term = mkdictterm(parsedict(word, bindings));
					gcdisable();
					break;
				default:
					term = mkstr(word->u[0].s);
					break;
				}
				list = mklist(term, list);
			}
			bindings = mkbinding(name->u[0].s, list, bindings);
		}
	}

	gcrderef(&r_word);
	gcrderef(&r_term);
	gcrderef(&r_name);
	gcrderef(&r_list);
	gcrderef(&r_defn);
	gcrderef(&r_bindings);
	gcrderef(&r_tree);

	return bindings;
}

extern Closure *
extractbindings(Tree *tree0)
{
	Chain me;
	Tree *volatile tree = tree0;
	Binding *volatile bindings = NULL;
	Closure *result = NULL; Root r_result;

	gcdisable();

	if(tree->kind == nList && tree->u[1].p == NULL)
		tree = tree->u[0].p;

	me.closure = mkclosure(NULL, NULL);
	me.next = chain;
	chain = &me;

	ExceptionHandler {
		while(tree->kind == nClosure) {
			bindings = extract(tree->u[0].p, bindings);
			tree = tree->u[1].p;

			if(!tree)
				fail("$&parse", "%%closure missing body");

			if(tree->kind == nList && tree->u[1].p == NULL)
				tree = tree->u[0].p;
		}
	} CatchException (e) {
		chain = chain->next;
		throw(e);
	} EndExceptionHandler;

	chain = chain->next;

	result = me.closure;
	gcref(&r_result, (void **)&result);

	result->tree = tree;
	result->binding = bindings;

	gcenable();

	gcderef(&r_result, (void **)&result);
	return result;
}

/*
 * Binding garbage collection support
 */

DefineTag(Binding, static);

extern Binding *
mkbinding(char *name, List *defn, Binding *next)
{
	Binding *binding = NULL; Root r_binding;

	assert(next == NULL || next->name != NULL);
	validatevar(name);
	gcdisable();
	binding = gcnew(Binding);
	gcref(&r_binding, (void **)&binding);
	binding->name = name;
	binding->defn = defn;
	binding->next = next;
	gcenable();
	gcderef(&r_binding, (void **)&binding);
	return binding;
}

extern Binding *
reversebindings(Binding *binding)
{
	if(binding == NULL)
		return NULL;
	else {
		Binding *prev, *next;
		prev = NULL;
		do {
			next = binding->next;
			binding->next = prev;
			prev = binding;
		} while((binding = next) != NULL);
		return prev;
	}
}

static void *
BindingCopy(void *op)
{
	void *np = gcnew(Binding);
	memcpy(np, op, sizeof(Binding));
	return np;
}

static size_t
BindingScan(void *p)
{
	Binding *binding = p;
	binding->name = forward(binding->name);
	binding->defn = forward(binding->defn);
	binding->next = forward(binding->next);
	return sizeof(Binding);
}

void
BindingMark(void *p)
{
	Binding *b;

	b = (Binding *)p;
	gc_set_mark(header(p));
	gcmark(b->name);
	gcmark(b->defn);
	gcmark(b->next);
}
