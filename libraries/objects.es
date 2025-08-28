#!/usr/bin/env es

# quick and dirty OO

library objects (init macros libraries types)

fn %classname name {
	result (__es_classname @{ result $name })
}

fn method method class {
	echo 'class =' $class
	echo 'method =' $method
	result <={dictget $class $method onerror {
		echo 'oh no!'
		throw error method $method^' not found'
	}}
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
	local(tmpmethods=){
		let(methods=<={gensym '__es_object_'}){
			tmpmethods = (
				<={$class}
				__es_delete @{ $methods= }
				__es_get_methods @{ result $$methods }
				__es_add_method @ m fun {
					assert2 addmethod {~ $#m 1}
					assert2 addmethod {~ $#fun 1}
					$methods = <={dictput $$methods $m $fun}
				}
				__es_get_class @{ result $class }
				__es_get_object @{ result $methods } # for debugging
				__es_is_object @{ result <=true }
			)
			echo $tmpmethods
			assert {~ <={mod <={%count $tmpmethods} 2} 0}
			$methods = <=dictnew
			for ((mn mf) = $tmpmethods) {
				echo $mn
				$methods = <={dictput $$methods $mn $mf}
			}
			dictnames $$methods |> echo
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

fn _object_testfn op v {
	match $op (
		'test' {
			if {~ <={prim_typeof $v} function} {
				if {$v __es_is_object} {
					let (classname = <={$v __es_classname}) {
						return composite 'object:'^$^classname
					}
				}
			}
			return false '__es_not_type'
		}
		'primordial' { return <=false }
		'search' {
			if {~ $v 'object'} { return <=true }
		}
		'name' {
			return 'object'
		}
		* { return <=false }
	)
}

install_dynamic_type 'object' @{ _object_testfn $* }

class_iterator = @{
	let(v=){
		result (
			<={%classname 'iterator'}
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
			<={%classname 'container'}
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
