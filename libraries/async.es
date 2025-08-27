#!/usr/bin/env es

library async (init libraries)

if {~ $#async_tempdir 0} {
	async_tempdir = /tmp
}

if {~ $#async_keep_files 0} {
	async_keep_files = false
}

if {~ $#async_subprocs 0} {
	async_subprocs = ()
}

if {~ $#async_track_procs 0} {
	async_track_procs = true
}

fn async_tmpfile_name subpid stream {
	let (
		sym = <={gensym es_async_temp_file_}
	) {
		result $async_tempdir^'/'^$sym^'.'^$subpid^'.'^$stream^'.bin'
	}
}

fn async_format_error e t m {
	result @{ result <={makeerror $e $t $m}}
}

fn async1 fun args {
	let (
		stdout = <={async_tmpfile_name $pid stdout}
		stderr = <={async_tmpfile_name $pid stderr}
		resfile = <={async_tmpfile_name $pid result}
		errfile = <={async_tmpfile_name $pid error}
		subpid = 0
		subproc_object =
	) {
		subpid = <={%background {
			let (rval = ){
				catch @ e {
					echo <={async_format_error $e} > $errfile
				} {
					rval = <={$fun $args >[2] $stderr > $stdout}
					echo $rval > $resfile
				}
			}
		}}
		result @ d {
			match $d (
				stdout { result $stdout }
				stderr { result $stderr }
				result { result $resfile }
				error { result $errfile }
				pid { result $subpid }
				files { result $stdout $stderr $resfile $errfile}
				alive { ~ $subpid <={%apids -a} }
				all { result $subpid $stdout $stderr $resfile $errfile}
			)
		}
	}
}

fn async args {
	let (obj = <={async1 $args}) {
		if {$async_track_procs} {
			async_subprocs = $obj $async_subprocs
		}
		result $obj
	}
}

fn async_remove_pid subpid {
	let (new_subprocs = ) {
		for (i = $async_subprocs) {
			if {! ~ $subpid <={$i pid}} {
				new_subprocs = $i $new_subprocs
			}
		}
		async_subprocs = $new_subprocs
	}
}

fn async_rm files {
	for (i = $files) {
		if {access -w -f $i} {
			rm $i
		}
	}
}

fn await1 backquote quiet asyncobj {
	let (
		stdout = <={$asyncobj stdout}
		stderr = <={$asyncobj stderr}
		resfile = <={$asyncobj result}
		errfile = <={$asyncobj error}
		subpid = <={$asyncobj pid}
		resval =
		stdout_data =
		errfn =
		error = false
	) {
		waitfor $subpid
		if {access -r -f $errfile} {
			errfn = <={%parsestring ``(\n){cat $errfile}}
			error = <=$errfn
		}
		if {$backquote} {
			stdout_data = `{cat $stdout}
		} {
			if {! $quiet} {
				cat $stdout
			}
		}
		if {! $quiet} {
			cat $stderr >[1=2]
		}
		if {access -r -f $resfile} {
			resval = `{cat $resfile}
		}
		if {! $async_keep_files} {
			async_rm $stdout $stderr $resfile $errfile
		}
		if {$async_track_procs} {
			async_remove_pid $subpid
		}
		if {$error} {
			throw $error
		}
		if {$backquote} {
			bqstatus = $resval
			result $stdout_data
		} {
			result $resval
		}
	}
}

fn await args {
	lets (
		quiet = <={
			if {~ $args(1) -q} {
				args = $args(2 ...)
				result true
			} {
				result false
			}
		}
		asyncobj = $args(1)
	) {
		await1 false $quiet $asyncobj
	}
}

fn alive asyncobj {
	$asyncobj alive
}

fn bqawait args {
	lets (
		quiet = <={
			if {~ $args(1) -q} {
				args = $args(2 ...)
				result true
			} {
				result false
			}
		}
		asyncobj = $args(1)
	) {
		await1 true $quiet $asyncobj
	}
}

fn akill obj {
	let (
		the_living = <={%apids -a}
		stdout = <={$obj stdout}
		stderr = <={$obj stderr}
		resfile = <={$obj result}
		errfile = <={$obj error}
		subpid = <={$obj pid}
	) {
		if {~ $subpid $the_living} {
			kill -9 $subpid
		}
		waitfor $subpid
		if {! $async_keep_files} {
			async_rm $stdout $stderr $resfile $errfile
		}
		if {$async_track_procs} {
			async_remove_pid $subpid
		}
		result <=true
	}
}

fn async_clean_glob glob {
	let (res = <={glob $glob}) {
		if {! ~ $glob $res} {
			result $res
		} {
			result ()
		}
	}
}

fn aclean {
	if {~ $#async_subprocs 0} {
		rm -fv $async_tempdir/es_async_temp_file_*.$pid.*.bin
		return <=true
	}
	let (
		the_living = <={%apids -a}
		new_subprocs =
		all_files = <={async_clean_glob $async_tempdir^'/es_async_temp_file_*.'^$pid^'.*.bin'}
		good_files =
		verbose = <={ if {~ $1 -v} { result true } { result false } }
	) {
		for (i = $async_subprocs) {
			if {~ <={$i pid} $the_living} {
				new_subprocs = $i $new_subprocs
				good_files = <={$i files} $good_files
			} {
				waitfor <={$i pid}
			}
		}
		for (f = $all_files) {
			if {! ~ $f $good_files} {
				if {$verbose} { rm -vf $f } { rm $f }
			}
		}
		async_subprocs = $new_subprocs
	}
	return <=true
}

fn run_in_subproc fun args {
	async $fun $args |> await
}

