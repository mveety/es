#!/usr/bin/env es

# quick and dirty OO

library objects (init macros libraries)

fn method method class {
	local(
		fn-object_findin = @ thing list {
			local(i=1){
				while {! ~ $#list 0} {
					if {~ $thing $list(1)} {
						return $i
					}
					i = <={add $i 1}
					list = $list(2 ...)
				}
				throw error method $thing^' not found'
			}
		}
	){
		result <={%elem <={object_findin $method $class |> add 1} $class}
	}
}

fn __es_filter_unique_pairs list {
	local(res=;seen_keys=){
		for((k v) = $list) {
			if{! ~ $k $seen_keys} {
				res = $res $k $v
				seen_keys = $seen_keys $k
			}
		}
		result $res
	}
}

fn new class rest {
	let(methods=<={gensym '__es_object_'}){
		$methods = (
			<={$class $rest |> __es_filter_unique_pairs}
			__es_delete @{ $methods= }
			__es_get_methods @{ result $$methods }
			__es_add_method @ m fun {
				assert2 addmethod {~ $#m 1}
				assert2 addmethod {~ $#fun 1}
				$methods = $$methods $m $fun
			}
			__es_get_class @{ result $class }
			__es_get_object @{ result $methods } # for debugging
		)
		assert2 $0 {mod <={%count $$methods} 2}
		catch @ e type msg {
			$methods=
			if {! ~ $e error} {
				throw $e $type $msg
			} {! ~ $type method} {
				throw $e $type $msg
			}
		} {
			<={method create $$methods} $rest
		}
		result @{
			if {~ $#* 0} {
				result <={<={method default $$methods}}
			} {
				result <={<={method $1 $$methods} $*(2 ...)}
			}
		}
	}
}

fn delete object rest {
	catch @ e type msg {
		if {! ~ $e error} {
			throw $e $type $msg
		} {! ~ $type method} {
			throw $e $type $msg
		}
	} {
		$object delete $rest
	}
	$object __es_delete
}

fn get-methods object {
	result <={$object __es_get_methods}
}

fn add-method object method fun {
	assert2 $0 {~ $#method 1 && ~ $#fun 1}
	result <={$object __es_add_method $method $fun}
}

fn get-class object {
	result <={$object __es_get_class}
}

class_iterator = @{
	let(v=){
		result (
			default @{ local(t=$v(1)){ v = $v(2 ...); result $t }}
			create @{ v = $* }
			next @{ local(t=$v(1)){ v = $v(2 ...); result $t }}
			get @{ result $v }
			set @{ v = $* }
		)
	}
}

class_container = @{
	let(d=){
		result (
			default @{ result $d }
			create @{ d = $* }
			get @{ result $d }
			set @{ d = $* }
		)
	}
}

#
# some example class definitions
# 
# ex1_class = @{
# 	let(a=;b=){
# 		result (
# 			seta @{ a = $* }
# 			setb @{ b = $* }
# 			geta @{ result $a }
# 			getb @{ result $b }
# 		)
# 	}
# }
# 
# ex2_class = @{
# 	let(parent=<=$ex1_class;c=){
# 		result (
# 			$parent
# 			setc @{ c = $* }
# 			getc @{ result $c }
# 			setb2 @{ <={method setb $parent} $* }
# 		)
# 	}
# }
# 
# ex3_class = @{
# 	let(parent=<=$ex2_class;d=){
# 		result (
# 			$parent
# 			create @{ echo 'hello world!' }
# 			setd @{ d = $* }
# 			getd @{ result $d }
# 		)
# 	}
# }
# 
# ex4_class = @{
# 	let(parent=<=$ex3_class){
# 		result (
# 			create @{ echo 'goodbye' }
# 			$parent
# 		)
# 	}
# }
# 
