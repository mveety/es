# part of system.es

fn apply fun args {
	for (i = $args)
		$fun $i
}

fn map fun lst {
	local (
		tmplist = ()
		tmp = ()
	) {
		for (i = $lst) {
			tmp = <={$fun $i}
			tmplist = $tmplist $tmp
		}
		return $tmplist
	}
}

# bqmap is functionally similar to map. The primary difference is that
# instead of using the <= operator it uses `. This is for situations
# where you want the program's output instead of return values. Do note
# the program's output is not flattened.
fn bqmap fun lst {
	local (
		tmplist = ()
		tmp = ()
	) {
		for (i = $lst) {
			tmp = `{$fun $i}
			tmplist = $tmplist $tmp
		}
		return $tmplist
	}
}

# this is bqmap, but the program's output is flattened before insertion.
fn fbqmap fun lst {
	local (
		tmplist = ()
		tmp = ()
	) {
		for (i = $lst) {
			tmp = `{$fun $i}
			tmplist = $tmplist $^tmp
		}
		return $tmplist
	}
}

