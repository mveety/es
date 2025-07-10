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

fn do2 fun iter1 iter2 {
	let (res=; x = <=$iter1; y = <=$iter2) {
		while {! ~ $#x 0 && ! ~ $#y 0} {
			res = $res <={$fun $x $y}
			x = <=$iter1
			y = <=$iter2
		}
		result $res
	}
}

fn accumulate start fun iter {
	let (res = $start; x = <=$iter) {
		while {! ~ $#x 0} {
			res = <={$fun $res $x}
			x = <=$iter
		}
		result $res
	}
}

fn accumulate2 start fun iter1 iter2 {
	let (res = $start; x = <=$iter1; y = <=$iter2) {
		while {! ~ $#x 0 && ! ~ $#y 0} {
			res = <={$fun $res $x $y}
			x = <=$iter1
			y = <=$iter2
		}
		result $res
	}
}


fn dolist fun list {
	iterator $list |> do $fun
}

