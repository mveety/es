library complete_git (init completion)

complete_git_commands = (
	'init' 'clone' 'add' 'status ' 'diff' 'commit'
	'notes' 'restore' 'reset' 'rm' 'mv'
	'branch' 'checkout' 'switch' 'merge' 'mergetool'
	'log' 'stash' 'tag' 'worktree'
	'fetch' 'pull' 'push' 'remove' 'submodule'
	'apply' 'cherry-pick' 'diff' 'rebase' 'revert'
	'bisect' 'blame' 'grep' 'am' 'apply' 'format-patch'
	'send-email' 'request-pull' 'svn' 'fast-import'
	'clean' 'gc' 'fsck' 'reflog' 'filter-branch' 'instaweb'
	'archive' 'bundle' 'daemon' 'update-server-info'
	'cat-file' 'check-ignore' 'checkout-index'
	'commit-tree' 'diff-index' 'for-each-ref'
	'hash-object' 'ls-files' 'ls-free' 'merge-base'
	'read-tree' 'rev-list' 'rev-parse' 'show-ref'
	'symbolic-ref' 'update-index' 'update-ref'
	'verify-pack' 'write-tree'
)

fn complete_git_filter_list str {
	let(res=) {
		for(i = $complete_git_commands) {
			if {~ $i $str^*} {
				res = $res <={es_complete_trim $i}
			}
		}
		result $res
	}
}

fn complete_git_hook curline partial {
	let (prevtok = <={
		es_complete_get_last_cmdline $curline |> es_complete_trim |>
			%fsplit ' ' |> %last |> es_complete_trim
	}) {
		if {~ $prevtok 'git'} {
			complete_git_filter_list $partial
		} {
			complete_files $partial
		}
	}
}

%complete_cmd_hook git @ curline partial {
	# echo 'complete_git: curline = '^<={format $curline}^', partial = '^<={format $partial}
	result <={complete_git_hook $curline $partial}
}

