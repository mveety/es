/* main.c -- initialization for es ($Revision: 1.3 $) */

#include "es.h"
#include "gc.h"
#include "stdenv.h"
#include <stdio.h>

Boolean gcverbose = FALSE;			/* -G */
Boolean gcinfo = FALSE;				/* -I */
Boolean assertions = FALSE;			/* -Da */
Boolean ref_assertions = FALSE;		/* -DA */
Boolean verbose_parser = FALSE;		/* -P */
Boolean verbose_match = FALSE;		/* -DM */
Boolean use_initialize_esrc = TRUE; /* -Dr */
extern Boolean generational;
extern Boolean use_array_sort;
extern Boolean use_list_sz;
extern int reserve_blocks;
volatile Boolean loginshell = FALSE; /* -l or $0[0] == '-' */
volatile Boolean readesrc = TRUE;
Boolean different_esrc = FALSE;	 /* -I */
char *altesrc = NULL;			 /* for -I */
Boolean additional_esrc = FALSE; /* -A */
char *extraesrc = NULL;			 /* for -A */

extern int optind;
extern char *optarg;
extern int yydebug;
extern size_t blocksize;
extern char **environ;
extern uint32_t gc_oldage;
extern int gc_oldsweep_after;
extern Boolean regex_debug;
extern Boolean dump_tok_status;
extern Boolean verbose_rangematch;
extern Boolean comprehensive_matches;
#ifdef DYNAMIC_LIBRARIES
extern Boolean dynlib_verbose;
#endif

void *
used(void *v)
{
	return v;
}

/* checkfd -- open /dev/null on an fd if it is closed */
static void
checkfd(int fd, OpenKind r)
{
	int new;
	new = dup(fd);
	if(new != -1)
		close(new);
	else if(errno == EBADF && (new = eopen("/dev/null", r)) != -1)
		mvfd(new, fd);
}

void
init_internal_vars(void)
{
	int i;
	static const char *const path[] = {INITIAL_PATH};
	List *list = nil; Root r_list;

	gcref(&r_list, (void **)&list);

	for(i = arraysize(path); i-- > 0;)
		list = mklist(mkstr((char *)path[i]), list);

	vardef("path", nil, list);
	vardef("__ppid", nil, mklist(mkstr(str("%d", getpid())), nil));
	vardef("__es_loginshell", nil, mklist(mkstr(str("%s", loginshell ? "true" : "false")), nil));
	vardef("__es_initialize_esrc", nil,
		   mklist(mkstr(str("%s", use_initialize_esrc ? "true" : "false")), nil));
	vardef("__es_readesrc", nil, mklist(mkstr(str("%s", readesrc ? "true" : "false")), nil));
	vardef("__es_different_esrc", nil,
		   mklist(mkstr(str("%s", different_esrc ? "true" : "false")), nil));
	vardef("__es_esrcfile", nil, mklist(mkstr(different_esrc ? str("%s", altesrc) : ""), nil));
	vardef("__es_extra_esrc", nil,
		   mklist(mkstr(str("%s", additional_esrc ? "true" : "false")), nil));
	vardef("__es_extra_esrcfile", nil,
		   mklist(mkstr(additional_esrc ? str("%s", extraesrc) : ""), nil));
	vardef("__es_comp_match", nil, mklist(mkstr(comprehensive_matches ? "true" : "false"), nil));

	gcderef(&r_list, (void **)&list);
}

static int
runinitialize(void)
{
	List *initialize;
	List *result = NULL; Root r_result;

	result = NULL;

	initialize = varlookup("fn-%initialize", NULL);
	if(initialize == NULL)
		return 0;

	gcref(&r_result, (void **)&result);

	ExceptionHandler
	{
		result = eval(initialize, NULL, 0);
	}
	CatchException(e)
	{
		if(termeq(e->term, "exit"))
			exit(exitstatus(e->next));
		else if(termeq(e->term, "error")) {
			eprint("init handler: %L\n", e, " ");
			exit(-1);
		} else if(!issilentsignal(e))
			eprint("init handler: uncaught exception: %L\n", e, " ");
	}
	EndExceptionHandler;

	gcderef(&r_result, (void **)&result);
	return 0;
}

