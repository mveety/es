library libutils_cache (init libutils dumplist)

fn luc_find_writable_libdir {
	if {~ $#libraries 0} {
		throw error '$libraries unset'
	}
	for(l = $libraries) {
		if {access -rw $l} {
			return $l
		}
	}
	throw error $0 'no writable $libraries'
}

fn luc_dump_function_data {
	let (libdir = <=luc_find_writable_libdir) {
		if {gt $#__libutil_function_data 0} {
			{
				dumplist __libutil_function_data
				if {~ $options gencomp_libutil} {
					dumplist __gclibutil_clean_system
					dumplist __gclibutil_all_functions
					dumplist __gclibutil_all_libraries
				}
			} > $libdir/library_cache.esdat
		}
	}
}

fn luc_load_function_data {
	let (libdir = <=luc_find_writable_libdir) {
		if {access -rw $libdir/library_cache.esdat} {
			. $libdir/library_cache.esdat
		} {
			throw error $0 'no library_cache.esdat'
		}
	}
}

let (
	fn do_rehash {
		__libutil_function_data = $_libutil_es_system <=libutil_enumerate_all_libs
		%hidevar __libutil_function_data
	}
) {
	fn libutil_rehash {
		assert2 libtuil {eq <={mod $#_libutil_es_system 2} 0}
		if {! ~ $#__libutil_function_data 0} {
			do_rehash
			if {! ~ $#fn-%libutil_rehash 0} { %libutil_rehash }
			luc_dump_function_data
		} {
			luc_load_function_data onerror {
				do_rehash
				if {! ~ $#fn-%libutil_rehash 0} { %libutil_rehash }
				luc_dump_function_data
			}
		}
		%hidevar __libutil_function_data
		assert2 libutil {eq <={mod $#__libutil_function_data 2} 0}
	}
}

