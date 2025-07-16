#!/usr/bin/env es

fn run_test_todecimal {
	local (
		numbers = 10 010 00010 -10 -010 0x10 0x010 -0x10 0b10 0b010 -0b10 \
			0d10 0d010 -0d10 0o10 0o010 -0o10
		results = 10 10 10 -10 -10 16 16 -16 2 2 -2 10 10 -10 8 8 -8
		bases = dec dec dec dec dec hex hex hex bin bin bin dec dec dec oct oct oct
	) {
		assert {eq $#numbers $#results}
		assert {eq $#numbers $#bases}
		for (testn = $numbers; resultn = $results; basen = $bases) {
			let ((base val) = <={todecimal1 $testn}) {
				echo -n '<={todecimal1 '^$testn^'} = '^$base^' '^$val^', '
				echo 'should be '^$basen^' '^$resultn
				assert {~ $base $basen}
				assert {eq $val $resultn}
			}
		}
		for (testn = $numbers; resultn = $results) {
			let(val = <={todecimal $testn}) {
				echo -n '<={todecimal '^$testn^'} = '^$val^', '
				echo 'should be '^$resultn
				assert {eq $val $resultn}
			}
		}
	}
}

