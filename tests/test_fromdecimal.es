#!/usr/bin/env es

fn run_test_fromdecimal {
	local (
		numbers = 10 -10 10 -10 10 -10 10 -10
		bases = dec dec bin bin oct oct hex hex
		results = 10 -10 0b1010 -0b1010 0o12 -0o12 0xa -0xa
	) {
		assert {eq $#numbers $#bases}
		assert {eq $#numbers $#results}

		for(num = $numbers; base = $bases; result = $results) {
			let (val = <={fromdecimal $base $num}) {
				echo -n '<={fromdecimal '^$base^' '^$num^' = '^$val^', '
				echo 'should be '^$result
				assert {~ $val $result}
			}
		}
	}
}

