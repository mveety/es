define plist
	set var $n = $arg0
	set $i = 0
	while $n
		printf "%d: %x: prev = %x, next = %x, size = %d\n", $i, $n, $n->prev, $n->next, $n->size
		set var $n = $n->next
		set var $i = $i + 1
    end
end

define peslist
	set var $n = $arg0
	set $i = 0
	while $n
		printf "List %d: %x: term = %x, next = %x\n", $i, $n, $n->term, $n->next
		print *$n->term
		set var $n = $n->next
		set var $i = $i + 1
	end
end

define pbinding
	set var $n = $arg0
	set $i = 0
	while $n
		print *$n
		print *$n->defn->term
		set var $n = $n->next
		set var $i = $i + 1
	end
end

