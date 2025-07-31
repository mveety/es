/* main.c -- initialization for es ($Revision: 1.3 $) */

#include "es.h"
#include "gc.h"
#include <stdio.h>
// #include "token.h"

Boolean gcverbose	= FALSE;	/* -G */
Boolean gcinfo		= FALSE;	/* -I */
Boolean assertions = FALSE;		/* -A */
Boolean verbose_parser = FALSE; /* -P */
Boolean use_initialize_esrc = TRUE; /* -Dr */
volatile Boolean loginshell = FALSE; /* -l or $0[0] == '-' */
volatile Boolean readesrc = TRUE;
Boolean different_esrc = FALSE; /* -I */
Boolean additional_esrc = FALSE; /* -A */

/* #if 0 && !HPUX && !defined(linux) && !defined(sgi) */
/* extern int getopt (int argc, char **argv, const char *optstring); */
/* #endif */

extern int optind;
extern char *optarg;
extern int yydebug;
extern size_t blocksize;

/* extern int isatty(int fd); */
extern char **environ;

/* for -I */
char *altesrc = NULL;
char *extraesrc = NULL;

/* checkfd -- open /dev/null on an fd if it is closed */
static void checkfd(int fd, OpenKind r) {
	int new;
	new = dup(fd);
	if (new != -1)
		close(new);
	else if (errno == EBADF && (new = eopen("/dev/null", r)) != -1)
		mvfd(new, fd);
}

/* initpath -- set $path based on the configuration default */
static void initpath(void) {
	int i;
	static const char * const path[] = { INITIAL_PATH };
	
	Ref(List *, list, NULL);
	for (i = arraysize(path); i-- > 0;) {
		Term *t = mkstr((char *) path[i]);
		list = mklist(t, list);
	}
	vardef("path", NULL, list);
	RefEnd(list);
}

static void init_internal_vars(void) {
	char *loginshell_st = loginshell ? "true" : "false";
	char *initialize_st = use_initialize_esrc ? "true" : "false";
	char *readesrc_st = readesrc ? "true" : "false";
	char *diffesrc_st = different_esrc ? "true" : "false";
	char *esrcfile = different_esrc ? str("%s", altesrc) : "";
	char *addesrc_st = additional_esrc ? "true" : "false" ;
	char *addesrcfile = additional_esrc ? str("%s", extraesrc) : "" ;

	vardef("pid", NULL, mklist(mkstr(str("%d", getpid())), NULL));
	vardef("__es_loginshell", NULL, mklist(mkstr(str("%s", loginshell_st)), NULL));
	vardef("__es_initialize_esrc", NULL, mklist(mkstr(str("%s", initialize_st)), NULL));
	vardef("__es_readesrc", NULL, mklist(mkstr(str("%s", readesrc_st)), NULL));
	vardef("__es_different_esrc", NULL, mklist(mkstr(str("%s", diffesrc_st)), NULL));
	vardef("__es_esrcfile", NULL, mklist(mkstr(esrcfile), NULL));
	vardef("__es_extra_esrc", NULL, mklist(mkstr(str("%s", addesrc_st)), NULL));
	vardef("__es_extra_esrcfile", NULL, mklist(mkstr(addesrcfile), NULL));

}

/* runesrc -- run the user's profile, if it exists */
static void runesrc(void) {
	char *esrc;
	int fd;

	esrc = different_esrc ? altesrc : str("%L/.esrc", varlookup("home", NULL), "\001");
	fd = eopen(esrc, oOpen);
	if (fd != -1) {
		ExceptionHandler
			runfd(fd, esrc, 0);
		CatchException (e)
			if (termeq(e->term, "exit"))
				exit(exitstatus(e->next));
			else if (termeq(e->term, "error"))
				eprint("%L\n",
				       e->next == NULL ? NULL : e->next->next,
				       " ");
			else if (!issilentsignal(e))
				eprint("uncaught exception: %L\n", e, " ");
			return;
		EndExceptionHandler
	}
}

static int
runinitialize(void) {
	List *initialize;
	List *result; Root r_result;

	result = NULL;

	initialize = varlookup("fn-%initialize", NULL);
	if(initialize == NULL)
		return 0;

	gcref(&r_result, (void**)&result);

	ExceptionHandler
		result = eval(initialize, NULL, 0);
	CatchException(e)
		if (termeq(e->term, "exit"))
			exit(exitstatus(e->next));
		else if (termeq(e->term, "error")){
			eprint("init handler: %L\n", e, " ");
			exit(-1);
		} else if(!issilentsignal(e))
			eprint("init handler: uncaught exception: %L\n", e, " ");
	EndExceptionHandler
	
	gcderef(&r_result, (void**)&result);
	return 0;
}

