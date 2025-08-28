library library_autoload (init libraries libutil)

if {~ $#fn-old_%pathsearch 0} {
	fn-old_%pathsearch = $fn-%pathsearch
}

fn %pathsearch name {
	catch @ e t m {
		if {~ $e 'error' && ~ $t '$&access'} {
			let (
				lib= <={libutil_function_search $name $__libutil_function_data onerror {
					throw $e $t $m
				}}
			) {
				import $lib
				if {%is-interactive} {
					throw autoload_error
				} {
					throw error '%pathsearch' 'library '^$lib^' autoloaded'
				}
			}
		} {
			throw $e $t $m
		}
	} {
		old_%pathsearch $name
	}
}

fn %interactive-loop {
	let (result = <=true) {
		catch @ e {
			%interactive-exception-handler <={makeerror $e} $result
		} {
			forever {
				%interactive-prompt-hook
				let (
					code = <={%parse $prompt}
					coderan = false
				) {
					if {!~ $#code 0} {
						catch @ e r {
							if {~ $e autoload_error} {
								throw retry
							} {
								throw $e $r
							}
						} {
							if {! $coderan} {
								%interactive-preexec-hook
								result = <={$fn-%dispatch $code}
								coderan = true
							}
							%interactive-postexec-hook $result
						}
					}
				}
			}
		}
	}
}

