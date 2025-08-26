#!/usr/bin/env es

fn run_test_tryoperator {
	local (
		tests = (
			'a &?' '{try {a}}'
			'a |? b' '{%onerror {a} {b}}'
			'a ; b |? c' '{%seq {a} {%onerror {b} {c}}}'
			'result <={a &?} |& c' '{%seq {%background {%pipe {result <={try {a}}} 1 0 {}}} {c}}'
		)
	) {
		for ((l r) = $tests) {
			echo '%parsestring '''^$l^''' --> '^$r
			echo '    should be: '''^<={%parsestring $l}^''''
			assert {~ <={%parsestring $l} $r}
		}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_tryoperator
}