/* usage -- print usage message and die */
static noreturn usage(void) {
	eprint(
		"usage: es [-c command] [-siIAleVxXnNpodvgSCB] [-D flags] [-r flags] [file [args ...]]\n"
		"	-c	cmd execute argument\n"
		"	-s	read commands from standard input; stop option parsing\n"
		"	-i	interactive shell\n"
		"	-I file	alternative init file\n"
		"	-A file	additional init file\n"
		"	-l	login shell\n"
		"	-e	exit if any command exits with false status\n"
		"	-V	print input to standard error\n"
		"	-x	print commands to standard error before executing\n"
		"	-n	just parse; don't execute\n"
		"	-N	ignore the .esrc\n"
		"	-p	don't load functions from the environment\n"
		"	-o	don't open stdin, stdout, and stderr if they were closed\n"
		"	-d	don't ignore SIGQUIT or SIGTERM\n"
		"	-X	use experimental gc\n"
		"	-v	print version\n"
		"	-g n	(new gc) collection frequency\n"
		"	-S n	(new gc) freelist sort frequency\n"
		"	-C n	(new gc) freelist coalesce frequency\n"
		"	-B n	(new gc) block size in megabytes\n"
		"	-D flags	debug flags (? for more info)\n"
		"	-r flags	run flags (? for more info)\n"
	);
	exit(1);
}

void
print_version(void)
{
	eprint("%s\n", buildstring);
	exit(0);
}

extern int gc_after;
extern int gc_sort_after_n;
extern int gc_coalesce_after_n;

void
do_usage(void)
{
	initgc();
	initconv();
	usage();
}

void
debug_flag_usage(void)
{
	dprintf(2, "debug flags: es -D [GIaEP]\n%s%s%s%s%s%s%s",
		"	? -- show this message\n",
		"	G -- gcverbose\n",
		"	I -- gcinfo\n",
		"	a -- assertions\n",
		"	E -- debug_exceptions\n",
		"	P -- verbose_parser\n",
		"	r -- run esrc from C instead of from %initialize\n"
	);
	exit(1);
}

void
run_flag_usage(void)
{
	dprintf(2, "run flags: es -r [einVxL]\n%s%s%s%s%s%s%s%s",
		"	? -- show this message\n",
		"	e -- exitonfalse\n",
		"	i -- interactive\n",
		"	n -- noexec\n",
		"	v -- echoinput\n",
		"	x -- printcmds\n",
		"	L -- lisptrees\n",
		"	a -- assertions\n"
	);
	exit(1);
}

