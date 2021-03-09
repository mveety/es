#!/usr/local/bin/es
option history

histmax = 25

fn history-filter start usedate {
	awk -v 'n='^$start -v 'usedate='^$usedate '
		BEGIN{
			curtime = ""
			curdate = ""
			havedate = 0
			unixtime = 0
		}

		/#\+/{
			unixtime = substr($1, 2, length($1))
			cmd = "date -r " unixtime " +%H:%M"
			cmd | getline curtime
			close(cmd)
			if (usedate == 1) {
				cmd1 = "date -r " unixtime " +%F"
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
				if(usedate == 1){
					printf "%d\t%s  %s  %s\n", n, curdate, curtime, $0
				} else {
					printf "%d\t%s  %s\n", n, curtime, $0
				}
				n++
			}
			havedate = 0
		}'
}

fn history {
	let(
		maxents = $histmax
		limitevents = true
		singleevent = false
		cleanhistory = false
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
			} {~ $1 -C } {
				cleanhistory = true
			} {~ $1 -d} {
				usedate = 1
			} {~ $1 -f } {
				* = $*(2 ...)
				histfile = $1
			} {
				throw error history 'usage: history [-f histfile] [-d] [-C] [-a | -n events | -e event]'
			}
			* = $*(2 ...)
		}
		if {$cleanhistory} {
			cp $histfile $histfile^'.old'
			tail -n $maxents $histfile^'.old' > $histfile
			return 0 # success?
		}
		if {$limitevents && ! $singleevent} {
			fsize = `{ wc -l $histfile }
			nents = `{ awk 'BEGIN{ print '^$fsize(1)^'/2 }' }
			st = `{ awk 'BEGIN { n='^$nents^' ; if( n < '^$maxents^') { print "yes\n" } else { print "no" } }' }
			if {~ $st 'no' } {
				nstart = `{ awk 'BEGIN{ print (('^$fsize^'/2) - '^$maxents^')+1 }' }
				nlines = `{ awk 'BEGIN{ print 2*'^$maxents^' }' }
				tail -n $nlines $histfile | history-filter $nstart $usedate
			} {
				history-filter 1 $usedate < $histfile
			}
		} {$singleevent} {
			history-filter 1 $usedate < $histfile | awk '($1 == '^$eventn^'){ print $0 ; exit 1 }'
		} {
			history-filter 1 $usedate < $histfile
		}
	}
}

noexport = $noexport fn-history-filter fn-history histmax
