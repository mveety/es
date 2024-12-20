/* input.c -- read input from files or strings ($Revision: 1.2 $) */
/* stdgetenv is based on the FreeBSD getenv */

#include "es.h"
#include "input.h"
#include <stdio.h> /* needed for dprintf */

#if READLINE
#include <readline/readline.h>
#endif

/*
 * constants
 */

#define	BUFSIZE		((size_t) 4096)		/* buffer size to fill reads into */


/*
 * macros
 */

#define	ISEOF(in)	((in)->fill == eoffill)


/*
 * globals
 */

Input *input;
char *prompt, *prompt2;

Boolean disablehistory = FALSE;
Boolean resetterminal = FALSE;
static char *history;
char *lastcmd, *nextlastcmd;
static int historyfd = -1;

#if READLINE
int rl_meta_chars;	/* for editline; ignored for gnu readline */
extern void add_history(char *);
/*
extern char *readline(char *);
extern void rl_reset_terminal(char *);
extern char *rl_basic_word_break_characters;
extern char *rl_completer_quote_characters;
extern char *rl_readline_name;
extern char *rl_line_buf;
*/

#if ABUSED_GETENV
static char *stdgetenv(const char *);
static char *esgetenv(const char *);
static char *(*realgetenv)(const char *) = stdgetenv;
#endif
#endif


/*
 * errors and warnings
 */

/* locate -- identify where an error came from */
static char *locate(Input *in, char *s) {
	return (in->runflags & run_interactive)
		? s
		: str("%s:%d: %s", in->name, in->lineno, s);
}

static char *error = NULL;

/* yyerror -- yacc error entry point */
extern void yyerror(char *s) {
#if sgi
	/* this is so that trip.es works */
	if (streq(s, "Syntax error"))
		s = "syntax error";
#endif
	if (error == NULL)	/* first error is generally the most informative */
		error = locate(input, s);
}

/* warn -- print a warning */
static void warn(char *s) {
	eprint("warning: %s\n", locate(input, s));
}


/*
 * history
 */

