/* var.c -- es variables ($Revision: 1.1.1.1 $) */

#include <es.h>
#include <gc.h>
#include <stdenv.h>

#if PROTECT_ENV
#define ENV_FORMAT "%F=%W"
#define ENV_DECODE "%N"
#else
#define ENV_FORMAT "%s=%W"
#define ENV_DECODE "%s"
#endif

Dict *vars;
static Dict *noexport;
static Vector *env, *sortenv;
static int envmin;
static Boolean isdirty = TRUE;
static Boolean rebound = TRUE;

DefineTag(Var, static);

static Boolean
specialvar(const char *name)
{
	return (*name == '*' || *name == '0') && name[1] == '\0';
}

static Boolean
hasbindings(List *list)
{
	for(; list != NULL; list = list->next)
		if(isclosure(list->term)) {
			Closure *closure = getclosure(list->term);
			assert(closure != NULL);
			if(closure->binding != NULL)
				return TRUE;
		}
	return FALSE;
}

static Var *
mkvar(List *defn)
{
	Ref(Var *, var, NULL);
	Ref(List *, lp, defn);
	var = gcnew(Var);
	var->env = NULL;
	var->defn = lp;
	var->flags = hasbindings(lp) ? var_hasbindings : 0;
	RefEnd(lp);
	RefReturn(var);
}

static void *
VarCopy(void *op)
{
	void *np = gcnew(Var);
	memcpy(np, op, sizeof(Var));
	return np;
}

static size_t
VarScan(void *p)
{
	Var *var = p;
	var->defn = forward(var->defn);
	var->env = ((var->flags & var_hasbindings) && rebound) ? NULL : forward(var->env);
	return sizeof(Var);
}

void
VarMark(void *p)
{
	Var *v;

	v = (Var *)p;
	gc_set_mark(header(p));
	gcmark(v->defn);
	if(!((v->flags & var_hasbindings) && rebound))
		gcmark(v->env);
}

/* iscounting -- is it a counter number, i.e., an integer > 0 */
static Boolean
iscounting(const char *name)
{
	int c;
	const char *s = name;
	while((c = *s++) != '\0')
		if(!isdigit(c))
			return FALSE;
	if(streq(name, "0"))
		return FALSE;
	return name[0] != '\0';
}

/*
 * public entry points
 */

/* validatevar -- ensure that a variable name is valid */
void
validatevar(const char *var)
{
	if(*var == '\0')
		fail("es:var", "zero-length variable name");
	if(iscounting(var))
		fail("es:var", "illegal variable name: %S", var);
	if(strchr(var, '=') != NULL)
		fail("es:var", "'=' in variable name: %S", var);
}

/* isexported -- is a variable exported? */
Boolean
isexported(const char *name)
{
	if(specialvar(name))
		return FALSE;
	if(noexport == NULL)
		return TRUE;
	return dictget(noexport, name) == NULL;
}

/* setnoexport -- mark a list of variable names not for export */
void
setnoexport(List *list)
{
	isdirty = TRUE;
	if(list == NULL) {
		noexport = NULL;
		return;
	}
	gcdisable();
	for(noexport = mkdict(); list != NULL; list = list->next)
		noexport = dictput(noexport, getstr(list->term), (void *)setnoexport);
	gcenable();
}

/* varlookup -- lookup a variable in the current context */
extern List *
varlookup(char *name, Binding *bp)
{
	Var *var;
	Push p;
	List *getter;
	List *defn;

	if(iscounting(name)) {
		Term *term = nth(varlookup("*", bp), strtol(name, NULL, 10));
		if(term == NULL)
			return NULL;
		return mklist(term, NULL);
	}

	validatevar(name);
	for(; bp != NULL; bp = bp->next)
		if(streq(name, bp->name))
			return bp->defn;

	var = dictget(vars, name);
	if(var == NULL)
		return NULL;
	defn = var->defn;

	if(!specialvar(name) && (getter = varlookup2("get-", name, NULL)) != NULL) {
		Ref(List *, lp, defn);
		Ref(List *, fn, getter);
		varpush(&p, "0", mklist(mkstr(name), NULL));
		lp = listcopy(eval(append(fn, lp), NULL, 0));
		varpop(&p);
		RefEnd(fn);
		RefReturn(lp);
	}
	return defn;
}

List *
varlookup2(char *name1, char *name2, Binding *bp)
{
	Var *var;

	for(; bp != NULL; bp = bp->next)
		if(streq2(bp->name, name1, name2))
			return bp->defn;

	var = dictget2(vars, name1, name2);
	if(var == NULL)
		return NULL;
	return var->defn;
}

Var *
varobjlookup(char *name)
{
	Var *v = NULL;

	if(iscounting(name))
		return NULL;

	validatevar(name);

	v = dictget(vars, name);
	return v;
}

void
varhide(Var *v)
{
	v->flags |= var_isinternal;
}

void
varunhide(Var *v)
{
	v->flags &= ~var_isinternal;
}

