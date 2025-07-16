#!/usr/bin/env es
library old_history (init libraries)

histmax = 25
history-reload = 25

fn history-filter start timeformat {
	awk -v 'n='^$start \
		-v 'timeformat='^$timeformat \
		-v 'platform='^`{uname} \
		'
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
					cmd = "date +%F_%H:%M"
				} else {
					if (platform == "Linux") {
						cmd = "date -d @" unixtime " +%F_%H:%M"
					} else {
						cmd = "date -r " unixtime " +%F_%H:%M"
					}
				}
				cmd | getline curdatetime
				close(cmd)
				split(curdatetime, timearr, "_")
				curdate = timearr[1]
				curtime = timearr[2]
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
				(-y) { yes = true }
				(-v) { verbose = true }
				(-u) { timeformat = unixtime }
				(-N) { timeformat = none }
				* { usage }
			)
		} @{
			echo 'usage: history [-f histfile] [-dcylCuN] [-a | -n events | -e event]'
			usageexit = true
		} $*

		if {$usageexit} { return 0 }

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

