/* main.c -- initialization for es ($Revision: 1.3 $) */

#include "es.h"
#include "gc.h"

Boolean gcverbose	= FALSE;	/* -G */
Boolean gcinfo		= FALSE;	/* -I */
Boolean assertions = FALSE;		/* -A */

/* #if 0 && !HPUX && !defined(linux) && !defined(sgi) */
/* extern int getopt (int argc, char **argv, const char *optstring); */
/* #endif */

extern int optind;
extern char *optarg;
extern int yydebug;

/* extern int isatty(int fd); */
extern char **environ;


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

/* initpid -- set $pid for this shell */
static void initpid(void) {
	vardef("pid", NULL, mklist(mkstr(str("%d", getpid())), NULL));
}

/* runesrc -- run the user's profile, if it exists */
static void runesrc(void) {
	char *esrc = str("%L/.esrc", varlookup("home", NULL), "\001");
	int fd = eopen(esrc, oOpen);
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

/* usage -- print usage message and die */
static noreturn usage(void) {
	eprint(
		"usage: es [-c command] [-silevxnpoIGL] [file [args ...]]\n"
		"	-c cmd execute argument\n"
		"	-s	read commands from standard input; stop option parsing\n"
		"	-i	interactive shell\n"
		"	-l	login shell\n"
		"	-e	exit if any command exits with false status\n"
		"	-V	print input to standard error\n"
		"	-x	print commands to standard error before executing\n"
		"	-n	just parse; don't execute\n"
		"	-N	ignore the .esrc\n"
		"	-p	don't load functions from the environment\n"
		"	-o	don't open stdin, stdout, and stderr if they were closed\n"
		"	-d	don't ignore SIGQUIT or SIGTERM\n"
		"	-I	print garbage collector information\n"
		"	-G	print verbose garbage collector information\n"
		"	-X	use experimental gc\n"
		"	-L	print parser results in LISP format\n"
		"	-A	enable assertions (slow)\n"
		"	-v	print version\n"
		"	-P	set yydebug = 1\n"
		"	-g n	(new gc) collection frequency\n"
		"	-S n	(new gc) freelist sort frequency\n"
		"	-C n	(new gc) freelist coalesce frequency\n"
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

/* main -- initialize, parse command arguments, and start running */
int main(int argc, char **argv) {
	int c;
	volatile int ac;
	char **volatile av;

	volatile int runflags = 0;		/* -[einvxL] */
	volatile Boolean protected = FALSE;	/* -p */
	volatile Boolean allowquit = FALSE;	/* -d */
	volatile Boolean cmd_stdin = FALSE;		/* -s */
	volatile Boolean loginshell = TRUE;	/* -l or $0[0] == '-' */
	volatile Boolean readesrc = TRUE;
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

	while ((c = getopt(argc, argv, "eilxXvnpPodsAVc:?GILNg:S:C:")) != EOF)
		switch (c) {
		case 'c':	cmd = optarg;			break;
		case 'e':	runflags |= eval_exitonfalse;	break;
		case 'i':	runflags |= run_interactive;	break;
		case 'n':	runflags |= run_noexec;		break;
		case 'N':	readesrc = FALSE; break;
		case 'V':	runflags |= run_echoinput;	break;
		case 'x':	runflags |= run_printcmds;	break;
		case 'L':	runflags |= run_lisptrees;	break;
		case 'l':	loginshell = TRUE;		break;
		case 'p':	protected = TRUE;		break;
		case 'o':	keepclosed = TRUE;		break;
		case 'd':	allowquit = TRUE;		break;
		case 's':	cmd_stdin = TRUE;			goto getopt_done;
		case 'G':	gcverbose = TRUE;		break;
		case 'I':	gcinfo = TRUE;			break;
		case 'X':	gctype = NewGc;			break;
		case 'A':	assertions = TRUE;		break;
		case 'P':	yydebug = 1; break;
		case 'g':
			gc_after = atoi(optarg);
			break;
		case 'S':
			gc_sort_after_n = atoi(optarg);
			break;
		case 'C':
			gc_coalesce_after_n = atoi(optarg);
			break;
		case 'v':
			initgc();
			initconv();
			print_version();
			break;
		default:
			initgc();
			initconv();
			usage();
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

	if (
		cmd == NULL
	     && (optind == argc || cmd_stdin)
	     && (runflags & run_interactive) == 0
	     && isatty(0)
	)
		runflags |= run_interactive;

	ac = argc;
	av = argv;

	ExceptionHandler
		roothandler = &_localhandler;	/* unhygeinic */

		initinput();
		initprims();
		initvars();
	
		runinitial();
	
		initpath();
		initpid();
		initsignals(runflags & run_interactive, allowquit);
		hidevariables();
		initenv(environ, protected);
	
		if (loginshell)
			if(readesrc)
				runesrc();
	
		if (cmd == NULL && !cmd_stdin && optind < ac) {
			int fd;
			char *file = av[optind++];
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
		else if (termeq(e->term, "error"))
			eprint("root handler: %L\n",
			       e->next == NULL ? NULL : e->next->next,
			       " ");
		else if (!issilentsignal(e))
			eprint("uncaught exception: %L\n", e, " ");
		return 1;

	EndExceptionHandler
}
