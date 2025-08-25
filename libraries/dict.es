#!/usr/bin/env es

# this is an extension on arrays to create associative arrays


if {~ $#__es_dict_assert 0} {
	__es_dict_assert = false
}

defconf dict useprimitive true

if {$dict_conf_useprimitive} {
	library dict (init macros libraries)

	fn dict:assert loc dict {
		result <=true
	}

	fn dict:new size {
		dictnew
	}

	fn dict:add dict key value {
		dictput $dict $key $value
	}

	fn dict:set1 dict key value {
		dictput $dict $key $value
	}

	fn dict:set dict key value {
		dictput $dict $key $value
	}

	fn dict:get dict key {
		(err dict) = <={try dictget $dict $key}
		if {$err} {
			errmatch $err (
				error '$&dictget' {
					throw error dict 'key not found'
				}
				{ throw $e }
			)
		}
		result $dict
	}

	fn dict:allkeys dict {
		dictnames $dict
	}

	fn dict:allvalues dict {
		dictvalues $dict
	}

	fn dict:delete odict key {
		dictremove $odict $key
	}

	fn dict:free {
		result <=true
	}

	fn dict:size dict {
		dictsize $dict
	}

	fn dict:stats dict {
		let (sz = <={dictsize $dict}) {
			result $sz $sz $sz $sz
		}
	}
} {
	library dict (init macros libraries array)

	fn dict:assert loc keysarray dataarray {
		if {$__es_dict_assert} {
			let (
				allkeys = <={array:getall $keysarray}
				alldata = <={array:getall $dataarray}
				keysz = <={array:size $keysarray}
				datasz = <={array:size $dataarray}
			) {
				assert2 $loc {~ $#allkeys $#alldata}
				assert2 $loc {~ $keysz $datasz}
				result 0
			}
		}
	}

	fn dict:new size {
		lets (
			sym = <={gensym '__es_dict_'}
			keysarray = $sym^'_keys'
			dataarray = $sym^'_data'
		) {
			size = <={if {~ $#size 0} { result 1 } { result $size }}
			$keysarray = <={array:new $size}
			$dataarray = <={array:new $size}
			dict:assert $0 $keysarray $dataarray
			result ($keysarray $dataarray)
		}
	}

	fn dict:add keysarray dataarray key value {
		let (
			nextfree = ''
		) {
			catch @ e type msg {
				if {~ $e error && ~ $type array && ~ $msg 'out of range'} {
					array:prepend $keysarray $key
					array:prepend $dataarray $value
					dict:assert $0 $keysarray $dataarray
					result ($keysarray $dataarray)
				} {
					throw $e $type $msg
				}
			} {
				nextfree = <={array:index $keysarray}
				array:set $keysarray $nextfree $key
				array:set $dataarray $nextfree $value
				dict:assert $0 $keysarray $dataarray
				result ($keysarray $dataarray)
			}
		}
	}

	fn dict:set1 keysarray dataarray key value {
		let (
			index=''
		) {
			catch @ e type msg {
				if {~ $e error && ~ $type array && ~ $msg 'out of range'} {
					throw error dict 'key not found'
				} {
					throw $e $type $msg
				}
			} {
				index = <={array:index $keysarray $key}
				array:set $dataarray $index $value
				dict:assert $0 $keysarray $dataarray
				result ($keysarray $dataarray)
			}
		}
	}

	fn dict:set keysarray dataarray key value {
		catch @ e type msg {
			if {~ $e error && ~ $type dict} {
				result <={dict:add $keysarray $dataarray $key $value}
			} {
				throw $e $type $msg
			}
		} {
			result <={dict:set1 $keysarray $dataarray $key $value}
		}
	}

	fn dict:get keysarray dataarray key {
		catch @ e type msg {
			if {~ $e error && ~ $type array && ~ $msg 'out of range'} {
				throw error dict 'key not found'
			} {
				throw $e $type $msg
			}
		} {
			result <={array:index $keysarray $key |> array:get $dataarray}
		}
	}

	fn dict:allkeys keysarray dataarray {
		dict:assert $0 $keysarray $dataarray
		result <={array:getall $keysarray}
	}

	fn dict:allvalues keysarray dataarray {
		dict:assert $0 $keysarray $dataarray
		result <={array:getall $dataarray}
	}

	fn dict:delete keysarray dataarray key {
		catch @ e type msg {
			if {~ $e error && ~ $type array && ~ $msg 'out of range'} {
				throw error dict 'key not found'
			} {
				throw $e $type $msg
			}
		} {
			let (
				index = ''
			) {
				index = <={array:index $keysarray $key}
				array:set $keysarray $index
				array:set $dataarray $index
				dict:assert $0 $keysarray $dataarray
				result ($keysarray $dataarray)
			}
		}
	}

	fn dict:free keysarray dataarray {
		array:delete $keysarray
		array:delete $dataarray
	}

	fn dict:size keysarray dataarray {
		let (
			allkeys = <={dict:allkeys $keysarray $dataarray}
		) {
			dict:assert $0 $keysarray $dataarray
			result ($#allkeys <={array:size $keysarray})
		}
	}

	fn dict:stats keysarray dataarray {
		let (
			allkeys = <={dict:allkeys $keysarray $dataarray}
			allvalues = <={dict:allvalues $keysarray $dataarray}
			keyssz = <={array:size $keysarray}
			datasz = <={array:size $dataarray}
		) {
			dict:assert $0 $keysarray $dataarray
			result ($#allkeys $#allvalues $keyssz $datasz)
		}
	}
}

