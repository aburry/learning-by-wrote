; built-ins: cons head tail lambda eq? if symbol? quote define

; cons :: Data -> List -> List
; head :: List -> Data
; tail :: List -> Data
; lambda :: List -> Data -> Function
; eq? :: Data -> Data -> Bool
; if :: Bool -> Data -> Data -> Data
; symbol? :: Data -> Bool
; quote :: Data -> Data
; define :: Symbol -> Data -> Void
; #f :: Bool
; #t :: Bool
; () :: List


(define test (lambda (name expected actual)
  (cons (eq? expected actual) (cons name ()))))

(define car head)
(define cdr tail)

(test 'head-1 'a (head (cons 'a ())))
(test 'car-1 'a (car (cons 'a ())))
(test 'tail-1 () (tail (cons 'a ())))
(test 'cdr-1 () (cdr (cons 'a ())))
(test 'if-1 'a (if #t 'a 'b))
(test 'if-2 'b (if #f 'a 'b))

(define boolean? (lambda (x)
  (if (eq? x #t) #t (if (eq? x #f) #t #f))))

(test 'boolean?-1 #t (boolean? #f))
(test 'boolean?-2 #t (boolean? #t))
(test 'boolean?-3 #f (boolean? 'a))
(test 'boolean?-4 #f (boolean? '(a s d f)))

(define not (lambda (x)
    (if x #f #t)))

(test 'not-1 #t (not (not #t)))
(test 'not-2 #f (not (not #f)))

;(define list? (lambda (x)
;  (not (symbol? x))))

(test 'list?-1 #f (list? 'a))
(test 'list?-2 #f (list? #f))
(test 'list?-3 #t (list? ()))
(test 'list?-4 #t (list? '(a s d f)))
(test 'list?-5 #f (list? (lambda (x) x)))

(define and (lambda (x y)
  (if (eq? x #t) (eq? y #t) #f)))

(test 'and-1 #t (and #t #t))
(test 'and-2 #f (and #t #f))
(test 'and-3 #f (and #f #t))
(test 'and-4 #f (and #f #f))
(test 'and-5 #f (and 'a 'a))

(define or (lambda (x y)
  (if (eq? x #f) (if (eq? x y) #f #t) #t)))

(test 'or-1 #t (or #t #t))
(test 'or-2 #t (or #t #f))
(test 'or-3 #t (or #f #t))
(test 'or-4 #f (or #f #f))

(define equal? (lambda (x y)
  (if (eq? x y) #t
  (if (and
        (and (list? x) (list? y))
        (and (not (eq? x ())) (not (eq? y ()))))
    (and (equal? (head x) (head y)) (equal? (tail x) (tail y)))
    #f))))

(test 'equal?-1 #t (equal? 'a 'a))
(test 'equal?-2 #t (equal? () ()))
(test 'equal?-3 #f (equal? 'a ()))
(test 'equal?-4 #t (equal? '(a) '(a)))
(test 'equal?-5 #f (equal? '(a) ()))
(test 'equal?-6 #f (equal? () '(a)))
(test 'equal?-7 #t (equal? '((a)) '((a))))
(test 'equal?-8 #f (equal? '(a) '(b)))
(test 'equal?-9 #f (equal? '((a c)) '(b)))

; redefine test in terms of equal?
(define test (lambda (name expected actual)
  (cons (equal? expected actual) (cons name ()))))

(define map (lambda (f l)
  (if (eq? l ()) ()
    (cons (f (head l)) (map f (tail l))))))

(test 'map-1 () (map (lambda (x) (cons x ())) ()))
(test 'map-2 '((a)(b)(c)(d)) (map (lambda (x) (cons x ())) '(a b c d)))

(define append (lambda (x y)
  (if (eq? x ()) y
    (cons (head x) (append (tail x) y)))))

(test 'append-1 () (append () ()))
(test 'append-2 '(a) (append '(a) ()))
(test 'append-3 '(a) (append () '(a)))
(test 'append-4 '(a a) (append '(a) '(a)))

; (define append (lambda (x y a)
;   (if (and (eq? x ()) (eq? a ())) y
;   (if (eq? x ()) (append x (cons (head a) y) (tail a))
;     (append (tail x) y (cons (head x) a))))))
; 
; (test 'append-5 () (append () () ()))
; (test 'append-6 '(a) (append '(a) () ()))
; (test 'append-7 '(a) (append () '(a) ()))
; (test 'append-8 '(a a) (append '(a) '(a) ()))

(define reverse (lambda (x)
  (if (eq? () x) ()
    (append (reverse (tail x)) (cons (head x) ())))))

(test 'reverse-1 () (reverse ()))
(test 'reverse-2 '(d c b a) (reverse '(a b c d)))

(define zip (lambda (x y)
  (if (eq? x ()) ()
    (cons (cons (head x) (cons (head y) ())) (zip (tail x) (tail y))))))

(test 'zip-1 '((a a) (b b) (c c)) (zip '(a b c) '(a b c)))
; reverse distributes over zip
(test 'zip-2 (reverse (zip '(a b c) '(a b c))) (zip (reverse '(a b c)) (reverse '(a b c)))) 

(define member? (lambda (x l)
  (if (eq? l ()) #f
  (if (eq? x (head l)) #t
    (member? x (tail l))))))

(test 'member?-1 #f (member? 'a ()))
(test 'member?-2 #f (member? 'a '(b c)))
(test 'member?-3 #t (member? 'a '(b c a)))

(define pair? (lambda (x)
  (and (list? x) (not (eq? x ())))))

(define var? (lambda (x)
  (member? x '(X Y Z))))

(test 'var?-1 #t (var? 'X))
(test 'var?-2 #f (var? 'NOT))

(define assoc (lambda (x y)
  (if (eq? y ()) #f
  (if (eq? x (car (car y))) (car y)
    (assoc x (cdr y))))))

(test 'assoc-1 '(x a) (assoc 'x '((x a))))
(test 'assoc-2 '(x a) (assoc 'x '((m n) (x a))))
(test 'assoc-3 '#f (assoc 'x ()))

(define bound? (lambda (x subst)
  (if (eq? (assoc x subst) #f) #f #t)))

(test 'bound?-1 #f (bound? 'Y '((X a))))
(test 'bound?-2 #f (bound? 'a '((X a))))
(test 'bound?-3 #t (bound? 'X '((X a))))
(test 'bound?-4 #f (bound? 'Y ()))

(define lookup (lambda (var subst)
  (car (cdr (assoc var subst)))))

(test 'lookup-1 'a (lookup 'X '((X a))))

(define extend-subst (lambda (var val subst)
  (cons (cons var (cons val ())) subst)))

(test 'extend-subst-1 '((X z)) (extend-subst 'X 'z ()))

(define occurs-in? (lambda (var x subst)
  ; "Does var occur anywhere inside x?"
  (if (equal? var x) #t
  (if (bound? x subst) (occurs-in? var (lookup x subst) subst)
  (if (pair? x) (or
        (occurs-in? var (car x) subst)
        (occurs-in? var (cdr x) subst))
    #f)))))

(test 'occurs-in?-1 #t (occurs-in? 'X 'X ()))
(test 'occurs-in?-2 #f (occurs-in? 'X 'a '((X a))))
(test 'occurs-in?-3 #t (occurs-in? 'X '(a X) '((X a))))

(define unify (lambda (x y subst)
  (if (equal? x y) subst
  (if (equal? subst #f) #f
  (if (var? x) (unify-variable x y subst)
  (if (var? y) (unify-variable y x subst)
  (if (or (symbol? x) (symbol? y)) #f
    (unify (cdr x) (cdr y) (unify (car x) (car y) subst)))))))))

(define unify-variable (lambda (var val subst)
  ; "Unify var with val, using (and possibly extending) subst."
  (if (equal? var val) subst
  (if (bound? var subst) (unify (lookup var subst) val subst)
  (if (and (var? val) (bound? val subst)) (unify var (lookup val subst) subst)
  (if (occurs-in? var val subst) #f
    (extend-subst var val subst)))))))

(test 'unify-variable-1 '((X a)) (unify-variable 'X 'a ()))
(test 'unify-variable-2 #f (unify-variable 'X 'a '((X b))))

(test 'unify-1 () (unify '(a b) '(a b) ()))
(test 'unify-2 '((Y  a)) (unify '(Y b) '(a b) ()))
(test 'unify-3 '((X  b) (Y  a)) (unify '(Y b) '(a X) ()))
(test 'unify-4 '((X Y)) (unify 'X 'Y ()))
(test 'unify-5 '((X  Y)) (unify '(p X Y) '(p Y X) ()))
(test 'unify-6 '((X  Y) (Z (p X Y))) (unify '(q (p X Y) (p Y X)) '(q Z Z) ()))
(test 'unify-7 '((Y  a) (X Y)) (unify '(p X Y a) '(p Y X X) ()))

(define types '(
  (cons (Data List List))
  (head (List Data))
  (tail (List Data))
  (lambda (List Data Function))
  (eq? (Data Data Bool))
  (if (Bool Data Data Data))
  (symbol? (Data Bool))
  (quote (Data Data))
  (define (Symbol Data Void))
  (#f Bool)
  (#t Bool)
  (() List)
))

(unify '(implies X Y) '(implies (and a b) (eq? a #t)) ())

'(define test (lambda (name expected actual)
  (cons (eq? expected actual) (cons name ()))))
'(test (Symbol Data Data List))
'(test Symbol)

(define type (lambda (x)
  (if (not (eq? #f (assoc x types))) (car (cdr (assoc x types)))
  (if (symbol? x) 'Symbol
    x))))
;  (if (eq? (car x) 'lambda) 'Function
;    'List)))))

test
(type 'test)
(test 'type-1 'Symbol (type 'test))
(type test)
(test 'type-2 'Function (type test))
(test 'type-3 'Bool (type #f))
(test 'type-4 'Bool (type '#f))
(test 'type-5 'List (type ()))
