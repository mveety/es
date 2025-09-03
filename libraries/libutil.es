library libutil (init libraries)

fn libutil_getlibfiles library {
	let (
		libpaths = $libraries $corelib
		libfiles = ()
	) {
		for (dir = $libpaths) {
			if {access -n $library.es -r $dir} {
				libfiles = $libfiles <={access -n $library.es -1 -r $dir}
			}
		}
		if {~ $#libfiles 0} {
			throw error $0 $library^' not found'
		}
		result $libfiles
	}
}
%hidefunction libutil_getlibfiles

fn libutil_getlibfile library {
	lets (
		libfile = <={access -n $library.es -1 -r $libraries $corelib}
	) {
		if {~ $#libfile 0} {
			throw error $0 $library^' not found'
		}
		result $libfile
	}
}
%hidefunction libutil_getlibfile

fn libutil_remove_nil_elements list {
	let (res = ()) {
		for (i = $list) {
			if {! ~ $i ''} {
				res = $res $i
			}
		}
		result $res
	}
}
%hidefunction libutil_remove_nil_elements

fn libutil_enumerate_file_info file {
	if {! access -r $file} {
		throw error $0 'file not found'
	}
	let (
		filedata = ``(\n){cat $file}
		libname = ()
		deps = ()
	) {
		for (line = $filedata) {
			local (
				sline = <={%fsplit <={%flatten \n \t ' '} $line}
				depsstring =
				tdeps = ()
			) {
				sline = <={libutil_remove_nil_elements $sline}
				match $sline(1) (
					'library' {
						if {! ~ $#libname 0 && ! ~ $libname $sline(2)} {
							throw error $0 'library has multiple definitions'
						}
						libname = $sline(2)
						depsstring = $sline(3 ...)
						eval 'tdeps = '^$^depsstring
						for (i = $tdeps) {
							if {! ~ $i $deps} {
								deps = $deps $i
							}
						}
					}
				)
			}
		}
		result $libname $deps
	}
}
%hidefunction libutil_enumerate_file_info

fn libutil_enumerate_file_functions file {
	if {! access -r $file } {
		throw error $0 'file not found'
	}
	let (
		filedata = ``(\n){cat $file}
		functions = ()
	) {
		for (line = $filedata) {
			let (
				sline = <={%fsplit <={%flatten \n \t ' '} $line}
				t=
			) {
				while {~ $sline(1) ''} { sline = $sline(2 ...) }
				match $sline(1) (
					'fn' {
						if {! ~ $sline(2) $functions} {
							functions = $functions $sline(2)
						}
					}
					fn-* {
						match $matchexpr (
							(*^'^'^*) { continue }
							(*^'*'^*) { continue }
							('fn-'^*^'$'^*) { continue }
						)
						if {! ~ <={~~ $matchexpr fn-*} $functions} {
							functions = $functions <={~~ $matchexpr fn-*}
						}
					}
				)
			}
		}
		result $functions
	}
}
%hidefunction libutil_enumerate_file_functions

fn libutil_enumerate_deps library {
	libutil_getlibfile $library |> libutil_enumerate_file_info |> %rest
}
%hidefunction libutil_enumerate_deps

fn libutil_check_definition library {
	if {! ~ $library <={libutil_getlibfile $library |> libutil_enumerate_file_info |> %elem 1}} {
		throw error $0 'library''s defined name does not match file name'
	}
	true
}
%hidefunction libutil_check_definition

fn libutil_enumerate_functions library {
	libutil_enumerate_file_functions <={libutil_getlibfile $library}
}
%hidefunction libutil_enumerate_functions

