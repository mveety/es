#!/usr/bin/env es

fn glob patternstr list {
	if {~ $#list 0 } {
		result <={eval 'result '^$^patternstr}
	} {
		local (res=) {
			for(i = $list) {
				if {eval '~ '^$i^' '^$^patternstr} {
					res = $res $i
				}
			}
			result $res
		}
	}
}

fn glob_extract patternstr elem {
	if {~ $#elem 0} {
		throw error $0 'list missing'
	}
	result <={eval '~~ '^$elem^' '^$^patternstr}
}

fn isextendedglob glob {
	process $:glob (
		('(') { return <=true }
	)
	result <=false
}

fn esmglob_partialcompilation xglob {
	local (
		xlglob = $:xglob
		xlglobsz=
		i = 1
		j = 1
		c=
		tmp=
		front=''
		mid=
		back=''
	) {
		xlglobsz = $#xlglob
		while {lte $i $xlglobsz} {
			if {~ $xlglob($i) '('} {
				break
			}
			front = $front $xlglob($i)
			i = <={add $i 1}
		}
		if {! lte $i $xlglobsz} {
			return $"front
		}
		front = $"front
		# we're starting on the pattern here
		if {~ $xlglob($i) '('} {
			i = <={add $i 1}
			assert2 $0 {lt $i $xlglobsz}
			while {lte $i $xlglobsz} {
				match $xlglob($i) (
					('|') {
						mid = $mid $"tmp
						tmp = ()
					}
					(')') {
						if {! ~ $#tmp 0} {
							mid = $mid $"tmp
							tmp = ()
						}
						i = <={add $i 1}
						break
					}
					('(') {
						throw error $0 'invalid ( at pos '^$i
					}
					* {
						tmp = $tmp $matchexpr
					}
				)
				i = <={add $i 1}
			}
			assert2 $0 {~ $#tmp 0}
		}

		if {lte $i $xlglobsz} {
			back = <={%string $xlglob($i ...)}
		}
		result $front^$mid^$back
	}
}

fn esmglob_compile xglob {
	if {! isextendedglob $xglob} {
		return $xglob
	}
	local (partcomp=;res=()) {
		for(i = <={esmglob_partialcompilation $xglob}) {
			res = $res <={esmglob_compile $i}
		}
		result $res
	}
}

fn esmglob_add_unique elem list {
	if {~ $elem $list } {
		result $list
	} {
		result $list $elem
	}
}

fn esmglob_do_glob globfn xglob list {
	local (
		cglobs = <={esmglob_compile $xglob}
		inter = ()
		res=
	) {
		for(glob = $cglobs) {
			inter = <={$globfn $glob $list}
			for(i = $inter) {
				res = <={esmglob_add_unique $i $res}
			}
		}
		result $res
	}
}

fn esmglob xglob list {
	local(tmp=;res=){
		tmp = <={esmglob_do_glob $fn-glob $xglob $list}
		for(i = $tmp) {
			if {! ~ $i *^'*'^*} {
				res = $res $i
			}
		}
		result $res
	}
}

fn esmglob_extract xglob elem {
	esmglob_do_glob $fn-glob_extract $xglob $elem
}

