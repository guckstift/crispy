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
	EX_ARRAY,
	EX_SUBSCRIPT,
	EX_UNARY,
} ExprType;

typedef enum {
	OP_CMP,
	OP_ADD,
	OP_MUL,
	_OPLEVEL_COUNT,
} OpLevel;

typedef struct Expr {
	ExprType type;
	int isconst;
	int islvalue;
	int has_side_effects;
	Token *start;
	struct Expr *next;
	
	union {
		int64_t value; // int, bool
		char *string; // string
		Token *ident; // var
		struct Expr *callee; // call
		struct Expr *left; // binop
		struct Expr *items; // array
		struct Expr *array; // subscript
		struct Expr *subexpr; // unary
	};
	
	union {
		struct Expr *right; // binop
		int64_t tmp_id; // call
		int64_t length; // array
		struct Expr *index; // subscript
		struct Stmt *decl; // var
	};
	
	union {
		Token *op; // binop, unary
		struct Expr *args; // call
	};
	
	union {
		int64_t argcount; // call
		OpLevel oplevel; // binop
	};
} Expr;

typedef enum {
	ST_VARDECL,
	ST_ASSIGN,
	ST_PRINT,
	ST_FUNCDECL,
	ST_CALL,
	ST_RETURN,
	ST_IF,
	ST_WHILE,
} StmtType;

typedef struct DeclItem {
	struct Stmt *decl;
	struct DeclItem *next;
} DeclItem;

typedef struct {
	DeclItem *first_item;
	DeclItem *last_item;
} DeclList;

typedef struct TokenItem {
	Token *token;
	struct TokenItem *next;
} TokenItem;

typedef struct {
	TokenItem *first_item;
	TokenItem *last_item;
	int64_t length;
} TokenList;

typedef struct Stmt {
	StmtType type;
	Token *start;
	Token *end;
	struct Scope *scope;
	struct Stmt *next;
	
	union {
		Token *ident; // vardecl, funcdecl
		Expr *target; // assign
		Expr *cond; // if, while
	};
	
	union {
		Expr *init; // vardecl
		Expr *value; // assign, return
		Expr *values; // print
		Expr *call; // call
		struct Block *body; // funcdecl, if, while
	};
	
	union {
		DeclList *used_vars; // funcdecl
		struct Block *else_body; // if
	};
	
	union {
		struct Stmt *next_decl; // vardecl, funcdecl
	};
	
	union {
		int init_deferred; // vardecl, funcdecl
	};
	
	union {
		int64_t func_id; // funcdecl
		int is_param; // vardecl
	};
	
	union {
		TokenList *params; // funcdecl
	};
} Stmt;

typedef struct Scope {
	struct Scope *parent;
	Stmt *first_decl;
	Stmt *last_decl;
	int64_t decl_count;
	int64_t scope_id;
	int had_side_effects;
	Stmt *hosting_func;
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
