;;; ef-mode.el --- a major-mode for editing EF files in Emacs
;; 
;; Copyright 2010-2012 Florian Kaufmann <sensorflo@gmail.com>
;;
;; Author: Florian Kaufmann <sensorflo@gmail.com>
;; URL: https://github.com/sensorflo/ef
;; Created: 2014
;; Keywords: languages
;; 
;; This file is not part of GNU Emacs.
;; 
;; This program is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;; 
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;; 
;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.
;;
;;; Commentary:
;; 
;; Provides font-locking for EF program code. You probably want to add this to
;; your .emacs file:
;; 
;;   (add-to-list 'auto-mode-alist '("\\.ef$" . ef-mode))
;; 
;;; Variables
(defvar ef-mode-hook nil
  "Normal hook run when entering EF mode.")

;;; Code
(defconst ef-font-lock-keywords
  (list
   "\\b\\(fun\\|val\\|var\\|decl\\|nop\\|if\\|elif\\|else\\|unless\\|for\\|foreach\\|in\\|while\\|until\\|do\\|throws\\|ret\\|goto\\|break\\|continue\\|noinit\\|end\\|endof\\|raw_new\\|raw_delete\\|mi_end\\|true\\|false\\|then\\)\\b"
   (list (concat "\\bfun"                                       ; fun
                 "\\(?:[ \t\r\n]*($[ \t\r\n]*\\|[ \t\r\n]+\\)"  ; ($ or blank
                 "\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)\\b[ \t\r\n]*"  ; id
                 "\\(?::[ \t\r\n]*\\)?"                         ; :
                 "\\(?:([ \t\r\n]*"                             ; (
                   "\\(?:\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)[ \t\r\n]*:[ \t\r\n]*\\([^,()]*?\\),[ \t\r\n]*\\)?"
                   "\\(?:\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)[ \t\r\n]*:[ \t\r\n]*\\([^,()]*?\\),[ \t\r\n]*\\)?"
                   "\\(?:\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)[ \t\r\n]*:[ \t\r\n]*\\([^,()]*?\\),[ \t\r\n]*\\)?"
                   "\\(?:\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)[ \t\r\n]*:[ \t\r\n]*\\([^,()]*?\\),[ \t\r\n]*\\)?"
                   "\\(?:\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)[ \t\r\n]*:[ \t\r\n]*\\([^,()]*?\\)\\)?"
                 ")[ \t\r\n]*\\)?"           ; )
                 "\\([^,$()]*?\\)[ \t\r\n]*" ; rettype
                 "[,$]")                     ; ,= or $
         '(1 font-lock-function-name-face)
         '(2 font-lock-variable-name-face nil t) '(3 font-lock-type-face nil t)
         '(4 font-lock-variable-name-face nil t) '(5 font-lock-type-face nil t)
         '(6 font-lock-variable-name-face nil t) '(7 font-lock-type-face nil t)
         '(8 font-lock-variable-name-face nil t) '(9 font-lock-type-face nil t)
         '(10 font-lock-variable-name-face nil t) '(11 font-lock-type-face nil t)
         '(12 font-lock-type-face))
   (list "->[ \t\r\n]*\\(.*?\\)\\(?:is\\|throws\\|,=\\)"
         '(1 font-lock-type-face))
   (list (concat "\\_<\\(?:val\\|var\\)"                       ; val|var
                 "\\(?:[ \t\r\n]*($[ \t\r\n]*\\|[ \t\r\n]+\\)" ; ($ or blank
                 "\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)[ \t\r\n]*")   ; id
         '(1 font-lock-variable-name-face))
   (list (concat "\\_<\\(?:val\\|var\\)\\_>"                   ; val|var
                 "\\(?:[ \t\r\n]*($[ \t\r\n]*\\|[ \t\r\n]+\\)" ; ($ or blank
                 "[^$)]*?"
                 ":[ \t\r\n]*"
                 "\\(.*?\\)[ \t\r\n]*"
                 "\\(?:,=\\|[$)]\\|\\bend\\b\\|\\bendof\\b\\)")
         '(1 font-lock-type-face nil t))
   (list (concat "\\_<raw_new\\_>[ \t\n]*"
                 "\\(?:([ \t\n]*\\)?"
                 "\\(.*?\\)"
                 "\\(?:,=\\|(=\\|\\$\\)")
         '(1 font-lock-type-face))
   (list "\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)[ \t\r\n]*:[ \t\r\n]*="
         '(1 font-lock-variable-name-face))
   ;; the ones above need to put protecting text properties over what they
   ;; match, also over things like separators like = : they don't fontify, so
   ;; these separtors are 'eaten' and not used/matched again.
   ;; (list (concat "\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)[ \t\r\n]*"
   ;;               ":[ \t\r\n]*"
   ;;               "\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)")
   ;;       '(1 font-lock-variable-name-face)
   ;;       '(2 font-lock-type-face))
   (list (concat
   "\\bdecl[ \t\r\n]+"
                 "\\([a-zA-Z0-9][a-zA-Z0-9_]*\\)[ \t\r\n]*"
                 ":[ \t\r\n]*"
                 "fun\\b")
         '(1 font-lock-function-name-face))
   (list "\\([;:\\]\\|,=\\)[ \t]*\\(?://\\|#!\\|$\\)" '(1 font-lock-semi-unimportant))
   (list "\\(?:^\\|[^(]\\)\\(\\$\\)[ \t]*\\(//\\|#!\\|$\\)" '(1 font-lock-semi-unimportant))
   (list "\\(?:^\\|[^a-zA-Z_]\\)\\([0-9]+\\(?:\\.[0-9]*\\)?\\)"
         '(1 font-lock-constant-face))))

(defconst ef-font-lock-syntactic-keywords
  (list
   (list "\\(#\\)!" '(1 "<"))))

(defconst ef-open-keywords
  '("fun" "decl" "if" "while"))

;;;###autoload
(define-derived-mode ef-mode prog-mode "EF"
  "Major mode for editing EF files.
Turning on EF mode runs the normal hook `ef-mode-hook'."
  (interactive)
  
  ;; syntax table
  (modify-syntax-entry ?\" "'")
  (modify-syntax-entry ?\" "\"")
  (modify-syntax-entry ?/ ". 124")
  (modify-syntax-entry ?* ". 23b")
  (modify-syntax-entry ?\n ">")
  (modify-syntax-entry ?\r ">")
  (modify-syntax-entry ?_ "_")
  (modify-syntax-entry ?$ ".")
  (set (make-local-variable 'parse-sexp-lookup-properties) t)
  (set (make-local-variable 'syntax-propertize-function)
       (syntax-propertize-via-font-lock ef-font-lock-syntactic-keywords))
  
  ;; comments
  (set (make-local-variable 'comment-column) 0)
  (set (make-local-variable 'comment-start) "// ")
  (set (make-local-variable 'comment-end) "")
  (set (make-local-variable 'comment-start-skip) "\\(?:/\\*+\\|//\\|#!\\)\\s-*")
  (set (make-local-variable 'comment-end-skip) "\\s-*\\(?:[\n\r]\\|\\*+/\\)")
  
  ;; font lock
  (set (make-local-variable 'font-lock-defaults) '(ef-font-lock-keywords))
  
  ;; 
  (run-hooks 'ef-mode-hook))

(provide 'ef-mode)

;;; ef-mode.el ends here
