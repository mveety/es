#!/usr/bin/env es

fn %gcstats args {
	local (
		(type stats) = <={$&gcstats}
		freemem=; usedmem=; nobjects=;
		ngcs=;
	) {
		match $type (
			(old) {
				freemem = $stats(1)
				usedmem = $stats(2)
				nobjects = $stats(3)
				ngcs = $stats(5)
			}
			(new) {
				freemem = $stats(1)
				usedmem = $stats(3)
				nobjects= $stats(8)
				ngcs = $stats(10)
			}
		)
		if {~ $args -v} {
			echo 'type = '^$type
			echo 'freemem = '^$freemem
			echo 'usedmem = '^$usedmem
			echo 'objects = '^$nobjects
			echo 'number of gcs = '^$ngcs
		}
		result $type $freemem $usedmem $nobjects $ngcs
	}
}

