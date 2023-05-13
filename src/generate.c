#include <stdarg.h>
#include <stdbool.h>
#include "generate.h"

static void g_expr(Expr *expr);
static void g_stmt(Stmt *stmt);
static void g_block(Block *block);

static FILE *file = 0;
static int64_t level = 0;
static Decl *cur_funcdecl = 0;

static void write(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	
	while(*msg) {
		if(*msg == '%') {
			msg ++;
			
			if(*msg == 'i') {
				fprintf(file, "%li", va_arg(args, int64_t));
			}
			else if(*msg == 'c') {
				fprintf(file, "%c", va_arg(args, int));
			}
			else if(*msg == 's') {
				fprintf(file, "%s", va_arg(args, char*));
			}
			else if(*msg == 'T') {
				Token *token = va_arg(args, Token*);
				fwrite(token->start, 1, token->length, file);
			}
			else if(*msg == 'F') {
				Decl *func = va_arg(args, Decl*);
				fprintf(file, "func%li_%s", func->func_id, func->ident->text);
			}
			else if(*msg == 'V') {
				Decl *vardecl = va_arg(args, Decl*);
				
				write(
					"scope%i.m_%T", vardecl->scope->scope_id, vardecl->ident
				);
			}
			else if(*msg == 'E') {
				g_expr(va_arg(args, Expr*));
			}
			else if(*msg == '%') {
				fputc('%', file);
			}
			else if(*msg == '>') {
				for(int64_t i=0; i<level; i++) {
					fputc('\t', file);
				}
			}
			else if(*msg == 0) {
				break;
			}
			else {
				fputc(*msg, file);
			}
			
			msg ++;
		}
		else {
			fputc(*msg, file);
			msg ++;
		}
	}
	
	va_end(args);
}

static void g_tmpvar(Expr *expr)
{
	write("scope%i.tmp%i", expr->scope->scope_id, expr->tmp_id);
}

static bool is_var_used_in_func(Decl *decl)
{
	if(cur_funcdecl == 0) {
		return false;
	}
	
	for(DeclItem *item = cur_funcdecl->used_vars; item; item = item->next) {
		if(decl == item->decl) {
			return true;
		}
	}
	
	return false;
}

static void g_var(Expr *var)
{
	if(is_var_used_in_func(var->decl)) {
		write(
			"(*check_var(%i, &%V, \"%T\"))",
			var->start->line, var->decl, var->ident
		);
	}
	else {
		write("%V", var->decl);
	}
}

static void g_array(Expr *array)
{
	write("NEW_ARRAY(%i", array->length);
	
	for(Expr *item = array->items; item; item = item->next) {
		write(", %E", item);
	}
	
	write(")");
}

static void g_call(Expr *call)
{
	write("call(%i, %E, %i", call->start->line, call->callee, call->argcount);
	
	for(Expr *arg = call->args; arg; arg = arg->next) {
		write(", %E", arg);
	}
	
	write(")");
}

static void g_const_init_expr(Expr *expr)
{
	switch(expr->type) {
		case EX_NULL:
			write("NULL_VALUE_INIT");
			break;
		case EX_BOOL:
			write("BOOL_VALUE_INIT(%i)", !!expr->value);
			break;
		case EX_INT:
			write("INT_VALUE_INIT(%i)", expr->value);
			break;
		case EX_STRING:
			write("STRING_VALUE_INIT(\"%s\")", expr->string);
			break;
		default:
			write("/* g_const_init_expr default */");
			break;
	}
}

static void g_binop(Expr *expr)
{
	if(expr->oplevel == OP_CMP) {
		write(
			"BINOP(%i, TY_BOOL, %E, %T, %E)",
			expr->start->line, expr->left, expr->op, expr->right
		);
	}
	else {
		write(
			"BINOP(%i, TY_INT, %E, %T, %E)",
			expr->start->line, expr->left, expr->op, expr->right
		);
	}
}

static void g_expr_immed(Expr *expr)
{
	switch(expr->type) {
		case EX_NULL:
			write("NULL_VALUE");
			break;
		case EX_BOOL:
			write("BOOL_VALUE(%i)", !!expr->value);
			break;
		case EX_INT:
			write("INT_VALUE(%i)", expr->value);
			break;
		case EX_STRING:
			write("STRING_VALUE(\"%s\")", expr->string);
			break;
		case EX_VAR:
			g_var(expr);
			break;
		case EX_BINOP:
			g_binop(expr);
			break;
		case EX_CALL:
			g_call(expr);
			break;
		case EX_ARRAY:
			g_array(expr);
			break;
		case EX_SUBSCRIPT:
			write(
				"(*subscript(%i, %E, %E))",
				expr->start->line, expr->array, expr->index
			);
			
			break;
		case EX_UNARY:
			write(
				"INT_UNARY(%i, %T, %E)",
				expr->start->line, expr->op, expr->subexpr
			);
			
			break;
	}
}

