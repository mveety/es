/* input.c -- read input from files or strings ($Revision: 1.2 $) */
/* stdgetenv is based on the FreeBSD getenv */

#include "editor.h"
#include "es.h"
#include "stdenv.h"
#include <stdio.h>
#include <string.h>
// #include <threads.h>
#include "input.h"

/*
 * constants
 */

#define BUFSIZE ((size_t)4096)	/* buffer size to fill reads into */
#define MATCHTABLE ((size_t)10) /* initial match table size */

/*
 * macros
 */

#define ISEOF(in) ((in)->fill == eoffill)

/*
 * globals
 */

Input *input;
int prompt = 0;
char *prompt1;
char *prompt2;

Boolean disablehistory = FALSE;
Boolean resetterminal = FALSE;
Boolean dump_tok_status = FALSE;
static char *history;
char *lastcmd, *nextlastcmd;
static int historyfd = -1;
extern Boolean verbose_parser;
EditorState *editor;
Keymap *default_keymap;
int editor_debugfd = -1;

#if ABUSED_GETENV
static char *stdgetenv(const char *);
static char *esgetenv(const char *);
static char *(*realgetenv)(const char *) = stdgetenv;
#endif

/*
 * errors and warnings
 */

/* locate -- identify where an error came from */
static char *
locate(Input *in, char *s)
{
	return (in->runflags & run_interactive) ? s : str("%s:%d: %s", in->name, in->lineno, s);
}

static char *error = NULL;

/* yyerror -- yacc error entry point */
extern void
yyerror(char *s)
{
#if sgi
	/* this is so that trip.es works */
	if(streq(s, "Syntax error"))
		s = "syntax error";
#endif
	if(error == NULL) /* first error is generally the most informative */
		error = locate(input, s);
}

/* warn -- print a warning */
static void
warn(char *s)
{
	eprint("warning: %s\n", locate(input, s));
}

/*
 * history
 */

void
setnextlastcmd(char *str)
{
	free(nextlastcmd);
	nextlastcmd = strdup(str);
}

char *
getnextlastcmd(void)
{
	return nextlastcmd;
}

/* loghistory -- write the last command out to a file */
static void
loghistory(const char *cmd, size_t len)
{
	const char *s, *end;
	unsigned long curtime;

	if(history == NULL || disablehistory)
		return;
	if(strnlen(cmd, len) > 1) {
		if(lastcmd != NULL)
			efree(lastcmd);
		if(nextlastcmd != NULL)
			lastcmd = nextlastcmd;
		nextlastcmd = ealloc(len + 2);
		memcpy(nextlastcmd, cmd, len);
		nextlastcmd[len - 1] = '\0';
	}
	if(historyfd == -1) {
		historyfd = eopen(history, oAppend);
		if(historyfd == -1) {
			eprint("history(%s): %s\n", history, esstrerror(errno));
			vardef("history", NULL, NULL);
			return;
		}
	}
	/* skip empty lines and comments in history */
	for(s = cmd, end = s + len; s < end; s++)
		switch(*s) {
		case '#':
		case '\n':
			return;
		case ' ':
		case '\t':
			break;
		default:
			goto writeit;
		}
writeit:;

	/*
	 * we're interested in writing out history in the csh format
	 * for compatibility reasons.
	 */
	curtime = time(NULL);
	dprintf(historyfd, "#+%lu\n", curtime);
	/*
	 * Small unix hack: since read() reads only up to a newline
	 * from a terminal, then presumably this write() will write at
	 * most only one input line at a time.
	 */
	ewrite(historyfd, cmd, len);

	// we close the history file after using it so that other programs
	// can use it without causing too many problems
	close(historyfd);
	historyfd = -1;
}

/* sethistory -- change the file for the history log */
extern void
sethistory(char *file)
{
	if(historyfd != -1) {
		close(historyfd);
		historyfd = -1;
	}
	history = file;
}

/*
 * unget -- character pushback
 */

/* ungetfill -- input->fill routine for ungotten characters */
static int
ungetfill(Input *in)
{
	int c;
	assert(in->ungot > 0);
	c = in->unget[--in->ungot];
	if(in->ungot == 0) {
		assert(in->rfill != NULL);
		in->fill = in->rfill;
		in->rfill = NULL;
		assert(in->rbuf != NULL);
		in->buf = in->rbuf;
		in->rbuf = NULL;
	}
	return c;
}

