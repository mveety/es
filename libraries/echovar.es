#!/usr/bin/env es

library echovar (types format)

fn fmtvar v {
	result $v^'='^<={format $$v}
}

fn echovar v {
	if {~ $v(1) -n} {
		echo -n <={fmtvar $v}
	} {
		echo <={fmtvar $v}
	}
}

