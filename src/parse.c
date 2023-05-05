#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "parse.h"
#include "print.h"

static Stmt *p_stmts();
static Block *p_block(TokenList *params);
static Expr *p_expr();

static Token *cur = 0;
static Scope *cur_scope = 0;
static int next_func_id = 0;
static int next_tmp_id = 1;
static int next_scope_id = 0;

static void error(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	verror_at(cur, msg, args);
}

static void error_after(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	verror_after_t(cur - 1, msg, args);
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
	cur_scope->decl_count ++;
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

static Token *see_punct(char *punct)
{
	int64_t ipunct = punct[0] | punct[1] << 8;
	
	if(cur->type == TK_PUNCT && cur->punct == ipunct) {
		return cur;
	}
	
	return 0;
}

static Token *eat_punct(char *punct)
{
	if(see_punct(punct)) {
		return cur ++;
	}
	
	return 0;
}

static Expr *p_array()
{
	Token *start = eat_punct("[");
	
	if(!start) {
		return 0;
	}
	
	Expr *item = p_expr();
	Expr *first = item;
	Expr *last = item;
	int64_t length = 0;
	
	if(item) {
		length = 1;
		
		while(eat_punct(",")) {
			item = p_expr();
			
			if(!item) {
				error_after("expected another array item after ','");
			}
			
			last->next = item;
			last = item;
			length ++;
		}
	}
	
	if(!eat_punct("]")) {
		error_after("expected ']' at the end of array literal");
	}
	
	Expr *expr = calloc(1, sizeof(Expr));
	expr->type = EX_ARRAY;
	expr->isconst = 0;
	expr->has_tmps = 1;
	expr->tmp_id = next_tmp_id++;
	expr->start = start;
	expr->items = first;
	expr->length = length;
	return expr;
}

static Expr *p_atom()
{
	Expr *array = p_array();
	
	if(array) {
		return array;
	}
	
	Token *token;
	
	(token = eat_token(TK_IDENT)) ||
	(token = eat_token(TK_INT)) ||
	(token = eat_token(TK_STRING)) ||
	(token = eat_keyword(KW_true)) ||
	(token = eat_keyword(KW_false)) ||
	(token = eat_keyword(KW_null)) ;
	
	if(!token) {
		return 0;
	}
	
	Expr *expr = calloc(1, sizeof(Expr));
	expr->start = token;
	
	switch(token->type) {
		case TK_INT:
			expr->type = EX_INT;
			expr->isconst = 1;
			expr->value = token->value;
			break;
		case TK_STRING:
			expr->type = EX_STRING;
			expr->isconst = 1;
			expr->string = token->text;
			break;
		case TK_IDENT:
			expr->type = EX_VAR;
			expr->isconst = 0;
			expr->islvalue = 1;
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

static Expr *p_callexpr_x(Expr *callee)
{
	Expr *first = p_expr();
	int64_t argcount = 0;
	
	if(first) {
		Expr *last = first;
		argcount = 1;
		
		while(eat_punct(",")) {
			Expr *next = p_expr();
			
			if(next == 0) {
				error_after("expected another argument after ','");
			}
			
			last->next = next;
			last = next;
			argcount ++;
		}
	}
	
	if(!eat_punct(")")) {
		error_after("expected ')' after argument list");
	}
	
	Expr *expr = calloc(1, sizeof(Expr));
	expr->type = EX_CALL;
	expr->isconst = 0;
	expr->has_tmps = 1;
	expr->start = callee->start;
	expr->callee = callee;
	expr->tmp_id = next_tmp_id++;
	expr->args = first;
	expr->argcount = argcount;
	cur_scope->had_side_effects = 1;
	return expr;
}

static Expr *p_subscript_x(Expr *array)
{
	Expr *index = p_expr();
	
	if(!index) {
		error_after("expected index expression in []");
	}
	
	if(!eat_punct("]")) {
		error_after("expected ']' after index");
	}
	
	Expr *expr = calloc(1, sizeof(Expr));
	expr->type = EX_SUBSCRIPT;
	expr->isconst = 0;
	expr->islvalue = 1;
	expr->has_tmps = array->has_tmps || index->has_tmps;
	expr->start = array->start;
	expr->array = array;
	expr->index = index;
	return expr;
}

static Expr *p_postfix()
{
	Expr *expr = p_atom();
	
	if(!expr) {
		return 0;
	}
	
	while(see_punct("(") || see_punct("[")) {
		if(eat_punct("(")) {
			expr = p_callexpr_x(expr);
		}
		else if(eat_punct("[")) {
			expr = p_subscript_x(expr);
		}
	}
	
	return expr;
}

static Expr *p_unary()
{
	Token *op = 0;
	
	(op = eat_punct("+")) ||
	(op = eat_punct("-")) ;
	
	if(op == 0) {
		return p_postfix();
	}
	
	Expr *subexpr = p_unary();
	
	if(subexpr == 0) {
		error_after("expected expression after unary %T", op);
	}
	
	Expr *expr = calloc(1, sizeof(Expr));
	expr->type = EX_UNARY;
	expr->isconst = subexpr->isconst;
	expr->has_tmps = subexpr->has_tmps;
	expr->start = op;
	expr->subexpr = subexpr;
	expr->op = op;
	return expr;
}

static Token *p_operator(int level)
{
	Token *op = 0;
	
	switch(level) {
		case OP_CMP:
			(op = eat_punct("<")) ||
			(op = eat_punct(">")) ||
			(op = eat_punct("==")) ||
			(op = eat_punct("!=")) ||
			(op = eat_punct("<=")) ||
			(op = eat_punct(">=")) ;
			break;
		case OP_ADD:
			(op = eat_punct("+")) ||
			(op = eat_punct("-")) ;
			break;
		case OP_MUL:
			(op = eat_punct("*")) ||
			(op = eat_punct("%")) ;
			break;
	}
	
	return op;
}

static Expr *p_binop(int level)
{
	if(level == _OPLEVEL_COUNT) {
		return p_unary();
	}
	
	Expr *left = p_binop(level + 1);
	
	if(!left) {
		return 0;
	}
	
	Token *op;
	
	while(op = p_operator(level)) {
		Expr *right = p_binop(level + 1);
		
		if(!right) {
			error_after("expected right side of %T", op);
		}
		
		if(left->type == EX_STRING || right->type == EX_STRING) {
			Token *at = left->type == EX_STRING ? left->start : right->start;
			error_at(at, "strings can not be used with %T", op);
		}
		
		if(left->type == EX_ARRAY || right->type == EX_ARRAY) {
			Token *at = left->type == EX_ARRAY ? left->start : right->start;
			error_at(at, "arrays can not be used with %T", op);
		}
		
		if(
			level == OP_CMP &&
			left->type == EX_BINOP && left->oplevel == OP_CMP
		) {
			error_at(op, "can not chain comparisons");
		}
		
		Expr *binop = calloc(1, sizeof(Expr));
		binop->type = EX_BINOP;
		binop->isconst = left->isconst && right->isconst;
		binop->has_tmps = left->has_tmps || right->has_tmps;
		binop->start = left->start;
		binop->left = left;
		binop->right = right;
		binop->op = op;
		binop->oplevel = level;
		left = binop;
	}
	
	return left;
}

static Expr *p_expr()
{
	return p_binop(0);
}

static Stmt *p_vardecl()
{
	Token *start = eat_keyword(KW_var);
	
	if(!start) {
		return 0;
	}
	
	Token *ident = eat_token(TK_IDENT);
	
	if(ident == 0) {
		error("expected identifier to declare");
	}
	
	Expr *init = 0;
	
	if(eat_punct("=")) {
		init = p_expr();
		
		if(init == 0) {
			error_after("expected initializer after '='");
		}
	}
	
	if(!eat_punct(";")) {
		error_after("expected ';' after variable declaration");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_VARDECL;
	stmt->start = start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->ident = ident;
	stmt->init = init;
	
	if(cur_scope->had_side_effects || init && !init->isconst) {
		stmt->init_deferred = 1;
	}
	
	if(!declare(stmt)) {
		error_at(ident, "name %T already declared", ident);
	}
	
	return stmt;
}

static Stmt *p_call_x(Expr *call)
{
	if(!eat_punct(";")) {
		error_after("expected ';' after function call");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->start = call->start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->type = ST_CALL;
	stmt->call = call;
	return stmt;
}

static Stmt *p_assign_x(Expr *target)
{
	Expr *value = p_expr();
	
	if(value == 0) {
		error_after("expected right side of assignment");
	}
	
	if(!eat_punct(";")) {
		error_after("expected ';' after assignment");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_ASSIGN;
	stmt->start = target->start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->target = target;
	stmt->value = value;
	return stmt;
}

static Stmt *p_assign_or_call()
{
	Expr *expr = p_expr();
	
	if(!expr) {
		return 0;
	}
	
	if(expr->type == EX_CALL) {
		return p_call_x(expr);
	}
	
	if(eat_punct("=")) {
		if(expr->islvalue == 0) {
			error_at(expr->start, "target is not assignable");
		}
		
		return p_assign_x(expr);
	}
	
	error_after("expected '=' or '('");
}

static Stmt *p_print()
{
	Token *start = eat_keyword(KW_print);
	
	if(!start) {
		return 0;
	}
	
	Expr *first = p_expr();
	
	if(first == 0) {
		error_after("expected value to print");
	}
	
	Expr *last = first;
	
	while(eat_punct(",")) {
		Expr *next = p_expr();
		
		if(next == 0) {
			error_after("expected another value after ',' to print");
		}
		
		last->next = next;
		last = next;
	}
	
	if(!eat_punct(";")) {
		error_after("expected ';' after print statement");
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
		error("expected function identifier");
	}
	
	if(!eat_punct("(")) {
		error_after("expected '(' after function name");
	}
	
	TokenList *params = calloc(1, sizeof(TokenList));
	Token *param = eat_token(TK_IDENT);
	
	if(param) {
		TokenItem *item = calloc(1, sizeof(TokenItem));
		item->token = param;
		item->next = 0;
		params->first_item = item;
		params->last_item = item;
		params->length = 1;
		
		while(eat_punct(",")) {
			Token *param = eat_token(TK_IDENT);
			
			if(param == 0) {
				error_after("expected another parameter after ','");
			}
			
			TokenItem *item = calloc(1, sizeof(TokenItem));
			item->token = param;
			item->next = 0;
			params->last_item->next = item;
			params->last_item = item;
			params->length ++;
		}
	}
	
	if(!eat_punct(")")) {
		error_after("expected ')'");
	}
	
	if(!eat_punct("{")) {
		error_after("expected '{'");
	}
	
	Block *body = p_block(params);
	
	if(!eat_punct("}")) {
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
	stmt->params = params;
	
	if(cur_scope->had_side_effects) {
		stmt->init_deferred = 1;
	}
	
	if(!declare(stmt)) {
		error_at(ident, "name %T already declared", ident);
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
		error_at(start, "return can only be used inside a function");
	}
	
	Expr *value = p_expr();
	
	if(!eat_punct(";")) {
		error_after("expected ';' after return statement");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_RETURN;
	stmt->start = start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->value = value;
	return stmt;
}

static Stmt *p_if()
{
	Token *start = eat_keyword(KW_if);
	
	if(!start) {
		return 0;
	}
	
	Expr *cond = p_expr();
	
	if(!cond) {
		error("expected a condition expression");
	}
	
	if(!eat_punct("{")) {
		error("expected '{'");
	}
	
	Block *body = p_block(0);
	
	if(!eat_punct("}")) {
		error("expected '}' or another statement");
	}
		
	Block *else_body = 0;
	
	if(eat_keyword(KW_else)) {
		if(!eat_punct("{")) {
			error("expected '{'");
		}
		
		else_body = p_block(0);
		
		if(!eat_punct("}")) {
			error("expected '}' or another statement");
		}
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_IF;
	stmt->start = start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->cond = cond;
	stmt->body = body;
	stmt->else_body = else_body;
	
	return stmt;
}

static Stmt *p_while()
{
	Token *start = eat_keyword(KW_while);
	
	if(!start) {
		return 0;
	}
	
	Expr *cond = p_expr();
	
	if(!cond) {
		error("expected a condition expression");
	}
	
	if(!eat_punct("{")) {
		error("expected '{'");
	}
	
	Block *body = p_block(0);
	
	if(!eat_punct("}")) {
		error("expected '}' or another statement");
	}
	
	Stmt *stmt = calloc(1, sizeof(Stmt));
	stmt->type = ST_WHILE;
	stmt->start = start;
	stmt->end = cur;
	stmt->scope = cur_scope;
	stmt->cond = cond;
	stmt->body = body;
	
	return stmt;
}

static Stmt *p_stmt()
{
	Stmt *stmt;
	
	(stmt = p_vardecl()) ||
	(stmt = p_print()) ||
	(stmt = p_funcdecl()) ||
	(stmt = p_return()) ||
	(stmt = p_if()) ||
	(stmt = p_while()) ||
	(stmt = p_assign_or_call()) ;
	
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

static Block *p_block(TokenList *params)
{
	Scope *scope = calloc(1, sizeof(Scope));
	scope->parent = cur_scope;
	scope->scope_id = next_scope_id ++;
	cur_scope = scope;
	
	if(params) {
		for(TokenItem *item = params->first_item; item; item = item->next) {
			Stmt *stmt = calloc(1, sizeof(Stmt));
			stmt->type = ST_VARDECL;
			stmt->start = item->token;
			stmt->end = stmt->start + 1;
			stmt->scope = cur_scope;
			stmt->ident = item->token;
			stmt->is_param = 1;
			
			if(!declare(stmt)) {
				error_at(
					item->token, "parameter %T already declared", item->token
				);
			}
		}
	}
	
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
	next_tmp_id = 1;
	next_scope_id = 0;
	Block *block = p_block(0);
	
	if(!eat_token(TK_EOF)) {
		error("unknown statement");
	}
	
	Module *module = calloc(1, sizeof(Module));
	module->block = block;
	return module;
}
