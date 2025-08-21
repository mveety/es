#!/usr/bin/env es

#	The vars function is provided for cultural compatibility with
#	rc's whatis when used without arguments.  The option parsing
#	is very primitive;  perhaps es should provide a getopt-like
#	builtin.
#
#	The options to vars can be partitioned into two categories:
#	those which pick variables based on their source (-e for
#	exported variables, -p for unexported, and -i for internal)
#	and their type (-f for functions, -s for settor functions,
#	and -v for all others).
#
#	Internal variables are those defined in initial.es (along
#	with pid and path), and behave like unexported variables,
#	except that they are known to have an initial value.
#	When an internal variable is modified, it becomes exportable,
#	unless it is on the noexport list.

fn es_old_vars {
	# choose default options
	* = <={es_canonicalize_args $*}
	if {~ $* -a} {
		* = -v -f -s -g -e -p -i
	} {
		if {!~ $* -[vfsg]}	{ * = $* -v }
		if {!~ $* -[epi]}	{ * = $* -e }
	}
	# check args
	for (i = $*)
		if {!~ $i -[vfsgepi]} {
			throw error vars illegal option: $i -- usage: vars '-[vfsgepia]'
		}
	let (
		vars	= false
		fns	= false
		sets	= false
		gets	= false
		export	= false
		priv	= false
		intern	= false
	) {
		process $* (
			(-v) {vars = true}
			(-f) {fns = true}
			(-s) {sets = true}
			(-g) {gets = true}
			(-e) {export = true}
			(-p) {priv = true}
			(-i) {intern = true}
			* {throw error vars bad option: $i}
		)
		let (
			dovar = @ var {
				# print functions and/or settor vars
				if {if {~ $var fn-*} $fns {~ $var set-*} $sets {~ $var get-*} $gets $vars} {
					echo <={%var $var}
				}
			}
		) {
			if {$export || $priv} {
				for (var = <= $&vars)
					# if not exported but in priv
					if {if {~ $var $noexport} $priv $export} {
						$dovar $var
					}
			}
			if {$intern} {
				for (var = <= $&internals)
					$dovar $var
			}
		}
	}
}

fn es_new_vars {
	let (
		vars = false # -v
		fns = false # -f
		sets = false # -s
		gets = false # -g
		config = false # -c
		export = false # -e
		priv = false # -p
		intern = false # -i
		usageexit = false
		fullhelp = false
		selected = false
		modified = false
		printable_vars = ()
		search = false
		search_glob = '*'
	) {
		parseargs @ arg {
			match $arg (
				# modifiers
				(-e) { export = true ; modified = true }
				(-p) { priv = true ; modified = true }
				(-i) { intern = true ; modified = true }
				# object types
				(-v) { vars = true ; selected = true }
				(-f) { fns = true ; selected = true }
				(-s) { sets = true ; selected = true }
				(-g) { gets = true ; selected = true }
				(-c) { config = true ; selected = true }
				# other
				(-a) {
					modified = true
					selected = true
					export = true
					priv = true
					intern = true
					vars = true
					fns = true
					sets = true
					gets = true
					config = true
				}
				(-M) {
					modified = true
					export = true
					priv = true
					intern = true
				}
				(-O) {
					selected = true
					vars = true
					fns = true
					sets = true
					gets = true
					config = true
				}
				(-S) { search = true ; search_glob = $flagarg ; done}
				(-h) { fullhelp = true ; usage }
				(-^'?') { usage }
				* { usage }
			)
		} @ {
			echo 'usage: vars [-a | -vfsgcOepiM] [-S glob]'
			if {$fullhelp} {
				echo '    -a        -- all objects and modifiers'
				echo '    -S [glob] -- filter selected objects'
				echo ''
				echo 'selectors'
				echo '    -v        -- variables (default)'
				echo '    -f        -- functions'
				echo '    -s        -- settors'
				echo '    -g        -- getters'
				echo '    -c        -- config'
				echo '    -O        -- all objects'
				echo 'object types'
				echo '    -e        -- exported (default)'
				echo '    -p        -- private'
				echo '    -i        -- internal'
				echo '    -M        -- all objects'
				echo ''
				echo 'no arguments is the same as vars -ve'
			}
			usageexit = true
		} $*

		if {$usageexit} { return 0 }
		if {! $selected} { vars = true }
		if {! $modified} { export = true }

		if {! ~ $search_glob *^'*'^*} {
			search_glob = '*'^$search_glob^'*'
		}

		let (
			fn-selected = @ v {
				match $v (
					(fn-*) { $fns }
					(set-*) { $sets }
					(get-*) { $gets }
					(es_conf_*) { $config }
					* { $vars }
				)
			}
			fn-printable = @ v { if {~ $v $noexport} $priv $export }
			fn-allvars = @ { if {$search} { $&vars |> glob $search_glob } { $&vars } }
			fn-allinternals = @{if {$search} {$&internals |> glob $search_glob} {$&internals}}
		) {
			if {$export || $priv} {
				for (x = <=allvars) {
					if {selected $x && printable $x} {
						%var $x |> echo
					}
				}
			}
			if {$intern} {
				for (x = <=allinternals) {
					if {selected $x} {
						%var $x |> echo
					}
				}
			}
		}
	}
}

fn vars {
	if {$es_conf_use_new_vars} {
		es_new_vars $*
	} {
		es_old_vars $*
	}
}

