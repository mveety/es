#!/usr/bin/env es

fn run_test_do {
	let(c = 0) {
		%range 1 100 |> iterator |> do @{ c = <={add $c 1} }
		assert {eq $c 100}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_do
}

