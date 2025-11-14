library atuin_history (init esmle history)

defconf atuin_history update-history-file true
defconf atuin_history debugging false
defconf atuin_history load-on-change true
defconf atuin_history enable-keymap-hook true
defconf atuin_history load-duplicate-entries true

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
	ATUIN_STTY = `{stty -g >[2] /dev/null}
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
			if {! gt <={%count $:cmdline} 0} {
				return <=false
			}
			local (id=; ctime = $unixtime_ns) {
				__atuin_echo 'id = `{atuin history start ''--'' '^$cmdline
				id = `{atuin history start '--' $cmdline}
				ATUIN_HISTORY_ID = $id
				cmdstarttime = $ctime
				if {$atuin_history_conf_update-history-file} {
					if {~ $history '' || ~ $#history 0} {
						return <=false
					}
					{
						echo '#+'^$unixtime
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
			ATUIN_HISTORY_ID = ''
		}
	}

	fn reload-history nelem {
		if {~ $#nelem 0} {
			nelem = $history_conf_reload
		}
		%clear-history
		# %add-history ``(\n){atuin history list --cmd-only | tail -n $nelem}
		if {$atuin_history_conf_load-duplicate-entries} {
			%add-history ``(\n){atuin search --include-duplicates --cmd-only --limit $nelem}
		} {
			%add-history ``(\n){atuin search --cmd-only --limit $nelem}
		}

	}

	fn atuin-ctrlr-hook {
		let (
			atuin_tmpfile = /tmp/es_atuin.$pid
			atuin_data =
		) {
			atuin search -i >[2] $atuin_tmpfile
			atuin_data = ``(\n){cat $atuin_tmpfile}
			rm $atuin_tmpfile
			if {~ $#atuin_data 0} {
				result <=false
			} {
				result <=true $atuin_data
			}
		}
	}

	fn atuin-ctrlup-hook cmdline {
		let (
			atuin_tmpfile = /tmp/es_atuin.$pid
			atuin_data =
		) {
			atuin search -i $cmdline >[2] $atuin_tmpfile
			atuin_data = ``(\n){cat $atuin_tmpfile}
			rm $atuin_tmpfile
			if {~ $#atuin_data 0} {
				result <=false
			} {
				result <=true $atuin_data
			}
		}
	}

	if {$atuin_history_conf_enable-keymap-hook} {
		%mapkey 'CtrlR' atuin-ctrlr-hook
		%mapkey 'CtrlUp' atuin-ctrlup-hook
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

