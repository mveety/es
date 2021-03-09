#!/usr/local/bin/es
options = $options history

histmax = 25

fn history-work hfile {
	cat $hfile | awk '
		BEGIN{
			curdate = ""
			havedate = 0
			n = 1
		}

		/#+/{
			unixtime = substr($1, 2, length($1))
			cmd = "date -r " unixtime " +%H:%M"
			cmd | getline result
			close(cmd)
			curdate = result
			havedate = 1
		}

		!/#+/{
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

fn history-getent hfile ent {
	history-work $hfile | awk '($1 == '^$ent^'){ print $0 }'
}

fn history {
	let(
		maxents = $histmax
		limitevents = true
		singleevent = false
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
			} {~ $1 -f } {
				* = $*(2 ...)
				histfile = $1
			} {
				throw error history 'usage: history [-f histfile] [-a | -n events | -e event]'
			}
			* = $*(2 ...)
		}
		if {$limitevents && ! $singleevent} {
			history-work $histfile | tail -n $maxents
		} {$singleevent} {
			history-getent $histfile $eventn
		} {
			history-work $histfile
		}
	}
}

noexport = $noexport fn-history-work fn-history-getent fn-history histmax
