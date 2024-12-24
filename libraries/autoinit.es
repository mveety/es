#!/usr/bin/env es

library autoinit (init libraries)

assert2 autoinit {gt $#libraries 0}

if {~ $#autoinit_echo_load 0} {
	autoinit_echo_load = false
}

if {~ $#autoloaded 0} {
	autoloaded = (autoinit)
}

if {~ $#autoinit_start_automatically 0} {
	autoinit_start_automatically = false
}

fn esrcd_getall {
	result $libraries/esrc.d/*.es
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

fn esrcd_last list {
	result $list($#list)
}

fn esrcd_extract_name name {
	result <={~~ <={%split '/' $name |> esrcd_last} *.es}
}

fn esrcd_all_scripts {
	process <=esrcd_getall (
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

fn esrcd_load_script script disabled_error verbose {
	local(name=<={esrcd_extract_name $script}){
		if {~ $autoloaded $name} {
			throw error $0 $name^' is already loaded'
		}
		if {! access -xf $script} {
			if {$disabled_error} {
				throw error $0 $name^' is disabled'
			}
		}
		. $script
		autoloaded = $autoloaded $name
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
			esrcd_load_script $s false $verbose
		}
	}
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
        load-all         -- load all enabled scripts
        load [script]    -- load any script
        enable [script]  -- enable a script
        disable [script] -- disable a script
        list-all         -- list all available scripts
        list-enabled     -- list all enabled scripts
        file [script]    -- print the path to a script
        help             -- print this message
%%end
}

fn esrcd_print args {
	if {~ $#args 0 } { return <=true }
	echo $args
}

fn autoinit command arg {
	if {~ $command -h} {
		esrcd_help
		return <=true
	}
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
	) {
		if {~ $#command 0} { esrcd_usage }
		match $command (
			(load-all) {
				esrcd_load_all_scripts $be_verbose
			}
			(load) {
				if {! ~ $#arg 1} { esrcd_usage }
				esrcd_load_script <={esrcd_find_by_name $arg} true $be_verbose
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
			(file) {
				if {! ~ $#arg 1} { esrcd_usage }
				esrcd_find_by_name $arg |> esrcd_print
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

