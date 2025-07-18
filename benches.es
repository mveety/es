#!/usr/bin/env es

if {~ $#bench_run_caret_srcdata 0} {
	bench_run_caret_srcdata = false
}

timecmd = time

let (srcdata=;srcdata_len=;no_srcdata=true){

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
	}

	fn bench-postpend_list {
		have_srcdata
		local (y=){
			for (i = $srcdata){
				y = $y $i
			}
		}
	}

	fn bench-count {
		have_srcdata
		%count $srcdata
	}

	fn bench-add {
		have_srcdata
		local(res = 0){
			for (i = $srcdata){
				res = <={add $res $i}
			}
		}
	}

	fn bench-strings {
		have_srcdata
		local(tmp=){
			tmp = <={%string $srcdata}
			tmp = <={%strlist $tmp}
		}
	}

	fn bench-iterate {
		have_srcdata
		for (i = $srcdata) {
			result
		}
	}

	fn bench-double_list {
		local (y=){
			y = $srcdata $srcdata
		}
	}

	fn bench-caret_srcdata {
		if {gt $#srcdata 2500 && ! $bench_run_caret_srcdata} {
			echo 'note: caret_srcdata will not run with more than 2500 elements'
			return
		}
		local (y=){
			y = $srcdata^$srcdata
		}
	}

	fn bench-caret_lists {
		local (y=){
			y = <={%range 0 10}^$srcdata
		}
	}

	fn bench-reverse {
		local (y=){
			y = <={reverse $srcdata}
		}
	}

	fn bench-last {
		local (y=){
			y = <={%last $srcdata}
		}
	}

	fn bench-middle_elem {
		local (y=){
			y = <={%count $srcdata |> @{div $1 2} |> @{%elem $1 $srcdata}}
		}
	}

	fn bench-range {
		%range 1 $srcdata_len
	}

}

fn run-bench bench {
	$timecmd { bench-^$bench }
}

fn bench-usage {
	throw usage bench 'bench [command] [args...]'
}

fn bench args {
	if {~ $args(1) -q} {
		timecmd =
		args = $args(2 ...)
	}
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

