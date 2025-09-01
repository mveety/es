#!/usr/bin/env es

fn run_test_dict {
	local (
		x = <=dictnew
		y =
		dnames = <={%range 1 100}
		dvals = <={%range 1 100 |> dolist @ x {mul $x 10}}
		sdict = '%dict(1=>2;3=>4;a=>b;c=>''hello world'';d=>1 2 3 4 5)'
	) {
		assert {~ $x '%dict('^*^')'}

		for (i = $dnames; j = $dvals) {
			x = <={dictput $x $i $j}
		}
		for (assoc = <={%fsplit ';' <={~~ $x '%dict('^*^')'}}){
			assert {~ $assoc *^'=>'^*}
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
		local (
			fn-test1 = @ nm {
				dictforall $x @ n _ {
					if {~ $n $nm} {
						return <=true
					}
				}
				result <=false
			}
		) {
			assert {test1 54}
			assert {test1 81}
			assert {! test1 105}
			assert {! test1 -100}
			assert {test1 10}
			assert {! test1 dicks}
		}
		y = %dict(a=>b;c=>d;e=>f)
		assert {~ $y '%dict(a => b; c => d; e => f)'}
		assert {~ $y(a) b}
		assert {~ $y(c) d}
		assert {~ $y(e) f}
		assert {<={result $y(g) onerror result true}}
		y := 1 => 2
		y := 3 => 4
		assert {~ $y(1) 2}
		assert {~ $y(3) 4}
		y := 1 =>
		y := 3 =>
		assert {<={result $y(1) onerror result true}}
		assert {<={result $y(3) onerror result true}}
		assert {~ <={$&termtypeof $sdict} string}
		sdict := e => 'we should be a dict now'
		assert {~ <={$&termtypeof $sdict} dict}
		assert {~ $sdict(e) 'we should be a dict now'}
		assert {~ $sdict(c) 'hello world'}
		assert {~ <={%count $sdict(d)} 5}
		sdict := * => true
		assert {$sdict(*)}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_dict
}

