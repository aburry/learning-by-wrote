: ,                / append top of stack to dictionary
  a @ ! a @ 4 + a !
;


: i                / immediate
  1
  h @              / fetch the address of head of the list
  1 +              / adjust the address to the flag field
  !                / set the flag to 1
;


: [                / state := interpret
  1 s !            / set the state to 1 (interpret)
; i                / macro


: ]                / state := compile
  0 s !            / set the state to 0 (compile)
; i                / macro


: u                / append temporary jump address and push its location
  a @              / push the address of the next free dictionary cell
  0 ,              / append 0 as a placeholder, will set a proper address later
;


: t                / fix temporary jump address
  c                / copy the address of the goto we are fixing
  a @              / push the address of the next free dictionary cell
  | -              / calculate the jump amount (to - from)
  | !              / write the result
;


: ?                / if
  l b ,            / append a branch instruction
  u                / append a temporary branch address
; i                / macro


: e                / endif
  t                / fix last temporary goto address
; i                / macro


: o                / else
  l g ,            / append a goto instruction
  u                / append a temporary goto address
  |                / swap our new unknown with the one we are now ready to solve
  t                / fix temporary goto address
; i                / macro


: n                / convert word to number (w -- n)
  l 48             / ‘0’
  -                / convert
;


: t                / dup2 (n n -- n n n n)
  | c %            / (a b -- b a -- b a a -- a b a)
  | c %            / (a b a -- a a b -- a a b b -- a b a b)
;


: u                / compare words (w a -- n)
  t                / copy top two stack items
  l 2 + @          / fetch word from dict entry
  -                / compare
;



: f                / find (w a -- x)
  c b 13           / if not null
    u b 6          / if not found
      @            / fetch next
      g -8         / find
      g 5
    else
      l 3 + @      / fetch execution token
    end
  end
  | d              / drop word
end


: t                / interpret or compile word (x -- )
  s @ b 4          / if interpret
    x              / execute
    g 2
  else
    ,              / append to dict
  end
;


: u                / interpret or compile number (n -- )
  s @ b 3          / if interpret
    g 8
  else
    l 108 h @ f    / find ‘l’
    , ,            / append l n to dictionary
  end
;


: #                / eval (w -- )
  c h @ f          / find word in dict
  c b 6            / if found
    | d            / drop word
    t              / interpret or compile word
    g 4
  else
    d              / drop execution token
    n              / convert word to number
    u              / interpret or compile number
  end
;


: $                / print prompt ( -- )
  l 111 w          / ‘o’
  l 107 w          / ‘k’
  l 32 w           / ‘ ’
;


: t                / prompt state variable ( -- a)
  l address
;
/ storage space allocated here, initial value 1


: z                / repl ( -- )
  t @ b 6          / if prompt (should hand this in instead of using a variable)
    l 0 t !        / state := !prompt
    $              / prompt
  end
  r                / read
  c b 21           / if word
    c l 10 - b 3   / if !LF
      g 8
    else
      d            / drop LF
      l 1 t !      / state := prompt
      g -27        / recursive jump
    end
    #              / eval
    g -30          / recursive jump
    g 2
  else
    q              / quit
  end
;

