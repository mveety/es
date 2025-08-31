#!/usr/bin/env es

library advanced_range (init)

if {~ $#es_conf_%range-use-primitive 0} {
	es_conf_%range-use-primitive = <={
		if {~ range <=$&primitives} {
			result true
		} {
			result false
		}
	}
}

fn range_toglob list {
	result '['^$"list^']'
}

let (
	letters = a b c d e f g h i j k l m n o p q r s t u v w x y z
	capletters = A B C D E F G H I J K L M N O P Q R S T U V W X Y Z
	numbers = 0 1 2 3 4 5 6 7 8 9
) {
	fn range_number2list n list {
		assert2 number2list {lte $n $#list}
		%elem $n $list
	}

	fn range_number2allletter n { range_number2list $n $letters $capletters }
	fn range_number2capletter n { range_number2list $n $capletters }
	fn range_number2letter n { range_number2list $n $letters }
	fn range_number2all n { range_number2list $n $numbers $letters $capletters }

	fn range_list2number l list {
		assert2 letter2number {eq 1 <={%count $:l}}
		local (i = 1) {
			for (li = $list) {
				if {~ $l $li} {
					return $i
				}
				i = <={add $i 1}
			}
		}
	}

	fn range_letter2number l { range_list2number $l $letters }
	fn range_capletter2number l { range_list2number $l $capletters }
	fn range_allletter2number l { range_list2number $l $letters $capletters }
	fn range_all2number l { range_list2number $l $numbers $letters $capletters }

	fn range_getchartype c {
		let (
			numglob = <={range_toglob $numbers}
			letterglob = <={range_toglob $letters}
			capglob = <={range_toglob $capletters}
			allletglob = <={range_toglob $letters $capletters}
		) {
			match $c (
				@{glob_test $numglob $1} { return number }
				@{glob_test $letterglob $1} { return letter }
				@{glob_test $capglob $1} { return capletter }
				* { throw error range_getchartype 'unknown symbol' }
			)
		}
	}

	fn range_getstrtype s {
		assert2 $0 {eq $#s 1}
		if {gt <={%count $:s} 1} {
			for (i = $:s) {
				if {! ~ <={range_getchartype $i} number} {
					throw error range_getstrtype 'invalid number: '^$s
				}
			}
			return multi_number
		} {eq <={%count $:s} 1} {
			range_getchartype $:s
		}
	}

}

if {~ $#fn-range_range 0} {
	fn-range_range = $fn-%range
}

fn range_allrange start0 end0 {
	let (
		starttype = <={range_getstrtype $start0}
		endtype = <={range_getstrtype $end0}
		start =
		end =
	) {
		match $starttype (
			multi_number {
				if {! ~ $endtype multi_number number} { throw error $0 'invalid argument types' }
				range_range $start0 $end0
			}
			number {
				match $endtype (
					(number multi_number) { range_range $start0 $end0 }
					(letter capletter) {
						start = <={range_all2number $start0}
						end = <={range_all2number $end0}
						range_range $start $end |> iterator |> do @{range_number2all $1}
					}
				)
			}
			(letter) {
				assert2 $0 {! ~ $endtype multi_number}
				match $endtype (
					number {
						start = <={range_all2number $start0}
						end = <={range_all2number $end0}
						range_range $start $end |> iterator |> do @{range_number2all $1}
					}
					capletter {
						start = <={range_allletter2number $start0}
						end = <={range_allletter2number $end0}
						range_range $start $end |> iterator |> do @{range_number2allletter $1}
					}
					letter {
						start = <={range_letter2number $start0}
						end = <={range_letter2number $end0}
						range_range $start $end |> iterator |> do @{range_number2letter $1}
					}
				)
			}
			(capletter) {
				assert2 $0 {! ~ $endtype multi_number}
				match $endtype (
					number {
						start = <={range_all2number $start0}
						end = <={range_all2number $end0}
						range_range $start $end |> iterator |> do @{range_number2all $1}
					}
					capletter {
						start = <={range_capletter2number $start0}
						end = <={range_capletter2number $end0}
						range_range $start $end |> iterator |> do @{range_number2capletter $1}
					}
					letter {
						start = <={range_allletter2number $start0}
						end = <={range_allletter2number $end0}
						range_range $start $end |> iterator |> do @{range_number2allletter $1}
					}
				)
			}
		)
	}
}

fn-%range = $fn-range_allrange

