library colors (init libraries)

if {~ $options esmle} {
	fn esmle_wrap string {
		result <={%string \x01 $string \x02}
	}
}

let (
	fn escape_code strlist {
		result <={%string \x1b $strlist}
	}
) {
	colors = %dict(
		fg_black => <={escape_code '[30m'}
		fg_red => <={escape_code '[31m'}
		fg_green => <={escape_code '[32m'}
		fg_yellow => <={escape_code '[33m'}
		fg_blue => <={escape_code '[34m'}
		fg_magenta => <={escape_code '[35m'}
		fg_cyan => <={escape_code '[36m'}
		fg_white => <={escape_code '[37m'}
		fg_default => <={escape_code '[39m'}
		bg_black => <={escape_code '[40m'}
		bg_red => <={escape_code '[41m'}
		bg_green => <={escape_code '[42m'}
		bg_yellow => <={escape_code '[43m'}
		bg_blue => <={escape_code '[44m'}
		bg_magenta => <={escape_code '[45m'}
		bg_cyan => <={escape_code '[46m'}
		bg_white => <={escape_code '[47m'}
		bg_default => <={escape_code '[49m'}
	)

	attrib = %dict(
		reset => <={escape_code '[0m'}
		bold => <={escape_code '[1m'}
		dim => <={escape_code '[2m'}
		underline => <={escape_code '[4m'}
		blink => <={escape_code '[5m'}
		inverse => <={escape_code '[7m'}
	)

	fn color256 colordict {
		let (res = ()) {
			if {dicthaskey $colordict fg} {
				res = $res <={escape_code '[38;5;'^$colordict(fg)^'m'}
			}
			if {dicthaskey $colordict bg} {
				res = $res <={escape_code '[48;5;'^$colordict(bg)^'m'}
			}
			result <={%string $res}
		}
	}
}

