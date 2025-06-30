#!/usr/bin/env es

fn es_canonicalize_args args {
	let (res=;tmp=) {
		for (i = $args) {
			if {~ $i '-'^*} {
				tmp = <={~~ $i '-'^*}
				if {eq <={%count $:tmp} 0} {
					res = $res '-'
				} {gt <={%count $:tmp} 1} {
					res = $res '-'^<={%strlist $tmp}
				} {
					res = $res $i
				}
			} {
				res = $res $i
			}
		}
		result $res
	}
}

fn es_run_parseargs argfn usagefn args {
	local (
		i = 1
		argslen = $#args
		cur =
		next =
		advance = 1
		fn-flagarg = @{
			if {~ $next '-'^*} {
				throw parseargs_error
			} {~ $#next 0 } {
				throw parseargs_error
			} {
				advance = 2
				result $next
			}
		}
		fn-usage = @{ throw parseargs_error }
		fn-done = @{ throw parseargs_done }
	) {
		catch @ e t m {
			if {~ $e parseargs_error} {
				$usagefn
				result <=false
			} {~ $e parseargs_done} {
				result <=true $args
			} {
				throw $e $t $m
			}
		} {
			while {lte $i $argslen} {
				advance = 1
				cur = $args(1)
				if {! ~ $cur '-'^*} { throw parseargs_done }
				next = $args(2)
				$argfn $cur
				i = <={add $i $advance}
				args = $args(<={add 1 $advance} ...)
			}
			result <=true $args
		}
	}
}

fn parseargs argfn maybe-usagefn maybe-args {
	if {~ $maybe-usagefn '@'^*} {
		es_run_parseargs $argfn $maybe-usagefn <={es_canonicalize_args $maybe-args}
	} {
		es_run_parseargs $argfn \
			@{throw error usage 'invalid arguments'} \
			<={es_canonicalize_args $maybe-usagefn $maybe-args}
	}
}

