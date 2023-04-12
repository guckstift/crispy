#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "analyze.h"

static Stmt *first_decl = 0;
static Stmt *last_decl = 0;

static void error(int line, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	
	fprintf(stderr, "error(%i): ", line);
	vfprintf(stderr, msg, args);
	fprintf(stderr, "\n");
	
	exit(EXIT_FAILURE);
}

static Stmt *lookup(Token *ident)
{
	for(Stmt *decl = first_decl; decl; decl = decl->next_decl) {
		if(strcmp(decl->ident->text, ident->text) == 0) {
			return decl;
		}
	}
	
	return 0;
}

static void a_expr(Expr *expr)
{
	if(expr->type == EX_VAR) {
		if(lookup(expr->ident) == 0) {
			error(
				expr->ident->line,
				"could not find variable %s",
				expr->ident->text
			);
		}
	}
}

static void a_vardecl(Stmt *vardecl)
{
	if(lookup(vardecl->ident)) {
		error(
			vardecl->ident->line,
			"variable %s already declared",
			vardecl->ident->text
		);
	}
	
	if(vardecl->init) {
		a_expr(vardecl->init);
	}
	
	if(first_decl) {
		last_decl->next_decl = vardecl;
	}
	else {
		first_decl = vardecl;
	}
	
	last_decl = vardecl;
}

static void a_assign(Stmt *assign)
{
	if(lookup(assign->ident) == 0) {
		error(
			assign->ident->line,
			"could not find variable %s",
			assign->ident->text
		);
	}
	
	a_expr(assign->value);
}

static void a_print(Stmt *print)
{
	a_expr(print->value);
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
	}
}

static void a_stmts(Stmt *stmts)
{
	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		a_stmt(stmt);
	}
}

void analyze(Module *module)
{
	first_decl = 0;
	last_decl = 0;
	a_stmts(module->stmts);
}
