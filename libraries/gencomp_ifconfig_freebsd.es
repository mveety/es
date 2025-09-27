library gencomp_ifconfig_freebsd (init completion general_completion)

if {! ~ `{uname} 'FreeBSD'} {
	throw error gencomp_ifconfig_freebsd 'only supports FreeBSD'
}

_gencomp_ifconfig_freebsd_parameters = (
	ether inet inet6 link lladdr
	add alias anycast arp staticarp stickyarp broadcast debug
	allmulti promic delete description down group eui64 fib
	tunnelfib maclabel media mediaopt mode txrtlmt instance
	name rxcsum txcsum rxcsum6 txcsum6 tso tso6 tso4 lro txtls
	txtlsrtlmt mextpg wol wol_ucast wol_mcast wol_magic
	vlanmtu vlanhwtag vlanhwfilter vlanhwcsum vlanhwtso
	vxlanhwcsum vxlanhwtso vnet polling create destroy plumb
	unplumb metric mtu netmask prefixlen remove link0 link1 link2
	defaultif monitor pcp up accept_rtadv no_radr auto_linklocal
	ifdisabled nud no_prefer_iface no_dad autoconf deprecated
	pltime prefer_source vltime wlandev wlanmode wlanbssid 
	wlanaddr wdslegacy bssid beacons ampdu ampdudensity ampdulimit
	amsdu amsdulimit apbridge authmode bgscan bgscanidle bgscanintvl 
	bintval bmissthreshold bssid burst chanlist channel country dfs
	dotd doth deftxkey dtimperiod quiet quiet_period quiet_count
	quiet_offset quiet_duration dturbo dwds ff fragthreshold hidessid
	ht htcompat htprotmode inact indoor list maxretry mcastrate mgtrate
	outdoor powersave powersavesleep protmode pureg puren regdomain rifs
	'roam:rate' 'roam:rssi' roaming rtsthreshold scan scanvalid shortgi
	smps smpsdyn ssid tdmaslot tdmaslotcnt tdmaslotlen tdmabintval
	tsn txpower ucastrate wepmode weptxkey wepkey wme wps
)

_gencomp_ifconfig_freebsd_list_param = (
	active caps chan countries mac mesh regdomain roam txparam txpower scan
	sta wme
)

_gencomp_ifconfig_freebsd_wme_param = (
	ack acm aifs cwmin cwmax txoplimit 'bss:aifs' 'bss:cwmin'
	'bss:cwmax' 'bss:txoplimit'
)

fn gencomp_ifconfig_freebsd_devs {
	result ``(\n){
		ifconfig |
			awk '/[a-zA-Z0-9]+[0-9]+:/ { print $0 }' |
			grep flags |
			awk 'BEGIN{ ifs=":"} { print $1 }' |
			sed 's/://g'
	}
}

fn gencomp_ifconfig_freebsd_hook curline partial {
	let (
		devs = <=gencomp_ifconfig_freebsd_devs
		cmdline = <={gencomp_split_cmdline $curline}
		cleanpartial = <={
			if {~ $partial -*} {
				result <={~~ $partial -*}
			} {
				result $partial
			}
		}
	) {
		if {~ $cmdline(1) 'doas' $es_conf_completion-prefix-commands} {
			cmdline = $cmdline(2 ...)
		}
		if {~ <={%last $cmdline} 'ifconfig' && ~ <={%count $cmdline} 1} {
			gencomp_filter_list $partial $devs
		} {
			match <={%last $cmdline} (
				'list' {
					gencomp_filter_list $partial $_gencomp_ifconfig_freebsd_list_param
				}
				'wme' {
					gencomp_filter_list $partial $_gencomp_ifconfig_freebsd_wme_param
				}
				* {
					if {~ $partial -*} {
						result '-'^<={gencomp_filter_list $cleanpartial $_gencomp_ifconfig_freebsd_parameters}
					} {
						gencomp_filter_list $cleanpartial $_gencomp_ifconfig_freebsd_parameters
					}
				}
			)
		}
	}
}

%complete_cmd_hook ifconfig @ curline partial {
	gencomp_ifconfig_freebsd_hook $curline $partial
}

