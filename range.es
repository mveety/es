#!/usr/bin/env es

if {~ $#es_conf_%range-use-primitive 0} {
	es_conf_%range-use-primitive = <={
		if {~ range <=$&primitives} {
			result true
		} {
			result false
		}
	}
}

fn %range start0 end0 {
	local(
		start = <={if{gt $start0 $end0}{result $end0}{result $start0}}
		end = <={if{gt $start0 $end0}{result $start0}{result $end0}}
		rev = <={if{gt $start0 $end0}{result true}{result false}}
		res = ()
		i=
	){
		if {$es_conf_%range-use-primitive} {
			res = <={$&range $start $end}
		} {
			i = $start
			while {lte $i $end} {
				res = $res $i
				i = <={add $i 1}
			}
		}
		if{$rev} {
			reverse $res |> result
		} {
			result $res
		}
	}
}

