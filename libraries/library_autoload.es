library library_autoload (init libraries libutils)

fn %autoload_function_search name {
	if {~ $#__libutil_function_data 0} {
		libutil_rehash
	}
	libutil_function_search $name $__libutil_function_data
}

let (old_pathsearch = $fn-%pathsearch) {
	fn %pathsearch name {
		catch @ e t m {
			if {~ $e 'error' && ~ $t '$&access'} {
				let (
					lib= <={%autoload_function_search $name onerror {
						throw $e $t $m
					}}
				) {
					import $lib
					if {%is-interactive} {
						throw autoload_error $name
					} {
						throw error '%pathsearch' 'library '^$lib^' autoloaded'
					}
				}
			} {
				throw $e $t $m
			}
		} {
			$old_pathsearch $name
		}
	}
}

noexport += fn-%pathsearch

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
					retry_autoload = true
				) {
					if {!~ $#code 0} {
						catch @ e r {
							if {~ $e autoload_error && $retry_autoload} {
								retry_autoload = false
								throw retry
							} {
								if {~ $e autoload_error} {
									throw error '%pathsearch' 'unable to find '^$^r
								}
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

fn %interactive-execute forms {
	let (result = <=true) {
		catch @ e {
			%interactive-exception-handler <={makeerror $e} $result
		} {
			let (
				coderan = false
				retry_autoload = true
			) {
				catch @ e r {
					if {~ $e autoload_error $retry_autoload} {
						retry_autoload = false
						throw retry
					} {
						if {~ $e autoload_error} {
							throw error '%pathsearch' 'unable to find '^$^r
						}
						throw $e $r
					}
				} {
					result = <={$forms}
				}
			}
		}
		return $result
	}
}