/* main -- initialize, parse command arguments, and start running */
int main(int argc, char **argv) {
	int c, fd;
	volatile int ac;
	char **volatile av;
	char *ds;
	char *rs;
	char *file;

	volatile int runflags = 0;		/* -[einvxL] */
	volatile Boolean protected = FALSE;	/* -p */
	volatile Boolean allowquit = FALSE;	/* -d */
	volatile Boolean cmd_stdin = FALSE;		/* -s */
	Boolean keepclosed = FALSE;		/* -o */
	const char *volatile cmd = NULL;	/* -c */

	if (argc == 0) {
		argc = 1;
		argv = ealloc(2 * sizeof (char *));
		argv[0] = "es";
		argv[1] = NULL;
	}
	if (*argv[0] == '-')
		loginshell = TRUE;

	/* yydebug = 1; */

	// removed IGAPL
	while ((c = getopt(argc, argv, "+eiI:A:lxXvnpodsVc:?hNg:S:C:B:D:r:")) != EOF)
		switch (c) {
		case 'D':
			for(ds = optarg; *ds != 0; ds++){
				switch(*ds){
				case 'G': gcverbose = TRUE; break;
				case 'I': gcinfo = TRUE; break;
				case 'a': assertions = TRUE; break;
				case 'E': debug_exceptions = TRUE; break;
				case 'P': verbose_parser = TRUE; break;
				case 'r': use_initialize_esrc = FALSE; break;
				case '?':
					debug_flag_usage();
					break;
				default:
					dprintf(2, "error: invalid debug flag: %c\n", *ds);
					debug_flag_usage();
					break;
				}
			}
			break;
		case 'r':
			for(rs = optarg; *rs != 0; rs++){
				switch (*rs) {
				case 'e': runflags |= eval_exitonfalse; break;
				case 'i': runflags |= run_interactive; break;
				case 'n': runflags |= run_noexec; break;
				case 'v': runflags |= run_echoinput; break;
				case 'x': runflags |= run_printcmds; break;
				case 'L': runflags |= run_lisptrees; break;
				case 'a': assertions = TRUE; break;
				case '?':
					run_flag_usage();
					break;
				default:
					dprintf(2, "error: invalid run flag: %c\n", *rs);
					run_flag_usage();
					break;
				}
			}
			break;
		case 'I':
			different_esrc = TRUE;
			altesrc = strdup(optarg);
			break;
		case 'A':
			additional_esrc = TRUE;
			extraesrc = strdup(optarg);
			break;
		case 'c': cmd = optarg; break;
		case 'e': runflags |= eval_exitonfalse; break;
		case 'i': runflags |= run_interactive; break;
		case 'n': runflags |= run_noexec; break;
		case 'N': readesrc = FALSE; break;
		case 'V': runflags |= run_echoinput; break;
		case 'x': runflags |= run_printcmds; break;
		case 'l': loginshell = TRUE; break;
		case 'p': protected = TRUE; break;
		case 'o': keepclosed = TRUE; break;
		case 'd': allowquit = TRUE; break;
		case 's': cmd_stdin = TRUE; goto getopt_done;
		case 'X': gctype = NewGc; break;
		case 'g':
			gc_after = atoi(optarg);
			break;
		case 'S':
			gc_sort_after_n = atoi(optarg);
			break;
		case 'C':
			gc_coalesce_after_n = atoi(optarg);
			break;
		case 'B':
			blocksize = strtoul(optarg, NULL, 10);
			if(blocksize == 0)
				do_usage();
			blocksize *= 1024*1024;
			if(blocksize < MIN_minspace){
				dprintf(2, "error: blocksize < %d\n", (MIN_minspace/1024));
				do_usage();
			}
			break;
		case 'v':
			initgc();
			initconv();
			print_version();
			break;
		case 'h':
		default:
			do_usage();
			break;
		}


getopt_done:
	initgc();
	initconv();

	if (cmd_stdin && cmd != NULL) {
		eprint("es: -s and -c are incompatible\n");
		exit(1);
	}

	if (!keepclosed) {
		checkfd(0, oOpen);
		checkfd(1, oCreate);
		checkfd(2, oCreate);
	}

	if (cmd == NULL &&
		(optind == argc || cmd_stdin) &&
		(runflags & run_interactive) == 0 &&
		isatty(0))
		runflags |= run_interactive;

	ac = argc;
	av = argv;

	ExceptionHandler
		roothandler = &_localhandler;	/* unhygeinic */

		initinput();
		initprims();
		initvars();
	
		runinitial();
	
		initpath(); /* $path */
		init_internal_vars(); /* $pid, $__es_loginshell, $__es_initialize_esrc, $__es_readesrc */
		initsignals(runflags & run_interactive, allowquit);
		hidevariables();
		initenv(environ, protected);

		runinitialize();

		if (loginshell && !use_initialize_esrc)
			if(readesrc)
				runesrc();
	
		if (cmd == NULL && !cmd_stdin && optind < ac) {
			file = av[optind++];
			if ((fd = eopen(file, oOpen)) == -1) {
				eprint("%s: %s\n", file, esstrerror(errno));
				return 1;
			}
			vardef("*", NULL, listify(ac - optind, av + optind));
			vardef("0", NULL, mklist(mkstr(file), NULL));
			return exitstatus(runfd(fd, file, runflags));
		}
	
		vardef("*", NULL, listify(ac - optind, av + optind));
		vardef("0", NULL, mklist(mkstr(av[0]), NULL));
		if (cmd != NULL)
			return exitstatus(runstring(cmd, NULL, runflags));
		return exitstatus(runfd(0, "stdin", runflags));

	CatchException (e)

		if (termeq(e->term, "exit"))
			return exitstatus(e->next);
		else if (termeq(e->term, "error")) {
			eprint("root handler: %L\n", e, " ");
		} else if (!issilentsignal(e))
			eprint("root handler: uncaught exception: %L\n", e, " ");
		return 1;

	EndExceptionHandler
}
