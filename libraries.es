#!/usr/bin/env es

# es library search path support. This will allow users to share
# libraries globally on a system.

# corelib = '/usr/local/share/es/'

set-libraries = @{ local (set-es_conf_libraries =) { es_conf_libraries = $* }; result $* }
set-es_conf_libraries = @{ local (set-libraries =) { libraries = $* }; result $* }

if {~ $#libraries 0} { libraries = () }
if {~ $#default_libraries 0} { default_libraries = () }
if {~ $#es_conf_library-import 0} { es_conf_library-import = true }
if {~ $#es_conf_import-panic 0} { es_conf_import-panic = true }
if {~ $#es_conf_automatic-import 0} { es_conf_automatic-import = true }
if {~ $#es_conf_library-import-once 0} { es_conf_library-import-once = false }

fn __es_libraries_initialize {
	local (
		datadir = <={
			local (tmp=) {
				if {~ $#XDG_CONFIG_HOME 0} {
					tmp = $home/.config/es-mveety
				} {
					tmp = $XDG_CONFIG_HOME/es-mveety
				}
				if {access -rw $tmp} {
					result $tmp
				} {
					result ()
				}
			}
		}
	) {
		if {~ $#datadir 0} {
			if {access -rw $home/eslib} {
				libraries = $home/eslib
			}
		} {
			libraries = $datadir
		}
		default_libraries = $libraries
	}
}


fn import-core-lib lib {
	let (libname = <={access -n $lib -1 -r $corelib}) {
		if {! ~ $#libname 0} {
			dprint 'loading '^$libname
			. $libname
			return 0
		} {
			throw error $0 $lib^' not found'
		}
	}
}

fn import-user-lib lib {
	if {~ $#libraries 0} {
		throw error $0 '$libraries not set'
	} {
		let (libname = <={access -n $lib -1 -r $libraries}) {
			if {~ $#libname 0} {
				throw error $0 $lib^' not found'
			} {
				dprint 'loading '^$libname
				. $libname
				return 0
			}
		}
	}
}

fn import_file lib {
	catch @ e type msg {
		if {! ~ $e error} {
			throw $e $type $msg
		}
		if {~ $type already_imported} {
			throw $e $type $msg
		}
		import-core-lib $lib
	} {
		import-user-lib $lib
	}
	result 0
}

fn import {
	if {! $es_conf_library-import} {
		throw error 'import' 'import is not enabled'
	}
	if {~ $#* 0} {
		throw usage import 'usage: import [libraries]'
	}
	for(i = $*) {
		catch @ e type msg {
			if {~ $e error && ~ $type import-core-lib} {
				throw error import 'library '^$i^' not found'
			}
			if {~ $e library_error && ~ $type already_imported} {
				throw error import 'library '^$msg
			}
			throw $e $type $msg
		} {
			catch @ e t m {
				if {$es_conf_import-panic} {
					throw $e $t $m
				}
				return <=true
			} {
				import_file $i^'.es'
			}
		}
	}
}

fn checkoption opt {
	for (i = $options) {
		if {~ $i $opt } { return 0 }
	}
	return 1
}

fn option opt {
	if {! checkoption $opt } {
		options = $options $opt
	}
}

fn check_and_load_options opts {
	for (i = $opts) {
		if {! checkoption $i} {
			if { $es_conf_automatic-import } {
				catch @ e type msg {
					if {~ $type import} {
						throw error library 'unable to load '^$i
					}
					throw $e $type $msg
				}{
					import $i
				}
			} {
				panic 'library' 'required library '^$i^' not loaded'
			}
		}
	}
	return true
}

fn library name requirements {
	catch @ e type msg {
		if {~ $type library} {
			throw error library 'unable to load library '^$name^': '^$msg
		}
		throw $e $type $msg
	} {
		check_and_load_options $requirements
		if {checkoption $name && $es_conf_library-import-once} {
			throw library_error already_imported $name^' is already imported'
		}
		option $name
	}
}

fn havelib name {
	let (
		syslibname = <={access -n $name.es -1 -r $corelib}
		userlibname = <={access -n $name.es -1 -r $libraries}
	) {
		if {! ~ $#userlibname 0} { return <=true }
		if {! ~ $#syslibname 0} { return <=true }
		return <=false
	}
}

if {~ <=$&primitives opendynlib} {

	if {~ $#es_conf_dynlib-automatic-reload 0} {
		es_conf_dynlib-automatic-reload = false
		set-es_conf_dynlib-automatic-reload = @ arg args {
			if {! ~ $arg true false } {
				result $es_conf_dynlib-automatic-reload
			} {
				result $arg
			}
		}
	}

	if {~ $#es_conf_dynlib-extension 0} {
		set-es_conf_dynlib-extension = @{
			result <={match <={%split ' ' $buildstring |> %elem 5} (
				'Darwin' { result 'dylib' }
				* { result 'so' }
			)}
		}
		es_conf_dynlib-extension = set
		noexport += es_conf_dynlib-extension
	}

	fn-%dynlibs = $&listdynlibs
	fn-%dynlib_prims = $&dynlibprims
	fn-%dynlib_file = $&dynlibfile
	fn-%open_dynlib = $&opendynlib
	fn-%close_dynlib = $&closedynlib


	fn load-dynlib lib {
		lets (
			platform = <={%split ' ' $buildstring |> %elem 5}
			arch = <={%split ' ' $buildstring |> %elem 6}
			ext = $es_conf_dynlib-extension
			sysdynlib = <={access -n $lib.$platform.$arch.$ext -1r $corelib}
			userdynlib = <={access -n $lib.$platform.$arch.$ext -1r $libraries}
			e =
		) {
			if {~ <=%dynlibs $lib} {
				if {$es_conf_dynlib-automatic-reload} {
					%close_dynlib $lib
				} {
					throw error load-dynlib 'dynamic library '^$lib^' already loaded'
				}
			}
			if {$es_conf_dynlib-local-search} {
				(e _) = <={try %open_dynlib ./$lib.$ext}
				if {! $e} {
					return <=true
				} {! ~ <={$e msg} *^'Cannot open'^*} {
					throw $e
				}
			}
			if {! ~ $#userdynlib 0} {
				(e _) = <={try %open_dynlib $userdynlib}
				if {! $e} {
					return <=true
				} {! ~ <={$e msg} *^'Cannot open'^*} {
					throw $e
				}
			}
			if {! ~ $#sysdynlib 0} {
				(e _) = <={try %open_dynlib $sysdynlib}
				if {! $e} {
					return <=true
				} {! ~ <={$e msg} *^'Cannot open'^*} {
					throw $e
				}
			}
			throw error load-dynlib 'unable to find dynamic library '^$lib
		}
	}

	fn is-dynlib-loaded lib {
		if {~ <=%dynlibs $lib} {
			result <=true
		} {
			result <=false
		}
	}

	fn with-dynlibs argsbody {
		local(
			args = <={__es_getargs $argsbody}
			body = <={__es_getbody $argsbody}
		) {
			if {~ $#args 0} { throw error with-dynlibs 'missing dynamic libraries' }
			if {! ~ $#body 1} { throw error with-dynlibs 'missing body' }
			for (dl = $args) {
				if {! ~ <=%dynlibs $dl} {
					load-dynlib $dl
				}
			}
			result <={$body}
		}
	}

} {
	let (failfn = @{ throw error 'es:dynlibs' 'dynamic libraries not enabled' }) {
		fn-%dynlibs = $failfn
		fn-%dynlib_prims = $failfn
		fn-%dynlib_file = $failfn
		fn-%open_dynlib = $failfn
		fn-%close_dynlib = $failfn
		fn-load-dynlib = $failfn
		fn-is-dynlib-loaded = $failfn
		fn-with-dynlibs = $failfn
	}
}

option libraries

