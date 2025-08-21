library completion (init)

if {~ $#es_complete_debug 0} {
	es_complete_debug = false
}

fn escomp_echo {
	if {$es_complete_debug} {
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

fn complete_all_variables prefix partial_name {
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
				res = $res <={~~ $v $prefix^*}
			}
		}
		result $res
	}
}

fn complete_functions partial_name {
	result <={complete_all_variables 'fn-' $partial_name}
}

fn complete_variables prefix partial_name {
	result $prefix^<={complete_all_variables '' <={~~ $partial_name $prefix^*}}
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

fn es_complete_is_sub_command cmdname curline {
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
		if {~ $cll(1) 'doas' || ~ $cll(1) 'sudo'} {
			result $cll(2)
		} {
			result $cll(1)
		}
	}
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
			if {es_complete_is_sub_command 'sudo' $curline ||
				es_complete_is_sub_command 'doas' $curline} {
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
		(err function) = <={try dictget $es_complete_command_hooks $pcmd}
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
	es_complete_command_hooks = <={dictremove $es_complete_command_hooks $cmdname}
	true
}

fn %complete_cmd_hook cmdname completefn {
	es_complete_command_hooks = <={dictput $es_complete_command_hooks $cmdname $completefn}
}

fn __es_complete_initialize {
	es_complete_command_hooks = <=dictnew
}

if {~ $#es-complete_conf_complete_order 0} {
	es-complete_conf_complete_order = executables functions variables
}

fn complete_base_complete partial {
	process $es-complete_conf_complete_order (
		executables { result <={complete_executables $partial} }
		functions { result <={complete_functions $partial} }
		variables { result <={complete_variables '' $partial} }
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
		if {~ <={es_complete_get_last_command $curline} <={dictnames $es_complete_command_hooks}} {
			result <={es_complete_run_command_hook $curline $partial}
		} {
			result <={complete_files $partial}
		}
	}
}

# this is the intermediate state for core_completer to improve performance
# perf tanks if there's a lot of completions
es_complete_current_curline = ''
es_complete_current_partial = ''
es_complete_current_start = ''
es_complete_current_end = ''
es_complete_current_completion = ()

fn es_complete_cc_checkstate linebuf text start end {
	if {~ $linebuf $es_complete_current_curline &&
		~ $text $es_complete_current_partial &&
		~ $start $es_complete_current_start &&
		~ $end $es_complete_current_end} {
		result <=true
	} {
		result <=false
	}
}

fn es_complete_format_list list {
	local(res=){
		for (i = $list) {
			res = $res ''''^$i^''''
		}
		result '('^$^res^')'
	}
}

fn es_complete_dump_state {
	echo 'es_complete_current_curline = '''^$es_complete_current_curline^''''
	echo 'es_complete_current_partial = '''^$es_complete_current_partial^''''
	echo 'es_complete_current_start = '^$es_complete_current_start
	echo 'es_complete_current_end = '^$es_complete_current_end
	echo 'es_complete_current_completion = '^<={es_complete_format_list $es_complete_current_completion}
}

# %core_completer is the hook es uses to call into the completion machinery.
# core_completer produces a string which is passed to rl_completion_matches in
# readline. rl_completion_matches makes successive calls to this function
# (incrementing state) in order to get the list of possible completions for text.
# the list ends with the sentinel value '%%end_complete'.
# this rendition of %core_completer will call %complete in initial calls to get
# the list of possible completions and iterates over it in sucessive calls. if it
# detects some change in state it will make a call to %complete to update it's list

fn %core_completer linebuf text start end state {
	if {~ $state 0 || ! es_complete_cc_checkstate $linebuf $text $start $end} {
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
			es_complete_current_completion = <={%complete $slb $text $nstart $nend}
			es_complete_current_curline = $linebuf
			es_complete_current_partial = $text
			es_complete_current_start = $start
			es_complete_current_end = $end
		}
	}
	local(cclen=$#es_complete_current_completion){
		if {~ $cclen 0 || gt <={add $state 1} $cclen} {
			escomp_echo '''%end_complete'''
			result '%%end_complete'
		} {
			escomp_echo ''''^$es_complete_current_completion(<={add $state 1})^''''
			result $es_complete_current_completion(<={add $state 1})
		}
	}
}

