library gencomp_pkg (init completion general_completion)

gencomp_pkg_cmds = (
	'add' 'alias' 'annotate' 'audit' 'autoremove'
	'check' 'clean' 'config' 'create' 'delete'
	'fetch' 'help' 'info' 'install' 'key' 'lock'
	'plugins' 'query' 'register' 'remove' 'repo'
	'repositories' 'rquery' 'search' 'set' 'ssh'
	'shell' 'shlib' 'stats' 'triggers' 'unlock'
	'update' 'updating' 'upgrade' 'version' 'which'
)

fn gencomp_pkg_search partial {
	if {lt <={%count $:partial} 2} { return () }
	result ``(\n) {pkg search -Q name -g $partial^'*' | grep Name | grep -v Comment | awk '{ print $3 }'}
}

fn gencomp_pkg_installed_search partial {
	result ``(\n) {pkg query -g '%n' $partial^'*'}
}

fn gencomp_pkg_hook curline partial {
	let (cmdline = <={gencomp_split_cmdline $curline}) {
		if {~ $cmdline(1) 'doas' || ~ $cmdline(1) 'sudo'} {
			cmdline = $cmdline(2 ...)
		}
		if {~ <={%last $cmdline} 'pkg'} {
			gencomp_filter_list $partial $gencomp_pkg_cmds
		} {
			cmdline = $cmdline(2 ...)
			while {~ $cmdline(1) -*} {
				cmdline = $cmdline(2 ...)
			}
			match $cmdline(1) (
				('install' 'updating' 'add' 'fetch') {
					gencomp_pkg_search $partial
				}
				('remove' 'delete' 'info' 'remove' 'upgrade' 'lock') {
					gencomp_pkg_installed_search $partial
				}
			)
		}
	}
}

%complete_cmd_hook pkg @ curline partial {
	gencomp_pkg_hook $curline $partial
}

