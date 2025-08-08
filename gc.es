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
			(new generational) {
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

fn %gc {
	assert {~ gc <=$&primitives}
	$&gc
}

fn %gcinfo {
	match <={$&gcstats} (
		(old) {
			echo 'type =' $matchexpr(1)
			echo 'freemem =' $matchexpr(2) '('^<={div $matchexpr(2) 1024} 'kb)'
			echo 'usedmem =' $matchexpr(3) '('^<={div $matchexpr(3) 1024} 'kb)'
			echo 'objects =' $matchexpr(4)
			echo 'allocations =' $matchexpr(5)
			echo 'number of gcs =' $matchexpr(6)
		}
		(new generational) {
			echo 'type =' $matchexpr(1)
			echo 'freemem (total) =' $matchexpr(2) '('^<={div $matchexpr(2) 1024} 'kb)'
			echo 'freemem (real) =' $matchexpr(3) '('^<={div $matchexpr(3) 1024} 'kb)'
			echo 'usedmem (total) =' $matchexpr(4) '('^<={div $matchexpr(4) 1024} 'kb)'
			echo 'usedmem (real) =' $matchexpr(5) '('^<={div $matchexpr(5) 1024} 'kb)'
			echo 'free blocks =' $matchexpr(6)
			echo 'used blocks =' $matchexpr(7)
			if {~ $matchexpr(1) generational} {
				echo 'old blocks =' $matchexpr(21)
			}
			echo 'number of frees =' $matchexpr(8)
			echo 'number of allocs =' $matchexpr(9)
			echo 'allocations =' $matchexpr(10)
			echo 'number of gcs =' $matchexpr(11)
			echo 'sort_after_n =' $matchexpr(12)
			echo 'nsortgc =' $matchexpr(13)
			echo 'coalesce_after =' $matchexpr(14)
			echo 'ncoalescegc =' $matchexpr(15)
			echo 'gc_after =' $matchexpr(16)
			echo 'nregions =' $matchexpr(17)
			echo 'nsort =' $matchexpr(18)
			echo 'ncoalesce =' $matchexpr(19)
			echo 'blocksize =' $matchexpr(20)
			if {~ $matchexpr(1) generational} {
				echo 'oldage =' $matchexpr(22)
				echo 'oldsweep_after =' $matchexpr(23)
			}
			if {~ $1 -v || ~ $1 -r} {
				for (r = <={$&dumpregions}; n = <={%range 1 $matchexpr(17)}) {
					echo '    region '^$n^': '^$r^' ('^<={div $r 1024 |> @{div $1 1024}}^' megabytes)'
				}
			}
		}
	)
}

