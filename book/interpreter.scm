;;; Interpreter from the Appendix C with bits of text added as
;;; comments.  The order of defines is shuffled around a bit, to group
;;; related things.

;;; Help functions for detecting the different kinds of expressions
(define constant?  (lambda (x) (and (pair? x) (eq? (car x) 'constant))))
(define parameter? (lambda (x) (and (pair? x) (eq? (car x) 'parameter))))
(define typed?     (lambda (x) (and (pair? x) (eq? (car x) 'typed))))
(define var?       (lambda (x) (and (pair? x) (eq? (car x) 'var))))
(define term?      (lambda (x) (and (pair? x) (eq? (car x) 'term))))
(define isis?      (lambda (x) (and (pair? x) (eq? (car x) 'is))))


;;; A rule is a vector containing a head expression, a body
;;; expression, and an optional tag.  If a rule does not contain a
;;; tag, then #f is returned instead.
(define head (lambda (x) (vector-ref x 0))) ; head of rule
(define body (lambda (x) (vector-ref x 1))) ; body of rule
(define tag                                 ; tag of rule
  (lambda (x)
    (if (=? (vector-length x) 3)
        (vector-ref x 2)
        #f)))     ; return false if no tag


;;; A state is a four-tuple, containing a subject expression, a global
;;; name space, a global type space, and an integer.  The integer is
;;; used for generating label names for unlabeled redexes (the newname
;;; function from Sections 3.2.2 and B.2.2).
(define make-state (lambda (s g t n) (vector s g t n)))
(define subject (lambda (x) (vector-ref x 0)))
(define globals (lambda (x) (vector-ref x 1)))
(define typesp (lambda (x) (vector-ref x 2)))
(define newname (lambda (x) (vector-ref x 3)))


;;; Functions that take a state and return a new state with one of the
;;; elements updated.

(define replace-s       ; replace subject expression in state
  (lambda (state new-subject)
    (vector new-subject
            (globals state)
            (typesp state)
            (newname state))))

(define replace-g       ; replace globals in state
  (lambda (state new-globals)
    (vector (subject state)
            new-globals
            (typesp state)
            (newname state))))

(define replace-t       ; replace type space in state
  (lambda (state new-typesp)
    (vector (subject state)
            (globals state)
            new-typesp
            (newname state))))

(define incr-n          ; increment label generator in state
  (lambda (state)
    (vector (subject state)
            (globals state)
            (typesp state)
            (+ 1 (newname state)))))


;;; The empty name space.
(define init-phi '((*reserved* . *reserved*)))

;;; Add a name-value pair onto the beginning of the name space list.
(define bind
  (lambda (var val name-space)
    (cons (cons var val) name-space)))

;;; Search the name space list for the specified name.
(define lookup
  (lambda (var name-space)
    (let ((entry (assoc var name-space)))
      (if entry
          (cdr entry)
          #f))))


;;; The constant "true" (nullary operator).
(define true-expr '(term (:) true))

;;; Skip over the first three elements of a list representing an
;;; expression (the symbol 'term, the label, and the operator) when
;;; calling rewrite-args.
(define first3
  (lambda (alist)
    (list (car alist) (cadr alist) (caddr alist))))


;;; The main function of the augmented term rewriter takes a subject
;;; expression and a list of rules, constructs a state, and passes the
;;; initial state and the rules to the rewriter.
(define augmented-term-rewriter
  (lambda (subject-exp rules)
    (rewrite
     (make-state        ; state
      subject-exp       ; subject expression
      init-phi          ; initial global name space
      init-phi          ; initial type space
      0)                ; initial generated label name
     rules)))           ; rules


;;; Take a state and a list of rules, and return a new state
;;; containing the completely rewritten subject expression, a global
;;; name space containing all of the bound variables and their values,
;;; a type space containing all the typed variables and their types,
;;; and an integer that indicates how many label names were generated.
(define rewrite
  (lambda (state rules)
    (let ((no-bv-state (rewrite-globals state)))
      (if no-bv-state                   ; bound var was found
          (rewrite no-bv-state rules)
          (let ((new-state (rewrite-exp state rules rules)))
            (if new-state               ; match (or "is") found
                (rewrite new-state rules)
                state))))))

(define rewrite-exp
  (lambda (state rules-left-to-try rules)
    (if (null? rules-left-to-try)
        ;; when the list of rules is empty, then the outermost term
        ;; has failed to match any rule in rules, so call
        ;; rewrite-subexpressions with the original list of rules.
        (rewrite-subexpressions state rules)

        ;; attempt to match the outermost term of the subject
        ;; expression against the first rule in rules.
        (let ((new-state (try-rule
                          state
                          (car rules-left-to-try))))
          (if new-state
              new-state
              ;; if try-rule above failed, recursively call ourselves,
              ;; removing the head of the list of rules.
              (rewrite-exp state
                           (cdr rules-left-to-try)
                           rules))))))

(define rewrite-subexpressions
  (lambda (state rules)
    (let ((expr (subject state)))
      (cond ((constant? expr) #f)
            ((var? expr) #f)
            ;; if the outermost term is an operator with arguments,
            ;; call rewrite recursively on each of the arguments
            ((term? expr)
             (rewrite-args (first3 expr)
                           (cdddr expr)
                           state
                           rules))
            ((isis? expr) (rewrite-is state))
            (else (error "Invalid subject expression:"
                         expr))))))


(define rewrite-args
  (lambda (previous-terms terms-to-try state rules)
    (if (null? terms-to-try)
        #f
        (let ((new-state (rewrite-exp
                          (replace-s state (car terms-to-try))
                          rules rules)))
          (if new-state
              ;; argument was a redex - that transformed argument is
              ;; reinserted into the subject expression in the state
              (replace-s
               new-state
               (append previous-terms
                       (cons (subject new-state)
                             (cdr terms-to-try))))
              ;; loop: next arg
              (rewrite-args
               (append previous-terms
                       (list (car terms-to-try)))
               (cdr terms-to-try) state rules))))))

(define rewrite-is
  (lambda (state)
    (let ((expr (subject state))
          (space (globals state)))
      (if (and (pair? (cdr expr))       ; two args?
               (var? (cadr expr))       ; first is var?
               (pair? (cddr expr))      ; second is expr?
               (not (lookup (cdadr expr)
                            space))     ; var not bound?
               (not (rewrite-globals    ; var not in expr?
                     (make-state (caddr expr)
                                 (bind (cdadr expr)
                                       '()
                                       init-phi)
                                 init-phi 0))))
          ;; The new global name space is the old global name space
          ;; with the addition of a new name/value pair for the new
          ;; bound variable.  The new subject expression is the
          ;; nullary operator (constant) true.
          (replace-g (replace-s state true-expr)
                     (bind (cdadr expr) (caddr expr) space))
          (error "invalid \"is\" expression:" expr)))))


;;; Try to match the head of the rule against the subject expression
;;; in the state.
(define try-rule
  (lambda (state rule)
    (let ((phi (match state (head rule) init-phi)))
      (if phi
          (let ((label (get-label (subject state)
                                  (newname state))))
            (replace-s
             (bind-type
              (if (eq? (last label) (newname state))
                  (incr-n state)
                  state)
              rule label)
             (transform (body rule) phi label)))
          #f))))

(define match
  (lambda (state pattern phi)
    (let ((expr (subject state)))
      (cond
       ((parameter? pattern) (bind (cadr pattern) expr phi))
       ((and (typed? pattern) (var? expr))
        (let ((var-type (lookup (cdr expr) (typesp state))))
          (if (and var-type
                   (memq var-type (cddr pattern)))
              (bind (cadr pattern) expr phi)
              #f)))
       ((and (typed? pattern) (constant? expr)
             (eq? (caddr pattern) 'constant))
        (bind (cadr pattern) expr phi))
       ((and (constant? pattern) (constant? expr)
             (=? (cdr pattern) (cdr expr))) phi)
       ((and (term? pattern) (term? expr)
             (eq? (caddr pattern) (caddr expr)))
        (match-args (replace-s state (cdddr expr))
                    (cdddr pattern) phi))
       ((var? pattern)
        (error "Local variable in head of rule"))
       (else #f)))))

(define match-args
  (lambda (state patterns phi)
    (let ((args (subject state)))
      (cond
       ((and (null? args) (null? patterns)) phi)
       ((null? args) #f)
       ((null? patterns) #f)
       (else
        (let ((new-phi (match (replace-s state (car args))
                              (car patterns) phi)))
          (if new-phi
              (match-args (replace-s state (cdr args))
                          (cdr patterns) new-phi)
              #f)))))))


;;; Get-label checks to see if the last element of the label of the
;;; matched expression is a colon, and if so replaces it with a
;;; generated name, which is simply a number (the user is not allowed
;;; to use numbers for labels, so there can be no conflict from
;;; generated labels).
(define get-label
  (lambda (expr lgen)
    (if (eq? (last (cadr expr)) ':)
        (replace-last (cadr expr) lgen)
        (cadr expr))))

;;; return the last element of a proper list
(define last
  (lambda (lst)
    (if (pair? lst)
        (if (null? (cdr lst))
            (car lst)
            (last (cdr lst)))
        (error "Cannot return last element of atom:" lst))))

;;; replace the last element of a list
(define replace-last
  (lambda (lst val)
    (if (and (pair? lst) (null? (cdr lst)))
        (list val)
        (cons (car lst) (replace-last (cdr lst) val)))))


;;; Bind a type to the label in the type space if the rule was tagged
;;; (even if the label was generated).
(define bind-type
  (lambda (state rule label)
    (let ((rule-tag (tag rule)))
      (if rule-tag
          (replace-t state
                     (bind label rule-tag (typesp state)))
          state))))


;;; Take the body of the matched rule, a parameter name space, and a
;;; label, and return a transformed expression.
(define transform
  (lambda (rule-body phi label)
    (cond
     ((parameter? rule-body)
      (let ((param-val (lookup (cadr rule-body) phi)))
        (if param-val
            (if (=? (length (cdr rule-body)) 1)
                param-val               ; not qualified parameter
                (if (var? param-val)
                    (cons (car param-val)
                          (append (cdr param-val)
                                  (cddr rule-body)))
                    (error
                     "A qualified parameter "
                     "matched a non-variable:"
                     param-val)))
            (error "Parameter in body that is not in head:"
                   rule-body))))
     ((var? rule-body)
      (cons (car rule-body) (append label (cdr rule-body))))
     ((constant? rule-body) rule-body)
     ((term? rule-body)
      (append (list
               (car rule-body)          ; 'term
               (append label (cadr rule-body))
               (caddr rule-body))
              (transform-args (cdddr rule-body) phi label)))
     ((isis? rule-body)
      (cons (car rule-body)
            (transform-args (cdr rule-body) phi label)))
     (else (error "Invalid body of rule:" rule-body)))))

(define transform-args
  (lambda (args phi label)
    (if (null? args)
        '()
        (cons (transform (car args) phi label)
              (transform-args (cdr args) phi label)))))


;;; Replace bound variables by their values.
(define rewrite-globals
  (lambda (state)
    (let ((expr (subject state))
          (space (globals state)))
      (cond
       ((var? expr)
        (let ((val (lookup (cdr expr) (globals state))))
          (if val                       ; variable is bound
              (replace-s state val)     ; replace by value
              #f)))
       ((constant? expr) #f)
       ((term? expr)
        (rewrite-g-args (first3 expr) (cdddr expr) state))
       ((isis? expr)
        (rewrite-g-args (list (car expr)) (cdr expr) state))
       (else (error "invalid subject expression:" expr))))))

(define rewrite-g-args
  (lambda (previous-terms terms state)
    (if (null? terms)
        #f
        (let ((new-state (rewrite-globals
                          (replace-s state (car terms)))))
          (if new-state
              (replace-s new-state
                         (append previous-terms
                                 (cons (subject new-state)
                                       (cdr terms))))
              (rewrite-g-args
               (append previous-terms (list (car terms)))
               (cdr terms) state))))))


;; smoke test...
;; (augmented-term-rewriter '(is (var x) (constant . 5)) '())
