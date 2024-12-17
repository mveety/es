#!/usr/bin/env es

library format_array (types format type_array)

fn fmt-array-elem num elem {
	local(elemfmt=) {
		elemfmt = $^num^'=' <={format $elem}
		result $"elemfmt
	}
}

fn fmt-array array {
	local (
		res=
		i=1
	) {
		for(elem = $array) {
			res = $res <={fmt-array-elem $i $$elem}
			i = <={add $i 1}
		}
		result 'array['^$^res^']'
	}
}

install_format 'array' @ v { result <={fmt-array $v} }

