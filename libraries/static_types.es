#!/usr/bin/env es

library static_types (types)

# quick and dirty implementation of static types and constants
# using the type library. this is pretty limited, but can work
# well for some applications

fn asserttype v typ {
	assert2 $0 {gte $#typ 1 && eq $#v 1}
	if {~ $#typ 1} {
		if {! ~ $typ <={typeof $$v}} {
			throw error type '$'^$v^' is not type '^$typ
		}
	} {
		if {! ~ <={%count $$v} $#typ} {
			throw error type '$'^$v^' type is invalid'
		}
		local(i=1;vv=$$v){
			for(t = $typ) {
				if {! ~ <={typeof $vv($i)} $t} {
					throw error type '$'^$v^'('^$i^') is not type '^$t
				}
			}
		}
	}
}

fn static vname {
	assert2 $0 {! ~ <={%count $$vname} 0}

	let(vtype = <={typeof $$vname}) {
		set-$vname = @ v {
			if {! ~ <={typeof $v} $vtype} {
				throw error type 'unable to assign '^<={typeof $v}^' to '^$vtype^' $'^$vname
			}
			result $v
		}
	}
	result <=true
}

fn const vname {
	assert2 $0 {! ~ <={%count $$vname} 0}

	set-$vname = @ { throw error type 'attempt to set const $'^$vname }
	result <=true
}

fn defstatic t v d {
	assert2 $0 {eq $#t 1 && eq $#v 1 && gte $#d 0}
	assert2 $0 {~ <={%count $$v} 0}
	assert2 $0 {~ $t <={typeof $d}}
	$v = $d
	static $v
}

fn defconst v d {
	assert2 $0 {eq $#v 1 && gte $#d 1}
	assert2 $0 {~ <={%count $$v} 0}
	$v = $d
	const $v
}

fn undefine vs {
	assert2 $0 {gte $#vs 1}
	for(v = $vs) {
		set-$v=
		get-$v=
		$v=
	}
}

