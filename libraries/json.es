with-dynlibs mod_json {
	library json (init libraries)

	fn-%json_decode = $&json_decode
	fn-%json_encode = $&json_encode
	fn-%json_encode_formatted = $&json_encode_formatted
	fn-%json_create = $&json_create
	fn-%json_addto = $&json_addto
	fn-%json_gettype = $&json_gettype
	fn-%json_getobject = $&json_getobject
	fn-%json_getdata = $&json_getdata

	fn json_readfile file {
		if {! access -r $file } {
			throw error json_readfile 'unable to read '^$file
		}
		%json_decode ``(){cat $file}
	}
}

