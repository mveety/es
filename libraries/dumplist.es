library dumplist (init)

defconf dumplist formatting %dict(maxcol => 65; tabsize => 4)

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

fn-dumplist = $&withbindings @ v {
	print-list $v %dict() $$v
}

