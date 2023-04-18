# crispy 0.1

This document specifies the *crispy* programming language 0.1 and its compiler.

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
	"var" | "print" | "true" | "false"
) ;

INT = [0-9]+ ;

PUNCT = ";" | "=" ;

-WHITE = [ \t\n\v\f\r]+ ;
```

### Grammar

```
module = stmt* ;
stmt = vardecl | assign | print ;
vardecl = "var" IDENT ( "=" expr )? ";" ;
assign = IDENT "=" expr ";" ;
print = "print" expr ";" ;
expr = INT | IDENT | "true" | "false" ;
```

### Module

A `module` is a sequence of statements.

### Statement

A statement (`stmt`) is one of these:

* variable declaration (`vardecl`)
* variable assignment (`assign`)
* `print` statement

A variable declaration introduces a new variable `IDENT` and optionally assigns
an initial value `expr` to it. The same variable `x` is not allowed to be
redeclared. A variable is not bound to a fixed type over its lifetime.

A variable assignment stores a new value `expr` into a variable `IDENT` which
must be declared be a previous corresponding `vardecl`.

A print statement writes a value `expr` to the standard output stream. Integers
are printed in decimal form. The value is printed with a newline character at
the end (`\n`).

### Expression

An expression is one of these:

* a decimal integer literal (`INT`)
* a variable identifier (`IDENT`)
* a boolean literal (`true` or `false`)

### Types

`crispy` has these types:

* `int` 64 bit signed integer
* `bool` boolean value
