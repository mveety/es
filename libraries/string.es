#!/usr/bin/env es

# this is the string library. it contains helper functions for dealing
# with strings. In addition, it's the first library to use the new
# library naming convention

library string (init libraries)

fn string:length str {
	local (str0 = $:str) {
		echo $#str0
		return true
	}
}

fn string:find needle haystack {
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
						echo $hcount
						return true
					}
				}
			}
			ncount = 1
			hcount = <={add $hcount 1}
		}
		echo 0
		return false
	}
}

fn string:slice str start end {
	local (
		lstr = $:str
		lslice =
	) {
		if { gt $start $#lstr || gt $end $#lstr || gt $start $end } {
			echo 0
			return false
		} {
			lslice = $lstr($start ... $end)
			echo $"lslice
			return true
		}
	}
}

fn string:sub str start len {
	local (
		slice_end = <={add $start $len}
		rval =
	) {
		rval = <-{string:slice $str $start $slice_end}
		echo $rval(2)
		return $rval(1)
	}
}
