# object dict: %dict(type => $type ; value => $value)
# json object dict: %dict(key => [object dict|string])
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
		local(
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
}

