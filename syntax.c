/* syntax.c -- abstract syntax tree re-writing rules ($Revision: 1.1.1.1 $) */

#include "es.h"
#include "input.h"
#include "syntax.h"
#include "token.h"

Tree errornode;
Tree *parsetree;

/* initparse -- called at the dawn of time */
extern void
initparse(void)
{
	globalroot(&parsetree);
}

/* treecons -- create new tree list cell */
extern Tree *
treecons(Tree *car, Tree *cdr)
{
	assert(cdr == NULL || cdr->kind == nList);
	return mk(nList, car, cdr);
}

/* treecons2 -- create new tree list cell or do nothing if car is NULL */
extern Tree *
treecons2(Tree *car, Tree *cdr)
{
	assert(cdr == NULL || cdr->kind == nList);
	return car == NULL ? cdr : mk(nList, car, cdr);
}

/* treeappend -- destructive append for tree lists */
extern Tree *
treeappend(Tree *head, Tree *tail)
{
	Tree *p, **prevp;
	for(p = head, prevp = &head; p != NULL; p = *(prevp = &p->CDR))
		assert(p->kind == nList || p->kind == nRedir);
	*prevp = tail;
	return head;
}

/* treeconsend -- destructive add node at end for tree lists */
extern Tree *
treeconsend(Tree *head, Tree *tail)
{
	return treeappend(head, treecons(tail, NULL));
}

/* treeconsend2 -- destructive add node at end for tree lists or nothing if tail is NULL */
extern Tree *
treeconsend2(Tree *head, Tree *tail)
{
	if(tail == NULL) {
		assert(head == NULL || head->kind == nList || head->kind == nRedir);
		return head;
	}
	return treeappend(head, treecons(tail, NULL));
}

/* thunkify -- wrap a tree in thunk braces if it isn't already a thunk */
extern Tree *
thunkify(Tree *tree)
{
	if(tree != NULL &&
	   ((tree->kind == nThunk) || (tree->kind == nList && tree->CAR->kind == nThunk && tree->CDR == NULL)))
		return tree;
	return mk(nThunk, tree);
}

/* firstis -- check if the first word of a literal command matches a known string */
static Boolean
firstis(Tree *t, const char *s)
{
	if(t == NULL || t->kind != nList)
		return FALSE;
	t = t->CAR;
	if(t == NULL || t->kind != nWord)
		return FALSE;
	assert(t->u[0].s != NULL);
	return streq(t->u[0].s, s);
}

/* prefix -- prefix a tree with a given word */
extern Tree *
prefix(char *s, Tree *t)
{
	return treecons(mk(nWord, s), t);
}

/* flatten -- flatten the output of the glommer so we can pass the result as a single element */
extern Tree *
flatten(Tree *t, char *sep)
{
	return mk(nCall, prefix("%flatten", treecons(mk(nQword, sep), treecons(t, NULL))));
}

/* backquote -- create a backquote command */
extern Tree *
backquote(Tree *ifs, Tree *body)
{
	return mk(nCall, prefix("%backquote", treecons(flatten(ifs, ""), treecons(body, NULL))));
}

/* stbackquote -- a mix of `{} and <={} */
extern Tree *
stbackquote(Tree *ifs, Tree *body)
{
	return mk(nCall, prefix("%stbackquote", treecons(flatten(ifs, ""), treecons(body, NULL))));
}

/* fnassign -- translate a function definition into an assignment */
extern Tree *
fnassign(Tree *name, Tree *params, Tree *body)
{
	Tree *defn;

	if(body == NULL)
		return mk(nAssign, mk(nConcat, mk(nWord, "fn-"), name), NULL);

	/*	defn = mk(nLambda, params, prefix("%block", treecons(thunkify(body), NULL))); */
	defn = mk(nLambda, params, body);
	return mk(nAssign, mk(nConcat, mk(nWord, "fn-"), name), defn);
}

