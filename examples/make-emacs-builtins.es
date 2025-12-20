#!/usr/bin/env es

import format syntax

esCmdlines = ()
esBuiltins = ()
keywords = $_es_syntax_defs(keywords)
res = '\\<\\('
dosep = false

process ``(\n){cat es.vim} (
	*^'esCmd'^* {
		if {! ~ $matchexpr *^'Statement'^*} {
			esCmdlines += $matchexpr
		}
	}
	* { result }
)

for (l = $esCmdlines) {
	for (entry = <={%split ' ' $l}) {
		if {~ $entry 'syn' 'keyword' 'esCmd'} { continue }
		if {~ $entry $keywords} { continue }
		esBuiltins += $entry
	}
}

for (b = $esBuiltins) {
	if {$dosep} {
		res = $res^'\\|'
	} {
		dosep = true
	}
	res = $res^$b
}
res = $res^'\\)\\>'
echo $res

