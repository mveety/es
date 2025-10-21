fn %mapkey keyname funname {
	$&mapkey $keyname $funname
}

fn %unmapkey keyname {
	$&unmapkey $keyname
}

fn %mapaskey keyname srckeyname {
	$&mapaskey $keyname $srckeyname
}

fn %clearkey keyname {
	$&clearkey $keyname
}

fn keymap dict {
	if {~ $#dict 0} {
		let (res = <=dictnew) {
			for ((k f) = <=$&getkeymap) {
				res := $k => $f
			}
			result $res
		}
	} {
		if {! ~ <={$&termtypeof $dict} dict} {
			throw error keymap 'usage: keymap [keymap dict]'
		}
		dictforall $dict @ k f {
			%mapkey $k $f
		}
		keymap
	}
}

