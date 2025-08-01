#!/usr/bin/env es

library autoinit (init libraries)

if {~ $#libraries 0 } {
	throw error autoinit '$libraries must be set appropriately'
}

if {~ $#autoinit_echo_load 0} {
	autoinit_echo_load = false
}

if {~ $#esrcd_debugging 0} {
	esrcd_debugging = false
}

if {~ $#autoloaded 0} {
	noexport = $noexport autoloaded
	autoloaded = (autoinit)
}

if {~ $#autoinit_start_automatically 0} {
	autoinit_start_automatically = false
}

fn esrcd_getall {
	result $libraries/esrc.d/*.es
}

fn esrcd_get_dir {
	for (i = $libraries/esrc.d) {
		if {access -w -d $i } { return $i }
	}
	throw error $0 'no writable esrc.d'
}

fn esrcd_executable_scripts {
	process <=esrcd_getall (
		* {
			if {access -xf $matchexpr} {
				result $matchexpr
			} {
				result
			}
		}
	)
}

fn esrcd_find_by_name name {
	process <=esrcd_getall (
		(*/$name.es) { return $matchexpr }
	)
	throw error $0 $name^' not found'
}

fn esrcd_delete be_verbose file {
	if {! access -f $file } { throw error $0 $file^' not found' }
	if {! access -wf $file } { throw error $0 $file^' is read-only' }
	if {$be_verbose || $esrcd_debugging } { echo 'rm '^$file }
	if {! $esrcd_debugging } { rm $file }
}

