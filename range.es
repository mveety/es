#!/usr/bin/env es

fn %range start0 end0 {
	local(
		start = <={if{gt $start0 $end0}{result $end0}{result $start0}}
		end = <={if{gt $start0 $end0}{result $start0}{result $end0}}
		rev = <={if{gt $start0 $end0}{result true}{result false}}
	){
		if{$rev} {
			$&range $start $end |> reverse $res |> result
		} {
			$&range $start $end |> result
		}
	}
}

