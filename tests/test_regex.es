#!/usr/bin/env es

fn run_test_regex {
	# really basic tests for regex functionality. basically if these pass es is good to go
	# generally if there are problems with the regex themselves that's a platform problem
	# more than anything. might pay dividends to either write or import a portable regex
	# implementation instead of using the host's and hoping for the best. i think though
	# basically all impls are posix compliant. the BSDs at least all uses henry spencer's
	# implementation i think.
	assert {~ 'hello' %re('h.*')}
	assert {! ~ 'hello' %re('x.*')}
	assert {~ '123abc456' %re('[0-9]+[a-z]+[0-9]+')}
	assert {~ ('hello' '123abc456') %re('^[0-9]+$') %re('h.*')}
	assert {~ ('hello' '123abc456') stuff %re('^[0-9]+$') h* test %re('x.*')}
	assert {! ~ ('hello' '123abc456') more stuff %re('^[0-9]+$') [0-9][a-z][0-9] %re('x.*')}
	assert {~ ('oh no a test' '3b6') more stuff %re('^[0-9]+$') [0-9][a-z][0-9] %re('x.*')}
	assert {~ ('hello' 123abc456) %re('^[0-9]+[a-z]+.*$') %re('x.*')}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_regex
}

