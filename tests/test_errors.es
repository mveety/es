#!/usr/bin/env es

fn run_test_errors {
	let (e=; s=){
		(e s) = <={try throw error test1 okay}
		assert {~ <={$&gettermtag $e} error}
		assert {~ <={$&termtypeof $e} closure}
		assert {$e}
		assert {~ $#s 0}
		assert {~ <={$e error} error}
		assert {~ <={$e type} test1}
		assert {~ <={$e msg} okay}
		catch @ err type msg {
			assert {~ $err error}
			assert {~ $type test1}
			assert {~ $msg okay}
		} {
			$e throw
		}
		catch @ err type msg {
			assert {~ $err error}
			assert {~ $type test1}
			assert {~ $msg okay}
		} {
			throw $e
		}

		(e s) = <={try {result hello}}
		assert {! $e}
		assert {~ $#s 1}
		assert {~ $s hello}
		catch @ err type msg {
			assert {~ $err error}
			assert {~ $type test2}
			assert {~ $msg good}
		} {
			throw error test2 good
		}
	}
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_errors
}

