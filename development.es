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

local(CC=<=get-cc;cmd = (./configure --enable-modules --enable-development))  {
	try make distclean
	if {~ $CC clang*} {
		cmd += --enable-addrsan
		cmd += --enable-bounds-safety
	}
	$cmd
}

