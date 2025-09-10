define peslist
	set var $n = $arg0
	set $i = 0
	while $n
		printf "%d: %x: term = %x, next = %x\n", $i, $n, $n->term, $n->next
		print *$n->term
		set var $n = $n->next
		set var $i = $i + 1
	end
end

