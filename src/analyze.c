#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "analyze.h"

static void a_block(Block *block);
static void a_expr(Expr *expr);

static Scope *cur_scope = 0;
static Stmt *cur_funcdecl = 0;

static void error(int line, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	
	fprintf(stderr, "error(%i): ", line);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	
	exit(EXIT_FAILURE);
}

static void add_used_var_to_func(Stmt *decl)
{
	DeclList *vars = cur_funcdecl->used_vars;
	
	for(DeclItem *item = vars->first_item; item; item = item->next) {
		if(decl == item->decl) {
			return;
		}
	}
	
	DeclItem *item = calloc(1, sizeof(DeclItem));
	item->decl = decl;
	
	if(vars->first_item) {
		vars->last_item->next = item;
	}
	else {
		vars->first_item = item;
	}
	
	vars->last_item = item;
}

static void a_var(Expr *var)
{
	Token *ident = var->ident;
	var->decl = lookup(ident, cur_scope);
	
	if(!var->decl) {
		return;
	}
	
	if(
		var->decl->scope->parent &&
		cur_scope->hosting_func != var->decl->scope->hosting_func
	) {
		// using outer local var in a nested function
		var->decl = 0;
	}
	else if(ident < var->decl->end) {
		if(var->decl->scope == cur_scope) {
			var->decl = 0;
		}
		else {
			add_used_var_to_func(var->decl);
		}
	}
}

static void a_callexpr(Expr *call)
{
	a_expr(call->callee);
	
	for(Expr *arg = call->args; arg; arg = arg->next) {
		a_expr(arg);
	}
}

static void a_array(Expr *array)
{
	for(Expr *item = array->items; item; item = item->next) {
		a_expr(item);
	}
}

static void a_subscript(Expr *subscript)
{
	a_expr(subscript->array);
	a_expr(subscript->index);
}

static void a_expr(Expr *expr)
{
	switch(expr->type) {
		case EX_VAR:
			a_var(expr);
			break;
		case EX_BINOP:
			a_expr(expr->left);
			a_expr(expr->right);
			break;
		case EX_CALL:
			a_callexpr(expr);
			break;
		case EX_ARRAY:
			a_array(expr);
			break;
		case EX_SUBSCRIPT:
			a_subscript(expr);
			break;
		case EX_UNARY:
			a_expr(expr->subexpr);
			break;
	}
}

static void a_vardecl(Stmt *vardecl)
{
	if(vardecl->init) {
		a_expr(vardecl->init);
	}
}

static void a_assign(Stmt *assign)
{
	a_expr(assign->target);
	a_expr(assign->value);
}

static void a_print(Stmt *print)
{
	for(Expr *value = print->values; value; value = value->next) {
		a_expr(value);
	}
}

static void a_funcdecl(Stmt *funcdecl)
{
	Stmt *old_funcdecl = cur_funcdecl;
	cur_funcdecl = funcdecl;
	funcdecl->used_vars = calloc(1, sizeof(DeclList));
	funcdecl->body->scope->hosting_func = funcdecl;
	a_block(funcdecl->body);
	cur_funcdecl = old_funcdecl;
}

static void a_call(Stmt *call)
{
	a_expr(call->call);
}

static void a_return(Stmt *returnstmt)
{
	if(returnstmt->value) {
		a_expr(returnstmt->value);
	}
}

static void a_if(Stmt *ifstmt)
{
	a_expr(ifstmt->cond);
	a_block(ifstmt->body);
	
	if(ifstmt->else_body) {
		a_block(ifstmt->else_body);
	}
}

static void a_while(Stmt *stmt)
{
	a_expr(stmt->cond);
	a_block(stmt->body);
}

static void a_stmt(Stmt *stmt)
{
	switch(stmt->type) {
		case ST_VARDECL:
			a_vardecl(stmt);
			break;
		case ST_ASSIGN:
			a_assign(stmt);
			break;
		case ST_PRINT:
			a_print(stmt);
			break;
		case ST_FUNCDECL:
			a_funcdecl(stmt);
			break;
		case ST_CALL:
			a_call(stmt);
			break;
		case ST_RETURN:
			a_return(stmt);
			break;
		case ST_IF:
			a_if(stmt);
			break;
		case ST_WHILE:
			a_while(stmt);
			break;
	}
}

static void a_block(Block *block)
{
	if(!block->scope->hosting_func) {
		block->scope->hosting_func = cur_scope ? cur_scope->hosting_func : 0;
	}
	
	cur_scope = block->scope;
	
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		a_stmt(stmt);
	}
	
	cur_scope = cur_scope->parent;
}

void analyze(Module *module)
{
	cur_scope = 0;
	a_block(module->block);
}
