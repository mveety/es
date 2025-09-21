#!/bin/sh

PLATFORM=$(uname)

# clean things up
if test -f Makefile
then
	make distclean
fi

# do autoconf stuff
./setup-autoconf.sh

# run configure
case "$PLATFORM" in
	FreeBSD)
		CC=clang ./configure --enable-modules
		;;
	Linux)
		CC=gcc ./configure --prefix=/usr --enable-modules
		;;
	*)
		echo "warning: es has not been tested on your platform"
		# just let configure work out the best defaults
		./configure
		;;
esac

make all

