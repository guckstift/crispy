#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "parse.h"

static Stmt *p_stmts();
static Block *p_block();

static Token *cur = 0;
static Scope *cur_scope = 0;
static int next_func_id = 0;

static void error(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	
	fprintf(stderr, "error(%i): ", cur->line);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	
	exit(EXIT_FAILURE);
}

static Stmt *lookup_flat(Token *ident, Scope *scope)
{
	for(Stmt *decl = scope->first_decl; decl; decl = decl->next_decl) {
		if(strcmp(decl->ident->text, ident->text) == 0) {
			return decl;
		}
	}
	
	return 0;
}

Stmt *lookup(Token *ident, Scope *scope)
{
	Stmt *decl = lookup_flat(ident, scope);
	
	if(decl) {
		return decl;
	}
	
	if(scope->parent) {
		return lookup(ident, scope->parent);
	}
	
	return 0;
}

static int declare(Stmt *decl)
{
	if(lookup_flat(decl->ident, cur_scope)) {
		return 0;
	}
	
	if(cur_scope->first_decl) {
		cur_scope->last_decl->next_decl = decl;
	}
	else {
		cur_scope->first_decl = decl;
	}
	
	cur_scope->last_decl = decl;
	return 1;
}

static Token *eat_token(TokenType type)
{
	if(cur->type == type) {
		return cur ++;
	}
	
	return 0;
}

static Token *eat_keyword(Keyword keyword)
{
	if(cur->type == TK_KEYWORD && cur->keyword == keyword) {
		return cur ++;
	}
	
	return 0;
}

static Token *eat_punct(char punct)
{
	if(cur->type == TK_PUNCT && cur->punct == punct) {
		return cur ++;
	}
	
	return 0;
}

static Expr *p_atom()
{
	Token *token;
	
	(token = eat_token(TK_IDENT)) ||
	(token = eat_token(TK_INT)) ||
	(token = eat_keyword(KW_true)) ||
	(token = eat_keyword(KW_false)) ||
	(token = eat_keyword(KW_null)) ;
	
	if(!token) {
		return 0;
	}
	
	Expr *expr = calloc(1, sizeof(Expr));
	
	switch(token->type) {
		case TK_INT:
			expr->type = EX_INT;
			expr->isconst = 1;
			expr->value = token->value;
			break;
		case TK_IDENT:
			expr->type = EX_VAR;
			expr->isconst = 0;
			expr->ident = token;
			break;
		default:
			if(token->keyword == KW_null) {
				expr->type = EX_NULL;
				expr->isconst = 1;
			}
			else {
				expr->type = EX_BOOL;
				expr->isconst = 1;
				expr->value = token->keyword == KW_true;
			}
			
			break;
	}
	
	return expr;
}

static Expr *p_call_or_atom()
{
	Token *ident = eat_token(TK_IDENT);
	
	if(!ident) {
		return p_atom();
	}
	
	if(!eat_punct('(')) {
		cur = ident;
		return p_atom();
	}
	
	if(!eat_punct(')')) {
		error("expected ')' after '('");
	}
	
	Expr *expr = calloc(1, sizeof(Expr));
	expr->type = EX_CALL;
	expr->isconst = 0;
	expr->ident = ident;
	return expr;
}

static Expr *p_binop()
{
	Expr *left = p_call_or_atom();
	
	Token *op;
	
	while((op = eat_punct('+')) || (op = eat_punct('-'))) {
		Expr *right = p_call_or_atom();
		
		if(!right) {
			error("expected right side of %c", op->punct);
		}
		
		if(left->isconst && right->isconst) {
			Expr *sum = calloc(1, sizeof(Expr));
			sum->type = EX_INT;
			sum->isconst = 1;
			sum->value = op->punct == '+'
				? left->value + right->value
				: left->value - right->value;
			left = sum;
		}
		else {
			Expr *binop = calloc(1, sizeof(Expr));
			binop->type = EX_BINOP;
			binop->isconst = 0;
			binop->left = left;
			binop->right = right;
			binop->op = op->punct;
			left = binop;
		}
		
	}
	
	return left;
}

static Expr *p_expr()
{
	return p_binop();
}

