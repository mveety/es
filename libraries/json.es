# object dict: %dict(type => $type ; value => $value)
# json object dict: %dict(key => [object dict|string|jsonobj])
# json array: ([object dict|string]...)

with-dynlibs mod_json {
	library json (init libraries)

	defconf json formatted-output true

	fn-%json_decode = $&json_decode
	fn-%json_encode = $&json_encode
	fn-%json_encode_formatted = $&json_encode_formatted
	fn-%json_create = $&json_create
	fn-%json_addto = $&json_addto
	fn-%json_gettype = $&json_gettype
	fn-%json_getobject = $&json_getobject
	fn-%json_detachobject = $&json_detachobject
	fn-%json_getdata = $&json_getdata
	fn-%json_getobjectnames = $&json_getobjectnames

	fn json_decode json_string {
		%json_decode $json_string
	}

	fn json_encode jsonobj {
		if {$json_conf_formatted-output} {
			%json_encode_formatted $jsonobj
		} {
			%json_encode $jsonobj
		}
	}

	fn json_readfile file {
		if {! access -r $file } {
			throw error json_readfile 'unable to read '^$file
		}
		%json_decode ``(){cat $file}
	}

	fn json_writefile jsonobj file {
		{
			json_encode $jsonobj |> echo
		} > $file
	}

	fn json_create objdict {
		match $objdict(type) (
			(object array) { %json_create $matchexpr }
			* { %json_create $objdict(type) $objdict(value) }
		)
	}

	fn json_addobject jsonobj dict {
		let (
			newobjdict=
			names = <={dictnames $dict}
			newobj=
			value=
		){
			for	(name = $names) {
				value = $dict($name)
				match <={$&termtypeof $value} (
					* { newobj = <={json_create %dict(type=>string;value=>$^value)} }
					dict { newobj = <={json_create $value} }
					object:json { newobj = $value}
				)
				jsonobj = <={%json_addto $jsonobj $newobj $name}
			}
			return $jsonobj
		}
	}

	fn json_addarray jsonobj objlist {
		for (obj = $objlist) {
			let (newobj=) {
				match <={$&termtypeof $obj} (
					* { newobj = <={json_create %dict(type=>string;value=>$^obj)} }
					dict { newobj = <={json_create $obj} }
					object_json { newobj = $obj }
				)
				jsonobj = <={%json_addto $jsonobj $newobj}
			}
		}
		return $jsonobj
	}

	fn json_add jsonobj args {
		let ((type _) = <={%json_gettype $jsonobj}) {
			match $type (
				* { throw error json_add 'object must be a json array or json object' }
				object { json_addobject $jsonobj $args }
				array { json_addarray $jsonobj $args }
			)
		}
	}

	fn json_get jsonobj name-or-index {
		match <={%json_gettype $jsonobj} (
			* { throw error json_get 'object must be a json array or json object' }
			object { %json_getobject $jsonobj $name-or-index }
			array { %json_getobject $jsonobj $name-or-index }
		)
	}

	fn json_todict1 jsonobj {
		match <={%json_gettype $jsonobj} (
			(number string) { result %dict(type => $matchexpr; value => <={%json_getdata $jsonobj}) }
			null { result %dict(type => null) }
			boolean {
				let (res = %dict(type=>boolean)) {
					if {%json_getdata $jsonobj} {
						res := value => true
					} {
						res := value => false
					}
					result $res
				}
			}
			* { throw error json_todict1 'object must not be a json array or json object' }
		)
	}

	fn json_todict_array jsonobj {
		if {! ~ <={%json_gettype $jsonobj} array} {
			throw error json_todict_array 'objects must be a json array'
		}
		let (
			res = ()
			(type len) = <={%json_gettype $jsonobj}
			i = 0
		) {
			while {lt $i $len} {
				res = $res <={json_get $jsonobj $i |> json_todict}
				i = <={add $i 1}
			}
			result %dict(type => array; value => $res)
		}
	}

	fn json_todict_object jsonobj {
		if {! ~ <={%json_gettype $jsonobj} object} {
			throw error json_todict_object 'objects must be a json array'
		}
		let (
			res = %dict()
		) {
			for (n = <={%json_getobjectnames $jsonobj}) {
				let (subobj = <={%json_getobject $jsonobj $n}) {
				 	res := $n => <={json_todict $subobj}
				}
			}
			result %dict(type=>object;value=>$res)
		}
	}

	fn json_todict jsonobj {
		match <={%json_gettype $jsonobj} (
			(array) { json_todict_array $jsonobj }
			(object) { json_todict_object $jsonobj }
			* { json_todict1 $jsonobj }
		)
	}
}

