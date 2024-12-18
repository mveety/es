#!/usr/bin/env es

import echovar

c_files = 0
h_files = 0
other_files = 0
files = *

process $files (
	(*.c) { c_files = <={add $c_files 1} }
	(*.h) { h_files = <={add $h_files 1} }
	* { other_files = <={add $other_files 1} }
)

echovar c_files
echovar h_files
echovar other_files
echo 'total files = '^$#files

