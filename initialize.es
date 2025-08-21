#!/usr/bin/env es

noexport = $noexport __es_initialize_esrc __es_loginshell __es_readesrc
noexport = $noexport __es_different_esrc __es_esrcfile __es_extra_esrc
noexport = $noexport __es_extra_esrcfile

fn __es_esrc_check {
	if {$es_enable_loginshell} {
		result <={$__es_initialize_esrc && $__es_readesrc && $__es_loginshell}
	} {
		result <={$__es_initialize_esrc && $__es_readesrc}
	}
}

fn %initialize {
	# run any setup functions that need to be run in es land, but before the
	# .esrc is run.
	__es_initgc
	__es_complete_initialize

	# run $home/.esrc if applicable
	if {__es_esrc_check} {
		catch @ el {
			errmatch <={makeerror $el} (
				exit { exit $type }
				error { echo >[1=2] 'esrc:' $err^':' $type^':' $msg }
				signal {
					if {! ~ $type 'sigint'} {
						echo >[1=2] 'esrc: uncaught signal:' $err $type $msg
					}
				}
				{ echo >[1=2] 'esrc: uncaught exception:' $err $type $msg }
			)
		} {
			if {! $__es_different_esrc} {
				__es_esrcfile = $home/.esrc
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
					if {!~ $type sigint sigterm sigquit} {
						echo >[1=2] '%user-init: caught unexpected signal:' $type
					}
					return <=false
				}
				{ echo >[1=2] '%user-init: uncaught exception:' $err $type $msg }
			)
		}
	}
	return <=true
}

