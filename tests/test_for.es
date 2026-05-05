#!/usr/bin/env es

# do a quick test to make sure for loops actually work

fn run_test_for {
	let (i = 0) {
		# check to make sure idiomatic loops don't break
		for (_ = <={%range 1 100}) {
			i = <={add $i 1}
		}
		assert {eq $i 100}
	}

	let (i = 1) {
		# check to see if normal loops still work
		for (n = <={%range 1 100}) {
			assert {eq $i $n}
			i = <={add $i 1}
		}
	}

	let (l = (a b c d e f g);i = 1) {
		for(v = $l) {
			assert {~ $v $l($i)}
			i = <={add $i 1}
		}
	}

	let (list = (a b c d e f g h i j); i = 1) {
		for ((l m) = $list) {
			assert {~ $l $list($i)}
			assert {~ $m $list(<={add $i 1})}
			i = <={add $i 2}
		}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_for
}

