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
	if {__es_esrc_check} {
		# optionally you can test for $__es_loginshell
		catch @ e t m {
			if {! ~ $e 'exit' && ! ~ $e 'signal' && ! ~ $t 'sigint'} {
				echo -n 'esrc handler: '
			}
			match $e (
				'exit' { exit $t }
				'error' { echo $e^':' $t^':' $m }
				'signal' {
					if {! ~ $t 'sigint'} { echo 'uncaught signal:' $e $t $m }
				}
				* { echo 'uncaught exception:' $e $t $m }
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
}

