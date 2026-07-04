#!/usr/bin/env es

import libutils

masked = (prompt.es)

for (f = libraries/*.es) {
	if {~ $f libraries/^$masked} {
		echo $f^': masked'
		continue
	}
	echo -n $f^': '
	if {~ <={~~ $f libraries/*.es} <={libutil_enumerate_file_info $f |> %elem 1}} {
		echo 'ok'
	} {
		echo 'malformed library'
	}
}

