#include "generate.h"
#include "runtime.c.res.h"

static void g_expr(Expr *expr, int in_decl_init);
static void g_tmp_assigns(Expr *expr);

static FILE *file = 0;
static int level = 0;

static void g_indent()
{
	for(int i=0; i<level; i++) {
		fprintf(file, "\t");
	}
}

static void g_ident(Token *ident)
{
	fprintf(file, "var_%s", ident->text);
}

static void g_funcname(Stmt *func)
{
	fprintf(file, "func%li_%s", func->func_id, func->ident->text);
}

static void g_initvar(Token *ident)
{
	fprintf(file, "init_%s", ident->text);
}

static void g_tmpvar(Expr *expr)
{
	fprintf(file, "tmp%li", expr->tmp_id);
}

static void g_atom(Expr *expr, int in_decl_init)
{
	int no_decl_init_const = !in_decl_init && expr->isconst;
	
	switch(expr->type) {
		case EX_NULL:
			fprintf(file, "NULL_VALUE%s", !no_decl_init_const ? "_INIT" : "");
			break;
		case EX_BOOL:
			fprintf(
				file,
				"BOOL_VALUE%s(%i)",
				!no_decl_init_const ? "_INIT" : "",
				!!expr->value
			);
			
			break;
		case EX_INT:
			fprintf(
				file,
				"INT_VALUE%s(%li)",
				!no_decl_init_const ? "_INIT" : "",
				expr->value
			);
			
			break;
		case EX_STRING:
			fprintf(
				file,
				"STRING_VALUE%s(\"%s\")",
				!no_decl_init_const ? "_INIT" : "",
				expr->string
			);
			
			break;
		case EX_VAR:
			g_ident(expr->ident);
			break;
	}
}

static void g_binop(Expr *expr, int in_decl_init)
{
	fprintf(file, "INT_BINOP(");
	g_expr(expr->left, in_decl_init);
	fprintf(file, ", %c, ", expr->op);
	g_expr(expr->right, in_decl_init);
	fprintf(file, ")");
}

static void g_callexpr(Expr *call)
{
	fprintf(file, "call(");
	g_ident(call->ident);
	fprintf(file, ", \"%s\")", call->ident->text);
}

static void g_callexpr_tmp(Expr *call)
{
	g_tmpvar(call);
}

static void g_array(Expr *array)
{
	fprintf(
		file, "ARRAY_VALUE(new_array(%li", array->length
	);
	
	for(Expr *item = array->items; item; item = item->next) {
		fprintf(file, ", ");
		g_expr(item, 0);
	}
	
	fprintf(file, "))");
}

static void g_expr(Expr *expr, int in_decl_init)
{
	switch(expr->type) {
		case EX_BINOP:
			g_binop(expr, in_decl_init);
			break;
		case EX_CALL:
			g_callexpr_tmp(expr);
			break;
		case EX_ARRAY:
			g_array(expr);
			break;
		default:
			g_atom(expr, in_decl_init);
			break;
	}
}

static void g_global_vardecl(Stmt *vardecl)
{
	fprintf(file, "Value ");
	g_ident(vardecl->ident);
	
	if(vardecl->init && vardecl->init->isconst) {
		fprintf(file, " = ");
		g_expr(vardecl->init, 1);
	}
	else if(!vardecl->init) {
		fprintf(file, " = ");
		g_expr(&(Expr){.type = EX_NULL}, 1);
	}
	
	fprintf(file, ";\n");
	
	if(vardecl->early_use) {
		fprintf(file, "int ");
		g_initvar(vardecl->ident);
		fprintf(file, " = 0;\n");
	}
}

static void g_local_vardecl(Stmt *vardecl)
{
	g_indent();
	fprintf(file, "Value ");
	g_ident(vardecl->ident);
	fprintf(file, " = ");
	
	if(vardecl->init) {
		fprintf(file, "value_incref(");
		g_expr(vardecl->init, 1);
		fprintf(file, ")");
	}
	else {
		g_expr(&(Expr){.type = EX_NULL}, 1);
	}
	
	fprintf(file, ";\n");
}

static void g_vardecl_assign(Stmt *vardecl)
{
	if(vardecl->init && !vardecl->init->isconst) {
		g_indent();
		fprintf(file, "value_assign(&");
		g_ident(vardecl->ident);
		fprintf(file, ", ");
		g_expr(vardecl->init, 0);
		fprintf(file, ");\n");
	}
	
	if(vardecl->early_use) {
		g_indent();
		g_initvar(vardecl->ident);
		fprintf(file, " = 1;\n");
	}
}

static void g_vardecl_stmt(Stmt *vardecl)
{
	g_tmp_assigns(vardecl->init);
	
	if(vardecl->scope->parent) {
		g_local_vardecl(vardecl);
	}
	else {
		g_vardecl_assign(vardecl);
	}
}

static void g_assign(Stmt *assign)
{
	g_tmp_assigns(assign->value);
	g_indent();
	fprintf(file, "value_assign(&");
	g_ident(assign->ident);
	fprintf(file, ", ");
	g_expr(assign->value, 0);
	fprintf(file, ");\n");
}

