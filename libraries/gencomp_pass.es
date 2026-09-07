library gencomp_pass (init completion general_completion complete_git)

if {~ $#PASSWORD_STORE_DIR 0} {
	_passdir = $home/.password-store
} {
	_passdir = $PASSWORD_STORE_DIR
}

set-PASSWORD_STORE_DIR = @ dir _ { _passdir = $dir }

_gencomp_pass_cmds = (
	'ls' 'find' 'show' 'grep' 'insert' 'edit' 'generate' 'rm' 'mv' 'cp'
	'git' 'help' 'version'
)

fn gencomp_pass_format_name list {
	let ( res = () ) {
		process $list (
			(*.gpg) { res += <={~~ $matchexpr *.gpg } }
			(*) { res += $matchexpr }
		)
		result $res
	}
}

fn gencomp_pass_complete_files partial {
	es_complete_run_glob $_passdir/^$partial |>
		es_complete_remove_empty_results |>
		iterator |>
		do @ path { result <={~~ $path $_passdir/*}} |>
		gencomp_pass_format_name
}

fn gencomp_pass_complete_partial partial {
	let (
		files = <={gencomp_pass_complete_files $partial}
		cmds = <={
			if {! ~ $partial *^'/'^*} {
				glob $partial^'*' $_gencomp_pass_cmds |> result
			} {
				result ()
			}
		}
	) {
		sortlist $files $cmds |> result
	}
}

fn gencomp_pass_hook curline partial {
	let (cmdline = <={gencomp_split_cmdline $curline}) {
		if {~ <={%last $cmdline} 'pass'} {
			gencomp_pass_complete_partial $partial
		} {
			match $cmdline(2) (
				('show' 'insert' 'edit' 'generate' 'rm' 'mv' 'cp') {
					gencomp_pass_complete_files $partial
				}
				('git') {
					complete_git_filter_list $partial
				}
				* { result () }
			)
		}
	}
}

%complete_cmd_hook pass @ curline partial {
	gencomp_pass_hook $curline $partial
}

