#include <stdio.h>
#include <stdlib.h>
#include "lex.h"
#include "parse.h"
#include "analyze.h"
#include "generate.h"
#include "print.h"
#include "array.h"

static void error(char *msg)
{
	fprintf(stderr, P_ERROR "error:" P_RESET " %s\n", msg);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if(argc < 3) {
		error("not enough arguments");
	}
	
	char *filename = argv[1];
	FILE *file = fopen(filename, "r");
	
	if(file == NULL) {
		error("could not open input file");
	}
	
	fseek(file, 0, SEEK_END);
	long filesize = ftell(file);
	rewind(file);
	
	char *src = malloc(filesize + 1);
	src[filesize] = 0;
	fread(src, 1, filesize, file);
	
	Token *tokens = lex(src, src + filesize);
	print_tokens(tokens);
	
	Module *module = parse(tokens);
	analyze(module);
	print_module(module);
	
	char *outfilename = argv[2];
	FILE *outfile = fopen(outfilename, "w");
	
	if(outfile == NULL) {
		error("could not open output file");
	}
	
	generate(module, outfile);
	
	return 0;
}
