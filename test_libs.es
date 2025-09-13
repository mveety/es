#!/usr/bin/env es

libraries = `{pwd}^/libraries $libraries
if {conf -X -p es dynlibs} {
	conf -p es -s dynlib-local-search true
}

