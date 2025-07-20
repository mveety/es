#!/usr/bin/env es
library history (init libraries)

if {~ $#histmax 0} {
	histmax = 25
}
if {~ $#history-reload 0} {
	history-reload = 25
}

fn history_call_date {
	let (utc = false; cmd = date; tmp=) {
		if {~ $*(1) -u} {
			utc = true
			* = $*(2 ...)
		}
		match `{uname} (
			(Linux) {
				if {$utc} { cmd = $cmd -u }
				cmd = $cmd -d '@'^$*(1)
			}
			* {
				if {$utc} { cmd = $cmd -u }
				cmd = $cmd -r $*(1)
			}
		)
		cmd = $cmd '+%H %M'
		tmp = `{$cmd}
		result $tmp(1) $tmp(2)
	}
}

fn gettzoffset {
	let (
		current_unixtime = $unixtime
		localhours =
		localminutes =
		localseconds =
		utchours =
		utcminutes =
		utcseconds =
	) {
		(localhours localminutes) = <={history_call_date $current_unixtime}
		localseconds = <={add <={mul <={mul $localhours 60} 60} <={mul $localminutes 60}}
		(utchours utcminutes) = <={history_call_date -u $current_unixtime}
		utcseconds = <={add <={mul <={mul $utchours 60} 60} <={mul $utcminutes 60}}
		result <={sub $localseconds $utcseconds}
	}
}

