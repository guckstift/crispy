# crispy 0.1

This document specifies the *crispy* programming language 0.1 and its compiler.

## Compiler

The `crispy` compiler program translates *crispy* module files to C source
files. It takes the path to a module file and the path to the C output source
file on the command line and then compiles it.

## Language

Module files are written in the `crispy` programming language.

### Lexis

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

## Sematics

### Module

A `module` is a sequence of statements.

### Statement

A statement (`stmt`) is one of these:

* a variable declaration (`vardecl`)
