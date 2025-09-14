with-dynlibs mod_float {
	library float (init libraries)

	fn is-float num {
		if {! ~ $#num 1} { return <=false }
		if {$&fgt $num 9223372036854775806} { return true }
		if {$&flt $num -9223372036854775807} { return true }
		result <={~ $num %re('^[+-]?[0-9]+[.][ 0-9]*([e][+-]?[0-9]+)?$')} #'
	}

	let (
		fn-old_add = $fn-add
		fn-old_sub = $fn-sub
		fn-old_mul = $fn-mul
		fn-old_div = $fn_div
		fn normalize n {
			if {is-float $n} {
				result $n
			} {
				todecimal $n
			}
		}

		fn intlte a b {
			if {%numcompfun $&eq $a $b} {
				true
			} {%numcompfun $&lt $a $b} {
				true
			} {
				false
			}
		}
		fn intgte a b {
			if {%numcompfun $&eq $a $b} {
				true
			} {%numcompfun $&gt $a $b} {
				true
			} {
				false
			}
		}

		fn remove_trailing_zeros str {
			if {~ $str *^'e'^*} { return $str}
			local(strl=<={reverse $:str}; res=(); remove=true) {
				for (c = $strl) {
					if {! $remove || ! ~ $c 0} {
						res = $res $c
						remove = false
					}
				}
				reverse $res |> %string
			}
		}
	) {
		fn add a b {
			if {is-float $a || is-float $b} {
				let (
					an = <={normalize $a}
					bn = <={normalize $b}
				) {
					$&addf $an $bn |> remove_trailing_zeros
				}
			} {
				old_add $a $b
			}
		}

		fn sub a b {
			if {is-float $a || is-float $b} {
				let (
					an = <={normalize $a}
					bn = <={normalize $b}
				) {
					$&subf $an $bn |> remove_trailing_zeros
				}
			} {
				old_sub $a $b
			}
		}

		fn mul a b {
			if {is-float $a || is-float $b} {
				let (
					an = <={normalize $a}
					bn = <={normalize $b}
				) {
					$&mulf $an $bn |> remove_trailing_zeros
				}
			} {
				old_mul $a $b
			}
		}

		fn div a b {
			if {is-float $a || is-float $b} {
				let (
					an = <={normalize $a}
					bn = <={normalize $b}
				) {
					$&divf $an $bn |> remove_trailing_zeros
				}
			} {
				old_div $a $b
			}
		}

		fn eq a b {
			if {~ $#a 0} { return <=true}
			if {~ $#b 0} { b = 0 }
			if {is-float $a || is-float $b} {
				$&feq $a $b
			} {
				%numcompfun $&eq $a $b
			}
		}

		fn gt a b {
			if {~ $#a 0} { return <=false}
			if {~ $#b 0} { b = 0 }
			if {is-float $a || is-float $b} {
				$&fgt $a $b
			} {
				%numcompfun $&gt $a $b
			}
		}

		fn gte a b {
			if {~ $#a 0} { return <=true}
			if {~ $#b 0} { b = 0 }
			if {is-float $a || is-float $b} {
				$&fgte $a $b
			} {
				intgte $a $b
			}
		}

		fn lt a b {
			if {~ $#a 0} { return <=false}
			if {~ $#b 0} { b = 0 }
			if {is-float $a || is-float $b} {
				$&flt $a $b
			} {
				%numcompfun $&lt $a $b
			}
		}

		fn lte a b {
			if {~ $#a 0} { return <=true}
			if {~ $#b 0} { b = 0 }
			if {is-float $a || is-float $b} {
				$&flte $a $b
			} {
				intlte $a $b
			}
		}

		noexport += is-float
		noexport += add sub mul div eq gt gte lt lte
	}
}

