#!/usr/bin/env es

fn run_test_parser {
	assert {~ <={%parsestring '! cmd'} '{%not {cmd}}'}
	assert {~ <={%parsestring 'cmd &'} '{%bghook {cmd}}'}
	assert {~ <={%parsestring 'cmd1 ; cmd2'} '{%seq {cmd1} {cmd2}}'}
	assert {~ <={%parsestring 'cmd1 && cmd2'} '{%and {cmd1} {cmd2}}'}
	assert {~ <={%parsestring 'cmd1 || cmd2'} '{%or {cmd1} {cmd2}}'}
	assert {~ <={%parsestring 'fn name args { cmd }'} '{fn-^name=@ args{cmd}}'}
	assert {~ <={%parsestring 'fn name { cmd }'} '{fn-^name=@ * {cmd}}'}
	assert {~ <={%parsestring 'cmd < file'} '{%open 0 <={%one file} {cmd}}'}
	assert {~ <={%parsestring 'cmd > file'} '{%create 1 <={%one file} {cmd}}'}
	assert {~ <={%parsestring 'cmd >[6] file'} '{%create 6 <={%one file} {cmd}}'}
	assert {~ <={%parsestring 'cmd >> file'} '{%append 1 <={%one file} {cmd}}'}
	assert {~ <={%parsestring 'cmd <> file'} '{%open-write 0 <={%one file} {cmd}}'}
	assert {~ <={%parsestring 'cmd <>> file'} '{%open-append 0 <={%one file} {cmd}}'}
	assert {~ <={%parsestring 'cmd >< file'} '{%open-create 1 <={%one file} {cmd}}'}
	assert {~ <={%parsestring 'cmd >>< file'} '{%open-append 1 <={%one file} {cmd}}'}
	assert {~ <={%parsestring 'cmd >[7=]'} '{%close 7 {cmd}}'}
	assert {~ <={%parsestring 'cmd >[8=9]'} '{%dup 8 9 {cmd}}'}
	assert {~ <={%parsestring 'cmd <<< string'} '{%here 0 string {cmd}}'}
	assert {~ <={%parsestring 'cmd1 | cmd2'} '{%pipe {cmd1} 1 0 {cmd2}}'}
	assert {~ <={%parsestring 'cmd1 |[10=11] cmd2'} '{%pipe {cmd1} 10 11 {cmd2}}'}
	assert {~ <={%parsestring '$#var'} '{<={%count $var}}'}
	assert {~ <={%parsestring '$^var'} '{<={%flatten '' '' $var}}'}
	assert {~ <={%parsestring '`{cmd args}'} '{<={%backquote <={%flatten '''' $ifs} {cmd args}}}'}
	assert {~ <={%parsestring '``ifs {cmd args}'} '{<={%backquote <={%flatten '''' ifs} {cmd args}}}'}
	assert {~ <={%parsestring 'a || b | c'} '{%or {a} {%pipe {b} 1 0 {c}}}'}
	assert {~ <={%parsestring 'a | b || c'} '{%or {%pipe {a} 1 0 {b}} {c}}'}
	assert {~ <={%parsestring '!a && b'} '{%and {%not {a}} {b}}'}
	assert {~ <={%parsestring '!a || b'} '{%or {%not {a}} {b}}'}
	assert {~ <={%parsestring '!a & b'} '{%seq {%bghook {%not {a}}} {b}}'}
	assert {~ <={%parsestring '!a | b'} '{%not {%pipe {a} 1 0 {b}}}'}
	assert {~ <={%parsestring 'let (a=b) x && y'} '{let(a=b)%and {x} {y}}'}
	assert {~ <={%parsestring 'let (a=b) x || y'} '{let(a=b)%or {x} {y}}'}
	assert {~ <={%parsestring 'let (a=b) x & y'} '{%seq {%bghook {let(a=b)x}} {y}}'}
	assert {~ <={%parsestring 'let (a=b) x | y'} '{let(a=b)%pipe {x} 1 0 {y}}'}
	assert {~ <={%parsestring 'a && b > c'} '{%and {a} {%create 1 <={%one c} {b}}}'}
	assert {~ <={%parsestring 'a | b > c'} '{%pipe {a} 1 0 {%create 1 <={%one c} {b}}}'}
	assert {~ <={%parsestring 'let (a=b) c > d'} '{let(a=b)%create 1 <={%one d} {c}}'}
	assert {~ <={%parsestring 'cmd1 >{ cmd2 }'} '{%writeto _devfd'^*^' {cmd2} {cmd1 $_devfd'^*^'}}'}
	assert {~ <={%parsestring 'cmd1 <{ cmd2 }'} '{%readfrom _devfd'^*^' {cmd2} {cmd1 $_devfd'^*^'}}'}
	assert {~ <={%parsestring '%dict(a=>b;c=>d;e=>f)'} '{%dict(a => b; c => d; e => f)}'}
	assert {~ <={%parsestring 'a 1 2 |> b 3 4 |> c'} '{c <={b 3 4 <={a 1 2}}}'}
	assert {~ <={%parsestring 'x := 1 => 2'} '{x=<={%dictput $x ''1'' 2}}'}
	assert {~ <={%parsestring '$:x'} '{<={%strlist $x}}'}
	assert {~ <={%parsestring '$"x'} '{<={%string $x}}'}
	assert {~ <={%parsestring '(x y) = <={ohcrap &?}'} '{(x y)=<={try {ohcrap}}}'}
	assert {~ <={%parsestring 'throw error onerror echo oopsie'} '{%onerror {throw error} {echo oopsie}}'}
	assert {~ <={%parsestring 'x += 1 2 3 4'} '{x=$x 1 2 3 4}'}
	assert {~ <={%parsestring '%re(''h.*'')'} '{%re(''h.*'')}'}
	assert {~ <={%parsestring '<-{test}'} '{<={%stbackquote <={%flatten '''' $ifs} {test}}}'}
	assert {~ <={%parsestring 'match $a (1 { cmd1 } ; (2 3) { cmd2 } ; (4 5 6) { cmd 3 } ; * { cmd 4 })'} '{let(matchexpr=$a){$&if {~ $matchexpr 1} {cmd1} {~ $matchexpr 2 3} {cmd2} {~ $matchexpr 4 5 6} {cmd 3} {cmd 4}}}'}
	assert {~ <={%parsestring 'match $a (* { hello })'} '{let(matchexpr=$a){hello}}'}
	assert {~ <={%parsestring 'matchall $a (1 { cmd1 } ; (2 3) { cmd2 } ; (4 5 6) { cmd 3 } ; * { cmd 4 })'} '{let(matchexpr=$a;resexpr=;__es_no_matches=true){%seq {resexpr=$resexpr <={$&if {~ $matchexpr 1} {%seq {__es_no_matches=false} {cmd1}} {result}}} {resexpr=$resexpr <={$&if {~ $matchexpr 2 3} {%seq {__es_no_matches=false} {cmd2}} {result}}} {resexpr=$resexpr <={$&if {~ $matchexpr 4 5 6} {%seq {__es_no_matches=false} {cmd 3}} {result}}} {$&if {$__es_no_matches} {resexpr=<={cmd 4}}} {result $resexpr}}}'}
	assert {~ <={%parsestring 'process $a (1 { cmd1 } ; (2 3) { cmd2 } ; (4 5 6) { cmd 3 } ; * { cmd 4 })'} '{let(__es_process_result=)%seq {for(matchelement=$a){__es_process_result=$__es_process_result <={let(matchexpr=$matchelement){$&if {~ $matchexpr 1} {cmd1} {~ $matchexpr 2 3} {cmd2} {~ $matchexpr 4 5 6} {cmd 3} {cmd 4}}}}} {result $__es_process_result}}'}
	assert {~ <={%parsestring 'process $a (* { hello })'} '{let(__es_process_result=)%seq {for(matchelement=$a){__es_process_result=$__es_process_result <={let(matchexpr=$matchelement){hello}}}} {result $__es_process_result}}'}
	assert {~ <={%parsestring 'a -b --c --d=e'} '{a -b ''--c'' ''--d=e''}'}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_parser
}

