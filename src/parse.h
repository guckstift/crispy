#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "lex.h"

typedef enum {
	EX_NULL,
	EX_BOOL,
	EX_INT,
	EX_VAR,
} ExprType;

typedef struct {
	ExprType type;
	union {
		int64_t value;
		Token *ident;
	};
} Expr;

typedef enum {
	ST_VARDECL,
	ST_ASSIGN,
	ST_PRINT,
} StmtType;

typedef struct Stmt {
	StmtType type;
	union {
		Token *ident;
	};
	union {
		Expr *init;
		Expr *value;
	};
	union {
		struct Stmt *next_decl;
	};
	struct Stmt *next;
} Stmt;

typedef struct {
	Stmt *stmts;
	Stmt *decls;
} Module;

Module *parse(Token *tokens);
void print_module(Module *module);

#endif
