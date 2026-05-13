library esbug (init)

fn import-os-release {
	for (l = ``(\n){cat /etc/os-release | sed 's/"/''/g'}) { eval $l }
}

fn esbug {
	hostname = `{hostname}
	unamea = `{uname -a}
	echo 'hostname = '^<={%fmt $^hostname}
	echo 'uname-a = '^<={%fmt $^unamea}
	echo 'buildstring = '^<={%fmt $buildstring}
	echo 'buildtime = '^<={%fmt $__es_buildtime}
	echo 'cflags = '^<={%fmt $__es_cflags}
	if {access -r /etc/os-release} {
		echo '## /etc/os-release'
		cat /etc/os-release | sed 's/"/''/g'
	}
}