/* unget -- push back one character */
extern void
unget(Input *in, int c)
{
	if(in->ungot > 0) {
		assert(in->ungot < MAXUNGET);
		in->unget[in->ungot++] = c;
	} else if(in->bufbegin < in->buf && in->buf[-1] == c && (input->runflags & run_echoinput) == 0)
		--in->buf;
	else {
		assert(in->rfill == NULL);
		in->rfill = in->fill;
		in->fill = ungetfill;
		assert(in->rbuf == NULL);
		in->rbuf = in->buf;
		in->buf = in->bufend;
		assert(in->ungot == 0);
		in->ungot = 1;
		in->unget[0] = c;
	}
}

/*
 * getting characters
 */

/* get -- get a character, filter out nulls */
static int
get(Input *in)
{
	int c;
	while((c = (in->buf < in->bufend ? *in->buf++ : (*in->fill)(in))) == '\0')
		warn("null character ignored");
	return c;
}

/* getverbose -- get a character, print it to standard error */
static int
getverbose(Input *in)
{
	if(in->fill == ungetfill)
		return get(in);
	else {
		int c = get(in);
		if(c != EOF) {
			char buf = c;
			ewrite(2, &buf, 1);
		}
		return c;
	}
}

int
input_getc(void)
{
	int cc;

	cc = (*input->get)(input);
	assert(input->tokstatusi < MAXTOKBUF);
	input->tokstatus[input->tokstatusi] = (char)cc;
	input->tokstatusi++;
	return cc;
}

void
input_ungetc(int c)
{
	assert(input->tokstatusi > 0);
	input->tokstatusi--;
	unget(input, c);
}

void
input_resettokstatus(void)
{
	char *s;

	s = input_dumptokstatus();
	if(verbose_parser == TRUE)
		dprintf(2, "input %s(%p): last tokstatus = \"%s\"\n", input->name, input, s);
	if(input->lasttokstatus != NULL) {
		free(input->lasttokstatus);
		input->lasttokstatus = NULL;
	}
	input->tokstatusi = 0;
}

char *
input_dumptokstatus(void)
{
	input->tokstatus[input->tokstatusi] = 0;
	input->lasttokstatus = strndup(&input->tokstatus[0], MAXTOKBUF);
	return input->lasttokstatus;
}

/* eoffill -- report eof when called to fill input buffer */
static int
eoffill(Input *in)
{
	assert(in->fd == -1);
	return EOF;
}

static char *
call_editor(int which_prompt)
{
	char *res;

	if(which_prompt > 1)
		which_prompt = 0;

	if(resetterminal) {
		resetterminal = FALSE;
	}
	editor->which_prompt = which_prompt;
	interrupted = FALSE;
	if(!setjmp(slowlabel)) {
		slow = TRUE;
		res = interrupted ? nil : line_editor(editor);
	} else
		res = nil;
	slow = FALSE;
	if(res == nil)
		errno = EINTR;
	SIGCHK();
	return res;
}

#if ABUSED_GETENV

/* getenv -- fake version of getenv for readline (or other libraries) */
static char *
esgetenv(const char *name)
{
	List *value = varlookup(name, NULL);
	if(value == NULL)
		return NULL;
	else {
		char *export;
		static Dict *envdict;
		static Boolean initialized = FALSE;
		Ref(char *, string, NULL);

		gcdisable();
		if(!initialized) {
			initialized = TRUE;
			envdict = mkdict();
			globalroot(&envdict);
		}

		string = dictget(envdict, name);
		if(string != NULL)
			efree(string);

		export = str("%W", value);
		string = ealloc(strlen(export) + 1);
		strcpy(string, export);
		envdict = dictput(envdict, (char *)name, string);

		gcenable();
		RefReturn(string);
	}
}

static char *
stdgetenv(name)
register const char *name;
{
	extern char **environ;
	register int len;
	register const char *np;
	register char **p, *c;

	if(name == NULL || environ == NULL)
		return (NULL);
	for(np = name; *np && *np != '='; ++np)
		continue;
	len = np - name;
	for(p = environ; (c = *p) != NULL; ++p)
		if(strncmp(c, name, len) == 0 && c[len] == '=') {
			return (c + len + 1);
		}
	return (NULL);
}

extern void
initgetenv(void)
{
	realgetenv = esgetenv;
}

#endif /* ABUSED_GETENV */

