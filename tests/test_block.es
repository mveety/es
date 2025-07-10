#!/usr/bin/env es

fn run_test_block {
	let(y = ; res = 5 4 3 2 1) {
		%block {
			defer {y = $y 1}
			defer {y = $y 2}
			defer {y = $y 3}
			defer {y = $y 4}
			y = $y 5
		}
		for (i = $y; j = $res) { assert {eq $i $j} }
	}
}

