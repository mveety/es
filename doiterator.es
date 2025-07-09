#!/usr/bin/env es

fn iterator list {
	result @{
		let(x = $list(1)) {
			list = $list(2 ...)
			result $x
		}
	}
}

fn iterator2 list {
	let(list1=;list2=) {
		for((i j) = $list) {
			list1 = $list1 $i
			list2 = $list2 $j
		}
		result <={iterator $list1} <={iterator $list2}
	}
}

fn do fun iter {
	let (res=; x = <=$iter) {
		while {! ~ $#x 0} {
			res = $res <={$fun $x}
			x = <=$iter
		}
		result $res
	}
}

fn dolist fun list {
	iterator $list |> do $fun
}

