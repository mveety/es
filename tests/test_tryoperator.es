#!/usr/bin/env es

fn run_test_tryoperator_testerror a {
	match $a (
		error { throw error $0 testerror }
		noerror { result <=true }
		* { throw error $0 'invalid argument' }
	)
}

fn run_test_tryoperator {
	local (
		tests = (
			'a &?' '{try {a}}'
			'a |? b' '{%onerror {a} {b}}'
			'a ; b |? c' '{%seq {a} {%onerror {b} {c}}}'
			'result <={a &?} |& c' '{%seq {%background {%pipe {result <={try {a}}} 1 0 {}}} {c}}'
			'a |? b && c &? && d' '{%seq {try {%and {%onerror {a} {b}} {c}}} {%and {} {d}}}'
		)
	) {
		for ((l r) = $tests) {
			echo '%parsestring '''^$l^''' --> '^$r
			echo '    should be: '''^<={%parsestring $l}^''''
			assert {~ <={%parsestring $l} $r}
		}
	}
	local (e=; r=) {
		(e r) = <={run_test_tryoperator_testerror error &?}
		assert {$e && iserror $e}
		(e r) = <={run_test_tryoperator_testerror noerror &?}
		assert {! $e && ~ $e false && ~ $r 0}
		assert {run_test_tryoperator_testerror error |? result <=true}
		assert {run_test_tryoperator_testerror noerror |? result <=false}
		assert {run_test_tryoperator_testerror error |? {
			assert {iserror <=error}
			assert {~ <={<=error error} error}
			assert {~ <={<=error type} run_test_tryoperator_testerror}
			assert {~ <={<=error msg} testerror}
			result <=true
		}}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_tryoperator
}


