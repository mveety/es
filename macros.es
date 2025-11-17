#!/usr/bin/env es

if {~ $#es_internal_symcount 0} {
	es_internal_symcount = 0
}

if {~ $#es_conf_tempdir 0} {
	local (set-es_conf_tempdir=) {
		es_conf_tempdir = '/tmp'
	}
}

set-es_conf_tempdir = @ arg _ {
	if {~ $arg */} {
		arg = <={~~ $arg */}
	}
	result $arg
}

fn gensym prefix {
	prefix = <={if {~ $#prefix 0} { result '__es_sym_'^$pid } { result $prefix }}
	es_internal_symcount = <={add $es_internal_symcount 1}
	result $prefix^$es_internal_symcount
}

fn tempfile prefix dir _ {
	let (t=){
		if {~ $prefix */*} {
			t = $dir
			dir = $prefix
			prefix = $t
		}
	}
	if {~ $#prefix 0} { prefix = 'tempfile' }
	if {~ <={%count $:prefix} 0} { prefix = 'tempfile' }
	let (
		filename = <={gensym $prefix^'.es'^$pid^'.'}^'.tmp'
		tmpfiledir = <={if {~ $#dir 0} {
				result $es_conf_tempdir
			} {
				result $dir
			}
		}
	) {
		if {! access -rw -d $tmpfiledir} {
			throw error 'tempfile' $tmpfiledir^' is not writable'
		}
		if {~ $tmpfiledir */} { tmpfiledir = <={~~ $tmpfiledir */} }
		result $tmpfiledir/$filename
	}
}

fn nrfn name argsbody {
	let (
		args = ''
		body = ''
	) {
		if {~ $#argsbody 0} {
			throw error noreturn 'missing body'
		} {~ $#argsbody 1} {
			eval 'fn-'^$name^' = $&noreturn @ '^$argsbody
		} {
			args = $argsbody(1 ... <={sub $#argsbody 1})
			body = $argsbody($#argsbody)
			eval 'fn-'^$name^' = $&noreturn @ '^$^args^' '^$body
		}
	}
}

fn macro name argsbody {
	if {~ $#argsbody 0} {
		throw error macro 'missing body'
	}
	let (
		macname = <={gensym '__es_macro_'}
		macfn = ''
		macevalfn = ''
		args = <={ if {~ $#argsbody 1} { result ''} { result $argsbody(1 ... <={sub $#argsbody 1}) } }
		body = <={ if {~ $#argsbody 1} { result $argsbody } { result $argsbody($#argsbody) } }
		macresult = '@ { echo -n $*; echo '';'' }'
		maclet = ''
		argsvar = ''
		macvarname = 'macro-'^$name
	) {
		$macvarname = 'fn-'^$name 'fn-'^$macname $macname
		argsvar = <={ if {~ $args ''} {result '$*'} {result '$'^$args} }
		maclet = 'let(fn-macresult = '$^macresult^' ; args = ('$^args'))'
		macfn = 'fn-'^$macname^' = @ '^$^args^' { '^$maclet^' '^$^body^' }'
		macevalfn = 'fn-'^$name^' = @ '^$^args^' { eval `{ '^$macname^' '^$^argsvar^' } }'
		eval $macfn
		eval $macevalfn
	}
}

macro macroexpand name rest {
	let (
		macvarname = 'macro-'^$name
		macdef = ''
	) {
		macdef = $$macvarname
		macresult $macdef(3)^' '$^rest
	}
}

options = $options macros

