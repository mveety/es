library noclobber (init)

# this is an implementation of noclobber from mainline es. This is broadly
# similar and as quick and dirty as the original.
# original: https://github.com/wryun/es-shell/blob/master/examples/noclobber.es
# this is up for inclusion in our es.

# by default we don't want to change behaviour at all, so this returns
# false
fn %clobbers file {
	false
}

let (old_create = $fn-%create) {
	fn %create fd file cmd {
		if {%clobbers $file} {
			throw error %create $^cmd^' would clobber '^$file
		} {
			$old_create $fd $file $cmd
		}
	}
}

fn noclobber body {
	if {! ~ $#body 1} {
		throw usage $0 'usage: noclobber body'
	}
	local (
		fn %clobbers file {
			access -f $file
		}
	) {
		$body
	}
}