/* usage -- print usage message and die */
void
usage(void)
{
	eprint(
		"usage: es [-c command] [-sIAlNpodv] [-X gcopt] [-D flags] [-r flags] [file [args ...]]\n"
		"	-c        cmd execute argument\n"
		"	-s        read commands from standard input; stop option parsing\n"
		"	-I file   alternative init file\n"
		"	-A file   additional init file\n"
		"	-l        login shell\n"
		"	-N        ignore the .esrc\n"
		"	-p        don't load functions from the environment\n"
		"	-o        don't open stdin, stdout, and stderr if they were closed\n"
		"	-d        don't ignore SIGQUIT or SIGTERM\n"
		"	-X gcopt  gc options (can be repeated)\n"
		"	-v        print $buildstring\n"
		"	-D flags  debug flags (? for more info)\n"
		"	-r flags  run flags (? for more info)\n");
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
{ /* this is a mess */
#ifdef DYNAMIC_LIBRARIES
	dprintf(2, "debug flags: es -D [GIaEPRAMrhHCmo]\n%s",
#else
	dprintf(2, "debug flags: es -D [GIaEPRAMrhHCo]\n%s",
#endif
			"	? -- show this message\n"
			"	G -- gcverbose\n"
			"	I -- gcinfo\n"
			"	a -- assertions\n"
			"	E -- debug_exceptions\n"
			"	P -- verbose_parser\n"
			"	R -- regex_debug\n"
			"	A -- ref_assertions (likely to crash)\n"
			"	M -- verbose_match\n"
			"	T -- dump_tok_status\n"
			"	r -- verbose_rangematch\n"
			"	h -- HaahrHash\n"
			"	H -- FNV1AHash\n"
			"	C -- comprehensive_matches\n"
#ifdef DYNAMIC_LIBRARIES
			"	m -- dynlib_verbose\n"
#endif
	);
	exit(1);
}

void
run_flag_usage(void)
{
	dprintf(2, "run flags: es -r [einVxL]\n%s",
			"	? -- show this message\n"
			"	e -- exitonfalse\n"
			"	i -- interactive\n"
			"	n -- noexec\n"
			"	v -- echoinput\n"
			"	x -- printcmds\n"
			"	L -- lisptrees\n"
			"	a -- assertions\n");
	exit(1);
}

int
parse_gcopt(char *optarg)
{
	char *orig_work = nil;
	char *work = nil;
	char *parameter = nil;
	char *arg = nil;
	int help = -1;

	orig_work = strdup(optarg);
	work = orig_work;
	parameter = strsep(&work, ":");

	if(streq(parameter, "help") || streq(parameter, "?")) {
		help = 1;
		goto fail;
	}

	if((arg = strsep(&work, ":")) == nil)
		goto fail;

	if(streq(parameter, "gc")) {
		if(streq(arg, "old")) {
			gctype = OldGc;
			generational = TRUE;
		} else if(streq(arg, "new")) {
			gctype = NewGc;
			generational = FALSE;
		} else if(streq(arg, "generational")) {
			gctype = NewGc;
			generational = TRUE;
		} else {
			goto fail;
		}
	} else if(streq(parameter, "gcafter")) {
		gc_after = atoi(arg);
	} else if(streq(parameter, "sortafter")) {
		gc_sort_after_n = atoi(arg);
	} else if(streq(parameter, "coalesceafter")) {
		gc_coalesce_after_n = atoi(arg);
	} else if(streq(parameter, "blocksize")) {
		blocksize = strtoul(arg, NULL, 10);
		if(blocksize == 0)
			goto fail;
		blocksize *= 1024 * 1024;
		if(blocksize < MIN_minspace) {
			dprintf(2, "error: blocksize < %d\n", (MIN_minspace / 1024));
			goto fail;
		}
	} else if(streq(parameter, "oldage")) {
		gc_oldage = atoi(arg);
	} else if(streq(parameter, "oldsweep")) {
		gc_oldsweep_after = atoi(arg);
	} else if(streq(parameter, "sorting")) {
		if(streq(arg, "list"))
			use_array_sort = FALSE;
		else if(streq(arg, "array")) {
			use_array_sort = TRUE;
			use_list_sz = FALSE;
		} else if(streq(arg, "arraysz")) {
			use_array_sort = TRUE;
			use_list_sz = TRUE;
		} else
			goto fail;
	} else if(streq(parameter, "reserveblocks")) {
		reserve_blocks = atoi(arg);
	} else {
		goto fail;
	}
	free(orig_work);
	return 0;
fail:
	free(orig_work);
	dprintf(2, "gc parameters: es -X [help|[parameter]:[value]]\n%s",
			"	gc:[old|new|generational] -- which gc to use (was -X and -G)\n"
			"	gcafter:[int] -- collection frequency (was -g)\n"
			"	sortafter:[int] -- sorting frequency (was -S)\n"
			"	coalesceafter:[int] -- coalescing frequency (was -C)\n"
			"	blocksize:[megabytes] -- memory block size (was -B)\n"
			"	reserveblocks:[int] -- blocks to reserve\n"
			"	oldage:[int] -- age to age out blocks (was -a)\n"
			"	oldsweep:[int] -- oldlist sweeping frequency (was -w)\n"
			"	sorting:[list|array|arraysz] -- which sorting function to use\n");
	return help;
}

/* main -- initialize, parse command arguments, and start running */
int
main(int argc, char *argv[])
{
	int c, fd;
	volatile int ac;
	char **volatile av;
	char *ds;
	char *rs;
	char *file;

	volatile int runflags = 0;			/* -[einvxL] */
	volatile Boolean protected = FALSE; /* -p */
	volatile Boolean allowquit = FALSE; /* -d */
	volatile Boolean cmd_stdin = FALSE; /* -s */
	Boolean keepclosed = FALSE;			/* -o */
	const char *volatile cmd = NULL;	/* -c */

	if(argc == 0) {
		argc = 1;
		argv = ealloc(2 * sizeof(char *));
		argv[0] = "es";
		argv[1] = NULL;
	}
	if(*argv[0] == '-')
		loginshell = TRUE;

	/* yydebug = 1; */

	// removed IGAPL
	while((c = getopt(argc, argv, "+I:A:lX:vpodsc:?hND:r:")) != EOF)
		switch(c) {
		case 'D':
			for(ds = optarg; *ds != 0; ds++) {
				switch(*ds) {
				case 'G':
					gcverbose = TRUE;
					break;
				case 'I':
					gcinfo = TRUE;
					break;
				case 'a':
					assertions = TRUE;
					break;
				case 'A':
					ref_assertions = TRUE;
					break;
				case 'E':
					debug_exceptions = TRUE;
					break;
				case 'P':
					verbose_parser = TRUE;
					break;
				case 'R':
					regex_debug = TRUE;
					break;
				case 'M':
					verbose_match = TRUE;
					break;
				case 'T':
					dump_tok_status = TRUE;
					break;
				case 'r':
					verbose_rangematch = TRUE;
					break;
				case 'h':
					hashfunction = HaahrHash;
					break;
				case 'H':
					hashfunction = FNV1AHash;
					break;
				case 'C':
					comprehensive_matches = TRUE;
					break;
#ifdef DYNAMIC_LIBRARIES
				case 'm':
					dynlib_verbose = TRUE;
					break;
#endif
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
			for(rs = optarg; *rs != 0; rs++) {
				switch(*rs) {
				case 'e':
					runflags |= eval_exitonfalse;
					break;
				case 'i':
					runflags |= run_interactive;
					break;
				case 'n':
					runflags |= run_noexec;
					break;
				case 'v':
					runflags |= run_echoinput;
					break;
				case 'x':
					runflags |= run_printcmds;
					break;
				case 'L':
					runflags |= run_lisptrees;
					break;
				case 'a':
					assertions = TRUE;
					break;
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
			altesrc = optarg;
			break;
		case 'A':
			additional_esrc = TRUE;
			extraesrc = optarg;
			break;
		case 'c':
			cmd = optarg;
			break;
		case 'N':
			readesrc = FALSE;
			break;
		case 'l':
			loginshell = TRUE;
			break;
		case 'p':
			protected = TRUE;
			break;
		case 'o':
			keepclosed = TRUE;
			break;
		case 'd':
			allowquit = TRUE;
			break;
		case 's':
			cmd_stdin = TRUE;
			goto getopt_done;
		case 'X':
			switch(parse_gcopt(optarg)) {
			default:
				unreachable();
			case 0:
				break;
			case -1:
				exit(-1);
			case 1:
				exit(0);
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

	if(cmd_stdin && cmd != NULL) {
		eprint("es: -s and -c are incompatible\n");
		exit(1);
	}

	if(!keepclosed) {
		checkfd(0, oOpen);
		checkfd(1, oCreate);
		checkfd(2, oCreate);
	}

	if(cmd == NULL && (optind == argc || cmd_stdin) && (runflags & run_interactive) == 0 &&
	   isatty(0))
		runflags |= run_interactive;

	ac = argc;
	av = argv;

	ExceptionHandler
	{
		roothandler = &_localhandler; /* unhygeinic */

		init_typedefs();
		init_objects();
		initinput();
		initprims();
		initvars();

		runinitial();

		init_internal_vars();
		initsignals(runflags & run_interactive, allowquit);
		hidevariables();
		initenv(environ, protected);

		runinitialize();

		if(cmd == NULL && !cmd_stdin && optind < ac) {
			file = av[optind++];
			if((fd = eopen(file, oOpen)) == -1) {
				eprint("%s: %s\n", file, esstrerror(errno));
				return 1;
			}
			vardef("*", NULL, listify(ac - optind, av + optind));
			vardef("0", NULL, mklist(mkstr(file), NULL));
			return exitstatus(runfd(fd, file, runflags));
		}

		vardef("*", NULL, listify(ac - optind, av + optind));
		vardef("0", NULL, mklist(mkstr(av[0]), NULL));
		if(cmd != NULL)
			return exitstatus(runstring(cmd, NULL, runflags));
		return exitstatus(runfd(0, "stdin", runflags));
	}
	CatchException (e)
	{
		if(termeq(e->term, "exit"))
			return exitstatus(e->next);
		else if(termeq(e->term, "error")) {
			eprint("root handler: %L\n", e, " ");
		} else if(!issilentsignal(e))
			eprint("root handler: uncaught exception: %L\n", e, " ");
		return 1;
	}
	EndExceptionHandler;
}
