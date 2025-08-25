library atuin_history (init history)

if {~ $#atuin_history_conf_update-history-file 0} {
	atuin_history_conf_update-history-file = true
}

if {~ $#atuin_history_conf_debugging 0} {
	atuin_history_conf_debugging = false
}

if {~ $#__atuin_started 0 || ~ $__atuin_start false} {
	old_history = $fn-history
	__atuin_old_history = ''
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
	conf -p history -s use-hook false

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
		fn history {
			if {~ $#__atuin_old_history 0 || ~ $__atuin_old_history ''} {
				return <=true
			}
			if {! $atuin_history_conf_update-history-file} {
				return <=true
			}
			local (set-history=) {
				local (history = $__atuin_old_history) {
					$old_history $*
				}
			}
		}

		fn %history cmdline {
			local (id=; ctime = $unixtime) {
				__atuin_echo 'id = `{atuin history start ''--'' '^$cmdline
				id = `{atuin history start '--' $cmdline}
				%add-history $cmdline
				ditto = $cmdline
				ATUIN_HISTORY_ID = $id
				cmdstarttime = $ctime
				if {$atuin_history_conf_update-history-file} {
					if {~ $__atuin_old_history '' || ~ $#__atuin_old_history 0} {
						return <=false
					}
					{
						echo '#+'^$ctime
						echo $cmdline
					} >> $__atuin_old_history
				}
				return <=false
			}
		}

		fn %postexec res {
			if {~ $ATUIN_HISTORY_ID ''} {
				return <=true
			}
			local (duration = <={sub $unixtime $cmdstarttime}) {
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
				__atuin_echo atuin history end '--exit' $res '--duration' $duration '--' $ATUIN_HISTORY_ID
				{ let (ATUIN_LOG = error) {
					atuin history end '--exit' $res '--duration' $duration '--' $ATUIN_HISTORY_ID >/dev/null >[2=1] }} &
			}
			cmdstarttime = 0
		}
	}
}

fn __atuin_disable {
	fn %history
	fn %postexec
	fn-history = $old_history
}

fn __atuin_reload-history nelem {
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

if {~ $#old_set-history_use-hook 0} {
	old_set-history_use-hook = $set-history_conf_use-hook
}

set-history_conf_use-hook = @ arg {
	if {~ $history atuin} {
		result atuin
	} {
		$old_set-history_use-hook $arg
	}
}

if {~ $#old_set-history 0} {
	old_set-history = $set-history
}

set-history = @ file {
	if {~ $file atuin} {
		local (set-history=; set-history_conf_file=; set-history_conf_use-hook=) {
			if {! ~ $history atuin} {
				__atuin_old_history = $history
				history = $file
				history_conf_file = $file
				history_conf_use-hook = atuin
				__atuin_enable
				__atuin_reload-history
				result $file
			}
		}
	} {
		if {~ $history atuin} {
			local (set-history_conf_use-hook=) {
				__atuin_disable
				history_conf_use-hook = false
			}
		}
		$old_set-history $file
	}
}

