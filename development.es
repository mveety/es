#!/usr/bin/env es

VALIDCC = (clang22 clang21 clang gcc)

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
	CC = <=get-cc
	cmd = (./configure --enable-modules --enable-development)
	enable-addrsan = false
	enable-bounds-safety = false
	enable-automated-crashing = false
)  {

	parseargs @ arg {
		match $arg (
			(-A) { enable-addrsan = true }
			(-B) { enable-bounds-safety = true }
			(-X) { enable-automated-crashing = true }
			(-h) { usage }
		)
	} @ {
		echo >[1=2] 'usage: ./development.es [-ABX]'
		exit 1
	} $*

	try make distclean
	if {~ $CC clang* && ! ~ $CC clang } {
		if { $enable-addrsan } { cmd += --enable-addrsan }
		if { $enable-bounds-safety } { cmd += --enable-bounds-safety }
	}
	if { $enable-automated-crashing } { cmd += --enable-automated-crashing }
	echo >[1=2] 'running: '^$^cmd
	$cmd
	if { $enable-automated-crashing } { echo >[1=2] 'note: building with automated crashing' }
	if { $enable-addrsan } { echo >[1=2] 'note: building with addrsan' }
	if { $enable-bounds-safety } { echo >[1=2] 'note: building with bounds safety' }
}

