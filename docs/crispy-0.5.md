# crispy 0.5

This document specifies the *crispy* programming language 0.5 and its compiler.

## New features

* if-else statement
* while statement
* function arguments
* unary + and -
* comparison operators (non-chaining) == != <= >= < >
* escape sequences \n \t \" and slash

## Bug fixes

* constructing an array of multiple arrays as items does not work:
  the garbage collector picks up the first array item before allocating the
  second array item, e.g.:
  `print [[1], [2]];`
  prints out `[[1], []]`

## Compiler

The `crispy` compiler program translates *crispy* module files to C source
files. It takes the path to a module file and the path to the C output source
file on the command line and then compiles it.

Command line usage:

```
crispy <path-to-source-file> <path-to-c-output-file>
```

## Language

Module files are written in the `crispy` programming language.

### Lexis

The relevant tokens are:

* variable identifiers (`IDENT`)
* keywords (`KEYWORD`)
* integer literals (`INT`)
* string literals (`STRING`)
* punctuators (`PUNCT`)

```
IDENT = [a-zA-Z_] [a-zA-Z_0-9]* ;
IDENT != KEYWORD ;

KEYWORD = [a-zA-Z_] [a-zA-Z_0-9]* & (
	"var" | "print" | "true" | "false" | "null" | "function" | "return"
) ;

INT = decint | hexint | binint ;
decint = [0-9]+ ;
hexint = "0x" [0-9a-fA-F]* ;
binint = "0b" [01]* ;

STRING = ["] strchar ["] ;
strchar = [^\0-\x1f"\\] | escseq ;
escseq = "\\" [nt"\\] ;

PUNCT =
	"==" | "!=" | "<=" | ">=" | "<" | ">" |
	";" | "=" | "(" | ")" | "{" | "}" | "+" | "-" | "*" | "%" | "[" | "]" |
	"," ;

-WHITE = [ \t\n\v\f\r]+ ;

-COMMENT = "#" [^\n]* ;

-MLCOMMENT = "/*" mlchar "*/" ;
mlchar = [^*] | "*" [^/] ;
```

### Grammar

```
module = stmt* ;

stmt = vardecl | assign | print | funcdecl | call | return | if | while ;
vardecl = "var" IDENT ( "=" expr )? ";" ;
assign = expr¹ "=" expr² ";" ;
print = "print" expr ";" ;
funcdecl = "function" IDENT "(" params? ")" "{" stmt* "}" ;
params = IDENT ( "," IDENT )* ;
call = postfix call_x ;
return = "return" expr? ";" ;
if = "if" expr "{" stmt* "}" else? ;
else = "else" "{" stmt* "}" ;
while = "while" expr "{" stmt* "}" ;

expr = binop ;
binop = cmpop ;
cmpop = addop ( cmp addop )? ;
cmp = "==" | "!=" | "<=" | ">=" | "<" | ">" ;
addop = mulop ( [+-] mulop )* ;
mulop = unary ( [*%] unary )* ;
unary = [+-] unary | postfix ;
postfix = atom postfix_x* ;
postfix_x = array_index | call_x ;
array_index = "[" expr "]" ;
call_x = "(" ")" ;
atom = INT | IDENT | STRING | "true" | "false" | "null" | array ;
array = "[" expr_list? "]" ;
expr_list = expr ( "," expr )* ;
```

### Module

A `module` is a sequence of statements.

### Statement

A statement (`stmt`) is one of these:

* variable declaration (`vardecl`)
* assignment (`assign`)
* `print` statement
* function declaration (`funcdecl`)
* function `call`
* `return` statement
* `if` statement
* `while` statement

A variable declaration introduces a new variable `IDENT` and optionally assigns
an initial value `expr` to it. The same variable `x` is not allowed to be
redeclared in the same scope. A variable is not bound to a fixed type over its
lifetime. If the initial value is omitted then the variable is initially `null`
by default.

An assignment stores a new value `expr²` into a target `expr¹` which must be a
variable name `IDENT` or an array item `expr[index]`. If the target contains a
variable then it must be declared by a previous corresponding `vardecl`, either
in the same scope or in an enclosing outer scope.

A print statement writes a value `expr` to the standard output stream. Integers
are printed in decimal form. Arrays are printed in the same syntax they appear
in the `crispy` language. If the array directly or indirectly contains a
reference to itself then the reference is just printed as `[...]` to avoid
infinite recursion. The whole value is printed with a newline character at the
end (`\n`).

A function declaration introduces a new function `IDENT` with its statements
`stmt*` enclosed in `{` and `}`. This is the function body. The function body
introduces a new scope which has the top level scope as its parent. Functions
can be declared in the top level scope or another functions scope. When
declared inside another function it can not access the local variables of the
outer function, only top level variables.

After the functions name a comma-separated list of parameters (`params`) might
be declared inside `(` and `)`. These are declared inside the local scope of
the function like its local variables.

A function `IDENT` is basically just a variable pointing to a function object.
Therefore it could be reassigned to a different value or it could be passed to
another variable or array item.

A function might refer to a variable declared in the top level scope. The
variable might even be declared further down in the source code but then the
function must be called after the variable declaration has been executed,
otherwise it is an error.

A `call` invokes a function referenced by an expression `postfix`. If the
expression is not storing a reference to a function object it is not allowed to
be called. The `call` expects a list of argument expressions inside `(` and `)`
which must be the same number as the parameters the function has defined.

A `return` statement can only be used inside a function body. It terminates the
function and optionally lets the it return a value `expr`. If the return value
is omitted the return value is implicitly `null`.

The `if` and `while` statements work just like you know them from other
languages. They both expect an expression that is tested for truthiness to
continue execute. `if` can have an optional `else` part but there is no
`else if` or `elif` syntax.

### Expression

An expression (`expr`) is one of these:

* a decimal integer literal (`decint`)
* a hexadecimal integer literal (`hexint`)
* a binary integer literal (`binint`)
* a string literal (`STRING`)
* a variable identifier (`IDENT`)
* a boolean literal (`true` or `false`)
* the `null` value
* a function call
* a binary operation (`binop`)
* an unary operation (`unary`)
* an `array` literal
* an array subscript `expr[index]`

A binary operation combines two or more values with operators.

| precedence | description | operators |
| --- | --- | --- |
| 1 | multiplication, modulo | `*` `%` |
| 2 | addition, subtraction | `+` `-` |
| 3 | comparison | `==` `!=` `<=` `>=` `<` `>` |

Operators `+`, `-`, `*` and `%` result in integer values. Comparison operators
result in booleans.

Comparison operations can not be chained: e.g.: `a < b == c` is not allowed.

All operators have left-to-right associativity and are also evaluated from left
to right.

All operands can be integers, booleans or even `null`. `null` is interpreted
as `0`, `true` as `1` and `false` as `0`.

A call expression is just like a call statement but evaluated to its return
value.

An `array` literal constructs a new array object with a fixed length of
arbitrary values.

An array subscript `expr[index]` accesses an item with the number `index` from
an array value `expr`. The `index` must evaluate to an integer value in the
range of 0 and the length of the array - 1. The array subscript can be used as
a target value of an assignment.

### Types

`crispy` has these types:

* `int` - 64 bit signed integer
* `bool` - boolean value
* `null` - the `null` type
* `function` - a reference to a function object
* `array` - a fixed-length sequence of mutable values

Arrays are objects which are passed by reference: e.g. assigning an existing
array stored in a variable `y` to a variable `x` does not create a new array.
Instead `y` is then referring to the same array as `x` and changes to the items
in `x` can be seen in `y` automatically and vice versa.
