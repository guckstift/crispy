#include <stdarg.h>
#include <stdbool.h>
#include "generate.h"

static void g_expr(Expr *expr, bool in_decl_init);
static void g_stmt(Stmt *stmt);

static FILE *file = 0;
static int64_t level = 0;

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
			else if(*msg == 's') {
				fprintf(file, "%s", va_arg(args, char*));
			}
			else if(*msg == 'T') {
				fprintf(file, "%s", va_arg(args, Token*)->text);
			}
			else if(*msg == 'F') {
				Stmt *func = va_arg(args, Stmt*);
				fprintf(file, "func%li_%s", func->func_id, func->ident->text);
			}
			else if(*msg == 'V') {
				Stmt *vardecl = va_arg(args, Stmt*);
				write("scope%i.%T", vardecl->scope->scope_id, vardecl->ident);
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
	write("tmp%i", expr->tmp_id);
}

static void g_array(Expr *array)
{
	write("ARRAY_VALUE(new_array(%i", array->length);
	
	for(Expr *item = array->items; item; item = item->next) {
		write(", ");
		g_expr(item, false);
	}
	
	write("))");
}

static void g_call(Expr *call)
{
	write("call(");
	g_expr(call->callee, false);
	write(")");
}

static void g_expr(Expr *expr, bool in_decl_init)
{
	char *value_init_postfix = in_decl_init ? "_INIT" : "";
	
	switch(expr->type) {
		case EX_NULL:
			write("NULL_VALUE%s", value_init_postfix);
			break;
		case EX_BOOL:
			write("BOOL_VALUE%s(%i)", value_init_postfix, !!expr->value);
			break;
		case EX_INT:
			write("INT_VALUE%s(%i)", value_init_postfix, expr->value);
			break;
		case EX_STRING:
			write("STRING_VALUE%s(\"%s\")", value_init_postfix, expr->string);
			break;
		case EX_VAR:
			write("%V", expr->decl);
			break;
		case EX_BINOP:
			write("INT_BINOP(");
			g_expr(expr->left, false);
			write(", %c, ", expr->op);
			g_expr(expr->right, false);
			write(")");
			break;
		case EX_CALL:
			g_tmpvar(expr);
			break;
		case EX_ARRAY:
			g_array(expr);
			break;
		case EX_SUBSCRIPT:
			write("*subscript(");
			g_expr(expr->array, false);
			write(", ");
			g_expr(expr->index, false);
			write(")");
			break;
	}
}

static void g_scope(Scope *scope)
{
	if(scope->decl_count == 0) {
		return;
	}
	
	write("%>struct {\n");
	
	for(Stmt *decl = scope->first_decl; decl; decl = decl->next_decl) {
		write("\t%>Value %s;\n", decl->ident->text);
	}
	
	write("%>} scope%i = {\n", scope->scope_id);
	
	for(Stmt *decl = scope->first_decl; decl; decl = decl->next_decl) {
		write("\t%>");
		
		if(decl->type == ST_VARDECL) {
			if(decl->init_deferred) {
				write("UNINITIALIZED");
			}
			else if(decl->init) {
				g_expr(decl->init, true);
			}
			else {
				write("NULL_VALUE_INIT");
			}
		}
		else if(decl->type == ST_FUNCDECL) {
			if(decl->init_deferred) {
				write("UNINITIALIZED");
			}
			else {
				write("FUNCTION_VALUE_INIT(%F)", decl);
			}
		}
		
		write(",\n");
	}
	
	write("%>};\n");
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
		g_tmp_assigns(expr->callee);
		write("%>Value ");
		g_tmpvar(expr);
		write(" = ");
		g_call(expr);
		write(";\n");
	}
	else if(expr->type == EX_ARRAY) {
		for(Expr *item = expr->items; item; item = item->next) {
			g_tmp_assigns(item);
		}
	}
	else if(expr->type == EX_SUBSCRIPT) {
		g_tmp_assigns(expr->array);
		g_tmp_assigns(expr->index);
	}
}

