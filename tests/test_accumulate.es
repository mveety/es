#!/usr/bin/env es

fn run_test_accumulate {
	%range 1 100 |> iterator2 |>
		do2 @ a b { result <={sub $b $a} } |> iterator |>
		accumulate 0 @ r e { add $r $e } |> @{assert {eq $1 50}}
}

