#!/usr/local/bin/es
# initial.es -- set up initial interpreter state
options = 'init'

# this is the es-mveety version info. the format is
# $esversion-mveety-$rev

version = <={$&version}
buildstring = <={$&buildstring}

#
# Introduction
#

#	Initial.es contains a series of definitions used to set up the
#	initial state of the es virtual machine.  In early versions of es
#	(those before version 0.8), initial.es was turned into a string
#	which was evaluated when the program started.  This unfortunately
#	took a lot of time on startup and, worse, created a lot of garbage
#	collectible memory which persisted for the life of the shell,
#	causing a lot of extra scanning.
#
#	Since version 0.8, building es is a two-stage process.  First a
#	version of the shell called esdump is built.  Esdump reads and
#	executes commands from standard input, similar to a normal interpreter,
#	but when it is finished, it prints on standard ouput C code for
#	recreating the current state of interpreter memory.  (The code for
#	producing the C code is in dump.c.)  Esdump starts up with no
#	variables defined.  This file (initial.es) is run through esdump
#	to produce a C file, initial.c, which is linked with the rest of
#	the interpreter, replacing dump.c, to produce the actual es
#	interpreter.
#
#	Because the shell's memory state is empty when initial.es is run,
#	you must be very careful in what instructions you put in this file.
#	For example, until the definition of fn-%pathsearch, you cannot
#	run external programs other than those named by absolute path names.
#	An error encountered while running esdump is fatal.
#
#	The bulk of initial.es consists of assignments of primitives to
#	functions.  A primitive in es is an executable object that jumps
#	directly into some C code compiled into the interpreter.  Primitives
#	are referred to with the syntactic construct $&name;  by convention,
#	the C function implementing the primitive is prim_name.  The list
#	of primitives is available as the return value (exit status) of
#	the primitive $&primitives.  Primitives may not be reassigned.
#
#	Functions in es are simply variables named fn-name.  When es
#	evaluates a command, if the first word is a string, es checks if
#	an appropriately named fn- variable exists.  If it does, then the
#	value of that variable is substituted for the function name and
#	evaluation starts over again.  Thus, for example, the assignment
#		fn-echo = $&echo
#	means that the command
#		echo foo bar
#	is internally translated to
#		$&echo foo bar
#	This mechanism is used pervasively.
#
#	Note that definitions provided in initial.es can be overriden (aka,
#	spoofed) without changing this file at all, just by redefining the
#	variables.  The only purpose of this file is to provide initial
#	values.


#
# Builtin functions
#

#	These builtin functions are straightforward calls to primitives.
#	See the manual page for details on what they do.

fn-.		= $&dot
fn-access	= $&access
fn-break	= $&break
fn-catch	= $&catch
fn-echo		= $&echo
fn-exec		= $&exec
fn-forever	= $&forever
fn-fork		= $&fork
fn-if		= $&if
fn-newpgrp	= $&newpgrp
fn-result	= $&result
fn-throw	= $&throw
fn-umask	= $&umask
fn-wait		= $&wait

fn-%read	= $&read

fn-reverse	= $&reverse

#	eval runs its arguments by turning them into a code fragment
#	(in string form) and running that fragment.

fn eval { '{' ^ $^* ^ '}' }

#	Through version 0.84 of es, true and false were primitives,
#	but, as many pointed out, they don't need to be.  These
#	values are not very clear, but unix demands them.

fn-true		= result 0
fn-false	= result 1

#	These functions just generate exceptions for control-flow
#	constructions.  The for command and the while builtin both
#	catch the break exception, and lambda-invocation catches
#	return.  The interpreter main() routine (and nothing else)
#	catches the exit exception.

fn-break	= throw break
fn-exit		= throw exit
fn-return	= throw return

#	unwind-protect is a simple wrapper around catch that is used
#	to ensure that some cleanup code is run after running a code
#	fragment.  This function must be written with care to make
#	sure that the return value is correct.

fn-unwind-protect = $&noreturn @ body cleanup {
	if {!~ $#cleanup 1} {
		throw error unwind-protect 'unwind-protect body cleanup'
	}
	let (exception = ) {
		let (
			result = <={
				catch @ e {
					exception = caught $e
				} {
					$body
				}
			}
		) {
			$cleanup
			if {~ $exception(1) caught} {
				throw $exception(2 ...)
			}
			result $result
		}
	}
}

