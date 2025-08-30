#!/usr/bin/env es

library types (init macros libraries)

fn primordial { result <=true }
fn composite { result <=false }

if {~ $#__es_type_tests 0 || ! ~ <={$&termtypeof $__es_type_tests} dict} {
	__es_type_tests = <=dictnew
}

fn is_type_installed t {
	let (res = false) {
		dictforall $__es_type_tests @ n _ {
			if {~ $n $t} { return <=true}
		}
		result <=false
	}
}

fn install_any_type primordial name test {
	local(typetest=){
		typetest = @ v { if {$test $v} { result $primordial $name } { result false '__es_not_type' }}
		__es_type_tests = <={dictput $__es_type_tests $name $typetest}
		return <=true
	}
}

fn install_type name test {
	install_any_type composite $name $test
}

fn uninstall_type name {
	__es_type_tests = <={dictremove $__es_type_tests $name}
}

fn typeof v {
	if {~ $#v 0 } {
		return 'nil'
	}
	local (p=;r=;primtype=;primfound=false;type=;found=false) {
		dictforall $__es_type_tests @ name testfn {
			(p r) = <={$testfn $v}
			if {! ~ $r '__es_not_type'} {
				if {$p} {
					primfound = true
					primtype = $r
				} {
					return $r
				}
			}
		}
		if {$primfound} {
			return $primtype
		} {
			result 'unknown'
		}
	}
}

# this is for testing inside of complex type checks. you shouldn't make
# calls to typeof inside of typeof. prim_typeof gives basic functionality
# while still being safe
fn prim_typeof v {
	if {~ $#v 0} {
		return 'nil'
	}
	local (rval = 'unknown';res=;prim=) {
		dictforall $__es_type_tests @ name testfn {
			(prim res) = <={$testfn $v}
			if {$prim} { rval = $res }
		}
		result $rval
	}
}

fn istype t v {
	if {~ $t <={typeof $v}} {
		return <=true
	}
	return <=false
}

fn is_prim_type t v {
	if {~ $t <={prim_typeof $v}} {
		return <=true
	}
	return <=false
}

fn types {
	dictnames $__es_type_tests
}

## below are the test functions for the built-in "types"

fn _is_a_number v {
	if {! ~ $#v 1} {
		return <=false
	}
	for(i = $:v){
		if {! ~ $i [1234567890]} {
			return <=false
		}
	}
	return <=true
}

fn _is_a_hex_number v {
	if {! ~ $#v 1} {
		return <=false
	}
	for (i = $:v){
		if {! ~ $i [1234567890abcdef]} {
			return <=false
		}
	}
	return <=true
}

fn _is_a_dict v {
	if {! ~ $#v 1} {
		return <=false
	}
	if {! ~ $v '%dict('^*^')'} {
		return <=false
	}
	result <=true
}

fn _is_a_function v {
	if {! ~ $#v 1} {
		return <=false
	}
	local (
		vlist = $:v
		headcheck=
	) {
		headcheck = $vlist(1 ... 8)
		headcheck = $"headcheck
		if {~ $vlist(1) '@' && ~ $vlist(2) ' ' } {
			return <=true
		} { ~ $headcheck '%closure' } {
			return <=true
		}
		return <=false
	}
}	

fn _is_a_primordial v {
	if {! ~ $#v 1} {
		return <=false
	}
	local (
		vlist = $:v
	) {
		if {~ $vlist(1) '$' && ~ $vlist(2) '&'} {
			return <=true
		}
	}
	return <=false
}

fn prim_type_test_number v {
	if {_is_a_number $v} {
		result <=true
	} {
		result <=false
	}
}

fn prim_type_test_string v {
	if {~ $#v 1 && ! _is_a_number $v && ! _is_a_function $v && ! _is_a_primordial $v} {
		result <=true
	} {
		result <=false
	}
}

fn prim_type_test_function v {
	if {~ $#v 1 && _is_a_function $v } {
		result <=true
	} {
		result <=false
	}
}

fn prim_type_test_primordial v {
	if {~ $#v 1 && _is_a_primordial $v} {
		result <=true
	} {
		result <=false
	}
}

fn prim_type_test_list v {
	if {gt $#v 1} {
		result <=true
	} {
		result <=false
	}
}

fn prim_type_test_dict v {
	_is_a_dict $v
}

fn prim_type_test_error v {
	if {~ <={$&gettermtag $v} error && _is_a_function $v} {
		result <=true
	} {
		result <=false
	}
}

fn prim_type_test_box v {
	if {~ <={$&gettermtag $v} box && _is_a_function $v} {
		result <=true
	} {
		result <=false
	}
}

install_any_type primordial 'number' @ v { result <={prim_type_test_number $v} }
install_any_type primordial 'string' @ v { result <={prim_type_test_string $v} }
install_any_type primordial 'function' @ v { result <={prim_type_test_function $v} }
install_any_type primordial 'primordial' @ v { result <={prim_type_test_primordial $v} }
install_any_type primordial 'list' @ v { result <={prim_type_test_list $v} }
install_any_type primordial 'dict' @ v { result <={prim_type_test_dict $v} }
install_any_type composite 'error' @ v { result <={prim_type_test_error $v} }
install_any_type composite 'box' @ v { result <={prim_type_test_box $v} }

