with-dynlibs mod_float {
	library float (init libraries)

	fn is-float num {
		if {! ~ $#num 1} { return <=false }
		result <={~ $num %re('[+-]?[0-9]+[.][ 0-9]*([e][+-]?[0-9]+)?')} #'
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
		fn remove_trailing_zeros str {
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
	}
}

