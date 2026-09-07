library direnv (init libraries esmle cwd)

defconf direnv enable false
set-direnv_conf_enable = @ arg _ {
	if {! ~ $arg true false} {
		return $direnv_conf_enable
	}
	match $arg (
		true { fn-%cdhook = @{ direnv_cdhook $* } }
		false { fn-%cdhook= }
	)
	result $arg
}

defconf direnv print-on-unload true
defconftype direnv print-on-unload true false

let (
	last-envdict = %dict()
	direnv-loaded = false
	varbackup = %dict()
) {
	fn direnv_export {
		result ``(\n){ direnv export systemd }
	}

	fn direnv_get_envdict {
		let (
			lines = ``(\n){direnv export systemd}
			envdict = %dict()
		) {
			for (l = $lines) let ((varname value) = <={~~ $l *^'='^*}) {
				envdict := $varname => $value
			}
			result $envdict
		}
	}

	fn direnv_load_envdict dict _ {
		let (newvarbackup = %dict()) {
			dictforall $dict @ varname value {
				if {! ~ <={%count $$varname} 0} {
					newvarbackup := $varname => $$varname
				}
				$varname = $value
			}
			varbackup = $newvarbackup $varbackup
		}
	}

	fn direnv_pop_dirbackup {
		let(old=;) {
			if {gt <={$&listcount $varbackup} 1} {
				old = $varbackup(1)
				varbackup = $varbackup(2 ...)
			} {
				old = $varbackup
			}
			result $old
		}
	}

	fn direnv_var_stack {
		result $varbackup
	}

	fn direnv_unload_envdict dict _ {
		let (curvarbackup = <=direnv_pop_dirbackup) {
			dictforall $dict @ varname _ {
				if {dicthaskey $curvarbackup $varname} {
					$varname = $curvarbackup($varname)
				} {
					$varname=
				}
			}
		}
	}

	fn direnv_in_loaded_dir {
		if {! $direnv-loaded } {
			return <=false
		}
		if {~ $#DIRENV_DIR 0} {
			return <=false
		}
		let (cleandir = <={~~ $DIRENV_DIR '-'^*}) {
			if {~ $cwd $cleandir^*} {
				return <=true
			}
		}
		return <=false
	}

	fn direnv_pathfmt path {
		let(homes=(/usr/home /home /export/home)) {
			if {~ $username 'root' && ~ $path /root /root/*} {
				return '~'^<={~~ $path /root*}
			}
			match $path (
				($homes/$username) {
					return '~'
				}
				($homes/$username/*) {
					return '~/'^<={~~ $path $homes/$username/*}
				}
				($homes/*) {
					return '~'^<={~~ $path $homes/*}
				}
				(/root) {
					return '~root'
				}
				(/root/*) {
					return '~root/'^<={~~ $path /root/*}
				}
				* {
					return $path
				}
			)
		}
	}

	fn direnv_cdhook _ {
		if { $direnv-loaded } {
			if {direnv_in_loaded_dir} {
				return <=true
			}
			if {$direnv_conf_print-on-unload} {
				echo >[2=1] 'direnv: unloading '^<={direnv_pathfmt $DIRENV_FILE}
			}
			direnv_unload_envdict $last-envdict
			last-envdict = %dict()
			direnv-loaded = false
		}
		let (edict = <=direnv_get_envdict) {
			if {lte $#edict 4} {
				return <=true
			}
			direnv_load_envdict $edict
			last-envdict = $edict
			direnv-loaded = true
			return <=true
		}
	}
}