static void g_expr(Expr *expr)
{
	if(expr->tmp_id > 0) {
		g_tmpvar(expr);
	}
	else {
		g_expr_immed(expr);
	}
}

static void g_scope(Scope *scope)
{
	if(scope->decl_count == 0 && scope->tmp_count == 0) {
		return;
	}
	
	write("%>struct {\n");
	level ++;
	
	for(Decl *decl = scope->decls; decl; decl = decl->next) {
		write("%>Value m_%s;\n", decl->ident->text);
	}
	
	for(int64_t i=0; i < scope->tmp_count; i++) {
		write("%>Value tmp%i;\n", i+1);
	}
	
	level --;
	write("%>} scope%i = {\n", scope->scope_id);
	level ++;
	
	for(Decl *decl = scope->decls; decl; decl = decl->next) {
		write("%>");
		
		if(decl->init_deferred) {
			write("UNINITIALIZED");
		}
		else if(!decl->isfunc) {
			if(decl->is_param) {
				write("UNINITIALIZED");
			}
			else if(decl->init) {
				g_const_init_expr(decl->init);
			}
			else {
				write("NULL_VALUE_INIT");
			}
		}
		
		write(",\n");
	}
	
	for(int64_t i=0; i < scope->tmp_count; i++) {
		write("%>UNINITIALIZED,\n");
	}
	
	level --;
	write("%>};\n");
	
	for(Decl *decl = scope->decls; decl; decl = decl->next) {
		if(!decl->isfunc && decl->is_param) {
			write("%>%V = va_arg(args, Value);\n", decl);
		}
	}
}

static void walk_expr(
	Expr *expr, bool (*previsitor)(Expr*), bool (*postvisitor)(Expr*)
) {
	if(!previsitor(expr)) {
		return;
	}
	
	if(expr->type == EX_BINOP) {
		walk_expr(expr->left, previsitor, postvisitor);
		walk_expr(expr->right, previsitor, postvisitor);
	}
	else if(expr->type == EX_CALL) {
		walk_expr(expr->callee, previsitor, postvisitor);
		
		for(Expr *arg = expr->args; arg; arg = arg->next) {
			walk_expr(arg, previsitor, postvisitor);
		}
	}
	else if(expr->type == EX_ARRAY) {
		for(Expr *item = expr->items; item; item = item->next) {
			walk_expr(item, previsitor, postvisitor);
		}
	}
	else if(expr->type == EX_SUBSCRIPT) {
		walk_expr(expr->array, previsitor, postvisitor);
		walk_expr(expr->index, previsitor, postvisitor);
	}
	else if(expr->type == EX_UNARY) {
		walk_expr(expr->subexpr, previsitor, postvisitor);
	}
	
	postvisitor(expr);
}

static bool tmp_assign_previsitor(Expr *expr)
{
	return expr->has_tmps;
}

static bool tmp_assign_postvisitor(Expr *expr)
{
	if(expr->tmp_id > 0) {
		write("%>");
		g_tmpvar(expr);
		write(" = ");
		g_expr_immed(expr);
		write(";\n");
	}
}

static void g_tmp_assigns(Expr *expr)
{
	walk_expr(expr, tmp_assign_previsitor, tmp_assign_postvisitor);
}

static bool tmp_clear_postvisitor(Expr *expr)
{
	if(expr->tmp_id > 0) {
		write("%>");
		g_tmpvar(expr);
		write(".type = TYX_UNINITIALIZED;\n");
	}
}

static void g_tmp_clears(Expr *expr)
{
	walk_expr(expr, tmp_assign_previsitor, tmp_clear_postvisitor);
}

static void g_vardecl(Decl *decl)
{
	if(decl->init_deferred) {
		Expr *init = decl->init;
		
		if(!init) {
			init = &(Expr){.type = EX_NULL};
		}
		
		g_tmp_assigns(init);
		write("%>%V = %E;\n", decl, init);
		g_tmp_clears(init);
	}
}

static void g_assign(Stmt *assign)
{
	g_tmp_assigns(assign->target);
	g_tmp_assigns(assign->value);
	write("%>%E = %E;\n", assign->target, assign->value);
	g_tmp_clears(assign->target);
	g_tmp_clears(assign->value);
}

static void g_print(Stmt *print)
{
	int64_t num = 0;
	
	for(Expr *value = print->values; value; value = value->next) {
		g_tmp_assigns(value);
		num ++;
	}
	
	write("%>print(%i", num);
	
	for(Expr *value = print->values; value; value = value->next) {
		write(",\n%>\t%E", value);
	}
	
	write("\n%>);\n");
	
	for(Expr *value = print->values; value; value = value->next) {
		g_tmp_clears(value);
	}
}

static void g_funcdecl(Decl *decl)
{
	if(decl->init_deferred) {
		write(
			"%>%V = NEW_FUNCTION(%F, %i);\n",
			decl, decl, decl->params->length
		);
	}
}

