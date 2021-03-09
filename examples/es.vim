" vim syntax file for es

if exists("b:current_syntax")
	finish
endif

let b:current_syntax = "es"
syn sync fromstart

" keywords
syn keyword esCmd access break catch cd echo
syn keyword esCmd eval exec exit false forever 
syn keyword esCmd fork if limit newpgrp result
syn keyword esCmd return throw time true umask unwind-protect
syn keyword esCmd var vars wait whats while
syn keyword esCmd let for fn
syn match eskeyList /[a-zA-Z0-9]*\s*(/
syn match esHook /[%][a-zA-Z0-9_]*/
syn match esComment  /#.*$/
syn match esCont /\\/
syn region esString start=/'/ skip=/\'\'/ end=/'/
syn match esVar /$[a-zA-Z0-9#^*][a-zA-Z0-9_-]*/
syn match esVarSet /[a-zA-Z][a-zA-Z0-9_-]*\s*=/
syn region esList start=/(/ end=/)/

hi def link esCmd Statement
hi def link eskeyList Statement
hi def link esHook Statement
hi def link esComment Comment
hi def link esCont Statement
hi def link esString String
hi def link esVar Identifier
hi def link esVarSet Identifier
hi def link esList Constant

