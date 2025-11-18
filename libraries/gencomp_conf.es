library gencomp_conf (general_completion)

let (
	fn gc_conf_complete_package partial {
		gencomp_filter_list $partial <={conf -Xa |> dictkeys}
	}

	fn gc_conf_complete_var package partial {
		let ((e vars) = <={try {conf -Xa |> @{dictget $1 $package}}}) {
			if {$e} { return () }
			gencomp_filter_list $partial $vars
		}
	}
) {
	fn gc_complete_conf cmdline partial {
		let (
			parsed_line = <={gencomp_split_cmdline $cmdline}
		) {
			if {~ $parsed_line(1) $es_conf_completion-prefix-commands} {
				parsed_line = $parsed_line(2 ...)
			}
			if {~ $partial *^':'^* || ~ $partial *^':'} {
				let ((pkg var) = <={~~ $partial *^':'^*};restmp=) {
					if {~ $#var 0} {var = ''}
					restmp = <={gc_conf_complete_var $pkg $var}
					return $pkg^':'^$restmp
				}
			}
			if {~ $parsed_line($#parsed_line) -p} {
				return <={gc_conf_complete_package $partial}
			}
			let (
				found_pkg = false
				pkgname = ''
				varname = ''
				next = none
				cannon_args = <={es_canonicalize_args $parsed_line(2 ...)}
			) {
				for (i = $cannon_args) {
					if {~ $i -p} {
						next = package
						continue
					}
					if {~ $next package} {
						found_pkg = true
						pkgname = $i
						break
					}
				}
				if {$found_pkg} {
					return <={gc_conf_complete_var $pkgname $partial}
				}
				let (r = <={gc_conf_complete_package $partial}) {
					if {~ $#r 1} {
						return $r^':'^<={gc_conf_complete_var $r ''}
					}
					return $r^':'
				}
			}
		}
	}
}

%complete_cmd_hook conf @ curline partial {
	gc_complete_conf $curline $partial
}

