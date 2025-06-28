#!/usr/bin/env es

fn run_test_frombase {
	local (
		testn = 12345
		tests = (16 3039 10 12345 8 30071
				 2 11000000111001)
	) {
		for ((base res) = $tests) {
			echo '$testn='^$testn^', $base = '^$base^', $res = '^$res
			assert {~ <={$&frombase $base $res} $testn}
		}
	}
}

