#!/usr/bin/env es

# es library search path support. This will allow users to share
# libraries globally on a system.

# corelib = '/usr/local/share/es/'

if {~ $#libraries 0} { libraries = () }
if {~ $#enable-import 0} { enable-import = true }
if {~ $#import-panic 0} { import-panic = false }
if {~ $#automatic-import 0} { automatic-import = true }

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
		import-core-lib $lib
	} {
		import-user-lib $lib
	}
	result 0
}

fn import {
	if {! $enable-import} {
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
			throw $e $type $msg
		} {
			import_file $i^'.es'
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
			if { $automatic-import } {
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

option libraries

