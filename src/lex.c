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
		Token token;
		
		if(*src == '#') {
			while(src < src_end && *src != '\n') {
				src++;
			}
			
			continue;
		}
		else if(src[0] == '/' && src[1] == '*') {
			src += 2;
			
			while(src <= src_end) {
				if(*src == '\n') {
					line ++;
				}
				else if(src[0] == '*' && src[1] == '/') {
					src += 2;
					break;
				}
				else if(src == src_end) {
					error("mult-line comment not terminated with */");
				}
				
				src++;
			}
			
			continue;
		}
		else if(isspace(*src)) {
			if(*src == '\n') {
				line ++;
			}
			
			src ++;
			continue;
		}
		else if(isalpha(*src) || *src == '_') {
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
		else if(src[0] == '0' && src[1] == 'x') {
			src += 2;
			int64_t value = 0;
			
			while(isxdigit(*src)) {
				value *= 16;
				
				if(*src >= '0' && *src <= '9') {
					value += *src - '0';
				}
				else if(*src >= 'a' && *src <= 'f') {
					value += *src - 'a' + 10;
				}
				else if(*src >= 'A' && *src <= 'F') {
					value += *src - 'A' + 10;
				}
				
				src ++;
			}
			
			token = (Token){.type = TK_INT, .value = value, .line = line};
		}
		else if(src[0] == '0' && src[1] == 'b') {
			src += 2;
			int64_t value = 0;
			
			while(*src == '0' || *src == '1') {
				value *= 2;
				value += *src - '0';
				src ++;
			}
			
			token = (Token){.type = TK_INT, .value = value, .line = line};
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
		else if(*src == '"') {
			src ++;
			char *start = src;
			
			while(*src != '"') {
				if(src == src_end) {
					error("string not terminated with \"");
				}
				else if(*src >= 0 && *src <= 0x1f) {
					error("no control characters allowed in string");
				}
				else if(*src == '\\') {
					error("no escape sequences supported");
				}
				
				src++;
			}
			
			int length = src - start;
			char *text = calloc(length + 1, 1);
			memcpy(text, start, length);
			token = (Token){.type = TK_STRING, .text = text, .line = line};
			src ++;
		}
		else if(
			*src == ';' || *src == '=' || *src == '(' || *src == ')' ||
			*src == '{' || *src == '}' || *src == '+' || *src == '-' ||
			*src == ',' || *src == '[' || *src == ']'
		) {
			token = (Token){.type = TK_PUNCT, .punct = *src, .line = line};
			src ++;
		}
		else if(src == src_end) {
			token = (Token){.type = TK_EOF, .line = line};
			src++;
		}
		else {
			error("unknown token");
		}
		
		num_tokens ++;
		tokens = realloc(tokens, sizeof(Token) * num_tokens);
		tokens[num_tokens - 1] = token;
	}
	
	return tokens;
}
