#include "generate.h"

static FILE *file = 0;

static void g_ident(Token *ident)
{
	fprintf(file, "cr_%s", ident->text);
}

static void g_expr(Expr *expr)
{
	switch(expr->type) {
		case EX_NULL:
			fprintf(file, "(Value){.type = TY_NULL}");
			break;
		case EX_BOOL:
			fprintf(
				file, "(Value){.type = TY_BOOL, .value = %i}", expr->value != 0
			);
			
			break;
		case EX_INT:
			fprintf(file, "(Value){.type = TY_INT, .value = %li}", expr->value);
			break;
		case EX_VAR:
			g_ident(expr->ident);
			break;
	}
}

static void g_vardecl(Stmt *vardecl)
{
	fprintf(file, "Value ");
	g_ident(vardecl->ident);
	fprintf(file, " = ");
	
	if(vardecl->init) {
		g_expr(vardecl->init);
	}
	else {
		g_expr(&(Expr){.type = EX_NULL});
	}
	
	fprintf(file, ";");
}

static void g_assign(Stmt *assign)
{
	g_ident(assign->ident);
	fprintf(file, " = ");
	g_expr(assign->value);
	fprintf(file, ";");
}

static void g_print(Stmt *print)
{
	fprintf(file, "print(");
	g_expr(print->value);
	fprintf(file, ");");
}

static void g_stmt(Stmt *stmt)
{
	switch(stmt->type) {
		case ST_VARDECL:
			g_vardecl(stmt);
			break;
		case ST_ASSIGN:
			g_assign(stmt);
			break;
		case ST_PRINT:
			g_print(stmt);
			break;
	}
}

static void g_stmts(Stmt *stmts)
{
	for(Stmt *stmt = stmts; stmt; stmt = stmt->next) {
		fprintf(file, "\t");
		g_stmt(stmt);
		fprintf(file, "\n");
	}
}

void generate(Module *module, FILE *outfile)
{
	file = outfile;
	
	fprintf(
		file,
		"#include <stdio.h>\n"
		"#include <stdint.h>\n"
		"typedef enum {\n"
		"\tTY_NULL,\n"
		"\tTY_BOOL,\n"
		"\tTY_INT,\n"
		"} Type;\n"
		"typedef struct {\n"
		"\tType type;\n"
		"\tint64_t value;\n"
		"} Value;\n"
		"void print(Value value) {\n"
		"\tswitch(value.type) {\n"
		"\t\tcase TY_NULL:\n"
		"\t\t\tprintf(\"null\\n\");\n"
		"\t\t\tbreak;\n"
		"\t\tcase TY_BOOL:\n"
		"\t\t\tprintf(\"%%s\\n\", value.value ? \"true\" : \"false\");\n"
		"\t\t\tbreak;\n"
		"\t\tcase TY_INT:\n"
		"\t\t\tprintf(\"%%li\\n\", value.value);\n"
		"\t\t\tbreak;\n"
		"\t}\n"
		"}\n"
	);
	
	fprintf(file, "int main(int argc, char **argv) {\n");
	g_stmts(module->stmts);
	fprintf(file, "\treturn 0;\n");
	fprintf(file, "}\n");
}
