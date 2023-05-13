#ifndef AST_H
#define AST_H

#include <stdint.h>
#include <stdbool.h>

#define IPUNCT(s) (s[0] | s[1] << 8)

#define KEYWORDS(f) \
	f(var) \
	f(print) \
	f(true) \
	f(false) \
	f(null) \
	f(function) \
	f(return) \
	f(if) \
	f(else) \
	f(while) \

typedef enum {
	TK_KEYWORD,
	TK_IDENT,
	TK_INT,
	TK_STRING,
	TK_PUNCT,
	TK_EOF,
} TokenType;

typedef enum {
	#define F(x) KW_ ## x,
	KEYWORDS(F)
	#undef F
	KEYWORD_COUNT
} Keyword;

typedef struct Token {
	TokenType type;
	int64_t line;
	char *linep;
	char *start;
	int64_t length;
	union {
		Keyword keyword;
		char *id;
		char *text;
		int64_t value;
		int64_t punct;
	};
} Token;

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
	bool isconst : 1;
	bool islvalue : 1;
	bool has_tmps : 1;
	int64_t tmp_id;
	Token *start;
	struct Scope *scope;
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
		int64_t length; // array
		struct Expr *index; // subscript
		struct Decl *decl; // var
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

typedef struct DeclItem {
	struct Decl *decl;
	struct DeclItem *next;
} DeclItem;

typedef struct TokenItem {
	Token *token;
	struct TokenItem *next;
} TokenItem;

typedef struct {
	TokenItem *items;
	int64_t length;
} TokenList;

typedef struct Decl {
	struct Decl *next; // next in scope
	struct Scope *scope;
	Token *ident;
	Token *end;
	bool isfunc : 1;
	bool init_deferred : 1;
	
	union {
		Expr *init; // vardecl
		struct Block *body; // funcdecl
	};
	
	union {
		int64_t func_id; // funcdecl
		bool is_param; // vardecl
	};
	
	union {
		DeclItem *used_vars; // funcdecl
	};
	
	union {
		TokenList *params; // funcdecl
	};
} Decl;

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

typedef struct Stmt {
	StmtType type;
	Token *start;
	Token *end;
	struct Stmt *next;
	
	union {
		Decl *decl; // vardecl, funcdecl
		Expr *target; // assign
		Expr *cond; // if, while
	};
	
	union {
		Expr *value; // assign, return
		Expr *values; // print
		Expr *call; // call
		struct Block *body; // funcdecl, if, while
	};
	
	union {
		struct Block *else_body; // if
	};
} Stmt;

typedef struct Scope {
	struct Scope *parent;
	Decl *decls;
	int64_t decl_count;
	int64_t scope_id;
	int had_side_effects;
	Decl *hosting_func;
	int64_t tmp_count;
} Scope;

typedef struct Block {
	Stmt *stmts;
	Scope *scope;
} Block;

typedef struct {
	char *filename;
	char *pathid;
	long srcsize;
	char *src;
	Token *tokens;
	Block *body;
	char *cfilename;
} Module;

typedef struct {
	Module *main;
	char *exename;
} Project;

#endif
