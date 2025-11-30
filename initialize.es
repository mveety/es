#!/usr/bin/env es

noexport += __es_initialize_esrc __es_loginshell __es_readesrc __es_different_esrc
noexport += __es_esrcfile __es_extra_esrc __es_extra_esrcfile __es_interactive_start

let (
	fn __es_esrc_check {
		if {$es_enable_loginshell} {
			result <={$__es_initialize_esrc && $__es_readesrc && $__es_loginshell}
		} {
			result <={$__es_initialize_esrc && $__es_readesrc}
		}
	}
) {
	fn %initialize {
		# run any setup functions that need to be run in es land, but before the
		# .esrc is run.

		local(set-ppid=){
			ppid = $__ppid
		}

		# tune max-eval-depth based on the systems stacksize
		# stacksize/1024 seems to be a reasonable number
		max-eval-depth = <={let (new-eval-depth = <={limit -r stacksize |> @{div $1 1024}}){
			if {lt $new-eval-depth 680} { new-eval-depth = 680 }
			result $new-eval-depth
		}}

		__es_initgc
		__es_complete_initialize
		__es_libraries_initialize
		%unhidevar es_internal_symcount

		# run $home/.esrc if applicable
		if {__es_esrc_check} {
			catch @ el {
				errmatch <={makeerror $el} (
					exit { exit $type }
					error { echo >[1=2] 'esrc:' $err^':' $type^':' $msg }
					signal {
						if {! ~ $type 'sigint' 'sigwinch'} {
							echo >[1=2] 'esrc: uncaught signal:' $err $type $msg
						}
					}
					{ echo >[1=2] 'esrc: uncaught exception:' $err $type $msg }
				)
			} {
				local(esrc_found = false){
					if {! $__es_different_esrc} {
						if {! ~ $#libraries 0} {
							for (l = $libraries) {
								if {access -r $l/init.es} {
									__es_esrcfile = $l/init.es
									esrc_found = true
									break
								}
							}
						}
						if {! $esrc_found} {
							__es_esrcfile = $home/.esrc
						}
					}
				}
				. $__es_esrcfile
				if {$__es_extra_esrc} {
					if {access -r $__es_extra_esrcfile} {
						. $__es_extra_esrcfile
					} {
						echo 'warning: '^$__es_extra_esrcfile^' not found!'
					}
				}
			}
		}

		# post .esrc setup functions
		__es_devfd_init

		# run %user-init hook if available
		if {! ~ $#fn-%user-init 0} {
			let (e=) {
				(e _) = <={try %user-init}
				if {! $e} { return <=true }
				errmatch $e (
					continue { return <=true }
					eof { return <=true }
					exit { exit $type }
					error {
						echo >[1=2] '%user-init: error:' $type^':' $msg
						return <=false
					}
					assert {
						echo >[1=2] '%user-init: assert:' $type $msg
						return <=false
					}
					usage {
						if {~ $#msg 0} {
							echo >[1=2] '%user-init:' $type
						} {
							echo >[1=2] '%user-init:' $msg
						}
						return <=false
					}
					signal {
						if {!~ $type sigint sigterm sigquit sigwinch} {
							echo >[1=2] '%user-init: caught unexpected signal:' $type
						}
						return <=false
					}
					{ echo >[1=2] '%user-init: uncaught exception:' $err $type $msg }
				)
			}
		}

		if {$__es_interactive_start} {
			if {! ~ $#fn-%interactive-start 0} {
				let (e=) {
					(e _) = <={try %interactive-start}
					if {! $e} { return <=true }
					errmatch $e (
						continue { return <=true }
						eof { return <=true }
						exit { exit $type }
						error {
							echo >[1=2] '%interactive-start: error:' $type^':' $msg
							return <=false
						}
						assert {
							echo >[1=2] '%interactive-start: assert:' $type $msg
							return <=false
						}
						usage {
							if {~ $#msg 0} {
								echo >[1=2] '%interactive-start:' $type
							} {
								echo >[1=2] '%interactive-start:' $msg
							}
							return <=false
						}
						signal {
							if {!~ $type sigint sigterm sigquit sigwinch} {
								echo >[1=2] '%interactive-start: caught unexpected signal:' $type
							}
							return <=false
						}
						{ echo >[1=2] '%interactive-start: uncaught exception:' $err $type $msg }
					)
				}
			}
		}
		return <=true
	}
}

