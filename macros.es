#!/usr/bin/env es

if {~ $#__es_symcount 0} {
	__es_symcount=1
}

fn gensym prefix {
	prefix = <={if {~ $#prefix 0} { result '__es_sym_' } { result $prefix }}
	let(symn = $__es_symcount) {
		__es_symcount = <={add $__es_symcount 1}
		result $prefix^$symn
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

