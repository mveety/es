define plist
	set var $n = $arg0
	set $i = 0
	while $n
		printf "%d(%d, %d): %x: prev = %x, next = %x, size = %d, h->tag = %d, h->flags = %d\n", $i, $n->intype, $n->alloc, $n, $n->prev, $n->next, $n->size, $n->h->tag, $n->h->flags
		set var $n = $n->next
		set var $i = $i + 1
	end
end

