#!/usr/bin/env es

test_fail_fail_pass = true

fn run_test_fail {
	echo 'this test should fail on us'
	throw error test_$testname 'this test has failed!'
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_fail
}

