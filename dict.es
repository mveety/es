
fn-dictnew = $&dictnew
fn-dictget = $&dictget
fn-%dictput = $&dictput_nocopy
fn-dictput = $&dictput
fn-%dictremove = $&dictremove_nocopy
fn-dictremove = $&dictremove
fn-dictsize = $&dictsize
fn-dictforall = $&dictforall
fn-dictcopy = $&dictcopy

fn dictkeys dict {
	let (names=) {
		dictforall $dict @ n v {
			names = $n $names
		}
		reverse $names
	}
}

fn-dictnames = $fn-dictkeys

fn dictvalues dict {
	let (values=) {
		dictforall $dict @ n v {
			values = $v $values
		}
		reverse $values
	}
}

fn sorted_dictnames dict {
	let (names=) {
		dictforall $dict @ n v {
			names = $n $names
		}
		sortlist $names
	}
}

fn sorted_dictvalues dict {
	let (values=) {
		for (name = <={sorted_dictnames $dict}) {
			values += $dict($name)
		}
		result $values
	}
}

fn dictdump_string dict {
	lets (
		fn-dictdump_statements = @ varname dict {
			let (stmts=) {
				dictforall $dict @ n v {
					let (vfmt = '(') {
						for (i = $v) { vfmt = $vfmt ''''^$i^'''' }
						vfmt = $vfmt ')'
						stmts = $stmts $varname^' := '^$n^' => '^$^vfmt
					}
				}
				result $stmts
			}
		}
		vsym = <={gensym __es_dictdump}
		stmts = <={dictdump_statements $vsym $dict |> %flatten ' ; '}
	) {
		result '@{let('^$vsym^'=<=dictnew) {'^$stmts^' ; result $'^$vsym^'}}'
	}
}

fn dictdump dict {
	dictdump_string $dict |> %parsestring
}

fn dictiter dict {
	let (names = <={dictnames $dict}) {
		result @{
			local (
				curname = $names(1)
				res = $dict($names(1))
			){
				names = $names(2 ...)
				result $curname $res
			}
		}
	}
}

fn dicthaskey dict key {
	_ = $dict($key) onerror return <=false
	return <=true
}

fn %dictstats {
	let (
		(maxsize totalsize ndicts nputs nlookups
			failed_lookups totalcompares
			bloomfail hashfail strcmpfail) = <=$&dictstats
		resdict = <=dictnew
	) {
		resdict := maxsize => $maxsize
		resdict := totalsize => $totalsize
		if {gt $ndicts 0} {
			resdict := avgsize => <={div $totalsize $ndicts}
		} {
			resdict := avgsize => 0
		}
		resdict := ndicts => $ndicts
		resdict := nputs => $nputs
		resdict := nlookups => $nlookups
		resdict := failed_lookups => $failed_lookups
		resdict := totalcompares => $totalcompares
		resdict := bloomfail => $bloomfail
		resdict := hashfail => $hashfail
		resdict := strcmpfail => $strcmpfail
		result $resdict
	}
}