char **
run_new_completer(List *completer0, const char *text, int start, int end)
{
	List *completer = nil; Root r_completer;
	List *args = nil; Root r_args;
	List *result = nil; Root r_result;
	List *lp = nil; Root r_lp;
	char **matches;
	size_t matchsz;
	size_t matchi;

	gcref(&r_completer, (void **)&completer);
	gcref(&r_args, (void **)&args);
	gcref(&r_result, (void **)&result);
	gcref(&r_lp, (void **)&lp);

	completer = completer0;
	matches = ealloc(MATCHTABLE * sizeof(char *));
	matchsz = MATCHTABLE;

	gcenable();
	assert(!gcisblocked());

	args = mklist(mkstr(str("%s", editor->buffer)),
				  mklist(mkstr(str("%s", text)),
						 mklist(mkstr(str("%d", start)), mklist(mkstr(str("%d", end)), nil))));
	completer = append(completer, args);
	result = eval(completer, nil, 0);

	gcdisable();

	if(result == nil) {
		free(matches);
		matches = nil;
		goto done;
	}

	for(lp = result, matchi = 0; lp != nil; lp = lp->next) {
		matches[matchi++] = estrdup(getstr(lp->term));
		if(matchi >= matchsz - 2) {
			matchsz += 10;
			matches = erealloc(matches, (matchsz * sizeof(char *)));
		}
	}
	while(matchi < matchsz)
		matches[matchi++] = nil;

done:
	gcenable();
	gcrderef(&r_lp);
	gcrderef(&r_result);
	gcrderef(&r_args);
	gcrderef(&r_completer);
	gcdisable();
	return matches;
}

char **
es_complete_hook(char *text, int start, int end)
{
	List *new_completer = nil; Root r_new_completer;
	List *complete_sort_list = nil; Root r_complete_sort_list;
	List *complete_remove_dupes = nil; Root r_complete_remove_dupes;

	new_completer = varlookup("fn-%new_completer", nil);
	if(!new_completer)
		return nil;

	gcref(&r_new_completer, (void **)&new_completer);
	gcref(&r_complete_sort_list, (void **)&complete_sort_list);
	gcref(&r_complete_remove_dupes, (void**)&complete_remove_dupes);

	complete_sort_list = varlookup("esmle_conf_sort-completions", nil);
	complete_remove_dupes = varlookup("esmle_conf_remove-duplicate-completions", nil);
	if(complete_sort_list != nil) {
		if(termeq(complete_sort_list->term, "true")) {
			editor->sort_completions = 1;
			if(complete_remove_dupes != nil && termeq(complete_remove_dupes->term, "true")){
				editor->remove_duplicates = 1;
			} else {
				editor->remove_duplicates = 0;
			}
		} else {
			editor->sort_completions = 0;
			editor->remove_duplicates = 0;
		}
	}

	gcrderef(&r_complete_remove_dupes);
	gcrderef(&r_complete_sort_list);
	gcrderef(&r_new_completer);

	return run_new_completer(new_completer, text, start, end);
}

int
is_whitespace(char c)
{
	switch(c) {
	case '\n':
	case '\t':
	case ' ':
	case 0:
		return 1;
	}
	return 0;
}

long
containsbangbang(char *s, int len)
{
	long pos = 0;
	int state = 0;
	long i;

	for(i = 0; s[i] != 0 && i < len; i++) {
		switch(state) {
		case 0:
			if(i == 0 && s[i] == '!')
				state = 2;
			else if(s[i] == ' ')
				state = 1;
			else if(s[i] == '\'')
				state = 4;
			break;
		case 1:
			if(s[i] == '!') {
				state = 2;
				pos = i;
			} else if(s[i] == '\'')
				state = 4;
			else
				state = 0;
			break;
		case 2:
			if(s[i] == '!')
				state = 3;
			else if(s[i] == '\'')
				state = 4;
			else
				state = 0;
			break;
		case 3:
			if(is_whitespace(s[i]))
				return pos;
			else if(s[i] == '\'')
				state = 4;
			else
				state = 0;
			break;
		case 4:
			if(s[i] == '\'')
				state = 5;
			break;
		case 5:
			if(s[i] == '\'')
				state = 4;
			else if(s[i] == ' ')
				state = 1;
			else
				state = 0;
		}
	}
	if(state == 3)
		return pos;
	return -1;
}

