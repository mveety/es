library esmle (init)

esmle_conf_match-braces = 'false'
set-esmle_conf_match-braces = $&editormatchbraces

esmle_conf_terminal = 'unknown'
get-esmle_conf_terminal = $&esmlegetterm

esmle_conf_word-start = ''
get-esmle_conf_word-start = $&esmlegetwordstart
set-esmle_conf_word-start = @ x {
	match $x (
		(first-letter first-break last-break) { $&esmlesetwordstart $x }
		* { $&esmlegetwordstart }
	)
}

fn %mapkey keyname funname {
	if {~ <={$&termtypeof $funname} closure} {
		fn-^$keyname^-hook = $funname
		funname = $keyname^-hook
	}
	$&mapkey $keyname $funname
}

fn %unmapkey keyname {
	$&unmapkey $keyname
	let (keyfun = fn-^$keyname^-hook) {
		if {gt <={%count $$keyfun} 0} {
			$$keyfun =
		}
	}
}

fn %mapaskey keyname srckeyname {
	$&mapaskey $keyname $srckeyname
}

fn %clearkey keyname {
	$&clearkey $keyname
	let (keyfun = fn-^$keyname^-hook) {
		if {gt <={%count $$keyfun} 0} {
			$keyfun =
		}
	}
}

fn keymap args {
	let (print = false){
		if {! ~ $#args 0} {
			if {~ <={%termtypeof $args} dict} {
				dict = $args
			} {~ $args(1) -v} {
				print = true
				args = $args(2 ...)
				if {~ <={%termtypeof $args} dict} {
					dict = $args
				}
			}
		}
		if {~ $#args 0} {
			let (res = <=dictnew) {
				for ((k f) = <=$&getkeymap) {
					res := $k => $f
				}
				if {$print} {
					dictforall $res @ k f {
						echo $k^': '^$f
					}
				}
				result $res
			}
		} {
			if {! ~ <={%termtypeof $dict} dict} {
				throw error keymap 'usage: keymap [keymap dict]'
			}
			dictforall $dict @ k f {
				if {~ $f '%esmle:Clear'} {
					%clearkey $k
				} {
					%mapkey $k $f
				}
			}
			if {$print} {
				keymap -v
			} {
				keymap
			}
		}
	}
}

