#ifndef PRINT_H
#define PRINT_H

#include <stdio.h>
#include <stdarg.h>
#include "lex.h"
#include "parse.h"

#define P_CSI        "\x1b["
#define P_RESET      P_CSI "0m"
#define P_BOLD       P_CSI "1m"
#define P_RGB(r,g,b) P_CSI "38;2;" #r ";" #g ";" #b "m"
#define P_RED        P_RGB(255, 0, 0)
#define P_BOLDRED    P_BOLD P_RED

#define P_COL_KEYWORD P_RGB(64, 128, 255)
#define P_COL_IDENT   P_RGB(64, 255, 255)
#define P_COL_LITERAL P_RGB(255, 64, 255)

void vfprint(FILE *fs, char *msg, va_list args);
void fprint(FILE *fs, char *msg, ...);
void print(char *msg, ...);
void eprint(char *msg, ...);

void print_tokens(Token *tokens);
void print_token(Token *token);
void print_scope(Scope *scope);
void print_module(Module *module);
void fprint_expr(FILE *fs, Expr *expr);

#endif
