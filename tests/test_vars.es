#!/usr/bin/env es

fn get_vars_values varsfn {
	iterator ``(\n) $varsfn |>
		do @{
			%fsplit '=' $1 |>
				@{ result <={es_complete_right_trim $1}} |>
				@{ if {! ~ $1 last } { result $1}} |>
				@{ if {! ~ $1 prompt} { result $1}}
		} |> result
}

fn run_test_vars {
	lets (
		olditer = <={get_vars_values @{ $fn-es_old_vars -a} |> iterator}
		newiter = <={get_vars_values @{ $fn-es_new_vars -a} |> iterator}
		x=;y=;
	) {
		x = <=$olditer
		y = <=$newiter
		while {! ~ $#x 0 && ! ~ $#y 0} {
			assert {~ $x $y}
			x = <=$olditer
			y = <=$newiter
		}
		assert {~ $x $y}
	}
}

