#!/usr/bin/env es

library type_dict (types array dict)

fn type_dict_test v {
	if {~ $v(1) __es_dict_*_keys && ~ $v(2) __es_dict_*_data} {
		result <=true
	} {
		result <=false
	}
}

install_type 'dict' @ v { result <={type_dict_test $v} }

