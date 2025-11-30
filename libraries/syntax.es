

with-dynlibs mod_syntax {
	library syntax (init libraries esmle colors)

	defconf syntax debugging false
	defconf syntax colors %dict(
		basic => $colors(fg_default)
		number => $colors(fg_red)
		variable => $colors(fg_cyan)
		keyword => $colors(fg_magenta)
		function => $colors(fg_yellow)
		string => $colors(fg_blue)
		comment => <={%string $colors(fg_white) $attrib(dim)}
		primitive => $colors(fg_green)
	)

	defconf syntax enable false
	set-syntax_conf_enable = @ arg _ {
		if {~ $arg true} {
			$&enablehighlighting
		} {~ $arg false} {
			$&disablehighlighting
		} {~ $arg fast} {
			$&enablefasthighlighting
		} {
			return $syntax_conf_enabled
		}
		result $arg
	}

	defconf syntax funvars-as-functions false
	set-syntax_conf_funvars-as-functions = @ arg _ {
		match $arg (
			(true false) { result $arg }
			* { result $set-syntax_conf_funvars-as-functions }
		)
	}

	defconf syntax accelerate false
	set-syntax_conf_accelerate = @ arg _ {
		match $arg (
			(true false) { result $arg }
			* { result $set-syntax_conf_accelerate }
		)
	}

	_es_syntax_defs = %dict(
		prim => %re('^\$&[a-zA-Z0-9\-_]+$')
		var => %re('^\$+[#\^":]?[a-zA-Z0-9\-_:%]+$')
		basic=> %re('^[a-zA-Z0-9\-_:%]+$')
		number => (%re('^[0-9]+$') %re('^0x[0-9a-fA-F]+$') %re('^0b[01]+$') %re('^0o[0-7]+$'))
		keywords => ('~' '~~' 'local' 'let' 'lets' 'for' 'fn' '%closure' 'match' 'matchall'
			'process' '%dict' '%re' 'onerror')
		string => %re('^''(.|'''')*''?$')
		comment => %re('^#.*$')
		whitespace => %re('^[ \t\r\n]+$')
	)

	fn isatom str {
		if {$syntax_conf_accelerate} {
			return <={$&syn_isatom $str}
		}
		let (
			validatoms = $_es_syntax_defs(prim var basic number keywords)
		){
			if {~ $str $validatoms} {
				return <=true
			}
			result <=false
		}
	}

	fn iscomment str {
		if {$syntax_conf_accelerate} {
			return <={$&syn_iscomment $str}
		}
		if {~ $str $_es_syntax_defs(comment)} {
			return <=true
		}
		result <=false
	}

	fn isstring str peeknexttok {
		if {$syntax_conf_accelerate} {
			return <={$&syn_isstring $str}
		}
		if {~ $str $_es_syntax_defs(string)} {
			return <=true
		}
		result <=false
	} # '

	fn atom_type str lasttok futuretok {
		if {$synax_conf_accelerate} {
			if {~ $#lasttok 0} { lasttok = '' }
			if {~ $#futuretok 0} { futuretok = '' }
			return <={$&syn_atom_type $str $lasttok $futuretok}
		}
		let (
			primregex = $_es_syntax_defs(prim)
			varregex = $_es_syntax_defs(var)
			basicatomregex = $_es_syntax_defs(basic)
			numberregexes = $_es_syntax_defs(number)
			keywords = $_es_syntax_defs(keywords)
			primtmp = ''
		) {
			if {~ $str $numberregexes} {
				return number
			}
			if {~ $str $keywords} {
				return keyword
			}
			if {~ $str $primregex} {
				primtmp = <={~~ $str '$&'^*}
				if {~  $primtmp <=$&primitives} {
					return primitive
				}
			}
			if {~ $str $varregex} {
				return variable
			}
			if {~ $str $basicatomregex} {
				let (fnname = fn-^$str) {
					if {! ~ <={%count $$fnname} 0} {
						return function
					}
				}
				if {~ $lasttok 'fn'} {
					return function
				}
				if {~ $futuretok '=' ':=' '+='} {
					if {~ $str 'fn-'^*} {
						if {$set-syntax_conf_funvars-as-functions} {
							return function
						}
					}
					return variable
				}
				return basic
			}
			return basic
		}
	}

	fn iswhitespace str {
		if {$syntax_conf_accelerate} {
			return <={$&syn_iswhitespace $str}
		}
		if {~ $str $_es_syntax_defs(whitespace)} {
			return <=true
		}
		result <=false
	}

	fn toksiterator toks {
		result (
			@ {
				let (ntok = $toks(1)) {
					toks = $toks(2 ...)
					result $ntok
				}
			}
			@ {
				for (t = $toks) {
					if {! iswhitespace $t} {
						return $t
					}
				}
			}
			@ { result $#toks }
			@ { result $toks(1) }
		)
	}

	fn %syntax_highlight str {
		if {$syntax_conf_accelerate} {
			return <={$&fasthighlighting $^str}
		}
		lets (
			lasttok = ''
			res = ()
			syncol = $syntax_conf_colors
			toks = <={$&basictokenize $str onerror return $str}
			(fn-nexttok fn-futuretok fn-toksleft fn-peektok) = <={toksiterator $toks}
		) {
			while {gt <=toksleft 0} {
				let (tok = <=nexttok) {
					if {isstring $tok <=peektok} {
						if {$syntax_conf_debugging} {
							echo <={%fmt $tok} 'is a string'
						}
						if {dicthaskey $syncol string} {
							res += \ds^$syncol(string)^\de^$tok^\ds^$attrib(reset)^\de
						}
					} {iscomment $tok} {
						if {dicthaskey $syncol comment} {
							res += \ds^$syncol(comment)^\de^$tok^\ds^$attrib(reset)^\de
						}
					} {isatom $tok} {
						if {$syntax_conf_debugging} {
							echo <={%fmt $tok} 'is a' <={atom_type $tok $lasttok}
						}
						if {dicthaskey $syncol <={atom_type $tok $lasttok <=futuretok}} {
							res += \ds^$syncol(<={atom_type $tok $lasttok <=futuretok})^\de^$tok^\ds^$attrib(reset)^\de
						}
					} {
						if {$syntax_conf_debugging} {
							echo <={%fmt $tok} 'is an other'
						}
						res += $tok
					}
					if {! iswhitespace $tok} {
						lasttok = $tok
					}
				}
			}
			result <={%string $res}
		}
	}

	noexport += syntax_conf_debugging syntax_conf_colors syntax_conf_enable
	noexport += set-syntax_conf_enable
	noexport += fn-isatom fn-iscomment fn-isstring fn-atom_type fn-iswhitespace
	noexport += fn-toksiterator fn-%syntax_highlight
}

