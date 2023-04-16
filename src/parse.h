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
	int isconst;
	
	union {
		int64_t value;
		Token *ident;
	};
} Expr;

typedef enum {
	ST_VARDECL,
	ST_ASSIGN,
	ST_PRINT,
	ST_FUNCDECL,
	ST_CALL,
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
		Expr *value; // assign, print
		struct Block *body; // funcdecl
	};
	
	union {
		int early_use; // vardecl
		DeclList *used_vars; // funcdecl
		struct Stmt *decl; // assign
	};
	
	union {
		struct Stmt *next_decl; // vardecl, funcdecl
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
void print_scope(Scope *scope);
void print_module(Module *module);

#endif
