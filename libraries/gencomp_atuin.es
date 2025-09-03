library complete_atuin (init completion general_completion atuin_history)

_complete_atuin_main = (
	history import stats search sync login
	logout register key status account kv
	store dotfiles scripts init info doctor
	wrapped daemon default-config server
	uuid contributors gen-completions
	help
)

_complete_atuin_history = (
	start end list last init-store prune dedup
	help
)

_complete_atuin_import = (
	auto zsh zsh-hist-db bash replxx resh fish
	nu nu-hist-db xonsh xonsh-sqlite help
)

_complete_atuin_account = (
	login register logout delete change-password
	verify help
)

_complete_atuin_kv = (
	set delete get list rebuild help
)

_complete_atuin_store = (
	status rebuild rekey purge verify push pull
	help
)

_complete_atuin_dotfiles = (
	alias var help
)

_complete_atuin_scripts = (
	new run list get edit delete help
)

_complete_atuin_server = (
	start default-config help
)

fn-gencomp_atuin_hook = <={gencomp_new_list_completer atuin <={box $_complete_atuin_main} (
	history <={box $_complete_atuin_history}
	import <={box $_complete_atuin_import}
	account <={box $_complete_atuin_account}
	kv <={box $_complete_atuin_kv}
	store <={box $_complete_atuin_store}
	dotfiles <={box $_complete_atuin_dotfiles}
	scripts <={box $_complete_atuin_scripts}
	server <={box $_complete_atuin_server}
)}

%complete_cmd_hook atuin @ curline partial {
	gencomp_atuin_hook $curline $partial
}

