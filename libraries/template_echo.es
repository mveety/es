library template_echo (init libraries template)

fn-echo = $&withbindings @ args {
	template $^args |> $&echo
}

