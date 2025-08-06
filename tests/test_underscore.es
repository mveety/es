#!/usr/bin/env es

fn run_test_underscore {
	assert {~ $#_ 0}
	let((a1 _ c1) = 1 2 3) {
		echo 'a1 =' $a1
		echo 'c1 =' $c1
		assert {~ $a1 1}
		assert {~ $c1 3}
		assert {~ $#_ 0}
	}

	local((a2 _ c2) = a1 b2 c3) {
		echo 'a2 =' $a2
		echo 'c2 =' $c2
		assert {~ $a2 a1}
		assert {~ $c2 c3}
		assert {~ $#_ 0}
	}
	let (testlist = a 1 b 2 c 3 d 4 e 5 f 6 g 7 h 8 i 9 j 0) {
		for ((_ i1) = $testlist) {
			echo 'i1 =' $i1
			assert {~ $i1 1 2 3 4 5 6 7 8 9 0}
		}
		for ((i2 _) = $testlist) {
			echo 'i2 =' $i2
			assert {~ $i2 a b c d e f g h i j}
		}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_underscore
}

