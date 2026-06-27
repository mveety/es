library dumplist (init)

defconf dumplist formatting %dict(maxcol => 65; tabsize => 4)
defconftypefn dumplist formatting @ arg _ { if {~ <={%termtypeof $arg} dict} { true } { false }}

fn print-list varname options list {
	if {! ~ <={%termtypeof $options} dict} {
		list = $options $list
		options = %dict()
	}
	options = $dumplist_conf_formatting^$options
	lets (
		col = 0
		ts =
		fn fmtecho s {
			if {gte $col $options(maxcol)} {
				echo ''
				col = 0
			}
			if {eq $col 0} {
				echo -n \t
				col = $options(tabsize)
			}
			ts = ''''^$s^''''
			if {gt $col $options(tabsize)} {
				ts = ' '^$ts
			}
			echo -n $ts
			col = <={%count $:ts |> add $col}
		}
	) {
		echo $varname^' = ('
		for (l = $list) {
			fmtecho $l
		}
		echo ''
		echo ')'
	}
}

fn print-dict varname options dict {
	if {! ~ <={%termtypeof $options} dict} {
		throw error print-dict 'options must be a dict'
	}
	options = $dumplist_conf_formatting^$options
	lets (
		keys = <={dictkeys $dict}
	) {
		echo $varname^' = %dict('
		for (k = $keys) {
			echo -n \t
			echo $k^' => '^<={%fmt $dict($k)}^';'
		}
		echo ')'
	}
}

fn-dumplist = $&withbindings @ v {
	print-list $v %dict() $$v
}

fn-dumpdict = $&withbindings @ v {
	print-dict $v %dict() $$v
}

fn-dumpstruct = $&withbindings @ v {
	if {~ <={%termtypeof $$v} dict} {
		dumpdict $v
	} {
		dumplist $v
	}
}