/* mklambda -- create a lambda */
extern Tree *
mklambda(Tree *params, Tree *body)
{
	return mk(nLambda, params, body);
}

/* mkseq -- destructively add to a sequence of nList/nThink operations */
extern Tree *
mkseq(char *op, Tree *t1, Tree *t2)
{
	Tree *tail;
	Boolean sametail;

	if(streq(op, "%seq")) {
		if(t1 == NULL)
			return t2;
		if(t2 == NULL)
			return t1;
	}

	sametail = firstis(t2, op);
	tail = sametail ? t2->CDR : treecons(thunkify(t2), NULL);
	if(firstis(t1, op))
		return treeappend(t1, tail);
	t1 = thunkify(t1);
	if(sametail) {
		t2->CDR = treecons(t1, tail);
		return t2;
	}
	return prefix(op, treecons(t1, tail));
}

/* mkpipe -- assemble a pipe from the commands that make it up (destructive) */
extern Tree *
mkpipe(Tree *t1, int outfd, int infd, Tree *t2)
{
	Tree *tail;
	Boolean pipetail;

	pipetail = firstis(t2, "%pipe");
	tail =
		prefix(str("%d", outfd), prefix(str("%d", infd), pipetail ? t2->CDR : treecons(thunkify(t2), NULL)));
	if(firstis(t1, "%pipe"))
		return treeappend(t1, tail);
	t1 = thunkify(t1);
	if(pipetail) {
		t2->CDR = treecons(t1, tail);
		return t2;
	}
	return prefix("%pipe", treecons(t1, tail));
}

extern Tree *
mkfunpipe(Tree *t1, Tree *t2)
{
	Tree *firstcall, *res;

	if(t1 == NULL || t2 == NULL)
		fail("$&parse", "syntax error in function pipe");
	if(t1->kind != nList || t2->kind != nList)
		fail("&parse", "syntax error in function pipe");

	firstcall = mk(nCall, thunkify(t1));
	res = treeconsend(t2, firstcall);

	return res;
}

extern Tree *
mkonerror(Tree *captured, Tree *handler)
{
	captured = thunkify(captured);
	handler = thunkify(handler);

	return prefix("%onerror", treecons(captured, treecons(handler, NULL)));
}

/*
 * redirections -- these involve queueing up redirection in the prefix of a
 *	tree and then rewriting the tree to include the appropriate commands
 */

static Tree placeholder = {nRedir};

extern Tree *
redirect(Tree *t)
{
	Tree *r, *p;
	if(t == NULL)
		return NULL;
	if(t->kind != nRedir)
		return t;
	r = t->CAR;
	t = t->CDR;
	for(; r->kind == nRedir; r = r->CDR)
		t = treeappend(t, r->CAR);
	for(p = r; p->CAR != &placeholder; p = p->CDR) {
		assert(p != NULL);
		assert(p->kind == nList);
	}
	if(firstis(r, "%heredoc"))
		if(!queueheredoc(r))
			return &errornode;
	p->CAR = thunkify(redirect(t));
	return r;
}

extern Tree *
mkredircmd(char *cmd, int fd)
{
	return prefix(cmd, prefix(str("%d", fd), NULL));
}

extern Tree *
mkredir(Tree *cmd, Tree *file)
{
	Tree *word = NULL;
	if(file != NULL && file->kind == nThunk) { /* /dev/fd operations */
		char *op;
		Tree *var;
		static int id = 0;
		if(firstis(cmd, "%open"))
			op = "%readfrom";
		else if(firstis(cmd, "%create"))
			op = "%writeto";
		else {
			yyerror("bad /dev/fd redirection");
			op = "";
		}
		var = mk(nWord, str("_devfd%d", id++));
		cmd = treecons(mk(nWord, op), treecons(var, NULL));
		word = treecons(mk(nVar, var), NULL);
	} else if(!firstis(cmd, "%heredoc") && !firstis(cmd, "%here"))
		file = mk(nCall, prefix("%one", treecons(file, NULL)));
	cmd = treeappend(cmd, treecons(file, treecons(&placeholder, NULL)));
	if(word != NULL)
		cmd = mk(nRedir, word, cmd);
	return cmd;
}

