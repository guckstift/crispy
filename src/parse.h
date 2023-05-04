#ifndef PARSER_H
#define PARSER_H

#include "ast.h"

Stmt *lookup(Token *ident, Scope *scope);
Module *parse(Token *tokens);

#endif
