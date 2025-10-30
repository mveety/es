#!/usr/bin/env es

# this is the string library. it contains helper functions for dealing
# with strings. In addition, it's the first library to use the new
# library naming convention

library string (init libraries)

fn string_length str {
	local (str0 = $:str) {
		return <=true $#str0
	}
}

fn string_find needle haystack {
	local (
		lhaystack = $:haystack
		lneedle = $:needle
		hcount = 1
		ncount = 1
		hsindex = 1
	) {
		while { lt <={sub $hcount 1} $#lhaystack } {
			if {~ $lhaystack(<={add $hcount <={sub $ncount 1}}) $lneedle($ncount) } {
				while {lt <={sub $ncount 1} $#lneedle} {
					hsindex = <={add $hcount <={sub $ncount 1}}
					if { !~ $lhaystack($hsindex) $lneedle($ncount) } {
						break
					} {
						ncount = <={add $ncount 1}
					}
					if { ~ $ncount $#lneedle || gt $ncount $#lneedle } {
						return <=true $hcount
					}
				}
			}
			ncount = 1
			hcount = <={add $hcount 1}
		}
		return <=false
	}
}

fn string_slice str start end {
	local (
		lstr = $:str
		lslice =
	) {
		if { gt $start $#lstr || gt $end $#lstr || gt $start $end } {
			return <=false 0
		} {
			lslice = $lstr($start ... $end)
			return <=true $"lslice
		}
	}
}

fn string_sub str start len {
	return <={string:slice $str $start <={add $start <={sub $len 1}}}
}

fn string_iterator string {
	let (strhead=(); strtail=$:string) {
		result (
			@ { # getchar
				local (s=) {
					s = $strtail(1)
					strhead = $strhead $s
					strtail = $strtail(2 ...)
					result $s
				}
			}
			@ { # ungetchar
				local (s=){
					if {lt <={sub $#strhead 1} 1} { return () }
					s = $strhead($#strhead)
					strhead = $strhead(1 ... <={sub $#strhead 1})
					strtail = $s $strtail
					result $s
				}
			}
			@ { # curchar
				if {lt <={sub $#strhead 1} 1} { return () }
				result $strhead($#strhead)
			}
			@ { %count $strtail }
		)
	}
}

fn string:length { string_length $* }
fn string:find { string_find $* }
fn string:slice { string_slice $* }
fn string:sub { string_sub $* }

