#!/usr/bin/env es

fn print_square {
	%range 1 9 |>
		@{echo $* ; result $*} |>
		%rest |>
		iterator |>
		do @{
			echo -n $1
			for (_ = <={%range 1 <={add <={sub $1 1} <={sub $1 2}}}) {
				echo -n ' '
			}
			echo '*'
		}
}

print_square

