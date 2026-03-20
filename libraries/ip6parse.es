library ip6parse init

fn get-raw-ip6-addresses interface _ {
	let (addrs = () ){
		for (l = ``(\n){ifconfig $interface | grep inet6}) {
			addrs += <={%split ' ' $l |> %elem 2}
		}
	}
}

fn makezeros list {
	let (res = ()) {
		for(_ = $list) {
			res += '0000'
		}
		result $res
	}
}

fn ip6expandaddr addr _ {
	let (
		res =
	) {
		if {~ $addr *^'::'^*} {
			let (
				(head tail) = <={~~ $addr *^'::'^*}
				midfix=
			) {
				echovar head
				echovar tail
				head = <={%split ':' $head}
				tail = <={%split ':' $tail}
				echovar head
				echovar tail
				midfix = <={
					add $#head $#tail |>
						sub 8 |>
						%range 1 |>
						makezeros
				}
				result <={%flatten ':' $head $midfix $tail}
			}
		} {
			result $addr
		}
	}
}

fn parseip6addr addr _ {
	lets (
		saddr = <={ip6expandaddr $addr |> %split ':'}
		prefixpart = $saddr(1 ... 4)
		postfixpart = $saddr(5 ...)
	) {
		result %dict(prefix=>$prefixpart;postfix=>$postfixpart)
	}
}

fn unparseip6addr paddr _ {
	%flatten ':' $paddr(prefix) $paddr(postfix)
}

fn ip6reprefix addr1 addrs {
	let (
		paddr1 = <={parseip6addr $addr1}
		res = ()
	) {
		for (addr = $addrs) {
			addr = <={parseip6addr $addr}
			res += %dict(prefix=>$paddr1(prefix);postfix=>$addr(postfix))
		}
		let (rr = () ) {
			for (r = $res) {
				rr += <={unparseip6addr $r}
			}
			return $rr
		}
	}
}