long
copybuffer(Input *in, char *linebuf, long nread)
{
	long bbpos = -1;
	long lastcmdlen;
	long lbi, lasti, inbufi;
	long nwrote;

	nwrote = nread;
	if(nextlastcmd != NULL) {
		if((bbpos = containsbangbang(linebuf, strlen(linebuf))) >= 0)
			nwrote += (strlen(nextlastcmd) - 2); /* remember we replaced the !! with nextlastcmd */
	}
	if(in->buflen < (unsigned int)nwrote + 1) {
		while(in->buflen < (unsigned int)nwrote + 1)
			in->buflen *= 2;
		in->bufbegin = erealloc(in->bufbegin, in->buflen);
	}
	if(bbpos < 0) {
		memcpy(in->bufbegin, linebuf, nwrote);
		return nwrote;
	}
	lbi = 0;
	inbufi = 0;
	lastcmdlen = strlen(nextlastcmd);
	while(lbi < nread) {
		if(lbi == bbpos) {
			for(lasti = 0; nextlastcmd[lasti] != 0 && lasti < lastcmdlen; inbufi++, lasti++)
				in->bufbegin[inbufi] = nextlastcmd[lasti];
			lbi += 2;
		} else {
			in->bufbegin[inbufi] = linebuf[lbi];
			inbufi++;
			lbi++;
		}
	}
	return nwrote;
}

/* fdfill -- fill input buffer by reading from a file descriptor */
static int
fdfill(Input *in)
{
	long nread;
	char *line_in;
	List *history_hook;
	List *args;
	List *result = NULL; Root r_result;
	size_t i;

	assert(in->buf == in->bufend);
	assert(in->fd >= 0);

	if(in->runflags & run_interactive && in->fd == 0) {
		char *rlinebuf = call_editor(prompt);
		if(rlinebuf == NULL)
			nread = 0;
		else {
			nread = copybuffer(in, rlinebuf, strlen(rlinebuf)) + 1;
			in->bufbegin[nread - 1] = '\n';
			efree(rlinebuf);
		}
	} else
		do {
			nread = eread(in->fd, (char *)in->bufbegin, in->buflen);
			SIGCHK();
		} while(nread == -1 && errno == EINTR);

	if(nread <= 0) {
		close(in->fd);
		in->fd = -1;
		in->fill = eoffill;
		in->runflags &= ~run_interactive;
		if(nread == -1)
			fail("$&parse", "%s: %s", in->name == NULL ? "es" : in->name, esstrerror(errno));
		return EOF;
	}

	if(in->runflags & run_interactive) {
		line_in = strndup((char *)in->bufbegin, nread);
		for(i = 0; line_in[i] != 0; i++)
			if(line_in[i] == '\n') {
				line_in[i] = 0;
				break;
			}
		history_hook = varlookup("fn-%history", NULL);
		if(!disablehistory) {
			if(history_hook == NULL) {
				if(*line_in != '\0')
					history_add(editor, line_in);
				loghistory((char *)in->bufbegin, nread);
			} else {
				gcenable();
				gcref(&r_result, (void **)&result);
				args = mklist(mkstr(str("%s", line_in)), NULL);
				history_hook = append(history_hook, args);
				result = eval(history_hook, NULL, 0);

				assert(result != NULL);

				if(strcmp("0", getstr(result->term)) == 0) {
					if(*line_in != '\0')
						history_add(editor, line_in);
					loghistory((char *)in->bufbegin, nread);
				}
				gcrderef(&r_result);
				gcdisable();
			}
		}
		free(line_in);
	}
	in->buf = in->bufbegin;
	in->bufend = &in->buf[nread];
	return *in->buf++;
}

/*
 * the input loop
 */

/* parse -- call yyparse(), but disable garbage collection and catch errors */
extern Tree *
parse(char *pr1, char *pr2)
{
	int result;
	assert(error == NULL);

	inityy();
	emptyherequeue();

	if(ISEOF(input))
		throw(mklist(mkstr("eof"), NULL));

	if(pr1 == nil)
		pr1 = "";
	if(pr2 == nil)
		pr2 = pr1;

	prompt = 0;
	if(input->runflags & run_interactive) {
		set_prompt1(editor, pr1);
		set_prompt2(editor, pr2);
	}

	gcreserve(300 * sizeof(Tree));
	gcdisable();
	result = yyparse();
	gcenable();

	if(result || error != NULL) {
		char *e;
		assert(error != NULL);
		e = error;
		error = NULL;
		if(dump_tok_status)
			fail("$&parse", "yyparse: %s: \"%s\"", e, input_dumptokstatus());
		fail("$&parse", "yyparse: %s", e);
	}
	if(input->runflags & run_lisptrees)
		eprint("%B\n", parsetree);
	return parsetree;
}

/* resetparser -- clear parser errors in the signal handler */
extern void
resetparser(void)
{
	error = NULL;
}

