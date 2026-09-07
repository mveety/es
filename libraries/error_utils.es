library error_utils (init)

fn-err = $&withbindings @ msg args {
	let (argsn=;edef=;preamble=true) {
		if {~ $msg -m} {
			preamble = false
			msg = $args(1)
			args = $args(2 ...)
		}
		(argsn edef) = $errmsgs($msg) onerror {
			throw error error_messages 'error '^$msg^' is not defined'
		}
		if {! ~ $#args $argsn} {
			throw error error_messages \
				'error '^$msg^' has the wrong number of args (should be '^$argsn^' but is '^$#args^')'
		}
		if {$preamble} {
			result error $0 <={$edef $args}
		} {
			$edef $args
		}
	}
}

fn rethrow fname error _ {
	assert iserror $error
	let ((e _ m) = <={$error info}) {
		throw $e $fname $m
	}
}

fn rethrow? fname error _ {
	if {$error} {
		rethrow $fname $error
	}
}

fn throw? error _ {
	if {$error} {
		assert iserror $error
		throw $error
	}
}

