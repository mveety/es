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
				match $sline(1) (
					'fn' { functions = $functions $sline(2) }
					fn-* { functions = $functions <={~~ $matchexpr fn-*} }
				)
			}
		}
		result $functions
	}
}

fn libutil_enumerate_functions library {
	libutil_enumerate_file_functions <={libutil_getlibfile $library}
}

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

fn libutil_function_search fun libhash {
	for ((mfun libname) = $libhash) {
		if {~ $mfun $fun} {
			return $libname
		}
	}
	throw error $0 'function '''^$fun^''' not found'
}

if {~ $#libutil_enable_build 0} { libutil_enable_build = false }
if {$libutil_enable_build} {
	echo 'libutil build tools enabled' >[1=2]

	fn libutil_build_make_es_system {
		import format
		let (
			init_funs =
			initfiles = (initial.es applymap.es range.es glob.es
						gc.es parseargs.es doiterator.es newvars.es
						initialize.es)
			macros_funs =
			macrosfiles = macros.es
			libraries_funs =
			librariesfiles = libraries.es
			completion_funs =
			completionfiles = complete.es
			funs = ()
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
			echo '' >[1=2]
			echo 'generating list...' >[1=2]
		
			for (i = $init_funs) {
				funs = $funs $i init
			}
			for (i = $macros_funs) {
				funs = $funs $i macros
			}
			for (i = $libraries_funs) {
				funs = $funs $i libraries
			}
			for (i = $completion_funs) {
				funs = $funs $i completion
			}
			echo 'libutil_es_system = '^<={format $funs}
		}
	}

}

libutil_es_system = (
	# init
	'.' 'init' 'access' 'init' 'break' 'init' 'catch' 'init'
	'echo' 'init' 'exec' 'init' 'forever' 'init' 'fork' 'init'
	'if' 'init' 'newpgrp' 'init' 'result' 'init' 'throw' 'init'
	'umask' 'init' 'wait' 'init' '%read' 'init' 'reverse' 'init'
	'eval' 'init' 'true' 'init' 'false' 'init' 'break' 'init'
	'exit' 'init' 'return' 'init' 'unwind-protect' 'init'
	'%block' 'init' '%apids' 'init' '%fsplit' 'init' '%newfd' 'init'
	'%run' 'init' '%split' 'init' '%var' 'init' '%whatis' 'init'
	'assert' 'init' 'assert2' 'init' '%enable-assert' 'init'
	'%disable-assert' 'init' 'var' 'init' 'whatis' 'init'
	'while' 'init' 'cd' 'init' '%count' 'init' '%flatten' 'init' 
	'%strlist' 'init' '%backquote' 'init' '%stbackquote' 'init'
	'%seq' 'init' '%not' 'init' '%and' 'init' '%or' 'init'
	'%background' 'init' '%openfile' 'init' '%open' 'init'
	'%create' 'init' '%append' 'init' '%open-write' 'init'
	'%open-create' 'init' '%open-append' 'init' '%one' 'init'
	'%here' 'init' '%close' 'init' '%dup' 'init' '%pipe' 'init'
	'%home' 'init' '%pathsearch' 'init' '%parse' 'init'
	'%is-interactive' 'init' '%batch-loop' 'init'
	'%interactive-loop' 'init' '%eval-noprint' 'init'
	'%eval-print' 'init' '%noeval-noprint' 'init'
	'%noeval-print' 'init' '%exit-on-false' 'init' 'panic' 'init'
	'dprint' 'init' 'tobase' 'init' 'frombase' 'init'
	'__es_nuke_zeros' 'init' 'todecimal1' 'init' 'todecimal' 'init'
	'fromdecimal' 'init' '%mathfun' 'init' '%noconvert_mathfun' 'init'
	'%numcompfun' 'init' 'add' 'init' 'sub' 'init' 'mul' 'init'
	'div' 'init' 'mod' 'init' 'eq' 'init' 'lt' 'init' 'gt' 'init'
	'gte' 'init' 'lte' 'init' 'waitfor' 'init' '%elem' 'init'
	'%last' 'init' '%first' 'init' '%rest' 'init' '%slice' 'init'
	'try' 'init' '__es_getargs' 'init' '__es_getbody' 'init'
	'apply' 'init' 'map' 'init' 'bqmap' 'init' 'fbqmap' 'init'
	'%range' 'init' 'glob' 'init' 'glob_test' 'init'
	'isextendedglob' 'init' 'esmglob_expandquestions' 'init'
	'dirlist2glob' 'init' 'esmglob_repeat_string' 'init'
	'matchlist_elem' 'init' 'esmglob_expand_qmacro' 'init'
	'esmglob_alllen1' 'init' 'esmglob_optimize_mid' 'init'
	'esmglob_partialcompilation' 'init'
	'esmglob_double_wild_removal' 'init' 'esmglob_parse_escapes' 'init'
	'esmglob_add_unique' 'init' 'esmglob_compile0' 'init'
	'esmglob_compile' 'init' 'esmglob_do_glob' 'init'
	'esmglob_compmatch' 'init' '%esmglob' 'init' '%esmglob_match' 'init'
	'%esm~' 'init' '%gcstats' 'init' '%gc' 'init' '%gcinfo' 'init'
	'es_canonicalize_args' 'init' 'es_run_parseargs' 'init'
	'parseargs' 'init' 'iterator' 'init' 'iterator2' 'init' 'do' 'init'
	'do2' 'init' 'accumulate' 'init' 'accumulate2' 'init' 'dolist' 'init'
	'es_old_vars' 'init' 'es_new_vars' 'init' 'vars' 'init'
	'%initialize' 'init'
	# macros 
	'gensym' 'macros' 'nrfn' 'macros' 'macro' 'macros'
	# libraries
	'import-core-lib' 'libraries' 'import-user-lib' 'libraries'
	'import_file' 'libraries' 'import' 'libraries' 'checkoption' 'libraries'
	'option' 'libraries' 'check_and_load_options' 'libraries'
	'library' 'libraries' 'havelib' 'libraries'
	# completion
	'escomp_echo' 'completion' 'es_complete_remove_empty_results' 'completion'
	'es_complete_only_executable' 'completion'
	'es_complete_executable_filter' 'completion'
	'es_complete_access' 'completion' 'es_complete_only_names' 'completion'
	'es_complete_run_glob' 'completion' 'complete_executables' 'completion'
	'complete_files' 'completion' 'complete_all_variables' 'completion'
	'complete_functions' 'completion' 'complete_variables' 'completion'
	'complete_primitives' 'completion'
	'es_complete_strip_leading_whitespace' 'completion'
	'es_complete_is_sub_command' 'completion'
	'es_complete_left_trim' 'completion'
	'es_complete_right_trim' 'completion' 'es_complete_trim' 'completion'
	'es_complete_get_last_cmdline' 'completion'
	'es_complete_get_last_raw_command' 'completion'
	'es_complete_get_last_command' 'completion'
	'es_complete_is_command' 'completion'
	'es_complete_run_command_hook' 'completion'
	'es_complete_remove' 'completion' 'es_complete_remove2' 'completion'
	'%complete_cmd_unhook' 'completion' '%complete_cmd_hook' 'completion'
	'%complete' 'completion' 'es_complete_cc_checkstate' 'completion'
	'es_complete_format_list' 'completion' 'es_complete_dump_state' 'completion'
	'%core_completer' 'completion'
)


if {~ $#__libutil_function_data 0} {
	assert2 libutil {eq <={mod $#libutil_es_system 2} 0}
	__libutil_function_data = $libutil_es_system <=libutil_enumerate_all_libs
	assert2 libutil {eq <={mod $#__libutil_function_data 2} 0}
}

fn libutil_rehash {
	assert2 libutil {eq <={mod $#libutil_es_system 2} 0}
	__libutil_function_data = $libutil_es_system <=libutil_enumerate_all_libs
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