/* loghistory -- write the last command out to a file */
static void loghistory(const char *cmd, size_t len) {
	const char *s, *end;
	unsigned long curtime;

	if (history == NULL || disablehistory)
		return;
	if(strnlen(cmd, len) > 1){
		if (lastcmd != NULL)
			efree(lastcmd);
		if (nextlastcmd != NULL)
			lastcmd = nextlastcmd;
		nextlastcmd = ealloc(len+2);
		memcpy(nextlastcmd, cmd, len);
		nextlastcmd[len-1] = '\0';
	}
	if (historyfd == -1) {
		historyfd = eopen(history, oAppend);
		if (historyfd == -1) {
			eprint("history(%s): %s\n", history, esstrerror(errno));
			vardef("history", NULL, NULL);
			return;
		}
	}
	/* skip empty lines and comments in history */
	for (s = cmd, end = s + len; s < end; s++)
		switch (*s) {
		case '#': case '\n':	return;
		case ' ': case '\t':	break;
		default:		goto writeit;
		}
	writeit:
		;

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
extern void sethistory(char *file) {
	if (historyfd != -1) {
		close(historyfd);
		historyfd = -1;
	}
	history = file;
}


/*
 * unget -- character pushback
 */

/* ungetfill -- input->fill routine for ungotten characters */
static int ungetfill(Input *in) {
	int c;
	assert(in->ungot > 0);
	c = in->unget[--in->ungot];
	if (in->ungot == 0) {
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
extern void unget(Input *in, int c) {
	if (in->ungot > 0) {
		assert(in->ungot < MAXUNGET);
		in->unget[in->ungot++] = c;
	} else if (in->bufbegin < in->buf && in->buf[-1] == c && (input->runflags & run_echoinput) == 0)
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
static int get(Input *in) {
	int c;
	while ((c = (in->buf < in->bufend ? *in->buf++ : (*in->fill)(in))) == '\0')
		warn("null character ignored");
	return c;
}

/* getverbose -- get a character, print it to standard error */
static int getverbose(Input *in) {
	if (in->fill == ungetfill)
		return get(in);
	else {
		int c = get(in);
		if (c != EOF) {
			char buf = c;
			ewrite(2, &buf, 1);
		}
		return c;
	}
}

/* eoffill -- report eof when called to fill input buffer */
static int eoffill(Input *in) {
	assert(in->fd == -1);
	return EOF;
}

#if READLINE
/* callreadline -- readline wrapper */
static char *
callreadline(char *prompt) {
	char *r;

	if (prompt == NULL)
		prompt = ""; /* bug fix for readline 2.0 */

	if (resetterminal) {
		rl_reset_terminal(NULL);
		resetterminal = FALSE;
	}
	interrupted = FALSE;
	if (!setjmp(slowlabel)) {
		slow = TRUE;
		r = interrupted ? NULL : readline(prompt);
	} else
		r = NULL;
	slow = FALSE;
	if (r == NULL)
		errno = EINTR;
	SIGCHK();
	return r;
}

#if ABUSED_GETENV

/* getenv -- fake version of getenv for readline (or other libraries) */
static char *esgetenv(const char *name) {
	List *value = varlookup(name, NULL);
	if (value == NULL)
		return NULL;
	else { 
		char *export;
		static Dict *envdict;
		static Boolean initialized = FALSE;
		Ref(char *, string, NULL);

		gcdisable();
		if (!initialized) {
			initialized = TRUE;
			envdict = mkdict();
			globalroot(&envdict);
		}

		string = dictget(envdict, name);
		if (string != NULL)
			efree(string);

		export = str("%W", value);
		string = ealloc(strlen(export) + 1);
		strcpy(string, export);
		envdict = dictput(envdict, (char *) name, string);

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

	if (name == NULL || environ == NULL)
		return (NULL);
	for (np = name; *np && *np != '='; ++np)
		continue;
	len = np - name;
	for (p = environ; (c = *p) != NULL; ++p)
		if (strncmp(c, name, len) == 0 && c[len] == '=') {
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

#endif	/* READLINE */

#if READLINE

volatile int es_rl_start;
volatile int es_rl_end;

char*
run_es_completer(const char *text, int state)
{
	List *args;
	List *completer;
	char *res;

	Ref(List *, result, NULL);

	gcenable();
	assert(!gcisblocked());

	completer = varlookup("fn-%core_completer", NULL);
	if(completer == NULL)
		return strdup(text);
	args = mklist(mkstr(str("%s", rl_line_buffer)),
			mklist(mkstr(str("%s", text)),
				mklist(mkstr(str("%d", es_rl_start)),
					mklist(mkstr(str("%d", es_rl_end)),
						mklist(mkstr(str("%d", state)), NULL)))));
	completer = append(completer, args);
	result = eval(completer, NULL, 0);

	assert(result != NULL);

	res = strdup(getstr(result->term));

	gcdisable();
	if(strcmp("%%end_complete", res) == 0){
		free(res);
		res = NULL;
	}

	RefEnd(result);

	return res;
}

char**
es_complete_hook(const char *text, int start, int end)
{
	List *completer;

	completer = varlookup("fn-%core_completer", NULL);
	if(completer == NULL)
		return NULL;
	rl_attempted_completion_over = 1;
	es_rl_start = start;
	es_rl_end = end;
	return rl_completion_matches(text, run_es_completer);
}

int
is_whitespace(char c){
	switch(c){
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

	for(i = 0; s[i] != 0 && i < len; i++){
		switch(state){
		case 0:
			if(i == 0 && s[i] == '!')
				state = 2;
			else if(s[i] == ' ')
				state = 1;
			else if(s[i] == '\'')
				state = 4;
			break;
		case 1:
			if(s[i] == '!'){
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
	if(nextlastcmd != NULL){
		if((bbpos = containsbangbang(linebuf, strlen(linebuf))) >= 0)
			nwrote += (strlen(nextlastcmd) - 2); /* remember we replaced the !! with nextlastcmd */
	}
	if (in->buflen < (unsigned int)nwrote+1){
		while(in->buflen < (unsigned int)nwrote+1)
			in->buflen *= 2;
		in->bufbegin = erealloc(in->bufbegin, in->buflen);
	}
	if(bbpos < 0){
		memcpy(in->bufbegin, linebuf, nwrote);
		return nwrote;
	}
	lbi = 0;
	inbufi = 0;
	lastcmdlen = strlen(nextlastcmd);
	while(lbi < nread){
		if(lbi == bbpos){
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

#endif

/* fdfill -- fill input buffer by reading from a file descriptor */
static int fdfill(Input *in) {
	long nread;
	assert(in->buf == in->bufend);
	assert(in->fd >= 0);

#if READLINE
	if (in->runflags & run_interactive && in->fd == 0) {
		char *rlinebuf = callreadline(prompt);
		if (rlinebuf == NULL)
			nread = 0;
		else {
			if (*rlinebuf != '\0')
				add_history(rlinebuf);
			nread = copybuffer(in, rlinebuf, strlen(rlinebuf))+1;
			in->bufbegin[nread-1] = '\n';
			efree(rlinebuf);
		}
	} else
#endif
		do {
			nread = eread(in->fd, (char *) in->bufbegin, in->buflen);
			SIGCHK();
		} while (nread == -1 && errno == EINTR);

	if (nread <= 0) {
		close(in->fd);
		in->fd = -1;
		in->fill = eoffill;
		in->runflags &= ~run_interactive;
		if (nread == -1)
			fail("$&parse", "%s: %s", in->name == NULL ? "es" : in->name, esstrerror(errno));
		return EOF;
	}

	if (in->runflags & run_interactive)
		loghistory((char *) in->bufbegin, nread);

	in->buf = in->bufbegin;
	in->bufend = &in->buf[nread];
	return *in->buf++;
}


/*
 * the input loop
 */

/* parse -- call yyparse(), but disable garbage collection and catch errors */
extern Tree *parse(char *pr1, char *pr2) {
	int result;
	assert(error == NULL);

	inityy();
	emptyherequeue();

	if (ISEOF(input))
		throw(mklist(mkstr("eof"), NULL));

#if READLINE
	prompt = (pr1 == NULL) ? "" : pr1;
#else
	if (pr1 != NULL)
		eprint("%s", pr1);
#endif
	prompt2 = pr2;

	gcreserve(300 * sizeof (Tree));
	gcdisable();
	result = yyparse();
	gcenable();

	if (result || error != NULL) {
		char *e;
		assert(error != NULL);
		e = error;
		error = NULL;
		fail("$&parse", "%s", e);
	}
#if LISPTREES
	if (input->runflags & run_lisptrees)
		eprint("%B\n", parsetree);
#endif
	return parsetree;
}

/* resetparser -- clear parser errors in the signal handler */
extern void resetparser(void) {
	error = NULL;
}

/* runinput -- run from an input source */
extern List *runinput(Input *in, int runflags) {
	volatile int flags = runflags;
	List * volatile result = NULL;
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

	ExceptionHandler

		dispatch
	          = varlookup(dispatcher[((flags & run_printcmds) ? 1 : 0)
					 + ((flags & run_noexec) ? 2 : 0)],
			      NULL);
		if (flags & eval_exitonfalse)
			dispatch = mklist(mkstr("%exit-on-false"), dispatch);
		varpush(&push, "fn-%dispatch", dispatch);
	
		repl = varlookup((flags & run_interactive)
				   ? "fn-%interactive-loop"
				   : "fn-%batch-loop",
				 NULL);
		result = (repl == NULL)
				? prim("batchloop", NULL, NULL, flags)
				: eval(repl, NULL, flags);
	
		varpop(&push);

	CatchException (e)

		(*input->cleanup)(input);
		input = input->prev;
		throw(e);

	EndExceptionHandler

	input = in->prev;
	(*in->cleanup)(in);
	return result;
}


/*
 * pushing new input sources
 */

/* fdcleanup -- cleanup after running from a file descriptor */
static void fdcleanup(Input *in) {
	unregisterfd(&in->fd);
	if (in->fd != -1)
		close(in->fd);
	efree(in->bufbegin);
}

/* runfd -- run commands from a file descriptor */
extern List *runfd(int fd, const char *name, int flags) {
	Input in;
	List *result;

	memzero(&in, sizeof (Input));
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
static void stringcleanup(Input *in) {
	efree(in->bufbegin);
}

/* stringfill -- placeholder than turns into EOF right away */
static int stringfill(Input *in) {
	in->fill = eoffill;
	return EOF;
}

/* runstring -- run commands from a string */
extern List *runstring(const char *str, const char *name, int flags) {
	Input in;
	List *result;
	unsigned char *buf;

	assert(str != NULL);

	memzero(&in, sizeof (Input));
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
extern Tree *parseinput(Input *in) {
	Tree * volatile result = NULL;

	in->prev = input;
	in->runflags = 0;
	in->get = get;
	input = in;

	ExceptionHandler
		result = parse(NULL, NULL);
		if (get(in) != EOF)
			fail("$&parse", "more than one value in term");
	CatchException (e)
		(*input->cleanup)(input);
		input = input->prev;
		throw(e);
	EndExceptionHandler

	input = in->prev;
	(*in->cleanup)(in);
	return result;
}

/* parsestring -- turn a string into a tree; must be exactly one tree */
extern Tree *parsestring(const char *str) {
	Input in;
	Tree *result;
	unsigned char *buf;

	assert(str != NULL);

	/* TODO: abstract out common code with runstring */

	memzero(&in, sizeof (Input));
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
extern Boolean isinteractive(void) {
	return input == NULL ? FALSE : ((input->runflags & run_interactive) != 0);
}


/*
 * initialization
 */

/* initinput -- called at dawn of time from main() */
extern void initinput(void) {
	input = NULL;

	/* declare the global roots */
	globalroot(&history);		/* history file */
	globalroot(&error);		/* parse errors */
	globalroot(&prompt);		/* main prompt */
	globalroot(&prompt2);		/* secondary prompt */

	/* mark the historyfd as a file descriptor to hold back from forked children */
	registerfd(&historyfd, TRUE);

	/* call the parser's initialization */
	initparse();

#if READLINE
	rl_meta_chars = 0;
	rl_basic_word_break_characters = " \t\n\\'`$><=;|&{()}";
	rl_special_prefixes = "$";
	rl_completer_quote_characters = "'";
	rl_readline_name = "es-mveety";
	rl_attempted_completion_function = es_complete_hook;
#endif
}
