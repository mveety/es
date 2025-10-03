

library named_pipes (init libraries)

defconf named_pipes tempdir /tmp
defconf named_pipes keep-files false
defconf named_pipes allow-unnamed-pipes false

if {~ $#_es_named_pipes 0} {
	_es_named_pipes = %dict()
}

fn es_np_add_named_pipe name file {
	if {~ $name unnamed} {
		if {dicthaskey $_es_named_pipes unnamed} {
			let (pipes = $_es_named_pipes(unnamed) ){
				pipes = $pipes $file
				_es_named_pipes := unnamed => $pipes
			}
		} {
			_es_named_pipes := unnamed => $file
		}
	} {
		if {dicthaskey $_es_named_pipes $name} {
			let (file = $_es_named_pipes($name)) {
				rm $file
			}
		}
		_es_named_pipes := $name => $file
	}
}

fn make-named-pipe label {
	lets (
		name = <={if {~ $#label 0} { result unnamed } { result $label }}
		sym = <={gensym es_named_pipe_}
		tmpdir = <={conf -X named_pipes:tempdir}
		fifo = $tmpdir^'/'^$sym^'.'^$pid^'.'^$name^'.fifo'
	) {
		if {~ $name unnamed && ! conf -X named_pipes:allow-unnamed-pipes} {
			throw error make-named-pipe 'unnamed pipes are forbidden'
		}
		if {! access -w -d $tmpdir} {
			throw error make-named-pipe $tmpdir^' is not writable'
		}
		if {dicthaskey $_es_named_pipes $name} {
			throw error make-named-pipe 'pipe '^$name^' already exists'
		}
		if {! mkfifo >/dev/null >[2=1] $fifo} {
			throw error make-named-pipe 'unable to create fifo '^$fifo
		}
		es_np_add_named_pipe $name $fifo
		result $fifo
	}
}

fn all-named-pipes {
	let (pipes = ()) {
		for (n = <={dictnames $_es_named_pipes}) {
			pipes = $pipes $_es_named_pipes($n)
		}
		result $pipes
	}
}

fn named-pipe name {
	if {~ $name unnamed} { throw error named-pipe 'unable to return an unnamed pipe' }
	result $_es_named_pipes($name) onerror {
		throw error named-pipe 'pipe '^$name^' does not exist'
	}
}

fn _es_np_rm file {
	if {! access -rp $file} {
		throw error _es_np_rm 'does not exist'
	}
	if {! conf -X named_pipes:keep-files && ! rm >/dev/null >[2=1] $file} {
		throw error _es_np_rm 'unable to delete file'
	}
	result 0
}

fn delete-named-pipe name {
	if {~ $name unnamed} { throw error named-pipe 'unable to return an unnamed pipe' }
	_es_np_rm $_es_named_pipes($name) onerror {
		if {error error es:glom} {
			throw error delete-named-pipe 'pipe '^$name^' does not exist'
		}
		if {error error _es_np_rm 'does not exist'} {
			throw error delete-named-pipe $name^' pipe file is missing'
		}
		if {error error _es_np_rm 'unable to delete file'} {
			throw error delete-named-pipe 'cannot delete '^$_es_named_pipes($name)
		}
	}
	_es_named_pipes := $name =>
	result <=true
}
		
fn delete-unnamed-pipes {
	if {! dicthaskey $_es_named_pipes unnamed} {
		return <=true
	}
	for (pipe = $_es_named_pipes(unnamed)) {
		_es_np_rm $pipe
	}
}

fn pipe-echo name rest {
	if {! dicthaskey $_es_named_pipes $name} {
		throw error pipe-echo 'pipe '^$name^' does not exist'
	}
	echo $rest > <={named-pipe $name}
}

fn pipe-read name {
	if {! dicthaskey $_es_named_pipes $name} {
		throw error pipe-read 'pipe '^$name^' does not exist'
	}
	result <={%read <={named-pipe $name}}
}

