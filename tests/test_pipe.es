#!/usr/bin/env es

fn run_test_pipe {
	let (
		fn f1 { return 1 }
		fn f2 { return 2 }
		fn f3 { return 3 }
		t1=;t2=;t3=;
	) {
		t1 = <={f1 | f2 | f3}
		t2 = <={f2 | f3 | f1}
		t3 = <={f3 | f1 | f2}

		assert {~ $t1(1) 1}
		assert {~ $t1(2) 2}
		assert {~ $t1(3) 3}

		assert {~ $t2(1) 2}
		assert {~ $t2(2) 3}
		assert {~ $t2(3) 1}

		assert {~ $t3(1) 3}
		assert {~ $t3(2) 1}
		assert {~ $t3(3) 2}
	}
	result <=true
}

if {~ $#inside_estest 0 || ! $inside_estest} {
	run_test_pipe
}

