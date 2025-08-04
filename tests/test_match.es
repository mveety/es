#!/usr/bin/env es

fn run_test_match {
	testl = hello 0 goodbye 1 crap wow stuff error x234 good
	for((i j) = $testl) {
		r = <={
			match $i (
				(he*) { result 0}
				(*ood*) { result 1}
				([cC][rR]*p) { result wow }
				(*2*4) { result good }
				* { result error }
			)
		}
		echo 'case '^$i^': $j = '^$j^', $r = '^$r
		assert {~ $j $r}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_match
}

