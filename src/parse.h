#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include "lex.h"

typedef enum {
	EX_NULL,
	EX_BOOL,
	EX_INT,
	EX_STRING,
	EX_VAR,
	EX_BINOP,
	EX_CALL,
} ExprType;

typedef struct Expr {
	ExprType type;
	int isconst;
	struct Expr *next;
	
	union {
		int64_t value;
		char *string;
		Token *ident;
		struct Expr *left;
	};
	
	union {
		struct Expr *right;
	};
	
	union {
		char op;
	};
} Expr;

typedef enum {
	ST_VARDECL,
	ST_ASSIGN,
	ST_PRINT,
	ST_FUNCDECL,
	ST_CALL,
	ST_RETURN,
} StmtType;

typedef struct DeclItem {
	struct Stmt *decl;
	struct DeclItem *next;
} DeclItem;

typedef struct {
	DeclItem *first_item;
	DeclItem *last_item;
} DeclList;

typedef struct Stmt {
	StmtType type;
	Token *start;
	Token *end;
	struct Scope *scope;
	struct Stmt *next;
	
	union {
		Token *ident; // vardecl, assign, funcdecl, call
	};
	
	union {
		Expr *init; // vardecl
		Expr *value; // assign, return
		Expr *values; // print
		struct Block *body; // funcdecl
	};
	
	union {
		DeclList *used_vars; // funcdecl
		struct Stmt *decl; // assign
	};
	
	union {
		struct Stmt *next_decl; // vardecl, funcdecl
	};
	
	union {
		int early_use; // vardecl, funcdecl
	};
	
	union {
		int64_t func_id;
	};
} Stmt;

typedef struct Scope {
	struct Scope *parent;
	Stmt *first_decl;
	Stmt *last_decl;
} Scope;

typedef struct Block {
	Stmt *stmts;
	Scope *scope;
} Block;

typedef struct {
	Block *block;
} Module;

Stmt *lookup(Token *ident, Scope *scope);
Module *parse(Token *tokens);

#endif
