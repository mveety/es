library general_completion (init completion)

fn gencomp_split_cmdline cmdline {
	%fsplit ' ' $cmdline |>
		@{ let (r=) { for (i = $*) { if {! ~ $i ''} { r = $r $i } }; result $r} }
}

fn gencomp_filter_list str list {
	let	(res=) {
		for (i = $list) {
			if {~ $i $str^*} {
				res = $res $i
			}
		}
		result $res
	}
}

# both main gencomp functions take multiple independent lists as arguements.
# because of how es maps args to variables they need to be contained in a
# function.
fn gencomp_list_container list {
	result @{ result $list }
}

# usage: main_command completion_function (subcommand subcommand_completion_function ...)
fn gencomp_new_general_completer cmdname compfn subcmdcomps {
	let (default_completer=) {
		if {~ $#subcmdcomps 0} {
			default_completer = $fn-complete_files
		} {
			assert2 $0 {eq <={mod $#subcmdcomps 2} 0}
			local(r=) {
				for ((subcmdname subcmdfn) = $subcmdcomps) {
					if {~ $subcmdname 'default'} {
						default_completer = $subcmdfn
					} {
						r = $subcmdname $subcmdfn
					}
				}
				subcmdcomps = $r
			}
		}
		result @ cmdline partial {
			let (parsed_line = <={gencomp_split_cmdline $cmdline}) {
				if {~ $parsed_line(1) 'doas' || ~ $parsed_line(1) 'sudo'} {
					parsed_line = $parsed_line(2 ...)
				}
				if {~ $#parsed_line 1} {
					result <={$compfn $partial $parsed_line}
				} {
					if {~ $#subcmdcmps 0} { return <={$default_completer $partial} }
					assert2 $cmdname^'_completer' {eq <={mod $#subcmdcomps 2} 0}
					for ((subcmdname subcmdfn) = $subcmdcomps) {
						if {~ $subcmdname <={%last $parsed_line}} {
							return <={$subcmdfn $partial $parsed_line}
						}
					}
					$default_completer $partial
				}
			}
		}
	}
}

# usage: cmdname contained_list_of_completions (subcommand contained_list_of_completions ...)
fn gencomp_new_list_completer cmdname comps subcmdcomps {
	local(compfn=;compfuns=) {
		compfn = @ p l { gencomp_filter_list $p <={$comps} }
		compfuns = default @ p { complete_files $p }
		for ((subcmd subcmdlistcon) = $subcmdcomps) {
			compfuns = $compfuns $subcmd @ p l { gencomp_filter_list $p <={$subcmdlistcon} }
		}
		gencomp_new_general_completer $cmdname $compfn $compfuns
	}
}

