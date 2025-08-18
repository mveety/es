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
fn-errmatch = $&noreturn @ errobj cases {
	# do this test initially to filter out successful trys
	# this lets you roll right into a errmatch after a try without having
	# to do a test
	if {~ $errobj false} { result <=false } {
		assert {iserror $errobj}
		let (
			l = $cases
			lf =
			case =
			casef =
			(oe ot om) = <={$errobj info}
			wildcase =
			e =
			matchedcase = false
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
							matchedcase = true
							$case(4)
							break
						} {~ $#case 4 && ~ $case(1) $oe && ~ $case(2) '*' && ~ $case(3) $om} {
							matchedcase = true
							$case(4)
							break
						} {~ $#case 3 && ~ $case(1) $oe && ~ $case(2) $ot} {
							matchedcase = true
							$case(3)
							break
						} {~ $#case 2 && ~ $case(1) $oe} {
							matchedcase = true
							$case(2)
							break
						}
					}
				}
			} { throw error errmatch 'missing arguments' }
			local (err = $oe; type = $ot; msg = $om) {
				if {! $matchedcase && ! ~ $#wildcase 0} {
					$wildcase
				}
			}
			false
		}
	}
}

