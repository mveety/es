#!/usr/bin/env es

fn %initialize {
	if {$__es_initialize_esrc && $__es_loginshell && $__es_readesrc} {
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
			. $home/.esrc
		}
	}
}

