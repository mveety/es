library template_echo (init libraries template)

fn-tempecho = $&withbindings @ args {
	template $^args |> $&echo
}

