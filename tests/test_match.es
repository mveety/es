#!/usr/bin/env es

fn run_test_match {
	let	(x = 2) {
		# check to see if case ordering is correct
		# the wildcard case should always be tested
		# last.
		match $x (
			* { unreachable }
			1 { result ok }
			2 { result ok }
			3 { result ok }
		)

		# this is a test for comprehensive matches
		# if the C debug flag is set comp matches are enabled
		# and $__es_comp_match is true.
		if {$__es_comp_match} {
			catch @ e rest {
				if {~ $e assert && ! ~ $rest bad_unreachable} {
					result ok
				} {
					throw $e $rest
				}
			} {
				match $x (
					1 { result ok }
					3 { result ok }
				)
				throw assert bad_unreachable
			}
		}
	}

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