Boolean
varishidden(Var *v)
{
	if(v->flags & var_isinternal)
		return TRUE;
	return FALSE;
}

List *
callsettor(char *name, List *defn)
{
	Push p;
	List *settor;
	List *lp = NULL; Root r_lp;
	List *fn = NULL; Root r_fn;

	if(specialvar(name) || (settor = varlookup2("set-", name, NULL)) == NULL)
		return defn;

	lp = defn;
	fn = settor;
	gcref(&r_lp, (void **)&lp);
	gcref(&r_fn, (void **)&fn);

	varpush(&p, "0", mklist(mkstr(name), NULL));

	lp = listcopy(eval(append(fn, lp), NULL, 0));

	varpop(&p);

	gcderef(&r_fn, (void **)&fn);
	gcderef(&r_lp, (void **)&lp);
	return lp;
}

void
vardef(char *name, Binding *binding, List *defn)
{
	Var *var;
	Root r_name;

	validatevar(name);
	for(; binding != NULL; binding = binding->next)
		if(streq(name, binding->name)) {
			binding->defn = defn;
			rebound = TRUE;
			return;
		}

	gcref(&r_name, (void **)&name);

	defn = callsettor(name, defn);
	if(isexported(name))
		isdirty = TRUE;

	var = dictget(vars, name);
	if(var != NULL)
		if(defn != NULL) {
			var->defn = defn;
			var->env = NULL;
			var->flags = hasbindings(defn) ? var_hasbindings : 0;
			if(name[0] == '_')
				var->flags |= var_isinternal;
		} else
			vars = dictput(vars, name, NULL);
	else if(defn != NULL) {
		var = mkvar(defn);
		if(name[0] == '_')
			var->flags |= var_isinternal;
		vars = dictput(vars, name, var);
	}

	gcderef(&r_name, (void **)&name);
}

void
varpush(Push *push, char *name, List *defn)
{
	Var *var;

	validatevar(name);
	push->name = name;
	push->nameroot.next = rootlist;
	if(rootlist)
		rootlist->prev = &push->nameroot;
	push->nameroot.p = (void **)&push->name;
	rootlist = &push->nameroot;

	if(isexported(name))
		isdirty = TRUE;
	defn = callsettor(name, defn);

	var = dictget(vars, push->name);
	if(var == NULL) {
		push->defn = NULL;
		push->flags = 0;
		var = mkvar(defn);
		vars = dictput(vars, push->name, var);
	} else {
		push->defn = var->defn;
		push->flags = var->flags;
		var->defn = defn;
		var->env = NULL;
		var->flags = hasbindings(defn) ? var_hasbindings : 0;
	}

	push->next = pushlist;
	pushlist = push;

	push->defnroot.next = rootlist;
	rootlist->prev = &push->defnroot;
	push->defnroot.p = (void **)&push->defn;
	rootlist = &push->defnroot;
}

void
varpop(Push *push)
{
	Var *var;

	assert(pushlist == push);
	assert(rootlist == &push->defnroot);
	assert(rootlist->next == &push->nameroot);

	if(isexported(push->name))
		isdirty = TRUE;
	push->defn = callsettor(push->name, push->defn);
	var = dictget(vars, push->name);

	if(var != NULL)
		if(push->defn != NULL) {
			var->defn = push->defn;
			var->flags = push->flags;
			var->env = NULL;
		} else
			vars = dictput(vars, push->name, NULL);
	else if(push->defn != NULL) {
		var = mkvar(NULL);
		var->defn = push->defn;
		var->flags = push->flags;
		vars = dictput(vars, push->name, var);
	}

	pushlist = pushlist->next;
	rootlist = rootlist->next->next;
}

void
mkenv0(void *dummy, char *key, void *value)
{
	Var *var = value;
	char *envstr;
	Vector *newenv;

	assert(gcisblocked());
	if(var == NULL || var->defn == NULL || (var->flags & var_isinternal) || !isexported(key))
		return;

	if(var->env == NULL || (rebound && (var->flags & var_hasbindings))) {
		envstr = str(ENV_FORMAT, key, var->defn);
		var->env = envstr;
	}
	assert(env->count < env->alloclen);
	env->vector[env->count++] = var->env;
	if(env->count == env->alloclen) {
		newenv = mkvector(env->alloclen * 2);
		newenv->count = env->count;
		memcpy(newenv->vector, env->vector, env->count * sizeof *env->vector);
		env = newenv;
	}
}

Vector *
mkenv(void)
{
	if(isdirty || rebound) {
		env->count = envmin;
		gcdisable(); /* TODO: make this a good guess */
		dictforall(vars, mkenv0, NULL);
		gcenable();
		env->vector[env->count] = NULL;
		isdirty = FALSE;
		rebound = FALSE;
		if(sortenv == NULL || env->count > sortenv->alloclen)
			sortenv = mkvector(env->count * 2);
		sortenv->count = env->count;
		memcpy(sortenv->vector, env->vector, sizeof(char *) * (env->count + 1));
		sortvector(sortenv);
	}
	return sortenv;
}

