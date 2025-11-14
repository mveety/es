library complete_dynlibs (init completion)

if {conf -X es:dynlibs} {

	# for %dynlib_prims, %dynlib_file, and %close_dynlib
	fn complete_dynlib_loaded_hook curline partial {
		let (res = ()) {
			for (dlib = <=%dynlibs){
				if {~ $dlib $partial^*} {
					res += $dlib
				}
			}
			result $res
		}
	}

	fn complete_dynlib_unloaded curline partial {
		lets (
			platform = <={%split ' ' $buildstring |> %elem 5}
			arch = <={%split ' ' $buildstring |> %elem 6}
			ext = <={conf -X es:dynlib-extension}
			sysdynlib = $corelib/*.$platform.$arch.$ext
			userdynlib = $libraries/*.$platform.$arch.$ext
			cwddynlib = ./*.$ext
			dynlibs = ()
			loaded_libs = <=%dynlibs
			res = ()
		) {
			for (l = $sysdynlib) {
				if {~ $l *^'*'^*} {
					continue
				}
				let (name = <={~~ $l $corelib/*.$platform.$arch.$ext}) {
					dynlibs += $name
				}
			}
			for (l = $userdynlib) {
				if {~ $l *^'*'^*} {
					continue
				}
				let (name = <={~~ $l $libraries/*.$platform.$arch.$ext}) {
					dynlibs += $name
				}
			}
			if {conf -X es:dynlib-local-search} {
				for (l = $cwddynlib) {
					let (name = <={~~ $l ./*.$ext}) {
						dynlibs += $name
					}
				}
			}
			dynlibs = <={remove-duplicates $dynlibs}
			for (d = $dynlibs) {
				if {~ $d $loaded_libs} {
					continue
				}
				if {~ $d $partial^*} {
					res += $d
				}
			}
			result $res
		}
	}


	%complete_cmd_hook %close_dynlib @ curline partial {
		complete_dynlib_loaded_hook $curline $partial
	}

	%complete_cmd_hook %dynlib_prims @ curline partial {
		complete_dynlib_loaded_hook $curline $partial
	}

	%complete_cmd_hook %dynlib_file @ curline partial {
		complete_dynlib_loaded_hook $curline $partial
	}

	%complete_cmd_hook load-dynlib @ curline partial {
		complete_dynlib_unloaded $curline $partial
	}

	if {checkoption libutils} {
		%complete_cmd_hook dynlib_info @ curline partial {
			complete_dynlib_loaded_hook $curline $partial
		}
	}

}
