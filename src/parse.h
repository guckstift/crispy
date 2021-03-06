#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

Decl *lookup(Token *ident, Scope *scope);
void parse(Module *module);

#endif
