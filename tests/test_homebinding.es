#!/usr/bin/env es

fn run_test_homebinding {
	let (tvar = no) let (fn-%home = @{ tvar = yes; $&home $* }) {
		echo ~
		assert {~ $tvar yes}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_homebinding
}

