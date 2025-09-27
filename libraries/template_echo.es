library template_echo (init libraries template)

fn-%%echo = $&withbindings @ __args {
	if {~ $__args(1) -n} {
		__args = $__args(2 ...)
		template $^__args |> echo -n
	} {
		temaplte $^__args |> echo
	}
}

fn-%%listecho = $&withbindings @ __le_list {
	local (__le_echoargs = ()) {
		if {~ $__le_list(1) -n} {
			__le_list = $__le_list(2 ...)
			__echoargs = -n
		}
		for (__le_line = $__le_list) {
			template $__le_line |> echo $__le_echoargs
		}
	}
}

