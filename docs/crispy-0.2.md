# crispy 0.2

This document specifies the *crispy* programming language 0.2 and its compiler.

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
* punctuators (`PUNCT`)

```
IDENT = [a-zA-Z_] [a-zA-Z_0-9]* ;
IDENT != KEYWORD ;

KEYWORD = [a-zA-Z_] [a-zA-Z_0-9]* & (
	"var" | "print" | "true" | "false" | "null" | "function"
) ;

INT = decint | hexint | binint;
decint = [0-9]+ ;
hexint = "0x" [0-9a-fA-F]* ;
binint = "0b" [01]* ;

PUNCT = ";" | "=" | "(" | ")" | "{" | "}" | "+" | "-" ;

-WHITE = [ \t\n\v\f\r]+ ;

-COMMENT = "#" [^\n]* ;
```

### Grammar

```
module = stmt* ;
stmt = vardecl | assign | print | funcdecl | call;
vardecl = "var" IDENT ( "=" expr )? ";" ;
assign = IDENT "=" expr ";" ;
print = "print" expr ";" ;
funcdecl = "function" IDENT "(" ")" "{" stmt* "}" ;
call = IDENT "(" ")" ;
expr = binop | atom ;
binop = atom ( op atom )* ;
op = "+" | "-" ;
atom = INT | IDENT | "true" | "false" | "null" ;
```

### Module

A `module` is a sequence of statements.

### Statement

A statement (`stmt`) is one of these:

* variable declaration (`vardecl`)
* variable assignment (`assign`)
* `print` statement
* function declaration (`funcdecl`)
* function `call`

A variable declaration introduces a new variable `IDENT` and optionally assigns
an initial value `expr` to it. The same variable `x` is not allowed to be
redeclared in the same scope. A variable is not bound to a fixed type over its
lifetime. If the initial value is omitted then the variable is initially `null`
by default.

A variable assignment stores a new value `expr` into a variable `IDENT` which
must be declared be a previous corresponding `vardecl`, either in the same
scope or in an enclosing outer scope.

A print statement writes a value `expr` to the standard output stream. Integers
are printed in decimal form. The value is printed with a newline character at
the end (`\n`).

A function declaration introduces a new function `IDENT` with its statements
`stmt*` enclosed in `{` and `}`. This is the function body. The function body
introduces a new scope which has the top level scope as its parent. Functions
can only be declared in the top level scope. A function `IDENT` is basically
just a variable pointing to a function object. Therefore it could be reassigned
to a different value or it could be passed to another variable.

A function might refer to a variable declared in the top level scope. The
variable might even be declared further down in the source code but then the
function must be called after the variable declaration has been executed,
otherwise it is an error.

A `call` invokes a function referenced with `IDENT`. If `IDENT` is not storing
a reference to a function object it is not allowed to be called.

### Expression

An expression is one of these:

* a decimal integer literal (`decint`)
* a hexadecimal integer literal (`hexint`)
* a binary integer literal (`binint`)
* a variable identifier (`IDENT`)
* a boolean literal (`true` or `false`)
* the `null` value
* a binary operation (`binop`)

A binary operation combines two or more values with operators. Operators are
`+` which adds values or `-` which subtracts the right value from the left one.
The result is an integer. The single operands can be integers, booleans or even
`null`. `null` is interpreted as `0`, `true` as `1` and `false` as `0`. The
binary expression is evaluated from left to right.

### Types

`crispy` has these types:

* `int` - 64 bit signed integer
* `bool` - boolean value
* `null` - the `null` type
* `function` - a reference to a function object
