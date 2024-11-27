#!/usr/bin/env es

libs = dirstack.es show.es lc.es history.es string.es
libdir = $HOME/eslib
libsrc = 'libraries/'

let (a = <={access -1 -r $libdir}) {
	if {~ $#a 0} {
		echo 'making '^$libdir^'...'
		mkdir -p $libdir
	}
}

for (i = $libs) {
	let (
			t = <={access -n $i -1 -r $libdir}
			srcfile = $libsrc^$i
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
