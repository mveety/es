#!/usr/bin/env es

import json
import template

xterm_colors = <={json_readfile xterm_colors.json}

let (color = ();colorname = ();namesdict = ();fgdict = (); bgdict= ()) {
	for (i = <={%range 0 255}) {
		color = <={json_get $xterm_colors $i}
		colorname = <={json_get $color 'name' |> %json_getdata}
		namesdict += <={template '${colorname} => ${i}'}
		fgdict += <={template 'fg_${colorname} => <={color256 %dict(fg=>${i})}'}
		bgdict += <={template 'bg_${colorname} => <={color256 %dict(bg=>${i})}'}
	}
	echo 'import colors'
	echo ''
	echo '256colornames = %dict('
	for (l = $namesdict) { echo <={%string \t}^$l }
	echo ')'
	echo ''
	echo '256colors = %dict('
	for (l = $fgdict) { echo <={%string \t}^$l }
	for (l = $bgdict) { echo <={%string \t}^$l }
	echo ')'
	echo ''
	echo 'fn 256swatch cname { echo $256colors(fg_^$cname bg_^$cname) ''hello world!'' $attrib(reset) }'
	echo ''
}

