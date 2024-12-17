#!/usr/bin/env es

library format_dict (types format dict type_dict)

fn fmt_dict_interleave l1 l2 {
	local (
		list1 = $$l1
		list2 = $$l2
		i = 0
		res =
	) {
		assert {~ $#list1 $#list2}
		while {lt $i $#list1} {
			res = $res $list1(<={add $i 1}) $list2(<={add $i 1})
			i = <={add $i 1}
		}
		assert2 $0 {~ <={mod $#res 2} 0}
		result $res
	}
}

fn fmt-dict-elem key elem {
	local(elemfmt=) {
		elemfmt = '('^$^key^')=' <={format $elem}
		result $"elemfmt
	}
}

fn fmt-dict dict {
	local (
		res=
		interdict = <={fmt_dict_interleave $dict}
	) {
		for((key value) = $interdict) {
			res = $res <={fmt-dict-elem <={format $$key} $$value}
		}
		result 'dict['^$^res^']'
	}
}

install_format 'dict' @ v { result <={fmt-dict $v} }

