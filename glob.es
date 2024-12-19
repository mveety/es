#!/usr/bin/env es

fn glob patternstr list {
	if {~ $#list 0 } {
		result <={eval 'result '^$^patternstr}
	} {
		local(res=) {
			for(i = $list) {
				if {eval '~ '^$i^' '^$^patternstr} {
					res = $res $i
				}
			}
			result $res
		}
	}
}

