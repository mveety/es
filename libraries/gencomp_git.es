# this is a quick example of using gencomp with a list of possible commands

library gencomp_git (init completion general_completion complete_git)

fn-gencomp_git_hook = <={gencomp_new_list_completer git <={box $_complete_git_commands}}

%complete_cmd_hook git @ curline partial {
	gencomp_git_hook $curline $partial
}

