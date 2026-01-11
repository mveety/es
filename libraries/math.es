# mod_math depends on symbols in mod_float.
with-dynlibs mod_float mod_math {
	library math (init libraries float)
#@libutil_hint_functions cbrt sqrt hypot sin asin
#@libutil_hint_functions sinh asinh cos acos cosh
#@libtuil_hint_functions acosh tan atan tanh atanh
#@libutil_hint_functions ceil floor log exp log10
#@libutil_hint_functions log2 exp2 pow intpow

#@libutil_hint_ignore_function undefine-prim-wrapper
#@libutil_hint_ignore_function define-prim-wrapper
#@libutil_hint_ignore_function mod_math_get_prims

	let (
		fn undefine-prim-wrapper prefix prim {
			local (
				fn_name = <={
					if {$prefix} {
						result fn-math_$prim
					} {
						result fn-$prim
					}
				}
				prim_name = '$&'^$prim
			) {
				$fn_name =
			}
		}

		fn define-prim-wrapper prefix prim {
			local (
				fn_name = <={
					if {$prefix} {
						result fn-math_$prim
					} {
						result fn-$prim
					}
				}
				prim_name = '$&'^$prim
			) {
				if {! ~ <={%count $$fn_name} 0} {
					$fn_name =
				}
				$fn_name = $prim_name
				if {$prefix} {
					%hidefunction math_$prim
				} {
					%hidefunction $prim
				}
			}
		}
		prims = (
			cbrt sqrt hypot sin asin
			sinh asinh cos acos cosh
			acosh tan atan tanh atanh
			ceil floor log exp log10
			log2 exp2 pow intpow
		)
	) {
		set-math_conf_prefix-wrappers = @ arg args {
			if {! ~ $arg true false} {
				result $math_conf_prefix-wrappers
			}
			if {$math_conf_prefix-wrappers && ! $arg} {
				for (p = $prims) {
					undefine-prim-wrapper true $p
					define-prim-wrapper false $p
				}
			} {! $math_conf_prefix-wrappers && $arg} {
				for (p = $prims) {
					undefine-prim-wrapper false $p
					define-prim-wrapper true $p
				}
			} {~ $#math_conf_prefix-wrappers 0} {
				if {$arg} {
					for (p = $prims) {
						undefine-prim-wrapper false $p
						define-prim-wrapper true $p
					}
				} {
					for (p = $prims) {
						undefine-prim-wrapper true $p
						define-prim-wrapper false $p
					}
				}
			}
			return $arg
		}

		set-math_conf_enable-build = @ arg args {
			if {! ~ $arg true false} {
				result $math_conf_enable-build
			}
			if {$arg} {
				fn mod_math_get_prims {
					if {! access -r mod_math.c} {
						throw error $0 'mod_math.c not found'
					}
					local (prims=()) {
						for(line = ``(\n){cat mod_math.c | grep PRIM}) {
							if {~ $line 'DYNPRIMS'^*} { continue }
							prims +=  <={~~ $line 'PRIM('^*^') {'}
						}
						return $prims
					}
				}
			} {
				fn mod_math_get_prims
			}
			return $arg
		}


		if {~ $#math_conf_prefix-wrappers 0} {
			math_conf_prefix-wrappers = false
		}

		if {~ $#math_conf_enable-build 0} {
			math_conf_enable-build = false
		}

	}
}
		
