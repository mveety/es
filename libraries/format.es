#!/usr/bin/env

library format (types)

fn is_format_installed t {
	for(fn = $__es_formatters) {
		if {$fn 'search' $t} {
			return <=true
		}
	}
	return <=false
}

fn uninstall_format t {
	local(fmtfuns=) {
		for(fn = $__es_formatters) {
			if {! $fn 'search' $t} {
				fmtfuns = $fmtfuns $fn
			}
		}
		__es_formatters = $fmtfuns
	}
}

fn install_format name fmt {
	local(fmtfn=){
		fmtfn = @ op v {
			assert2 format {~ $op 'search' || ~ $op 'run' || ~ $op 'name'}
			if {~ $op 'search'} {
				if {~ $v $name} {
					result <=true
				} {
					result <=false
				}
			} {~ $op 'run'} {
				result <={$fmt $v}
			} {~ $op 'name'} {
				result $name
			}
		}
		if {is_format_installed $name} {
			uninstall_format $name
		}
		__es_formatters = $__es_formatters $fmtfn
		result <=true
	}
}

fn format v {
	local(
		vartype = <={typeof $v}
		res=
	){
		assert2 $0 {! ~ $vartype 'unknown'}
		for(fmtfn = $__es_formatters) {
			if {$fmtfn 'search' $vartype} {
				return <={$fmtfn 'run' $v}
			}
		}
		throw error format 'formatter for '^$^vartype^' is not installed'
	}
}

fn _formatters1 head rest {
	assert2 $0 {! ~ $#head 0}
	if {~ $#rest 0} {
		result <={$head 'name'}
	} {
		result <={$head 'name'} <={_formatters1 $rest}
	}
}

fn formatters {
	result <={_formatters1 $__es_formatters}
}

fn fmt-number v {
	result $v
}

fn fmt-string v {
	local(res=) {
		for(c = $:v){
			if {~ $c ''''} {
				res = $res ''''''
			} {
				res = $res $c
			}
		}
		result ''''^$"res^''''
	}
}

fn fmt-fun-prim v {
	result $v
}

fn fmt-list v {
	local(fmtres=) {
		for(x = $v) {
			fmtres = $fmtres <={format $x}
		}
		result '('^$^fmtres^')'
	}
}

fn fmt-prim-dict v {
	let (reslist =; res='') {
		dictforall $v @ name val {
			reslist = $reslist '['^<={format $name}^']:'^<={format $val}
		}
		while {gt $#reslist 0} {
			if {eq $#reslist 1} {
				res = $res^$reslist(1)
			} {
				res = $res^$reslist(1)^', '
			}
			reslist = $reslist(2 ...)
		}
		result 'dict('^$res^')'
	}
}

fn fmt-prim-error v {
	local (err=;type=;msg=){
		(err type msg) = <={$v info}
		result 'error(err = '^<={format $err}^', type = '^<={format $type}^', msg = '^<={format $msg}^')'
	}
}

fn fmt-prim-box v {
	local (boxdata = <=$v) {
		result 'box['^<={format $boxdata}^']'
	}
}

install_format 'nil' @ v { result 'nil' }
install_format 'number' @ v { result <={fmt-number $v} }
install_format 'string' @ v { result <={fmt-string $v} }
install_format 'function' @ v { result <={fmt-fun-prim $v} }
install_format 'primordial' @ v { result <={fmt-fun-prim $v} }
install_format 'list' @ v { result <={fmt-list $v} }
install_format 'dict' @ v { result <={fmt-prim-dict $v} }
install_format 'error' @ v { result <={fmt-prim-error $v} }
install_format 'box' @ v { result <={fmt-prim-box $v} }