static void g_callstmt(Stmt *stmt)
{
	g_tmp_assigns(stmt->call);
	write("%>%E;\n", stmt->call);
	g_tmp_clears(stmt->call);
}

static void g_return(Stmt *stmt)
{
	if(stmt->value) {
		g_tmp_assigns(stmt->value);
	}
	
	if(stmt->value) {
		write("%>RETURN(%E);\n", stmt->value);
	}
	else {
		write("%>RETURN(NULL_VALUE);\n");
	}
}

static void g_if(Stmt *ifstmt)
{
	g_tmp_assigns(ifstmt->cond);
	write("%>if(truthy(%E)) {\n", ifstmt->cond);
	
	level ++;
	g_tmp_clears(ifstmt->cond);
	level --;
	
	g_block(ifstmt->body);
	write("%>}\n");
	write("%>else {\n");
	
	if(ifstmt->else_body) {
		g_block(ifstmt->else_body);
	}
	else {
		level ++;
		g_tmp_clears(ifstmt->cond);
		level --;
	}
	
	write("%>}\n");
}

static void g_while(Stmt *whilestmt)
{
	g_tmp_assigns(whilestmt->cond);
	write("%>while(truthy(%E)) {\n", whilestmt->cond);
	
	level ++;
	g_tmp_clears(whilestmt->cond);
	level --;
	
	g_block(whilestmt->body);
	level ++;
	g_tmp_assigns(whilestmt->cond);
	level --;
	write("%>}\n");
	g_tmp_clears(whilestmt->cond);
}

static void g_stmt(Stmt *stmt)
{
	switch(stmt->type) {
		case ST_VARDECL:
			g_vardecl(stmt->decl);
			break;
		case ST_ASSIGN:
			g_assign(stmt);
			break;
		case ST_PRINT:
			g_print(stmt);
			break;
		case ST_FUNCDECL:
			g_funcdecl(stmt->decl);
			break;
		case ST_CALL:
			g_callstmt(stmt);
			break;
		case ST_RETURN:
			g_return(stmt);
			break;
		case ST_IF:
			g_if(stmt);
			break;
		case ST_WHILE:
			g_while(stmt);
			break;
	}
}

static void g_block(Block *block)
{
	level ++;
	
	if(block->scope->parent) {
		g_scope(block->scope);
	}
	
	Decl *hosting_func = block->scope->hosting_func;
	char *func_name = 0;
	
	if(hosting_func && block == hosting_func->body) {
		func_name = hosting_func->ident->text;
	}
	else if(block->scope->parent == 0) {
		func_name = "<main>";
	}
	
	if(block->scope->decl_count > 0 || block->scope->tmp_count > 0) {
		write("%>PUSH_SCOPE(scope%i, ", block->scope->scope_id);
	}
	else {
		write("%>PUSH_EMPTY_SCOPE(");
	}
	
	if(func_name) {
		write("\"%s\");\n", func_name);
	}
	else {
		write("0);\n");
	}
	
	if(hosting_func && block == hosting_func->body) {
		write("%>cur_scope_frame->funcframe = cur_scope_frame;\n");
	}
	
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		g_stmt(stmt);
	}
	
	write("%>POP_SCOPE();\n");
	level --;
}

static void g_funcproto(Decl *funcdecl)
{
	write("Value %F(va_list args);\n", funcdecl);
}

static void g_funcprotos(Block *block)
{
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		if(stmt->type == ST_FUNCDECL) {
			g_funcproto(stmt->decl);
			g_funcprotos(stmt->decl->body);
		}
		else if(stmt->type == ST_IF) {
			g_funcprotos(stmt->body);
		}
	}
}

static void g_funcimpl(Decl *funcdecl)
{
	cur_funcdecl = funcdecl;
	write("Value %F(va_list args) {\n", funcdecl);
	g_block(funcdecl->body);
	write("\t""return NULL_VALUE;\n");
	write("}\n");
	cur_funcdecl = 0;
}

static void g_funcimpls(Block *block)
{
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		if(stmt->type == ST_FUNCDECL) {
			g_funcimpl(stmt->decl);
			g_funcimpls(stmt->decl->body);
		}
		else if(stmt->type == ST_IF) {
			g_funcimpls(stmt->body);
		}
	}
}

void generate(Module *module)
{
	file = fopen(module->cfilename, "w");
	level = 0;
	write("#include \"runtime.h\"\n");
	write("// function prototypes:\n");
	g_funcprotos(module->body);
	write("// global scope:\n");
	g_scope(module->body->scope);
	write("// function implementations:\n");
	g_funcimpls(module->body);
	write("// main function:\n");
	write("int main(int argc, char **argv) {\n");
	g_block(module->body);
	write("\t""return 0;\n");
	write("}\n");
	fclose(file);
}
