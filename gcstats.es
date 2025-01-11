#!/usr/bin/env es

fn %gcstats args {
	local (
		freemem=; usedmem=; nobjects=;
		ngcs=; type=;
	) {
		match <={$&gcstats} (
			(old) {
				type = $matchexpr(1)
				freemem = $matchexpr(2)
				usedmem = $matchexpr(3)
				nobjects = $matchexpr(4)
				ngcs = $matchexpr(6)
			}
			(new) {
				type = $matchexpr(1)
				freemem = $matchexpr(2)
				usedmem = $matchexpr(4)
				nobjects= $matchexpr(9)
				ngcs = $matchexpr(11)
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