fn history-filter start timeformat {
	awk -v 'n='^$start \
		-v 'timeformat='^$timeformat \
		-v 'platform='^`{uname} \
		-v 'current_unixtime='^$unixtime \
		-v 'seconds_offset='^<={gettzoffset} \
		'
		function time_format(epoch) {
			epoch += seconds_offset
			seconds = epoch % 60
			epoch -= seconds
			epoch /= 60
			minutes = epoch % 60
			epoch -= minutes
			epoch /= 60
			hours = epoch % 24

			return sprintf("%0.2d:%0.2d", hours, minutes)
		}

		function date_format(epoch) {
			epoch += seconds_offset
			JD = (epoch/86400)+2440588

			alpha = int((JD - 1867216.25)/ 36524.25)
			A = JD + 1 + alpha - int(alpha/4)
			B = A + 1524
			C = int((B - 122.1) / 365.25)
			D = int(365.25 * C)
			E = int((B - D) / 30.6001)
			day = B - D - int(30.6001 * E)

			if(E < 14) {
				month = int(E -1)
			} else {
				month = int(E - 13)
			}

			if(month > 2) {
				year = int(C - 4716)
			} else {
				year = int(C - 4715)
			}

			return sprintf("%0.4d-%0.2d-%0.2d",year, month, day)
		}

		BEGIN{
			havetime = 0
			unixtime = ""
		}

		/^#\+[0-9]+$/{
			unixtime = substr($1, 2, length($1))
			havetime = 1
		}

		!/^#\+[0-9]+$/{
			if($1 != ""){
				if(havetime == 0){
					unixtime = current_unixtime
				}
				curdate = date_format(unixtime)
				curtime = time_format(unixtime)
				if(timeformat == "datetime"){
					printf "%-6d  %s  %s  %s\n", n, curdate, curtime, $0
				} else if (timeformat == "unixtime") {
					printf "%-6d  %s  %s\n", n, unixtime, $0
				} else if (timeformat == "none") {
					printf "%-6d  %s\n", n, $0
				} else if (timeformat == "noevent") {
					printf "%s\n", $0
				} else {
					printf "%-6d  %s  %s\n", n, curtime, $0
				}
				n++
			}
			havetime = 0
		}'
}

fn fix-history file {
	let(tmpfile = $file^'.'^<={gensym tmp}){
		awk \
		'
		BEGIN{
			state = 0
		}

		/^#\+[0-9]+$/{
			if (state == 0) {
				print $0
				state = 1
			}
		}

		!/^#\+[0-9]+$/{
			if (state == 1) {
				print $0
				state = 0
			}
		}

		' $file > $tmpfile
		cp $file $file.old
		rm $file
		cp $tmpfile $file
		rm $tmpfile
	}
}

fn history {
	if {~ $#history 0 } {
		throw error history '$history not set'
	}
	let (
		maxents = $histmax
		limitevents = true
		singleevent = false
		cleanhistory = false
		lastevent = false
		histfile = $history
		timeformat = time
		yes = false
		verbose = false
		usageexit = false
		useunixtime = 0
		fixhistory = false
		analyze = false
	) {
		parseargs @ arg {
			match $arg (
				(-n) { limitevents = true; maxents = $flagarg }
				(-e) { singleevent = true; eventn = $flagarg }
				(-a) { limitevents = false; singleevent = false }
				(-l) { lastevent = true ; maxents = 1 }
				(-C) { cleanhistory = true }
				(-c) { timeformat = noevent }
				(-d) { timeformat = datetime }
				(-f) { histfile = $flagarg }
				(-F) { fixhistory = true }
				(-y) { yes = true }
				(-v) { verbose = true }
				(-u) { timeformat = unixtime }
				(-N) { timeformat = none }
				(-A) { analyze = true }
				* { usage }
			)
		} @{
			echo 'usage: history [-f histfile] [-dcylCuNAF] [-a | -n events | -e event]'
			usageexit = true
		} $*

		if {$usageexit} { return 0 }

		if {$analyze} {
			nlines = `{wc -l $histfile | awk '{print $1}'}
			if {! eq <={mod $nlines 2} 0} {
				echo 'error: '^$histfile^' is corrupted. run history -F.'
				return <=false
			}
			echo $histfile^' seems to be okay'
			return <=true
		}

		if {$fixhistory} {
			if {! $yes} {
				echo -n 'fix history file? [no] '
				r = <=%read
				if {%esm~ $r '[yY]([eE][sS]|)'} { yes = true }
			}
			if {$yes} {
				fix-history $histfile
				echo 'old history file saved as '^$histfile^'.old'
			}
			return 0
		}

		if {$cleanhistory} {
			if {! $yes} {
				echo -n 'only keep last '
				match $maxents (
					1 { echo -n 'entry' }
					* { echo -n $maxents^' entries' }
				)
				echo -n '? [no] '
				r = <=%read
				if {%esm~ $r '[yY]([eE][sS]|)'} { yes = true }
			}
			if {$yes} {
				cp $histfile $histfile^'.old'
				nlines = <={ mul $maxents 2}
				tail -n $nlines $histfile^'.old' > $histfile
				if {$verbose} {
					echo -n 'kept '^$maxents^' '
					match $maxents (
						1 { echo -n 'entry' }
						* { echo -n 'entries' }
					)
					echo '. save old history file as '^$histfile^'.old'
				}
			}
			return 0 # success?
		}

		fsize = `{ wc -l $histfile }
		nents = <={ div $fsize(1) 2 }
		if {$limitevents && ! $singleevent && ! $lastevent} {
			if {gt $nents $maxents} {
				nstart = <={div $fsize(1) 2 |> @{sub $1 $maxents} |> add 1}
				nlines = <={mul $maxents 2}
				tail -n $nlines $histfile | history-filter $nstart $timeformat
			} {
				history-filter 1 $timeformat < $histfile
			}
		} {$singleevent} {
			headarg = <={ mul $eventn 2 }
			if { gt $eventn $nents } {
				echo 'history: event too large' >[1=2]
				return 1
			}
				head -n $headarg $histfile \
				| tail -n 2 \
				| history-filter $eventn $timeformat
		} {$lastevent} {
			eventn = <={ sub $nents 1 }
			headarg = <={ mul $eventn 2 }
			if { gt $eventn $nents } {
				echo 'history: event too large' >[1=2]
				return 1
			}
				head -n $headarg $histfile \
				| tail -n 2 \
				| history-filter $eventn $timeformat
		} {
			history-filter 1 $timeformat < $histfile
		}
	}
}

fn lastcmd {
	eval `{history -c -l}
}

fn reload-history nelem {
	if {~ $#nelem 0} {
		throw usage reload-history 'usage: reload-history [# of commands]'
	}
	local(hist = ``(\n) {history -c -n $nelem}) {
		for (h = $hist) {
			%add-history $h
		}
	}
}

set-history = @ file {
	local(basename=<={%last <={%split '/' $file}}) {
		match $file (
			(~/*) {file = $home^/^<={~~ $matchexpr ~/*}}
			(./$basename $basename) { file = `{pwd}^'/'^$basename }
			(*/$basename) { result 0 }
		)
	}
	if {! access -rwf $file } {
		touch $file
	}
	local(set-history=) {
		$&sethistory $file
		history = $file
		if {! ~ $#history-reload 0} {
			%clear-history
			reload-history $history-reload
		}
	}
	result $file
}

fn change-history file {
	history = $file
}