fn esrcd_last list {
	result $list($#list)
}

fn esrcd_extract_name name {
	result <={~~ <={%split '/' $name |> esrcd_last} *.es}
}

fn esrcd_all_scripts {
	process <=esrcd_getall (
		*^'*'^* { result }
		* { esrcd_extract_name $matchexpr |> result }
	)
}

fn esrcd_active_scripts {
	process <=esrcd_executable_scripts (
		* { esrcd_extract_name $matchexpr |> result }
	)
}

fn esrcd_enable_script script {
	esrcd_find_by_name $script |> chmod +x
}

fn esrcd_disable_script script {
	esrcd_find_by_name $script |> chmod -x
}

fn esrcd_load_script script disabled_error verbose force {
	local(name=<={esrcd_extract_name $script}){
		if {~ $autoloaded $name && ! $force} {
			throw error $0 $name^' is already loaded'
		}
		if {! access -xf $script && ! $force} {
			if {$disabled_error} {
				throw error $0 $name^' is disabled'
			}
		}
		. $script
		if {! ~ $autoloaded $name} {
			autoloaded = $autoloaded $name
		}
		if {$verbose} {
			echo 'loaded '^$script
		}
	}
}

fn esrcd_load_all_scripts verbose {
	for(s = <=esrcd_executable_scripts) {
		catch @ e type msg {
			if {~ $e error} {
				match $type (
					(esrcd_load_script) { result }
					* { throw $e $type $msg }
				)
			} {
				throw $e $type $msg
			}
		} {
			esrcd_load_script $s false $verbose false
		}
	}
}

fn esrcd_in_git_repo dir {
	result <={git -C $dir rev-parse --is-inside-work-tree >/dev/null >[2=1]}
}

fn esrcd_usage {
	throw usage autoinit 'usage: autoinit [-v|-h] command [script]'
}

fn esrcd_help {
	cat << '%%end'
usage: autoinit [-v|-h] command [script]
    flags:
        -v -- be more verbose
        -h -- print this message
    commands:
        load-all             -- load all enabled scripts
        load [-f] [script]   -- load any script (-f to force)
        enable [script]      -- enable a script
        disable [script]     -- disable a script
        list-all             -- list all available scripts
        list-enabled         -- list all enabled scripts
        list-loaded          -- list all loaded scripts
        file [script]        -- print the path to a script
        new [script]         -- print the path to a new script
        delete [-y] [script] -- delete a script
        dir                  -- list esrc.d directories
        git [git args]       -- run git in esrc.d
        help                 -- print this message
%%end
}

fn esrcd_print args {
	if {~ $#args 0 } { return <=true }
	echo $args
}

fn autoinit command arg {
	match $command (
		(-h) {
			esrcd_help
			return <=true
		}
		([+-]d) {
			match $matchexpr (
				(-*) { esrcd_debugging = false }
				(+*) { esrcd_debugging = true }
			)
			command = $arg(1)
			arg = $arg(2 ...)
		}

	)

	if {$esrcd_debugging} { echo 'autoinit: debugging enabled' >[1=2] }

	local (
			be_verbose = <={
				if {~ $command -v} {
					command = $arg(1)
					arg = $arg(2 ...)
					result true
				} {
					result $autoinit_echo_load
				}
			}
			force_load = false
			yes = false
	) {
		if {~ $#command 0} { esrcd_usage }
		match $command (
			(load-all) {
				esrcd_load_all_scripts $be_verbose
			}
			(load) {
				if {~ $arg(1) -f} {
					force_load = true
					arg = $arg(2 ...)
				}
				if {! ~ $#arg 1} { esrcd_usage }
				esrcd_load_script <={esrcd_find_by_name $arg} true $be_verbose $force_load
			}
			(enable) {
				if {! ~ $#arg 1} { esrcd_usage }
				esrcd_enable_script $arg
			}
			(disable) {
				if {! ~ $#arg 1} { esrcd_usage }
				esrcd_disable_script $arg
			}
			(list-all) {
				esrcd_all_scripts |> esrcd_print
			}
			(list-enabled) {
				esrcd_active_scripts |> esrcd_print
			}
			(list-loaded) {
				echo $autoloaded
			}
			(file) {
				if {! ~ $#arg 1} { esrcd_usage }
				esrcd_find_by_name $arg |> esrcd_print
			}
			(new) {
				if {! ~ $#arg 1} { esrcd_usage }
				catch @ e type msg {
					if {! ~ $e error} { throw $e $type $msg }
					if {! ~ $type esrcd_find_by_name} { throw $e $type $msg }
					if {! ~ $msg $arg^' not found'} { throw $e $type $msg }
					catch @ e type msg {
						if {~ $e error && ~ $type esrcd_get_dir} {
							throw error $0 $msg
						}
						throw $e $type $msg
					} {
						esrcd_print <={esrcd_get_dir}^'/'^$arg^'.es'
					}
				} {
					esrcd_find_by_name $arg
					throw error $0 $arg^' already exists'
				}
			}
			(delete) {
				if {! gte $#arg 1} { esrcd_usage }
				if {~ $arg(1) -y} {
					yes = true
					arg = $arg(2 ...)
				}
				if {! ~ $#arg 1} { esrcd_usage }
				catch @ e type msg {
					if {! ~ $e error} { throw $e $type $msg }
					match $type (
						(esrcd_find_by_name) { throw $e $0 $msg }
						(esrcd_delete) { throw $e $0 'unable to delete '^$arg }
						* { throw $e $type $msg }
					)
				} {
					local (file = <={esrcd_find_by_name $arg}; r = 'n') {
						if {! $yes} {
							echo -n 'delete '^$arg^'?[no] '
							r = <=%read
							if {~ $r [yY]*} { yes = true }
						}
						if {$yes} { esrcd_delete $be_verbose $file }
					}
				}
			}
			(dir*) {
				for (i = $libraries/esrc.d) {
					if {access -r -d $i} { esrcd_print $i }
				}
			}
			(git) {
				let (
					cwd = `{pwd}
					esrcd_dir = ()
				) {
					for (i = $libraries/esrc.d) {
						if {access -rw -d $i && esrcd_in_git_repo $i} {
							esrcd_dir = $i
							break
						}
					}
					if {~ $#esrcd_dir 0} {
						echo 'error: no esrc.d in git repo found!' >[1=2]
						return <=false
					}
					if {~ $#arg 0} {
						return <=true
					}
					cd $esrcd_dir
					git $arg
					cd $cwd
					return <=true
				}
			}
			(help) {
				esrcd_help
			}
			* {
				esrcd_usage
				return <=false
			}
		)
	}
	return <=true
}

