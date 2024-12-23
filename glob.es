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

fn isextendedglob glob {
	local(state=0) {
		process $:glob (
			('\') {
				if {~ $state 0} {
					state = 1
				} {
					state = 0
				}
			}
			('(') {
				if {~ $state 0} {
					return <=true
				} {
					state = 0
				}
			}
			* {
				state = 0
			}
		)
	}
	result <=false
}

fn esmglob_expandquestions xglob {
	local (
		xlglob = $:xglob
		state = default # can also be escape and count
		tmp = ()
		res = ()
	){
		for(i = $:xglob) {
			match $state (
				(default) {
					match $i (
						('\') {
							state = escape
							res = $res '\'
						}
						('?') {
							tmp = ()
							state = count
						}
						* {
							res = $res $i
						}
					)
				}
				(escape) {
					res = $res $i
					state = default
				}
				(count) {
					if {~ $i [1234567890]} {
						tmp = $tmp $i
					} {
						if {~ $#tmp 0} {
							res = $res '?'
						} {
							tmp = $"tmp
							while {gt $tmp 0} {
								res = $res '?'
								tmp = <={sub $tmp 1}
							}
							tmp = ()
						}
						state = default
						res = $res $i
					}
				}
				* {
					throw error assert $0^': unknown state '^$^state
				}
			)
		}
		if {~ $state count} {
			if {~ $#tmp 0} {
				res = $res '?'
			} {
				tmp = $"tmp
				while {gt $tmp 0} {
					res = $res '?'
					tmp = <={sub $tmp 1}
				}
				tmp = ()
			}
		}

		result $"res
	}
}

fn esmglob_alllen1 strs {
	for(i = $strs) {
		if {! ~ <={%count $:i} 1} {
			return <=false
		}
	}
	return <=true
}

fn esmglob_partialcompilation xglob {
	local (
		xlglob = $:xglob
		xlglobsz=0
		i = 1
		tmp = ()
		frontl = ()
		front = ''
		mid = ()
		back = ''
		escaped = false
		inparens = 0
	) {
		xlglobsz = $#xlglob
		while {lte $i $xlglobsz} {
			if {~ $xlglob($i) '\' && ! $escaped} {
				escaped = true
			} {
				if {~ $xlglob($i) '(' && ! $escaped} {
					escaped = false
					break
				}
				if {~ $xlglob($i) '(' || ~ $xlglob($i) '\' && $escaped} {
					frontl = $frontl '\'
				}
				frontl = $frontl $xlglob($i)
				if {$escaped} { escaped = false }
			}
				i = <={add $i 1}
		}
		if {! lte $i $xlglobsz} {
			return $"frontl
		}
		front = $"frontl
		# we're starting on the pattern here
		if {~ $xlglob($i) '('} {
			i = <={add $i 1}
			assert2 $0 {lt $i $xlglobsz}
			while {lte $i $xlglobsz} {
				match $xlglob($i) (
					('\') {
						if {$escaped} {
							escaped = false
							tmp = $tmp '\'
							tmp = $tmp $matchexpr
						} {
							escaped = true
						}
					}
					('|') {
						if {$escaped} {
							escaped = false
							tmp = $tmp $matchexpr
						} {
							mid = $mid $"tmp
							tmp = ()
						}
					}
					(')') {
						if {$escaped} {
							escaped = false
							tmp = $tmp $matchexpr
						} {
							if {! ~ $#mid 0} {
								mid = $mid $"tmp
								tmp = ()
							}
							i = <={add $i 1}
							break
						}
					}
					('(') {
						if {$escaped} {
							escaped = false
							tmp = $tmp '\'
							tmp = $tmp $matchexpr
						} {
							i = <={add $i 1}
							tmp = $tmp '('
							while {lte $i $xlglobsz} {
								tmp = $tmp $xlglob($i)
								if {~ $xlglob($i) '('} {
									inparens = <={add $inparens 1}
								}
								if {~ $xlglob($i) ')'} {
									if {gt $inparens 0} {
										inparens = <={sub $inparens 1}
									} {
										break
									}
								}
								assert2 $0 {gte $inparens 0}
								i = <={add $i 1}
							}
						}
					}
					('*') {
						if {$escaped} {
							escaped = false
							tmp = $tmp $matchexpr
						} {
							if {~ $#tmp 0 && ~ $xlglob(<={add $i 1}) '|'} {
								while {lte $i $xlglobsz} {
									if {~ $xlglob($i) ')'} {
										break
									}
									i = <={add $i 1}
								}
								i = <={add $i 1}
								if {~ $#frontl 0} {
									mid = '*'
									break
								}
								if {~ $frontl($#frontl) '*'} {
									mid = ''
									break
								}
								mid = '*'
								break
							} {
								tmp = $tmp $matchexpr
							}
						}
					}
					* {
						escaped = false
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
		if {esmglob_alllen1 $mid} {
			mid = <={%string '[' $mid ']'}
			if {~ $mid *^'*'^*} {
				mid = '*'
			}
		}
		result $front^$mid^$back
	}
}

fn esmglob_double_wild_removal xglob {
	local (
		state = 0
		resl = ()
	) {
		for(i = $:xglob) {
			match $state (
				(0) {
					if {~ $i '*'} {
						state = 1
					} {
						resl = $resl $i
					}
				}
				(1) {
					if {! ~ $i '*'} {
						resl = $resl '*' $i
						state = 0
					}
				}
				* {
					assert2 $0 {~ $state 0 || ~ $state 1}
				}
			)
		}
		if {~ $state 1} {
			resl = $resl '*'
		}
		result $"resl
	}
}

fn esmglob_parse_escapes escape_slash xglob {
	local(res=;state = 0){
		process $:xglob (
			('\') {
				if {~ $state 0} {
					state = 1
				} {
					state = 0
					if {$escape_slash} {
						res = $res $matchexpr
					}
					res = $res $matchexpr
				}
			}
			* {
				res = $res $matchexpr
				state = 0
			}
		)
		result $"res
	}
}

fn esmglob_add_unique elem list {
	if {~ $elem $list } {
		result $list
	} {
		result $list $elem
	}
}

fn esmglob_compile0 xglob {
	if {! isextendedglob $xglob} {
		return <={esmglob_double_wild_removal $xglob}
	}
	local (partcomp=;res=();tmp=) {
		for(i = <={esmglob_partialcompilation <={esmglob_double_wild_removal $xglob}}) {
			tmp = <={esmglob_compile0 $i}
			for(j = $tmp) {
				res = <={esmglob_add_unique $j $res}
			}
		}
		result $res
	}
}

fn esmglob_compile xglob {
	local(
		res = <={esmglob_compile0 <={esmglob_expandquestions $xglob}}
	) {
		process $res (
			* {
				result <={esmglob_parse_escapes true $matchexpr}
			}
		)
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
			if {! ~ $i *^'*'^* && ! ~ $i *^'?'^*} {
				res = $res $i
			}
		}
		result $res
	}
}

fn esmglob_compmatch elem cglobs {
	for(i = $cglobs) {
		if{eval '{~ '^$elem^' '^$i^'}'} {
			return <=true
		}
	}
	result <=false
}


fn esmglob_match elem xglob {
	result <={esmglob_compmatch $elem <={esmglob_compile $xglob}}
}

fn esmgm elem xglob_or_cglobs {
	if {~ $#xglob_or_cglobs 1} {
		esmglob_match $elem $xglob_or_cglobs
	} {
		esmglob_compmatch $elem $xglob_or_cglobs
	}
}