fn-%block = @ rest {
	local(
		__es_deferbody=
		__es_res=
		fn-defer = @ body { __es_deferbody = {$body} $__es_deferbody }
		__es_exception =
	) {
		catch @ e {
			__es_exception = caught $e
		} {
			__es_res = <={$rest}
		}
		for(fn = $__es_deferbody) {
			$fn
		}
		if {~ $__es_exception(1) caught} {
			throw $__es_exception(2 ...)
		} {
			result $__es_res
		}
	}
}


#	These builtins are not provided on all systems, so we check
#	if the accompanying primitives are defined and, if so, define
#	the builtins.  Otherwise, we'll just not have a limit command
#	and get time from /bin or wherever.

if {~ <=$&primitives limit} {fn-limit = $&limit}
if {~ <=$&primitives time}  {fn-time  = $&time}

#	These builtins are mainly useful for internal functions, but
#	they're there to be called if you want to use them.

fn-%apids	= $&apids
fn-%fsplit      = $&fsplit
fn-%newfd	= $&newfd
fn-%run         = $&run
fn-%split       = $&split
fn-%var		= $&var
fn-%whatis	= $&whatis

# assert and assert2 are used for debugging and error checking

es_assert_def = @ body {
	if {$body} {
		result 0
	} {
		throw assert $^body
	}
}

es_assert2_def = @ loc body {
	if {$body} {
		result 0
	} {
		throw assert $^loc^': '^$^body
	}
}

fn-assert = $es_assert_def
fn-assert2 = $es_assert2_def

fn %enable-assert {
	fn-assert = $es_assert_def
	fn-assert2 = $es_assert2_def
}

fn %disable-assert {
	fn-assert = @ { true }
	fn-assert2 = @ { true }
}

#	These builtins are only around as a matter of convenience, so
#	users don't have to type the infamous <= (nee <>) operator.
#	Whatis also protects the used from exceptions raised by %whatis.

fn var		{ for (i = $*) echo <={%var $i} }

fn whatis {
	let (result = ) {
		for (i = $*) {
			catch @ e from message {
				if {!~ $e error} {
					throw $e $from $message
				}
				echo >[1=2] $message
				result = $result 1
			} {
				echo <={%whatis $i}
				result = $result 0
			}
		}
		result $result
	}
}

#	The while function is implemented with the forever looping primitive.
#	While uses $&noreturn to indicate that, while it is a lambda, it
#	does not catch the return exception.  It does, however, catch break.

fn-while = $&noreturn @ cond body else {
	catch @ e value {
		if {!~ $e break} {
			throw $e $value
		}
		result $value
	} {
		let (result = <=true; hasrun = false)
			forever {
				if {!$cond} {
					if {! ~ $#else 0 && ! $hasrun} {
						throw break <=$else
					} {
						throw break $result
					}
				} {
					hasrun = true
					result = <=$body
				}
			}
	}
}

#	The cd builtin provides a friendlier veneer over the cd primitive:
#	it knows about no arguments meaning ``cd $home'' and has friendlier
#	error messages than the raw $&cd.  (It also used to search $cdpath,
#	but that's been moved out of the shell.)
# mveety addition notes:
# %cdhook is a function to allow hooking in to directory changes. this
# is run, if it exists, every time cd is called. This can be used for
# things like setting your terminal window title automatically or the
# like.

fn cd dir {
	if {~ $#dir 1} {
		$&cd $dir
		if {!~ $#fn-%cdhook 0} {
			%cdhook $dir
		}
	} {~ $#dir 0} {
		if {!~ $#home 1} {
			throw error cd <={
				if {~ $#home 0} {
					result 'no home directory'
				} {
					result 'home directory must be one word'
				}
			}
		}
		$&cd $home
		if {!~ $#fn-%cdhook 0} {
			%cdhook $home
		}
	} {
		throw usage cd 'usage: cd [directory]'
	}
}

#
# Syntactic sugar
#

#	Much of the flexibility in es comes from its use of syntactic rewriting.
#	Traditional shell syntax is rewritten as it is parsed into calls
#	to ``hook'' functions.  Hook functions are special only in that
#	they are the result of the rewriting that goes on.  By convention,
#	hook function names begin with a percent (%) character.