/* runinput -- run from an input source */
extern List *
runinput(Input *in, int runflags)
{
	volatile int flags = runflags;
	List *volatile result = NULL;
	List *repl, *dispatch;
	Push push;
	char *dispatcher[] = {
		"fn-%eval-noprint",
		"fn-%eval-print",
		"fn-%noeval-noprint",
		"fn-%noeval-print",
	};

	flags &= ~eval_inchild;
	in->runflags = flags;
	in->get = (flags & run_echoinput) ? getverbose : get;
	in->prev = input;
	input = in;

	ExceptionHandler {
		dispatch = varlookup(
			dispatcher[((flags & run_printcmds) ? 1 : 0) + ((flags & run_noexec) ? 2 : 0)], NULL);
		if(flags & eval_exitonfalse)
			dispatch = mklist(mkstr("%exit-on-false"), dispatch);
		varpush(&push, "fn-%dispatch", dispatch);

		repl =
			varlookup((flags & run_interactive) ? "fn-%interactive-loop" : "fn-%batch-loop", NULL);
		result = (repl == NULL) ? prim("batchloop", NULL, NULL, flags) : eval(repl, NULL, flags);

		varpop(&push);
	} CatchException (e) {
		(*input->cleanup)(input);
		input = input->prev;
		throw(e);
	} EndExceptionHandler;

	input = in->prev;
	(*in->cleanup)(in);
	return result;
}

/*
 * pushing new input sources
 */

/* fdcleanup -- cleanup after running from a file descriptor */
static void
fdcleanup(Input *in)
{
	unregisterfd(&in->fd);
	if(in->fd != -1)
		close(in->fd);
	efree(in->bufbegin);
}

/* runfd -- run commands from a file descriptor */
extern List *
runfd(int fd, const char *name, int flags)
{
	Input in;
	List *result;

	memzero(&in, sizeof(Input));
	in.lineno = 1;
	in.fill = fdfill;
	in.cleanup = fdcleanup;
	in.fd = fd;
	registerfd(&in.fd, TRUE);
	in.buflen = BUFSIZE;
	in.bufbegin = in.buf = ealloc(in.buflen);
	in.bufend = in.bufbegin;
	in.name = (name == NULL) ? str("fd %d", fd) : name;

	RefAdd(in.name);
	result = runinput(&in, flags);
	RefRemove(in.name);

	return result;
}

/* stringcleanup -- cleanup after running from a string */
static void
stringcleanup(Input *in)
{
	efree(in->bufbegin);
}

/* stringfill -- placeholder than turns into EOF right away */
static int
stringfill(Input *in)
{
	in->fill = eoffill;
	return EOF;
}

/* runstring -- run commands from a string */
extern List *
runstring(const char *str, const char *name, int flags)
{
	Input in;
	List *result;
	unsigned char *buf;

	assert(str != NULL);

	memzero(&in, sizeof(Input));
	in.fd = -1;
	in.lineno = 1;
	in.name = (name == NULL) ? str : name;
	in.fill = stringfill;
	in.buflen = strlen(str);
	buf = ealloc(in.buflen + 1);
	memcpy(buf, str, in.buflen);
	in.bufbegin = in.buf = buf;
	in.bufend = in.buf + in.buflen;
	in.cleanup = stringcleanup;

	RefAdd(in.name);
	result = runinput(&in, flags);
	RefRemove(in.name);
	return result;
}

/* parseinput -- turn an input source into a tree */
extern Tree *
parseinput(Input *in)
{
	Tree *volatile result = NULL;

	in->prev = input;
	in->runflags = 0;
	in->get = get;
	input = in;

	ExceptionHandler {
		result = parse(NULL, NULL);
		if(get(in) != EOF)
			fail("$&parse", "more than one value in term");
	} CatchException (e) {
		(*input->cleanup)(input);
		input = input->prev;
		throw(e);
	} EndExceptionHandler;

	input = in->prev;
	(*in->cleanup)(in);
	return result;
}

/* parsestring -- turn a string into a tree; must be exactly one tree */
extern Tree *
parsestring(const char *str)
{
	Input in;
	Tree *result;
	unsigned char *buf;

	assert(str != NULL);

	/* TODO: abstract out common code with runstring */

	memzero(&in, sizeof(Input));
	in.fd = -1;
	in.lineno = 1;
	in.name = str;
	in.fill = stringfill;
	in.buflen = strlen(str);
	buf = ealloc(in.buflen + 1);
	memcpy(buf, str, in.buflen);
	in.bufbegin = in.buf = buf;
	in.bufend = in.buf + in.buflen;
	in.cleanup = stringcleanup;

	RefAdd(in.name);
	result = parseinput(&in);
	RefRemove(in.name);
	return result;
}

/* isinteractive -- is the innermost input source interactive? */
extern Boolean
isinteractive(void)
{
	return input == NULL ? FALSE : ((input->runflags & run_interactive) != 0);
}

