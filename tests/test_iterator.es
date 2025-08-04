#!/usr/bin/env es

fn run_test_iterator {
	let (tmp = <={%range 1 100 |> iterator}; c = 0; x=) {
		x = <=$tmp
		while {! ~ $#x 0} {
			c = <={add $c 1}
			x = <=$tmp
		}
		assert {eq $c 100}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_iterator
}