/* mkclose -- make a %close node with a placeholder */
extern Tree *
mkclose(int fd)
{
	return prefix("%close", prefix(str("%d", fd), treecons(&placeholder, NULL)));
}

/* mkdup -- make a %dup node with a placeholder */
extern Tree *
mkdup(int fd0, int fd1)
{
	return prefix("%dup", prefix(str("%d", fd0), prefix(str("%d", fd1), treecons(&placeholder, NULL))));
}

/* redirappend -- destructively add to the list of redirections, before any other nodes */
extern Tree *
redirappend(Tree *tree, Tree *r)
{
	Tree *t, **tp;
	for(; r->kind == nRedir; r = r->CDR)
		tree = treeappend(tree, r->CAR);
	assert(r->kind == nList);
	for(t = tree, tp = &tree; t != NULL && t->kind == nRedir; t = *(tp = &t->CDR))
		;
	assert(t == NULL || t->kind == nList);
	*tp = mk(nRedir, r, t);
	return tree;
}

/* mkmatch -- rewrite for match */
extern Tree *
mkmatch(Tree *subj, Tree *cases)
{
	const char *varname = "matchexpr";
	Tree *sass, *svar, *matches = NULL;
	Tree *pattlist, *cmd, *match;

	sass = treecons2(mk(nAssign, mk(nWord, varname), subj), NULL);
	svar = mk(nVar, mk(nWord, varname));

	for(; cases != NULL; cases = cases->CDR) {
		pattlist = cases->CAR->CAR;
		cmd = cases->CAR->CDR;

		if(pattlist != NULL && pattlist->kind == nLambda) {
			pattlist = treecons(pattlist, NULL);
			pattlist = treeconsend(pattlist, svar);
		} else if(pattlist != NULL && pattlist->kind != nList) {
			pattlist = treecons(pattlist, NULL);
			pattlist = mk(nMatch, svar, pattlist);
		} else
			pattlist = mk(nMatch, svar, pattlist);

		match = treecons(thunkify(pattlist), treecons(cmd, NULL));
		matches = treeappend(matches, match);
	}

	matches = thunkify(prefix("if", matches));

	return mk(nLocal, sass, matches);
}

int
is_all_patterns(Tree *pattern)
{
	char *str;
	Tree *p, *e;

	if(pattern == NULL)
		return 0;
	if(pattern->kind == nWord) {
		str = pattern->u[0].s;
		if(str[0] == '*' && str[1] == 0)
			return 1;
	}
	if(pattern->kind == nList) {
		e = pattern->CAR;
		p = pattern->CDR;
		if(is_all_patterns(e))
			return 1;
		return is_all_patterns(p);
	}
	return 0;
}

