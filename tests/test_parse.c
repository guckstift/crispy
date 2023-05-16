#include <assert.h>
#include "../src/array.c"
#include "../src/print.c"
#include "../src/lex.c"
#include "../src/parse.c"

int main(int argc, char *argv[])
{
	{
		Module module = {0};
		module.src = "var foo;";
		module.srcsize = strlen(module.src);
		
		lex(&module);
		parse(&module);
		
		Block *body = module.body;
		assert(body);
		
		Stmt *stmt = body->stmts;
		assert(stmt);
		assert(stmt->type == ST_VARDECL);
		assert(stmt->next == 0);
		
		Decl *decl = stmt->decl;
		assert(decl);
		assert(decl->ident == module.tokens + 1);
		assert(decl->init == 0);
	}
	
	{
		Module module = {0};
		module.src = "var foo = 90;";
		module.srcsize = strlen(module.src);
		
		lex(&module);
		parse(&module);
		
		Block *body = module.body;
		assert(body);
		
		Stmt *stmt = body->stmts;
		assert(stmt);
		assert(stmt->type == ST_VARDECL);
		assert(stmt->next == 0);
		
		Decl *decl = stmt->decl;
		assert(decl);
		assert(decl->ident == module.tokens + 1);
		
		Expr *init = decl->init;
		assert(init);
		assert(init->type == EX_INT);
		assert(init->value == 90);
	}
	
	return 0;
}
