#!/usr/bin/env es

fn run_test_errmatch {
	let (res=) {
		res = <={errmatch <={makeerror error test1 okay} (
			error nottest1 notokay { return false }
			error test1 notokay { return false }
			error test1 okay { return true }
			error test2 okay { return false }
			{ return false }
		)}
		assert {$res}
	}

	let (res=) {
		res = <={errmatch <={makeerror assert {~ 0 1}} (
			error { return false }
			usage notokay { return false }
			assert { return true }
			{ return false }
		)}
		assert {$res}
	}

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
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_errmatch
}

