library complete_atuin (init completion general_completion atuin_history)

complete_atuin_main = (
	history import stats search sync login
	logout register key status account kv
	store dotfiles scripts init info doctor
	wrapped daemon default-config server
	uuid contributors gen-completions
	help
)

complete_atuin_history = (
	start end list last init-store prune dedup
	help
)

complete_atuin_import = (
	auto zsh zsh-hist-db bash replxx resh fish
	nu nu-hist-db xonsh xonsh-sqlite help
)

complete_atuin_account = (
	login register logout delete change-password
	verify help
)

complete_atuin_kv = (
	set delete get list rebuild help
)

complete_atuin_store = (
	status rebuild rekey purge verify push pull
	help
)

complete_atuin_dotfiles = (
	alias var help
)

complete_atuin_scripts = (
	new run list get edit delete help
)

complete_atuin_server = (
	start default-config help
)

fn-gencomp_atuin_hook = <={gencomp_new_list_completer atuin <={gencomp_list_container $complete_atuin_main} (
	history <={gencomp_list_container $complete_atuin_history}
	import <={gencomp_list_container $complete_atuin_import}
	account <={gencomp_list_container $complete_atuin_account}
	kv <={gencomp_new_list_completer $complete_atuin_kv}
	store <={gencomp_list_container $complete_atuin_store}
	dotfiles <={gencomp_list_container $complete_atuin_dotfiles}
	scripts <={gencomp_list_container $complete_atuin_scripts}
	server <={gencomp_list_container $complete_atuin_server}
)}

%complete_cmd_hook atuin @ curline partial {
	gencomp_atuin_hook $curline $partial
}

