#!/usr/bin/env es

fn run_test_assert {
	catch @ e t m {
		if {! ~ $e assert || ! ~ $t false} {
			throw $e $t $m
		}
	} {
		assert false
	}

	catch @ e t m {
		if {! ~ $e assert || ! ~ $t 'run_test_assert: false'} {
			throw $e $t $m
		}
	} {
		assert2 run_test_assert false
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_assert
}

