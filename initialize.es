#!/usr/bin/env es

fn %initialize {
	if {$__es_initialize_esrc} {
		echo 'reading esrc in %initialize'
		if {$__es_loginshell && $__es_readesrc} {
			. $home/.esrc
		}
	}
}

