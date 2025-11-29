#!/usr/bin/env es

fn macroreplace {
	if {~ $#1 0} {
		throw usage 'usage: '^$1^' file'
	}

	filedata = ``(\n){ cat $1 }

	process ``(\n){ cat $1 } (
		('@@ '^*) {
			let (name = <={~~ $matchexpr '@@ '^*}) {
				if {! access -r -f $name} {
					throw error $0 'file not found: '^$name
				}
				cat $name
			}
		}
		(\e^*) { continue }
		('@!! '^*) { continue }
		* { echo $matchexpr }
	)
}

