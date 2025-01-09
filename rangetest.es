#!/usr/bin/env es

fn rangetest {
	echo 'old gc'
	time {./es -N -c '%range 0 100'}
	time {./es -N -c '%range 0 1000'}
	time {./es -N -c '%range 0 10000'}
#	time {./es -N -c '%range 0 100000'}
	time {./es -N -c '%range 0 100'}
	time {./es -N -c '%range 0 1000'}
	time {./es -N -c '%range 0 10000'}
#	time {./es -N -c '%range 0 100000'}

	echo 'new gc + sort after sweep defaults'
	time {./es -NX -c '%range 0 100'}
	time {./es -NX -c '%range 0 1000'}
	time {./es -NX -c '%range 0 10000'}
#	time {./es -NX -c '%range 0 100000'}
	time {./es -NX -c '%range 0 100'}
	time {./es -NX -c '%range 0 1000'}
	time {./es -NX -c '%range 0 10000'}
#	time {./es -NX -c '%range 0 100000'}

	echo 'new gc + sort/coalesce after every 10th sweep'
	time {./es -NX -S 10 -C 10 -c '%range 0 100'}
	time {./es -NX -S 10 -C 10 -c '%range 0 1000'}
	time {./es -NX -S 10 -C 10 -c '%range 0 10000'}
#	time {./es -NX -S 10 -C 10 -c '%range 0 100000'}
	time {./es -NX -S 10 -C 10 -c '%range 0 100'}
	time {./es -NX -S 10 -C 10 -c '%range 0 1000'}
	time {./es -NX -S 10 -C 10 -c '%range 0 10000'}
#	time {./es -NX -S 10 -C 10 -c '%range 0 100000'}

	echo 'new gc + sort/coalesce after every 50th sweep'
	time {./es -NX -S 50 -C 50 -c '%range 0 100'}
	time {./es -NX -S 50 -C 50 -c '%range 0 1000'}
	time {./es -NX -S 50 -C 50 -c '%range 0 10000'}
#	time {./es -NX -S 50 -C 50 -c '%range 0 100000'}
	time {./es -NX -S 50 -C 50 -c '%range 0 100'}
	time {./es -NX -S 50 -C 50 -c '%range 0 1000'}
	time {./es -NX -S 50 -C 50 -c '%range 0 10000'}
#	time {./es -NX -S 50 -C 50 -c '%range 0 100000'}

	echo 'new gc + sort/coalesce after every 100th sweep'
	time {./es -NX -S 100 -C 100 -c '%range 0 100'}
	time {./es -NX -S 100 -C 100 -c '%range 0 1000'}
	time {./es -NX -S 100 -C 100 -c '%range 0 10000'}
#	time {./es -NX -S 100 -C 100 -c '%range 0 100000'}
	time {./es -NX -S 100 -C 100 -c '%range 0 100'}
	time {./es -NX -S 100 -C 100 -c '%range 0 1000'}
	time {./es -NX -S 100 -C 100 -c '%range 0 10000'}
#	time {./es -NX -S 100 -C 100 -c '%range 0 100000'}

}

