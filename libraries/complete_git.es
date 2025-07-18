library complete_git (init completion)

#if {~ $#complete_git_use_list 0} {
#	if {~ <={access -r -1 -d $libraries/complete_git |> %count} 0} {
#		echo 'warning: list completer is non-functional. run complete_git_setup for'
#		echo '         a supported configuration.'
#		complete_git_use_list = true
#	} {
#		complete_git_use_list = false
#	}
#}

if {~ $#complete_git_use_list 0} {
	complete_git_use_list = true
} {
	complete_git_use_list = false
}

complete_git_commands = (
	'init' 'clone' 'add' 'status' 'diff' 'commit'
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
		return $res
	}
}

fn complete_git_filter_dir str {
	es_complete_run_glob '$libraries/complete_git/'^$str^'*' |>
		es_complete_remove_empty_results |>
		es_complete_only_names |> result
}	

fn complete_git_hook curline partial {
	let (prevtok = <={
		es_complete_get_last_cmdline $curline |> es_complete_trim |>
			%fsplit ' ' |> %last |> es_complete_trim
	}) {
		if {~ $prevtok 'git'} {
			if {$complete_git_use_list} {
				complete_git_filter_list $partial
			} {
				complete_git_filter_dir $partial
			}
		} {
			complete_files $partial
		}
	}
}

%complete_cmd_hook git @ curline partial {
	# echo 'complete_git: curline = '^<={format $curline}^', partial = '^<={format $partial}
	complete_git_hook $curline $partial
}

# this is a total hack to get around the problems with readline and the parser
fn complete_git_setup {
	if {~ <={access -r -1 -d $libraries/complete_git |> %count} 0} {
		for (comppath = $libraries) {
			if {access -w -d $comppath} {
				echo 'making completer directory at '^$comppath^'/complete_git'
				mkdir $comppath/complete_git
				for (i = $complete_git_commands) {
					# echo 'making ' $comppath/complete_git/$i
					touch $comppath/complete_git/$i
				}
				complete_git_use_list = false
				return <=true
			}
		}
	} {
		echo 'git completion already set up'
		return <=false
	}
}

