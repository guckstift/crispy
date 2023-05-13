#ifndef PRINT_H
#define PRINT_H

#include <stdio.h>
#include <stdarg.h>
#include "ast.h"

#define P_CSI          "\x1b["
#define P_RESET        P_CSI "0m"
#define P_BOLD         P_CSI "1m"
#define P_RGB(r,g,b)   P_CSI "38;2;" #r ";" #g ";" #b "m"
#define P_COL_ERROR    P_RGB(255, 0, 0)
#define P_COL_KEYWORD  P_RGB(64, 128, 255)
#define P_COL_IDENT    P_RGB(64, 255, 255)
#define P_COL_LITERAL  P_RGB(255, 64, 255)
#define P_COL_LINENO   P_RGB(128, 128, 128)
#define P_COL_COMMENT  P_RGB(160, 160, 160)
#define P_COL_SECTION  P_RGB(255, 255, 64)
#define P_ERROR        P_BOLD P_COL_ERROR

void vfprint(FILE *fs, char *msg, va_list args);
void fprint(FILE *fs, char *msg, ...);
void print(char *msg, ...);

void vprint_error(
	int64_t line, char *linep, char *errpos, char *msg, va_list args
);

void verror_at(Token *at, char *msg, va_list args);
void error_at(Token *at, char *msg, ...);
void verror_after_t(Token *t, char *msg, va_list args);

void print_tokens(Token *tokens);
void print_token(Token *token);
void print_scope(Scope *scope);
void print_module_block(Block *body);
void fprint_expr(FILE *fs, Expr *expr);

#endif
