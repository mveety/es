#!/usr/bin/env -S es -N

assert {gte $#* 1}

libsrc = <={if {~ $#* 1} {
				let(al=$:1) {
					if {~ $al($#al) '/'} {
						result $"al(1 ... `{sub $#al 1})
					} {
						result $1
					}
				}
			} {
				result `{pwd}^'/libraries'
			}
		}

libs = <={let (r=;dl=;f=) {
			for(i = $libsrc/*.es){
				dl = <={%fsplit '/' $i}
				f = $dl($#dl)
				r = $r $f
			}
			result $r
		}
	}

libdir = <={if {! ~ $#2 0} {
				let(al=$:2) {
					if {~ $al($#al) '/'} {
						result $"al(1 ... `{sub $#al 1})
					} {
						result $2
					}
				}
			} {
				result $HOME/eslib
			}
		}

if {! ~ $#debugging 0 } {
	echo 'note: debugging enabled. file operations are neutered'
}

fn copyfile src dest {
	if {~ $#debugging 0} {
		cp $src $dest
	}
}

fn makedir {
	if {~ $#debugging 0} {
		mkdir $*
	}
}

let (a = <={access -1 -r $libdir}) {
	if {~ $#a 0} {
		echo 'making '^$libdir^'...'
		makedir -p $libdir
	}
}

for (i = $libs) {
	let (
			t = <={access -n $i -1 -r $libdir}
			srcfile = $libsrc/^$i
	) {
		if {~ $#t 0 } {
			echo 'installing '^$i^' -> '^$libdir^'/'^$i
			copyfile $srcfile $libdir
		} {
			let (
				curfile_md5 = `{md5sum $t}
				srcfile_md5 = `{md5sum $srcfile}
			) {
				if {! ~ $curfile_md5 $srcfile_md5} {
					echo 'copying '^$t^' -> '^$t^'.old'
					copyfile $t $t^'.old'
					echo 'installing '^$i^' -> '^$libdir^'/'^$i
					copyfile $srcfile $libdir
				}
			}
		}
	}
}
echo 'done.'

