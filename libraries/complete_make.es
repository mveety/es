library complete_make (init completion)

fn complete_make_bin_exists bin {
	(e _) = <={try access -n $bin -1e -xf $path}
	if {$e} { return <=false }
	return <=true
}

fn complete_make_type bin {
	if {! complete_make_bin_exists $bin} {
		return notmake
	}
	if {~ ``(\n){ $bin -v >[2] /dev/null | head -n 1 } 'GNU Make'^*} {
		return gnumake
	}
	return bsdmake
}

fn complete_make_get_targets file {
	if {! access -r $file } { throw error complete_make no_makefile }
	let(targs=) {
		targs = ``(\n) {cat $file | awk 'BEGIN{ FS=":" } /^([^.%]|[-_a-zA-Z0-9~\/])[-_a-zA-Z0-9.~\/]+[[:space:]]*:.*$/ { print $1 }'}
		result <={process $targs ( * { es_complete_trim $matchexpr } )}
	}
}

fn complete_make_filter_targets file partial {
	let(res=) {
		for (i = <={complete_make_get_targets $file}){
			if {~ $i $partial^*} {
				res = $res $i
			}
		}
		result $res
	}
}

fn complete_make_hook maketype curline partial {
	match $maketype (
		bsdmake { result <={complete_make_filter_targets Makefile $partial} }
		gnumake {
			if {access -rf GNUmakefile} {
				return <={complete_make_filter_targets GNUmakefile $partial}
			} {
				return <={complete_make_filter_targets Makefile $partial}
			}
		}
		* { result () }
	)
}

let (maketype = <={complete_make_type make}) {
	if {! ~ $maketype notmake} {
		%complete_cmd_hook make @ curline partial {
			complete_make_hook $maketype $curline $partial
		}
	}
}

if {eq <={access -1 -n gmake $path |> %count} 1} {
	let (maketype = <={complete_make_type gmake}) {
		if {! ~ $maketype notmake} {
			%complete_cmd_hook gmake @ curline partial {
				complete_make_hook $maketype $curline $partial
			}
		}
	}
}

