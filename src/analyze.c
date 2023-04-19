#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "analyze.h"

static void a_block(Block *block);

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

static void add_used_var(Stmt *decl)
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

static Stmt *a_ident(Token *ident)
{
	Stmt *decl = lookup(ident, cur_scope);
	
	if(!decl) {
		error(
			ident->line, "could not find %s in scope %p",
			ident->text, cur_scope
		);
	}
	else if(decl->scope == cur_scope && ident < decl->end) {
		error(ident->line, "%s is declared later", ident->text);
	}
	else if(ident < decl->end) {
		decl->early_use = 1;
		add_used_var(decl);
	}
	else if(decl->scope != cur_scope && decl->scope->parent) {
		error(
			ident->line,
			"can not use local variable %s in enclosed function",
			ident->text
		);
	}
	
	return decl;
}

static void a_expr(Expr *expr)
{
	if(expr->type == EX_VAR) {
		a_ident(expr->ident);
	}
	else if(expr->type == EX_BINOP) {
		a_expr(expr->left);
		a_expr(expr->right);
	}
	else if(expr->type == EX_CALL) {
		a_ident(expr->ident);
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
	assign->decl = a_ident(assign->ident);
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
	a_block(funcdecl->body);
	cur_funcdecl = old_funcdecl;
}

static void a_call(Stmt *call)
{
	a_ident(call->ident);
}

static void a_return(Stmt *returnstmt)
{
	a_expr(returnstmt->value);
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
	}
}

static void a_block(Block *block)
{
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
