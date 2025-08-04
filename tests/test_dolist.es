#!/usr/bin/env es

fn run_test_dolist {
	let(a = 0; b = 0) {
		for (i = <={%range 1 100}) {
			a = <={add $a $i}
		}
		%range 1 100 |> dolist @{ b = <={add $b $1 } }
		assert {eq $a $b}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_dolist
}

