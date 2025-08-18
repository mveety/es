#!/usr/bin/env es

fn test_errmatch_test1 x {
	errmatch $x error { return 'okay' }
	return 'not okay'
}

fn test_errmatch_test2 {
	errmatch false error { return 'not okay' }
	return 'okay'
}

fn run_test_errmatch {
	errmatch <={makeerror error test1 okay} (
		error nottest1 notokay { assert false }
		error test1 notokay { assert false }
		error test1 okay { assert true }
		error test2 okay { assert false }
		{ assert false }
	)

	errmatch <={makeerror assert {~ 0 1}} (
		error { assert false }
		usage notokay { assert false }
		assert { assert true }
		{ assert false }
	)

	let (e=) {
		(e _) = <={try errmatch}
		assert {$e}
		assert {~ <={$e error} error &&
			~ <={$e type} $&termtypeof &&
			~ <={$e msg} 'missing argument'}
		(e _) = <={try errmatch @{result hello}}
		assert {$e}
		assert {~ <={$e error} assert}
		(e _) = <={try errmatch <={makeerror error 1 2}}
		assert {$e}
		assert {~ <={$e error} error &&
			~ <={$e type} errmatch &&
			~ <={$e msg} 'missing arguments'}
	}
	let (e=<={makeerror error test ohno}; t=) {
		t = <={test_errmatch_test1 $e}
		assert {~ $t 'okay'}
	}
	assert {~ <={test_errmatch_test2} 'okay'}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_errmatch
}