int
getrunflags(char *s, size_t sz)
{
	int i = 0;

	if(sz < 8)
		return -1;
	memset(s, 0, sz);

	if(input == NULL)
		return 0;
	if(assertions == TRUE)
		s[i++] = 'a';
	if(input->runflags & eval_exitonfalse)
		s[i++] = 'e';
	if(input->runflags & run_interactive)
		s[i++] = 'i';
	if(input->runflags & run_noexec)
		s[i++] = 'n';
	if(input->runflags & run_echoinput)
		s[i++] = 'v';
	if(input->runflags & run_printcmds)
		s[i++] = 'x';
	if(input->runflags & run_lisptrees)
		s[i++] = 'L';

	if(i == 0)
		s[0] = '-';

	return 0;
}

int
setrunflags(char *s, size_t sz)
{
	char *si;
	size_t i = 0;
	int state = 0; // 0 -- add flag, 1 -- remove flag
	int new_runflags = 0;
	int nextflag = 0;

	if(input == NULL)
		return -1;
	if(sz == 0)
		return 0;
	new_runflags = input->runflags;

	for(si = s; *si != 0 && i < sz; si++, i++) {
		switch(*si) {
		case '-':
			state = 1;
			continue;
			break;
		case 'a':
			if(state)
				assertions = FALSE;
			else
				assertions = TRUE;
			state = 0;
			continue;
			break;
		case 'e':
			nextflag = eval_exitonfalse;
			break;
		case 'i':
			nextflag = run_interactive;
			break;
		case 'n':
			nextflag = run_noexec;
			break;
		case 'v':
			nextflag = run_echoinput;
			break;
		case 'x':
			nextflag = run_printcmds;
			break;
		case 'L':
			nextflag = run_lisptrees;
			break;
		}
		if(state)
			new_runflags &= ~nextflag;
		else
			new_runflags |= nextflag;

		state = 0;
	}

	input->runflags = new_runflags;
	return 0;
}

char *
line_editor_hook(EditorState *state, int key, void *aux)
{
	char *fnname = nil;
	int gcblocked = 0;
	List *hook = nil; Root r_hook;
	List *args = nil; Root r_args;
	List *res = nil; Root r_res;
	char *resstr = nil;
	char *curline = nil;

	fnname = aux;

	gcref(&r_hook, (void **)&hook);
	gcref(&r_args, (void **)&args);
	gcref(&r_res, (void **)&res);

	if((hook = varlookup2("fn-", fnname, nil)) == nil)
		goto fail;

	if(gcisblocked()) {
		gcenable();
		gcblocked = 1;
	}

	curline = getcurrentline(state);
	args = mklist(mkstr(str("%s", curline)), nil);
	hook = append(hook, args);
	rawmode_off(state);
	res = eval(hook, nil, 0);
	rawmode_on(state);

	if(res == nil)
		goto fail;

	if(res->next == nil)
		goto fail;

	if(termeq(res->term, "0"))
		resstr = estrdup(getstr(res->next->term));

fail:
	if(curline)
		free(curline);
	if(gcblocked)
		gcdisable();
	gcrderef(&r_res);
	gcrderef(&r_args);
	gcrderef(&r_hook);
	return resstr;
}

int
bind_es_function(char *keyname, char *function)
{
	int key = 0;

	if((key = name2key(keyname)) < 0)
		return -1;

	bindmapping(editor, key,
				(Mapping){
					.hook = &line_editor_hook,
					.base_hook = nil,
					.aux = estrdup(function),
					.breakkey = 0,
					.reset_completion = 1,
					.end_of_file = 0,
				});

	return 0;
}

int
unbind_es_function(char *keyname)
{
	int key = 0;
	Mapping default_map;
	Mapping current_map;

	if((key = name2key(keyname)) < 0)
		return -1;

	if(key > KeyNull && key < KeyMax) {
		default_map = default_keymap->base_keys[key];
		current_map = editor->keymap->base_keys[key];
	} else if(key >= ExtKeyOffset && key < ExtKeyMax) {
		default_map = default_keymap->ext_keys[key - ExtKeyOffset];
		current_map = editor->keymap->ext_keys[key - ExtKeyOffset];
	} else
		return -2;

	if((void *)current_map.hook != (void *)&line_editor_hook)
		return -3;

	bindmapping(editor, key, default_map);
	return 0;
}

