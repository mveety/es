#!/usr/bin/env es

library types (init macros libraries)

fn primordial { result <=true }

fn is_type_installed t {
	for(fn = $__es_type_tests) {
		if {$fn 'search' $t} {
			return <=true
		}
	}
	return <=false
}

fn install_any_type primordial name test {
	local(typetest=){
		typetest = @ op v {
						if { ~ $op 'test'} {
							if { $test $v } {
								result $primordial $name
							} {
								result false '__es_not_type'
							}
						} { ~ $op 'search' && ~ $v $name} {
							result <=true
						} { ~ $op 'name'} {
							result $name
						} {
							result <=false
						}
					}
		if {! is_type_installed $name} {
			__es_type_tests = $__es_type_tests $typetest
			result <=true
		} {
			result <=false
		}
	}
}

fn install_type name test {
	install_any_type false $name $test
}

fn uninstall_type name {
	local(typefuns=) {
		for(fn = $__es_type_tests) {
			if {! $fn 'search' $name} {
				typefuns = $typefuns $fn
			}
		}
		__es_type_tests = $typefuns
	}
}

fn typeof v {
	if {~ $#v 0 } {
		return 'nil'
	} {
		local(res=;prim=;primtype=) {
			for(testfn = $__es_type_tests) {
				(prim res) = <={$testfn 'test' $v}
				if {! ~ $res '__es_not_type'} {
					if {$prim} {
						primtype = $res
					} {
						return $res
					}
				}
			}
			if {! ~ $#primtype 0} {
				return $primtype
			} {
				return 'unknown'
			}
		}
	}
}

# this is for testing inside of complex type checks. you shouldn't make
# calls to typeof inside of typeof. prim_typeof gives basic functionality
# while still being safe
fn prim_typeof v {
	if {~ $#v 0} {
		return 'nil'
	} {
		local(res=;prim=) {
			for(testfn = $__es_type_tests) {
				(prim res) = <={$testfn 'test' $v}
				if {$prim} {
					return $res
				}
			}
			return 'unknown'
		}
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
	local(res=){
		for(fn = $__es_type_tests) {
			res = $res <={$fn 'name'}
		}
		result $res
	}
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

install_any_type primordial 'number' @ v { result <={prim_type_test_number $v} }
install_any_type primordial 'string' @ v { result <={prim_type_test_string $v} }
install_any_type primordial 'function' @ v { result <={prim_type_test_function $v} }
install_any_type primordial 'primordial' @ v { result <={prim_type_test_primordial $v} }
install_any_type primordial 'list' @ v { result <={prim_type_test_list $v} }

