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
					throw assert $0^': unknown state '^$^state
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

fn dirlist2glob args {
	local(
		separator = false
		emptyelem = false
		first = true
		arg=;
		dirs=;
		res=;
	) {
		while {true} {
			arg = $args(1)
			if {! ~ $arg -*} {
				break
			}
			arg = $:arg
			arg = $arg(2 ...)
			process $arg (
				s { separator = true }
				e { emptyelem = true }
				* { throw error usage $0 '[-es] [dirs...]' }
			)
			args = $args(2 ...)
		}
		if {$emptyelem} {
			res = '(|'
		} {
			res = '('
		}
		for	(i = $args) {
			if {eq <={%count $:i} 0} {
				if {! $emptyelem} {
					if {$first} {
						res = $res^$i
						first = false
					} {
						res = $res^'|'^$i
					}
					emptyelem = true
				}
			} {
				if {$separator} { i = $i^'/' }
				if {$first} {
					res = $res^$i
					first = false
				} {
					res = $res^'|'^$i
				}
			}
			dirs = $i $dirs
		}
		res = $res^')'
		if {~ $#dirs 0} {
			result ''
		} {~ $#dirs 1} {
			result $dirs(1)
		} {
			result $res
		}
	}
}

fn esmglob_repeat_string e t {
	local (r='') {
		while {gt $t 0} {
			r = $r^$e
			t = <={sub $t 1}
		}
		result $r
	}
}

fn matchlist_elem elem times {
	local (
		start = true
		res = '('
		tmp = ''
	) {
		if {~ $#times 1} {
			return <={esmglob_repeat_string $elem $times}
		}
		for	(i = $times) {
			tmp = <={esmglob_repeat_string $elem $i}
			if {$start} {
				res = $res^$tmp
				start = false
			} {
				res = $res^'|'^$tmp
			}
		}
		result $res^')'
	}
}

fn esmglob_expand_qmacro xglob {
	local(
		head=;mid=;tmp=;tail=;start=;end=
		elem=;elemhead=;elemtail=
		state = default
	) {
		for(i = $:xglob) {
			match $state (
				(default) {
					match $i (
						('?') {
							state = inquestion0
							mid = $matchexpr
						}
						('%') {
							state = ingeneral0
							mid = $matchexpr
						}
						('\') {
							state = startescape
							head = $head $matchexpr
						}
						* {
							head = $head $matchexpr
						}
					)
				}
				(startescape) {
					head = $head $i
					state = default
				}
				(inquestion0) {
					match $i (
						('<') {
							state = inquestion1
							mid = $mid $matchexpr
						}
						* {
							state = default
							head = $head $mid $matchexpr
							mid=
						}
					)
				}
				(ingeneral0) {
					match $i (
						('<') {
							state = inquestion1
							mid = $mid $matchexpr
						}
						('\') {
							state = ingeneral0_escape
							mid = $mid $matchexpr
						}
						* {
							mid = $mid $matchexpr
						}
					)
				}
				(ingeneral0_escape) {
					mid = $mid $i
					state = ingeneral0
				}
				(inquestion1) {
					match $i (
						([1234567890]) {
							mid = $mid $matchexpr
						}
						('-') {
							state = inquestion2
							mid = $mid $matchexpr
						}
						* {
							state = default
							head = $head $mid $matchexpr
							mid=
						}
					)
				}
				(inquestion2) {
					match $i (
						([1234567890]) {
							mid = $mid $matchexpr
						}
						('>') {
							mid = $mid $matchexpr
							state = rest
						}
						* {
							state = default
							head = $head $mid $matchexpr
							mid=
						}
					)
				}
				(rest) {
					tail = $tail $i
				}
			)
		}
		if {~ $#mid 0} { return $"head }
		head = $"head
		mid = $"mid
		tail = $"tail
		match $mid (
			('?'^*) {
				(start end) = <={~~ $matchexpr '?<'^*^'-'^*^'>'}
				mid = <={dirlist2glob '?'^<={%range $start $end}}
			}
			('%\<<'^*) {
				(start end) = <={~~ $matchexpr '%\<<'^*^'-'^*^'>'}
				mid = <={matchlist_elem '\<' <={%range $start $end}}
			}
			('%'^*^'\<'^*) {
				(elemhead elemtail) = <={~~ $matchexpr '%'^*^'\<'^*}
				(elemtail start end) = <={~~ $elemtail *^'<'^*^'-'^*^'>'}
				elem = $elemhead^'\<'^$elemtail
				mid = <={matchlist_elem $elem <={%range $start $end}}
			}
			('%'^*) {
				(elem start end) = <={~~ $matchexpr '%'^*^'<'^*^'-'^*^'>'}
				mid = <={matchlist_elem $elem <={%range $start $end}}
			}
		)
		result $head^$mid^<={esmglob_expand_qmacro $tail}
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

fn esmglob_optimize_mid list {
	if {~ $#list 0} {
		return ''
	}
	if {~ $#list 1 && ~ <={%count $:list} 0} {
		return ''
	}
	local(single=;multiple=;res=){
		for(i = $list){
			if {~ <={%count $:i} 1} {
				single = $i $single
			} {
				multiple = $i $multiple
			}
		}
		if {~ $#single 0} {
			return $multiple
		}
		single = <={%string '[' <={reverse $single} ']'}
		single = <={if {~ $single *^'*'^*} {result '*'} {result $single}}
		result $single $multiple
	}
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
		starinparens = 0
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
							if {~ $#tmp 0 && ~ $xlglob(<={add $i 1}) '|' ')'} {
								while {lte $i $xlglobsz} {
									if {~ $xlglob($i) '('} {
										starinparens = <={add $starinparens 1}
									} {~ $xlglob($i) ')' && eq $starinparens 0} {
										break
									} {~ $xlglob($i) ')' && gt $starinparens 0} {
										starinparens = <={sub $starinparens 1}
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
		result $front^<={esmglob_optimize_mid $mid}^$back
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
		for (i = <={esmglob_double_wild_removal $xglob |> esmglob_partialcompilation}) {
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
		res = <={
			esmglob_expand_qmacro $xglob |>
				esmglob_expandquestions |>
				esmglob_compile0
		}
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

fn esmglob_compmatch elem cglobs {
	for(i = $cglobs) {
		if{eval '{~ '^$elem^' '^$i^'}'} {
			return <=true
		}
	}
	result <=false
}

## external api

fn %esmglob xglob list {
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

fn %esmglob_match elem xglob {
	esmglob_compile $xglob |> esmglob_compmatch $elem |> result
}

fn %esm~ elem xglob_or_cglobs {
	if {~ $#xglob_or_cglobs 1} {
		%esmglob_match $elem $xglob_or_cglobs
	} {
		esmglob_compmatch $elem $xglob_or_cglobs
	}
}