#	One piece of syntax rewriting invokes no hook functions:
#
#		fn name args { cmd }	fn-^name=@ args{cmd}

#	The following expressions are rewritten:
#
#		$#var                  <={%count $var}
#		$^var                  <={%flatten ' ' $var}
#		$"var                  <={%string $var}
#		$:var                  <={%strlist $var}
#		`{cmd args}            <={%backquote <={%flatten '' $ifs} {cmd args}}
#		``ifs {cmd args}       <={%backquote <={%flatten '' ifs} {cmd args}}

fn-%count	= $&count
fn-%flatten	= $&flatten

fn  %string v { %flatten '' $v }
fn %strlist v { %fsplit '' $v }

#	Note that $&backquote returns the status of the child process
#	as the first value of its result list.  The default %backquote
#	puts that value in $bqstatus.

fn %backquote {
	let ((status output) = <={ $&backquote $* }) {
		bqstatus = $status
		result $output
	}
}

# stbackquote returns a list containing the status and the output
# of the command executed. This will get syntactic sugar once I
# work out what char it should use.

fn %stbackquote {
	let ((status output) = <={ $&backquote $* }){
		result ($status $output)
	}
}

#	The following syntax for control flow operations are rewritten
#	using hook functions:
#
#		! cmd			%not {cmd}
#		cmd1; cmd2		%seq {cmd1} {cmd2}
#		cmd1 && cmd2		%and {cmd1} {cmd2}
#		cmd1 || cmd2		%or {cmd1} {cmd2}
#
#	Note that %seq is also used for newline-separated commands within
#	braces.  The logical operators are implemented in terms of if.
#
#	%and and %or are recursive, which is slightly inefficient given
#	the current implementation of es -- it is not properly tail recursive
#	-- but that can be fixed and it's still better to write more of
#	the shell in es itself.

fn-%seq		= $&seq

fn-%not = $&noreturn @ cmd {
	if {$cmd} {false} {true}
}

fn-%and = $&noreturn @ first rest {
	let (result = <={$first}) {
		if {~ $#rest 0} {
			result $result
		} {result $result} {
			%and $rest
		} {
			result $result
		}
	}
}

fn-%or = $&noreturn @ first rest {
	if {~ $#first 0} {
		false
	} {
		let (result = <={$first}) {
			if {~ $#rest 0} {
				result $result
			} {!result $result} {
				%or $rest
			} {
				result $result
			}
		}
	}
}

#	Background commands could use the $&background primitive directly,
#	but some of the user-friendly semantics ($apid, printing of the
#	child process id) were easier to write in es.
#
#		cmd &			%background {cmd}

usebg = 'new' # for compatibility

fn %background cmd {
	let (pid = <={$&background $cmd}) {
		if {%is-interactive} {
			cmds = `` (' ' '{' '}') (echo $cmd)
			if {! ~ $cmds(1) %*} {
				echo $cmds(1)^': '^$pid >[1=2]
			}
		}
		apid = $pid
	}
}

#	These redirections are rewritten:
#
#		cmd < file		%open 0 file {cmd}
#		cmd > file		%create 1 file {cmd}
#		cmd >[n] file		%create n file {cmd}
#		cmd >> file		%append 1 file {cmd}
#		cmd <> file		%open-write 0 file {cmd}
#		cmd <>> file		%open-append 0 file {cmd}
#		cmd >< file		%open-create 1 file {cmd}
#		cmd >>< file		%open-append 1 file {cmd}
#
#	All the redirection hooks reduce to calls on the %openfile hook
#	function, which interprets an fopen(3)-style mode argument as its
#	first parameter.  The other redirection hooks (e.g., %open and
#	%create) exist so that they can be spoofed independently of %openfile.
#
#	The %one function is used to make sure that exactly one file is
#	used as the argument of a redirection.

fn-%openfile	= $&openfile
fn-%open	= %openfile r		# < file
fn-%create	= %openfile w		# > file
fn-%append	= %openfile a		# >> file
fn-%open-write	= %openfile r+		# <> file
fn-%open-create	= %openfile w+		# >< file
fn-%open-append	= %openfile a+		# >>< file, <>> file

fn %one {
	if {!~ $#* 1} {
		throw error %one <={
			if {~ $#* 0} {
				result 'null filename in redirection'
			} {
				result 'too many files in redirection: ' $*
			}
		}
	}
	result $*
}

