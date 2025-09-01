
fn-dictnew = $&dictnew
fn-dictget = $&dictget
fn-dictput = $&dictput
fn-dictremove = $&dictremove
fn-dictsize = $&dictsize
fn-dictforall = $&dictforall
fn-dictcopy = $&dictcopy

fn dictnames dict {
	let (names=) {
		dictforall $dict @ n v {
			names = $n $names
		}
		reverse $names
	}
}

fn dictvalues dict {
	let (values=) {
		dictforall $dict @ n v {
			values = $v $values
		}
		reverse $values
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
						# stmts = $stmts $varname^' = <={$&dictput $'^$varname^' '''^$n^''' '^$^vfmt^'}'
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

