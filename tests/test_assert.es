#!/usr/bin/env es

fn run_test_assert {
	catch @ e t m {
		if {! ~ $e error || ! ~ $t assert || ! ~ $m false} {
			throw $e $t $m
		}
	} {
		assert false
	}

	catch @ e t m {
		if {! ~ $e error || ! ~ $t assert || ! ~ $m 'run_test_assert: false'} {
			throw $e $t $m
		}
	} {
		assert2 run_test_assert false
	}
}

