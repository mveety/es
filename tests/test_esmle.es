#!/usr/bin/env es

test_esmle_disable = false
test_esmle_subprocess = true
test_esmle_subprocess_arguments = -N -Dt

fn run_test_esmle {
	let (
		test_map = <=keymap
		default_map = %dict(
			Space => '%esmle:PassKey'
			ShiftTab => '%esmle:PrevCompletion'
			ArrowUp => '%esmle:NextHistory'
			CtrlD => '%esmle:DeleteOrEOF'
			Home => '%esmle:CursorHome'
			CtrlH => '%esmle:Backspace'
			CtrlL => '%esmle:ClearScreen'
			ExtDelete => '%esmle:Delete'
			ArrowRight => '%esmle:CursorRight'
			Altf => '%esmle:CursorWordRight'
			End => '%esmle:CursorEnd'
			ArrowLeft => '%esmle:CursorLeft'
			Altb => '%esmle:CursorWordLeft'
			CtrlA => '%esmle:CursorHome'
			CtrlC => '%esmle:Break'
			CtrlE => '%esmle:CursorEnd'
			Backspace => '%esmle:Backspace'
			CtrlK => '%esmle:DeleteToEnd'
			ArrowDown => '%esmle:PrevHistory'
			CtrlU => '%esmle:DeleteToStart'
			CtrlW => '%esmle:DeleteWord'
			Tab => '%esmle:NextCompletion'
			Enter => '%esmle:EndInput'
		)
	) {
		for (keyname = <={dictnames $default_map}) {
			assert {dicthaskey $test_map $keyname}
		}
		for (keyname = <={dictnames $default_map}) {
			assert {~ $default_map($keyname) $test_map($keyname)}
		}
	}
	result <=true
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_esmle
}

