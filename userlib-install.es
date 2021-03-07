#!/usr/local/bin/es

libs = dirstack.es show.es lc.es
libdir = $HOME/eslib

let (a = <={access -1 -r $libdir}) {
	if {~ $#a 0} {
		echo 'making '^$libdir^'...'
		mkdir -p $libdir
	}
}

for(i = $lib) {
	let (t = <={access -n $libdir -1 -r $i}) {
		if {! $#t 0 } {
			echo 'installing '^$i^'...'
			cp $i $libdir
		}
	}
}
echo 'done.'
