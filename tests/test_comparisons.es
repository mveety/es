#!/usr/bin/env es

fn rtc_error str {
	throw error run_test_comparisons $str
}

fn run_test_comparisons {
	assert {! $&eq 0 1}
	assert {$&eq 100 100}
	assert {! $&gt 100 110}
	assert {$&gt 100 90}
	assert {! $&lt 100 90}
	assert {$&lt 100 110}
}

