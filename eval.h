typedef struct Stackframe Stackframe;

struct Stackframe {
	Stackframe *prev;
	/* gc roots */
	Root list_root;
	Root binding_root;
	Root funcname_root;
};

extern noreturn failexec(char *, List *);
extern List *assign(Tree *, Tree *, Binding *);
extern Binding *letbindings1(Tree *, Binding *, Binding *, int, int);
extern Binding *letbindings(Tree *, Binding *, Binding *, int);
extern Binding *letsbindings(Tree *, Binding *, Binding *, int);
extern List *localbindings(Binding *, Binding *, Tree *, int);
extern List *local(Tree *, Tree *, Binding *, int);
extern List *forloop(Tree *, Tree *, Binding *, int);
extern List *matchpattern(Tree *, Tree *, Binding *);
extern List *extractpattern(Tree *, Tree *, Binding *);
/* extern List *walk(Tree*, Binding*, int); */
extern Binding *bindargs(Tree *, List *, Binding *);
extern List *pathsearch(Term *);
extern List *old_eval(List *, Binding *, int flags) extern List *new_eval(List *, Binding *, int);
