#include <inttypes.h>
#include <stdlib.h>
#include "print.h"

static void print_block(Block *block);
static void print_expr(Expr *expr);

static int level = 0;
static FILE *outfile = 0;

void vfprint(FILE *fs, char *msg, va_list args)
{
	while(*msg) {
		if(*msg != '%') {
			fputc(*msg, fs);
		}
		else {
			msg ++;
			
			if(*msg == 'i') {
				fprintf(fs, "%" PRId64, va_arg(args, int64_t));
			}
			else if(*msg == 'c') {
				fputc(va_arg(args, int), fs);
			}
			else if(*msg == 's') {
				fprintf(fs, "%s", va_arg(args, char*));
			}
			else if(*msg == 'p') {
				void *p = va_arg(args, void*);
				
				if(p) {
					fprintf(fs, "%p", p);
				}
				else {
					fprintf(fs, "null");
				}
			}
			else if(*msg == 't') {
				Token *token = va_arg(args, Token*);
				fwrite(token->start, 1, token->length, fs);
			}
			else if(*msg == 'K') {
				fprint(fs, P_COL_KEYWORD "%s" P_RESET, va_arg(args, char*));
			}
			else if(*msg == 'I') {
				fprint(fs, P_COL_LITERAL "%i" P_RESET, va_arg(args, int64_t));
			}
			else if(*msg == 'T') {
				Token *token = va_arg(args, Token*);
				
				if(token->type == TK_KEYWORD) {
					fprint(fs, P_COL_KEYWORD "%t" P_RESET, token);
				}
				else if(token->type == TK_IDENT) {
					fprint(fs, P_COL_IDENT "%t" P_RESET, token);
				}
				else if(token->type == TK_INT || token->type == TK_STRING) {
					fprint(fs, P_COL_LITERAL "%t" P_RESET, token);
				}
				else {
					fprint(fs, "%t", token);
				}
			}
			else if(*msg == '>') {
				for(int i=0; i < level; i++) {
					fprint(fs, "\t");
				}
			}
		}
		
		msg ++;
	}
	
	vfprintf(fs, msg, args);
}

void fprint(FILE *fs, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprint(fs, msg, args);
	va_end(args);
}

void print(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprint(stdout, msg, args);
	va_end(args);
}

static int64_t dec_len(int64_t val)
{
	int64_t count = 1;
	
	while(val >= 10) {
		val /= 10;
		count ++;
	}
	
	return count;
}

static void print_error_src_line(int64_t line, char *linep, char *errpos)
{
	int64_t col = 0;
	int64_t errcol = 0;
	
	fprint(stderr, P_COL_LINENO "%i: " P_RESET, line);
	
	for(char *p = linep; *p && *p != '\n'; p ++) {
		if(*p == '\t') {
			do {
				fprint(stderr, " ");
				col ++;
			} while(col % 4 != 0);
		}
		else {
			fprint(stderr, "%c", *p);
			col ++;
		}
		
		if(p+1 == errpos) {
			errcol = col;
		}
	}
	
	fprint(stderr, "\n");
	errcol += dec_len(line) + 2;
	
	for(int64_t i=0; i < errcol; i++) {
		fprint(stderr, " ");
	}
	
	fprint(stderr, P_COL_ERROR "^" P_RESET "\n");
}

void vprint_error(
	int64_t line, char *linep, char *errpos, char *msg, va_list args
) {
	fprint(stderr, P_ERROR "error: " P_RESET);
	vfprint(stderr, msg, args);
	fprint(stderr, "\n");
	print_error_src_line(line, linep, errpos);
}

void verror_at(Token *at, char *msg, va_list args)
{
	vprint_error(at->line, at->linep, at->start, msg, args);
	exit(EXIT_FAILURE);
}

void error_at(Token *at, char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	verror_at(at, msg, args);
}

void verror_after_t(Token *t, char *msg, va_list args)
{
	vprint_error(t->line, t->linep, t->start + t->length, msg, args);
	exit(EXIT_FAILURE);
}

void print_token(Token *token)
{
	switch(token->type) {
		case TK_KEYWORD:
			print("KEYWORD ");
			break;
		case TK_IDENT:
			print("IDENT   ");
			break;
		case TK_INT:
			print("INT     ");
			break;
		case TK_STRING:
			print("STRING  ");
			break;
		case TK_PUNCT:
			print("PUNCT   ");
			break;
	}
	
	print("%T", token);
}

void print_tokens(Tokens *list)
{
	int64_t line = 0;
	int64_t space = dec_len(list->eof_line) + 2;
	
	print(P_COL_SECTION "=== tokens ===\n" P_RESET);
	
	for(Token *token = list->tokens; token->type != TK_EOF; token ++) {
		if(token->line != line) {
			line = token->line;
			print(P_COL_LINENO "%i:" P_RESET, line);
			int64_t post_space = space - dec_len(line) - 1;
			
			for(int64_t i=0; i < post_space; i++) {
				print(" ");
			}
		}
		else {
			for(int64_t i=0; i < space; i++) {
				print(" ");
			}
		}
		
		print_token(token);
		print("\n");
	}
}