static void g_tmp_assigns(Expr *expr)
{
	if(!expr->has_side_effects) {
		return;
	}
	
	if(expr->type == EX_BINOP) {
		g_tmp_assigns(expr->left);
		g_tmp_assigns(expr->right);
	}
	else if(expr->type == EX_CALL) {
		g_indent();
		fprintf(file, "Value ");
		g_tmpvar(expr);
		fprintf(file, " = ");
		g_callexpr(expr);
		fprintf(file, ";\n");
	}
}

static void g_print(Stmt *print)
{
	for(Expr *value = print->values; value; value = value->next) {
		g_tmp_assigns(value);
	}
	
	for(Expr *value = print->values; value; value = value->next) {
		if(value != print->values) {
			g_indent();
			fprintf(file, "printf(\" \");\n");
		}
		
		g_indent();
		fprintf(file, "print(");
		g_expr(value, 0);
		fprintf(file, ");\n");
	}
	
	g_indent();
	fprintf(file, "printf(\"\\n\");\n");
}

static void g_funcdecl(Stmt *funcdecl)
{
	g_indent();
	fprintf(file, "Value ");
	g_ident(funcdecl->ident);
	fprintf(file, " = FUNCTION_VALUE_INIT(");
	g_funcname(funcdecl);
	fprintf(file, ");\n");
	
	if(funcdecl->early_use) {
		fprintf(file, "int ");
		g_initvar(funcdecl->ident);
		fprintf(file, " = 0;\n");
	}
}

static void g_funcdecl_init(Stmt *funcdecl)
{
	if(funcdecl->early_use) {
		g_indent();
		g_initvar(funcdecl->ident);
		fprintf(file, " = 1;\n");
	}
}

static void g_funcdecl_stmt(Stmt *funcdecl)
{
	if(funcdecl->scope->parent) {
		g_funcdecl(funcdecl);
	}
	else {
		g_funcdecl_init(funcdecl);
	}
}

static void g_call(Stmt *call)
{
	g_indent();
	fprintf(file, "value_decref(");
	g_callexpr(call->call);
	fprintf(file, ");\n");
}

static void g_return(Stmt *returnstmt)
{
	if(returnstmt->value) {
		g_tmp_assigns(returnstmt->value);
	}
	
	g_indent();
	fprintf(file, "return ");
	
	if(returnstmt->value) {
		g_expr(returnstmt->value, 0);
	}
	else {
		fprintf(file, "NULL_VALUE");
	}
	
	fprintf(file, ";\n");
}

static void g_stmt(Stmt *stmt)
{
	switch(stmt->type) {
		case ST_VARDECL:
			g_vardecl_stmt(stmt);
			break;
		case ST_ASSIGN:
			g_assign(stmt);
			break;
		case ST_PRINT:
			g_print(stmt);
			break;
		case ST_FUNCDECL:
			g_funcdecl_stmt(stmt);
			g_funcdecl_init(stmt);
			break;
		case ST_CALL:
			g_call(stmt);
			break;
		case ST_RETURN:
			g_return(stmt);
			break;
	}
}

static void g_block(Block *block)
{
	level ++;
	
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		g_stmt(stmt);
	}
	
	level --;
}

static void g_funcproto(Stmt *funcdecl)
{
	fprintf(file, "Value ");
	g_funcname(funcdecl);
	fprintf(file, "();\n");
}

static void g_funcprotos(Scope *scope)
{
	for(Stmt *decl = scope->first_decl; decl; decl = decl->next_decl) {
		if(decl->type == ST_FUNCDECL) {
			g_funcproto(decl);
			g_funcprotos(decl->body->scope);
		}
	}
}

static void g_funcimpl(Stmt *funcdecl)
{
	fprintf(file, "Value ");
	g_funcname(funcdecl);
	fprintf(file, "() {\n");
	
	for(
		DeclItem *item = funcdecl->used_vars->first_item; item;
		item = item->next
	) {
		fprintf(file, "\t""if(!");
		g_initvar(item->decl->ident);
		
		fprintf(
			file,
			") error(\"variable %s used by function %s"
			" is not initialized yet\");\n",
			item->decl->ident->text, funcdecl->ident->text
		);
	}
	
	g_block(funcdecl->body);
	g_indent();
	fprintf(file, "\t""return NULL_VALUE;\n");
	fprintf(file, "}\n");
}

static void g_funcimpls(Scope *scope)
{
	for(Stmt *decl = scope->first_decl; decl; decl = decl->next_decl) {
		if(decl->type == ST_FUNCDECL) {
			g_funcimpl(decl);
			g_funcimpls(decl->body->scope);
		}
	}
}

static void g_global_decls(Scope *scope)
{
	for(Stmt *decl = scope->first_decl; decl; decl = decl->next_decl) {
		if(decl->type == ST_VARDECL) {
			g_global_vardecl(decl);
		}
		else if(decl->type == ST_FUNCDECL) {
			g_funcdecl(decl);
		}
	}
}

void generate(Module *module, FILE *outfile)
{
	file = outfile;
	level = 0;
	
	fprintf(file, RUNTIME_RES "\n");
	
	g_funcprotos(module->block->scope);
	g_global_decls(module->block->scope);
	g_funcimpls(module->block->scope);
		
	fprintf(file, "int main(int argc, char **argv) {\n");
	g_block(module->block);
	fprintf(file, "\treturn 0;\n");
	fprintf(file, "}\n");
}
