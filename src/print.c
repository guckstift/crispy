#include <stdio.h>
#include "print.h"

static void print_block(Block *block);

static int level = 0;

void print_token(Token *token)
{
	switch(token->type) {
		case TK_KEYWORD:
			printf("%i: KEYWORD: %s", token->line, keywords[token->keyword]);
			break;
		case TK_IDENT:
			printf("%i: IDENT: %s", token->line, token->text);
			break;
		case TK_INT:
			printf("%i: INT: %li", token->line, token->value);
			break;
		case TK_PUNCT:
			printf("%i: PUNCT: %c", token->line, token->punct);
			break;
	}
}

void print_tokens(Token *tokens)
{
	for(Token *token = tokens; token->type != TK_EOF; token ++) {
		print_token(token);
		printf("\n");
	}
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
		case EX_BINOP:
			print_expr(expr->left);
			printf("%c", expr->op);
			print_expr(expr->right);
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

static void print_return(Stmt *returnstmt)
{
	printf("return ");
	print_expr(returnstmt->value);
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
		case ST_RETURN:
			print_return(stmt);
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