#	Here documents and here strings are internally rewritten to the
#	same form, the %here hook function.
#
#		cmd << tag input tag	%here 0 input {cmd}
#		cmd <<< string		%here 0 string {cmd}

fn-%here	= $&here

#	These operations are like redirections, except they don't include
#	explicitly named files.  They do not reduce to the %openfile hook.
#
#		cmd >[n=]		%close n {cmd}
#		cmd >[m=n]		%dup m n {cmd}
#		cmd1 | cmd2		%pipe {cmd1} 1 0 {cmd2}
#		cmd1 |[m=n] cmd2	%pipe {cmd1} m n {cmd2}

fn-%close	= $&close
fn-%dup		= $&dup
fn-%pipe	= $&pipe

#	Input/Output substitution (i.e., the >{} and <{} forms) provide an
#	interesting case.  If es is compiled for use with /dev/fd, these
#	functions will be built in.  Otherwise, versions of the hooks are
#	provided here which use files in /tmp.
#
#	The /tmp versions of the functions are straightforward es code,
#	and should be easy to follow if you understand the rewriting that
#	goes on.  First, an example.  The pipe
#		ls | wc
#	can be simulated with the input/output substitutions
#		cp <{ls} >{wc}
#	which gets rewritten as (formatting added):
#		%readfrom _devfd0 {ls} {
#			%writeto _devfd1 {wc} {
#				cp $_devfd0 $_devfd1
#			}
#		}
#	What this means is, run the command {ls} with the output of that
#	command available to the {%writeto ....} command as a file named
#	by the variable _devfd0.  Similarly, the %writeto command means
#	that the input to the command {wc} is taken from the contents of
#	the file $_devfd1, which is assumed to be written by the command
#	{cp $_devfd0 $_devfd1}.
#
#	All that, for example, the /tmp version of %readfrom does is bind
#	the named variable (which is the first argument, var) to the name
#	of a (hopefully unique) file in /tmp.  Next, it runs its second
#	argument, input, with standard output redirected to the temporary
#	file, and then runs the final argument, cmd.  The unwind-protect
#	command is used to guarantee that the temporary file is removed
#	even if an error (exception) occurs.  (Note that the return value
#	of an unwind-protect call is the return value of its first argument.)
#
#	By the way, creative use of %newfd and %pipe would probably be
#	sufficient for writing the /dev/fd version of these functions,
#	eliminating the need for any builtins.  For now, this works.
#
#		cmd <{input}		%readfrom var {input} {cmd $var}
#		cmd >{output}		%writeto var {output} {cmd $var}

if {~ <=$&primitives readfrom} {
	fn-%readfrom = $&readfrom
} {
	fn %readfrom var input cmd {
		local ($var = /tmp/es.$var.$pid) {
			unwind-protect {
				$input > $$var
				# text of $cmd is   command file
				$cmd
			} {
				rm -f $$var
			}
		}
	}
}

if {~ <=$&primitives writeto} {
	fn-%writeto = $&writeto
} {
	fn %writeto var output cmd {
		local ($var = /tmp/es.$var.$pid) {
			unwind-protect {
				> $$var
				$cmd
				$output < $$var
			} {
				rm -f $$var
			}
		}
	}
}

#	These versions of %readfrom and %writeto (contributed by Pete Ho)
#	support the use of System V FIFO files (aka, named pipes) on systems
#	that have them.  They seem to work pretty well.  The authors still
#	recommend using files in /tmp rather than named pipes.

#fn %readfrom var cmd body {
#	local ($var = /tmp/es.$var.$pid) {
#		unwind-protect {
#			/etc/mknod $$var p
#			$&background {$cmd > $$var; exit}
#			$body
#		} {
#			rm -f $$var
#		}
#	}
#}

#fn %writeto var cmd body {
#	local ($var = /tmp/es.$var.$pid) {
#		unwind-protect {
#			/etc/mknod $$var p
#			$&background {$cmd < $$var; exit}
#			$body
#		} {
#			rm -f $$var
#		}
#	}
#}


#
# Hook functions
#

#	These hook functions aren't produced by any syntax rewriting, but
#	are still useful to override.  Again, see the manual for details.

