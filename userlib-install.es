#!/usr/local/bin/es

libs = dirstack.es show.es lc.es history.es
libdir = $HOME/eslib

let (a = <={access -1 -r $libdir}) {
	if {~ $#a 0} {
		echo 'making '^$libdir^'...'
		mkdir -p $libdir
	}
}

for(i = $libs) {
	let (t = <={access -n $i -1 -r $libdir}) {
		if {~ $#t 0 } {
			echo 'installing '^$i^'...'
			cp $i $libdir
		}
	}
}
echo 'done.'
