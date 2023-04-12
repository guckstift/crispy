#include <stdio.h>
#include <stdlib.h>
#include "parse.h"

static Token *cur = 0;

static void error(char *msg)
{
	fprintf(stderr, "error(%i): %s\n", cur->line, msg);
	exit(EXIT_FAILURE);
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
			expr->value = token->value;
			break;
		case TK_IDENT:
			expr->type = EX_VAR;
			expr->ident = token;
			break;
		default:
			if(token->keyword == KW_null) {
				expr->type = EX_NULL;
			}
			else {
				expr->type = EX_BOOL;
				expr->value = token->keyword == KW_true;
			}
			
			break;
	}
	
	return expr;
}

static Stmt *p_vardecl()
{
	if(!eat_keyword(KW_var)) {
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
	stmt->ident = ident;
	stmt->init = init;
	stmt->next_decl = 0;
	return stmt;
}

static Stmt *p_assign()
{
	Token *ident = eat_token(TK_IDENT);
	
	if(ident == 0) {
		return 0;
	}
	
	if(!eat_punct('=')) {
		error("expected '='");
	}
	
	Expr *value = p_expr();
	
	if(value == 0) {
		error("expected right side of assignment");
	}
	
	if(!eat_punct(';')) {
		error("expected ';' after assignment");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_ASSIGN;
	stmt->ident = ident;
	stmt->value = value;
	return stmt;
}

static Stmt *p_print()
{
	if(!eat_keyword(KW_print)) {
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
	stmt->value = value;
	return stmt;
}

static Stmt *p_stmt()
{
	Stmt *stmt;
	
	(stmt = p_vardecl()) ||
	(stmt = p_assign()) ||
	(stmt = p_print()) ;
	
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

Module *parse(Token *tokens)
{
	cur = tokens;
	Stmt *stmts = p_stmts();
	
	if(!eat_token(TK_EOF)) {
		error("unknown statement");
	}
	
	Module *module = malloc(sizeof(Module));
	module->stmts = stmts;
	module->decls = 0;
	
	return module;
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
	}
}

static void print_stmts(Stmt *stmts)
{
	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		print_stmt(stmt);
		printf("\n");
	}
}

void print_module(Module *module)
{
	print_stmts(module->stmts);
}