#	%home, which is used for ~expansion.  ~ and ~/path generate calls
#	to %home without arguments;  ~user and ~user/path generate calls
#	to %home with one argument, the user name.

fn-%home	= $&home

#	Path searching used to be a primitive, but the access function
#	means that it can be written easier in es.  Is is not called for
#	absolute path names or for functions.

fn %pathsearch name { access -n $name -1e -xf $path }

#	The exec-failure hook is called in the child if an exec() fails.
#	A default version is provided (under conditional compilation) for
#	systems that don't do #! interpretation themselves.

if {~ <=$&primitives execfailure} {fn-%exec-failure = $&execfailure}


#
# Read-eval-print loops
#

#	In es, the main read-eval-print loop (REPL) can lie outside the
#	shell itself.  Es can be run in one of two modes, interactive or
#	batch, and there is a hook function for each form.  It is the
#	responsibility of the REPL to call the parser for reading commands,
#	hand those commands to an appropriate dispatch function, and handle
#	any exceptions that may be raised.  The function %is-interactive
#	can be used to determine whether the most closely binding REPL is
#	interactive or batch.
#
#	The REPLs are invoked by the shell's main() routine or the . or
#	eval builtins.  If the -i flag is used or the shell determimes that
#	it's input is interactive, %interactive-loop is invoked; otherwise
#	%batch-loop is used.
#
#	The function %parse can be used to call the parser, which returns
#	an es command.  %parse takes two arguments, which are used as the
#	main and secondary prompts, respectively.  %parse typically returns
#	one line of input, but es allows commands (notably those with braces
#	or backslash continuations) to continue across multiple lines; in
#	that case, the complete command and not just one physical line is
#	returned.
#
#	By convention, the REPL must pass commands to the fn %dispatch,
#	which has the actual responsibility for executing the command.
#	Whatever routine invokes the REPL (internal, for now) has
#	the resposibility of setting up fn %dispatch appropriately;
#	it is used for implementing the -e, -n, and -x options.
#	Typically, fn %dispatch is locally bound.
#
#	The %parse function raises the eof exception when it encounters
#	an end-of-file on input.  You can probably simulate the C shell's
#	ignoreeof by restarting appropriately in this circumstance.
#	Other than eof, %interactive-loop does not exit on exceptions,
#	where %batch-loop does.
#
#	The looping construct forever is used rather than while, because
#	while catches the break exception, which would make it difficult
#	to print ``break outside of loop'' errors.
#
#	The parsed code is executed only if it is non-empty, because otherwise
#	result gets set to zero when it should not be.

fn-%parse	= $&parse
fn-%is-interactive = $&isinteractive

fn %batch-loop {
	catch @ e type msg {
		match $e (
			(error) {
				echo >[1=2] 'error: '^$type^': '^$^msg
				return <=false
			}
			(assert) {
				echo >[1=2] 'assert:' $type $msg
				return <=false
			}
			(usage) {
				if {~ $#msg 0} {
					echo >[1=2] $type
				} {
					echo >[1=2] $msg
				}
				return <=false
			}
			* {
				throw $e $type $msg
			}
		)
	} {
		result <={$&batchloop $*}
	}
}

fn %interactive-loop {
	let (result = <=true) {
		catch @ e type msg {
			match $e (
				(eof) { return $result }
				(exit) { throw $e $type $msg}
				(error) { echo >[1=2] 'error: '^$type^': '^$^msg }
				(assert) { echo >[1=2] 'assert:' $type $msg }
				(usage) {
					if {~ $#msg 0} {
						echo >[1=2] $type
					} {
						echo >[1=2] $msg
					}
				}
				(signal) {
					if {!~ $type sigint sigterm sigquit} {
						echo >[1=2] caught unexpected signal: $type
					}
				}
				* { echo >[1=2] uncaught exception: $e $type $msg }
			)
			throw retry
		} {
			forever {
				if {!~ $#fn-%prompt 0} {
					local(bqstatus=) { %prompt }
				}
				let (code = <={%parse $prompt}) {
					if {!~ $#code 0} {
						result = <={$fn-%dispatch $code}
					}
				}
			}
		}
	}
}

#	These functions are potentially passed to a REPL as the %dispatch
#	function.  (For %eval-noprint, note that an empty list prepended
#	to a command just causes the command to be executed.)

