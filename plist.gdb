define plist
	set var $n = $arg0
	set $i = 0
	while $n
		printf "%d(%d, %d): %x: prev = %x, next = %x\n", $i, $n->intype, $n->alloc, $n, $n->prev, $n->next
		set var $n = $n->next
		set var $i = $i + 1
	end
end

