
fn es_get_confvars {
	let (allvars = <=$&vars <=$&internals) {
		process $allvars (
			(fn-* get-* set-*) { result () }
			*_conf_* { result $matchexpr }
			* { result () }
		)
	}
}

fn es_sort_confvars cvars {
	let (
		res = <=dictnew
		pkg= ; name=
		t =
	) {
		for (cv = $cvars) {
			(pkg name) = <={~~ $cv *_conf_*}
			catch @ e t m {
				if {~ $e error && ~ $t '$&dictget'} {
					res = <={dictput $res $pkg ($name)}
				} {
					throw $e $t $m
				}
			} {
				t = <={dictget $res $pkg}
				res = <={dictput $res $pkg $t $name}
			}
		}
		result $res
	}
}

fn esconf_packages {
	dictnames <={es_sort_confvars <=es_get_confvars}
}

fn esconf_havepackage pkg {
	if {~ $pkg <=esconf_packages} {
		result <=true
	} {
		result <=false
	}
}

fn esconf_havevar pkg varname {
	lets (
		confdict = <={es_sort_confvars <=es_get_confvars}
		pkgvars = <={dictget $confdict $pkg}
	) {
		if {~ $varname $pkgvars} {
			result <=true
		} {
			result <=false
		}
	}
}

fn esconf_printpackage raw pkg {
	let (confdict = <={es_sort_confvars <=es_get_confvars}) {
		echo 'package '^$pkg^':'
		for (vn = <={dictget $confdict $pkg}) {
			echo -n '    '
			let (var = $pkg^_conf_^$vn) {
				if {$raw} {
					echo $var^' = '^<={$&fmtvar $var}
				} {
					echo $vn^' = '^<={$&fmtvar $var}
				}
			}
		}
	}
}

fn esconf_printall raw {
	let (
		confdict = <={es_sort_confvars <=es_get_confvars}
	) {
		dictforall $confdict @ n v {
			echo 'package '^$n^':'
			for (vn = $v) {
				echo -n '    '
				let (var = $n^_conf_^$vn) {
					if {$raw} {
						echo $var^' = '^<={$&fmtvar $var}
					} {
						echo $vn^' = '^<={$&fmtvar $var}
					}
				}
			}
		}
	}
}				

fn esconf_printvar raw pkg name {
	let (var = $pkg^_conf_^$name) {
		if {$raw} {
			echo $var^' = '^<={$&fmtvar $var}
		} {
			echo $pkg^': '^$name^' = '^<={$&fmtvar $var}
		}
	}
}

fn esconf_printusage {
	echo >[1=2] 'usage: conf [-vr] -p package [var]'
	echo >[1=2] '       conf [-v] [-A | -P] -p package -s var value'
	echo >[1=2] '       conf [-vr] -a'
	echo >[1=2] '       conf -p'
}

fn conf args {
	let (
		mode = print # if -s then set, if -h then help
		all = false # -a
		package = __es_none # -p [package]
		raw = false # -r
		varname =
		value =
		rest =
		usageexit = false
		verbose = false # -v
		setmode = set # -P is prepend, -A is append
	) {
		(_ rest) = <={parseargs @ arg {
			match $arg (
				(-a) { all = true ; done }
				(-p) {
					if {has_argument} {
						package = $flagarg
					} {
						mode = printpackages
						done
					}
				}
				(-r) { raw = true }
				(-s) { mode = set ; done }
				(-v) { verbose = true }
				(-A) { setmode = append }
				(-P) { setmode = prepend }
				(-h) { mode = help ; usage }
				* { usage }
			)
		} @ {
			usageexit = true
			esconf_printusage
		} $args}

		if {$usageexit} { return <=false }

		if {~ $mode printpackages} {
			echo 'available packages:' <={esconf_packages}
			return <=true
		}

		if {! $all && ~ $package __es_none} {
			echo >[1=2] 'error: invalid options'
			esconf_printusage
			return <=false
		}

		if {$all} {
			esconf_printall $raw
			return <=true
		}

		if {! esconf_havepackage $package} {
			echo >[1=2] 'error: package '^$package^' not found'
			return <=false
		}

		if {~ $rest(1) -s} { rest = $rest(2 ...) }
		(varname value) = $rest

		match $mode (
			(print) {
				if {~ $#varname 0} {
					esconf_printpackage $raw $package
					return <=true
				} {
					if {! esconf_havevar $package $varname} {
						echo >[1=2] 'error var '^$varname^' in '^$package^' not found'
						return <=false
					}
					esconf_printvar $raw $package $varname
					return <=true
				}
			}
			(set) {
				if {~ $#varname 0} {
					echo >[1=2] 'error: missing variable name'
					esconf_printusage
					return <=false
				}
				if {~ $#value 0} {
					echo >[1=2] 'error: missing value'
					esconf_printusage
					return <=false
				}
				if {! esconf_havevar $package $varname} {
					echo >[1=2] 'error var '^$varname^' in '^$package^' not found'
					return <=false
				}
				let (v = $package^_conf_^$varname) {
					match $setmode (
						append { $v = ($$v $value) }
						prepend { $v = ($value $$v) }
						set { $v = $value }
					)
					if {$verbose} {
						echo >[1=2] $v^' = '^<={$&fmtvar $v}
					}

				}
				return <=true
			}
			* {
				esconf_printusage
				return <=true
			}
		)
	}
}

