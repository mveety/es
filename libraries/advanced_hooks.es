library advanced_hooks (init libraries)

# allows setting multiple hooks on %preexec, %postexec, %cdhook, and %prompt

let (
	preexec-hooks = %dict()
	enabled = false
) {
	fn adv_preexec_hook {
		if {~ $#preexec-hooks 0} {
			return <=true
		}
		for (hookname = <={dictnames $preexec-hooks |> sortlist}){
			$preexec-hooks($hookname)
		}
	}

	fn add-preexec-hook name fun {
		preexec-hooks := $name => $fun
	}

	fn del-preexec-hook name {
		{preexec-hooks = <={dictremove $preexec-hooks $name}} onerror {
			throw error 'del-preexec-hook' 'hook '^$name^' does not exist'
		}
		return <=true
	}

	fn preexec-hooks {
		dictnames $preexec-hooks |> sortlist |> result
	}

	fn get-preexec-hook hookname {
		result $preexec-hooks($hookname) onerror {
			throw error 'get-preexec-hook' $hookname^' not found'
		}
	}

	fn enable-preexec-hook {
		if {$enabled} {
			throw error 'enable-preexec-hook' 'hook already enabled'
		}
		if {! ~ $#fn-%preexec 0} {
			preexec-hooks := default => $fn-%preexec
		}
		fn-%preexec = $fn-adv_preexec_hook
		set-fn-%preexec = @ arg {
			throw error 'adv_preexec_hook' 'tried to set %preexec'
		}
		enabled = true
		return <=true
	}

	fn disable-preexec-hook {
		if {! $enabled} {
			throw error 'disable-preexec-hook' 'hook already disabled'
		}
			if {dicthaskey $preexec-hooks default} {
				oldhook = $preexec-hooks(default)
				set-fn-%preexec=
				fn-%preexec = $preexec-hooks(default)
				enabled = false
				return <=true
			}
			set-fn-%preexec=
			fn %preexec
			enabled = false
			return <=true
	}

	defconf advanced_hooks preexec disable
	set-advanced_hooks_conf_preexec = @ arg _ {
		if {! ~ $arg enable disable} {
			return $advanced_hooks_conf_preexec
		}
		if {~ $arg enable && ! $enabled} {
			enable-preexec-hook
		} {~ $arg disable && $enabled} {
			disable-preexec-hook
		}
		return $arg
	}

	# work around the closure.c:121 crash
	noexport += fn-adv_preexec_hook
	noexport += fn-add-preexec-hook
	noexport += fn-del-preexec-hook
	noexport += fn-preexec-hooks
	noexport += fn-get-preexec-hook
	noexport += fn-enable-preexec-hook
	noexport += fn-disable-preexec-hook
	noexport += advanced_hooks_conf_preexec
	noexport += set-advanced_hooks_conf_preexec
}

let (
	postexec-hooks = %dict()
	enabled = false
) {
	fn adv_postexec_hook args {
		if {~ $#postexec-hooks 0} {
			return <=true
		}
		for (hookname = <={dictnames $postexec-hooks |> sortlist}){
			$postexec-hooks($hookname)
		}
	}

	fn add-postexec-hook name fun {
		postexec-hooks := $name => $fun
	}

	fn del-postexec-hook name {
		{postexec-hooks = <={dictremove $postexec-hooks $name}} onerror {
			throw error 'del-postexec-hook' 'hook '^$name^' does not exist'
		}
		return <=true
	}

	fn postexec-hooks {
		dictnames $postexec-hooks |> sortlist |> result
	}

	fn get-postexec-hook hookname {
		result $postexec-hooks($hookname) onerror {
			throw error 'get-postexec-hook' $hookname^' not found'
		}
	}

	fn enable-postexec-hook {
		if {$enabled} {
			throw error 'enable-postexec-hook' 'hook already enabled'
		}
		if {! ~ $#fn-%postexec 0} {
			postexec-hooks := default => $fn-%postexec
		}
		fn-%postexec = $fn-adv_postexec_hook
		set-fn-%postexec = @ arg {
			throw error 'adv_postexec_hook' 'tried to set %postexec'
		}
		enabled = true
		return <=true
	}

	fn disable-postexec-hook {
		if {! $enabled} {
			throw error 'disable-postexec-hook' 'hook already disabled'
		}
			if {dicthaskey $postexec-hooks default} {
				oldhook = $postexec-hooks(default)
				set-fn-%postexec=
				fn-%postexec = $postexec-hooks(default)
				enabled = false
				return <=true
			}
			set-fn-%postexec=
			fn %postexec
			enabled = false
			return <=true
	}

	defconf advanced_hooks postexec disable
	set-advanced_hooks_conf_postexec = @ arg _ {
		if {! ~ $arg enable disable} {
			return $advanced_hooks_conf_postexec
		}
		if {~ $arg enable && ! $enabled} {
			enable-postexec-hook
		} {~ $arg disable && $enabled} {
			disable-postexec-hook
		}
		return $arg
	}

	# work around the closure.c:121 crash
	noexport += fn-adv_postexec_hook
	noexport += fn-add-postexec-hook
	noexport += fn-del-postexec-hook
	noexport += fn-postexec-hooks
	noexport += fn-get-postexec-hook
	noexport += fn-enable-postexec-hook
	noexport += fn-disable-postexec-hook
	noexport += advanced_hooks_conf_postexec
	noexport += set-advanced_hooks_conf_postexec
}

