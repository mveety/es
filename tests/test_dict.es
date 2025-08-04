#!/usr/bin/env es

fn run_test_dict {
	let (
		x = <=dictnew
		dnames = <={%range 1 100}
		dvals = <={%range 1 100 |> dolist @ x {mul $x 10}}
	) {
		assert {~ $x 'dict('^*^')'}
		for (i = $dnames; j = $dvals) {
			x = <={dictput $x $i $j}
		}
		assert {eq <={dictsize $x} <={%count $dnames}}
		for (i = <={dictnames $x}) {
			assert {~ $i $dnames}
		}
		for (i = <={dictvalues $x}) {
			assert {~ $i $dvals}
		}
		dictforall $x @ n v {
			echo '$x['^$n^'] = '''^$v^''''
			assert {~ $#n 1}
			assert {~ $#v 1}
			assert {~ $n $dnames}
			assert {~ $v $dvals}
		}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_dict
}

