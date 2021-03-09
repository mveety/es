#!/usr/local/bin/es
options = $options history

histmax = 25

fn history-filter start {
	awk -v 'n='^$start '
		BEGIN{
			curdate = ""
			havedate = 0
			unixtime = 0
		}

		/#\+/{
			unixtime = substr($1, 2, length($1))
			cmd = "date -r " unixtime " +%H:%M"
			cmd | getline result
			close(cmd)
			curdate = result
			havedate = 1
		}

		!/#\+/{
			if($1 != ""){
				if(havedate == 0){
					"date +%H:%M" | getline curdate
				}
				printf "%d\t%s  %s\n", n, curdate, $0
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
			} {~ $1 -f } {
				* = $*(2 ...)
				histfile = $1
			} {
				throw error history 'usage: history [-f histfile] [-C] [-a | -n events | -e event]'
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
				tail -n $nlines $histfile | history-filter $nstart
			} {
				history-filter 0 < $histfile
			}
		} {$singleevent} {
			history-filter 1 < $histfile | awk '($1 == '^$eventn^'){ print $0 ; exit 1 }'
		} {
			history-filter 1 < $histfile
		}
	}
}

noexport = $noexport fn-history-filter fn-history histmax
