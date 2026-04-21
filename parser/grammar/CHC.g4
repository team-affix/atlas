grammar CHC;

database
    : clause* EOF
    ;

clause
    : expr '.'
    | expr ':-' body '.'
    ;

body
    : expr (',' expr)*
    ;

expr
    : functor
    | VARIABLE
    ;

functor
    : ATOM ('(' (expr (',' expr)*)? ')')?
    | list
    ;

list
    : '[' ']'
    | '[' expr (',' expr)* ('|' expr)? ']'
    ;

ATOM     : [a-z][a-zA-Z0-9_]*
         | [0-9]+
         | '\'' ( ~['\\] | '\\' . )* '\''
         ;

VARIABLE : [A-Z_][a-zA-Z0-9_]*
         ;

COMMENT       : '%' ~[\r\n]* -> skip ;
BLOCK_COMMENT : '/*' .*? '*/' -> skip ;
WS            : [ \t\r\n]+ -> skip ;
