#ifndef PRINT_H
#define PRINT_H

#include "lex.h"
#include "parse.h"

void print_tokens(Token *tokens);
void print_token(Token *token);
void print_scope(Scope *scope);
void print_module(Module *module);

#endif
