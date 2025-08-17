#!/usr/bin/env es

fn __es_getnextcase cases {
	let (ncase = $cases; res= ) {
		forever {
			if {~ $#ncase 0} {
				throw error $0 'malformed case list'
			}
			match <={$&termtypeof $ncase(1)} (
				closure {
					res = $res $ncase(1)
					ncase = $ncase(2 ...)
					return @{result $res} @{result $ncase}
				}
				string {
					res = $res $ncase(1)
					ncase = $ncase(2 ...)
				}
				* { throw error $0 'invalid term in case' }
			)
		}
	}
}

fn iserror err {
	if {~ <={$&termtypeof $err} closure && ~ <={$&gettermtag $err} error} {
		return <=true
	}
	return <=false
}

# I might turn this into a rewrite like match. This works fine for now and
# I don't know if the potential performance gain is really needed.
fn errmatch errobj cases {
	# do this test initially to filter out successful trys
	# this lets you roll right into a errmatch after a try without having
	# to do a test
	if {~ $errobj false} { return <=false }
	assert {iserror $errobj}
	let (
		l = $cases
		lf =
		case =
		casef =
		(oe ot om) = <={$errobj info}
		wildcase =
		e =
	) {
		while {gte $#l 1} {
			(e casef lf) = <={try __es_getnextcase $l}
			if {$e} {
				throw error $0 <={$e msg}
			}
			l = <={$lf}
			case = <={$casef}
			local (err = $oe; type = $ot; msg = $om) {
				if {~ $#case 1 && ~ <={$&termtypeof $case} closure} {
					wildcase = $case
				} {
					if {~ $#case 4 && ~ $case(1) $oe && ~ $case(2) $ot && ~ $case(3) $om} {
						return <={$case(4)}
					} {~ $#case 4 && ~ $case(1) $oe && ~ $case(2) '*' && ~ $case(3) $om} {
						return <={$case(4)}
					} {~ $#case 3 && ~ $case(1) $oe && ~ $case(2) $ot} {
						return <={$case(3)}
					} {~ $#case 2 && ~ $case(1) $oe} {
						return <={$case(2)}
					}
				}
			}
		} { throw error $0 'missing arguments' }
		local (err = $oe; type = $ot; msg = $om) {
			if {! ~ $#wildcase 0} {
				return <={$wildcase}
			}
		}
		return <=false
	}
}

