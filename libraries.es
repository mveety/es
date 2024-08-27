#!/usr/local/bin/es

# es library search path support. This will allow users to share
# libraries globally on a system.

corelib = '/usr/local/share/es/'
libraries = ()
enable-import = 'yes'
import-panic = false
automatic-import = false

fn import-core-lib lib {
	dprint 'import-core-lib exec''d'
	let(libname = <={access -n $lib -1 -r $corelib}) {
		if {! ~ $#libname 0} {
			dprint 'loading '^$libname
			. $libname
			return 0
		} {
			return 1
		}
	}
}

fn import-user-lib lib {
	dprint 'import-user-lib-exec''d'
	if {~ $#libraries 0} {
		return 1
	} {
		let (libname = <={access -n $lib -1 -r $libraries}) {
			if {~ $#libname 0} {
				return 1
			} {
				. $libname
				return 0
			}
		}
	}
}

fn import lib {
	if {~ $#enable-import 0} {
		panic 'import' 'import is not enabled'
		return 1
	}
	if {! import-user-lib $lib} {
		dprint 'no userlib named '^$lib
		if {! import-core-lib $lib} {
			if {$import-panic} {
				panic 'import' $lib^' not found'
			} {
				echo 'import: '^$lib^' not found'
			}
			return 1
		}
	}
	return 0
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
				if {! import $i^'.es'} {
					panic 'library' 'error: library: unable to load '^$i
					return false
				}
			} {
				panic 'library' 'required library '^$i^' not loaded'
				return false
			}
		}
	}
	return true
}

fn library name requirements {
	if { <={check_and_load_options $requirements} } {
		option $name
	} {
		panic 'library' 'unable to load library '^$name
	}
}

option libraries

