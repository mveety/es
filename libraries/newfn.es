#!/usr/bin/env es

library newfn (init libraries)

fn newfn name argsbody {
	if {~ $#argsbody 0} {
		fn-^$name=
		return <=true
	}
	local (
		args = <={__es_getargs $argsbody}
		body = <={__es_getbody $argsbody}
	) {
		eval 'fn-'^$name^' = @ '^$^args^' { %block '^$^body^' }'
	}
}