fn %eval-noprint				# <default>
fn %eval-print		{ echo $* >[1=2]; $* }	# -x
fn %noeval-noprint	{ }			# -n
fn %noeval-print	{ echo $* >[1=2] }	# -n -x
fn-%exit-on-false = $&exitonfalse		# -e


#
# Settor functions
#

#	Settor functions are called when the appropriately named variable
#	is set, either with assignment or local binding.  The argument to
#	the settor function is the assigned value, and $0 is the name of
#	the variable.  The return value of a settor function is used as
#	the new value of the variable.  (Most settor functions just return
#	their arguments, but it is always possible for them to modify the
#	value.)

#	These functions are used to alias the standard unix environment
#	variables HOME and PATH with their es equivalents, home and path.
#	With path aliasing, colon separated strings are split into lists
#	for their es form (using the %fsplit builtin) and are flattened
#	with colon separators when going to the standard unix form.
#
#	These functions are pretty idiomatic.  set-home disables the set-HOME
#	settor function for the duration of the actual assignment to HOME,
#	because otherwise there would be an infinite recursion.  So too for
#	all the other shadowing variables.

set-home = @ { local (set-HOME = ) HOME = $*; result $* }
set-HOME = @ { local (set-home = ) home = $*; result $* }

set-path = @ { local (set-PATH = ) PATH = <={%flatten : $*}; result $* }
set-PATH = @ { local (set-path = ) path = <={%fsplit  : $*}; result $* }

#	These settor functions call primitives to set data structures used
#	inside of es.

set-history		= $&sethistory
set-signals		= $&setsignals
set-noexport		= $&setnoexport
set-max-eval-depth	= $&setmaxevaldepth

#	If the primitive $&resetterminal is defined (meaning that readline
#	or editline is being used), setting the variables $TERM or $TERMCAP
#	should notify the line editor library.

if {~ <=$&primitives resetterminal} {
	set-TERM	= @ { $&resetterminal; result $* }
	set-TERMCAP	= @ { $&resetterminal; result $* }
	fn-resetterminal = $&resetterminal
}

#
# Getter functions
#
# These functions work similarly to settor functions but execute when a
# variable's contents are retreived. The value of the variable is $1 to
# the getter function. If the variable is not defined the getter is not
# called.
last = ''
get-last = @ {
	$&getlast |>
		%fsplit ' ' |>
		@{ let(r=) { for (i = $*) { if {! ~ $i ''} {r = $r $i} } ; result $r }}
}

eval-depth = ''
get-eval-depth = $&getevaldepth

apids = ''
get-apids = @ { result <=%apids }

unixtime = ''
get-unixtime = $&unixtime

#
# Variables
#

#	These variables are given predefined values so that the interpreter
#	can run without problems even if the environment is not set up
#	correctly.

home		= /
ifs		= ' ' \t \n
prompt		= '; ' ''
max-eval-depth	= 2500 # found experimentally. this will work on linux and freebsd
noexport = noexport

#
# es-mveety extentions
#

# error throwing and debugging prints
fn panic lib rest {
	throw error $lib $rest
}

fn dprint msg {
	if {~ $#debugging 0} {
		return 0
	} {
		echo 'debug:' $msg
		return 0
	}
}

# math and numerical functions
fn-tobase = @ base n { result <={$&tobase $base $n} }
fn-frombase = @ base n { result <={$&frombase $base $n} }

fn __es_nuke_zeros a {
	let(al = <={%strlist $a}) {
		while {~ $al(1) 0} {
			al = $al(2 ...)
		}
		if {~ $#al 0} {
			result '0'
		} {
			result <={%string $al}
		}
	}
}

fn todecimal1 a {
	local (negative = false; abs = $a; r=){
		if {~ $a -*} {
			negative = true
			abs = <={~~ $a -*}
		}
		r = <={match $abs (
			(0) { result dec 0}
			(0x*) { result hex <={frombase 16 <={__es_nuke_zeros <={~~ $abs 0x*}}} }
			(0b*) { result bin <={frombase 2 <={__es_nuke_zeros <={~~ $abs 0b*}}} }
			(0d*) { result dec <={frombase 10 <={__es_nuke_zeros <={~~ $abs 0d*}}} }
			(0o*) { result oct <={frombase 8 <={__es_nuke_zeros <={~~ $abs 0o*}}} }
			* { result dec <={__es_nuke_zeros $abs} }
		)}
		if {$negative} {
			result $r(1) '-'^$r(2)
		} {
			result $r
		}
	}
}

