library complete_import (init)

fn complete_import_remove_ext ext files {
	result <={ process $files (
		* { result <={~~ $matchexpr *^$ext} }
	)}
}

fn complete_import_hook curline partial {
	let (raw_files=;) {
		raw_files = <={ es_complete_run_glob '$libraries/'^$partial |>
					complete_import_remove_ext '.es'
				} <={ es_complete_run_glob '$corelib/'^$partial |>
					complete_import_remove_ext '.es'
				}
		let (res=) {
			for (i = <={es_complete_only_names $raw_files}) {
				if {! ~ $i $res} {
					res = $res $i
				}
			}
			result $res
		}
	}
}

%complete_cmd_hook import @ curline partial {
	result <={complete_import_hook $curline $partial}
}

