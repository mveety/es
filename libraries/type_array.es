#!/usr/bin/env es

library type_array (types)

fn type_array_test v {
	if {~ $v(1) __es_array_*} {
		result <=true
	} {
		result <=false
	}
}

install_type 'array' @ v { result <={type_array_test $v} }

