library completion (init)

if {~ $#_es_complete_debug 0} {
	_es_complete_debug = false
}

if {~ $#es_conf_completion-prefix-commands 0} {
	es_conf_completion-prefix-commands = (doas sudo)
}

fn escomp_echo {
	if {$_es_complete_debug} {
		echo $*
	}
}

fn es_complete_remove_empty_results files {
	local(res=){
		for(i = $files) {
			if {! ~ $i *^'*'} {
				res = $res $i
			}
		}
		result $res
	}
}

fn es_complete_only_executable files {
	local(res=){
		for(i = $files) {
			if {access -xf $i} {
				res = $res $i
			}
		}
		result $res
	}
}

fn es_complete_executable_filter files {
	local(res=){
		for(i = $files) {
			if {! ~ $i *^'*'} {
				if {access -xf $i} {
					res = $res $i
				}
			}
		}
		result $res
	}
}

fn es_complete_access perms files {
	local(res=){
		for(i = $files) {
			if {access -$perms $i} {
				res = $res $i
			}
		}
		result $res
	}
}

fn es_complete_only_names files {
	local(res=){
		for(i = $files) {
			res = $res <={%split '/' $i |> %last}
		}
		result $res
	}
}

fn es_complete_run_glob pattern {
	if {~ $pattern *^'*'^*} {
		result <={glob $pattern}
	} {
		result <={glob $pattern^'*'}
	}
}