int
map_as_key(char *keyname, char *srckeyname)
{
	int key = 0;
	int srckey = 0;
	Mapping default_map;

	if((key = name2key(keyname)) < 0)
		return -1;
	if((srckey = name2key(srckeyname)) < 0)
		return -2;

	if(srckey > KeyNull && srckey < KeyMax) {
		default_map = default_keymap->base_keys[srckey];
	} else if(srckey >= ExtKeyOffset && srckey < ExtKeyMax) {
		default_map = default_keymap->ext_keys[srckey - ExtKeyOffset];
	} else
		return -3;

	bindmapping(editor, key, default_map);
	return 0;
}

int
clear_key_mapping(char *keyname)
{
	int key = 0;
	Mapping clear_mapping;

	if((key = name2key(keyname)) < 0)
		return -1;

	memset(&clear_mapping, 0, sizeof(Mapping));

	bindmapping(editor, key, clear_mapping);
	return 0;
}

void
exit_rawmode(void)
{
	if(editor->rawmode) {
		write(editor->ofd, "\r\n", 2);
		rawmode_off(editor);
	}
}

// clang-format off
EditorFnDef editfndefs[] = {
	[FnInvalid] = (EditorFnDef) {
		.name = "%esmle:Invalid",
		.map = (Mapping){nil, nil, nil, 0 ,0 ,0 ,0 ,0},
	},
	[FnEsFunction] = (EditorFnDef) {
		.name = "%esmle:EsFunction",
		.map = (Mapping){
			.hook = nil,
			.base_hook = nil,
			.aux = nil,
			.breakkey = 0,
			.reset_completion = 0,
			.end_of_file = 0,
		},
	},
	[FnEndInput] = (EditorFnDef) {
		.name = "%esmle:EndInput",
		.map = (Mapping){
			.input_finished = 1,
		},
	},
	[FnPassKey] = (EditorFnDef) {
		.name = "%esmle:PassKey",
		.map = (Mapping){
			.hook = &pass_key,
			.reset_completion = 1,
		},
	},
	[FnBreak] = (EditorFnDef) {
		.name = "%esmle:Break",
		.map = (Mapping){
			.breakkey = 1,
		},
	},
	[FnDelete] = (EditorFnDef) {
		.name = "%esmle:Delete",
		.map = (Mapping) {
			.base_hook = &delete_char,
			.reset_completion = 1,
		},
	},
	[FnDeleteOrEOF] = (EditorFnDef) {
		.name = "%esmle:DeleteOrEOF",
		.map = (Mapping){
			.base_hook = &delete_char,
			.reset_completion = 1,
			.eof_if_empty = 1,
		},
	},
	[FnEOF] = (EditorFnDef) {
		.name = "%esmle:EOF",
		.map = (Mapping){
			.end_of_file = 1,
		},
	},
	[FnBackspace] = (EditorFnDef) {
		.name = "%esmle:Backspace",
		.map = (Mapping){
			.base_hook = &backspace_char,
			.reset_completion = 1,
		},
	},
	[FnNextCompletion] = (EditorFnDef) {
		.name = "%esmle:NextCompletion",
		.map = (Mapping){
			.base_hook = &completion_next,
		},
	},
	[FnPrevCompletion] = (EditorFnDef) {
		.name = "%esmle:PrevCompletion",
		.map = (Mapping){
			.base_hook = &completion_prev,
		},
	},
	[FnCursorHome] = (EditorFnDef) {
		.name = "%esmle:CursorHome",
		.map = (Mapping){
			.hook = nil,
			.base_hook = &cursor_home,
			.reset_completion = 2,
		},
	},
	[FnCursorEnd] = (EditorFnDef) {
		.name = "%esmle:CursorEnd",
		.map = (Mapping){
			.hook = nil,
			.base_hook = &cursor_end,
			.reset_completion = 2,
		},
	},
	[FnDeleteWord] = (EditorFnDef) {
		.name = "%esmle:DeleteWord",
		.map = (Mapping){
			.hook = nil,
			.base_hook = &delete_word,
			.reset_completion = 1,
		},
	},
	[FnDeleteToStart] = (EditorFnDef) {
		.name = "%esmle:DeleteToStart",
		.map = (Mapping){
			.hook = nil,
			.base_hook = &delete_to_start,
			.reset_completion = 1,
		},
	},
	[FnDeleteToEnd] = (EditorFnDef) {
		.name = "%esmle:DeleteToEnd",
		.map = (Mapping){
			.hook = nil,
			.base_hook = &delete_to_end,
			.reset_completion = 1,
		},
	},
	[FnClearScreen] = (EditorFnDef) {
		.name = "%esmle:ClearScreen",
		.map = (Mapping){
			.hook = nil,
			.base_hook = &clear_screen,
		},
	},
	[FnNextHistory] = (EditorFnDef) {
		.name = "%esmle:NextHistory",
		.map = (Mapping) {
			.base_hook = &history_next,
			.reset_completion = 1,
		},
	},
	[FnPrevHistory] = (EditorFnDef) {
		.name = "%esmle:PrevHistory",
		.map = (Mapping) {
			.base_hook = &history_prev,
			.reset_completion = 1,
		},
	},
	[FnCursorLeft] = (EditorFnDef) {
		.name = "%esmle:CursorLeft",
		.map = (Mapping){
			.base_hook = &cursor_move_left,
			.reset_completion = 2,
		},
	},
	[FnCursorRight] = (EditorFnDef) {
		.name = "%esmle:CursorRight",
		.map = (Mapping){
			.base_hook = &cursor_move_right,
			.reset_completion = 2,
		},
	},
	[FnCursorWordLeft] = (EditorFnDef) {
		.name = "%esmle:CursorWordLeft",
		.map = (Mapping){
			.base_hook = &cursor_move_word_left,
			.reset_completion = 1,
		},
	},
	[FnCursorWordRight] = (EditorFnDef) {
		.name = "%esmle:CursorWordRight",
		.map = (Mapping){
			.base_hook = &cursor_move_word_right,
			.reset_completion = 1,
		},
	},
};
// clang-format on

