# Making a programming language from scratch - Part 1

In this tutorial series we will create a dynamic programming language that is
pretty much like JavaScript or Python and gets compiled to C code. Bytecode is
nice and relatively simple to implement. Python uses bytecode. But performance
can be much better with natively compiled code. Spitting out C code is a lazy
but effective way to get started when we want our language to translate to a
native executable.

Our language will be stupidly simple at first. It will feature only 3
statement types: variable declarations, variable assignments and print
statements. Aside from that it'll have only 3 simple types: the null type,
boolean and integer. This seems pretty useless at first. It is indeed not more
than a toy language. But it will be a good starting point built on a solid
project structure to extend on in later parts.

Let's call our language `crispy`. I just think this sounds catchy and we could
extend our source file names with `.cr`. The compiler will snack a single source
file and digest it into a C file within these 4 well defined steps:

1. lexical analysis (`lex`) - chop the source code into *tokens*
2. parsing (`parse`) - create a tree structure from the token list
3. semantic analysis (`analyze`) - declare and check variables
4. code generation (`generate`) - transform the analyzed tree into C code

## Language

Let's first specify how our language should look like. The more precisely we
describe and specify it, the more straight forward the development process gets.

So let's start with statements. Like I said: `crispy` has only 3 of them:

### The variable declaration

**var** *IDENT* = *expr* ;

declares a new variable with the name *IDENT* and optionally assigns an initial
value *expr* to it.

**var** *IDENT* ;

When the initializer is missing, the default value `null` is assigned to the
variable.

A variable `x` can only be declared once. Otherwise the compiler cries.

### The variable assignment

Variables previously introduced by a declaration can be (re-)assigned to new
values.

*IDENT* = *expr* ;

Since `crispy` is a dynamic language the type of *expr* can be any type no
matter what value the variable was assigned before.

Trying to assign a variable that is not declared (yet) isn't allowed of course.

### The print statement

Last but not least `crispy` also provides a built-in print statement:

**print** *expr* ;

It prints a single value on a seperate line to the standard output stream
(appending a line break after it).

### Types

Now let's talk about our type system. `crispy` should support dynamic typing
which simply means: variables are not bound to a fixed type. The type can
change over the variables lifetime.

In our first version of `crispy` these 3 types will be sufficient:

* the null type which has only one valid value `null`. This is the value that a
  variable gets implicitly initialized with by default.
* a boolean type, as you might expect, only has `true` or `false` values.
* an integer type which features 64 bit signed integer
  numbers (sadly we won't implement any operators yet to form negative numbers
  yet).

Yes, we don't have strings, arrays or even objects yet. These would
introduce a lot more issues to tackle and questions to be answered. We try to
keep things simple for the start. Extending our language later will be
comfortable once we implemented a minimal base.

### Expressions

To write values of these different types we have expressions. In this version
of `crispy` we will do without operators and composed expressions and only
support the most simple forms of them: variables and literals.

Literals are:

* integer literals in decimal: [0-9]+
* `null`
* `true` and `false`

And that's it.

### Lexis

Now we only have to clarify some details about our lexis: how shall the most
atomic pieces of a `crispy` source file be formed. Because this will be the
question of the compilers first step: lexical analysis or the *lexer*.

The first step chops the source code into a sequence of so called *tokens*.
Each of these tokens gets classified as a certain token type.

We specify these types and their formats. We could describe
each tokens format with a regular expression or we could describe it in prose.

* a variable identifier *IDENT*
  format: `[a-zA-Z_][a-zA-Z_0-9]*`
  an identifier consists of latin letters (upper and lower case), digits and the
  underscore. But it should not begin with a digit. The lexer first consumes as
  many fitting characters as it can. The word that is consumed can not be equal
  to one of the keywords.
* a *KEYWORD* has the same format as an identifier but is only one of this list:
  `var`, `print`, `null`, `true`, `false`
* an integer literal *INT*
  format: `[0-9]+`
  consists of one or more decimal digits. Unlike in other languages a leading
  zero does not make the literal an octal number. `crispy` actually only
  supports decimal integers at the time
* punctuators *PUNCT* are one of:
  `;`, `=`
  just punctuation and operators





