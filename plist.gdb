define plist
	set var $n = $arg0
	set $i = 0
	while $n
		printf "%d: %x: prev = %x, next = %x, size = %d\n", $i, $n, $n->prev, $n->next, $n->size
		set var $n = $n->next
		set var $i = $i + 1
	end
end

