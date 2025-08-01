library gencomp_autoinit (init completion autoinit general_completion complete_git)

gencomp_autoinit_cmds = (
	'load' 'load-all' 'enable' 'disable' 'list-all' 'list-enabled'
	'list-loaded' 'file' 'new' 'delete' 'dir' 'help' 'git'
)

fn gencomp_autoinit_inactive_scripts {
	let (r=; active = <=esrcd_active_scripts) {
		for (i = <=esrcd_all_scripts) {
			if {! ~ $i $active} {
				r = $r $i
			}
		}
		result $r
	}
}

fn gencomp_autoinit_hook curline partial {
	let (force = false ; cmdline = <={gencomp_split_cmdline $curline}; t=) {
		for (i = $cmdline) {
			if {~ $i -f} {force = true}
			if {! ~ $i -[vhfy]} { t = $t $i }
		}
		cmdline = $t

		if {~ <={%last $cmdline} 'autoinit'} {
			gencomp_filter_list $partial $gencomp_autoinit_cmds
		} {
			match $cmdline(2) (
				('load') {
					if {$force} {
						gencomp_filter_list $partial <=esrcd_all_scripts
					} {
						gencomp_filter_list $partial <=esrcd_active_scripts
					}
				}
				('file' 'delete') {
					gencomp_filter_list $partial <=esrcd_all_scripts
				}
				('disable') {
					gencomp_filter_list $partial <=esrcd_active_scripts
				}
				('enable') {
					gencomp_filter_list $partial <=gencomp_autoinit_inactive_scripts
				}
				('git') {
					complete_git_filter_list $partial
				}
				* { return () }
			)
		}
	}
}

%complete_cmd_hook autoinit @ curline partial {
	gencomp_autoinit_hook $curline $partial
}

