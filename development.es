#!/usr/bin/env es

CLANGS = (clang22 clang21 clang)
GCCS = (gcc)
VALIDCC = $CLANGS $GCCS

fn exists name {
	{ access -n $name -1e -xf $path; result <=true } onerror { result <=false}
}

fn get-cc {
	for (cc = $VALIDCC) {
		if {exists $cc} { return $cc }
	}
	throw error development 'es needs one of '^$^VALIDCC
}

local(
	CC = ''
	cmd = (./configure --enable-modules --enable-development)
	enable-addrsan = false
	enable-bounds-safety = false
	enable-automated-crashing = false
	show-full-help = false
	platform = <={%split ' ' $buildstring |> %elem 5}
)  {

	parseargs @ arg {
		match $arg (
			(*) { usage }
			(-A) { enable-addrsan = true }
			(-B) { enable-bounds-safety = true }
			(-X) { enable-automated-crashing = true }
			(-G) { VALIDCC = $GCCS }
			(-C) { VALIDCC = $CLANGS }
			(-h) { show-full-help = true; usage }
		)
	} @ {
		echo >[1=2] 'usage: ./development.es [-ABCGhX]'
		if { $show-full-help } {
			echo >[1=2] '    -A -- Enable address sanitizer (requires clang)'
			echo >[1=2] '    -B -- Enable bounds safety (requires clang)'
			echo >[1=2] '    -C -- force use of clang'
			echo >[1=2] '    -G -- force use of gcc'
			echo >[1=2] '    -h -- show this message'
			echo >[1=2] '    -X -- enable ''automated crashing'''
			exit 0
		}
		exit 1
	} $*

	CC = <=get-cc
	try make distclean
	match $platform (
		(*) {
			enable-addrsan = false
			enable-bounds-safety = false
		}
		('FreeBSD') {
			if {~ $CC clang* && ! ~ $CC clang } {
				if { $enable-addrsan } { cmd += --enable-addrsan }
				if { $enable-bounds-safety } { cmd += --enable-bounds-safety }
			} {
				enable-addrsan = false
				enable-bounds-safety = false
			}
		}
		('Linux') {
			if {~ $CC clang*} {
				if { $enable-addrsan } { cmd += --enable-addrsan }
				if { $enable-bounds-safety } { cmd += --enable-bounds-safety }
			} {
				enable-addrsan = false
				enable-bounds-safety = false
			}
		}
	)
	if { $enable-automated-crashing } { cmd += --enable-automated-crashing }
	echo >[1=2] 'running: '^$^cmd
	$cmd
	if { $enable-automated-crashing } { echo >[1=2] 'note: building with automated crashing' }
	if { $enable-addrsan } { echo >[1=2] 'note: building with addrsan' }
	if { $enable-bounds-safety } { echo >[1=2] 'note: building with bounds safety' }
}

