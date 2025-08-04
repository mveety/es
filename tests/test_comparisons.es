#!/usr/bin/env es

fn rtc_error str {
	throw error run_test_comparisons $str
}

fn run_test_comparisons {
	assert {! eq 0 1}
	assert {! eq 0o10 0x7}
	assert {eq 100 100}
	assert {eq 0x100 256}
	assert {! gt 100 110}
	assert {! gt -0x44 0o77}
	assert {gt 100 90}
	assert {gt 0b100 -0o74}
	assert {! lt 100 90}
	assert {! lt 0x100 0o45}
	assert {lt 100 110}
	assert {lt 0b100 0x100}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_comparisons
}

