library complete_git (init completion)

complete_git_commands = init clone add status diff commit notes restore reset rm mv
complete_git_commands = $complete_git_commands branch checkout switch merge mergetool
complete_git_commands = $complete_git_commands log stash tag worktree
complete_git_commands = $complete_git_commands fetch pull push remove submodule
complete_git_commands = $complete_git_commands apply cherry-pick diff rebase revert
complete_git_commands = $complete_git_commands bisect blame grep am apply format-patch
complete_git_commands = $complete_git_commands send-email request-pull svn fast-import
complete_git_commands = $complete_git_commands clean gc fsck reflog filter-branch instaweb
complete_git_commands = $complete_git_commands archive bundle daemon update-server-info
complete_git_commands = $complete_git_commands cat-file check-ignore checkout-index
complete_git_commands = $complete_git_commands commit-tree diff-index for-each-ref
complete_git_commands = $complete_git_commands hash-object ls-files ls-free merge-base
complete_git_commands = $complete_git_commands read-tree rev-list rev-parse show-ref
complete_git_commands = $complete_git_commands symbolic-ref update-index update-ref
complete_git_commands = $complete_git_commands verify-pack write-tree

fn complete_git_filter_list str {
	process $complete_git_commands (
		($str^*) { result $matchexpr }
		* { result }
	)
}

fn complete_git_hook curline partial {
	let (prevtok = <={
		es_complete_get_last_cmdline $curline |> es_complete_trim |>
			%fsplit ' ' |> %last |> es_complete_trim
	}) {
		if {~ $prevtok 'git'} {
			result <={complete_git_filter_list $partial}
		} {
			result <={complete_files $partial}
		}
	}
}

%complete_cmd_hook git @ curline partial {
	# echo 'complete_git: curline = '^<={format $curline}^', partial = '^<={format $partial}
	result <={complete_git_hook $curline $partial}
}