static void g_vardecl(Stmt *decl)
{
	if(decl->init_deferred) {
		Expr *init = decl->init;
		
		if(!init) {
			init = &(Expr){.type = EX_NULL};
		}
		
		g_tmp_assigns(init);
		write("%>%V = ", decl);
		g_expr(init, false);
		write(";\n");
	}
}

static void g_print(Stmt *print)
{
	for(Expr *value = print->values; value; value = value->next) {
		if(value != print->values) {
			write("%>printf(\" \");\n");
		}
		
		g_tmp_assigns(value);
		write("%>print(");
		g_expr(value, false);
		write(");\n");
	}
	
	write("%>printf(\"\\n\");\n");
}

static void g_funcdecl(Stmt *decl)
{
	if(decl->init_deferred) {
		write("%>%V = FUNCTION_VALUE(%F);\n", decl, decl);
	}
}

static void g_return(Stmt *stmt)
{
	if(stmt->scope->decl_count > 0) {
		write("%>cur_scope_frame = cur_scope_frame->parent;\n");
	}
	
	if(stmt->value) {
		g_tmp_assigns(stmt->value);
	}
	
	write("%>return ");
	
	if(stmt->value) {
		g_expr(stmt->value, false);
	}
	else {
		write("NULL_VALUE");
	}
	
	write(";\n");
}

static void g_stmt(Stmt *stmt)
{
	switch(stmt->type) {
		case ST_VARDECL:
			g_vardecl(stmt);
			break;
		case ST_ASSIGN:
			g_tmp_assigns(stmt->target);
			g_tmp_assigns(stmt->value);
			write("%>");
			g_expr(stmt->target, false);
			write(" = ");
			g_expr(stmt->value, false);
			write(";\n");
			break;
		case ST_PRINT:
			g_print(stmt);
			break;
		case ST_FUNCDECL:
			g_funcdecl(stmt);
			break;
		case ST_CALL:
			g_tmp_assigns(stmt->call->callee);
			write("%>");
			g_call(stmt->call);
			write(";\n");
			break;
		case ST_RETURN:
			g_return(stmt);
			break;
	}
}

static void g_block(Block *block)
{
	level ++;
	
	if(block->scope->parent) {
		g_scope(block->scope);
	}
	
	if(block->scope->decl_count > 0) {
		write(
			"%>cur_scope_frame = &(ScopeFrame){"
			"cur_scope_frame, &scope%i, %i};\n",
			block->scope->scope_id, block->scope->decl_count
		);
	}
	
	for(Stmt *stmt = block->stmts; stmt; stmt = stmt->next) {
		g_stmt(stmt);
	}
	
	if(block->scope->decl_count > 0) {
		write("%>cur_scope_frame = cur_scope_frame->parent;\n");
	}
	
	level --;
}

static void g_funcproto(Stmt *funcdecl)
{
	write("Value %F();\n", funcdecl);
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
	write("Value %F() {\n", funcdecl);
	
	for(
		DeclItem *item = funcdecl->used_vars->first_item; item;
		item = item->next
	) {
		write(
			"\t%>if(%V.type == TYX_UNINITIALIZED) "
			"error(\"variable %T used by function %T "
			"is not initialized yet\");\n",
			item->decl, item->decl->ident, funcdecl->ident
		);
	}
	
	g_block(funcdecl->body);
	write("\t""return NULL_VALUE;\n");
	write("}\n");
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

void generate(Module *module, FILE *outfile)
{
	file = outfile;
	level = 0;
	
	write("#include \"src/runtime.c\"\n");
	write("// function prototypes:\n");
	g_funcprotos(module->block->scope);
	write("// global scope:\n");
	g_scope(module->block->scope);
	write("// function implementations:\n");
	g_funcimpls(module->block->scope);
	write("// main function:\n");
	write("int main(int argc, char **argv) {\n");
	g_block(module->block);
	write("\t""return 0;\n");
	write("}\n");
}
