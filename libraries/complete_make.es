library complete_make (init completion)

_complete_make_lastcwd = ''
_complete_make_lastmd5 = ''
_complete_make_last_targets = ''

fn complete_make_get_targets {
	if {! access -r Makefile } { throw error complete_make no_makefile }
	let(targs=) {
		targs = ``(\n) {cat Makefile | awk 'BEGIN{ FS=":" } /^[-_a-zA-Z0-9.]+[[:space:]]*:.*$/ { print $1 }'}
		result <={process $targs ( * { es_complete_trim $matchexpr } )}
	}
}

fn complete_make_get_md5sum {
	if {! access -r Makefile } { throw error complete_make no_makefile }
	result `{md5sum Makefile | awk '{print $1}'}
}

fn complete_make_filter_targets partial {
	let(res=) {
		for(i = $_complete_make_last_targets){ 
			if {~ $i $partial^*} {
				res = $res $i
			}
		}
		result $res
	}
}

fn complete_make_update_data {
	_complete_make_lastcwd = `{pwd}
	_complete_make_lastmd5 = <={complete_make_get_md5sum}
	_complete_make_last_targets = <={complete_make_get_targets}
}

fn complete_make_hook curline partial {
	if {! access -r Makefile } {
		_complete_make_lastcwd = ''
		_complete_make_lastmd5 = ''
		return ()
	}
	if {! ~ $_complete_make_lastcwd `{pwd}} {
		complete_make_update_data
	}
	if {! ~ $_complete_make_lastmd5 <={complete_make_get_md5sum}} {
		complete_make_update_data
	}

	complete_make_filter_targets $partial
}

%complete_cmd_hook make @ curline partial {
	complete_make_hook $curline $partial
}

if {eq <={access -1 -n gmake $path |> %count} 1} {
	%complete_cmd_hook gmake @ curline partial {
		complete_make_hook $curline $partial
	}
}

