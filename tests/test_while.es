#!/usr/bin/env es

fn run_test_while {
	let (
		t1 = <={let (x = 0) {
			while {lt $x 10 } {
				x = <={add $x 1}
			}
			result $x
		}}
		t2 = <={let (x = 10) {
			while {lt $x 10 } { x = 10 } { x = 13 }
			result $x
		}}
		t3 = <={let (x = 0) {
			while {lt $x 10 } {
				x = <={add $x 1}
			} {
				x = <={sub $x 1}
			}
			result $x
		}}
	) {
		assert {eq $t1 10}
		assert {eq $t2 13}
		assert {eq $t3 10}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_while
}

