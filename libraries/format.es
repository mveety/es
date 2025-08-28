#!/usr/bin/env

library format (types)

if {~ $#__es_formatters 0 || ! ~ <={$&termtypeof $__es_formatters} dict} {
	__es_formatters = <=dictnew
}

fn is_format_installed t {
	let (res = false) {
		dictforall $__es_formatters @ n _ {
			if {~ $n $t} { res = true; break}
		}
		return <=$res
	}
}

fn uninstall_format t {
	__es_formatters = <={dictremove $__es_formatters $t}
}

fn install_format name fmt {
	__es_formatters = <={dictput $__es_formatters $name @ v {
		$fmt $v
	}}
}

fn format v {
	local(
		vartype = <={typeof $v}
		fmtfn =
	){
		assert2 $0 {! ~ $vartype 'unknown'}
		fmtfn = <={dictget $__es_formatters $vartype onerror {
			throw error format 'formatter for '^$^vartype^' is not installed'
		}}
		$fmtfn $v
	}
}

fn formatters {
	dictnames $__es_formatters
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
	dictiter $v |>
		do @ n v { result <={format $n}^' => '^<={format $v} } |>
		%flatten ', ' |>
		@{ result '['^$1^']' } |>
		result
}

fn fmt-prim-error v {
	local (err=;type=;msg=){
		(err type msg) = <={$v info}
		result 'error(err = '^<={format $err}^', type = '^<={format $type}^', msg = '^<={format $msg}^')'
	}
}

fn fmt-prim-box v {
	local (boxdata = <=$v) {
		result '<'^<={format $boxdata}^'>'
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

