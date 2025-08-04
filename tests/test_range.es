#!/usr/bin/env es

fn run_test_range {
	assert {~ range <=$&primitives}
	local (range1 = <={$&range 1 10}; i = 1) {
		assert {~ $#range1 10}
		for (x = $range1) {
			assert {eq $x $i}
			i = <={add $i 1}
		}
	}
	catch @ e type msg {
		if {~ $e error && ~ $type '$&range' && ~ $msg 'start > end'} {
			true
		} {
			throw $e $type $msg
		}
	} {
		$&range 10 1
		throw error not_reached 'point should not be reached'
	}
	local (range2 = <={%range 1 10}; i = 1) {
		assert {~ $#range2 10}
		for (x = $range2) {
			assert {eq $x $i}
			i = <={add $i 1}
		}
	}
	local (range3 = <={%range 10 1}; i = 10) {
		assert {~ $#range3 10}
		for (x = $range3) {
			assert {eq $x $i}
			i = <={sub $i 1}
		}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_range
}

