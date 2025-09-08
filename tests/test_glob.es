#!/usr/bin/env es

# broadly nicked from wryun/es-shell
fn run_test_glob {
	assert {~ '' ''}
	assert {! ~ x ''}
	assert {! ~ '' x}
	assert {~ abc abc}
	assert {! ~ abc 123}
	assert {~ abc *}
	assert {~ abc a*}
	assert {~ abc *c}
	assert {! ~ abc *b}
	assert {~ abc *b*}
	assert {~ a a*}
	assert {~ a *a}
	assert {~ axbxcxdxe	a*b*c*d*e*}
	assert {~ axbxcxdxexxx	a*b*c*d*e*}
	assert {~ abxbbxdbxebxczzx	a*b?c*x}
	assert {! ~ abxbbxdbxebxczzy	a*b?c*x}
	assert {! ~ aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa	a*a*a*a*b}
	assert {~ aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab	a*a*a*a*b}

	for ((test want) = (
		{~ x [x]} true
		{~ x [y]} false
		{~ x [xyz]} true
		{~ y [xyz]} true
		{~ z [xyz]} true
		{~ x [xy  } false
		{~ [xy  [xy } true
		{~ [xy] [xy]} false
		{~ px p[~a]} true
		{~ pa p[~a]} false
		{~ p  p[~a]} false	# https://github.com/rakitzis/rc/issues/115
		{~ pa p[a~]} true
		{~ p~ p[a~]} true
		{~ ab a[a-z]b} false
		{~ axb a[a-z]b} true
		{~ a-b a[a-z]b} false
		{~ a-b a[-az]b} true
		{~ axb a[-az]b} false
		{~ azb a[-az]b} true
		{~ a-b a[a\-z]b} true
		{~ axb a[a\-z]b} false
		{~ azb a[a\-z]b} true
		{~ axb a[~a-z]b} false
		{~ a-b a[~a-z]b} true
		{~ axb a[a-z~]b} true
		{~ a-b a[a-z~]b} false
	)) {
		assert {~ <={$test} <={$want}}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_glob
}

