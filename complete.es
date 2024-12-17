
fn glob str {
	result <={eval 'result '^$^str }
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
	local(res=;plist=){
		for(i = $files) {
			plist = <={%split '/' $i}
			res = $res $plist($#plist)
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
		# files = <={es_complete_remove_empty_results $files}
		# files = <={es_complete_only_executable $files}
		files = <={es_complete_executable_filter $files}
		if {$only_name} {
			result <={es_complete_only_names $files}
		} {
			result $files
		}
	}
}

fn complete_files partial_name {
	local(files=){
		files = <={es_complete_run_glob $partial_name}
		files = <={es_complete_remove_empty_results $files}
		result $files
	}
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

fn es_complete_is_command curline {
	local(curlinel=$:curline;i=){
		i = $#curlinel
		while {gt $i 0} {
			if {~ $curlinel($i) ';' '{'} {
				return <=true
			}
			if {~ $curlinel($i) '='} {
				if {gt $i 1} {
					if {~ $curlinel(<={sub $i 1}) '<'} {
						return <=true
					}
				}
			}
			if {! ~ $curlinel($i) (' ' \t \n \r)} {
				return <=false
			}
			i = <={sub $i 1}
		}
		return <=false
	}
}

# set this to true if you want to list executables before functions
es_completion_executables_first = true

# %complete returns a list of possible completions based on the context
# at this time it's pretty simple, completing executables if start is 1
# and files otherwise.
# curline is the current input buffer up to the partial with leading
# whitespace stripped. partial is the entity being completed. start and end
# are the start and end arguments given to es_complete_hook by readline but
# indexed starting at 1 for cultural compatibility with es.
fn %complete curline partial start end {
	if {~ $partial '$'^*} {
		result <={complete_variables '$' $partial}
	} {~ $start 1 || es_complete_is_command $curline} {
		if {$es_completion_executables_first} {
			result <={complete_executables $partial} <={complete_functions $partial}
		} {
			result <={complete_functions $partial} <={complete_executables $partial}
		}
	} {
		result <={complete_files $partial}
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
						nstart = <={add <={sub $start <={sub $lbllen <={%count $:tmp}}} 1}
						nend = <={add <={sub $end <={sub $lbllen <={%count $:tmp}}} 1}
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
			result '%%end_complete'
		} {
			result $es_complete_current_completion(<={add $state 1})
		}
	}
}