let (
	cdhook-hooks = %dict()
	enabled = false
) {
	fn adv_cdhook_hook args {
		if {~ $#cdhook-hooks 0} {
			return <=true
		}
		for (hookname = <={dictnames $cdhook-hooks |> sortlist}){
			$cdhook-hooks($hookname) $args
		}
	}

	fn add-cdhook-hook name fun {
		cdhook-hooks := $name => $fun
	}

	fn del-cdhook-hook name {
		{cdhook-hooks = <={dictremove $cdhook-hooks $name}} onerror {
			throw error 'adv-cdhook-hook' 'hook '^$name^' does not exist'
		}
		return <=true
	}

	fn cdhook-hooks {
		dictnames $cdhook-hooks |> sortlist |> result
	}

	fn get-cdhook-hook hookname {
		result $cdhook-hooks($hookname) onerror {
			throw error 'get-cdhook-hook' $hookname^' not found'
		}
	}


	fn enable-cdhook-hook {
		if {$enabled} {
			throw error 'enable-cdhook-hook' 'hook already enabled'
		}
		if {! ~ $#fn-%cdhook 0} {
			cdhook-hooks := default => $fn-%cdhook
		}
		fn-%cdhook = $fn-adv_cdhook_hook
		set-fn-%cdhook = @ arg {
			throw error 'adv_cdhook_hook' 'tried to set %cdhook'
		}
		enabled = true
		return <=true
	}

	fn disable-cdhook-hook {
		if {! $enabled} {
			throw error 'disable-cdhook-hook' 'hook already disabled'
		}
			if {dicthaskey $cdhook-hooks default} {
				oldhook = $cdhook-hooks(default)
				set-fn-%cdhook=
				fn-%cdhook = $cdhook-hooks(default)
				enabled = false
				return <=true
			}
			set-fn-%cdhook=
			fn %cdhook
			enabled = false
			return <=true
	}

	defconf advanced_hooks cdhook disable
	set-advanced_hooks_conf_cdhook = @ arg _ {
		if {! ~ $arg enable disable} {
			return $advanced_hooks_conf_cdhook
		}
		if {~ $arg enable && ! $enabled} {
			enable-cdhook-hook
		} {~ $arg disable && $enabled} {
			disable-cdhook-hook
		}
		return $arg
	}

	# work around the closure.c:121 crash
	noexport += fn-adv_cdhook_hook
	noexport += fn-add-cdhook-hook
	noexport += fn-del-cdhook-hook
	noexport += fn-cdhook-hooks
	noexport += fn-get-cdhook-hook
	noexport += fn-enable-cdhook-hook
	noexport += fn-disable-cdhook-hook
	noexport += advanced_hooks_conf_cdhook
	noexport += set-advanced_hooks_conf_cdhook

}

let (
	prompt-hooks = %dict()
	enabled = false
) {
	fn adv_prompt_hook {
		if {~ $#prompt-hooks 0} {
			return <=true
		}
		for (hookname = <={dictnames $prompt-hooks |> sortlist}){
			$prompt-hooks($hookname)
		}
	}

	fn add-prompt-hook name fun {
		prompt-hooks := $name => $fun
	}

	fn del-prompt-hook name {
		{prompt-hooks = <={dictremove $prompt-hooks $name}} onerror {
			throw error 'del-prompt-hook' 'hook '^$name^' does not exist'
		}
		return <=true
	}

	fn prompt-hooks {
		dictnames $prompt-hooks |> sortlist |> result
	}

	fn get-prompt-hook hookname {
		result $prompt-hooks($hookname) onerror {
			throw error 'get-prompt-hook' $hookname^' not found'
		}
	}

	fn enable-prompt-hook {
		if {$enabled} {
			throw error 'enable-prompt-hook' 'hook already enabled'
		}
		if {! ~ $#fn-%prompt 0} {
			prompt-hooks := default => $fn-%prompt
		}
		fn-%prompt = $fn-adv_prompt_hook
		set-fn-%prompt = @ arg {
			throw error 'adv_prompt_hook' 'tried to set %prompt'
		}
		enabled = true
		return <=true
	}

	fn disable-prompt-hook {
		if {! $enabled} {
			throw error 'disable-prompt-hook' 'hook already disabled'
		}
			if {dicthaskey $prompt-hooks default} {
				oldhook = $prompt-hooks(default)
				set-fn-%prompt=
				fn-%prompt = $prompt-hooks(default)
				enabled = false
				return <=true
			}
			set-fn-%prompt=
			fn %prompt
			enabled = false
			return <=true
	}

	defconf advanced_hooks prompt disable
	set-advanced_hooks_conf_prompt = @ arg _ {
		if {! ~ $arg enable disable} {
			return $advanced_hooks_conf_prompt
		}
		if {~ $arg enable && ! $enabled} {
			enable-prompt-hook
		} {~ $arg disable && $enabled} {
			disable-prompt-hook
		}
		return $arg
	}

	# work around the closure.c:121 crash
	noexport += fn-adv_prompt_hook
	noexport += fn-add-prompt-hook
	noexport += fn-del-prompt-hook
	noexport += fn-prompt-hooks
	noexport += fn-get-prompt-hook
	noexport += fn-enable-prompt-hook
	noexport += fn-disable-prompt-hook
	noexport += advanced_hooks_conf_prompt
	noexport += set-advanced_hooks_conf_prompt
}


