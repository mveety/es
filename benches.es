#!/usr/bin/env es

if {~ $#bench_run_caret_srcdata 0} {
	bench_run_caret_srcdata = false
}

if {~ $#bench_print_gcinfo 0} {
	bench_print_gcinfo = false
}

lets (
	srcdata=
	srcdata_len=
	no_srcdata=true
	old_gcinfo = $fn-%gcinfo
	fn-%gcinfo = @{ if {$bench_print_gcinfo} { $old_gcinfo -r } }
	){

	fn populate_source_data elems {
		assert2 $0 {gt $elems 0}
		srcdata = <={%range 1 $elems}
		srcdata_len = $#srcdata
		no_srcdata = false
	}

	fn have_srcdata {
		if {$no_srcdata} {
			throw error benches 'missing source data'
		}
	}

	fn bench-prepend_list {
		have_srcdata
		local(y=){
			for (i = $srcdata){
				y = $i $y
			}
		}
		%gcinfo
	}

	fn bench-postpend_list {
		have_srcdata
		local (y=){
			for (i = $srcdata){
				y = $y $i
			}
		}
		%gcinfo
	}

	fn bench-count {
		have_srcdata
		%count $srcdata
		%gcinfo
	}

	fn bench-add {
		have_srcdata
		local(res = 0){
			for (i = $srcdata){
				res = <={add $res $i}
			}
		}
		%gcinfo
	}

	fn bench-strings {
		have_srcdata
		local(tmp=){
			tmp = <={%string $srcdata}
			tmp = <={%strlist $tmp}
		}
		%gcinfo
	}

	fn bench-iterate {
		have_srcdata
		for (i = $srcdata) {
			result
		}
		%gcinfo
	}

	fn bench-double_list {
		local (y=){
			y = $srcdata $srcdata
		}
		%gcinfo
	}

	fn bench-caret_srcdata {
		if {gt $#srcdata 2500 && ! $bench_run_caret_srcdata} {
			echo 'note: caret_srcdata will not run with more than 2500 elements'
			return
		}
		local (y=){
			y = $srcdata^$srcdata
		}
		%gc
		%gcinfo
	}

	fn bench-caret_lists {
		local (y=){
			y = <={%range 0 10}^$srcdata
		}
		%gcinfo
	}

	fn bench-reverse {
		local (y=){
			y = <={reverse $srcdata}
		}
		%gcinfo
	}

	fn bench-last {
		local (y=){
			y = <={%last $srcdata}
		}
		%gcinfo
	}

	fn bench-middle_elem {
		local (y=){
			y = <={%count $srcdata |> @{div $1 2} |> @{%elem $1 $srcdata}}
		}
		%gcinfo
	}

	fn bench-range {
		%range 1 $srcdata_len
		%gcinfo
	}

}

fn run-bench bench {
	time { bench-^$bench }
}

fn bench-usage {
	throw usage bench 'bench [command] [args...]'
}

fn bench args {
	if {~ $#args 0} { bench-usage }
	local (
		cmd = $args(1)
		cmdargs = $args(2 ...)
		all_benches = <={
			process <={~~ <={glob 'fn-bench-*' <=$&internals <=$&vars} fn-bench-*} (
				usage { result }
				* { result $matchexpr }
			)}
	) {
		match $cmd (
			list {
				echo $all_benches
				return <=true
			}
			setup {
				if {~ $#cmdargs 0} {
					cmdargs = 1000
				}
				echo 'bench: using '^$cmdargs^' elements as source data'
				populate_source_data $cmdargs
				return <=true
			}
			run {
				if {~ $#cmdargs 0} { bench-usage }
				process $cmdargs (
					$all_benches { result }
					* { bench-usage }
				)
				have_srcdata
				for (i = $cmdargs) {
					run-bench $i
				}
				return <=true
			}
			run-all {
				have_srcdata
				for (i = $all_benches){
					run-bench $i
				}
				return <=true
			}
			go {
				bench setup $cmdargs
				bench run-all
			}
			* { bench-usage }
		)
	}
}

fn power x y {
	if {lt $y 0} { throw error $0 '$y < 0' }
	if {eq $y 0} { return 1 }
	if {eq $y 1} { return $x}
	let (res = 1) {
		for(i = <={%range 1 $y}) {
			res = <={mul $res $x}
		}
		result $res
	}
}

fn qdbench1_work {
	benchcmd = {. benches.es ; bench go 2500 ; %gcinfo -v}
	escmd = ./es -X -S 25 -C 25 -B

	for (i = <={%range 1 4 |> iterator |> do @{power 2 $1}}) {
		echo '>>>' $escmd $i -c $benchcmd
		time $escmd $i -c $benchcmd
	}
	echo '>>> ./es -c' $benchcmd
	time ./es -c $benchcmd
}

fn qdbench1 file {
	if {~ $#file 0} {
		qdbench1_work
	} {
		echo 'piping output to' $file
		qdbench1_work >[2=1] > $file
	}
}


fn qdbench2_work {
	escmd = '%range 1 100000 |> reverse ; %gcinfo -v'

	for (i = <={%range 1 4 |> iterator |> do @{power 2 $1}}) {
		cmd = ./es -X -B $i -c $escmd
		echo '>>> running:' $cmd
		time $cmd
	}

	cmd = ./es -c $escmd
	echo '>>> running:' $cmd
	time $cmd
}

fn qdbench2 file {
	if {~ $#file 0} {
		qdbench2_work
	} {
		echo 'piping output to' $file
		qdbench2_work >[2=1] > $file
	}
}