extern Tree *
mkmatchall(Tree *subj, Tree *cases)
{
	const char *varname = "matchexpr";
	const char *resname = "resexpr";
	const char *result = "result";
	const char *hasmatched = "__es_no_matches";
	const char *truestr = "true";
	const char *falsestr = "false";
	int is_wild = 0, has_wild = 0;
	Tree *ifs = NULL;
	Tree *svar, *resvar, *matchvar, *varbinding;
	Tree *pattlist, *cmd, *match, *ifbody, *wildmatch;
	Tree *resultnil;

	svar = mk(nVar, mk(nWord, varname));
	resvar = mk(nVar, mk(nWord, resname));
	matchvar = mk(nVar, mk(nWord, hasmatched));
	varbinding = treecons(mk(nAssign, mk(nWord, varname), subj), NULL);
	varbinding = treeconsend2(varbinding, mk(nAssign, mk(nWord, resname), NULL));
	varbinding = treeconsend2(varbinding, mk(nAssign, mk(nWord, hasmatched), mk(nWord, truestr)));

	for(; cases != NULL; cases = cases->CDR) {
		pattlist = cases->CAR->CAR;
		cmd = cases->CAR->CDR;

		is_wild = is_all_patterns(pattlist);

		if(pattlist != NULL && pattlist->kind == nLambda) {
			pattlist = treecons(pattlist, NULL);
			pattlist = treeconsend(pattlist, svar);
		} else if(pattlist != NULL && pattlist->kind != nList) {
			pattlist = treecons(pattlist, NULL);
			pattlist = mk(nMatch, svar, pattlist);
		} else
			pattlist = mk(nMatch, svar, pattlist);

		resultnil = treecons(thunkify(treecons(mk(nWord, result), NULL)), NULL);
		if(!is_wild) {
			ifbody = mkseq("%seq", thunkify(mk(nAssign, mk(nWord, hasmatched), mk(nWord, falsestr))), cmd);
			ifbody = thunkify(ifbody);
			match = treecons(
				mk(nCall, thunkify(prefix("if", treecons(thunkify(pattlist), treecons(ifbody, resultnil))))),
				NULL);
			match = mk(nList, resvar, match);
			match = thunkify(mk(nAssign, mk(nWord, resname), match));
			ifs = mkseq("%seq", ifs, match);
		} else if(is_wild && has_wild) {
			fail("$&parse", "more than one always match case in matchall");
		} else {
			wildmatch = treecons(mk(nCall, cmd), NULL);
			wildmatch = thunkify(mk(nAssign, mk(nWord, resname), wildmatch));
			wildmatch = prefix("if", treecons(thunkify(matchvar), treecons(wildmatch, NULL)));
			has_wild = 1;
		}
	}
	if(has_wild)
		ifs = mkseq("%seq", ifs, wildmatch);
	ifs = mkseq("%seq", ifs, thunkify(mk(nList, mk(nWord, result), mk(nList, resvar, NULL))));
	return mk(nLocal, varbinding, thunkify(ifs));
}

extern Tree *
mkprocess(Tree *subj, Tree *cases)
{
	const char *varstr = "matchelement";
	const char *resstr = "__es_process_result";
	const char *resultstr = "result";
	Tree *forbindings, *mevar;
	Tree *localbindings, *resvar, *resname;
	Tree *matchterms, *forterm, *resultterm;

	resname = mk(nWord, resstr);
	forbindings = treecons(mk(nAssign, mk(nWord, varstr), subj), NULL);
	localbindings = treecons(mk(nAssign, resname, NULL), NULL);
	mevar = mk(nVar, mk(nWord, varstr));
	resvar = mk(nVar, resname);

	matchterms = treecons(mk(nCall, thunkify(mkmatch(treecons(mevar, NULL), cases))), NULL);
	matchterms = mk(nList, resvar, matchterms);
	matchterms = thunkify(mk(nAssign, resname, matchterms));

	forterm = mk(nFor, forbindings, matchterms);
	resultterm = thunkify(mk(nList, mk(nWord, resultstr), mk(nList, resvar, NULL)));
	return mk(nLocal, localbindings, mkseq("%seq", thunkify(forterm), resultterm));
}

Tree *
mkdictassign(Tree *sub, Tree *assoc)
{
	Tree *args;

	if(assoc->CDR == NULL || assoc->CDR->CAR == NULL)
		return mk(
			nAssign, sub,
			mk(nCall, thunkify(prefix("dictremove", treecons(mk(nVar, sub), treecons(assoc->CAR, NULL))))));
	args = treeappend(treecons(assoc->CAR, NULL), assoc->CDR);
	return mk(nAssign, sub, mk(nCall, thunkify(prefix("dictput", treecons(mk(nVar, sub), args)))));
}
