library atuin_history (init history)


defconf atuin_history update-history-file true
defconf atuin_history debugging false
defconf atuin_history load-on-change true

if {~ $#__atuin_started 0 || ~ $__atuin_start false} {
	old_reload-history = $fn-reload-history
	old_set-history_use-hook = $set-history_conf_use-hook
	__atuin_started = true
}

fn __atuin_echo args {
	if {$atuin_history_conf_debugging} {
		echo $args
	}
}

fn __atuin_enable {
	ATUIN_SESSION = `{atuin uuid}
	ATUIN_STTY = `{stty -g}
	ATUIN_HISTORY_ID = ''

	let (
		cmdstarttime = 0
		fn is_a_number v {
			if {! ~ $#v 1} {
				return <=false
			}
			for (i = $:v) {
				if {! ~ $i [1234567890]} {
					return <=false
				}
			}
			return <=true
		}
	) {
		fn %history cmdline {
			local (id=; ctime = $unixtime_ns) {
				__atuin_echo 'id = `{atuin history start ''--'' '^$cmdline
				id = `{atuin history start '--' $cmdline}
				%add-history $cmdline
				ditto = $cmdline
				ATUIN_HISTORY_ID = $id
				cmdstarttime = $ctime
				if {$atuin_history_conf_update-history-file} {
					if {~ $history '' || ~ $#history 0} {
						return <=false
					}
					{
						echo '#+'^$ctime
						echo $cmdline
					} >> $history
				}
				return <=false
			}
		}

		fn %postexec res {
			if {~ $ATUIN_HISTORY_ID ''} {
				return <=true
			}
			local (duration = <={sub $unixtime_ns $cmdstarttime}) {
				if {~ $#res 0} { res = 1 }
				res = <={%last res}
				match $res (
					true { res = 0 }
					false { res = 1 }
					'' { res = 127 }
					* {
						if {! is_a_number $res} {
							res = 0
						}
					}
				)
				__atuin_echo atuin history end -e $res -d $duration '--' $ATUIN_HISTORY_ID
				{ local (ATUIN_LOG = error) {
					atuin history end -e $res -d $duration '--' $ATUIN_HISTORY_ID >/dev/null >[2=1] }} &
			}
			cmdstarttime = 0
		}
	}

	fn reload-history nelem {
		if {~ $#nelem 0} {
			nelem = $history_conf_reload
		}
		%clear-history
		local (hist = ``(\n){atuin history list --cmd-only | tail -n $nelem}) {
			for (h = $hist) {
				%add-history $h
			}
		}
	}
}

fn __atuin_disable {
	fn %history
	fn %postexec
	fn-reload-history = $old_reload-history
}

set-history_conf_use-hook = @ arg {
	if {~ $arg atuin} {
		__atuin_enable
		if {$atuin_history_conf_load-on-change} {
			reload-history
		}
		result atuin
	} {
		__atuin_disable
		$old_set-history_use-hook $arg
	}
}

