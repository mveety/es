#!/usr/bin/env es

fn run_test_process {
	i = 1
	testl = hello goodbye crap stuff x234
	goodl = 0 1 wow error good
	resl = <={
		process $testl (
			(he*) { result 0}
			(*ood*) { result 1}
			([cC][rR]*p) { result wow }
			(*2*4) { result good }
			* { result error }
		)
	}
	echo 'goodl =' $goodl
	echo 'resl =' $resl
	while {lte $i $#goodl} {
		assert {~ $resl($i) $goodl($i)}
		i = <={add $i 1}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_process
}

