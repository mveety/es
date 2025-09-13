#!/usr/bin/env -S es -N

debugging = false
if {~ $*(1) -d} {
	debugging=true
	* = $*(2 ...)
}

assert {gte $#* 1}

need_dynlibs = %dict(
	float.es => mod_float
)

libsrc = <={if {~ $#* 1} {
				let(al=$:1) {
					if {~ $al($#al) '/'} {
						result $"al(1 ... <={sub $#al 1})
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
				if {! ~ $f $ignore} { r = $r $f }
			}
			result $r
		}
	}

libdir = <={if {! ~ $#2 0} {
				let(al=$:2) {
					if {~ $al($#al) '/'} {
						result $"al(1 ... <={sub $#al 1})
					} {
						result $2
					}
				}
			} {
				result $default_libraries
			}
		}

dynlibsrc = <={
	if {! ~ $#3 0} {
		let (al=$:3) {
			if {~ $al($#al) '/'} {
				result $"al(1 ... <={sub $#al 1})
			} {
				result $3
			}
		}
	} {
		result `{pwd}
	}
}

if {$debugging} {
	echo 'note: debugging enabled. file operations are neutered'
}

fn copyfile src dest {
	if {! $debugging} {
		cp $src $dest
	}
}

fn makedir {
	if {! $debugging} {
		if{! mkdir $* } {
			throw error 'libinstall.es' 'mkdir '^$^*^' failed'
		}
	}
}

let (a = <={access -1 -d $libdir}) {
	if {~ $#a 0} {
		echo 'making '^$libdir^'...'
		makedir -p $libdir
	}
}

if {! access -w $libdir} {
	throw error 'libinstall.es' 'unable to write to '^$libdir
}

if {$debugging} {
	echo 'libsrc = '^$libsrc
	echo 'libdir = '^$libdir
	echo 'libraries =' $libs
	if {$es_conf_dynlibs} {
		echo 'dynlibsrc = '^$dynlibsrc
	}
}

fn copy_dynlib lib libdir {
	if {! dicthaskey $need_dynlibs $lib} {
		return <=true
	}
	if {! $es_conf_dynlibs} { return <=false }
	local(
		platform = <={%split ' ' $buildstring |> %elem 5}
		arch = <={%split ' ' $buildstring |> %elem 6}
		dynlibfile=
		ext = <={
			if {$es_conf_dynlibs} {
				result $es_conf_dynlib-extension
			} {
				result ''
			}
		}
	) {
		%dict($lib => dynlibfile) = $need_dynlibs
		if {! access -r $dynlibsrc/$dynlibfile.$ext} {
			return <=false
		}
		if {access -rw $libdir/$dynlibfile.$ext} {
			let (
				srcfile_md5 = `{md5sum $dynlibsrc/$dynlibfile.$ext}
				curfile_md5 = `{md5sum $libdir/$dynlibfile.$platform.$arch.$ext}
			) {
				if {! ~ $srcfile_md5 $curfile_md5} {
					echo 'installing' $dynlibfile.$ext '->' $libdir^'/'^$dynlibfile^'.'^$platform^'.'^$arch^'.'^$ext
					copyfile $dynlibsrc/$dynlibfile.$ext $libdir/$dynlibfile.$platform.$arch.$ext
				}
			}
		} {
			echo 'installing' $dynlibfile.$ext '->' $libdir^'/'^$dynlibfile^'.'^$platform^'.'^$arch^'.'^$ext
			copyfile $dynlibsrc/$dynlibfile.$ext $libdir/$dynlibfile.$platform.$arch.$ext
		}
		return <=true
	}
}

for (i = $libs) {
	let (
			t = <={access -n $i -1 -w $libdir}
			srcfile = $libsrc/^$i
	) {
		if {~ $#t 0 } {
			if {copy_dynlib $i $libdir} {
				echo 'installing '^$i^' -> '^$libdir^'/'^$i
				copyfile $srcfile $libdir
			}
		} {
			let (
				curfile_md5 = `{md5sum $t}
				srcfile_md5 = `{md5sum $srcfile}
			) {
				if{copy_dynlib $i $libdir} {
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
}
echo 'done.'

