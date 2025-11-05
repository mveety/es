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

esmle_conf_highlight = \e^'[46m'^\e^'[39m'
# esmle:highlight is stored formatted in es land so relay that to
# conf. the actual highlight data is stored inside of the editor
esmle_conffmt_highlight = 'formatted'
get-esmle_conf_highlight = $&esmlegethighlight
set-esmle_conf_highlight = @ arg {
	if {~ $arg none || ~ $#arg 0} {
		$&esmlesethighlight
	} {
		$&esmlesethighlight <={%string $arg}
	}
}

fn _esmle_genfunction key func {
	local (funname = <={gensym __esmle_^$key^_hook_}) {
		fn-$funname = $func
		result $funname
	}
}

fn _esmle_getkeyfunction key {
	for ((k f) = <=$&getkeymap) {
		if {~ $key $k} {
			if {! ~ $f %esmle:*} {
				return true $f
			}
		}
	}
	return false
}

fn %mapkey keyname funname {
	if {~ <={$&termtypeof $funname} closure} {
		funname = <={_esmle_genfunction $keyname $funname}
	}
	$&mapkey $keyname $funname
}

fn %unmapkey keyname {
	let ((haskeyfun keyfun) = <={_esmle_getkeyfunction $keyname}) {
		if {~ $keyfun __esmle_^$keyname^_hook_*} {
			fn-^$keyfun =
		}
	}
	$&unmapkey $keyname
}

fn %mapaskey keyname srckeyname {
	$&mapaskey $keyname $srckeyname
}

fn %clearkey keyname {
	let ((haskeyfun keyfun) = <={_esmle_getkeyfunction $keyname}) {
		if {~ $keyfun __esmle_^$keyname^_hook_*} {
			fn-^$keyfun =
		}
	}
	$&clearkey $keyname
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
					if {~ $f __esmle_^$k^_hook_*} {
						let (tmpvar = fn-^$f) {
							f = $$tmpvar
						}
					}
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