fn todecimal n {
	local (b=) {
		(_ b) = <={todecimal1 $n}
		result $b
	}
}

fn fromdecimal base n {
	local (negative = false; abs = $n; r=) {
		if {~ $n -*} {
			negative = true
			abs = <={~~ $n -*}
		}
		r = <={match $base (
			hex { result '0x'^<={tobase 16 $abs} }
			bin { result '0b'^<={tobase 2 $abs} }
			oct { result '0o'^<={tobase 8 $abs} }
			dec { result $abs }
			* { throw assert $base^' != (hex bin oct dec)' }
		)}
		if {$negative} {
			result '-'^$r
		} {
			result $r
		}
	}
}

fn %mathfun fun a b {
	local (base = dec; an =; bn =) {
		(base bn) = <={todecimal1 $b}
		(base an) = <={todecimal1 $a}

		$fun $an $bn |> fromdecimal $base |> result
	}
}

fn %noconvert_mathfun fun a b {
	local (an = <={todecimal $a}; bn = <={todecimal $b}) {
		$fun $an $bn
	}
}

fn %numcompfun fun a b {
	catch @ e t m {
		if {! ~ $e error} { throw $e $t $m }
		if {! ~ $t '$&frombase'} { throw $e $t $m }
		match $m (
			('invalid input') { false }
			('conversion overflow') { false }
			* { throw $e $t $m }
		)
	} {
		local(an = <={todecimal $a}; bn = <={todecimal $b}) {
			catch @ e t m {
				if {! ~ $e error} { throw $e $t $m }
				if {! ~ $t $fun} { throw $e $t $m }
				match $m (
					('invalid input') { false }
					('conversion overflow') { false }
					* { throw $e $t $m }
				)
			} {
				$fun $an $bn
			}
		}
	}
}

#fn-add = $&add
#fn-sub = $&sub
#fn-mul = $&mul
#fn-div = $&div
#fn-mod = $&mod
#fn-eq = $&eq
#fn-lt = $&lt
#fn-gt = $&gt

fn-add = @ a b { %mathfun $&add $a $b }
fn-sub = @ a b { %mathfun $&sub $a $b }
fn-mul = @ a b { %mathfun $&mul $a $b }
fn-div = @ a b { %mathfun $&div $a $b }
fn-mod = @ a b { %mathfun $&mod $a $b }
fn-eq = @ a b { %numcompfun $&eq $a $b }
fn-lt = @ a b { %numcompfun $&lt $a $b }
fn-gt = @ a b { %numcompfun $&gt $a $b }

fn gte a b {
	if {eq $a $b} {
		true
	} {gt $a $b} {
		true
	} {
		false
	}
}

fn lte a b {
	if {eq $a $b} {
		true
	} {lt $a $b} {
		true
	} {
		false
	}
}

fn waitfor pids {
	local (res =) {
		for (pid = $pids) {
			res = <={wait $pid} $res
		}
		result <={reverse $res}
	}
}

if {~ <=$&primitives addhistory} {
	fn %add-history {
		$&addhistory $1
	}
}

if {~ <=$&primitives clearhistory} {
	fn %clear-history {
		$&clearhistory
	}
}

fn %elem n list {
	result $list($n)
}

fn %last list {
	result $list($#list)
}

fn %first list {
	return $list(1)
}

fn %rest list {
	return $list(2 ...)
}

fn %slice s e list {
	if {gt $s $e} { throw error $0 '$s > $e' }
	result $list($s ... $e)
}

fn try body {
	catch @ e type msg {
		if {~ $e error} {
			result true $type $msg
		} {~ $e assert} {
			throw $e $type $msg
		} {~ $e usage} {
			result true usage
		} {
			throw $e $type $msg
		}
	} {
		local(res = <={$body}) {
			result false $res
		}
	}
}

fn __es_getargs argsbody {
	if {~ $#argsbody 1 } {
		result ()
	} {
		result $argsbody(1 ... <={sub $#argsbody 1})
	}
}

fn __es_getbody argsbody {
	if {~ $#argsbody 1 } {
		result $argsbody
	} {
		result <={%last $argsbody}
	}
}

