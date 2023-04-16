#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "parse.h"

static Stmt *p_stmts();
static Block *p_block();
static void print_block(Block *block);

static Token *cur = 0;
static int level = 0;
static Scope *cur_scope = 0;

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

static Expr *p_expr()
{
	Token *token;
	
	(token = eat_token(TK_IDENT)) ||
	(token = eat_token(TK_INT)) ||
	(token = eat_keyword(KW_true)) ||
	(token = eat_keyword(KW_false)) ||
	(token = eat_keyword(KW_null)) ;
	
	if(token == 0) {
		error("expected identifier, boolean, integer or null");
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
	
	Expr *value = p_expr();
	
	if(value == 0) {
		error("expected value to print");
	}
	
	if(!eat_punct(';')) {
		error("expected ';' after print statement");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_PRINT;
	stmt->start = start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->value = value;
	return stmt;
}

static Stmt *p_funcdecl()
{
	Token *start = eat_keyword(KW_function);
	
	if(!start) {
		return 0;
	}
	
	if(cur_scope->parent) {
		error("functions can only be declared at top level");
	}
	
	Token *ident = eat_token(TK_IDENT);
	
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
	
	if(!declare(stmt)) {
		error("name %s already declared", ident->text);
	}
	
	return stmt;
}

static Stmt *p_stmt()
{
	Stmt *stmt;
	
	(stmt = p_vardecl()) ||
	(stmt = p_assign_or_call()) ||
	(stmt = p_print()) ||
	(stmt = p_funcdecl()) ;
	
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
	Block *block = p_block();
	
	if(!eat_token(TK_EOF)) {
		error("unknown statement");
	}
	
	Module *module = calloc(1, sizeof(Module));
	module->block = block;
	return module;
}

static void print_indent()
{
	for(int i=0; i < level; i++) {
		printf("\t");
	}
}

static void print_expr(Expr *expr)
{
	switch(expr->type) {
		case EX_NULL:
			printf("null");
			break;
		case EX_BOOL:
			printf("%s", expr->value ? "true" : "false");
			break;
		case EX_INT:
			printf("%li", expr->value);
			break;
		case EX_VAR:
			printf("%s", expr->ident->text);
			break;
	}
}

static void print_vardecl(Stmt *vardecl)
{
	printf("var ");
	printf("%s", vardecl->ident->text);
	
	if(vardecl->init) {
		printf(" = ");
		print_expr(vardecl->init);
	}
}

static void print_assign(Stmt *assign)
{
	printf("%s", assign->ident->text);
	printf(" = ");
	print_expr(assign->value);
}

static void print_print(Stmt *print)
{
	printf("print ");
	print_expr(print->value);
}

static void print_funcdecl(Stmt *funcdecl)
{
	printf("function ");
	printf("%s", funcdecl->ident->text);
	printf("() {\n");
	level++;
	print_block(funcdecl->body);
	level--;
	print_indent();
	printf("}");
}

static void print_call(Stmt *assign)
{
	printf("%s", assign->ident->text);
	printf("()");
}

static void print_stmt(Stmt *stmt)
{
	switch(stmt->type) {
		case ST_VARDECL:
			print_vardecl(stmt);
			break;
		case ST_ASSIGN:
			print_assign(stmt);
			break;
		case ST_PRINT:
			print_print(stmt);
			break;
		case ST_FUNCDECL:
			print_funcdecl(stmt);
			break;
		case ST_CALL:
			print_call(stmt);
			break;
	}
}

void print_scope(Scope *scope)
{
	printf("# scope %p: ", (void*)scope);
	
	for(Stmt *decl = scope->first_decl; decl; decl = decl->next_decl) {
		printf("%s ", decl->ident->text);
	}
	
	printf("\n");
}

static void print_block(Block *block)
{
	print_indent();
	print_scope(block->scope);
	
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		print_indent();
		print_stmt(stmt);
		printf("\n");
	}
}

void print_module(Module *module)
{
	level = 0;
	print_block(module->block);
}
