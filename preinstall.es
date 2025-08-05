#!/usr/bin/env es

if {access -x $1/es} {
	echo cp $1/es $1/es.old
	cp $1/es $1/es.old
}

