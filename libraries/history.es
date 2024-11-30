#!/usr/local/bin/es
library history (init libraries)

histmax = 25

fn history-filter start usedate commandonly {
	awk -v 'n='^$start \
		-v 'usedate='^$usedate \
		-v 'platform='^`{uname} \
		-v 'commandonly='^$commandonly \
		'
		BEGIN{
			curtime = ""
			curdate = ""
			havedate = 0
			unixtime = 0
		}

		/#\+/{
			unixtime = substr($1, 2, length($1))
			if (platform == "Linux") {
				cmd = "date -d @" unixtime " +%H:%M"
				cmd1 = "date -d @" unixtime " +%F"
			} else {
				cmd = "date -r " unixtime " +%H:%M"
				cmd1 = "date -r " unixtime " +%F"
			}
			cmd | getline curtime
			close(cmd)
			if (usedate == 1) {
				cmd1 | getline curdate
				close(cmd1)
			}
			havedate = 1
		}

		!/#\+/{
			if($1 != ""){
				if(havedate == 0){
					"date +%H:%M" | getline curtime
					if(usedate == 1){
						"date +%F" | getline curdate
					}
				}
				if(commandonly == 1) {
					printf "%s\n", $0
				} else {
					if(usedate == 1){
						printf "%d\t%s  %s  %s\n", n, curdate, curtime, $0
					} else {
						printf "%d\t%s  %s\n", n, curtime, $0
					}
				}
				n++
			}
			havedate = 0
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
		commandonly = 0
		histfile = $history
		usedate = 0
	) {
		while {! ~ $#1 0} {
			if {~ $1 -n } {
				limitevents = true
				* = $*(2 ...)
				maxents = $1
			} {~ $1 -e } {
				singleevent = true
				* = $*(2 ...)
				eventn = $1
			} {~ $1 -a } {
				limitevents = false
				singleevent = false
			} {~ $1 -l} {
				lastevent = true
			} {~ $1 -C } {
				cleanhistory = true
			} {~ $1 -c } {
				commandonly = 1
			} {~ $1 -d} {
				usedate = 1
			} {~ $1 -f } {
				* = $*(2 ...)
				histfile = $1
			} {
				throw usage history 'usage: history [-f histfile] [-d] [-c] [-C| -l | -a | -n events | -e event]'
			}
			* = $*(2 ...)
		}

		if {$cleanhistory} {
			cp $histfile $histfile^'.old'
			nlines = `{ mul $maxents 2}
			tail -n $nlines $histfile^'.old' > $histfile
			return 0 # success?
		}

		fsize = `{ wc -l $histfile }
		nents = `{ div $fsize(1) 2 }
		if {$limitevents && ! $singleevent && ! $lastevent} {
			if {gt $nents $maxents} {
				nstart = `{add `{sub `{div $fsize(1) 2} $maxents} 1}
				nlines = `{mul $maxents 2}
				tail -n $nlines $histfile | history-filter $nstart $usedate $commandonly
			} {
				history-filter 1 $usedate $commandonly < $histfile
			}
		} {$singleevent} {
			headarg = `{ mul $eventn 2 }
			if { gt $eventn $nents } {
				echo 'history: event too large' >[1=2]
				return 1
			}
			if {~ $commandonly 0} {
				head -n $headarg $histfile \
				| tail -n 2 \
				| history-filter $eventn $usedate 0 \
				| awk '($1 == '^$eventn^'){ print $0 ; exit }'
			} {
				head -n $headarg $histfile \
				| tail -n 2 \
				| history-filter $eventn $usedate 0 \
				| awk '($1 == '^$eventn^'){ print substr($0, index($0,$3)) ; exit }'
			}
		} {$lastevent} {
			eventn = `{ sub $nents 1 }
			headarg = `{ mul $eventn 2 }
			if { gt $eventn $nents } {
				echo 'history: event too large' >[1=2]
				return 1
			}
			if {~ $commandonly 0} {
				head -n $headarg $histfile \
				| tail -n 2 \
				| history-filter $eventn $usedate 0 \
				| awk '($1 == '^$eventn^'){ print $0 ; exit }'
			} {
				head -n $headarg $histfile \
				| tail -n 2 \
				| history-filter $eventn $usedate 0 \
				| awk '($1 == '^$eventn^'){ print substr($0, index($0,$3)) ; exit }'
			}
		} {
			history-filter 1 $usedate $commandonly < $histfile
		}
	}
}

fn lastcmd {
	eval `{history -c -l}
}

noexport = $noexport fn-history-filter fn-history histmax