static Stmt *p_vardecl()
{
	Token *start = eat_keyword(KW_var);
	
	if(!start) {
		return 0;
	}
	
	Token *ident = eat_token(TK_IDENT);
	
	if(ident == 0) {
		error("expected identifier");
	}
	
	Expr *init = 0;
	
	if(eat_punct('=')) {
		init = p_expr();
		
		if(init == 0) {
			error("expected initializer after '='");
		}
	}
	
	if(!eat_punct(';')) {
		error("expected ';' after variable declaration");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_VARDECL;
	stmt->start = start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->ident = ident;
	stmt->init = init;
	
	if(!declare(stmt)) {
		error("name %s already declared", ident->text);
	}
	
	return stmt;
}

static Stmt *p_call_x(Token *ident)
{
	if(!eat_punct(')')) {
		error("expected ')' after '('");
	}
	
	if(!eat_punct(';')) {
		error("expected ';' after function call");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->start = ident;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->type = ST_CALL;
	stmt->ident = ident;
	return stmt;
}

static Stmt *p_assign_x(Token *ident)
{
	Expr *value = p_expr();
	
	if(value == 0) {
		error("expected right side of assignment");
	}
	
	if(!eat_punct(';')) {
		error("expected ';' after assignment");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_ASSIGN;
	stmt->start = ident;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->ident = ident;
	stmt->value = value;
	return stmt;
}

static Stmt *p_assign_or_call()
{
	Token *ident = eat_token(TK_IDENT);
	
	if(ident == 0) {
		return 0;
	}
	
	if(eat_punct('(')) {
		return p_call_x(ident);
	}
	else if(eat_punct('=')) {
		return p_assign_x(ident);
	}
	
	error("expected '=' or '('");
}

static Stmt *p_print()
{
	Token *start = eat_keyword(KW_print);
	
	if(!start) {
		return 0;
	}
	
	Expr *first = p_expr();
	
	if(first == 0) {
		error("expected value to print");
	}
	
	Expr *last = first;
	
	while(eat_punct(',')) {
		Expr *next = p_expr();
		
		if(next == 0) {
			error("expected another value after ',' to print");
		}
		
		last->next = next;
		last = next;
	}
	
	if(!eat_punct(';')) {
		error("expected ';' after print statement");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_PRINT;
	stmt->start = start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->values = first;
	return stmt;
}

static Stmt *p_funcdecl()
{
	Token *start = eat_keyword(KW_function);
	
	if(!start) {
		return 0;
	}
	
	Token *ident = eat_token(TK_IDENT);
	int64_t func_id = next_func_id;
	next_func_id ++;
	
	if(ident == 0) {
		error("expected identifier");
	}
	
	if(!eat_punct('(')) {
		error("expected '(' after function name");
	}
	
	if(!eat_punct(')')) {
		error("expected ')'");
	}
	
	if(!eat_punct('{')) {
		error("expected '{'");
	}
	
	Block *body = p_block();
	
	if(!eat_punct('}')) {
		error("expected '}' or another statement");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_FUNCDECL;
	stmt->start = start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->ident = ident;
	stmt->body = body;
	stmt->func_id = func_id;
	
	if(!declare(stmt)) {
		error("name %s already declared", ident->text);
	}
	
	return stmt;
}

static Stmt *p_return()
{
	Token *start = eat_keyword(KW_return);
	
	if(!start) {
		return 0;
	}
	
	if(!cur_scope->parent) {
		error("return can only be used inside a function");
	}
	
	Expr *value = p_expr();
	
	if(value == 0) {
		error("expected value to return");
	}
	
	if(!eat_punct(';')) {
		error("expected ';' after return statement");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_RETURN;
	stmt->start = start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->value = value;
	return stmt;
}

static Stmt *p_stmt()
{
	Stmt *stmt;
	
	(stmt = p_vardecl()) ||
	(stmt = p_assign_or_call()) ||
	(stmt = p_print()) ||
	(stmt = p_funcdecl()) ||
	(stmt = p_return()) ;
	
	return stmt;
}

static Stmt *p_stmts()
{
	Stmt *first = 0;
	Stmt *last = 0;
	
	while(1) {
		Stmt *stmt = p_stmt();
		
		if(!stmt) {
			break;
		}
		
		if(first) {
			last->next = stmt;
		}
		else {
			first = stmt;
		}
		
		last = stmt;
	}
	
	return first;
}

static Block *p_block()
{
	Scope *scope = calloc(1, sizeof(Scope));
	scope->parent = cur_scope;
	cur_scope = scope;
	
	Stmt *stmts = p_stmts();
	Block *block = calloc(1, sizeof(Block));
	block->stmts = stmts;
	block->scope = scope;
	
	cur_scope = scope->parent;
	return block;
}

Module *parse(Token *tokens)
{
	cur = tokens;
	cur_scope = 0;
	next_func_id = 0;
	Block *block = p_block();
	
	if(!eat_token(TK_EOF)) {
		error("unknown statement");
	}
	
	Module *module = calloc(1, sizeof(Module));
	module->block = block;
	return module;
}
