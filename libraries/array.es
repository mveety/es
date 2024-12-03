#!/usr/bin/env es

# this library implements arrays using lists of variables. it's a good
# example for the use of pointers

library array (init macros libraries)

fn _array:range start end {
	if {gt $start $end && ! eq $start $end} {
		throw error '_array:range' 'invalid range'
	}
	let(
		res = ()
		i = $start
	) {
		while {lt $i `{add $end 1}} {
			res = $res $i
			i = `{add $i 1}
		}
		result $res
	}
}


fn array:set name elem val {
	lets (
		array = $$name
		elemvar = $array($elem)
	) {
		if {gt $elem $#array} {
			throw error array 'out of range'
		} {
			$elemvar = $val
			result $$elemvar
		}
	}
}

fn array:setall name val {
	lets (
		array = $$name
		i = 1
		n=()
		bounds = $#val
	) {
		if {lt $#array $#val} {
			throw error array:setall 'value larger than array'
		}
		while {lt $i `{add $bounds 1}} {
			n = $array($i)
			$n = $val($i)
			i = `{add $i 1}
		}
	}
}

fn _array:settor {
	lets (
		name = $0
		getter-name = get^'-'^$name
		getter = $$getter-name
		res = ()
	) {
		unwind-protect {
			$getter-name =
			res = $$name
			if {lt $#res $#*} {
				throw error array:settor 'value larger than array'
			}
			array:setall $name $*
			result $res
		} {
			$getter-name = $getter
		}
	}
}

fn array:get name elem {
	lets (
		array = $$name
		elemvar = $array($elem)
	) {
		if {gt $elem $#array} {
			throw error array 'out of range'
		} {
			result $$elemvar
		}
	}
}

fn _array:getall array {
	let (
		res = ()
	) {
		for (i = $array) {
			res = $res $$i
		}
		result $res
	}
}

fn array:getall name {
	result <={_array:getall $$name}
}

fn _array:getter array {
	result <={_array:getall $array}
}

fn array:register name {
	get-$name = $'fn-_array:getter'
	set-$name = $'fn-_array:settor'
}

fn array:unregister name {
	get-$name=
	set-$name=
}

fn array:delete name {
	get-$name=
	for (i = $$name) {
		$i=
	}
	$name=
}

fn array:size name {
	lets (
		array = $$name
	) {
		result $#array
	}
}

fn array:new sz {
	let (
		res = ()
	) {
		for (i = <={_array:range 1 $sz}) {
			res = $res <={gensym '__es_array_'}
		}
		result $res
	}
}

fn array:newset elems {
	let (
		newarray = <={gensym '__es_array_tmp_'}
		res=
	) {
		$newarray = <={array:new $#elems}
		array:setall $newarray $elems
		res = $$newarray
		$newarray=
		result $res
	}
}

fn array:reallocate name newsz {
	lets (
		oldarray = <={array:getall $name}
		oldsize = <={array:size $name}
	) {
		array:delete $name
		$name = <={array:new $newsz}
		oldarray = <={if {lt $newsz $oldsize} {
							result $oldarray(1 ... $newsz)
						} {
							result $oldarray
						}
					}
		array:setall $name $oldarray
	}
}

fn array:index name elem {
	let (
		n = 1
		array = $$name
	) {
		for(i = $array) {
			if {~ $$i $elem} {
				return $n
			}
			n = `{add $n 1}
		}
		throw error array:index 'out of range'
	}
}

fn array:prepend name elem {
	lets (
		newelem = <={gensym '__es_array_'}
	) {
		$newelem = $elem
		$name = $newelem $$name
		result $$name
	}
}

