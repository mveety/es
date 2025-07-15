#!/usr/bin/env es

fn run_test_string {
	local (
		exstring = 'this is a test hi there'
		res = ''
		elem = ''
		len = ''
		resstring = ''
		true = <=true
		false = <=false
	) {
		import string
		(res len) = <={string:length $exstring}
		assert2 test_string:length {~ $res $true}
		assert2 test_string:length {eq $len <={%count $:exstring}}
		res = ''
		(res elem) = <={string:find 'test' $exstring}
		assert2 test_string:find {~ $res $true}
		assert2 test_string:find {eq $elem 11}
		res = ''
		(res _) = <={string:find 'missing' $exstring}
		assert2 test_string:find {~ $res $false}
		(res resstring) = '' ''
		(res resstring) = <={string:slice $exstring 11 14}
		assert2 test_string:slice {~ $res $true}
		assert2 test_string:slice {~ $resstring 'test'}
		(res resstring) = '' ''
		(res resstring) = <={string:sub $exstring 11 4}
		assert2 test_string:sub {~ $res $true}
		assert2 test_string:sub {~ $resstring 'test'}
	}
}

