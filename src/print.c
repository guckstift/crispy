#include <stdio.h>
#include <stdarg.h>
#include "print.h"

static void print_block(Block *block);
static void print_expr(Expr *expr);

static int level = 0;
static FILE *outfile = 0;

static void write(char *msg, ...)
{
	if(outfile == 0) outfile = stdout;
	va_list args;
	va_start(args, msg);
	vfprintf(outfile, msg, args);
	va_end(args);
}

void print_token(Token *token)
{
	switch(token->type) {
		case TK_KEYWORD:
			write("%i: KEYWORD: %s", token->line, keywords[token->keyword]);
			break;
		case TK_IDENT:
			write("%i: IDENT: %s", token->line, token->text);
			break;
		case TK_INT:
			write("%i: INT: %li", token->line, token->value);
			break;
		case TK_STRING:
			write("%i: STRING: \"%s\"", token->line, token->text);
			break;
		case TK_PUNCT:
			write("%i: PUNCT: %c", token->line, token->punct);
			break;
	}
}

void print_tokens(Token *tokens)
{
	for(Token *token = tokens; token->type != TK_EOF; token ++) {
		print_token(token);
		write("\n");
	}
}

static void print_indent()
{
	for(int i=0; i < level; i++) {
		write("\t");
	}
}

static void print_callexpr(Expr *call)
{
	print_expr(call->callee);
	write("(");
	
	for(Expr *arg = call->args; arg; arg = arg->next) {
		if(arg != call->args) {
			write(", ");
		}
		
		print_expr(arg);
	}
	
	write(")");
}

static void print_array(Expr *array)
{
	write("[");
	
	for(Expr *item = array->items; item; item = item->next) {
		if(item != array->items) {
			write(", ");
		}
		
		print_expr(item);
	}
	
	write("]");
}

static void print_subscript(Expr *subscript)
{
	print_expr(subscript->array);
	write("[");
	print_expr(subscript->index);
	write("]");
}

static void print_expr(Expr *expr)
{
	switch(expr->type) {
		case EX_NULL:
			write("null");
			break;
		case EX_BOOL:
			write("%s", expr->value ? "true" : "false");
			break;
		case EX_INT:
			write("%li", expr->value);
			break;
		case EX_STRING:
			write("\"%s\"", expr->string);
			break;
		case EX_VAR:
			write("%s", expr->ident->text);
			break;
		case EX_BINOP:
			print_expr(expr->left);
			write("%c", expr->op);
			print_expr(expr->right);
			break;
		case EX_CALL:
			print_callexpr(expr);
			break;
		case EX_ARRAY:
			print_array(expr);
			break;
		case EX_SUBSCRIPT:
			print_subscript(expr);
			break;
		case EX_UNARY:
			write("%c", expr->op);
			print_expr(expr->subexpr);
			break;
	}
}

void fprint_expr(FILE *fs, Expr *expr)
{
	outfile = fs;
	print_expr(expr);
}

static void print_vardecl(Stmt *vardecl)
{
	write("var ");
	write("%s", vardecl->ident->text);
	
	if(vardecl->init) {
		write(" = ");
		print_expr(vardecl->init);
	}
}

static void print_assign(Stmt *assign)
{
	print_expr(assign->target);
	write(" = ");
	print_expr(assign->value);
}

static void print_print(Stmt *print)
{
	write("print ");
	
	for(Expr *value = print->values; value; value = value->next) {
		if(value != print->values) {
			write(", ");
		}
		
		print_expr(value);
	}
}

static void print_funcdecl(Stmt *funcdecl)
{
	write("function ");
	write("%s", funcdecl->ident->text);
	write("(");
	TokenList *params = funcdecl->params;
	
	for(TokenItem *item = params->first_item; item; item = item->next) {
		if(item != params->first_item) {
			write(", ");
		}
		
		write("%s", item->token->text);
	}
	
	write(") {\n");
	level++;
	print_block(funcdecl->body);
	level--;
	print_indent();
	write("}");
}

static void print_call(Stmt *call)
{
	print_callexpr(call->call);
}

static void print_return(Stmt *returnstmt)
{
	write("return ");
	
	if(returnstmt->value) {
		print_expr(returnstmt->value);
	}
}

static void print_if(Stmt *ifstmt)
{
	write("if ");
	print_expr(ifstmt->cond);
	write(" {\n");
	level++;
	print_block(ifstmt->body);
	level--;
	print_indent();
	write("}");
	
	if(ifstmt->else_body) {
		write("\n");
		print_indent();
		write("else {\n");
		level++;
		print_block(ifstmt->else_body);
		level--;
		print_indent();
		write("}");
	}
}

static void print_while(Stmt *stmt)
{
	write("while ");
	print_expr(stmt->cond);
	write(" {\n");
	level++;
	print_block(stmt->body);
	level--;
	print_indent();
	write("}");
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
		case ST_IF:
			print_if(stmt);
			break;
		case ST_WHILE:
			print_while(stmt);
			break;
	}
}

void print_scope(Scope *scope)
{
	outfile = stdout;
	write("# scope(funchost:%p) %p: ", scope->hosting_func, (void*)scope);
	
	for(Stmt *decl = scope->first_decl; decl; decl = decl->next_decl) {
		write("%s ", decl->ident->text);
	}
	
	write("\n");
}

static void print_block(Block *block)
{
	print_indent();
	print_scope(block->scope);
	
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		print_indent();
		print_stmt(stmt);
		write("\n");
	}
}

void print_module(Module *module)
{
	level = 0;
	outfile = stdout;
	print_block(module->block);
}