/* addtolist -- dictforall procedure to create a list */
extern void
addtolist(void *arg, char *key, void *value)
{
	List **listp;
	Term *term = nil; Root r_term;

	gcref(&r_term, (void **)&term);

	listp = arg;
	/* dprintf(2, "addtolist: key = %p, ", key);
	dprintf(2, "*key = \"%s\"\n", key); */
	term = mkstr(key);
	*listp = mklist(term, *listp);

	gcrderef(&r_term);
}

static void
listexternal(void *arg, char *key, void *value)
{
	if((((Var *)value)->flags & var_isinternal) == 0 && !specialvar(key))
		addtolist(arg, key, value);
}

static void
listinternal(void *arg, char *key, void *value)
{
	if(((Var *)value)->flags & var_isinternal)
		addtolist(arg, key, value);
}

/* listvars -- return a list of all the (dynamic) variables */
List *
listvars(Boolean internal)
{
	List *varlist = NULL; Root r_varlist;

	varlist = NULL;
	gcref(&r_varlist, (void **)&varlist);

	dictforall(vars, internal ? listinternal : listexternal, &varlist);
	varlist = sortlist(varlist);

	gcderef(&r_varlist, (void **)&varlist);
	return varlist;
}

/* hide -- worker function for dictforall to hide initial state */
static void
hide(void *dummy, char *key, void *value)
{
	((Var *)value)->flags |= var_isinternal;
}

/* hidevariables -- mark all variables as internal */
extern void
hidevariables(void)
{
	dictforall(vars, hide, NULL);
}

/* initvars -- initialize the variable machinery */
extern void
initvars(void)
{
	globalroot(&vars);
	globalroot(&noexport);
	globalroot(&env);
	globalroot(&sortenv);
	vars = mkdict();
	noexport = NULL;
	env = mkvector(10);
}

/* importvar -- import a single environment variable */
void
importvar(char *name0, char *value)
{
	char sep[2] = {ENV_SEPARATOR, '\0'};
	char *str, *str2, *word, *escape;
	int offset;
	List *list;
	char *name = NULL; Root r_name;
	List *defn = NULL; Root r_defn;
	List *lp = NULL; Root r_lp;

	name = name0;
	defn = NULL;

	if(strcmp(name, "_") == 0)
		return;

	gcref(&r_name, (void **)&name);
	gcref(&r_defn, (void **)&defn);

	defn = fsplit(sep, mklist(mkstr(value + 1), NULL), FALSE);

	if(strchr(value, ENV_ESCAPE) != NULL) {
		gcdisable();
		for(list = defn; list != NULL; list = list->next) {
			offset = 0;
			word = list->term->str;
			while((escape = strchr(word + offset, ENV_ESCAPE)) != NULL) {
				offset = escape - word + 1;
				switch(escape[1]) {
				case '\0':
					if(list->next != NULL) {
						str2 = list->next->term->str;
						str = gcalloc(offset + strlen(str2) + 1, tString);
						memcpy(str, word, offset - 1);
						str[offset - 1] = ENV_SEPARATOR;
						strcpy(str + offset, str2);
						list->term->str = str;
						list->next = list->next->next;
					}
					break;
				case ENV_ESCAPE:
					str = gcalloc(strlen(word), tString);
					memcpy(str, word, offset);
					strcpy(str + offset, escape + 2);
					list->term->str = str;
					offset += 1;
					break;
				}
			}
		}
		gcenable();
	}
	gcref(&r_lp, (void **)&lp);
	for(lp = defn; lp != NULL; lp = lp->next)
		if(hasprefix(getstr(lp->term), "%dict"))
			getdict(lp->term);

	vardef(name, NULL, defn);

	gcderef(&r_lp, (void **)&lp);
	gcderef(&r_defn, (void **)&defn);
	gcderef(&r_name, (void **)&name);
}

/* initenv -- load variables from the environment */
void
initenv(char **envp, Boolean protected)
{
	char *envstr, *eq, *name, *buf;
	size_t bufsize = 1024;
	size_t nlen;
	Vector *newenv;

	buf = ealloc(bufsize);
	for(; (envstr = *envp) != NULL; envp++) {
		eq = strchr(envstr, '=');
		if(eq == NULL) {
			env->vector[env->count++] = envstr;
			if(env->count == env->alloclen) {
				newenv = mkvector(env->alloclen * 2);
				newenv->count = env->count;
				memcpy(newenv->vector, env->vector, env->count * sizeof(*env->vector));
				env = newenv;
			}
			continue;
		}
		for(nlen = eq - envstr; nlen >= bufsize; bufsize *= 2)
			buf = erealloc(buf, bufsize);
		memcpy(buf, envstr, nlen);
		buf[nlen] = '\0';
		name = str(ENV_DECODE, buf);
		if(!protected || (!hasprefix(name, "fn-") && !hasprefix(name, "set-")))
			importvar(name, eq);
	}

	envmin = env->count;
	efree(buf);
}
