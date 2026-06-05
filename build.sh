#!/bin/sh

PLATFORM=$(uname)

# clean things up
if test -f Makefile
then
	case "$PLATFORM" in
		SunOS)
			gmake distclean
			;;
		*)
			make distclean
			;;
	esac
fi

# do autoconf stuff
./setup-autoconf.sh

# run configure
case "$PLATFORM" in
	FreeBSD)
		CC=clang ./configure --enable-modules
		make -j $(nproc) $* all
		;;
	Linux)
		CC=gcc ./configure --prefix=/usr --enable-modules
		make -j $(nproc) $* all
		;;
	Haiku)
		./configure --prefix=/boot/home/config/non-packages --enable-modules
		make -j $(nproc) $* all
		;;
	SunOS)
		./configure --prefix=/usr --enable-modules
		gmake -j $(nproc) $* all
		;;
	NetBSD)
		CC=clang ./configure --prefix=/usr/pkg --enable-modules
		make $* all
		;;
	*)
		echo "warning: es has not been tested on your platform and won't be automatically built"
		echo "run 'make all' or 'gmake all' to finish if configure succeeds."
		# just let configure work out the best defaults
		./configure
		;;
esac

