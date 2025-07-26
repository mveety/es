library input_glob (init)

fn input_glob_escape_string string {
	{process $:string (
		'''' { result '''''' }
		* { result $matchexpr }
	)} |> %string |> result
}

fn input-glob glob {
	catch @ e t m {
		if {~ $e error && ~ $t glob_eof} {
			return <=true
		}
		throw $e $t $m
	} {
		let (line=){
			forever {
				line = <=%read
				if {~ $#line 0} { throw error glob_eof }
				if {eval '~ '''^<={input_glob_escape_string $line}^''' '^$glob} {
					echo $line
				}
			}
		}
	}
}

fn input-esmglob xglob {
	let (line=;cglobs=) {
		cglobs = <={esmglob_compile $xglob}
		catch @ e t m {
			if {~ $e error && ~ $t glob_eof} {
				return <=true
			}
			throw $e $t $m
		} {
			forever {
				line = <=%read
				if {~ $#line 0} { throw error glob_eof }
				if {%esm~ ''''^<={input_glob_escape_string $line}^'''' $cglobs} {
					echo $line
				}
			}
		}
	}
}
			

