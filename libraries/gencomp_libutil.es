library gencomp_libutil (init completion libutils general_completion)

fn gclibutil_clean_system {
	process $_libutil_es_system (
		'init' { continue }
		'macros' { continue }
		'libraries' { continue }
		'completion' { continue }
		* { result $matchexpr }
	)
}

fn %libutil_rehash {
	if {~ $#__gclibutil_clean_system 0} {
		__gclibutil_clean_system = <={gclibutil_clean_system}
	}
	__gclibutil_all_libraries = <={libutil_all_libraries}
	__gclibutil_all_functions = $__gclibutil_clean_system
	for (lib = $__gclibutil_all_libraries) {
		__gclibutil_all_functions = $__gclibutil_all_functions <={libutil_enumerate_functions $lib}
	}
}

fn gclibutil_clear_state {
	__gclibutil_clean_system = ()
	__gclibutil_all_functions = ()
	__gclibutil_all_libraries = ()
}

fn gclibutil_all_functions {
	if {~ $#__gclibutil_all_functions 0} {
		libutil_rehash
	}
	result $__gclibutil_all_functions
}

fn gclibutil_all_libraries {
	if {~ $#__gclibutil_all_libraries 0} {
		libutil_rehash
	}
	result $__gclibutil_all_libraries
}

fn-gclibutil_whereis_hook = <={gencomp_new_simple_completer libutil_whereis $fn-gclibutil_all_functions}
fn-gclibutil_deps_hook = <={gencomp_new_simple_completer libutil_deps $fn-gclibutil_all_libraries}
fn-gclibutil_funs_hook = <={gencomp_new_simple_completer libutil_funs $fn-gclibutil_all_libraries}

%complete_cmd_hook libutil_whereis @ curline partial {
	gclibutil_whereis_hook $curline $partial
}

%complete_cmd_hook libutil_deps @ curline partial {
	gclibutil_deps_hook $curline $partial
}

%complete_cmd_hook libutil_funs @ curline partial {
	gclibutil_funs_hook $curline $partial
}