fn libutil_all_libraries {
	lets (
		all_lib_files = $libraries/*.es $corelib/*.es
		libname =
		all_libs = ()
	) {
		for (libfile = $all_lib_files) {
			match $libfile (
				$libraries/*.es { libname = <={~~ $libfile $libraries/*.es} }
				$corelib/*.es { libname = <={~~ $libfile $corelib/*.es} }
			)
			if {! ~ $libname $all_libs} {
				all_libs = $all_libs $libname
			}
		}
		result $all_libs
	}
}
%hidefunction libutil_all_libraries

fn libutil_enumerate_all_funs {
	let (funs = ()) {
		for (lib = <=libutil_all_libraries) {
			funs = $funs <={libutil_enumerate_functions $lib}
		}
		result $funs
	}
}
%hidefunction libutil_enumerate_all_funs

fn libutil_enumerate_all_libs {
	lets (funlibs = ()) {
		for (lib = <=libutil_all_libraries) {
			let (tmp =) {
				for (f = <={libutil_enumerate_functions $lib}) {
					tmp = $tmp $f $lib
				}
				funlibs = $funlibs $tmp
			}
		}
		result $funlibs
	}
}
%hidefunction libutil_enumerate_all_libs

fn libutil_function_search fun libhash {
	for ((mfun libname) = $libhash) {
		if {~ $mfun $fun} {
			return $libname
		}
	}
	throw error $0 'function '''^$fun^''' not found'
}
%hidefunction libutil_function_search

defconf libutil enable-build false
if {$libutil_conf_enable-build} {
	echo 'libutil build tools enabled' >[1=2]

	fn libutil_build_make_es_system {
		import format
		let (
			init_funs =
			init_fmt_funs =
			initfiles = (initial.es applymap.es range.es glob.es
						gc.es parseargs.es doiterator.es newvars.es
						dict.es errmatch.es initialize.es)
			macros_funs =
			macros_fmt_funs =
			macrosfiles = macros.es
			libraries_funs =
			libraries_fmt_funs =
			librariesfiles = libraries.es
			completion_funs =
			completion_fmt_funs =
			completionfiles = complete.es
		) {
			echo -n 'hashing...' >[1=2]
			for (file = $initfiles) {
				echo -n $file^'...' >[1=2]
				init_funs = $init_funs <={libutil_enumerate_file_functions $file}
			}
			for (file = $macrosfiles) {
				echo -n $file^'...' >[1=2]
				macros_funs = $macros_funs <={libutil_enumerate_file_functions $file}
			}
			for (file = $librariesfiles) {
				echo -n $file^'...' >[1=2]
				libraries_funs = $libraries_funs <={libutil_enumerate_file_functions $file}
			}
			for (file = $completionfiles) {
				echo -n $file^'...' >[1=2]
				completion_funs = $completion_funs <={libutil_enumerate_file_functions $file}
			}
			init_funs = <={let (t=) {
				for (i = $init_funs) {
					if {! ~ $i __es_*} {
						t = $t $i
					}
				}
				result $t
			}}
			echo '' >[1=2]
			echo 'generating lists...' >[1=2]
		
			for (i = $init_funs) {
				init_fmt_funs = $init_fmt_funs $i init
			}
			assert {eq <={mod $#init_fmt_funs 2} 0}
			for (i = $macros_funs) {
				macros_fmt_funs = $macros_fmt_funs $i macros
			}
			assert {eq <={mod $#macros_fmt_funs 2} 0}
			for (i = $libraries_funs) {
				libraries_fmt_funs = $libraries_fmt_funs $i libraries
			}
			assert {eq <={mod $#libraries_fmt_funs 2} 0}
			for (i = $completion_funs) {
				completion_fmt_funs = $completion_fmt_funs $i completion
			}
			assert {eq <={mod $#completion_fmt_funs 2} 0}
			echo '_libutil_es_system = ('
			lets (
				col = 0
				ts =
				fn-fmtecho = @ s {
					if {gte $col 65} {
						echo ''
						col = 0
					}
					if {eq $col 0} {
						echo -n \t
						col = 4
					}
					ts = ''''^$s^''''
					if {gt $col 4} { ts = ' '^$ts }
					echo -n $ts
					col = <={%count $:ts |> add $col}
				}
			) {
				echo -n \t
				echo '# init'
				for (f = $init_fmt_funs) {
					fmtecho $f
				}
				echo ''
				col = 0
				echo -n \t
				echo '# macros'
				for (f = $macros_fmt_funs) {
					fmtecho $f
				}
				echo ''
				col = 0
				echo -n \t
				echo '# libraries'
				for (f = $libraries_fmt_funs) {
					fmtecho $f
				}
				echo ''
				col = 0
				echo -n \t
				echo '# completion'
				for (f = $completion_fmt_funs) {
					fmtecho $f
				}
				echo ''
				echo ')'
			}
		}
	}
}

_libutil_es_system = (
	# init
	'.' 'init' 'access' 'init' 'break' 'init' 'catch' 'init' 'echo'
	'init' 'exec' 'init' 'forever' 'init' 'fork' 'init' 'if' 'init'
	'newpgrp' 'init' 'result' 'init' 'throw' 'init' 'umask' 'init'
	'wait' 'init' '%read' 'init' 'reverse' 'init' 'eval' 'init' 'true'
	'init' 'false' 'init' 'exit' 'init' 'return' 'init' 'continue'
	'init' 'unwind-protect' 'init' '%block' 'init' 'defer' 'init'
	'%apids' 'init' '%fsplit' 'init' '%newfd' 'init' '%run' 'init'
	'%split' 'init' '%var' 'init' '%whatis' 'init' 'assert' 'init'
	'assert2' 'init' '%enable-assert' 'init' '%disable-assert' 'init'
	'var' 'init' 'whatis' 'init' 'while' 'init' 'cd' 'init' '%count'
	'init' '%flatten' 'init' '' 'init' '%strlist' 'init' '%backquote'
	'init' '%stbackquote' 'init' '%seq' 'init' '%not' 'init' '%and'
	'init' '%or' 'init' '%background' 'init' '%openfile' 'init' '%open'
	'init' '%create' 'init' '%append' 'init' '%open-write' 'init'
	'%open-create' 'init' '%open-append' 'init' '%one' 'init' '%here'
	'init' '%close' 'init' '%dup' 'init' '%pipe' 'init' '%readfrom'
	'init' '%writeto' 'init' '%home' 'init' '%pathsearch' 'init' '%parse'
	'init' '%is-interactive' 'init' '%batch-exception-handler' 'init'
	'%old-batch-loop' 'init' '%batch-loop' 'init' '%interactive-exception-handler'
	'init' '%interactive-hook-exception-handler' 'init' '%interactive-prompt-hook'
	'init' '%interactive-preexec-hook' 'init' '%interactive-postexec-hook'
	'init' '%interactive-loop' 'init' '%eval-noprint' 'init' '%eval-print'
	'init' '%noeval-noprint' 'init' '%noeval-print' 'init' '%exit-on-false'
	'init' 'resetterminal' 'init' 'panic' 'init' 'dprint' 'init' 'tobase'
	'init' 'frombase' 'init' 'todecimal1' 'init' 'todecimal' 'init'
	'fromdecimal' 'init' '%mathfun' 'init' '%noconvert_mathfun' 'init'
	'%numcompfun' 'init' 'add' 'init' 'sub' 'init' 'mul' 'init' 'div'
	'init' 'mod' 'init' 'eq' 'init' 'lt' 'init' 'gt' 'init' 'gte'
	'init' 'lte' 'init' 'waitfor' 'init' '%add-history' 'init' '%clear-history'
	'init' '%readline' 'init' '%elem' 'init' '%last' 'init' '%first'
	'init' '%rest' 'init' '%slice' 'init' 'makeerror' 'init' 'try'
	'init' 'box' 'init' 'apply' 'init' 'map' 'init' 'bqmap' 'init'
	'fbqmap' 'init' '%range' 'init' 'glob' 'init' 'glob_test' 'init'
	'isextendedglob' 'init' 'esmglob_expandquestions' 'init' 'dirlist2glob'
	'init' 'esmglob_repeat_string' 'init' 'matchlist_elem' 'init'
	'esmglob_expand_qmacro' 'init' 'esmglob_alllen1' 'init' 'esmglob_optimize_mid'
	'init' 'esmglob_partialcompilation' 'init' 'esmglob_double_wild_removal'
	'init' 'esmglob_parse_escapes' 'init' 'esmglob_add_unique' 'init'
	'esmglob_compile0' 'init' 'esmglob_compile' 'init' 'esmglob_do_glob'
	'init' 'esmglob_compmatch' 'init' '%esmglob' 'init' '%esmglob_match'
	'init' '%esm~' 'init' '%gcstats' 'init' '%gc' 'init' '%gcinfo'
	'init' 'es_canonicalize_args' 'init' 'es_run_parseargs' 'init'
	'usage' 'init' 'done' 'init' 'has_argument' 'init' 'parseargs'
	'init' 'iterator' 'init' 'iterator2' 'init' 'do' 'init' 'do2'
	'init' 'accumulate' 'init' 'accumulate2' 'init' 'dolist' 'init'
	'es_old_vars' 'init' 'es_new_vars' 'init' 'selected' 'init' 'printable'
	'init' 'allvars' 'init' 'allinternals' 'init' 'vars' 'init' 'dictnew'
	'init' 'dictget' 'init' 'dictput' 'init' 'dictremove' 'init' 'dictsize'
	'init' 'dictforall' 'init' 'dictnames' 'init' 'dictvalues' 'init'
	'dictdump' 'init' 'dictdump_statements' 'init' 'iserror' 'init'
	'errmatch' 'init' '%initialize' 'init'
	# macros
	'gensym' 'macros' 'nrfn' 'macros' 'macro' 'macros'
	# libraries
	'import-core-lib' 'libraries' 'import-user-lib' 'libraries' 'import_file'
	'libraries' 'import' 'libraries' 'checkoption' 'libraries' 'option'
	'libraries' 'check_and_load_options' 'libraries' 'library' 'libraries'
	'havelib' 'libraries'
	# completion
	'escomp_echo' 'completion' 'es_complete_remove_empty_results'
	'completion' 'es_complete_only_executable' 'completion' 'es_complete_executable_filter'
	'completion' 'es_complete_access' 'completion' 'es_complete_only_names'
	'completion' 'es_complete_run_glob' 'completion' 'complete_executables'
	'completion' 'complete_files' 'completion' 'complete_all_variables'
	'completion' 'complete_functions' 'completion' 'complete_variables'
	'completion' 'complete_primitives' 'completion' 'es_complete_strip_leading_whitespace'
	'completion' 'es_complete_is_sub_command' 'completion' 'es_complete_left_trim'
	'completion' 'es_complete_right_trim' 'completion' 'es_complete_trim'
	'completion' 'es_complete_get_last_cmdline' 'completion' 'es_complete_get_last_raw_command'
	'completion' 'es_complete_get_last_command' 'completion' 'es_complete_is_command'
	'completion' 'es_complete_run_command_hook' 'completion' 'es_complete_remove'
	'completion' 'es_complete_remove2' 'completion' '%complete_cmd_unhook'
	'completion' '%complete_cmd_hook' 'completion' '__es_complete_initialize'
	'completion' 'complete_base_complete' 'completion' '%complete'
	'completion' 'es_complete_cc_checkstate' 'completion' 'es_complete_format_list'
	'completion' 'es_complete_dump_state' 'completion' '%core_completer'
	'completion'
)

if {~ $#__libutil_function_data 0} {
	__libutil_function_data = $_libutil_es_system <=libutil_enumerate_all_libs
	$&varhide __libutil_function_data
}

fn libutil_rehash {
	assert2 libutil {eq <={mod $#_libutil_es_system 2} 0}
	__libutil_function_data = $_libutil_es_system <=libutil_enumerate_all_libs
	if {! ~ $#fn-%libutil_rehash 0} {
		%libutil_rehash
	}
	assert2 libutil {eq <={mod $#__libutil_function_data 2} 0}
}

fn libutil_whereis fun {
	if {~ $#__libutil_function_data 0} { libutil_rehash }
	let ( lib = <={libutil_function_search $fun $__libutil_function_data}) {
		if {~ $lib $options} {
			echo $lib '(loaded)'
		} {
			echo $lib
		}
	}
}

fn libutil_deps libraries {
	if {~ $#libraries 0} {
		libraries = <={libutil_all_libraries}
	}
	for (library = $libraries) {
		echo $library^':' <={libutil_enumerate_deps $library}
	}
}

fn libutil_funs libraries {
	if {~ $#libraries 0} {
		libraries = <={libutil_all_libraries}
	}
	for (library = $libraries) {
		echo $library^':' <={libutil_enumerate_functions $library}
	}
}

