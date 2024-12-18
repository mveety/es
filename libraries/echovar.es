#!/usr/bin/env es

library echovar (types format)

fn fmtvar v d {
	result $v^'='^<={format $d}
}

fn echovar v {
	if {~ $v(1) -n} {
		v = $v(2)
		echo -n <={fmtvar $v $$v}
	} {
		echo <={fmtvar $v $$v}
	}
}

fn echovar2 v d {
	let(echoargs=){
		if {~ $v -n} {
			echoargs = '-n'
			v = $d(1)
			d = $d(2 ...)
		}
		echo $echoargs <={fmtvar $v $d}
	}
}

