#include <assert.h>
#include "../src/array.c"
#include "../src/print.c"
#include "../src/lex.c"

int main(int argc, char *argv[])
{
	Module module = {0};
	
	module.src =
		"# a /* test program\n"
		"function foobar 123456789\"Hello\\nWorld\" =\n"
		"0xffFFffFFffFFffFF 0b1010"
	;
	
	module.srcsize = strlen(module.src);
	lex(&module);
	
	assert(array_length(module.tokens) == 8);
	
	assert(module.tokens[0].type == TK_KEYWORD);
	assert(module.tokens[0].keyword == KW_function);
	assert(module.tokens[0].line == 2);
	
	assert(module.tokens[1].type == TK_IDENT);
	assert(strcmp(module.tokens[1].id, "foobar") == 0);
	
	assert(module.tokens[2].type == TK_INT);
	assert(module.tokens[2].value == 123456789);
	
	assert(module.tokens[3].type == TK_STRING);
	assert(strcmp(module.tokens[3].text, "Hello\\nWorld") == 0);
	
	assert(module.tokens[4].type == TK_PUNCT);
	assert(module.tokens[4].punct == '=');
	
	assert(module.tokens[5].type == TK_INT);
	assert(module.tokens[5].value == -1);
	
	assert(module.tokens[6].type == TK_INT);
	assert(module.tokens[6].value == 10);
	
	assert(module.tokens[7].type == TK_EOF);
	assert(module.tokens[7].line == 3);
	
	return 0;
}
