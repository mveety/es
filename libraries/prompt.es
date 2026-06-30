#!/usr/bin/env es

library prompt (init libraries cwd colors)

let (
	hostname = `{uname -n}
	username = `{whoami}
	redbold = <={%string $colors(fg_red) $attrib(bold) |> esmle_wrap}
	green = <={esmle_wrap $colors(fg_green)}
	nocolor = <={esmle_wrap $attrib(reset)}
) {

	# get git branch info
	fn mv-git-branch {
		local(b=<-{git branch '--show-current' >[2=1]}) {
			if {~ $b(1) 0} {
				if {~ <={%count `{git status --porcelain >[2=]}} 0} {
					result ' ('^$green^$b(2)^$nocolor^')'
				} {
					result ' ('^$redbold^$b(2)^$nocolor^')'
				}
			} {
				result ''
			}
		}
	}

	# format path to shorten it (if possible)
	fn mv-pathfmt path {
		local(homes=(/usr/home /home /export/home)) {
			if {~ $username 'root' && ~ $path /root /root/*} {
				return '~'^<={~~ $path /root*}
			}
			match $path (
				($homes/$username) {
					return '~'
				}
				($homes/$username/*) {
					return '~/'^<={~~ $path $homes/$username/*}
				}
				($homes/*) {
					return '~'^<={~~ $path $homes/*}
				}
				(/root) {
					return '~root'
				}
				(/root/*) {
					return '~root/'^<={~~ $path /root/*}
				}
				* {
					return $path
				}
			)
		}
	}

	# easy switching between prompts
	fn set-prompt pname {
		if {~ $#pname 0} {
			throw usage set-prompt $0^' [-a | prompt-name]'
		}
		if {~ $pname(1) -a} {
			echo -n 'prompts available: '
			echo <={process <={~~ <={glob 'fn-*-prompt' <=$&internals <=$&vars} fn-*-prompt} (
				(set) {result}
				* {result $matchexpr}
			)} default
			return <=true
		}
		if {~ $pname default} {
			fn-%prompt=
			prompt = ('; ' '')
			return <=true
		}
		if {~ $#('fn-'^$pname^'-prompt') 0} {
			throw error set-prompt 'prompt '^$pname^' does not exist'
		} {
			fn-%prompt = $('fn-'^$pname^'-prompt')
			result <=true
		}
	}

	if {~ $#prompt_conf_name 0} {
		prompt_conf_name = ''
	}
	defconf prompt name ''
	set-prompt_conf_name = @{ set-prompt $* ; result $* }

	## prompt options

	# pretty prompt
	fn mveety-prompt {
		prompt = ($hostname^':'^<={mv-pathfmt $cwd}^<={mv-git-branch}^' % ' '_% ')
	}

	fn mveety-nogit-prompt {
		prompt = ($hostname^':'^<={mv-pathfmt $cwd}^' % ' '_% ')
	}

	fn mveety-nopath-prompt {
		prompt = ($hostname^' '^<={mv-git-branch}^' % ' '_% ')
	}

	fn git-only-prompt {
		prompt = (<={mv-git-branch}^' % ' '_% ')
	}

	fn path-prompt {
		prompt = ($cwd^'% ' '_% ')
	}

	fn pathfmt-prompt {
		prompt = (<={mv-pathfmt $cwd}^'% ' '_% ')
	}

	fn hostname-prompt {
		prompt = ($hostname^'% ' '_% ')
	}

	fn test-prompt {
		prompt = ('test% ' '_% ')
	}
}

