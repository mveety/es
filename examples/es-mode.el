(defvar es-mode-hook nil)
(defvar es-mode-map
  (let ((es-mode-map (make-keymap)))
	(define-key es-mode-map "\C-j" 'newline-and-indent)
	es-mode-map)
    "Keymap for es major mode")

(add-to-list 'auto-mode-alist '("\\.es\\'" . es-mode))

(defvar es-font-lock-keywords
  '(("\\<\\(let\\|local\\|lets\\|if\\|for\\|while\\|fn\\|%closure\\|match\\|matchall\\|process\\|%dict\\|%re\\|onerror\\)\\>" . 'font-lock-keyword-face)
    ("\\<\\(access\\|break\\|catch\\|cd\\|echo\\|eval\\|exec\\|exit\\|false\\|forever\\|fork\\|if\\|limit\\|newpgrp\\|result\\|return\\|throw\\|time\\|true\\|umask\\|unwind-protect\\|var\\|vars\\|wait\\|whats\\|while\\|add\\|sub\\|mul\\|div\\|mod\\|lt\\|gt\\|lte\\|gte\\|eq\\|tobase\\|frombase\\|reverse\\|waitfor\\|assert\\|assert2\\|gensym\\|apply\\|bqmap\\|fbqmap\\|map\\|macro\\|import\\|library\\|whatis\\|panic\\|option\\|try\\|iterator\\|iterator2\\|do\\|do2\\|accumulate\\|accumulate2\\|dolist\\|glob\\|parseargs\\|makeerror\\|errmatch\\|iserror\\|continue\\|dictnew\\|dictget\\|dictput\\|dictremove\\|dictsize\\|dictforall\\|dictnames\\|dictvalues\\|dictdump\\|dictiter\\|conf\\|box\\|defconf\\|defconfalias\\|dictcopy\\|dicthaskey\\|dictkeys\\)\\>
" . 'font-lock-builtin-face)
    ("'[^']*'" . 'font-lock-string-face)
    ;;("`{[^}]*}" . 'font-lock-variable-name-face)
    ("\\<-\\w*\\>" . 'font-lock-reference-face)
    ("\$\\w*" . 'font-lock-reference-face)
    ))

(defvar es-mode-syntax-table
  (let ((es-mode-syntax-table (make-syntax-table)))
    (modify-syntax-entry ?_ "w" es-mode-syntax-table)
    (modify-syntax-entry ?- "w" es-mode-syntax-table)
    (modify-syntax-entry ?. "w" es-mode-syntax-table)
    (modify-syntax-entry ?' "\"" es-mode-syntax-table)
    (modify-syntax-entry ?\" "." es-mode-syntax-table)
    (modify-syntax-entry ?\n ">" es-mode-syntax-table)
    (modify-syntax-entry ?# "<" es-mode-syntax-table)
    (modify-syntax-entry ?$ "/" es-mode-syntax-table)
    es-mode-syntax-table)
  "Syntax table for es-mode")

(defun es-indent-line ()
  "Indent current line as es code"
  (interactive)
  (beginning-of-line))

(define-derived-mode es-mode fundamental-mode "es"
  "Major mode for editing Extensible Shell scripts."
;  (setq comment-start "#") 
;  (setq comment-end "")
  (setq string-start nil)
  (setq string-end nil)
  (set (make-local-variable 'font-lock-defaults) '(es-font-lock-keywords))
  (set (make-local-variable 'indent-line-function) 'es-indent-line))

(provide 'es-mode)