static void print_callexpr(Expr *call)
{
	print_expr(call->callee);
	print("(");
	
	for(Expr *arg = call->args; arg; arg = arg->next) {
		if(arg != call->args) {
			print(", ");
		}
		
		print_expr(arg);
	}
	
	print(")");
}

static void print_array(Expr *array)
{
	print("[");
	
	for(Expr *item = array->items; item; item = item->next) {
		if(item != array->items) {
			print(", ");
		}
		
		print_expr(item);
	}
	
	print("]");
}

static void print_subscript(Expr *subscript)
{
	print_expr(subscript->array);
	print("[");
	print_expr(subscript->index);
	print("]");
}

static void print_expr(Expr *expr)
{
	switch(expr->type) {
		case EX_NULL:
			print("%K", "null");
			break;
		case EX_BOOL:
			print("%K", expr->value ? "true" : "false");
			break;
		case EX_INT:
			print("%I", expr->value);
			break;
		case EX_STRING:
			print("%T", expr->start);
			break;
		case EX_VAR:
			print("%T", expr->ident);
			break;
		case EX_BINOP:
			print_expr(expr->left);
			print(" %T ", expr->op);
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
			print("%T ", expr->op);
			print_expr(expr->subexpr);
			break;
	}
}

void fprint_expr(FILE *fs, Expr *expr)
{
	outfile = fs;
	print_expr(expr);
}

static void print_vardecl(Decl *vardecl)
{
	print("%>%K %T", "var", vardecl->ident);
	
	if(vardecl->init) {
		print(" = ");
		print_expr(vardecl->init);
	}
	
	print("\n");
}

static void print_assign(Stmt *assign)
{
	print("%>");
	print_expr(assign->target);
	print(" = ");
	print_expr(assign->value);
	print("\n");
}

static void print_print(Stmt *stmt)
{
	print("%>%K ", "print");
	
	for(Expr *value = stmt->values; value; value = value->next) {
		if(value != stmt->values) {
			print(", ");
		}
		
		print_expr(value);
	}
	
	print("\n");
}

static void print_funcdecl(Decl *funcdecl)
{
	print("%>%K %T(", "function", funcdecl->ident);
	TokenList *params = funcdecl->params;
	
	for(TokenItem *item = params->items; item; item = item->next) {
		if(item != params->items) {
			print(", ");
		}
		
		print("%T", item->token);
	}
	
	print(") {\n");
	level++;
	print_block(funcdecl->body);
	level--;
	print("%>}\n");
}

static void print_call(Stmt *call)
{
	print("%>");
	print_callexpr(call->call);
	print("\n");
}

static void print_return(Stmt *returnstmt)
{
	print("%>%K ", "return");
	
	if(returnstmt->value) {
		print_expr(returnstmt->value);
	}
	
	print("\n");
}

static void print_if(Stmt *ifstmt)
{
	print("%>%K ", "if");
	print_expr(ifstmt->cond);
	print(" {\n");
	level++;
	print_block(ifstmt->body);
	level--;
	print("%>}\n");
	
	if(ifstmt->else_body) {
		print("%>%K {\n", "else");
		level++;
		print_block(ifstmt->else_body);
		level--;
		print("%>}\n");
	}
}

static void print_while(Stmt *stmt)
{
	print("%>%K ", "while");
	print_expr(stmt->cond);
	print(" {\n");
	level++;
	print_block(stmt->body);
	level--;
	print("%>}\n");
}

static void print_stmt(Stmt *stmt)
{
	switch(stmt->type) {
		case ST_VARDECL:
			print_vardecl(stmt->decl);
			break;
		case ST_ASSIGN:
			print_assign(stmt);
			break;
		case ST_PRINT:
			print_print(stmt);
			break;
		case ST_FUNCDECL:
			print_funcdecl(stmt->decl);
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
	print("%>" P_COL_COMMENT "# scope %p", scope);
	
	if(scope->hosting_func) {
		print(", func=%T" P_COL_COMMENT, scope->hosting_func->ident);
	}
	
	if(scope->parent) {
		print(", parent=%p", scope->parent);
	}
	
	if(scope->decl_count > 0) {
		print(", symbols=");
		
		for(Decl *decl = scope->decls; decl; decl = decl->next) {
			if(decl != scope->decls) {
				print(",");
			}
			
			print("%T" P_COL_COMMENT, decl->ident);
		}
	}
	
	print(P_RESET "\n");
}

static void print_block(Block *block)
{
	print_scope(block->scope);
	
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		print_stmt(stmt);
	}
}

void print_module(Module *module)
{
	print(P_COL_SECTION "=== module AST ===\n" P_RESET);
	level = 0;
	outfile = stdout;
	print_block(module->block);
}
