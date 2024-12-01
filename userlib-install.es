#!/usr/bin/env -S es -N

libsrc = <={if {~ $#* 1} {
				let(al=$:1) {
					if {~ $al($#al) '/'} {
						al = $al(1 ... `{sub $#al 1})
						result $"al
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
libdir = $HOME/eslib

let (a = <={access -1 -r $libdir}) {
	if {~ $#a 0} {
		echo 'making '^$libdir^'...'
		mkdir -p $libdir
	}
}

for (i = $libs) {
	let (
			t = <={access -n $i -1 -r $libdir}
			srcfile = $libsrc/^$i
		) {
		if {~ $#t 0 } {
			echo 'installing '^$i^'...'
			cp $srcfile $libdir
		} {
			let (
				curfile_md5 = `{md5sum $t}
				srcfile_md5 = `{md5sum $srcfile}
			) {
				if {! ~ $curfile_md5 $srcfile_md5} {
					echo 'installing '^$i^'...'
					cp $t $t^'.old'
					cp $srcfile $libdir
				}
			}
		}
	}
}
echo 'done.'

