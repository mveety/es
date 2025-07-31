library library_autoload (init libraries libutil)

let (fn-old_%pathsearch = $fn-%pathsearch) {
	fn %pathsearch name {
		catch @ e t m {
			if {~ $e 'error' && ~ $t '$&access'} {
				let (s=; lib=) {
					(s lib) = <={try libutil_function_search $name $__libutil_function_data}
					if {$s} { throw $e $t $m }
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
						catch @ e r {
							if {~ $e autoload_error} {
								throw retry
							} {
								throw $e $r
							}
						} {
							result = <={$fn-%dispatch $code}
						}
					}
				}
			}
		}
	}
}

