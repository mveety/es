library gencomp_ssh (init completion general_completion)

if {~ $#ssh_known_hosts 0} {
	ssh_known_hosts = $home/.ssh/known_hosts
}

fn gencomp_ssh_hosts {
	if {! access -r $ssh_known_hosts} {
		return ()
	}
	result <={process ``(\n){cat ~/.ssh/known_hosts | awk '{print $1}' | uniq} (
		'['^*^']:'^* { %first <={~~ $matchexpr '['^*^']:'^*} |> result }
		* { result $matchexpr }
	)}
}

fn-gencomp_ssh_hook = <={gencomp_new_list_completer ssh $fn-gencomp_ssh_hosts}

%complete_cmd_hook ssh @ curline partial {
	gencomp_ssh_hook $curline $partial
}

