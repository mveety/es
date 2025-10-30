library template (init libraries string)

# template 'hello ${world} this is ${a} ${test}' %dict(world=>'people';a=>'a really cool';test=>'test')

fn parse_template_string string {
	local (
		(fn-getchar fn-ungetchar fn-curchar fn-length) = <={string_iterator $string}
		state = instring
		curstring=
		workstring=
		results=()
		c=
	) {
		while {gt <=length 0} {
			match $state (
				* { throw error $0 'invalid state' }
				instring {
					c = <=getchar
					if {~ $c '$'} {
						if {~ <=getchar '$'} {
							workstring += '$'
						} {
							ungetchar
							results += %dict(type=>string;value=>$"workstring)
							workstring = ()
							state = indollar
						}
					} {
						workstring += $c
					}
				}
				indollar {
					c = <=getchar
					if {! ~ $c '{'} {
						throw error $0 'missing { after $'
					}
					state = invar
				}
				invar {
					c = <=getchar
					while {! ~ $c '}'} {
						if {eq $#c 0} { throw error $0 'unclosed var' }
						if {~ $c ' ' \t \r \n} { throw error $0 'whitespace in var' }
						workstring += $c
						c = <=getchar
					}
					results += %dict(type=>var;name=>$"workstring)
					state = instring
					workstring = ()
				}
			)		
		} { throw error $0 'zero length string'}
		if {gt $#workstring 0} {
			results += %dict(type=>string; value=>$"workstring)
		}
		result $results
	}
}

fn process_parsed_string vardict parsed_string {
	local (res=()) {
		for (strelem = $parsed_string) {
			match $strelem(type) (
				* { unreachable }
				string { res += $strelem }
				var { res += %dict(type=>string;value=><={%string $vardict($strelem(name))}) }
			)
		}
		result $res
	}
}

fn unparse_template_string parsed_string {
	local(res=''){
		for (strelem = $parsed_string) {
			match $strelem(type) (
				* { unreachable }
				string { res = $res^$strelem(value) }
				var { res = $res^'${'^$strelem(name)^'}' }
			)
		}
		return $res
	}
}

fn template_replace_dict vardict string {
	parse_template_string $string |>
		process_parsed_string $vardict |>
		unparse_template_string
}

fn template_vars string {
	local (res=()) {
		for (strelem = <={parse_template_string $string}) {
			if {~ $strelem(type) var} {
				res += $strelem(name)
			}
		}
		result $res
	}
}

fn-template_dict_from_env = $&withbindings @ string {
	local(vars = <={template_vars $string};resdict = %dict()) {
		for	(v = $vars) {
			if {lt <={%count $$v} 1} {
				throw error template_dict_from_env 'var $'^$v^' is not defined'
			}
			resdict := $v => <={%flatten ' ' $$v}
		}
		result $resdict
	}
}

fn-template_replace_env = $&withbindings @ string {
	template_replace_dict <={template_dict_from_env $string} $string
}

fn-template = $&withbindings @ maybedict_and_strings {
	if {~ <={$&termtypeof $maybedict_and_strings($#maybedict_and_strings)} dict} {
		if {lt $#maybedict_and_strings 2} {
			throw error template 'missing string'
		}
		local(
			rdict = $maybedict_and_strings($#maybedict_and_strings)
			strings = $maybedict_and_strings(1 ... <={sub $#maybedict_and_strings 1})) {
			template_replace_dict $rdict $^strings |> return
		}
	} {
		template_replace_env $^maybedict_and_strings |> return
	}
}

