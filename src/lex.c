#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lex.h"

char *keywords[] = {
	#define F(x) #x,
	KEYWORDS(F)
	#undef F
};

static int line = 1;

static void error(char *msg)
{
	fprintf(stderr, "error(%i): %s\n", line, msg);
	exit(EXIT_FAILURE);
}

Token *lex(char *src, char *src_end)
{
	Token *tokens = 0;
	int num_tokens = 0;
	line = 1;
	
	while(src <= src_end) {
		Token token = {.type = TK_UNKNOWN};
		
		if(isalpha(*src) || *src == '_') {
			char *start = src;
			
			while(isalnum(*src) || *src == '_') {
				src++;
			}
			
			int length = src - start;
			char *text = calloc(length + 1, 1);
			memcpy(text, start, length);
			token = (Token){.type = TK_IDENT, .text = text, .line = line};
			
			for(int i=0; i < sizeof(keywords) / sizeof(char*); i++) {
				if(strcmp(keywords[i], text) == 0) {
					token = (Token){
						.type = TK_KEYWORD, .keyword = i, .line = line
					};
					
					break;
				}
			}
		}
		else if(isdigit(*src)) {
			int64_t value = 0;
			
			while(isdigit(*src)) {
				value *= 10;
				value += *src - '0';
				src ++;
			}
			
			token = (Token){.type = TK_INT, .value = value, .line = line};
		}
		else if(*src == ';' || *src == '=') {
			token = (Token){.type = TK_PUNCT, .punct = *src, .line = line};
			src ++;
		}
		else if(isspace(*src)) {
			if(*src == '\n') {
				line ++;
			}
			
			src ++;
		}
		else if(src == src_end) {
			token = (Token){.type = TK_EOF, .line = line};
			src++;
		}
		else {
			error("unknown token");
		}
		
		if(token.type != TK_UNKNOWN) {
			num_tokens ++;
			tokens = realloc(tokens, sizeof(Token) * num_tokens);
			tokens[num_tokens - 1] = token;
		}
	}
	
	return tokens;
}

void print_tokens(Token *tokens)
{
	for(Token *token = tokens; token->type != TK_EOF; token ++) {
		switch(token->type) {
			case TK_KEYWORD:
				printf(
					"%i: KEYWORD: %s\n", token->line, keywords[token->keyword]
				);
				
				break;
			case TK_IDENT:
				printf("%i: IDENT: %s\n", token->line, token->text);
				break;
			case TK_INT:
				printf("%i: INT: %li\n", token->line, token->value);
				break;
			case TK_PUNCT:
				printf("%i: PUNCT: %c\n", token->line, token->punct);
				break;
		}
	}
}