fn complete_executables partial_name {
	local(files=;only_name=true){
		files = <={if {~ $partial_name */*} {
						only_name = false
						result <={es_complete_run_glob $partial_name}
					} {
						result <={es_complete_run_glob '$path/'^$partial_name}
					}
				}
		files = <={es_complete_executable_filter $files}
		if {$only_name} {
			es_complete_only_names $files |> result
		} {
			result $files
		}
	}
}

fn complete_files partial_name {
	es_complete_run_glob $partial_name |>
		es_complete_remove_empty_results |>
		result
}

fn complete_all_variables type prefix partial_name {
	if {~ $partial_name */*} {
		return ()
	}
	local(
		allvars= <=$&vars <=$&internals
		res=
	) {
		partial_name = <={if{~ $#partial_name 0 || ~ $partial_name ''}{
							result '*'
						}{
							if {~ $partial_name *^'*'^*} {
								result $partial_name
							} {
								result $partial_name^'*'
							}
						}}
		for(v = $allvars) {
			if {eval '~ '^$v^' '^$prefix^$partial_name} {
				match $type (
					all { res = $res <={~~ $v $prefix^*} }
					visible {
						if {! %isvarhidden $v} {
							res = $res <={~~ $v $prefix^*}
						}
					}
					hidden {
						if {%isvarhidden $v} {
							res = $res <={~~ $v $prefix^*}
						}
					}
					* { unreachable }
				)
			}
		}
		result $res
	}
}

fn complete_functions partial_name {
	result <={complete_all_variables all 'fn-' $partial_name}
}

fn complete_visible_functions partial {
	result <={complete_all_variables visible 'fn-' $partial}
}

fn complete_hidden_functions partial {
	result <={complete_all_variables hidden 'fn-' $partial}
}

fn complete_variables prefix partial_name {
	result $prefix^<={complete_all_variables all '' <={~~ $partial_name $prefix^*}}
}

fn complete_visible_variables prefix partial {
	result $prefix^<={complete_all_variables visible '' <={~~ $partial_name $prefix^*}}
}

fn complete_hidden_variables prefix partial {
	result $prefix^<={complete_all_variables hidden '' <={~~ $partial_name $prefix^*}}
}

fn complete_primitives prefix partial {
	local (
		allprims = <={if {$prefix} { result '$&'^<=$&primitives } { $&primitives }}
		res=
	) {
		for (i = $allprims) {
			if {~ $i $partial^*} {
				res = $res $i
			}
		}
		result $res
	}
}

fn es_complete_strip_leading_whitespace str {
	local(strl=$:str;i=1){
		for(c = $strl){
			if {! ~ $c (' ' \t \n \r)} {
				return $"strl($i ...)
			}
			i = <={add $i 1}
		}
		return ''
	}
}

fn es_complete_is_sub_command1 cmdname curline {
	local (
		curlastcmd = <={
			%fsplit ' ' $curline |>
				@{process ($*) (
					'' { result }
					* { result $matchexpr }
				)} |> %last
		}
	) {
		if {~ $curlastcmd $cmdname} {
			result <=true
		} {
			result <=false
		}
	}
}

fn es_complete_left_trim string {
	let(do_trim = true) {
		{process $:string (
			(' ' \t \n \r) {
				if {$do_trim} { result } { result $matchexpr }
			}
			* {
				if {$do_trim} { do_trim = false }
				result $matchexpr
			}
		)} |> %string |> result
	}
}

fn es_complete_right_trim string {
	reverse $:string |> %string |> es_complete_left_trim |>
	%strlist |> reverse |> %string |> result
}

fn es_complete_trim string {
	es_complete_left_trim $string |> es_complete_right_trim |> result
}

fn es_complete_get_last_cmdline cmdline {
	lets (
		cmdlinel = $:cmdline
		i = $#cmdlinel
	) {
		while {gt $i 0} {
			match $cmdlinel($i) (
				(';' '{' '!' '&' '|') {
					if {~ $cmdlinel(<={add $i 1}) '>'} {
						i = <={add $i 1}
					}
					result $cmdlinel(<={add $i 1} ... $#cmdlinel) |>
						%string |> es_complete_left_trim |> return
				}
			)
			i = <={sub $i 1}
		}
		result $cmdline
	}
}

fn es_complete_get_last_raw_command cmdline {
	es_complete_get_last_cmdline $cmdline |> %fsplit ' ' |>
		%elem 1 |> result
}

fn es_complete_get_last_command cmdline {
	let (cll = <={es_complete_get_last_cmdline $cmdline |> %fsplit ' '}){
		if {~ $cll(1) $es_conf_completion-prefix-commands} {
			result $cll(2)
		} {
			result $cll(1)
		}
	}
}

fn es_complete_is_sub_command curline {
	for (cmd = $es_conf_completion-prefix-commands) {
		if {es_complete_is_sub_command1 $cmd $curline} {
			return <=true
		}
	}
	result <=false
}

fn es_complete_is_command curline {
	local(curlinel=$:curline;i=){
		i = $#curlinel
		while {gt $i 0} {

			if {~ $curlinel($i) ';' '{' '!'} {
				return <=true
			}
			if {~ $curlinel($i) '&' '|'} {
				if {~ $curlinel(<={sub $i 1}) '&' '|'} {
					return <=true
				}
			}
			if {~ $curlinel($i) '='} {
				if {gt $i 1} {
					if {~ $curlinel(<={sub $i 1}) '<'} {
						return <=true
					}
				}
			}
			if {~ $curlinel($i) '>'} {
				if {gt $i 1} {
					if{~ $curlinel(<={sub $i 1}) '|'} {
						return <=true
					}
				}
			}
			if {es_complete_is_sub_command $curline} {
				return <=true
			}
			if {! ~ $curlinel($i) (' ' \t \n \r)} {
				return <=false
			}
			i = <={sub $i 1}
		}
		return <=false
	}
}

fn es_complete_run_command_hook curline partial {
	let (pcmd = <={es_complete_get_last_command $curline}; err=; function=) {
		(err function) = <={try dictget $_es_complete_command_hooks $pcmd}
		if {$err} {
			if {$es_complete_default_to_files} {
				return <={complete_files $partial}
			}
		}
		return <={$function <={es_complete_get_last_cmdline $curline} $partial}
	}
}

fn es_complete_remove elem list {
	let(res=) {
		for (i = $list) {
			if {! ~ $i $elem} {
				res = $res $i
			}
		}
		result $res
	}
}

fn es_complete_remove2 elem list {
	let(res=){
		for ((cmd fn) = $list) {
			if {! ~ $cmd $elem} {
				res = $res $cmd $fn
			}
		}
		result $res
	}
}

fn %complete_cmd_unhook cmdname {
	_es_complete_command_hooks = <={dictremove $_es_complete_command_hooks $cmdname}
	true
}

fn %complete_cmd_hook cmdname completefn {
	_es_complete_command_hooks = <={dictput $_es_complete_command_hooks $cmdname $completefn}
}

fn __es_complete_initialize {
	_es_complete_command_hooks = <=dictnew
}

if {~ $#es_conf_complete_order 0} {
	es_conf_complete_order = executables functions variables
}

if {~ $#es_conf_sort-completions 0} {
	es_conf_sort-completions = true
} {
	if {! ~ $es_conf_sort-completions true false} {
		es_conf_sort-completions = true
	}
}

# try to protect this mess
set-es_conf_sort-completions = @{
	if{~ $1 true || ~ $1 false} {
		result $1
	} {
		result $es_conf_sort-completions
	}
}

fn complete_base_complete partial {
	process $es_conf_complete_order (
		executables { result <={complete_executables $partial} }
		functions { result <={complete_functions $partial} }
		visible_functions { result <={complete_visible_functions $partial} }
		hidden_functions { result <={complete_hidden_functions $partial} }
		variables { result <={complete_variables '' $partial} }
		visible_variables { result <={complete_visible_variables '' $partial} }
		hidden_variables { result <={complete_hidden_variables '' $partial} }
		* { unreachable }
	)
}

# %complete returns a list of possible completions based on the context
# at this time it's pretty simple, completing executables if start is 1
# and files otherwise.
# curline is the current input buffer up to the partial with leading
# whitespace stripped. partial is the entity being completed. start and end
# are the start and end arguments given to es_complete_hook by readline but
# indexed starting at 1 for cultural compatibility with es.
fn %complete curline partial start end {
	if {~ $partial '$'^*} {
		match <={%elem 2 $:partial} (
			('#' '^' '"' ':') {
				result <={complete_variables '$'^<={%elem 2 $:partial} $partial}
			}
			('&') {
				result <={complete_primitives true $partial}
			}
			* {
				result <={complete_variables '$' $partial}
			}
		)
	} {~ $curline *^'$&'} {
		result <={complete_primitives false $partial}
	} {~ $start 1 || es_complete_is_command $curline} {
		result <={complete_base_complete $partial}
	} {
		if {~ <={es_complete_get_last_command $curline} <={dictnames $_es_complete_command_hooks}} {
			result <={es_complete_run_command_hook $curline $partial}
		} {
			result <={complete_files $partial}
		}
	}
}

# %core_completer is the hook es uses to call into the completion machinery.
# core_completer produces a string which is passed to rl_completion_matches in
# readline. rl_completion_matches makes successive calls to this function
# (incrementing state) in order to get the list of possible completions for text.
# the list ends with the sentinel value '%%end_complete'.
# this rendition of %core_completer will call %complete in initial calls to get
# the list of possible completions and iterates over it in sucessive calls. if it
# detects some change in state it will make a call to %complete to update it's list

### formerly occupied by %core_completer. the above comment still more or less
### applies to %new_completer

# %new_completer is the hook that the new completion machinery hooks into. basically
# this is a thin layer between %complete and readline that strips whitespace and
# translates start and end to index by one, then just returns the list producted by
# %complete.

fn %new_completer linebuf text start end {
	if {~ $#fn-%complete 0} {
		return ()
	}
	local(lbl=$:linebuf;lbllen=;tmp=;slb=;nstart=;nend=){
		slb = <={if {~ $linebuf '' || ~ $start 0} {
					nstart = <={add $start 1}
					nend = <={add $end 1}
					result ''
				} {
					tmp = <={es_complete_strip_leading_whitespace $"lbl(1 ... $start)}
					lbllen = <={%count $lbl(1 ... $start)}
					nstart = <={%count $:tmp |> sub $lbllen |> sub $start |> add 1 }
					nend = <={%count $:tmp |> sub $lbllen |> sub $end |> add 1}
					assert2 $0 {gt $nstart 0 && gt $nend 0}
					result $tmp
				}
			}
		%complete $slb $text $nstart $nend
	}
}

