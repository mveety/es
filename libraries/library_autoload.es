library library_autoload (init libraries libutil)

fn-old_%pathsearch = $fn-%pathsearch

fn %pathsearch name {
	catch @ e t m {
		if {~ $e 'error' && ~ $t '$&access'} {
			let (s=; lib=) {
				(s lib) = <={try libutil_function_search $name $__libutil_function_data}
				if {$s} { throw $e $t $m }
				import $lib
				throw error '%pathsearch' 'library '^$lib^' autoloaded'
			}
		} {
			throw $e $t $m
		}
	} {
		old_%pathsearch $name
	}
}