EditorFunction
name2edfn(char *name)
{
	int i = 0;

	for(i = 0; i <= FnCursorWordRight; i++){
		if(streq(name, editfndefs[i].name))
			return i;
	}

	return FnInvalid;
}

char*
edfn2name(EditorFunction edfn)
{
	return editfndefs[edfn].name;
}

Mapping
edfn2mapping(EditorFunction edfn)
{
	return editfndefs[edfn].map;
}

EditorFunction
mapping2edfn(Mapping map)
{
	int i = 0;

	if(map.hook == &line_editor_hook)
		return FnEsFunction;

	for(i = 0; i <= FnCursorWordRight; i++){
		if(map.hook != editfndefs[i].map.hook)
			continue;
		if(map.base_hook != editfndefs[i].map.base_hook)
			continue;
		if(map.aux != editfndefs[i].map.aux)
			continue;
		if(map.breakkey != editfndefs[i].map.breakkey)
			continue;
		if(map.reset_completion != editfndefs[i].map.reset_completion)
			continue;
		if(map.end_of_file != editfndefs[i].map.end_of_file)
			continue;
		if(map.eof_if_empty != editfndefs[i].map.eof_if_empty)
			continue;
		if(map.input_finished != editfndefs[i].map.input_finished)
			continue;
		return i;
	}

	return FnInvalid;
}

char*
mapping2name(Mapping map)
{
	EditorFunction fn;

	fn = mapping2edfn(map);

	if(fn == FnEsFunction)
		return map.aux;
	return editfndefs[fn].name;
}

Mapping
name2mapping(char *name)
{
	EditorFunction fn;

	fn = name2edfn(name);
	return edfn2mapping(fn);
}

int
bind_base_function(char *keyname, char *function)
{
	int key = 0;
	EditorFunction fn;
	Mapping basemap;

	if((key = name2key(keyname)) < 0)
		return -1;

	fn = name2edfn(function);
	if(fn == FnInvalid || fn == FnEsFunction)
		return -2;

	basemap = edfn2mapping(fn);
	bindmapping(editor, key, basemap);

	return 0;
}

/*
 * initialization
 */

/* initinput -- called at dawn of time from main() */
extern void
initinput(void)
{
	input = NULL;

	/* declare the global roots */
	globalroot(&history); /* history file */
	globalroot(&error);	  /* parse errors */
	globalroot(&prompt1); /* main prompt */
	globalroot(&prompt2); /* secondary prompt */

	/* mark the historyfd as a file descriptor to hold back from forked children */
	registerfd(&historyfd, TRUE);

	/* call the parser's initialization */
	initparse();

	editor = ealloc(sizeof(EditorState));
	if(initialize_editor(editor, 0, 1) == 0) {
		editor_debugging(editor, editor_debugfd);
		set_prompt1(editor, "% ");
		set_prompt2(editor, "_% ");
		set_complete_hook(editor, &es_complete_hook);
		editor->wordbreaks = " \t\n\\'`$><=;|&{()}";
		editor->prefixes = "$";
		editor->sort_completions = 1;
		editor->remove_duplicates = 1;
		editor->match_braces = 0;
		default_keymap = ealloc(sizeof(Keymap));
		memcpy(default_keymap, editor->keymap, sizeof(Keymap));
	}
}
